import { CompanionSnapshotConsts, type WeatherReading } from '../domain/types';
import type { WeatherCity } from './weatherCities';

export class OpenMeteoClient {
  private lastFetchAt: Date | null = null;
  private cached: WeatherReading | null = null;

  async fetch(city: WeatherCity): Promise<WeatherReading | null> {
    const now = new Date();
    if (
      this.cached &&
      this.lastFetchAt &&
      now.getTime() - this.lastFetchAt.getTime() < 15 * 60 * 1000
    ) {
      return this.cached;
    }

    const params = new URLSearchParams({
      latitude: String(city.latitude),
      longitude: String(city.longitude),
      current: 'temperature_2m,precipitation',
      hourly: 'precipitation_probability',
      forecast_hours: '1',
      timezone: 'GMT',
    });
    try {
      const response = await fetch(`https://api.open-meteo.com/v1/forecast?${params}`, {
        signal: AbortSignal.timeout(8000),
      });
      if (!response.ok) return this.staleOrNull();
      const json = (await response.json()) as Record<string, unknown>;
      const current = json.current as Record<string, unknown> | undefined;
      if (!current) return this.staleOrNull();
      const temp = current.temperature_2m as number | undefined;
      const precipitation = (current.precipitation as number | undefined) ?? 0;
      const hourly = json.hourly as Record<string, unknown> | undefined;
      const popList = hourly?.precipitation_probability as number[] | undefined;
      const pop =
        !popList || popList.length === 0
          ? CompanionSnapshotConsts.popInvalid
          : Math.max(0, Math.min(100, Math.round(popList[0])));

      if (temp === undefined) return this.staleOrNull();
      const reading: WeatherReading = {
        tempCX10: Math.round(temp * 10),
        popPct: pop,
        rainNow: precipitation > 0,
        rainSoon: pop >= 30,
        stale: false,
      };
      this.cached = reading;
      this.lastFetchAt = now;
      return reading;
    } catch {
      return this.staleOrNull();
    }
  }

  private staleOrNull(): WeatherReading | null {
    if (!this.cached) return null;
    return { ...this.cached, stale: true };
  }
}
