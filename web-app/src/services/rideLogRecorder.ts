import type { DeviceConfig, DeviceInfo, Telemetry } from '../domain/types';
import type { DiagnosticSnapshot } from '../domain/types';
import type { ErrorLogBatch } from '../domain/types';

export interface RideLogSample {
  at: string;
  telemetry: Telemetry;
}

export interface SessionLog {
  schema: 1;
  exportedAt: string;
  device: {
    id: string | null;
    name: string | null;
    info: DeviceInfo | null;
    config: DeviceConfig | null;
  };
  diagnostic: DiagnosticSnapshot | null;
  errorLog: ErrorLogBatch | null;
  samples: RideLogSample[];
}

export class RideLogRecorder {
  private samples: RideLogSample[] = [];
  private maxSamples = 3600;

  reset() {
    this.samples = [];
  }

  record(telemetry: Telemetry) {
    if (this.samples.length >= this.maxSamples) {
      this.samples.shift();
    }
    this.samples.push({ at: new Date().toISOString(), telemetry });
  }

  getSamples() {
    return [...this.samples];
  }
}

export function buildSessionLog(input: {
  deviceId: string | null;
  deviceName: string | null;
  deviceInfo: DeviceInfo | null;
  config: DeviceConfig | null;
  diagnostic: DiagnosticSnapshot | null;
  errorLog: ErrorLogBatch | null;
  samples: RideLogSample[];
}): SessionLog {
  return {
    schema: 1,
    exportedAt: new Date().toISOString(),
    device: {
      id: input.deviceId,
      name: input.deviceName,
      info: input.deviceInfo,
      config: input.config,
    },
    diagnostic: input.diagnostic,
    errorLog: input.errorLog,
    samples: input.samples,
  };
}

export function downloadJson(filename: string, data: unknown) {
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = filename;
  anchor.click();
  URL.revokeObjectURL(url);
}
