import 'dart:async';

import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../core/app_error.dart';
import '../core/result.dart';
import '../data/bike_computer_repository.dart';
import '../data/ble/ble_transport.dart';
import '../data/ble/fake_ble_transport.dart';
import '../data/ble/reactive_ble_transport.dart';
import '../data/local/preferences_store.dart';
import '../data/protocol/ble_uuids.dart';
import '../domain/entities/models.dart';
import '../domain/validators/config_validator.dart';
import '../platform/android_ble_platform.dart';
import 'app_states.dart';

part 'providers.g.dart';

const _useFakeBle = bool.fromEnvironment('BIKECOMP_FAKE_BLE');

@Riverpod(keepAlive: true)
BleTransport bleTransport(Ref ref) {
  final transport = _useFakeBle ? FakeBleTransport() : ReactiveBleTransport();
  ref.onDispose(() => unawaited(transport.dispose()));
  return transport;
}

@Riverpod(keepAlive: true)
PreferencesStore preferencesStore(Ref ref) => PreferencesStore();

@Riverpod(keepAlive: true)
AndroidBlePlatform androidBlePlatform(Ref ref) => const AndroidBlePlatform();

@Riverpod(keepAlive: true)
class ConfigDraftController extends _$ConfigDraftController {
  @override
  ConfigDraftState build() => const ConfigDraftState();

  PreferencesStore get _store => ref.read(preferencesStoreProvider);

  Future<void> loadDeviceConfig(String deviceId, DeviceConfig remote) async {
    final persisted = await _store.readDraft(deviceId);
    if (persisted == null || persisted.draft == remote) {
      if (persisted?.draft == remote) await _store.clearDraft(deviceId);
      state = ConfigDraftState(
        deviceId: deviceId,
        deviceConfig: remote,
        draft: remote,
        lastSyncedAt: DateTime.now(),
      );
      return;
    }

    state = ConfigDraftState(
      deviceId: deviceId,
      deviceConfig: remote,
      draft: persisted.draft,
      fieldErrors: ConfigValidator.validate(persisted.draft).fieldErrors,
      lastSyncedAt: DateTime.now(),
      hasConflict: persisted.base != remote,
    );
  }

  void update(DeviceConfig value) {
    state = state.copyWith(
      draft: value,
      fieldErrors: ConfigValidator.validate(value).fieldErrors,
      hasConflict: false,
    );
    unawaited(_persist());
  }

  void resetDefaults() => update(DeviceConfig.defaults);

  Future<void> resolveConflict({required bool keepDraft}) async {
    final remote = state.deviceConfig;
    if (remote == null) return;
    if (keepDraft) {
      state = state.copyWith(hasConflict: false);
      await _persist();
    } else {
      state = state.copyWith(
        draft: remote,
        fieldErrors: const <String, String>{},
        hasConflict: false,
      );
      final id = state.deviceId;
      if (id != null) await _store.clearDraft(id);
    }
  }

  Future<Result<void>> save() async {
    final draft = state.draft;
    if (draft == null || !state.isDirty || !state.isValid || state.isWriting) {
      return const Failure<void>(
        AppFailure(
          kind: AppErrorKind.validationFailed,
          message: 'Нет корректных несохранённых изменений',
        ),
      );
    }
    state = state.copyWith(isWriting: true);
    final result = await ref
        .read(connectionControllerProvider.notifier)
        .writeConfig(draft);
    if (result is Success<void>) {
      state = state.copyWith(
        deviceConfig: draft,
        draft: draft,
        isWriting: false,
        lastSyncedAt: DateTime.now(),
      );
      final id = state.deviceId;
      if (id != null) await _store.clearDraft(id);
    } else {
      state = state.copyWith(isWriting: false);
    }
    return result;
  }

  Future<void> persistNow() => _persist();

  Future<void> _persist() async {
    final id = state.deviceId;
    final base = state.deviceConfig;
    final draft = state.draft;
    if (id == null || base == null || draft == null) return;
    if (draft == base) {
      await _store.clearDraft(id);
      return;
    }
    await _store.writeDraft(
      PersistedDraft(
        deviceId: id,
        base: base,
        draft: draft,
        savedAt: DateTime.now(),
      ),
    );
  }
}

