import 'dart:async';

import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import '../protocol/ble_uuids.dart';
import 'ble_transport.dart';

class ReactiveBleTransport implements BleTransport {
  ReactiveBleTransport({FlutterReactiveBle? ble})
    : _ble = ble ?? FlutterReactiveBle();

  final FlutterReactiveBle _ble;
  String? _deviceId;
  StreamSubscription<ConnectionStateUpdate>? _connectionSubscription;
  StreamSubscription<DiscoveredDevice>? _scanSubscription;
  StreamController<BleScanResult>? _scanController;
  StreamController<BleLinkState>? _connectionController;

  @override
  Stream<BleAdapterState> get adapterState =>
      _ble.statusStream.map(_mapAdapterState).distinct();

  BleAdapterState _mapAdapterState(BleStatus status) => switch (status) {
    BleStatus.ready => BleAdapterState.ready,
    BleStatus.poweredOff => BleAdapterState.poweredOff,
    BleStatus.unauthorized => BleAdapterState.unauthorized,
    BleStatus.locationServicesDisabled => BleAdapterState.locationServicesOff,
    BleStatus.unsupported => BleAdapterState.unsupported,
    _ => BleAdapterState.unknown,
  };

  @override
  Stream<BleScanResult> scan() {
    unawaited(stopScan());
    final controller = StreamController<BleScanResult>.broadcast();
    _scanController = controller;
    _scanSubscription = _ble
        .scanForDevices(
          withServices: [Uuid.parse(BleUuids.service)],
          scanMode: ScanMode.lowLatency,
          requireLocationServicesEnabled: false,
        )
        .listen(
          (device) => controller.add(
            BleScanResult(
              deviceId: device.id,
              name: device.name.isEmpty ? 'BikeComp' : device.name,
              rssi: device.rssi,
            ),
          ),
          onError: controller.addError,
        );
    controller.onCancel = stopScan;
    return controller.stream;
  }

  @override
  Future<void> stopScan() async {
    await _scanSubscription?.cancel();
    _scanSubscription = null;
    final controller = _scanController;
    _scanController = null;
    if (controller != null && !controller.isClosed) await controller.close();
  }

  @override
  Stream<BleLinkState> connect(String deviceId) {
    unawaited(disconnect());
    _deviceId = deviceId;
    final controller = StreamController<BleLinkState>.broadcast();
    _connectionController = controller;
    controller.add(BleLinkState.connecting);
    _connectionSubscription = _ble
        .connectToAdvertisingDevice(
          id: deviceId,
          withServices: [Uuid.parse(BleUuids.service)],
          prescanDuration: const Duration(seconds: 5),
          servicesWithCharacteristicsToDiscover: {
            Uuid.parse(BleUuids.service): BleUuids.requiredCharacteristics
                .map(Uuid.parse)
                .toList(),
          },
          connectionTimeout: const Duration(seconds: 10),
        )
        .listen(
          (update) => controller.add(switch (update.connectionState) {
            DeviceConnectionState.connecting => BleLinkState.connecting,
            DeviceConnectionState.connected => BleLinkState.connected,
            DeviceConnectionState.disconnecting => BleLinkState.disconnecting,
            DeviceConnectionState.disconnected => BleLinkState.disconnected,
          }),
          onError: controller.addError,
          onDone: () {
            if (!controller.isClosed) controller.add(BleLinkState.disconnected);
          },
        );
    return controller.stream;
  }

  @override
  Future<void> disconnect() async {
    await _connectionSubscription?.cancel();
    _connectionSubscription = null;
    final controller = _connectionController;
    _connectionController = null;
    if (controller != null && !controller.isClosed) {
      controller.add(BleLinkState.disconnected);
      await controller.close();
    }
    _deviceId = null;
  }

  String get _connectedDeviceId {
    final id = _deviceId;
    if (id == null) throw StateError('BLE device is not connected');
    return id;
  }

  QualifiedCharacteristic _characteristic(String uuid) =>
      QualifiedCharacteristic(
        serviceId: Uuid.parse(BleUuids.service),
        characteristicId: Uuid.parse(uuid),
        deviceId: _connectedDeviceId,
      );

  @override
  Future<int> requestMtu(int mtu) =>
      _ble.requestMtu(deviceId: _connectedDeviceId, mtu: mtu);

  @override
  Future<BleDiscovery> discoverServices() async {
    await _ble.discoverAllServices(_connectedDeviceId);
    final services = await _ble.getDiscoveredServices(_connectedDeviceId);
    final characteristics = <String>{};
    for (final service in services) {
      if (service.id.toString().toLowerCase() != BleUuids.service) {
        continue;
      }
      for (final characteristic in service.characteristics) {
        characteristics.add(characteristic.id.toString().toLowerCase());
      }
    }
    return BleDiscovery(characteristics);
  }

  @override
  Future<List<int>> read(String characteristicUuid) =>
      _ble.readCharacteristic(_characteristic(characteristicUuid));

  @override
  Future<void> writeWithResponse(String characteristicUuid, List<int> value) =>
      _ble.writeCharacteristicWithResponse(
        _characteristic(characteristicUuid),
        value: value,
      );

  @override
  Stream<List<int>> subscribe(String characteristicUuid) =>
      _ble.subscribeToCharacteristic(_characteristic(characteristicUuid));

  @override
  Future<int> readRssi() => _ble.readRssi(_connectedDeviceId);

  @override
  Future<void> dispose() async {
    await stopScan();
    await disconnect();
    await _ble.deinitialize();
  }
}
