import { BleUuids } from '../protocol/uuids';
import {
  decodeCommand,
  decodeConfig,
  encodeCommandResult,
  encodeConfig,
  encodeDeviceInfo,
  encodeTelemetry,
  SIZES,
} from '../protocol/codecs';
import { validateConfig } from '../domain/configValidator';
import {
  type DeviceCommand,
  type DeviceConfig,
  type DeviceInfo,
  type Telemetry,
  defaultConfig,
  type CommandStatus,
  type DeviceCommandId,
} from '../domain/types';
import type { BleDiscovery, BleLinkState, BleScanResult, BleTransport } from './bleTransport';

export type FakeBleScenario =
  | 'normal'
  | 'serviceMissing'
  | 'mtuTooSmall'
  | 'protocolMajor2'
  | 'pairingClosed'
  | 'writeTimeout'
  | 'busyThenOk'
  | 'rangeError'
  | 'storageError'
  | 'disconnectOnWrite';

export type FakeRideProfile = 'idle' | 'moving' | 'paused' | 'lowBattery';

type NotifyHandler = (bytes: Uint8Array) => void;

export class FakeBleTransport implements BleTransport {
  scenario: FakeBleScenario = 'normal';
  private _profile: FakeRideProfile = 'moving';
  private _connected = false;
  private _sensorTest = false;
  private _busyResponses = 0;
  private _pendingDangerousToken = 0;
  private _linkListeners = new Set<(state: BleLinkState) => void>();
  private _notifyHandlers = new Map<string, Set<NotifyHandler>>();
  private _telemetryTimer: ReturnType<typeof setInterval> | null = null;
  readonly operationLog: string[] = [];

  private _config: DeviceConfig = defaultConfig();
  private _telemetry: Telemetry = {
    structVersion: 1,
    flags: 35,
    speedX100: 2550,
    avgSpeedX100: 2200,
    maxSpeedX100: 3500,
    tripDistanceCm: 125000,
    movingTimeS: 1800,
    odometerM: 123456,
    batteryMv: 3900,
    batteryPct: 75,
    rideState: 'moving',
    revolutions: 500,
    lastPulseAgeMs: 250,
    seq: 42,
    sensorState: 'ok',
    powerState: 'active',
    cadenceX10: 0,
    cscFlags: 0,
    lastCrankEventAgeMs: 0xffffffff,
  };

  constructor(profile: FakeRideProfile = 'moving') {
    this._profile = profile;
  }

  get adapterState() {
    return 'ready' as const;
  }

  get connected() {
    return this._connected;
  }

  private get deviceInfo(): DeviceInfo {
    return {
      structVersion: 1,
      protoMajor: this.scenario === 'protocolMajor2' ? 2 : 1,
      protoMinor: 2,
      hwRevision: 1,
      model: 'BIKECOMP-XIAO',
      fwVersion: '1.0.0-fake',
      serial: [1, 2, 3, 4, 5, 6, 7, 8],
      uptimeS: 3600,
      resetReason: 'powerOn',
      bootCount: 42,
      flags: this.scenario === 'pairingClosed' ? 0x0f : 0x1f,
    };
  }

  async *scan(): AsyncIterable<BleScanResult> {
    await delay(40);
    yield {
      deviceId: 'FA:KE:BI:KE:00:01',
      name: 'BikeComp-FAKE',
      rssi: -58,
      bondState: 'bonded',
    };
  }

  async stopScan(): Promise<void> {}

  async *connect(_deviceId: string): AsyncIterable<BleLinkState> {
    const queue: BleLinkState[] = [];
    let resolve: (() => void) | null = null;
    const push = (state: BleLinkState) => {
      queue.push(state);
      resolve?.();
      resolve = null;
    };
    const listener = (state: BleLinkState) => push(state);
    this._linkListeners.add(listener);

    push('connecting');
    await delay(40);
    this._connected = true;
    this._restartTelemetryTimer();
    push('connected');

    try {
      while (this._connected || queue.length > 0) {
        if (queue.length === 0) {
          await new Promise<void>((r) => {
            resolve = r;
          });
        }
        while (queue.length > 0) {
          yield queue.shift()!;
        }
      }
    } finally {
      this._linkListeners.delete(listener);
    }
  }

