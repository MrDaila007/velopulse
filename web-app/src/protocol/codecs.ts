import {
  type CommandResult,
  type CompanionSnapshot,
  type DeviceCommand,
  type DeviceConfig,
  type DeviceInfo,
  type DiagnosticSnapshot,
  type ErrorLogBatch,
  type ErrorLogEntry,
  type Telemetry,
  COMMAND_ID_CODES,
  COMMAND_STATUS_CODES,
  fromCode,
  POWER_STATE_CODES,
  RESET_REASON_CODES,
  RIDE_STATE_CODES,
  SENSOR_STATE_CODES,
  type CommandStatus,
  type DeviceCommandId,
  type PowerState,
  type ResetReason,
  type RideState,
  type SensorState,
} from '../domain/types';

export class ProtocolCodecError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'ProtocolCodecError';
  }
}

export const SIZES = {
  deviceInfo: 48,
  telemetry: 36,
  telemetryV2: 44,
  config: 48,
  companion: 15,
  diagnosticPayload: 16,
} as const;

function dataView(bytes: Uint8Array, v1Size: number, name: string): DataView {
  if (bytes.length === 0) throw new ProtocolCodecError(`${name}: пустой пакет`);
  const version = bytes[0];
  if (version < 1) {
    throw new ProtocolCodecError(`${name}: версия структуры ${version} не поддерживается`);
  }
  if (version === 1 && bytes.length !== v1Size) {
    throw new ProtocolCodecError(`${name}: ожидалось ${v1Size} байт, получено ${bytes.length}`);
  }
  if (version > 1 && bytes.length < v1Size) {
    throw new ProtocolCodecError(`${name}: нет полного v1-префикса`);
  }
  return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
}

function readAscii(bytes: Uint8Array, offset: number, length: number): string {
  const slice = bytes.subarray(offset, offset + length);
  const zero = slice.indexOf(0);
  const value = zero < 0 ? slice : slice.subarray(0, zero);
  for (const byte of value) {
    if (byte > 0x7f) throw new ProtocolCodecError('Строка содержит не-ASCII байты');
  }
  return new TextDecoder('ascii').decode(value);
}

function writeAscii(out: Uint8Array, offset: number, length: number, value: string): void {
  const encoded = new TextEncoder().encode(value);
  if (encoded.length > length) {
    throw new ProtocolCodecError(`Строка "${value}" длиннее ${length} байт`);
  }
  out.set(encoded, offset);
}

export function decodeDeviceInfo(bytes: Uint8Array): DeviceInfo {
  const view = dataView(bytes, SIZES.deviceInfo, 'Device Info');
  return {
    structVersion: view.getUint8(0),
    protoMajor: view.getUint8(1),
    protoMinor: view.getUint8(2),
    hwRevision: view.getUint8(3),
    model: readAscii(bytes, 4, 16),
    fwVersion: readAscii(bytes, 20, 12),
    serial: Array.from(bytes.subarray(32, 40)),
    uptimeS: view.getUint32(40, true),
    resetReason: fromCode(RESET_REASON_CODES, view.getUint8(44), 'unknown' as ResetReason),
    bootCount: view.getUint16(45, true),
    flags: view.getUint8(47),
  };
}

export function encodeDeviceInfo(value: DeviceInfo): Uint8Array {
  const out = new Uint8Array(SIZES.deviceInfo);
  const view = new DataView(out.buffer);
  view.setUint8(0, value.structVersion);
  view.setUint8(1, value.protoMajor);
  view.setUint8(2, value.protoMinor);
  view.setUint8(3, value.hwRevision);
  writeAscii(out, 4, 16, value.model);
  writeAscii(out, 20, 12, value.fwVersion);
  if (value.serial.length !== 8) {
    throw new ProtocolCodecError('Serial должен содержать 8 байт');
  }
  out.set(value.serial, 32);
  view.setUint32(40, value.uptimeS, true);
  view.setUint8(44, RESET_REASON_CODES[value.resetReason]);
  view.setUint16(45, value.bootCount, true);
  view.setUint8(47, value.flags);
  return out;
}

