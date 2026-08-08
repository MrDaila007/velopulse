import { BleUuids } from './uuids';

/** Canonical lowercase 128-bit UUID for GATT comparisons. */
export function normalizeUuid(uuid: string): string {
  try {
    if (typeof BluetoothUUID !== 'undefined') {
      return BluetoothUUID.getService(uuid).toLowerCase();
    }
  } catch {
    // fall through
  }
  return uuid.trim().toLowerCase();
}

export function normalizeCharacteristicUuid(uuid: string): string {
  try {
    if (typeof BluetoothUUID !== 'undefined') {
      return BluetoothUUID.getCharacteristic(uuid).toLowerCase();
    }
  } catch {
    // fall through
  }
  return uuid.trim().toLowerCase();
}

export function bytesFromDataView(view: DataView): Uint8Array {
  return new Uint8Array(view.buffer.slice(view.byteOffset, view.byteOffset + view.byteLength));
}

export const bikeCompRequestFilters: BluetoothLEScanFilter[] = [
  { services: [BleUuids.service] },
  { namePrefix: 'BikeComp' },
];

export const bikeCompOptionalServices = [BleUuids.service];
