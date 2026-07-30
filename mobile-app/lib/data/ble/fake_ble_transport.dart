// ignore_for_file: prefer_initializing_formals

import 'dart:async';

import '../../domain/entities/models.dart';
import '../../domain/validators/config_validator.dart';
import '../protocol/ble_uuids.dart';
import '../protocol/protocol_codecs.dart';
import 'ble_transport.dart';

enum FakeBleScenario {
  normal,
  serviceMissing,
  mtuTooSmall,
  protocolMajor2,
  pairingClosed,
  writeTimeout,
  busyThenOk,
  rangeError,
  storageError,
  disconnectOnWrite,
}

enum FakeRideProfile { idle, moving, paused, lowBattery }

class FakeBleTransport implements BleTransport {
  FakeBleTransport({
    this.scenario = FakeBleScenario.normal,
    FakeRideProfile profile = FakeRideProfile.moving,
  }) : _profile = profile;

  FakeBleScenario scenario;
  FakeRideProfile _profile;
  bool _connected = false;
  bool _sensorTest = false;
  int _busyResponses = 0;
  Timer? _telemetryTimer;
  StreamController<BleLinkState>? _linkController;
  final Map<String, StreamController<List<int>>> _notifications = {};

  DeviceConfig _config = DeviceConfig.defaults;
  Telemetry _telemetry = const Telemetry(
    structVersion: 1,
    flags: 35,
    speedX100: 2550,
    avgSpeedX100: 2200,
    maxSpeedX100: 3500,
    tripDistanceCm: 125000,
    movingTimeS: 1800,
    odometerM: 123456,
    batteryMv: 3900,
    batteryPct: 75,
    rideState: RideState.moving,
    revolutions: 500,
    lastPulseAgeMs: 250,
    seq: 42,
    sensorState: SensorState.ok,
    powerState: PowerState.active,
  );

  DeviceInfo get _deviceInfo => DeviceInfo(
    structVersion: 1,
    protoMajor: scenario == FakeBleScenario.protocolMajor2 ? 2 : 1,
    protoMinor: 0,
    hwRevision: 1,
    model: 'BIKECOMP-XIAO',
    fwVersion: '1.0.0-fake',
    serial: const [1, 2, 3, 4, 5, 6, 7, 8],
    uptimeS: 3600,
    resetReason: ResetReason.powerOn,
    bootCount: 42,
    flags: scenario == FakeBleScenario.pairingClosed ? 0x0F : 0x1F,
  );

  @override
  Stream<BleAdapterState> get adapterState =>
      Stream.value(BleAdapterState.ready);

  @override
  Stream<BleScanResult> scan() async* {
    await Future<void>.delayed(const Duration(milliseconds: 40));
    yield const BleScanResult(
      deviceId: 'FA:KE:BI:KE:00:01',
      name: 'BikeComp-FAKE',
      rssi: -58,
      bondState: BondState.bonded,
    );
  }

  @override
  Future<void> stopScan() async {}

  @override
  Stream<BleLinkState> connect(String deviceId) {
    final controller = StreamController<BleLinkState>.broadcast();
    _linkController = controller;
    Timer.run(() async {
      controller.add(BleLinkState.connecting);
      await Future<void>.delayed(const Duration(milliseconds: 40));
      _connected = true;
      _restartTelemetryTimer();
      controller.add(BleLinkState.connected);
    });
    return controller.stream;
  }

  @override
  Future<void> disconnect() async {
    _connected = false;
    final controller = _linkController;
    _linkController = null;
    if (controller != null && !controller.isClosed) {
      controller.add(BleLinkState.disconnected);
      await controller.close();
    }
  }

  void _requireConnected() {
    if (!_connected) throw StateError('Fake BLE device is disconnected');
  }

  @override
  Future<int> requestMtu(int mtu) async {
    _requireConnected();
    return scenario == FakeBleScenario.mtuTooSmall ? 23 : 247;
  }

  @override
  Future<BleDiscovery> discoverServices() async {
    _requireConnected();
    if (scenario == FakeBleScenario.serviceMissing) {
      return const BleDiscovery(<String>{});
    }
    return const BleDiscovery(BleUuids.requiredCharacteristics);
  }

  @override
  Future<List<int>> read(String characteristicUuid) async {
    _requireConnected();
    return switch (characteristicUuid) {
      BleUuids.deviceInfo => ProtocolCodecs.encodeDeviceInfo(_deviceInfo),
      BleUuids.telemetry => ProtocolCodecs.encodeTelemetry(
        _profiledTelemetry(),
      ),
      BleUuids.configRead => ProtocolCodecs.encodeConfig(_config),
      BleUuids.commandResult => ProtocolCodecs.encodeCommandResult(
        const CommandResult(
          structVersion: 1,
          commandId: DeviceCommandId.unknown,
          status: CommandStatus.ok,
          detail: 0,
          token: 0,
          payload: <int>[],
        ),
      ),
      _ => throw StateError(
        'Unsupported fake characteristic $characteristicUuid',
      ),
    };
  }

  @override
  Future<void> writeWithResponse(
    String characteristicUuid,
    List<int> value,
  ) async {
    _requireConnected();
    if (scenario == FakeBleScenario.disconnectOnWrite) {
      _connected = false;
      _linkController?.add(BleLinkState.disconnected);
      throw StateError('Injected disconnect');
    }
    if (characteristicUuid == BleUuids.configWrite) {
      await _writeConfig(value);
      return;
    }
    if (characteristicUuid == BleUuids.command) {
      await _writeCommand(value);
      return;
    }
    throw StateError('Unsupported fake write $characteristicUuid');
  }