export function decodeTelemetry(bytes: Uint8Array): Telemetry {
  const view = dataView(bytes, SIZES.telemetry, 'Telemetry');
  return {
    structVersion: view.getUint8(0),
    flags: view.getUint8(1),
    speedX100: view.getUint16(2, true),
    avgSpeedX100: view.getUint16(4, true),
    maxSpeedX100: view.getUint16(6, true),
    tripDistanceCm: view.getUint32(8, true),
    movingTimeS: view.getUint32(12, true),
    odometerM: view.getUint32(16, true),
    batteryMv: view.getUint16(20, true),
    batteryPct: view.getUint8(22),
    rideState: fromCode(RIDE_STATE_CODES, view.getUint8(23), 'unknown' as RideState),
    revolutions: view.getUint32(24, true),
    lastPulseAgeMs: view.getUint32(28, true),
    seq: view.getUint16(32, true),
    sensorState: fromCode(SENSOR_STATE_CODES, view.getUint8(34), 'unknown' as SensorState),
    powerState: fromCode(POWER_STATE_CODES, view.getUint8(35), 'unknown' as PowerState),
    cadenceX10: bytes.length >= SIZES.telemetryV2 ? view.getUint16(36, true) : 0,
    cscFlags: bytes.length >= SIZES.telemetryV2 ? view.getUint8(38) : 0,
    lastCrankEventAgeMs:
      bytes.length >= SIZES.telemetryV2 ? view.getUint32(40, true) : 0xffffffff,
  };
}

export function encodeTelemetry(value: Telemetry): Uint8Array {
  const size = value.structVersion >= 2 ? SIZES.telemetryV2 : SIZES.telemetry;
  const out = new Uint8Array(size);
  const view = new DataView(out.buffer);
  view.setUint8(0, value.structVersion);
  view.setUint8(1, value.flags);
  view.setUint16(2, value.speedX100, true);
  view.setUint16(4, value.avgSpeedX100, true);
  view.setUint16(6, value.maxSpeedX100, true);
  view.setUint32(8, value.tripDistanceCm, true);
  view.setUint32(12, value.movingTimeS, true);
  view.setUint32(16, value.odometerM, true);
  view.setUint16(20, value.batteryMv, true);
  view.setUint8(22, value.batteryPct);
  view.setUint8(23, RIDE_STATE_CODES[value.rideState]);
  view.setUint32(24, value.revolutions, true);
  view.setUint32(28, value.lastPulseAgeMs, true);
  view.setUint16(32, value.seq, true);
  view.setUint8(34, SENSOR_STATE_CODES[value.sensorState]);
  view.setUint8(35, POWER_STATE_CODES[value.powerState]);
  if (size >= SIZES.telemetryV2) {
    view.setUint16(36, value.cadenceX10, true);
    view.setUint8(38, value.cscFlags);
    view.setUint8(39, 0);
    view.setUint32(40, value.lastCrankEventAgeMs, true);
  }
  return out;
}

export function decodeConfig(bytes: Uint8Array): DeviceConfig {
  const view = dataView(bytes, SIZES.config, 'Configuration');
  return {
    structVersion: view.getUint8(0),
    flags: view.getUint8(1),
    wheelCircumferenceMm: view.getUint16(2, true),
    maxSpeedKmh: view.getUint8(4),
    stopTimeoutS: view.getUint8(5),
    displayTimeoutS: view.getUint16(6, true),
    deepSleepTimeoutS: view.getUint16(8, true),
    brightnessPct: view.getUint8(10),
    pageSwitchPeriodS: view.getUint8(11),
    enabledPagesMask: view.getUint8(12),
    lowBatteryPct: view.getUint8(13),
    odometerSaveIntervalM: view.getUint16(14, true),
    smoothingWindow: view.getUint8(16),
    debounceMs: view.getUint8(17),
    activeEdge: view.getUint8(18),
    pinnedPage: view.getUint8(19),
    battCalScalePermille: view.getUint16(20, true),
    battCalOffsetMv: view.getInt16(22, true),
    pageOrder: Array.from(bytes.subarray(24, 29)),
    reservedPage: view.getUint8(29),
    deviceName: readAscii(bytes, 30, 16),
    reserved: view.getUint16(46, true),
  };
}

