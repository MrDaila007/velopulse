import 'dart:io';

import 'package:bikecomp_mobile/application/app_states.dart';
import 'package:bikecomp_mobile/data/log_exporter.dart';
import 'package:bikecomp_mobile/data/protocol/protocol_codecs.dart';
import 'package:bikecomp_mobile/data/ride_log_recorder.dart';
import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('diagnostic and error log codecs', () {
    test('decodeDiagnostic maps little-endian payload', () {
      final bytes = <int>[
        0x2A,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x01,
        0x00,
        0x02,
        0x00,
        0x2A,
        0x00,
        0x20,
        0x00,
        0x01,
        0x3F,
      ];
      final value = ProtocolCodecs.decodeDiagnostic(bytes);
      expect(value.rawPulseCount, 42);
      expect(value.rejectedDebounce, 3);
      expect(value.rejectedOverspeed, 1);
      expect(value.isrOverflow, 2);
      expect(value.flashWriteCount, 42);
      expect(value.freeHeapBytes, 0x20 * 16);
      expect(value.i2cErrorCount, 1);
      expect(value.selftestMask, 0x3F);
    });

    test('decodeErrorLog maps batch entries', () {
      final bytes = <int>[
        1,
        2,
        10,
        0,
        0,
        0,
        0x05,
        1,
        0x0A,
        0x00,
        20,
        0,
        0,
        0,
        0x07,
        2,
        0x14,
        0x00,
      ];
      final batch = ProtocolCodecs.decodeErrorLog(bytes);
      expect(batch.entries, hasLength(2));
      expect(batch.entries.first.uptimeS, 10);
      expect(batch.entries.first.code, 0x05);
      expect(batch.entries.first.severity, 1);
      expect(batch.entries.first.detail, 10);
      expect(batch.entries.last.code, 0x07);
    });
  });

  group('ride log export', () {
    test('LogExporter writes schema v1 json', () async {
      final recorder = RideLogRecorder();
      recorder.record(
        const Telemetry(
          structVersion: 1,
          flags: 0,
          speedX100: 2500,
          avgSpeedX100: 2400,
          maxSpeedX100: 3000,
          tripDistanceCm: 1000,
          movingTimeS: 60,
          odometerM: 100,
          batteryMv: 3900,
          batteryPct: 80,
          rideState: RideState.moving,
          revolutions: 10,
          lastPulseAgeMs: 100,
          seq: 1,
          sensorState: SensorState.ok,
          powerState: PowerState.active,
        ),
        rssi: -60,
      );
      final exportedAt = DateTime.utc(2026, 8, 2, 12, 0);
      final directory = Directory.systemTemp.createTempSync(
        'bikecomp-log-test',
      );
      final file = await LogExporter.write(
        exportedAt: exportedAt,
        directory: directory,
        deviceId: 'FA:KE',
        deviceInfo: const DeviceInfo(
          structVersion: 1,
          protoMajor: 1,
          protoMinor: 0,
          hwRevision: 1,
          model: 'BIKECOMP-XIAO',
          fwVersion: '1.0.0',
          serial: <int>[1, 2, 3, 4, 5, 6, 7, 8],
          uptimeS: 10,
          resetReason: ResetReason.powerOn,
          bootCount: 1,
          flags: 0,
        ),
        config: DeviceConfig.defaults,
        diagnostic: const DiagnosticSnapshot(
          rawPulseCount: 12,
          rejectedDebounce: 0,
          rejectedOverspeed: 0,
          isrOverflow: 0,
          flashWriteCount: 1,
          freeHeapBytes: 4096,
          i2cErrorCount: 0,
          selftestMask: 0x3F,
        ),
        errorLog: const ErrorLogBatch(
          entries: <ErrorLogEntry>[
            ErrorLogEntry(uptimeS: 5, code: 5, severity: 1, detail: 0),
          ],
        ),
        samples: recorder.samples,
        rssi: -60,
        connectionState: const ConnectionState.ready(),
        sensorTestActive: false,
      );
      final text = await file.readAsString();
      expect(text, contains('"schema": 1'));
      expect(text, contains('"device"'));
      expect(text, contains('"telemetrySamples"'));
      expect(text, contains('"diagnostic"'));
      expect(text, contains('"errorLog"'));
      expect(text, contains('"session"'));
      expect(text, contains('"speedIntervalCorrected": 0'));
    });
  });
}
