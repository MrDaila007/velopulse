import type { DeviceConfig } from '../../../domain/types';

export interface SettingsSectionProps {
  draft: DeviceConfig;
  fieldErrors: Record<string, string>;
  deepSleepSupported: boolean;
  onChange: (next: DeviceConfig) => void;
}

export function patchDraft(
  draft: DeviceConfig,
  patch: Partial<DeviceConfig>,
): DeviceConfig {
  return { ...draft, ...patch };
}
