import type { DeviceConfig } from '../domain/types';

const BACKUP_KEY = 'bikecomp.v1.firmware_migration.backup';

export interface FirmwareBackup {
  deviceId: string;
  deviceName: string;
  config: DeviceConfig;
  odometerM: number;
  fwVersion: string;
  savedAt: string;
}

export class FirmwareMigrationStore {
  readBackup(): FirmwareBackup | null {
    const raw = localStorage.getItem(BACKUP_KEY);
    if (!raw) return null;
    try {
      return JSON.parse(raw) as FirmwareBackup;
    } catch {
      return null;
    }
  }

  writeBackup(backup: FirmwareBackup): void {
    localStorage.setItem(BACKUP_KEY, JSON.stringify(backup));
  }

  clearBackup(): void {
    localStorage.removeItem(BACKUP_KEY);
  }
}
