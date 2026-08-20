import 'dart:convert';
import 'dart:typed_data';

import '../../domain/entities/companion_models.dart';
import '../../domain/entities/models.dart';

class ProtocolCodecException implements Exception {
  const ProtocolCodecException(this.message);
  final String message;

  @override
  String toString() => 'ProtocolCodecException: $message';
}

abstract final class ProtocolCodecs {
  static const deviceInfoSize = 48;
  static const telemetrySize = 36;
  static const configSize = 48;
  static const companionSize = 15;
  static const diagnosticPayloadSize = 16;

  static ByteData _data(List<int> bytes, int v1Size, String name) {
    if (bytes.isEmpty) throw ProtocolCodecException('$name: пустой пакет');
    final version = bytes.first;
    if (version < 1) {
      throw ProtocolCodecException(
        '$name: версия структуры $version не поддерживается',
      );
    }
    if (version == 1 && bytes.length != v1Size) {
      throw ProtocolCodecException(
        '$name: ожидалось $v1Size байт, получено ${bytes.length}',
      );
    }
    if (version > 1 && bytes.length < v1Size) {
      throw ProtocolCodecException('$name: нет полного v1-префикса');
    }
    return ByteData.sublistView(Uint8List.fromList(bytes));
  }

  static String _ascii(List<int> bytes) {
    final zero = bytes.indexOf(0);
    final value = zero < 0 ? bytes : bytes.sublist(0, zero);
    if (value.any((byte) => byte > 0x7F)) {
      throw const ProtocolCodecException('Строка содержит не-ASCII байты');
    }
    return ascii.decode(value);
  }

  static void _writeAscii(Uint8List out, int offset, int length, String value) {
    final encoded = ascii.encode(value);
    if (encoded.length > length) {
      throw ProtocolCodecException('Строка "$value" длиннее $length байт');
    }
    out.setRange(offset, offset + encoded.length, encoded);
  }

  static DeviceInfo decodeDeviceInfo(List<int> bytes) {
    final data = _data(bytes, deviceInfoSize, 'Device Info');
    return DeviceInfo(
      structVersion: data.getUint8(0),
      protoMajor: data.getUint8(1),
      protoMinor: data.getUint8(2),
      hwRevision: data.getUint8(3),
      model: _ascii(bytes.sublist(4, 20)),
      fwVersion: _ascii(bytes.sublist(20, 32)),
      serial: List.unmodifiable(bytes.sublist(32, 40)),
      uptimeS: data.getUint32(40, Endian.little),
      resetReason: ResetReason.fromCode(data.getUint8(44)),
      bootCount: data.getUint16(45, Endian.little),
      flags: data.getUint8(47),
    );
  }

  static Uint8List encodeDeviceInfo(DeviceInfo value) {
    final out = Uint8List(deviceInfoSize);
    final data = ByteData.sublistView(out);
    data.setUint8(0, value.structVersion);
    data.setUint8(1, value.protoMajor);
    data.setUint8(2, value.protoMinor);
    data.setUint8(3, value.hwRevision);
    _writeAscii(out, 4, 16, value.model);
    _writeAscii(out, 20, 12, value.fwVersion);
    if (value.serial.length != 8) {
      throw const ProtocolCodecException('Serial должен содержать 8 байт');
    }
    out.setRange(32, 40, value.serial);
    data.setUint32(40, value.uptimeS, Endian.little);
    data.setUint8(44, value.resetReason.code);
    data.setUint16(45, value.bootCount, Endian.little);
    data.setUint8(47, value.flags);
    return out;
  }

