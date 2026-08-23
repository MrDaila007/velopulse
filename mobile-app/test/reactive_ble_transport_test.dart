import 'dart:async';
import 'dart:typed_data';

import 'package:bikecomp_mobile/data/ble/ble_transport.dart';
import 'package:bikecomp_mobile/data/ble/reactive_ble_transport.dart';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:mocktail/mocktail.dart';

class _MockReactiveBle extends Mock implements FlutterReactiveBle {}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUpAll(() {
    registerFallbackValue(<Uuid>[]);
    registerFallbackValue(<Uuid, List<Uuid>>{});
  });

  test('scan stays active and filters an unfiltered platform stream', () async {
    final ble = _MockReactiveBle();
    var platformScanCancelled = false;
    final platformScan = StreamController<DiscoveredDevice>.broadcast(
      onCancel: () => platformScanCancelled = true,
    );
    when(
      () => ble.scanForDevices(
        withServices: any(named: 'withServices'),
        scanMode: ScanMode.lowLatency,
        requireLocationServicesEnabled: false,
      ),
    ).thenAnswer((_) => platformScan.stream);
    final transport = ReactiveBleTransport(ble: ble);

    final resultFuture = transport.scan().first;
    platformScan.add(
      DiscoveredDevice(
        id: 'AA:BB:CC:DD:EE:FF',
        name: 'BikeComp-D210',
        serviceData: const {},
        manufacturerData: Uint8List(0),
        rssi: -51,
        serviceUuids: const [],
      ),
    );

    final result = await resultFuture.timeout(const Duration(seconds: 1));
    expect(result.deviceId, 'AA:BB:CC:DD:EE:FF');
    expect(result.name, 'BikeComp-D210');
    expect(result.rssi, -51);
    final capturedServices =
        verify(
              () => ble.scanForDevices(
                withServices: captureAny(named: 'withServices'),
                scanMode: ScanMode.lowLatency,
                requireLocationServicesEnabled: false,
              ),
            ).captured.single
            as List<Uuid>;
    expect(capturedServices, isEmpty);
    expect(platformScanCancelled, isTrue);
    await platformScan.close();
  });

  test('cancelling link stream disconnects the platform connection', () async {
    final ble = _MockReactiveBle();
    var platformConnectionCancelled = false;
    final platformConnection =
        StreamController<ConnectionStateUpdate>.broadcast(
          onCancel: () => platformConnectionCancelled = true,
        );
    when(
      () => ble.connectToAdvertisingDevice(
        id: 'AA:BB:CC:DD:EE:FF',
        withServices: any(named: 'withServices'),
        prescanDuration: const Duration(seconds: 8),
        servicesWithCharacteristicsToDiscover: any(
          named: 'servicesWithCharacteristicsToDiscover',
        ),
        connectionTimeout: const Duration(seconds: 15),
      ),
    ).thenAnswer((_) => platformConnection.stream);
    final transport = ReactiveBleTransport(ble: ble);
    final states = <BleLinkState>[];

    final subscription = transport
        .connect('AA:BB:CC:DD:EE:FF')
        .listen(states.add);
    platformConnection.add(
      const ConnectionStateUpdate(
        deviceId: 'AA:BB:CC:DD:EE:FF',
        connectionState: DeviceConnectionState.connected,
        failure: null,
      ),
    );
    await Future<void>.delayed(Duration.zero);
    expect(states, contains(BleLinkState.connected));

    await subscription.cancel().timeout(const Duration(seconds: 1));
    expect(platformConnectionCancelled, isTrue);
    final capturedServices =
        verify(
              () => ble.connectToAdvertisingDevice(
                id: 'AA:BB:CC:DD:EE:FF',
                withServices: captureAny(named: 'withServices'),
                prescanDuration: const Duration(seconds: 8),
                servicesWithCharacteristicsToDiscover: any(
                  named: 'servicesWithCharacteristicsToDiscover',
                ),
                connectionTimeout: const Duration(seconds: 15),
              ),
            ).captured.single
            as List<Uuid>;
    expect(capturedServices, isEmpty);
    await platformConnection.close();
  });

  Future<ReactiveBleTransport> connectForMtu(
    _MockReactiveBle ble,
    StreamController<ConnectionStateUpdate> platformConnection,
  ) async {
    when(
      () => ble.connectToAdvertisingDevice(
        id: 'AA:BB:CC:DD:EE:FF',
        withServices: any(named: 'withServices'),
        prescanDuration: const Duration(seconds: 8),
        servicesWithCharacteristicsToDiscover: any(
          named: 'servicesWithCharacteristicsToDiscover',
        ),
        connectionTimeout: const Duration(seconds: 15),
      ),
    ).thenAnswer((_) => platformConnection.stream);
    final transport = ReactiveBleTransport(
      ble: ble,
      mtuSettleDelay: Duration.zero,
      mtuRetryDelay: Duration.zero,
    );
    transport.connect('AA:BB:CC:DD:EE:FF').listen((_) {});
    platformConnection.add(
      const ConnectionStateUpdate(
        deviceId: 'AA:BB:CC:DD:EE:FF',
        connectionState: DeviceConnectionState.connected,
        failure: null,
      ),
    );
    await Future<void>.delayed(Duration.zero);
    return transport;
  }

  test('requestMtu retries once after a transient platform failure', () async {
    final ble = _MockReactiveBle();
    final platformConnection =
        StreamController<ConnectionStateUpdate>.broadcast();
    addTearDown(platformConnection.close);
    var calls = 0;
    when(
      () => ble.requestMtu(
        deviceId: any(named: 'deviceId'),
        mtu: any(named: 'mtu'),
      ),
    ).thenAnswer((_) async {
      calls++;
      if (calls == 1) throw StateError('transient MTU');
      return 247;
    });
    final transport = await connectForMtu(ble, platformConnection);

    expect(await transport.requestMtu(247), 247);
    expect(calls, 2);
  });

  test(
    'requestMtu keeps the session if both attempts fail while connected',
    () async {
      final ble = _MockReactiveBle();
      final platformConnection =
          StreamController<ConnectionStateUpdate>.broadcast();
      addTearDown(platformConnection.close);
      when(
        () => ble.requestMtu(
          deviceId: any(named: 'deviceId'),
          mtu: any(named: 'mtu'),
        ),
      ).thenThrow(StateError('MTU rejected'));
      final transport = await connectForMtu(ble, platformConnection);

      expect(await transport.requestMtu(247), 247);
    },
  );

  test('requestMtu fails when the link drops during negotiation', () async {
    final ble = _MockReactiveBle();
    final platformConnection =
        StreamController<ConnectionStateUpdate>.broadcast();
    addTearDown(platformConnection.close);
    final transport = await connectForMtu(ble, platformConnection);
    when(
      () => ble.requestMtu(
        deviceId: any(named: 'deviceId'),
        mtu: any(named: 'mtu'),
      ),
    ).thenAnswer((_) async {
      platformConnection.add(
        const ConnectionStateUpdate(
          deviceId: 'AA:BB:CC:DD:EE:FF',
          connectionState: DeviceConnectionState.disconnected,
          failure: null,
        ),
      );
      await Future<void>.delayed(Duration.zero);
      throw StateError('GATT 133');
    });

    await expectLater(transport.requestMtu(247), throwsA(isA<StateError>()));
  });
}
