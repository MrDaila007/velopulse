import '../../domain/entities/models.dart';

enum BleAdapterState {
  ready,
  poweredOff,
  unauthorized,
  locationServicesOff,
  unsupported,
  unknown,
}

enum BleLinkState { connecting, connected, disconnecting, disconnected }

class BleScanResult {
  const BleScanResult({
    required this.deviceId,
    required this.name,
    required this.rssi,
    this.bondState = BondState.unknown,
  });

  final String deviceId;
  final String name;
  final int rssi;
  final BondState bondState;

  BleScanResult copyWith({BondState? bondState, int? rssi, String? name}) =>
      BleScanResult(
        deviceId: deviceId,
        name: name ?? this.name,
        rssi: rssi ?? this.rssi,
        bondState: bondState ?? this.bondState,
      );
}

class BleDiscovery {
  const BleDiscovery(this.characteristicUuids);
  final Set<String> characteristicUuids;
}

abstract interface class BleTransport {
  Stream<BleAdapterState> get adapterState;
  Stream<BleScanResult> scan();
  Future<void> stopScan();
  Stream<BleLinkState> connect(String deviceId);
  Future<void> disconnect();
  Future<int> requestMtu(int mtu);
  Future<BleDiscovery> discoverServices();
  Future<List<int>> read(String characteristicUuid);
  Future<void> writeWithResponse(String characteristicUuid, List<int> value);
  Stream<List<int>> subscribe(String characteristicUuid);
  Future<int> readRssi();
  Future<void> dispose();
}
