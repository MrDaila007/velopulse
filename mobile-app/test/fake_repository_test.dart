import 'package:bikecomp_mobile/core/app_error.dart';
import 'package:bikecomp_mobile/core/result.dart';
import 'package:bikecomp_mobile/data/bike_computer_repository.dart';
import 'package:bikecomp_mobile/data/ble/ble_transport.dart';
import 'package:bikecomp_mobile/data/ble/fake_ble_transport.dart';
import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:flutter_test/flutter_test.dart';

Future<(FakeBleTransport, BikeComputerRepositoryImpl)> connectedRepository({
  FakeBleScenario scenario = FakeBleScenario.normal,
}) async {
  final transport = FakeBleTransport(scenario: scenario);
  await transport
      .connect('FA:KE:BI:KE:00:01')
      .firstWhere((state) => state == BleLinkState.connected);
  final repository = BikeComputerRepositoryImpl(transport);
  await repository.start();
  return (transport, repository);
}

void main() {
  test('fake exposes domain objects and confirmed commands', () async {
    final (transport, repository) = await connectedRepository();
    addTearDown(() async {
      await repository.dispose();
      await transport.dispose();
    });

    final info = await repository.readDeviceInfo();
    final config = await repository.readConfig();
    final telemetry = await repository.readTelemetry();
    final command = await repository.sendCommand(
      const DeviceCommand(id: DeviceCommandId.displayOff),
    );

    expect((info as Success<DeviceInfo>).value.protoMajor, 1);
    expect((config as Success<DeviceConfig>).value, DeviceConfig.defaults);
    expect((telemetry as Success<Telemetry>).value.speedX100, 2550);
    expect((command as Success<CommandResult>).value.status, CommandStatus.ok);
  });

  test('config write sends full value and verifies notification', () async {
    final (transport, repository) = await connectedRepository();
    addTearDown(() async {
      await repository.dispose();
      await transport.dispose();
    });
    await repository.readConfig();
    final changed = DeviceConfig.defaults.copyWith(
      wheelCircumferenceMm: 2136,
      brightnessPct: 80,
    );

    final result = await repository.writeConfig(changed);
    final reread = await repository.readConfig();

    expect(result, isA<Success<void>>());
    expect((reread as Success<DeviceConfig>).value, changed);
  });

  test('timeout plus matching reread is recovered confirmation', () async {
    final (transport, repository) = await connectedRepository(
      scenario: FakeBleScenario.writeTimeout,
    );
    addTearDown(() async {
      await repository.dispose();
      await transport.dispose();
    });
    final changed = DeviceConfig.defaults.copyWith(brightnessPct: 75);

    final result = await repository.writeConfig(changed);

    expect(result, isA<Success<void>>());
  });

  test('device range rejection carries rejected field id', () async {
    final (transport, repository) = await connectedRepository(
      scenario: FakeBleScenario.rangeError,
    );
    addTearDown(() async {
      await repository.dispose();
      await transport.dispose();
    });

    final result = await repository.writeConfig(
      DeviceConfig.defaults.copyWith(wheelCircumferenceMm: 2200),
    );

    final error = (result as Failure<void>).error as AppError;
    expect(error.kind, AppErrorKind.deviceRejected);
    expect(error.fieldId, 2);
  });

  test('disconnect during write never reports success', () async {
    final (transport, repository) = await connectedRepository(
      scenario: FakeBleScenario.disconnectOnWrite,
    );
    addTearDown(() async {
      await repository.dispose();
      await transport.dispose();
    });

    final result = await repository.writeConfig(
      DeviceConfig.defaults.copyWith(brightnessPct: 70),
    );

    expect(result, isA<Failure<void>>());
  });
}
