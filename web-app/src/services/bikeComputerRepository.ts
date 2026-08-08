import {
  type AppError,
  AppErrors,
  failure,
  type Result,
  success,
} from '../core/result';
import {
  type CommandResult,
  type CompanionSnapshot,
  configEquals,
  type DeviceCommand,
  type DeviceConfig,
  type DeviceInfo,
  type DiagnosticSnapshot,
  type ErrorLogBatch,
  type Telemetry,
  type DeviceCommandId,
} from '../domain/types';
import { validateConfig } from '../domain/configValidator';
import {
  decodeCommandResult,
  decodeConfig,
  decodeDeviceInfo,
  decodeDiagnostic,
  decodeErrorLog,
  decodeTelemetry,
  encodeCommand,
  encodeCompanion,
  encodeConfig,
  ProtocolCodecError,
  SIZES,
} from '../protocol/codecs';
import { BleUuids } from '../protocol/uuids';
import type { BleLinkState, BleTransport } from '../transport/bleTransport';

type Listener<T> = (value: T) => void;

export class BikeComputerRepository {
  private connectionListeners = new Set<Listener<BleLinkState>>();
  private telemetryListeners = new Set<Listener<Telemetry>>();
  private configListeners = new Set<Listener<DeviceConfig>>();
  private rssiListeners = new Set<Listener<number>>();
  private commandResultListeners = new Set<Listener<CommandResult>>();

  private subscriptions: Array<() => void> = [];
  private telemetryAbort: AbortController | null = null;
  private rssiTimer: ReturnType<typeof setInterval> | null = null;
  private latestConfig: DeviceConfig | null = null;
  private operationTail: Promise<void> = Promise.resolve();

  constructor(private readonly transport: BleTransport) {}

  onConnection(listener: Listener<BleLinkState>) {
    this.connectionListeners.add(listener);
    return () => this.connectionListeners.delete(listener);
  }

  onTelemetry(listener: Listener<Telemetry>) {
    this.telemetryListeners.add(listener);
    return () => this.telemetryListeners.delete(listener);
  }

  onConfig(listener: Listener<DeviceConfig>) {
    this.configListeners.add(listener);
    return () => this.configListeners.delete(listener);
  }

  onRssi(listener: Listener<number>) {
    this.rssiListeners.add(listener);
    return () => this.rssiListeners.delete(listener);
  }

  noteLinkState(state: BleLinkState) {
    for (const l of this.connectionListeners) l(state);
  }

  async start(): Promise<void> {
    const configAbort = new AbortController();
    const resultAbort = new AbortController();
    void this.pumpSubscribe(BleUuids.configRead, configAbort.signal, (bytes) => {
      const config = decodeConfig(bytes);
      this.latestConfig = config;
      for (const l of this.configListeners) l(config);
    });
    void this.pumpSubscribe(BleUuids.commandResult, resultAbort.signal, (bytes) => {
      const result = decodeCommandResult(bytes);
      for (const l of this.commandResultListeners) l(result);
    });
    this.subscriptions.push(() => {
      configAbort.abort();
      resultAbort.abort();
    });
    await this.setTelemetrySubscribed(true);
    this.rssiTimer = setInterval(async () => {
      try {
        const rssi = await this.transport.readRssi();
        for (const l of this.rssiListeners) l(rssi);
      } catch {
        // auxiliary
      }
    }, 1000);
  }

  async dispose(): Promise<void> {
    if (this.rssiTimer) clearInterval(this.rssiTimer);
    this.telemetryAbort?.abort();
    for (const unsub of this.subscriptions) unsub();
    this.subscriptions = [];
  }

  private async pumpSubscribe(
    uuid: string,
    signal: AbortSignal,
    handler: (bytes: Uint8Array) => void,
  ) {
    try {
      for await (const bytes of this.transport.subscribe(uuid)) {
        if (signal.aborted) break;
        handler(bytes);
      }
    } catch {
      // subscription ended
    }
  }

  private guard<T>(body: () => Promise<T>): Promise<Result<T>> {
    return body()
      .then((value) => success(value))
      .catch((error: unknown) => {
        if (error instanceof ProtocolCodecError) {
          return failure<T>(AppErrors.malformed(error.message));
        }
        return failure<T>(AppErrors.unknown(error));
      });
  }

