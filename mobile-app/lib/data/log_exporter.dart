import 'dart:convert';
import 'dart:io';

import 'package:path_provider/path_provider.dart';

import '../application/app_states.dart';
import '../domain/entities/models.dart';
import 'ride_log_recorder.dart';

abstract final class LogExporter {
  static Future<File> write({
    required DateTime exportedAt,
    String? deviceId,
    DeviceInfo? deviceInfo,
    DeviceConfig? config,
    DiagnosticSnapshot? diagnostic,
    ErrorLogBatch? errorLog,
    required List<RideLogSample> samples,
    int? rssi,
    ConnectionState? connectionState,
    bool sensorTestActive = false,
    Directory? directory,
  }) async {
    final target = directory ?? await getTemporaryDirectory();
    final stamp = exportedAt.toUtc().toIso8601String().replaceAll(':', '-');
    final file = File('${target.path}/bikecomp-log-$stamp.json');
    final payload = <String, Object?>{
      'schema': 1,
      'exportedAt': exportedAt.toUtc().toIso8601String(),
      'device': <String, Object?>{
        'deviceId': deviceId,
        'rssi': rssi,
        'info': deviceInfo == null ? null : _deviceInfoJson(deviceInfo),
        'config': config == null ? null : _configJson(config),
      },
      'diagnostic': diagnostic == null ? null : _diagnosticJson(diagnostic),
      'errorLog': errorLog == null
          ? <Object?>[]
          : errorLog.entries.map(_errorLogEntryJson).toList(growable: false),
      'telemetrySamples': samples
          .map((sample) => sample.toJson())
          .toList(growable: false),
      'session': <String, Object?>{
        'connectionState': _connectionStateName(connectionState),
        'sensorTestActive': sensorTestActive,
      },
    };
    await file.writeAsString(
      const JsonEncoder.withIndent('  ').convert(payload),
      flush: true,
    );
    return file;
  }

  static Map<String, Object?> _deviceInfoJson(DeviceInfo value) =>
      <String, Object?>{
        'model': value.model,
        'fwVersion': value.fwVersion,
        'protoMajor': value.protoMajor,
        'protoMinor': value.protoMinor,
        'uptimeS': value.uptimeS,
        'bootCount': value.bootCount,
        'resetReason': value.resetReason.name,
      };

  static Map<String, Object?> _configJson(DeviceConfig value) =>
      <String, Object?>{
        'wheelCircumferenceMm': value.wheelCircumferenceMm,
        'maxSpeedKmh': value.maxSpeedKmh,
        'smoothingWindow': value.smoothingWindow,
        'debounceMs': value.debounceMs,
        'deviceName': value.deviceName,
      };

  static Map<String, Object?> _diagnosticJson(DiagnosticSnapshot value) =>
      <String, Object?>{
        'rawPulseCount': value.rawPulseCount,
        'rejectedDebounce': value.rejectedDebounce,
        'rejectedOverspeed': value.rejectedOverspeed,
        'isrOverflow': value.isrOverflow,
        'speedIntervalCorrected': 0,
      };

  static Map<String, Object?> _errorLogEntryJson(ErrorLogEntry value) =>
      <String, Object?>{
        'uptimeMs': value.uptimeS * 1000,
        'code': value.code,
        'severity': value.severity,
        'detail': value.detail,
      };

  static String _connectionStateName(ConnectionState? state) => switch (state) {
    ConnectionReady() => 'ready',
    ConnectionReadOnly() => 'readOnly',
    ConnectionScanning() => 'scanning',
    ConnectionConnecting() => 'connecting',
    ConnectionSynchronizing() => 'synchronizing',
    ConnectionReconnecting() => 'reconnecting',
    ConnectionFailed() => 'failed',
    ConnectionBluetoothOff() => 'bluetoothOff',
    ConnectionPermissionRequired() => 'permissionRequired',
    ConnectionIncompatibleProtocol() => 'incompatibleProtocol',
    ConnectionIdle() || null => 'idle',
  };
}
