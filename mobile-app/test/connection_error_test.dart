import 'package:bikecomp_mobile/application/app_states.dart';
import 'package:bikecomp_mobile/application/providers.dart';
import 'package:bikecomp_mobile/core/app_error.dart';
import 'package:bikecomp_mobile/data/ble/ble_transport.dart';
import 'package:bikecomp_mobile/data/ble/fake_ble_transport.dart';
import 'package:bikecomp_mobile/data/protocol/ble_uuids.dart';
import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

const fakeDevice = BleScanResult(
  deviceId: 'FA:KE:BI:KE:00:01',
  name: 'BikeComp-FAKE',
  rssi: -58,
);

Future<(ProviderContainer, FakeBleTransport)> connectWith(
  FakeBleScenario scenario, {
  BleScanResult device = fakeDevice,
}) async {
  SharedPreferencesAsyncPlatform.instance =
      InMemorySharedPreferencesAsync.empty();
  final fake = FakeBleTransport(scenario: scenario);
  final container = ProviderContainer(
    overrides: [bleTransportProvider.overrideWithValue(fake)],
  );
  await container
      .read(connectionControllerProvider.notifier)
      .connectDevice(device);
  return (container, fake);
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test('sync reads Device Info before protected GATT operations', () async {
    final (container, fake) = await connectWith(FakeBleScenario.normal);
    addTearDown(() async {
      container.dispose();
      await fake.dispose();
    });

    final deviceInfoRead = fake.operationLog.indexOf(
      'read:${BleUuids.deviceInfo}',
    );
    final configRead = fake.operationLog.indexOf('read:${BleUuids.configRead}');
    final firstProtectedSubscription = fake.operationLog.indexWhere(
      (entry) => entry.startsWith('subscribe:'),
    );
    final telemetryRead = fake.operationLog.indexOf(
      'read:${BleUuids.telemetry}',
    );

    expect(deviceInfoRead, greaterThanOrEqualTo(0));
    expect(configRead, greaterThan(deviceInfoRead));
    expect(firstProtectedSubscription, greaterThan(configRead));
    expect(telemetryRead, greaterThan(firstProtectedSubscription));
    expect(
      container.read(connectionControllerProvider).connection,
      isA<ConnectionReady>(),
    );
  });

  test(
    'MTU request exception still reaches ready if the link stays up',
    () async {
      final (container, fake) = await connectWith(FakeBleScenario.mtuThrows);
      addTearDown(() async {
        container.dispose();
        await fake.dispose();
      });

      final session = container.read(connectionControllerProvider);
      expect(session.connection, isA<ConnectionReady>());
      expect(fake.operationLog, contains('mtu:247'));
      expect(fake.connected, isTrue);
    },
  );

  test('MTU disconnect is recovered by a single reconnect path', () async {
    final (container, fake) = await connectWith(
      FakeBleScenario.disconnectOnMtu,
    );
    addTearDown(() async {
      container.dispose();
      await fake.dispose();
    });

    await Future<void>.delayed(const Duration(milliseconds: 20));
    final session = container.read(connectionControllerProvider);
    expect(session.connection, isA<ConnectionReconnecting>());
    expect((session.connection as ConnectionReconnecting).attempt, 1);
  });

  test('MTU below 51 reaches read-only with domain data available', () async {
    final (container, fake) = await connectWith(FakeBleScenario.mtuTooSmall);
    addTearDown(() async {
      container.dispose();
      await fake.dispose();
    });

    final session = container.read(connectionControllerProvider);
    expect(session.connection, isA<ConnectionReadOnly>());
    expect(session.deviceInfo, isNotNull);
    expect(session.deviceConfig, isNotNull);
    expect(session.telemetry, isNotNull);
  });

  test(
    'missing service is a typed terminal failure without reconnect',
    () async {
      final (container, fake) = await connectWith(
        FakeBleScenario.serviceMissing,
      );
      addTearDown(() async {
        container.dispose();
        await fake.dispose();
      });

      final session = container.read(connectionControllerProvider);
      expect(session.connection, isA<ConnectionFailed>());
      expect(session.lastError?.kind, AppErrorKind.serviceMissing);
      expect(fake.connected, isFalse);
    },
  );

  test('closed pairing window gives typed terminal not-paired error', () async {
    final (container, fake) = await connectWith(
      FakeBleScenario.pairingClosed,
      device: fakeDevice.copyWith(bondState: BondState.none),
    );
    addTearDown(() async {
      container.dispose();
      await fake.dispose();
    });

    final session = container.read(connectionControllerProvider);
    expect(session.connection, isA<ConnectionFailed>());
    expect(session.lastError?.kind, AppErrorKind.notPaired);
    expect(session.deviceInfo?.pairingWindowOpen, isFalse);
    expect(fake.operationLog, contains('read:${BleUuids.deviceInfo}'));
    expect(fake.operationLog, isNot(contains('read:${BleUuids.configRead}')));
    expect(
      fake.operationLog.any((entry) => entry.startsWith('subscribe:')),
      isFalse,
    );
    expect(fake.connected, isFalse);
    await Future<void>.delayed(const Duration(milliseconds: 50));
    expect(
      container.read(connectionControllerProvider).connection,
      isA<ConnectionFailed>(),
    );
  });
}