@Riverpod(keepAlive: true)
class ConnectionController extends _$ConnectionController {
  StreamSubscription<BleAdapterState>? _adapterSubscription;
  StreamSubscription<BleScanResult>? _scanSubscription;
  StreamSubscription<BleLinkState>? _linkSubscription;
  StreamSubscription<Telemetry>? _telemetrySubscription;
  StreamSubscription<DeviceConfig>? _configSubscription;
  StreamSubscription<int>? _rssiSubscription;
  BikeComputerRepositoryImpl? _repository;
  RememberedDevice? _remembered;
  Timer? _reconnectTimer;
  bool _explicitDisconnect = false;
  bool _foreground = true;
  int _reconnectAttempt = 0;

  BleTransport get _transport => ref.read(bleTransportProvider);
  PreferencesStore get _store => ref.read(preferencesStoreProvider);
  AndroidBlePlatform get _platform => ref.read(androidBlePlatformProvider);

  @override
  SessionState build() {
    ref.onDispose(_dispose);
    _adapterSubscription = _transport.adapterState.listen(_onAdapterState);
    unawaited(_loadRememberedDevice());
    return const SessionState();
  }

  Future<void> _loadRememberedDevice() async {
    _remembered = await _store.readRememberedDevice();
  }

  void _onAdapterState(BleAdapterState adapter) {
    switch (adapter) {
      case BleAdapterState.poweredOff:
        state = state.copyWith(
          connection: const ConnectionState.bluetoothOff(),
        );
      case BleAdapterState.unauthorized:
        state = state.copyWith(
          connection: const ConnectionState.permissionRequired(
            AppErrors.permissionDenied,
          ),
        );
      case BleAdapterState.locationServicesOff:
        state = state.copyWith(
          connection: const ConnectionState.permissionRequired(
            AppErrors.locationServicesOff,
          ),
        );
      case BleAdapterState.unsupported:
        state = state.copyWith(
          connection: const ConnectionState.failed(
            AppFailure(
              kind: AppErrorKind.unsupported,
              message: 'Bluetooth LE не поддерживается на этом телефоне',
            ),
          ),
        );
      case BleAdapterState.ready:
        if (state.connection is ConnectionBluetoothOff) {
          state = state.copyWith(connection: const ConnectionState.idle());
        }
      case BleAdapterState.unknown:
        break;
    }
  }

  Future<void> scan() async {
    final permissionError = await _platform.ensureScanPermissions();
    if (permissionError != null) {
      state = state.copyWith(
        connection: ConnectionState.permissionRequired(permissionError),
        lastError: permissionError,
      );
      return;
    }

    await _scanSubscription?.cancel();
    state = state.copyWith(
      connection: const ConnectionState.scanning(),
      devices: const <BleScanResult>[],
      lastError: null,
    );
    final devices = <String, BleScanResult>{};
    _scanSubscription = _transport.scan().listen(
      (found) async {
        final enriched = found.copyWith(
          bondState: await _platform.bondState(found.deviceId),
        );
        devices[found.deviceId] = enriched;
        state = state.copyWith(devices: devices.values.toList(growable: false));
        if (_remembered?.id == found.deviceId &&
            state.connection is ConnectionScanning) {
          await connectDevice(enriched);
        }
      },
      onError: (Object error) {
        final appError = AppErrors.unknown(error);
        state = state.copyWith(
          connection: ConnectionState.failed(appError),
          lastError: appError,
        );
      },
    );
  }

  Future<void> stopScan() async {
    final subscription = _scanSubscription;
    _scanSubscription = null;
    subscription?.cancel().ignore();
    await _transport.stopScan();
    if (state.connection is ConnectionScanning) {
      state = state.copyWith(connection: const ConnectionState.idle());
    }
  }

