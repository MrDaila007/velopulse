import 'package:bikecomp_mobile/application/app_states.dart';
import 'package:bikecomp_mobile/application/providers.dart';
import 'package:bikecomp_mobile/data/ble/ble_transport.dart';
import 'package:bikecomp_mobile/data/ble/fake_ble_transport.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  test(
    'connection FSM reaches ready through all fake synchronization stages',
    () async {
      SharedPreferencesAsyncPlatform.instance =
          InMemorySharedPreferencesAsync.empty();
      final fake = FakeBleTransport();
      final container = ProviderContainer(
        overrides: [bleTransportProvider.overrideWithValue(fake)],
      );
      addTearDown(() async {
        container.dispose();
        await fake.dispose();
      });
      final states = <ConnectionState>[];
      final subscription = container.listen(
        connectionControllerProvider,
        (previous, next) => states.add(next.connection),
        fireImmediately: true,
      );
      addTearDown(subscription.close);

      await container
          .read(connectionControllerProvider.notifier)
          .connectDevice(
            const BleScanResult(
              deviceId: 'FA:KE:BI:KE:00:01',
              name: 'BikeComp-FAKE',
              rssi: -58,
            ),
          );

      final session = container.read(connectionControllerProvider);
      expect(session.connection, isA<ConnectionReady>());
      expect(session.deviceInfo?.protoMajor, 1);
      expect(session.deviceConfig, isNotNull);
      expect(session.telemetry, isNotNull);
      expect(states, contains(isA<ConnectionConnecting>()));
      expect(states, contains(isA<ConnectionSynchronizing>()));
    },
  );

  test('protocol major 2 remains connected but write-blocked', () async {
    SharedPreferencesAsyncPlatform.instance =
        InMemorySharedPreferencesAsync.empty();
    final fake = FakeBleTransport(scenario: FakeBleScenario.protocolMajor2);
    final container = ProviderContainer(
      overrides: [bleTransportProvider.overrideWithValue(fake)],
    );
    addTearDown(() async {
      container.dispose();
      await fake.dispose();
    });

    await container
        .read(connectionControllerProvider.notifier)
        .connectDevice(
          const BleScanResult(
            deviceId: 'FA:KE:BI:KE:00:01',
            name: 'BikeComp-FAKE',
            rssi: -58,
          ),
        );

    final session = container.read(connectionControllerProvider);
    expect(session.connection, isA<ConnectionIncompatibleProtocol>());
    expect(session.deviceInfo?.protoMajor, 2);
    expect(session.deviceConfig, isNull);
  });
}