  private enqueue<T>(body: () => Promise<Result<T>>): Promise<Result<T>> {
    const run = this.operationTail.then(body);
    this.operationTail = run.then(() => undefined).catch(() => undefined);
    return run;
  }

  readDeviceInfo(): Promise<Result<DeviceInfo>> {
    return this.guard(async () =>
      decodeDeviceInfo(await this.transport.read(BleUuids.deviceInfo)),
    );
  }

  readConfig(): Promise<Result<DeviceConfig>> {
    return this.guard(async () => {
      const value = decodeConfig(await this.transport.read(BleUuids.configRead));
      this.latestConfig = value;
      for (const l of this.configListeners) l(value);
      return value;
    });
  }

  readTelemetry(): Promise<Result<Telemetry>> {
    return this.guard(async () => {
      const value = decodeTelemetry(await this.transport.read(BleUuids.telemetry));
      for (const l of this.telemetryListeners) l(value);
      return value;
    });
  }

  readErrorLog(): Promise<Result<ErrorLogBatch>> {
    return this.guard(async () => {
      try {
        return decodeErrorLog(await this.transport.read(BleUuids.errorLog));
      } catch {
        return { entries: [] };
      }
    });
  }

  getDiagnostic(): Promise<Result<DiagnosticSnapshot>> {
    return this.enqueue(async () => {
      const response = this.nextResult('getDiagnostic');
      try {
        await this.transport.writeWithResponse(
          BleUuids.command,
          encodeCommand({ structVersion: 1, id: 'getDiagnostic', hasToken: false, payload: [] }),
        );
        const result = await response;
        if (result.status !== 'ok') return failure<DiagnosticSnapshot>(this.resultError(result));
        if (result.payload.length !== SIZES.diagnosticPayload) {
          return failure<DiagnosticSnapshot>(
            AppErrors.malformed(
              `Diagnostic payload: ожидалось ${SIZES.diagnosticPayload} байт`,
            ),
          );
        }
        return success(decodeDiagnostic(Uint8Array.from(result.payload)));
      } catch (error) {
        return failure<DiagnosticSnapshot>(AppErrors.unknown(error));
      }
    });
  }

  writeConfig(config: DeviceConfig): Promise<Result<void>> {
    return this.enqueue(async () => {
      const validation = validateConfig(config);
      if (!validation.isValid) {
        const first = validation.issues[0];
        return failure<void>(AppErrors.validation(first.message, first.fieldId));
      }
      for (let attempt = 0; attempt <= 3; attempt++) {
        const response = this.nextResult('configWrite');
        try {
          await this.transport.writeWithResponse(BleUuids.configWrite, encodeConfig(config));
          const result = await response;
          if (result.status === 'busy' && attempt < 3) {
            await delay(1000);
            continue;
          }
          if (result.status !== 'ok') return failure<void>(this.resultError(result));
          if (this.latestConfig && !configEquals(this.latestConfig, config)) {
            await this.waitForConfig(config, 3000);
          }
          return success(undefined);
        } catch (error) {
          const reread = await this.readConfig();
          if (reread.ok && configEquals(reread.value, config)) return success(undefined);
          return failure<void>(AppErrors.unknown(error));
        }
      }
      return failure<void>({
        kind: 'busy',
        message: 'Устройство занято. Повторите операцию',
        action: 'Повторить',
      });
    });
  }

  writeCompanionSnapshot(snapshot: CompanionSnapshot): Promise<Result<void>> {
    return this.enqueue(async () => {
      try {
        await this.transport.writeWithResponse(BleUuids.companionWrite, encodeCompanion(snapshot));
        return success(undefined);
      } catch (error) {
        return failure<void>(AppErrors.unknown(error));
      }
    });
  }

  sendCommand(command: DeviceCommand): Promise<Result<CommandResult>> {
    return this.enqueue(() => this.sendCommandDirect(command));
  }

  setOdometerMeters(odometerM: number): Promise<Result<void>> {
    const payload = new Uint8Array(4);
    new DataView(payload.buffer).setUint32(0, odometerM, true);
    return this.enqueue(() =>
      this.sendDangerousCommand({
        structVersion: 1,
        id: 'setOdometer',
        hasToken: false,
        payload: Array.from(payload),
      }),
    );
  }

