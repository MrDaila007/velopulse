export type RideState = 'idle' | 'moving' | 'paused' | 'unknown';
export type SensorState = 'ok' | 'idle' | 'stuck' | 'noSignal' | 'unknown';
export type PowerState =
  | 'active'
  | 'shortStop'
  | 'idleDisplayOff'
  | 'deepSleepPending'
  | 'bleConfig'
  | 'charging'
  | 'unknown';
export type ResetReason =
  | 'unknown'
  | 'powerOn'
  | 'pinReset'
  | 'watchdog'
  | 'softReset'
  | 'lockup'
  | 'wakeFromSleep'
  | 'brownout';
export type CommandStatus =
  | 'ok'
  | 'unknownCommand'
  | 'invalidLength'
  | 'invalidStructVersion'
  | 'range'
  | 'needsConfirm'
  | 'tokenInvalid'
  | 'tokenExpired'
  | 'notPaired'
  | 'busy'
  | 'storage'
  | 'hardware'
  | 'notSupported'
  | 'unknown';

export type DeviceCommandId =
  | 'resetTrip'
  | 'resetMaxSpeed'
  | 'forceSave'
  | 'displayOn'
  | 'displayOff'
  | 'displayTest'
  | 'sensorTestStart'
  | 'sensorTestStop'
  | 'batteryTest'
  | 'startDiagnostic'
  | 'getDiagnostic'
  | 'resetOdometer'
  | 'factoryReset'
  | 'reboot'
  | 'setBatteryCalibration'
  | 'setOdometer'
  | 'openPairingWindow'
  | 'configWrite'
  | 'unknown';

export type DisplayTestPattern = 'fill' | 'checkerboard' | 'text';
export type BondState = 'bonded' | 'none' | 'unknown';

export interface DeviceInfo {
  structVersion: number;
  protoMajor: number;
  protoMinor: number;
  hwRevision: number;
  model: string;
  fwVersion: string;
  serial: number[];
  uptimeS: number;
  resetReason: ResetReason;
  bootCount: number;
  flags: number;
}

export interface Telemetry {
  structVersion: number;
  flags: number;
  speedX100: number;
  avgSpeedX100: number;
  maxSpeedX100: number;
  tripDistanceCm: number;
  movingTimeS: number;
  odometerM: number;
  batteryMv: number;
  batteryPct: number;
  rideState: RideState;
  revolutions: number;
  lastPulseAgeMs: number;
  seq: number;
  sensorState: SensorState;
  powerState: PowerState;
  cadenceX10: number;
  cscFlags: number;
  lastCrankEventAgeMs: number;
}

export interface DeviceConfig {
  structVersion: number;
  flags: number;
  wheelCircumferenceMm: number;
  maxSpeedKmh: number;
  stopTimeoutS: number;
  displayTimeoutS: number;
  deepSleepTimeoutS: number;
  brightnessPct: number;
  pageSwitchPeriodS: number;
  enabledPagesMask: number;
  lowBatteryPct: number;
  odometerSaveIntervalM: number;
  smoothingWindow: number;
  debounceMs: number;
  activeEdge: number;
  pinnedPage: number;
  battCalScalePermille: number;
  battCalOffsetMv: number;
  pageOrder: number[];
  reservedPage: number;
  deviceName: string;
  reserved: number;
}

export interface DeviceCommand {
  structVersion: number;
  id: DeviceCommandId;
  hasToken: boolean;
  payload: number[];
}

export interface CommandResult {
  structVersion: number;
  commandId: DeviceCommandId;
  status: CommandStatus;
  detail: number;
  token: number;
  payload: number[];
}

export interface DiagnosticSnapshot {
  rawPulseCount: number;
  rejectedDebounce: number;
  rejectedOverspeed: number;
  isrOverflow: number;
  flashWriteCount: number;
  freeHeapBytes: number;
  i2cErrorCount: number;
  selftestMask: number;
}

export interface ErrorLogEntry {
  uptimeS: number;
  code: number;
  severity: number;
  detail: number;
}

export interface ErrorLogBatch {
  entries: ErrorLogEntry[];
}

export interface CompanionSnapshot {
  structVersion: number;
  unixTime: number;
  tzOffsetMin: number;
  tempCX10: number;
  popPct: number;
  flags: number;
  validUntil: number;
}

export interface CompanionPreferences {
  showClockOnDevice: boolean;
  showWeatherOnDevice: boolean;
  weatherCityId: string;
  weatherUseFahrenheit: boolean;
}

export interface WeatherReading {
  tempCX10: number;
  popPct: number;
  rainNow: boolean;
  rainSoon: boolean;
  stale: boolean;
}

export const RIDE_STATE_CODES: Record<RideState, number> = {
  idle: 0,
  moving: 1,
  paused: 2,
  unknown: 255,
};

export const SENSOR_STATE_CODES: Record<SensorState, number> = {
  ok: 0,
  idle: 1,
  stuck: 2,
  noSignal: 3,
  unknown: 255,
};

