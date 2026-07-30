import 'package:bikecomp_mobile/application/app_states.dart';
import 'package:bikecomp_mobile/application/providers.dart';
import 'package:bikecomp_mobile/core/app_error.dart';
import 'package:bikecomp_mobile/data/ble/ble_transport.dart';
import 'package:bikecomp_mobile/data/ble/fake_ble_transport.dart';
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
  FakeBleScenario scenario,
) async {
  SharedPreferencesAsyncPlatform.instance =
      InMemorySharedPreferencesAsync.empty();
  final fake = FakeBleTransport(scenario: scenario);
  final container = ProviderContainer(
    overrides: [bleTransportProvider.overrideWithValue(fake)],
  );
  await container
      .read(connectionControllerProvider.notifier)
      .connectDevice(fakeDevice);
  return (container, fake);
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

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
    },
  );
}
