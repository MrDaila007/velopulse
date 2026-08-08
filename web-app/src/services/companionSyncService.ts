import { CompanionSnapshotConsts, type CompanionSnapshot } from '../domain/types';
import type { BikeComputerRepository } from './bikeComputerRepository';
import { OpenMeteoClient } from './openMeteoClient';
import { PreferencesStore } from './preferencesStore';
import { weatherCityById } from './weatherCities';

export class CompanionSyncService {
  private repository: BikeComputerRepository | null = null;
  private companionSupported = false;
  private timer: ReturnType<typeof setInterval> | null = null;
  private readonly weather = new OpenMeteoClient();

  constructor(private readonly preferences = new PreferencesStore()) {}

  configure(companionSupported: boolean) {
    this.companionSupported = companionSupported;
  }

  start(repository: BikeComputerRepository) {
    this.repository = repository;
    void this.sync();
    this.timer = setInterval(() => void this.sync(), 15 * 60 * 1000);
  }

  stop() {
    if (this.timer) clearInterval(this.timer);
    this.timer = null;
    this.repository = null;
  }

  async sync(): Promise<boolean> {
    const repository = this.repository;
    if (!repository || !this.companionSupported) return false;

    const prefs = this.preferences.readCompanionPreferences();
    const nowUtc = new Date();
    let flags = 0;
    let tempCX10 = CompanionSnapshotConsts.tempInvalid;
    let popPct = CompanionSnapshotConsts.popInvalid;
    const validUntil = Math.floor((nowUtc.getTime() + 30 * 60 * 1000) / 1000);

    if (prefs.showClockOnDevice) {
      flags |= CompanionSnapshotConsts.flagTimeValid;
    }

    if (prefs.showWeatherOnDevice) {
      const city = weatherCityById(prefs.weatherCityId) ?? weatherCityById('minsk')!;
      const weather = await this.weather.fetch(city);
      if (weather) {
        flags |= CompanionSnapshotConsts.flagWeatherValid;
        tempCX10 = weather.tempCX10;
        popPct = weather.popPct;
        if (weather.rainNow) flags |= CompanionSnapshotConsts.flagRainNow;
        if (weather.rainSoon) flags |= CompanionSnapshotConsts.flagRainSoon;
        if (weather.stale) flags |= CompanionSnapshotConsts.flagStale;
      }
    }

    if (flags === 0) return false;

    const snapshot: CompanionSnapshot = {
      structVersion: 1,
      unixTime: Math.floor(nowUtc.getTime() / 1000),
      tzOffsetMin: -nowUtc.getTimezoneOffset(),
      tempCX10,
      popPct,
      flags,
      validUntil,
    };
    const result = await repository.writeCompanionSnapshot(snapshot);
    return result.ok;
  }
}