  Future<void> connectDevice(BleScanResult device) async {
    _explicitDisconnect = false;
    _reconnectTimer?.cancel();
    await stopScan();
    state = state.copyWith(
      connection: const ConnectionState.connecting(),
      selectedDevice: device,
      lastError: null,
      lastMessage: null,
    );

    final connected = Completer<void>();
    await _linkSubscription?.cancel();
    _linkSubscription = _transport
        .connect(device.deviceId)
        .listen(
          (link) {
            _repository?.noteLinkState(link);
            if (link == BleLinkState.connected && !connected.isCompleted) {
              connected.complete();
            } else if (link == BleLinkState.disconnected &&
                connected.isCompleted) {
              unawaited(_handleUnexpectedDisconnect());
            }
          },
          onError: (Object error) {
            if (!connected.isCompleted) connected.completeError(error);
          },
        );

    try {
      await connected.future.timeout(const Duration(seconds: 12));
      await _synchronize(device);
    } on Object catch (error) {
      final appError = error is AppError ? error : AppErrors.unknown(error);
      state = state.copyWith(
        connection: ConnectionState.failed(appError),
        lastError: appError,
      );
      if (appError.kind != AppErrorKind.serviceMissing &&
          appError.kind != AppErrorKind.unsupported &&
          appError.kind != AppErrorKind.notPaired) {
        _scheduleReconnect();
      }
    }
  }

