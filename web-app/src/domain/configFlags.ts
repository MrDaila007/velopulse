import type { DeviceConfig } from './types';

export const ConfigFlagMask = {
  smoothingEnabled: 0x01,
  autoPageSwitch: 0x02,
  displayAutoOff: 0x04,
  bleAlwaysAdvertise: 0x08,
  unitsImperial: 0x10,
  sensorInvert: 0x20,
  powerSaveMode: 0x40,
  deepSleepEnabled: 0x80,
} as const;

export type ConfigFlagKey = keyof typeof ConfigFlagMask;

export function readConfigFlags(flags: number) {
  return {
    smoothingEnabled: (flags & ConfigFlagMask.smoothingEnabled) !== 0,
    autoPageSwitch: (flags & ConfigFlagMask.autoPageSwitch) !== 0,
    displayAutoOff: (flags & ConfigFlagMask.displayAutoOff) !== 0,
    bleAlwaysAdvertise: (flags & ConfigFlagMask.bleAlwaysAdvertise) !== 0,
    unitsImperial: (flags & ConfigFlagMask.unitsImperial) !== 0,
    sensorInvert: (flags & ConfigFlagMask.sensorInvert) !== 0,
    powerSaveMode: (flags & ConfigFlagMask.powerSaveMode) !== 0,
    deepSleepEnabled: (flags & ConfigFlagMask.deepSleepEnabled) !== 0,
  };
}

export function withConfigFlag(config: DeviceConfig, key: ConfigFlagKey, enabled: boolean): DeviceConfig {
  const mask = ConfigFlagMask[key];
  return {
    ...config,
    flags: enabled ? config.flags | mask : config.flags & ~mask,
  };
}

export function configFlagHelpers(config: DeviceConfig) {
  const flags = readConfigFlags(config.flags);
  return {
    ...flags,
    withFlag: (key: ConfigFlagKey, enabled: boolean) => withConfigFlag(config, key, enabled),
  };
}