  async disconnect(): Promise<void> {
    this._connected = false;
    this._stopTelemetryTimer();
    for (const listener of this._linkListeners) {
      listener('disconnected');
    }
  }

  async requestMtu(mtu: number): Promise<number> {
    this.operationLog.push(`mtu:${mtu}`);
    this._requireConnected();
    return this.scenario === 'mtuTooSmall' ? 23 : 247;
  }

  async discoverServices(): Promise<BleDiscovery> {
    this.operationLog.push('discover');
    this._requireConnected();
    if (this.scenario === 'serviceMissing') {
      return { characteristics: new Set() };
    }
    return {
      characteristics: new Set([
        BleUuids.deviceInfo,
        BleUuids.telemetry,
        BleUuids.configRead,
        BleUuids.configWrite,
        BleUuids.command,
        BleUuids.commandResult,
        BleUuids.companionWrite,
        BleUuids.errorLog,
      ]),
    };
  }

  async read(characteristicUuid: string): Promise<Uint8Array> {
    this.operationLog.push(`read:${characteristicUuid}`);
    this._requireConnected();
    switch (characteristicUuid) {
      case BleUuids.deviceInfo:
        return encodeDeviceInfo(this.deviceInfo);
      case BleUuids.telemetry:
        return encodeTelemetry(this._profiledTelemetry());
      case BleUuids.configRead:
        return encodeConfig(this._config);
      case BleUuids.errorLog:
        return new Uint8Array([1, 1, 0, 0, 0, 0, 0, 0, 0, 0]);
      default:
        throw new Error(`Unsupported fake characteristic ${characteristicUuid}`);
    }
  }

  async writeWithResponse(characteristicUuid: string, value: Uint8Array): Promise<void> {
    this._requireConnected();
    if (this.scenario === 'disconnectOnWrite') {
      this._connected = false;
      for (const listener of this._linkListeners) listener('disconnected');
      throw new Error('Injected disconnect');
    }
    if (characteristicUuid === BleUuids.configWrite) {
      await this._writeConfig(value);
      return;
    }
    if (characteristicUuid === BleUuids.companionWrite) {
      this.operationLog.push(`companion:${value.length}`);
      return;
    }
    if (characteristicUuid === BleUuids.command) {
      await this._writeCommand(value);
      return;
    }
    throw new Error(`Unsupported fake write ${characteristicUuid}`);
  }

  async *subscribe(characteristicUuid: string): AsyncIterable<Uint8Array> {
    const handlers = new Set<NotifyHandler>();
    this._notifyHandlers.set(characteristicUuid, handlers);
    const queue: Uint8Array[] = [];
    let wake: (() => void) | null = null;
    const handler: NotifyHandler = (bytes) => {
      queue.push(bytes);
      wake?.();
      wake = null;
    };
    handlers.add(handler);
    try {
      while (true) {
        if (queue.length === 0) {
          await new Promise<void>((r) => {
            wake = r;
          });
        }
        while (queue.length > 0) {
          yield queue.shift()!;
        }
      }
    } finally {
      handlers.delete(handler);
    }
  }

  async readRssi(): Promise<number> {
    return -58;
  }

  private _requireConnected() {
    if (!this._connected) throw new Error('Fake BLE device is disconnected');
  }

  private _emit(uuid: string, bytes: Uint8Array) {
    const handlers = this._notifyHandlers.get(uuid);
    if (!handlers) return;
    for (const h of handlers) h(bytes);
  }

  private _emitResult(id: DeviceCommandId, status: CommandStatus, detail = 0, token = 0) {
    if (this.scenario === 'writeTimeout') return;
    this._emit(
      BleUuids.commandResult,
      encodeCommandResult({
        structVersion: 1,
        commandId: id,
        status,
        detail,
        token,
        payload: [],
      }),
    );
  }

