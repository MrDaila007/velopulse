import type { DeviceConfig, DeviceInfo, Telemetry } from '../domain/types';
import { BleUuids } from '../protocol/uuids';
import { normalizeCharacteristicUuid } from '../protocol/uuidUtils';
import { requiredCharacteristics } from '../protocol/uuids';
import type { BleTransport } from '../transport/bleTransport';
import { BikeComputerRepository } from './bikeComputerRepository';
import { AppErrors } from '../core/result';
import { deviceInfoFlags } from '../domain/types';

export type ConnectionPhase =
  | 'idle'
  | 'scanning'
  | 'connecting'
  | 'mtu'
  | 'discovery'
  | 'deviceInfo'
  | 'pairing'
  | 'ready'
  | 'failed'
  | 'incompatible'
  | 'pairingClosed';

export interface ConnectionState {
  phase: ConnectionPhase;
  message: string;
  deviceId: string | null;
  deviceName: string | null;
  repository: BikeComputerRepository | null;
  deviceInfo: DeviceInfo | null;
  config: DeviceConfig | null;
  telemetry: Telemetry | null;
  rssi: number | null;
  companionSupported: boolean;
  error: string | null;
  useFakeBle: boolean;
}

const initialState = (): ConnectionState => ({
  phase: 'idle',
  message: 'Не подключено',
  deviceId: null,
  deviceName: null,
  repository: null,
  deviceInfo: null,
  config: null,
  telemetry: null,
  rssi: null,
  companionSupported: false,
  error: null,
  useFakeBle: import.meta.env.VITE_FAKE_BLE === 'true',
});

type Listener = (state: ConnectionState) => void;

function formatBleError(error: unknown): string {
  if (error instanceof DOMException) {
    if (error.name === 'NotFoundError') return 'Устройство не выбрано';
    if (error.name === 'SecurityError') {
      return 'Нет доступа к Bluetooth или требуется сопряжение. Откройте окно: USB open-pairing';
    }
    if (error.name === 'NetworkError') {
      return 'GATT ошибка: устройство отключилось или отказало в доступе (нужно bonding?)';
    }
    return `${error.name}: ${error.message}`;
  }
  if (error instanceof Error) return error.message;
  return String(error);
}

function hasRequiredCharacteristics(found: Set<string>): boolean {
  for (const required of requiredCharacteristics) {
    if (!found.has(normalizeCharacteristicUuid(required))) return false;
  }
  return true;
}

export class ConnectionController {
  private state: ConnectionState = initialState();
  private listeners = new Set<Listener>();
  private transport: BleTransport | null = null;
  private unsubscribers: Array<() => void> = [];

