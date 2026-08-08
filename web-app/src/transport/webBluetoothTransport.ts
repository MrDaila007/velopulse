import { BleUuids } from '../protocol/uuids';
import {
  bikeCompOptionalServices,
  bikeCompRequestFilters,
  bytesFromDataView,
  normalizeCharacteristicUuid,
} from '../protocol/uuidUtils';
import { assertWebBluetoothAvailable } from './bleTransport';
import type { BleDiscovery, BleLinkState, BleScanResult, BleTransport } from './bleTransport';

export class WebBluetoothTransport implements BleTransport {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  private characteristics = new Map<string, BluetoothRemoteGATTCharacteristic>();
  private notifyControllers = new Map<string, AbortController>();

  get adapterState() {
    if (!navigator.bluetooth) return 'off' as const;
    return 'ready' as const;
  }

  async *scan(): AsyncIterable<BleScanResult> {
    assertWebBluetoothAvailable();
    const device = await navigator.bluetooth!.requestDevice({
      filters: bikeCompRequestFilters,
      optionalServices: bikeCompOptionalServices,
    });
    this.device = device;
    yield {
      deviceId: device.id,
      name: device.name ?? 'BikeComp',
      rssi: -60,
      bondState: 'unknown',
    };
  }

  /** Reconnect to a previously permitted device without opening the picker. */
  async tryGetPermittedDevice(deviceId: string): Promise<BluetoothDevice | null> {
    if (!navigator.bluetooth.getDevices) return null;
    const devices = await navigator.bluetooth.getDevices();
    const match = devices.find((d) => d.id === deviceId) ?? null;
    if (match) this.device = match;
    return match;
  }

  async stopScan(): Promise<void> {}

  async *connect(deviceId: string): AsyncIterable<BleLinkState> {
    if (!this.device || this.device.id !== deviceId) {
      const permitted = await this.tryGetPermittedDevice(deviceId);
      if (!permitted) {
        throw new Error('Сначала выберите устройство через сканирование');
      }
      this.device = permitted;
    }
    yield 'connecting';
    this.device.addEventListener('gattserverdisconnected', () => {
      this.server = null;
      this.characteristics.clear();
    });
    const server = await this.device.gatt?.connect();
    if (!server) throw new Error('Не удалось подключиться к GATT');
    this.server = server;
    yield 'connected';
  }

  async disconnect(): Promise<void> {
    for (const controller of this.notifyControllers.values()) {
      controller.abort();
    }
    this.notifyControllers.clear();
    if (this.server?.connected) {
      this.server.disconnect();
    }
    this.server = null;
    this.characteristics.clear();
    this.device = null;
  }

  async requestMtu(_mtu: number): Promise<number> {
    return 247;
  }

  async discoverServices(): Promise<BleDiscovery> {
    const service = await this.getService();
    const chars = await service.getCharacteristics();
    const set = new Set<string>();
    const map = new Map<string, BluetoothRemoteGATTCharacteristic>();
    for (const c of chars) {
      const uuid = normalizeCharacteristicUuid(c.uuid);
      set.add(uuid);
      map.set(uuid, c);
    }
    this.characteristics = map;
    return { characteristics: set };
  }

  async read(characteristicUuid: string): Promise<Uint8Array> {
    const characteristic = await this.getCharacteristic(characteristicUuid);
    const value = await characteristic.readValue();
    return bytesFromDataView(value);
  }

  async writeWithResponse(characteristicUuid: string, value: Uint8Array): Promise<void> {
    const characteristic = await this.getCharacteristic(characteristicUuid);
    await characteristic.writeValueWithResponse(value);
  }

  async *subscribe(characteristicUuid: string): AsyncIterable<Uint8Array> {
    const characteristic = await this.getCharacteristic(characteristicUuid);
    const controller = new AbortController();
    this.notifyControllers.set(characteristicUuid, controller);
    const queue: Uint8Array[] = [];
    let wake: (() => void) | null = null;

    const handler = (event: Event) => {
      const target = event.target as BluetoothRemoteGATTCharacteristic;
      if (!target.value) return;
      queue.push(bytesFromDataView(target.value));
      wake?.();
      wake = null;
    };
    await characteristic.startNotifications();
    characteristic.addEventListener('characteristicvaluechanged', handler);

    try {
      while (!controller.signal.aborted) {
        if (queue.length === 0) {
          await new Promise<void>((resolve, reject) => {
            if (controller.signal.aborted) {
              reject(new Error('aborted'));
              return;
            }
            wake = resolve;
            controller.signal.addEventListener('abort', () => reject(new Error('aborted')), {
              once: true,
            });
          });
        }
        while (queue.length > 0) {
          yield queue.shift()!;
        }
      }
    } finally {
      characteristic.removeEventListener('characteristicvaluechanged', handler);
      try {
        await characteristic.stopNotifications();
      } catch {
        // ignore
      }
      this.notifyControllers.delete(characteristicUuid);
    }
  }

  async readRssi(): Promise<number> {
    return -60;
  }

  get rawDevice(): BluetoothDevice | null {
    return this.device;
  }

  private async getService(): Promise<BluetoothRemoteGATTService> {
    if (!this.server?.connected) throw new Error('GATT не подключён');
    return this.server.getPrimaryService(BleUuids.service);
  }

  private async getCharacteristic(uuid: string): Promise<BluetoothRemoteGATTCharacteristic> {
    const key = normalizeCharacteristicUuid(uuid);
    const cached = this.characteristics.get(key);
    if (cached) return cached;
    const service = await this.getService();
    const characteristic = await service.getCharacteristic(uuid);
    this.characteristics.set(key, characteristic);
    return characteristic;
  }
}
