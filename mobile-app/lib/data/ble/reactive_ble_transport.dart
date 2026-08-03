import 'dart:async';

import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';

import '../protocol/ble_uuids.dart';
import 'ble_discovery_filter.dart';
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
  final Map<String, DiscoveredDevice> _latestScanSightings =
      <String, DiscoveredDevice>{};

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

  DiscoveredDevice _mergeScanSighting(DiscoveredDevice device) {
    final previous = _latestScanSightings[device.id];
    if (previous == null) {
      _latestScanSightings[device.id] = device;
      return device;
    }
    final merged = DiscoveredDevice(
      id: device.id,
      name: device.name.isNotEmpty ? device.name : previous.name,
      serviceData: device.serviceData.isNotEmpty
          ? device.serviceData
          : previous.serviceData,
      manufacturerData: device.manufacturerData.isNotEmpty
          ? device.manufacturerData
          : previous.manufacturerData,
      rssi: device.rssi,
      serviceUuids: device.serviceUuids.isNotEmpty
          ? device.serviceUuids
          : previous.serviceUuids,
    );
    _latestScanSightings[device.id] = merged;
    return merged;
  }

  @override
  Stream<BleScanResult> scan() {
    final controller = StreamController<BleScanResult>.broadcast();
    _scanController = controller;
    _latestScanSightings.clear();
    _scanSubscription = _ble
        .scanForDevices(
          // Filter in Dart: native Android filters can drop advertisements
          // containing a vendor-specific 128-bit service UUID.
          withServices: const [],
          scanMode: ScanMode.lowLatency,
          requireLocationServicesEnabled: false,
        )
        .listen((device) {
          final merged = _mergeScanSighting(device);
          if (!isBikeCompAdvertisement(
            name: merged.name,
            serviceUuids: merged.serviceUuids.map((uuid) => uuid.toString()),
          )) {
            return;
          }
          controller.add(
            BleScanResult(
              deviceId: merged.id,
              name: merged.name.isEmpty ? 'BikeComp' : merged.name,
              rssi: merged.rssi,
            ),
          );
        }, onError: controller.addError);
    controller.onCancel = stopScan;
    return controller.stream;
  }

  @override
  Future<void> stopScan() async {
    await _scanSubscription?.cancel();
    _scanSubscription = null;
    _latestScanSightings.clear();
    final controller = _scanController;
    _scanController = null;
    if (controller != null && !controller.isClosed) await controller.close();
  }

  @override
  Stream<BleLinkState> connect(String deviceId) {
    _deviceId = deviceId;
    final controller = StreamController<BleLinkState>.broadcast();
    _connectionController = controller;
    controller.add(BleLinkState.connecting);
    _connectionSubscription = _ble
        .connectToAdvertisingDevice(
          id: deviceId,
          withServices: const [],
          prescanDuration: const Duration(seconds: 8),
          servicesWithCharacteristicsToDiscover: {
            Uuid.parse(BleUuids.service): <Uuid>[
              ...BleUuids.requiredCharacteristics.map(Uuid.parse),
              Uuid.parse(BleUuids.companionWrite),
            ],
          },
          connectionTimeout: const Duration(seconds: 15),
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
    controller.onCancel = disconnect;
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