  rebootDevice(): Promise<Result<void>> {
    return this.enqueue(() =>
      this.sendDangerousCommand({
        structVersion: 1,
        id: 'reboot',
        hasToken: false,
        payload: [],
      }),
    );
  }

  async setTelemetrySubscribed(value: boolean): Promise<void> {
    if (!value) {
      this.telemetryAbort?.abort();
      this.telemetryAbort = null;
      return;
    }
    if (this.telemetryAbort) return;
    const abort = new AbortController();
    this.telemetryAbort = abort;
    void this.pumpSubscribe(BleUuids.telemetry, abort.signal, (bytes) => {
      const telemetry = decodeTelemetry(bytes);
      for (const l of this.telemetryListeners) l(telemetry);
    });
  }

  disconnect(): Promise<void> {
    return this.transport.disconnect();
  }

  private async sendCommandDirect(command: DeviceCommand): Promise<Result<CommandResult>> {
    for (let attempt = 0; attempt <= 3; attempt++) {
      const response = this.nextResult(command.id);
      try {
        await this.transport.writeWithResponse(BleUuids.command, encodeCommand(command));
        const result = await response;
        if (result.status === 'busy' && attempt < 3) {
          await delay(1000);
          continue;
        }
        if (result.status === 'ok' || result.status === 'needsConfirm') {
          return success(result);
        }
        return failure<CommandResult>(this.resultError(result));
      } catch (error) {
        return failure<CommandResult>(AppErrors.unknown(error));
      }
    }
    return failure<CommandResult>({
      kind: 'busy',
      message: 'Устройство занято. Повторите команду',
      action: 'Повторить',
    });
  }

  private async sendDangerousCommand(request: DeviceCommand): Promise<Result<void>> {
    const stepOne = await this.sendCommandDirect(request);
    if (!stepOne.ok) return failure(stepOne.error);
    const needs = stepOne.value;
    if (needs.status !== 'needsConfirm') {
      return needs.status === 'ok' ? success(undefined) : failure(this.resultError(needs));
    }
    const tokenBytes = new Uint8Array(4);
    new DataView(tokenBytes.buffer).setUint32(0, needs.token, true);
    const confirmPayload = [...request.payload, ...tokenBytes];
    const confirm: DeviceCommand = {
      structVersion: 1,
      id: request.id,
      hasToken: true,
      payload: confirmPayload,
    };
    const stepTwo = await this.sendCommandDirect(confirm);
    if (!stepTwo.ok) return failure(stepTwo.error);
    if (stepTwo.value.status === 'ok') return success(undefined);
    return failure(this.resultError(stepTwo.value));
  }

  private nextResult(commandId: DeviceCommandId): Promise<CommandResult> {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        cleanup();
        reject(new Error('timeout'));
      }, 3000);
      const listener = (result: CommandResult) => {
        if (result.commandId !== commandId) return;
        cleanup();
        resolve(result);
      };
      const cleanup = () => {
        clearTimeout(timeout);
        this.commandResultListeners.delete(listener);
      };
      this.commandResultListeners.add(listener);
    });
  }

  private waitForConfig(config: DeviceConfig, ms: number): Promise<void> {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('timeout')), ms);
      const listener = (value: DeviceConfig) => {
        if (configEquals(value, config)) {
          clearTimeout(timer);
          this.configListeners.delete(listener);
          resolve();
        }
      };
      this.configListeners.add(listener);
    });
  }

  private resultError(result: CommandResult): AppError {
    switch (result.status) {
      case 'range':
        return {
          kind: 'deviceRejected',
          message: `Устройство отклонило поле конфигурации (offset ${result.detail})`,
          fieldId: result.detail,
        };
      case 'notPaired':
        return AppErrors.notPaired;
      case 'storage':
        return { kind: 'storageError', message: 'Устройство не смогло сохранить настройки' };
      case 'tokenExpired':
        return { kind: 'tokenExpired', message: 'Время подтверждения истекло' };
      case 'hardware':
        return { kind: 'hardware', message: 'Устройство сообщило об аппаратной ошибке' };
      case 'notSupported':
        return { kind: 'unsupported', message: 'Функция отсутствует в этой версии прошивки' };
      default:
        return { kind: 'unknown', message: `Устройство отклонило операцию: ${result.status}` };
    }
  }
}

function delay(ms: number) {
  return new Promise((r) => setTimeout(r, ms));
}
