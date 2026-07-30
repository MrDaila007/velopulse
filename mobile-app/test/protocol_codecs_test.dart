import 'dart:io';

import 'package:bikecomp_mobile/data/protocol/protocol_codecs.dart';
import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:flutter_test/flutter_test.dart';

List<int> fixture(String name) {
  final hex = File('../protocol/fixtures/$name.hex').readAsStringSync().trim();
  return <int>[
    for (var offset = 0; offset < hex.length; offset += 2)
      int.parse(hex.substring(offset, offset + 2), radix: 16),
  ];
}

void main() {
  group('protocol v1 golden fixtures', () {
    test('Device Info decodes, encodes and round-trips', () {
      final bytes = fixture('device_info_v1_nominal');
      final value = ProtocolCodecs.decodeDeviceInfo(bytes);

      expect(value.model, 'BIKECOMP-XIAO');
      expect(value.fwVersion, '1.0.0');
      expect(value.uptimeS, 3600);
      expect(value.bootCount, 42);
      expect(value.resetReason, ResetReason.powerOn);
      expect(ProtocolCodecs.encodeDeviceInfo(value), bytes);
    });

    for (final name in <String>['telemetry_v1_moving', 'telemetry_v1_paused']) {
      test('$name decodes, encodes and round-trips', () {
        final bytes = fixture(name);
        final value = ProtocolCodecs.decodeTelemetry(bytes);
        expect(ProtocolCodecs.encodeTelemetry(value), bytes);
      });
    }

    for (final name in <String>['config_v1_defaults', 'config_v1_imperial']) {
      test('$name decodes, encodes and round-trips', () {
        final bytes = fixture(name);
        final value = ProtocolCodecs.decodeConfig(bytes);
        expect(value.unitsImperial, name.endsWith('imperial'));
        expect(ProtocolCodecs.encodeConfig(value), bytes);
      });
    }

    for (final name in <String>[
      'command_reset_trip',
      'command_reset_odo_with_token',
    ]) {
      test('$name decodes, encodes and round-trips', () {
        final bytes = fixture(name);
        final value = ProtocolCodecs.decodeCommand(bytes);
        expect(ProtocolCodecs.encodeCommand(value), bytes);
      });
    }

    for (final name in <String>['result_ok', 'result_err_range_wheel']) {
      test('$name decodes, encodes and round-trips', () {
        final bytes = fixture(name);
        final value = ProtocolCodecs.decodeCommandResult(bytes);
        expect(ProtocolCodecs.encodeCommandResult(value), bytes);
      });
    }
  });

  group('strict framing and compatibility', () {
    test('v1 fixed structures reject malformed length', () {
      expect(
        () => ProtocolCodecs.decodeConfig(
          fixture('config_v1_defaults').sublist(0, 47),
        ),
        throwsA(isA<ProtocolCodecException>()),
      );
      expect(
        () => ProtocolCodecs.decodeTelemetry(<int>[1, 0]),
        throwsA(isA<ProtocolCodecException>()),
      );
    });

    test('future structure accepts complete v1 prefix and ignores tail', () {
      final bytes = fixture('telemetry_v1_moving');
      bytes[0] = 2;
      bytes.addAll(<int>[0xAA, 0x55]);

      final telemetry = ProtocolCodecs.decodeTelemetry(bytes);

      expect(telemetry.structVersion, 2);
      expect(telemetry.speedX100, 2550);
    });

    test('future structure rejects truncated v1 prefix', () {
      final bytes = fixture('device_info_v1_nominal').sublist(0, 47);
      bytes[0] = 2;
      expect(
        () => ProtocolCodecs.decodeDeviceInfo(bytes),
        throwsA(isA<ProtocolCodecException>()),
      );
    });

    test('invalid ASCII is rejected', () {
      final bytes = fixture('device_info_v1_nominal');
      bytes[4] = 0xFF;
      expect(
        () => ProtocolCodecs.decodeDeviceInfo(bytes),
        throwsA(isA<ProtocolCodecException>()),
      );
    });

    test('unknown enum and status values are retained as unknown', () {
      final telemetryBytes = fixture('telemetry_v1_moving');
      telemetryBytes[23] = 77;
      expect(
        ProtocolCodecs.decodeTelemetry(telemetryBytes).rideState,
        RideState.unknown,
      );

      final resultBytes = fixture('result_ok');
      resultBytes[2] = 77;
      expect(
        ProtocolCodecs.decodeCommandResult(resultBytes).status,
        CommandStatus.unknown,
      );
    });

    test('variable packets validate payload length', () {
      expect(
        () => ProtocolCodecs.decodeCommand(<int>[1, 1, 0, 2, 0]),
        throwsA(isA<ProtocolCodecException>()),
      );
      expect(
        () => ProtocolCodecs.decodeCommandResult(<int>[
          1,
          1,
          0,
          0,
          0,
          0,
          0,
          0,
          1,
        ]),
        throwsA(isA<ProtocolCodecException>()),
      );
    });
  });
}