  static Telemetry decodeTelemetry(List<int> bytes) {
    final data = _data(bytes, telemetrySize, 'Telemetry');
    return Telemetry(
      structVersion: data.getUint8(0),
      flags: data.getUint8(1),
      speedX100: data.getUint16(2, Endian.little),
      avgSpeedX100: data.getUint16(4, Endian.little),
      maxSpeedX100: data.getUint16(6, Endian.little),
      tripDistanceCm: data.getUint32(8, Endian.little),
      movingTimeS: data.getUint32(12, Endian.little),
      odometerM: data.getUint32(16, Endian.little),
      batteryMv: data.getUint16(20, Endian.little),
      batteryPct: data.getUint8(22),
      rideState: RideState.fromCode(data.getUint8(23)),
      revolutions: data.getUint32(24, Endian.little),
      lastPulseAgeMs: data.getUint32(28, Endian.little),
      seq: data.getUint16(32, Endian.little),
      sensorState: SensorState.fromCode(data.getUint8(34)),
      powerState: PowerState.fromCode(data.getUint8(35)),
      cadenceX10: bytes.length >= 44 ? data.getUint16(36, Endian.little) : 0,
      cscFlags: bytes.length >= 44 ? data.getUint8(38) : 0,
      lastCrankEventAgeMs: bytes.length >= 44
          ? data.getUint32(40, Endian.little)
          : 0xFFFFFFFF,
    );
  }

  static Uint8List encodeTelemetry(Telemetry value) {
    final out = Uint8List(telemetrySize);
    final data = ByteData.sublistView(out);
    data.setUint8(0, value.structVersion);
    data.setUint8(1, value.flags);
    data.setUint16(2, value.speedX100, Endian.little);
    data.setUint16(4, value.avgSpeedX100, Endian.little);
    data.setUint16(6, value.maxSpeedX100, Endian.little);
    data.setUint32(8, value.tripDistanceCm, Endian.little);
    data.setUint32(12, value.movingTimeS, Endian.little);
    data.setUint32(16, value.odometerM, Endian.little);
    data.setUint16(20, value.batteryMv, Endian.little);
    data.setUint8(22, value.batteryPct);
    data.setUint8(23, value.rideState.code);
    data.setUint32(24, value.revolutions, Endian.little);
    data.setUint32(28, value.lastPulseAgeMs, Endian.little);
    data.setUint16(32, value.seq, Endian.little);
    data.setUint8(34, value.sensorState.code);
    data.setUint8(35, value.powerState.code);
    if (value.structVersion >= 2) {
      final v2 = Uint8List(44);
      v2.setRange(0, 36, out);
      final v2data = ByteData.sublistView(v2);
      v2data.setUint16(36, value.cadenceX10, Endian.little);
      v2data.setUint8(38, value.cscFlags);
      v2data.setUint8(39, 0);
      v2data.setUint32(40, value.lastCrankEventAgeMs, Endian.little);
      return v2;
    }
    return out;
  }

  static DeviceConfig decodeConfig(List<int> bytes) {
    final data = _data(bytes, configSize, 'Configuration');
    return DeviceConfig(
      structVersion: data.getUint8(0),
      flags: data.getUint8(1),
      wheelCircumferenceMm: data.getUint16(2, Endian.little),
      maxSpeedKmh: data.getUint8(4),
      stopTimeoutS: data.getUint8(5),
      displayTimeoutS: data.getUint16(6, Endian.little),
      deepSleepTimeoutS: data.getUint16(8, Endian.little),
      brightnessPct: data.getUint8(10),
      pageSwitchPeriodS: data.getUint8(11),
      enabledPagesMask: data.getUint8(12),
      lowBatteryPct: data.getUint8(13),
      odometerSaveIntervalM: data.getUint16(14, Endian.little),
      smoothingWindow: data.getUint8(16),
      debounceMs: data.getUint8(17),
      activeEdge: data.getUint8(18),
      pinnedPage: data.getUint8(19),
      battCalScalePermille: data.getUint16(20, Endian.little),
      battCalOffsetMv: data.getInt16(22, Endian.little),
      pageOrder: List.unmodifiable(bytes.sublist(24, 29)),
      reservedPage: data.getUint8(29),
      deviceName: _ascii(bytes.sublist(30, 46)),
      reserved: data.getUint16(46, Endian.little),
    );
  }

