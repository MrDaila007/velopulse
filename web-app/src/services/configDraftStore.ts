import type { DeviceConfig } from '../domain/types';
import { configEquals } from '../domain/types';

const draftKey = (deviceId: string) => `bikecomp.v1.configDraft.${deviceId}`;

export interface StoredDraft {
  config: DeviceConfig;
  savedAt: string;
}

export class ConfigDraftStore {
  read(deviceId: string): StoredDraft | null {
    const raw = localStorage.getItem(draftKey(deviceId));
    if (!raw) return null;
    try {
      return JSON.parse(raw) as StoredDraft;
    } catch {
      return null;
    }
  }

  write(deviceId: string, config: DeviceConfig): void {
    const value: StoredDraft = { config, savedAt: new Date().toISOString() };
    localStorage.setItem(draftKey(deviceId), JSON.stringify(value));
  }

  clear(deviceId: string): void {
    localStorage.removeItem(draftKey(deviceId));
  }

  hasLocalChanges(draft: DeviceConfig, device: DeviceConfig | null): boolean {
    if (!device) return true;
    return !configEquals(draft, device);
  }
}