  private async _writeConfig(bytes: Uint8Array) {
    const value = decodeConfig(bytes);
    if (this.scenario === 'busyThenOk' && this._busyResponses < 3) {
      this._busyResponses++;
      this._emitResult('configWrite', 'busy');
      return;
    }
    if (this.scenario === 'rangeError') {
      this._emitResult('configWrite', 'range', 2);
      return;
    }
    if (this.scenario === 'storageError') {
      this._emitResult('configWrite', 'storage');
      return;
    }
    const validation = validateConfig(value);
    if (!validation.isValid) {
      this._emitResult('configWrite', 'range', validation.issues[0].fieldId);
      return;
    }
    this._config = value;
    this._emit(BleUuids.configRead, encodeConfig(this._config));
    this._emitResult('configWrite', 'ok');
  }

  private _handleDangerous(command: DeviceCommand): boolean {
    const dangerous = ['resetOdometer', 'factoryReset', 'reboot', 'setOdometer', 'openPairingWindow'];
    if (!dangerous.includes(command.id)) return false;

    if (!command.hasToken) {
      this._pendingDangerousToken = (Math.random() * 0xffffffff) >>> 0;
      this._emitResult(command.id, 'needsConfirm', 0, this._pendingDangerousToken);
      return true;
    }
    const tokenOk =
      command.payload.length >= 4 &&
      new DataView(Uint8Array.from(command.payload.slice(-4)).buffer).getUint32(0, true) ===
        this._pendingDangerousToken;
    this._emitResult(command.id, tokenOk ? 'ok' : 'tokenInvalid');
    return true;
  }

  private async _writeCommand(bytes: Uint8Array) {
    const command = decodeCommand(bytes);
    if (this._handleDangerous(command)) return;

    switch (command.id) {
      case 'resetTrip':
        this._telemetry = {
          ...this._telemetry,
          speedX100: 0,
          avgSpeedX100: 0,
          maxSpeedX100: 0,
          tripDistanceCm: 0,
          movingTimeS: 0,
          revolutions: 0,
        };
        break;
      case 'displayOn':
        this._telemetry = { ...this._telemetry, flags: this._telemetry.flags | 0x02 };
        break;
      case 'displayOff':
        this._telemetry = { ...this._telemetry, flags: this._telemetry.flags & ~0x02 };
        break;
      case 'sensorTestStart':
        this._sensorTest = true;
        this._restartTelemetryTimer();
        break;
      case 'sensorTestStop':
        this._sensorTest = false;
        this._restartTelemetryTimer();
        break;
      case 'getDiagnostic': {
        const payload = new Uint8Array(SIZES.diagnosticPayload);
        const view = new DataView(payload.buffer);
        view.setUint32(0, 1000, true);
        this._emitResult('getDiagnostic', 'ok', 0, 0);
        this._emit(
          BleUuids.commandResult,
          encodeCommandResult({
            structVersion: 1,
            commandId: 'getDiagnostic',
            status: 'ok',
            detail: 0,
            token: 0,
            payload: Array.from(payload),
          }),
        );
        return;
      }
      default:
        break;
    }
    this._emitResult(command.id, 'ok');
    this._emit(BleUuids.telemetry, encodeTelemetry(this._profiledTelemetry()));
  }

  private _profiledTelemetry(): Telemetry {
    switch (this._profile) {
      case 'idle':
        return { ...this._telemetry, speedX100: 0, rideState: 'idle' };
      case 'paused':
        return { ...this._telemetry, speedX100: 0, rideState: 'paused' };
      case 'lowBattery':
        return {
          ...this._telemetry,
          batteryPct: 12,
          batteryMv: 3400,
          flags: this._telemetry.flags | 0x10,
        };
      default:
        return this._telemetry;
    }
  }

  private _restartTelemetryTimer() {
    this._stopTelemetryTimer();
    const interval = this._sensorTest ? 200 : 1000;
    this._telemetryTimer = setInterval(() => {
      if (!this._connected) return;
      this._telemetry = {
        ...this._telemetry,
        seq: (this._telemetry.seq + 1) % 65536,
      };
      this._emit(BleUuids.telemetry, encodeTelemetry(this._profiledTelemetry()));
    }, interval);
  }

  private _stopTelemetryTimer() {
    if (this._telemetryTimer) {
      clearInterval(this._telemetryTimer);
      this._telemetryTimer = null;
    }
  }
}

function delay(ms: number) {
  return new Promise((r) => setTimeout(r, ms));
}