export const POWER_STATE_CODES: Record<PowerState, number> = {
  active: 0,
  shortStop: 1,
  idleDisplayOff: 2,
  deepSleepPending: 3,
  bleConfig: 4,
  charging: 5,
  unknown: 255,
};

export const RESET_REASON_CODES: Record<ResetReason, number> = {
  unknown: 0,
  powerOn: 1,
  pinReset: 2,
  watchdog: 3,
  softReset: 4,
  lockup: 5,
  wakeFromSleep: 6,
  brownout: 7,
};

export const COMMAND_STATUS_CODES: Record<CommandStatus, number> = {
  ok: 0,
  unknownCommand: 1,
  invalidLength: 2,
  invalidStructVersion: 3,
  range: 4,
  needsConfirm: 5,
  tokenInvalid: 6,
  tokenExpired: 7,
  notPaired: 8,
  busy: 9,
  storage: 10,
  hardware: 11,
  notSupported: 12,
  unknown: 255,
};

export const COMMAND_ID_CODES: Record<DeviceCommandId, number> = {
  resetTrip: 0x01,
  resetMaxSpeed: 0x02,
  forceSave: 0x03,
  displayOn: 0x04,
  displayOff: 0x05,
  displayTest: 0x06,
  sensorTestStart: 0x07,
  sensorTestStop: 0x08,
  batteryTest: 0x09,
  startDiagnostic: 0x0a,
  getDiagnostic: 0x0b,
  resetOdometer: 0x20,
  factoryReset: 0x21,
  reboot: 0x22,
  setBatteryCalibration: 0x23,
  setOdometer: 0x30,
  openPairingWindow: 0x40,
  configWrite: 0xf0,
  unknown: 0xff,
};

export function fromCode<T extends string>(
  map: Record<T, number>,
  code: number,
  fallback: T,
): T {
  for (const [key, value] of Object.entries(map) as [T, number][]) {
    if (value === code) return key;
  }
  return fallback;
}

export const defaultConfig = (): DeviceConfig => ({
  structVersion: 1,
  flags: 15,
  wheelCircumferenceMm: 2100,
  maxSpeedKmh: 100,
  stopTimeoutS: 3,
  displayTimeoutS: 60,
  deepSleepTimeoutS: 900,
  brightnessPct: 60,
  pageSwitchPeriodS: 4,
  enabledPagesMask: 31,
  lowBatteryPct: 20,
  odometerSaveIntervalM: 500,
  smoothingWindow: 3,
  debounceMs: 3,
  activeEdge: 0,
  pinnedPage: 0,
  battCalScalePermille: 1000,
  battCalOffsetMv: 0,
  pageOrder: [0, 1, 2, 3, 4],
  reservedPage: 0,
  deviceName: 'BikeComp-XXXX',
  reserved: 0,
});

export function configEquals(a: DeviceConfig, b: DeviceConfig): boolean {
  return JSON.stringify(a) === JSON.stringify(b);
}

export function deviceInfoFlags(info: DeviceInfo) {
  return {
    bonded: (info.flags & 0x08) !== 0,
    pairingWindowOpen: (info.flags & 0x10) !== 0,
    deepSleepSupported: (info.flags & 0x40) !== 0,
  };
}

export function telemetryFlags(t: Telemetry) {
  return {
    displayOn: (t.flags & 0x02) !== 0,
    charging: (t.flags & 0x04) !== 0,
    lowBattery: (t.flags & 0x10) !== 0,
    cscConnected: (t.cscFlags & 0x01) !== 0,
    cscWheelPresent: (t.cscFlags & 0x02) !== 0,
    cscCrankPresent: (t.cscFlags & 0x04) !== 0,
    cadenceValid: (t.cscFlags & 0x08) !== 0,
    cscPairing: (t.cscFlags & 0x10) !== 0,
    cscSpeedSource: (t.cscFlags & 0x20) !== 0,
  };
}

export { configFlagHelpers, ConfigFlagMask, readConfigFlags, withConfigFlag } from './configFlags';

export const CompanionSnapshotConsts = {
  size: 15,
  tempInvalid: 0x7fff,
  popInvalid: 0xff,
  flagTimeValid: 1 << 0,
  flagWeatherValid: 1 << 1,
  flagRainNow: 1 << 2,
  flagRainSoon: 1 << 3,
  flagStale: 1 << 4,
};

export function buildSafeCommand(
  id: DeviceCommandId,
  pattern?: DisplayTestPattern,
): DeviceCommand {
  let payload: number[] = [];
  if (id === 'displayTest') {
    const codes = { fill: 0, checkerboard: 1, text: 2 };
    payload = [codes[pattern ?? 'checkerboard']];
  } else if (id === 'sensorTestStart') {
    payload = [60, 0];
  }
  return { structVersion: 1, id, hasToken: false, payload };
}