export function encodeConfig(value: DeviceConfig): Uint8Array {
  const out = new Uint8Array(SIZES.config);
  const view = new DataView(out.buffer);
  view.setUint8(0, value.structVersion);
  view.setUint8(1, value.flags);
  view.setUint16(2, value.wheelCircumferenceMm, true);
  view.setUint8(4, value.maxSpeedKmh);
  view.setUint8(5, value.stopTimeoutS);
  view.setUint16(6, value.displayTimeoutS, true);
  view.setUint16(8, value.deepSleepTimeoutS, true);
  view.setUint8(10, value.brightnessPct);
  view.setUint8(11, value.pageSwitchPeriodS);
  view.setUint8(12, value.enabledPagesMask);
  view.setUint8(13, value.lowBatteryPct);
  view.setUint16(14, value.odometerSaveIntervalM, true);
  view.setUint8(16, value.smoothingWindow);
  view.setUint8(17, value.debounceMs);
  view.setUint8(18, value.activeEdge);
  view.setUint8(19, value.pinnedPage);
  view.setUint16(20, value.battCalScalePermille, true);
  view.setInt16(22, value.battCalOffsetMv, true);
  if (value.pageOrder.length !== 5) {
    throw new ProtocolCodecError('Page order должен содержать 5 элементов');
  }
  out.set(value.pageOrder, 24);
  view.setUint8(29, value.reservedPage);
  writeAscii(out, 30, 16, value.deviceName);
  view.setUint16(46, value.reserved, true);
  return out;
}

