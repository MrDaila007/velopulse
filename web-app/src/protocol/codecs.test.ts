import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, expect, it } from 'vitest';
import {
  bytesToHex,
  decodeCommand,
  decodeCommandResult,
  decodeCompanion,
  decodeConfig,
  decodeDeviceInfo,
  decodeTelemetry,
  encodeCommand,
  encodeCommandResult,
  encodeCompanion,
  encodeConfig,
  encodeDeviceInfo,
  encodeTelemetry,
  hexToBytes,
} from './codecs';
import type {
  CommandResult,
  CompanionSnapshot,
  DeviceCommand,
  DeviceConfig,
  DeviceInfo,
  Telemetry,
} from '../domain/types';
import { fromCode, RIDE_STATE_CODES, SENSOR_STATE_CODES, POWER_STATE_CODES, RESET_REASON_CODES, COMMAND_ID_CODES, COMMAND_STATUS_CODES } from '../domain/types';

const fixturesDir = join(dirname(fileURLToPath(import.meta.url)), '../../../protocol/fixtures');

function readHex(name: string): Uint8Array {
  const text = readFileSync(join(fixturesDir, name), 'utf8').trim();
  return hexToBytes(text);
}

function readJson<T>(name: string): T {
  return JSON.parse(readFileSync(join(fixturesDir, name), 'utf8')) as T;
}

function deviceInfoFromJson(j: Record<string, unknown>): DeviceInfo {
  return {
    structVersion: j.struct_version as number,
    protoMajor: j.proto_major as number,
    protoMinor: j.proto_minor as number,
    hwRevision: j.hw_revision as number,
    model: j.model as string,
    fwVersion: j.fw_version as string,
    serial: j.serial as number[],
    uptimeS: j.uptime_s as number,
    resetReason: fromCode(RESET_REASON_CODES, j.reset_reason as number, 'unknown'),
    bootCount: j.boot_count as number,
    flags: j.flags as number,
  };
}

function telemetryFromJson(j: Record<string, unknown>): Telemetry {
  return {
    structVersion: j.struct_version as number,
    flags: j.flags as number,
    speedX100: j.speed_x100 as number,
    avgSpeedX100: j.avg_speed_x100 as number,
    maxSpeedX100: j.max_speed_x100 as number,
    tripDistanceCm: j.trip_distance_cm as number,
    movingTimeS: j.moving_time_s as number,
    odometerM: j.odometer_m as number,
    batteryMv: j.battery_mv as number,
    batteryPct: j.battery_pct as number,
    rideState: fromCode(RIDE_STATE_CODES, j.ride_state as number, 'unknown'),
    revolutions: j.revolutions as number,
    lastPulseAgeMs: j.last_pulse_age_ms as number,
    seq: j.seq as number,
    sensorState: fromCode(SENSOR_STATE_CODES, j.sensor_state as number, 'unknown'),
    powerState: fromCode(POWER_STATE_CODES, j.power_state as number, 'unknown'),
  };
}

function configFromJson(j: Record<string, unknown>): DeviceConfig {
  return {
    structVersion: j.struct_version as number,
    flags: j.flags as number,
    wheelCircumferenceMm: j.wheel_circumference_mm as number,
    maxSpeedKmh: j.max_speed_kmh as number,
    stopTimeoutS: j.stop_timeout_s as number,
    displayTimeoutS: j.display_timeout_s as number,
    deepSleepTimeoutS: j.deep_sleep_timeout_s as number,
    brightnessPct: j.brightness_pct as number,
    pageSwitchPeriodS: j.page_switch_period_s as number,
    enabledPagesMask: j.enabled_pages_mask as number,
    lowBatteryPct: j.low_battery_pct as number,
    odometerSaveIntervalM: j.odometer_save_interval_m as number,
    smoothingWindow: j.smoothing_window as number,
    debounceMs: j.debounce_ms as number,
    activeEdge: j.active_edge as number,
    pinnedPage: j.pinned_page as number,
    battCalScalePermille: j.batt_cal_scale_permille as number,
    battCalOffsetMv: j.batt_cal_offset_mv as number,
    pageOrder: j.page_order as number[],
    reservedPage: 0,
    deviceName: j.device_name as string,
    reserved: 0,
  };
}

