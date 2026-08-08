import type { CompanionPreferences } from '../domain/types';

const LAST_DEVICE_KEY = 'bikecomp.v1.lastDevice';
const COMPANION_PREFS_KEY = 'bikecomp.v1.companionPrefs';

export interface LastDevice {
  deviceId: string;
  name: string;
}

export class PreferencesStore {
  readLastDevice(): LastDevice | null {
    const raw = localStorage.getItem(LAST_DEVICE_KEY);
    if (!raw) return null;
    try {
      return JSON.parse(raw) as LastDevice;
    } catch {
      return null;
    }
  }

  writeLastDevice(device: LastDevice): void {
    localStorage.setItem(LAST_DEVICE_KEY, JSON.stringify(device));
  }

  clearLastDevice(): void {
    localStorage.removeItem(LAST_DEVICE_KEY);
  }

  readCompanionPreferences(): CompanionPreferences {
    const raw = localStorage.getItem(COMPANION_PREFS_KEY);
    if (!raw) {
      return {
        showClockOnDevice: true,
        showWeatherOnDevice: true,
        weatherCityId: 'minsk',
        weatherUseFahrenheit: false,
      };
    }
    try {
      return JSON.parse(raw) as CompanionPreferences;
    } catch {
      return {
        showClockOnDevice: true,
        showWeatherOnDevice: true,
        weatherCityId: 'minsk',
        weatherUseFahrenheit: false,
      };
    }
  }

  writeCompanionPreferences(prefs: CompanionPreferences): void {
    localStorage.setItem(COMPANION_PREFS_KEY, JSON.stringify(prefs));
  }
}