  subscribe(listener: Listener): () => void {
    listener(this.state);
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  getState() {
    return this.state;
  }

  private set(patch: Partial<ConnectionState>) {
    this.state = { ...this.state, ...patch };
    for (const l of this.listeners) l(this.state);
  }

  setUseFakeBle(value: boolean) {
    this.set({ useFakeBle: value });
  }

  async scanAndConnect(transport: BleTransport): Promise<void> {
    this.transport = transport;
    this.set({ phase: 'scanning', message: 'Поиск устройств…', error: null });

    try {
      let selected: { deviceId: string; name: string } | null = null;
      for await (const result of transport.scan()) {
        selected = { deviceId: result.deviceId, name: result.name };
        break;
      }
      await transport.stopScan();
      if (!selected) {
        this.set({ phase: 'failed', message: 'Устройство не выбрано', error: 'Отменено' });
        return;
      }
      await this.connectTo(selected.deviceId, selected.name, transport);
    } catch (error) {
      this.set({
        phase: 'failed',
        message: 'Ошибка подключения',
        error: formatBleError(error),
      });
    }
  }

  async connectTo(deviceId: string, deviceName: string, transport: BleTransport): Promise<void> {
    this.transport = transport;
    try {
      this.set({
        phase: 'connecting',
        message: 'Подключение…',
        deviceId,
        deviceName,
        error: null,
      });

      for await (const link of transport.connect(deviceId)) {
        if (link === 'connecting') continue;
        if (link === 'disconnected') {
          this.handleDisconnect();
          return;
        }
        break;
      }

      const repository = new BikeComputerRepository(transport);

      this.set({ phase: 'mtu', message: 'Запрос MTU…' });
      const mtu = await transport.requestMtu(247);
      if (mtu < 51) {
        this.set({
          phase: 'failed',
          message: 'MTU слишком мал',
          error: `MTU=${mtu}`,
        });
        return;
      }

      this.set({ phase: 'discovery', message: 'Обнаружение сервисов…' });
      const discovery = await transport.discoverServices();
      if (!hasRequiredCharacteristics(discovery.characteristics)) {
        const missing = [...requiredCharacteristics].find(
          (uuid) => !discovery.characteristics.has(normalizeCharacteristicUuid(uuid)),
        );
        this.set({
          phase: 'failed',
          message: 'Несовместимое устройство',
          error: missing ? `Отсутствует ${missing}` : 'Не все характеристики найдены',
        });
        return;
      }
      const companionSupported = discovery.characteristics.has(
        normalizeCharacteristicUuid(BleUuids.companionWrite),
      );

      this.set({ phase: 'deviceInfo', message: 'Чтение Device Info…' });
      const infoResult = await repository.readDeviceInfo();
      if (!infoResult.ok) {
        this.set({ phase: 'failed', message: 'Ошибка чтения', error: infoResult.error.message });
        return;
      }
      const deviceInfo = infoResult.value;
      if (deviceInfo.protoMajor !== 1) {
        this.set({
          phase: 'incompatible',
          message: 'Несовместимая версия протокола',
          error: `protoMajor=${deviceInfo.protoMajor}`,
          deviceInfo,
        });
        return;
      }

      const flags = deviceInfoFlags(deviceInfo);
      if (!flags.pairingWindowOpen && !flags.bonded) {
        this.set({
          phase: 'pairingClosed',
          message: 'Окно сопряжения закрыто',
          error: `${AppErrors.pairingClosed.message}. ${AppErrors.pairingClosed.action}`,
          deviceInfo,
        });
        return;
      }

      // Pairing / encryption: read encrypted characteristics before notify subscriptions.
      this.set({ phase: 'pairing', message: 'Сопряжение и чтение конфигурации…' });
      const configResult = await repository.readConfig();
      if (!configResult.ok) {
        const hint = flags.pairingWindowOpen
          ? 'Подтвердите запрос сопряжения в системе.'
          : AppErrors.pairingClosed.action;
        this.set({
          phase: 'failed',
          message: 'Не удалось прочитать конфигурацию',
          error: `${configResult.error.message}. ${hint}`,
          deviceInfo,
        });
        return;
      }

      this.attachRepository(repository);
      await repository.start();

      const telemetryResult = await repository.readTelemetry();
      if (!telemetryResult.ok) {
        this.set({
          phase: 'failed',
          message: 'Не удалось прочитать телеметрию',
          error: telemetryResult.error.message,
          deviceInfo,
          config: configResult.value,
        });
        return;
      }

      this.set({
        phase: 'ready',
        message: 'Подключено',
        repository,
        deviceInfo,
        companionSupported,
        config: configResult.value,
        telemetry: telemetryResult.value,
        error: null,
      });
    } catch (error) {
      this.set({
        phase: 'failed',
        message: 'Ошибка подключения',
        error: formatBleError(error),
      });
    }
  }

  async disconnect(): Promise<void> {
    await this.state.repository?.dispose();
    await this.transport?.disconnect();
    this.cleanup();
    this.set({ ...initialState(), useFakeBle: this.state.useFakeBle });
  }

  private attachRepository(repository: BikeComputerRepository) {
    this.cleanup();
    this.unsubscribers.push(
      repository.onTelemetry((telemetry) => this.set({ telemetry })),
      repository.onConfig((config) => this.set({ config })),
      repository.onRssi((rssi) => this.set({ rssi })),
      repository.onConnection((link) => {
        if (link === 'disconnected') this.handleDisconnect();
      }),
    );
  }

  private handleDisconnect() {
    this.cleanup();
    this.set({
      ...initialState(),
      useFakeBle: this.state.useFakeBle,
      phase: 'idle',
      message: 'Соединение потеряно',
    });
  }

  private cleanup() {
    for (const unsub of this.unsubscribers) unsub();
    this.unsubscribers = [];
  }
}