  Future<void> _synchronize(BleScanResult device) async {
    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.mtu),
    );
    final mtu = await _transport.requestMtu(247);
    final readOnly = mtu < 51;

    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.discovery),
    );
    final discovery = await _transport.discoverServices();
    if (!discovery.characteristicUuids.containsAll(
      BleUuids.requiredCharacteristics,
    )) {
      throw AppErrors.serviceMissing;
    }

    final repository = BikeComputerRepositoryImpl(_transport);
    _repository = repository;
    await repository.start();
    _bindRepository(repository);

    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.deviceInfo),
    );
    final infoResult = await repository.readDeviceInfo();
    if (infoResult case Failure<DeviceInfo>(:final error)) throw error;
    final info = (infoResult as Success<DeviceInfo>).value;
    state = state.copyWith(deviceInfo: info);
    if (info.protoMajor != 1) {
      final error = AppErrors.incompatible(info.protoMajor, 1);
      await _store.rememberDevice(device.deviceId, device.name);
      _remembered = RememberedDevice(id: device.deviceId, name: device.name);
      state = state.copyWith(
        connection: ConnectionState.incompatibleProtocol(error),
        lastError: error,
      );
      return;
    }

    if (device.bondState == BondState.none && !info.pairingWindowOpen) {
      throw AppErrors.notPaired;
    }

    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.pairing),
    );
    final configResult = await repository.readConfig();
    if (configResult case Failure<DeviceConfig>(:final error)) throw error;
    final config = (configResult as Success<DeviceConfig>).value;
    state = state.copyWith(deviceConfig: config);
    await ref
        .read(configDraftControllerProvider.notifier)
        .loadDeviceConfig(device.deviceId, config);

    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.subscriptions),
    );
    await repository.readTelemetry();
    await _store.rememberDevice(device.deviceId, device.name);
    _remembered = RememberedDevice(id: device.deviceId, name: device.name);
    _reconnectAttempt = 0;
    state = state.copyWith(
      connection: readOnly
          ? const ConnectionState.readOnly(AppErrors.mtuTooSmall)
          : const ConnectionState.ready(),
      lastError: readOnly ? AppErrors.mtuTooSmall : null,
    );
  }

  void _bindRepository(BikeComputerRepositoryImpl repository) {
    unawaited(_telemetrySubscription?.cancel());
    unawaited(_configSubscription?.cancel());
    unawaited(_rssiSubscription?.cancel());
    _telemetrySubscription = repository.telemetry.listen((telemetry) {
      state = state.copyWith(
        telemetry: telemetry,
        lastTelemetryAt: DateTime.now(),
      );
    });
    _configSubscription = repository.configUpdates.listen((config) {
      state = state.copyWith(deviceConfig: config);
    });
    _rssiSubscription = repository.rssi.listen((rssi) {
      state = state.copyWith(rssi: rssi);
    });
  }

  Future<void> _handleUnexpectedDisconnect() async {
    if (_explicitDisconnect) return;
    state = state.copyWith(lastError: AppErrors.connectionLost);
    _scheduleReconnect();
  }

  void _scheduleReconnect() {
    if (_explicitDisconnect || !_foreground || state.selectedDevice == null) {
      return;
    }
    const delays = <int>[1, 2, 4, 8, 15];
    final delay = delays[_reconnectAttempt.clamp(0, delays.length - 1)];
    _reconnectAttempt++;
    state = state.copyWith(
      connection: ConnectionState.reconnecting(_reconnectAttempt, delay),
    );
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(Duration(seconds: delay), () {
      final device = state.selectedDevice;
      if (device != null && _foreground && !_explicitDisconnect) {
        unawaited(connectDevice(device));
      }
    });
  }

  Future<void> setForeground(bool value) async {
    _foreground = value;
    await ref.read(configDraftControllerProvider.notifier).persistNow();
    await _repository?.setTelemetrySubscribed(value);
    if (!value) {
      _reconnectTimer?.cancel();
    } else if (state.connection is ConnectionReconnecting) {
      _scheduleReconnect();
    }
  }

  Future<Result<void>> writeConfig(DeviceConfig config) async {
    if (state.connection is! ConnectionReady || _repository == null) {
      return const Failure<void>(
        AppFailure(
          kind: AppErrorKind.connectionLost,
          message: 'Запись доступна только при готовом подключении',
        ),
      );
    }
    state = state.copyWith(commandInFlight: true, lastError: null);
    final result = await _repository!.writeConfig(config);
    switch (result) {
      case Success<void>():
        state = state.copyWith(
          deviceConfig: config,
          commandInFlight: false,
          lastMessage: 'Настройки сохранены и проверены',
        );
      case Failure<void>(:final error):
        state = state.copyWith(
          commandInFlight: false,
          lastError: error is AppError ? error : AppErrors.unknown(error),
        );
    }
    return result;
  }

  Future<Result<CommandResult>> sendCommand(DeviceCommand command) async {
    if (state.connection is! ConnectionReady || _repository == null) {
      return const Failure<CommandResult>(
        AppFailure(
          kind: AppErrorKind.connectionLost,
          message: 'Команда доступна только при готовом подключении',
        ),
      );
    }
    state = state.copyWith(commandInFlight: true, lastError: null);
    final result = await _repository!.sendCommand(command);
    switch (result) {
      case Success<CommandResult>():
        state = state.copyWith(
          commandInFlight: false,
          lastMessage: 'Команда «${command.id.name}» выполнена',
          sensorTestActive: switch (command.id) {
            DeviceCommandId.sensorTestStart => true,
            DeviceCommandId.sensorTestStop => false,
            _ => state.sensorTestActive,
          },
        );
      case Failure<CommandResult>(:final error):
        state = state.copyWith(
          commandInFlight: false,
          lastError: error is AppError ? error : AppErrors.unknown(error),
        );
    }
    return result;
  }

  Future<void> refresh() async {
    await _repository?.readTelemetry();
    final result = await _repository?.readConfig();
    if (result case Success<DeviceConfig>(:final value)) {
      state = state.copyWith(
        deviceConfig: value,
        lastMessage: 'Данные обновлены',
      );
    }
  }

  Future<void> disconnect({bool explicit = true}) async {
    _explicitDisconnect = explicit;
    _reconnectTimer?.cancel();
    await _repository?.dispose();
    _repository = null;
    await _transport.disconnect();
    state = state.copyWith(
      connection: const ConnectionState.idle(),
      commandInFlight: false,
      sensorTestActive: false,
    );
  }

  Future<void> forgetDevice() async {
    final id = state.selectedDevice?.deviceId;
    await disconnect();
    await _store.forgetDevice();
    if (id != null) await _store.clearDraft(id);
    _remembered = null;
    state = const SessionState();
  }

  Future<void> requestEnableBluetooth() => _platform.requestEnableBluetooth();
  Future<bool> openSettings() => _platform.openSettings();

  void clearMessage() =>
      state = state.copyWith(lastMessage: null, lastError: null);

  void _dispose() {
    _reconnectTimer?.cancel();
    unawaited(_adapterSubscription?.cancel());
    unawaited(_scanSubscription?.cancel());
    unawaited(_linkSubscription?.cancel());
    unawaited(_telemetrySubscription?.cancel());
    unawaited(_configSubscription?.cancel());
    unawaited(_rssiSubscription?.cancel());
    unawaited(_repository?.dispose());
  }
}