  static Uint8List encodeConfig(DeviceConfig value) {
    final out = Uint8List(configSize);
    final data = ByteData.sublistView(out);
    data.setUint8(0, value.structVersion);
    data.setUint8(1, value.flags);
    data.setUint16(2, value.wheelCircumferenceMm, Endian.little);
    data.setUint8(4, value.maxSpeedKmh);
    data.setUint8(5, value.stopTimeoutS);
    data.setUint16(6, value.displayTimeoutS, Endian.little);
    data.setUint16(8, value.deepSleepTimeoutS, Endian.little);
    data.setUint8(10, value.brightnessPct);
    data.setUint8(11, value.pageSwitchPeriodS);
    data.setUint8(12, value.enabledPagesMask);
    data.setUint8(13, value.lowBatteryPct);
    data.setUint16(14, value.odometerSaveIntervalM, Endian.little);
    data.setUint8(16, value.smoothingWindow);
    data.setUint8(17, value.debounceMs);
    data.setUint8(18, value.activeEdge);
    data.setUint8(19, value.pinnedPage);
    data.setUint16(20, value.battCalScalePermille, Endian.little);
    data.setInt16(22, value.battCalOffsetMv, Endian.little);
    if (value.pageOrder.length != 5) {
      throw const ProtocolCodecException(
        'Page order должен содержать 5 элементов',
      );
    }
    out.setRange(24, 29, value.pageOrder);
    data.setUint8(29, value.reservedPage);
    _writeAscii(out, 30, 16, value.deviceName);
    data.setUint16(46, value.reserved, Endian.little);
    return out;
  }

  static DeviceCommand decodeCommand(List<int> bytes) {
    if (bytes.length < 4 || bytes.length > 20) {
      throw const ProtocolCodecException(
        'Command: размер должен быть 4–20 байт',
      );
    }
    final data = ByteData.sublistView(Uint8List.fromList(bytes));
    final version = data.getUint8(0);
    if (version != 1) {
      throw ProtocolCodecException(
        'Command: версия $version не поддерживается',
      );
    }
    final payloadLength = data.getUint8(3);
    if (payloadLength > 16 || bytes.length != 4 + payloadLength) {
      throw const ProtocolCodecException('Command: неверный payload_len');
    }
    return DeviceCommand(
      structVersion: version,
      id: DeviceCommandId.fromCode(data.getUint8(1)),
      hasToken: data.getUint8(2) & 0x01 != 0,
      payload: List.unmodifiable(bytes.sublist(4)),
    );
  }

  static Uint8List encodeCommand(DeviceCommand value) {
    if (value.payload.length > 16) {
      throw const ProtocolCodecException('Command payload длиннее 16 байт');
    }
    final out = Uint8List(4 + value.payload.length);
    out[0] = value.structVersion;
    out[1] = value.id.code;
    out[2] = value.hasToken ? 1 : 0;
    out[3] = value.payload.length;
    out.setRange(4, out.length, value.payload);
    return out;
  }

  static CommandResult decodeCommandResult(List<int> bytes) {
    if (bytes.length < 9 || bytes.length > 25) {
      throw const ProtocolCodecException(
        'Command Result: размер должен быть 9–25 байт',
      );
    }
    final data = ByteData.sublistView(Uint8List.fromList(bytes));
    final version = data.getUint8(0);
    if (version != 1) {
      throw ProtocolCodecException(
        'Command Result: версия $version не поддерживается',
      );
    }
    final payloadLength = data.getUint8(8);
    if (payloadLength > 16 || bytes.length != 9 + payloadLength) {
      throw const ProtocolCodecException(
        'Command Result: неверный payload_len',
      );
    }
    return CommandResult(
      structVersion: version,
      commandId: DeviceCommandId.fromCode(data.getUint8(1)),
      status: CommandStatus.fromCode(data.getUint8(2)),
      detail: data.getUint8(3),
      token: data.getUint32(4, Endian.little),
      payload: List.unmodifiable(bytes.sublist(9)),
    );
  }