export function decodeCommand(bytes: Uint8Array): DeviceCommand {
  if (bytes.length < 4 || bytes.length > 20) {
    throw new ProtocolCodecError('Command: размер должен быть 4–20 байт');
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const version = view.getUint8(0);
  if (version !== 1) {
    throw new ProtocolCodecError(`Command: версия ${version} не поддерживается`);
  }
  const payloadLength = view.getUint8(3);
  if (payloadLength > 16 || bytes.length !== 4 + payloadLength) {
    throw new ProtocolCodecError('Command: неверный payload_len');
  }
  return {
    structVersion: version,
    id: fromCode(COMMAND_ID_CODES, view.getUint8(1), 'unknown' as DeviceCommandId),
    hasToken: (view.getUint8(2) & 0x01) !== 0,
    payload: Array.from(bytes.subarray(4)),
  };
}

export function encodeCommand(value: DeviceCommand): Uint8Array {
  if (value.payload.length > 16) {
    throw new ProtocolCodecError('Command payload длиннее 16 байт');
  }
  const out = new Uint8Array(4 + value.payload.length);
  out[0] = value.structVersion;
  out[1] = COMMAND_ID_CODES[value.id];
  out[2] = value.hasToken ? 1 : 0;
  out[3] = value.payload.length;
  out.set(value.payload, 4);
  return out;
}

export function decodeCommandResult(bytes: Uint8Array): CommandResult {
  if (bytes.length < 9 || bytes.length > 25) {
    throw new ProtocolCodecError('Command Result: размер должен быть 9–25 байт');
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const version = view.getUint8(0);
  if (version !== 1) {
    throw new ProtocolCodecError(`Command Result: версия ${version} не поддерживается`);
  }
  const payloadLength = view.getUint8(8);
  if (payloadLength > 16 || bytes.length !== 9 + payloadLength) {
    throw new ProtocolCodecError('Command Result: неверный payload_len');
  }
  return {
    structVersion: version,
    commandId: fromCode(COMMAND_ID_CODES, view.getUint8(1), 'unknown' as DeviceCommandId),
    status: fromCode(COMMAND_STATUS_CODES, view.getUint8(2), 'unknown' as CommandStatus),
    detail: view.getUint8(3),
    token: view.getUint32(4, true),
    payload: Array.from(bytes.subarray(9)),
  };
}

export function encodeCommandResult(value: CommandResult): Uint8Array {
  if (value.payload.length > 16) {
    throw new ProtocolCodecError('Command Result payload длиннее 16 байт');
  }
  const out = new Uint8Array(9 + value.payload.length);
  const view = new DataView(out.buffer);
  view.setUint8(0, value.structVersion);
  view.setUint8(1, COMMAND_ID_CODES[value.commandId]);
  view.setUint8(2, COMMAND_STATUS_CODES[value.status]);
  view.setUint8(3, value.detail);
  view.setUint32(4, value.token, true);
  view.setUint8(8, value.payload.length);
  out.set(value.payload, 9);
  return out;
}

export function decodeDiagnostic(bytes: Uint8Array): DiagnosticSnapshot {
  if (bytes.length !== SIZES.diagnosticPayload) {
    throw new ProtocolCodecError(
      `Diagnostic: ожидалось ${SIZES.diagnosticPayload} байт, получено ${bytes.length}`,
    );
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return {
    rawPulseCount: view.getUint32(0, true),
    rejectedDebounce: view.getUint16(4, true),
    rejectedOverspeed: view.getUint16(6, true),
    isrOverflow: view.getUint16(8, true),
    flashWriteCount: view.getUint16(10, true),
    freeHeapBytes: view.getUint16(12, true) * 16,
    i2cErrorCount: view.getUint8(14),
    selftestMask: view.getUint8(15),
  };
}

export function decodeErrorLog(bytes: Uint8Array): ErrorLogBatch {
  if (bytes.length === 0) throw new ProtocolCodecError('Error Log: пустой пакет');
  const version = bytes[0];
  if (version !== 1) {
    throw new ProtocolCodecError(`Error Log: версия ${version} не поддерживается`);
  }
  if (bytes.length < 2) throw new ProtocolCodecError('Error Log: нет entry_count');
  const entryCount = bytes[1];
  if (entryCount < 1 || entryCount > 4) {
    throw new ProtocolCodecError(`Error Log: entry_count=${entryCount} вне диапазона 1…4`);
  }
  const expected = 2 + entryCount * 8;
  if (bytes.length !== expected) {
    throw new ProtocolCodecError(`Error Log: ожидалось ${expected} байт, получено ${bytes.length}`);
  }
  const entries: ErrorLogEntry[] = [];
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  for (let index = 0; index < entryCount; index++) {
    const offset = 2 + index * 8;
    entries.push({
      uptimeS: view.getUint32(offset, true),
      code: view.getUint8(offset + 4),
      severity: view.getUint8(offset + 5),
      detail: view.getUint16(offset + 6, true),
    });
  }
  return { entries };
}

export function decodeCompanion(bytes: Uint8Array): CompanionSnapshot {
  const view = dataView(bytes, SIZES.companion, 'Companion');
  return {
    structVersion: view.getUint8(0),
    unixTime: view.getUint32(1, true),
    tzOffsetMin: view.getInt16(5, true),
    tempCX10: view.getInt16(7, true),
    popPct: view.getUint8(9),
    flags: view.getUint8(10),
    validUntil: view.getUint32(11, true),
  };
}

export function encodeCompanion(value: CompanionSnapshot): Uint8Array {
  const out = new Uint8Array(SIZES.companion);
  const view = new DataView(out.buffer);
  view.setUint8(0, value.structVersion);
  view.setUint32(1, value.unixTime, true);
  view.setInt16(5, value.tzOffsetMin, true);
  view.setInt16(7, value.tempCX10, true);
  view.setUint8(9, value.popPct);
  view.setUint8(10, value.flags);
  view.setUint32(11, value.validUntil, true);
  return out;
}

export function hexToBytes(hex: string): Uint8Array {
  const cleaned = hex.replace(/\s+/g, '').toLowerCase();
  const bytes = new Uint8Array(cleaned.length / 2);
  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(cleaned.slice(i * 2, i * 2 + 2), 16);
  }
  return bytes;
}

export function bytesToHex(bytes: Uint8Array): string {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join('');
}