function commandFromJson(j: Record<string, unknown>): DeviceCommand {
  const idCode = j.command_id as number;
  return {
    structVersion: j.struct_version as number,
    id: fromCode(COMMAND_ID_CODES, idCode, 'unknown'),
    hasToken: ((j.flags as number) & 0x01) !== 0,
    payload: (j.payload as number[]) ?? [],
  };
}

function commandResultFromJson(j: Record<string, unknown>): CommandResult {
  return {
    structVersion: j.struct_version as number,
    commandId: fromCode(COMMAND_ID_CODES, j.command_id as number, 'unknown'),
    status: fromCode(COMMAND_STATUS_CODES, j.status as number, 'unknown'),
    detail: j.detail as number,
    token: j.token as number,
    payload: (j.payload as number[]) ?? [],
  };
}

function companionFromJson(j: Record<string, unknown>): CompanionSnapshot {
  return {
    structVersion: j.struct_version as number,
    unixTime: j.unix_time as number,
    tzOffsetMin: j.tz_offset_min as number,
    tempCX10: j.temp_c_x10 as number,
    popPct: j.pop_pct as number,
    flags: j.flags as number,
    validUntil: j.valid_until as number,
  };
}

describe('protocol codecs vs fixtures', () => {
  it('device info roundtrip', () => {
    const expected = deviceInfoFromJson(readJson('device_info_v1_nominal.json'));
    const bytes = readHex('device_info_v1_nominal.hex');
    expect(decodeDeviceInfo(bytes)).toEqual(expected);
    expect(bytesToHex(encodeDeviceInfo(expected))).toBe(bytesToHex(bytes));
  });

  it('telemetry moving roundtrip', () => {
    const expected = telemetryFromJson(readJson('telemetry_v1_moving.json'));
    const bytes = readHex('telemetry_v1_moving.hex');
    expect(decodeTelemetry(bytes)).toEqual(expected);
    expect(bytesToHex(encodeTelemetry(expected))).toBe(bytesToHex(bytes));
  });

  it('telemetry paused roundtrip', () => {
    const expected = telemetryFromJson(readJson('telemetry_v1_paused.json'));
    const bytes = readHex('telemetry_v1_paused.hex');
    expect(decodeTelemetry(bytes)).toEqual(expected);
    expect(bytesToHex(encodeTelemetry(expected))).toBe(bytesToHex(bytes));
  });

  it('config defaults roundtrip', () => {
    const expected = configFromJson(readJson('config_v1_defaults.json'));
    const bytes = readHex('config_v1_defaults.hex');
    expect(decodeConfig(bytes)).toEqual(expected);
    expect(bytesToHex(encodeConfig(expected))).toBe(bytesToHex(bytes));
  });

  it('config imperial roundtrip', () => {
    const expected = configFromJson(readJson('config_v1_imperial.json'));
    const bytes = readHex('config_v1_imperial.hex');
    expect(decodeConfig(bytes)).toEqual(expected);
    expect(bytesToHex(encodeConfig(expected))).toBe(bytesToHex(bytes));
  });

  it('command reset trip', () => {
    const expected = commandFromJson(readJson('command_reset_trip.json'));
    const bytes = readHex('command_reset_trip.hex');
    expect(decodeCommand(bytes)).toEqual(expected);
    expect(bytesToHex(encodeCommand(expected))).toBe(bytesToHex(bytes));
  });

  it('command result ok', () => {
    const expected = commandResultFromJson(readJson('result_ok.json'));
    const bytes = readHex('result_ok.hex');
    expect(decodeCommandResult(bytes)).toEqual(expected);
    expect(bytesToHex(encodeCommandResult(expected))).toBe(bytesToHex(bytes));
  });

  it('companion nominal', () => {
    const expected = companionFromJson(readJson('companion_v1_nominal.json'));
    const bytes = readHex('companion_v1_nominal.hex');
    expect(decodeCompanion(bytes)).toEqual(expected);
    expect(bytesToHex(encodeCompanion(expected))).toBe(bytesToHex(bytes));
  });
});