  static Uint8List encodeCommandResult(CommandResult value) {
    if (value.payload.length > 16) {
      throw const ProtocolCodecException(
        'Command Result payload длиннее 16 байт',
      );
    }
    final out = Uint8List(9 + value.payload.length);
    final data = ByteData.sublistView(out);
    data.setUint8(0, value.structVersion);
    data.setUint8(1, value.commandId.code);
    data.setUint8(2, value.status.code);
    data.setUint8(3, value.detail);
    data.setUint32(4, value.token, Endian.little);
    data.setUint8(8, value.payload.length);
    out.setRange(9, out.length, value.payload);
    return out;
  }

  static DiagnosticSnapshot decodeDiagnostic(List<int> bytes) {
    if (bytes.length != diagnosticPayloadSize) {
      throw ProtocolCodecException(
        'Diagnostic: ожидалось $diagnosticPayloadSize байт, получено ${bytes.length}',
      );
    }
    final data = ByteData.sublistView(Uint8List.fromList(bytes));
    return DiagnosticSnapshot(
      rawPulseCount: data.getUint32(0, Endian.little),
      rejectedDebounce: data.getUint16(4, Endian.little),
      rejectedOverspeed: data.getUint16(6, Endian.little),
      isrOverflow: data.getUint16(8, Endian.little),
      flashWriteCount: data.getUint16(10, Endian.little),
      freeHeapBytes: data.getUint16(12, Endian.little) * 16,
      i2cErrorCount: data.getUint8(14),
      selftestMask: data.getUint8(15),
    );
  }

  static ErrorLogBatch decodeErrorLog(List<int> bytes) {
    if (bytes.isEmpty) {
      throw const ProtocolCodecException('Error Log: пустой пакет');
    }
    final version = bytes.first;
    if (version != 1) {
      throw ProtocolCodecException(
        'Error Log: версия $version не поддерживается',
      );
    }
    if (bytes.length < 2) {
      throw const ProtocolCodecException('Error Log: нет entry_count');
    }
    final entryCount = bytes[1];
    if (entryCount < 1 || entryCount > 4) {
      throw ProtocolCodecException(
        'Error Log: entry_count=$entryCount вне диапазона 1…4',
      );
    }
    final expected = 2 + entryCount * 8;
    if (bytes.length != expected) {
      throw ProtocolCodecException(
        'Error Log: ожидалось $expected байт, получено ${bytes.length}',
      );
    }
    final entries = <ErrorLogEntry>[];
    for (var index = 0; index < entryCount; index++) {
      final offset = 2 + index * 8;
      final data = ByteData.sublistView(Uint8List.fromList(bytes));
      entries.add(
        ErrorLogEntry(
          uptimeS: data.getUint32(offset, Endian.little),
          code: data.getUint8(offset + 4),
          severity: data.getUint8(offset + 5),
          detail: data.getUint16(offset + 6, Endian.little),
        ),
      );
    }
    return ErrorLogBatch(entries: List.unmodifiable(entries));
  }

  static CompanionSnapshot decodeCompanion(List<int> bytes) {
    final data = _data(bytes, companionSize, 'Companion');
    return CompanionSnapshot(
      structVersion: data.getUint8(0),
      unixTime: data.getUint32(1, Endian.little),
      tzOffsetMin: data.getInt16(5, Endian.little),
      tempCX10: data.getInt16(7, Endian.little),
      popPct: data.getUint8(9),
      flags: data.getUint8(10),
      validUntil: data.getUint32(11, Endian.little),
    );
  }

  static Uint8List encodeCompanion(CompanionSnapshot value) {
    final out = Uint8List(companionSize);
    final data = ByteData.sublistView(out);
    data.setUint8(0, value.structVersion);
    data.setUint32(1, value.unixTime, Endian.little);
    data.setInt16(5, value.tzOffsetMin, Endian.little);
    data.setInt16(7, value.tempCX10, Endian.little);
    data.setUint8(9, value.popPct);
    data.setUint8(10, value.flags);
    data.setUint32(11, value.validUntil, Endian.little);
    return out;
  }
}