  Future<void> _writeConfig(List<int> bytes) async {
    final value = ProtocolCodecs.decodeConfig(bytes);
    if (scenario == FakeBleScenario.busyThenOk && _busyResponses < 3) {
      _busyResponses++;
      _emitResult(DeviceCommandId.configWrite, CommandStatus.busy);
      return;
    }
    if (scenario == FakeBleScenario.rangeError) {
      _emitResult(DeviceCommandId.configWrite, CommandStatus.range, detail: 2);
      return;
    }
    if (scenario == FakeBleScenario.storageError) {
      _emitResult(DeviceCommandId.configWrite, CommandStatus.storage);
      return;
    }
    final validation = ConfigValidator.validate(value);
    if (!validation.isValid) {
      _emitResult(
        DeviceCommandId.configWrite,
        CommandStatus.range,
        detail: validation.issues.first.fieldId,
      );
      return;
    }
    _config = value;
    _emit(BleUuids.configRead, ProtocolCodecs.encodeConfig(_config));
    if (scenario != FakeBleScenario.writeTimeout) {
      _emitResult(DeviceCommandId.configWrite, CommandStatus.ok);
    }
  }

  Future<void> _writeCommand(List<int> bytes) async {
    final command = ProtocolCodecs.decodeCommand(bytes);
    switch (command.id) {
      case DeviceCommandId.resetTrip:
        _telemetry = _telemetry.copyWith(
          speedX100: 0,
          avgSpeedX100: 0,
          maxSpeedX100: 0,
          tripDistanceCm: 0,
          movingTimeS: 0,
          revolutions: 0,
        );
      case DeviceCommandId.displayOn:
        _telemetry = _telemetry.copyWith(flags: _telemetry.flags | 0x02);
      case DeviceCommandId.displayOff:
        _telemetry = _telemetry.copyWith(flags: _telemetry.flags & ~0x02);
      case DeviceCommandId.sensorTestStart:
        _sensorTest = true;
        _restartTelemetryTimer();
      case DeviceCommandId.sensorTestStop:
        _sensorTest = false;
        _restartTelemetryTimer();
      default:
        break;
    }
    _emitResult(command.id, CommandStatus.ok);
    _emit(
      BleUuids.telemetry,
      ProtocolCodecs.encodeTelemetry(_profiledTelemetry()),
    );
  }

  void _emitResult(
    DeviceCommandId command,
    CommandStatus status, {
    int detail = 0,
  }) {
    _emit(
      BleUuids.commandResult,
      ProtocolCodecs.encodeCommandResult(
        CommandResult(
          structVersion: 1,
          commandId: command,
          status: status,
          detail: detail,
          token: 0,
          payload: const <int>[],
        ),
      ),
    );
  }

  void _emit(String uuid, List<int> bytes) {
    final controller = _notifications[uuid];
    if (controller != null && !controller.isClosed) controller.add(bytes);
  }

  @override
  Stream<List<int>> subscribe(String characteristicUuid) => _notifications
      .putIfAbsent(characteristicUuid, StreamController<List<int>>.broadcast)
      .stream;

  Telemetry _profiledTelemetry() {
    final seq = (_telemetry.seq + 1) & 0xFFFF;
    _telemetry = switch (_profile) {
      FakeRideProfile.idle => _telemetry.copyWith(
        flags: 0x02,
        speedX100: 0,
        rideState: RideState.idle,
        sensorState: SensorState.noSignal,
        powerState: PowerState.shortStop,
        lastPulseAgeMs: 0xFFFFFFFF,
        seq: seq,
      ),
      FakeRideProfile.paused => _telemetry.copyWith(
        flags: 0x22,
        speedX100: 0,
        rideState: RideState.paused,
        sensorState: SensorState.idle,
        powerState: PowerState.shortStop,
        lastPulseAgeMs: 5000,
        seq: seq,
      ),
      FakeRideProfile.lowBattery => _telemetry.copyWith(
        flags: 0x33,
        batteryMv: 3420,
        batteryPct: 12,
        seq: seq,
      ),
      FakeRideProfile.moving => _telemetry.copyWith(
        flags: 0x23,
        speedX100: 2550,
        tripDistanceCm: _telemetry.tripDistanceCm + 70,
        movingTimeS: _telemetry.movingTimeS + 1,
        revolutions: _telemetry.revolutions + 1,
        lastPulseAgeMs: 250,
        rideState: RideState.moving,
        sensorState: SensorState.ok,
        powerState: PowerState.active,
        seq: seq,
      ),
    };
    return _telemetry;
  }

  void setRideProfile(FakeRideProfile profile) {
    _profile = profile;
  }

  void _restartTelemetryTimer() {
    _telemetryTimer?.cancel();
    _telemetryTimer = Timer.periodic(
      Duration(milliseconds: _sensorTest ? 200 : 1000),
      (_) {
        if (_connected) {
          _emit(
            BleUuids.telemetry,
            ProtocolCodecs.encodeTelemetry(_profiledTelemetry()),
          );
        }
      },
    );
  }

  @override
  Future<int> readRssi() async {
    _requireConnected();
    return -58;
  }

  @override
  Future<void> dispose() async {
    _telemetryTimer?.cancel();
    await disconnect();
    for (final controller in _notifications.values) {
      await controller.close();
    }
    _notifications.clear();
  }
}
