import 'dart:async';

import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../core/app_error.dart';
import '../core/result.dart';
import '../data/companion_sync_service.dart';
import '../data/bike_computer_repository.dart';
import '../data/ble/ble_transport.dart';
import '../data/ble/fake_ble_transport.dart';
import '../data/ble/reactive_ble_transport.dart';
import '../data/local/firmware_migration_store.dart';
import '../data/local/preferences_store.dart';
import '../data/log_exporter.dart';
import '../data/protocol/ble_uuids.dart';
import '../data/ride_log_recorder.dart';
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
FirmwareMigrationStore firmwareMigrationStore(Ref ref) =>
    FirmwareMigrationStore();

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
  bool _handlingUnexpectedDisconnect = false;
  int _reconnectAttempt = 0;
  late BleTransport _transport;
  late PreferencesStore _store;
  late FirmwareMigrationStore _migrationStore;
  late AndroidBlePlatform _platform;
  final RideLogRecorder _rideLog = RideLogRecorder();
  late final CompanionSyncService _companionSync;
  bool _companionSupported = false;

  @override
  SessionState build() {
    _transport = ref.read(bleTransportProvider);
    _store = ref.read(preferencesStoreProvider);
    _companionSync = CompanionSyncService(preferences: _store);
    _migrationStore = ref.read(firmwareMigrationStoreProvider);
    _platform = ref.read(androidBlePlatformProvider);
    ref.onDispose(_dispose);
    _adapterSubscription = _transport.adapterState.listen(_onAdapterState);
    unawaited(_loadRememberedDevice());
    return const SessionState();
  }

  Future<void> _loadRememberedDevice() async {
    _remembered = await _store.readRememberedDevice();
    final remembered = _remembered;
    if (remembered == null || state.selectedDevice != null) return;
    state = state.copyWith(
      selectedDevice: BleScanResult(
        deviceId: remembered.id,
        name: remembered.name,
        rssi: 0,
      ),
    );
  }

  Future<void> connectLastDevice() async {
    final device = state.selectedDevice;
    if (device == null) {
      final remembered = _remembered ?? await _store.readRememberedDevice();
      if (remembered == null) return;
      await connectDevice(
        BleScanResult(
          deviceId: remembered.id,
          name: remembered.name,
          rssi: 0,
        ),
      );
      return;
    }
    await connectDevice(device);
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
        if (state.connection is! ConnectionScanning) return;
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
    _handlingUnexpectedDisconnect = false;
    _reconnectTimer?.cancel();
    _rideLog.clear();
    state = state.copyWith(
      connection: const ConnectionState.connecting(),
      selectedDevice: device,
      lastError: null,
      lastMessage: null,
    );
    await stopScan();
    await _disposeRepository();
    await _cancelLink();

    final connected = Completer<void>();
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
      await _disposeRepository();
      await _cancelLink();
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
    _companionSupported = discovery.characteristicUuids.contains(
      BleUuids.companionWrite,
    );
    _companionSync.configure(companionSupported: _companionSupported);

    state = state.copyWith(
      connection: const ConnectionState.synchronizing(SyncStage.deviceInfo),
    );
    final repository = BikeComputerRepositoryImpl(_transport);
    _repository = repository;
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

    final hasKnownBond =
        device.bondState == BondState.bonded ||
        (device.bondState == BondState.unknown && info.bonded);
    if (!hasKnownBond && !info.pairingWindowOpen) {
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
    await repository.start();
    _bindRepository(repository);
    if (_companionSupported) {
      _companionSync.start(repository);
    }
    final telemetryResult = await repository.readTelemetry();
    if (telemetryResult case Failure<Telemetry>(:final error)) throw error;
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
      _rideLog.record(telemetry, rssi: state.rssi);
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

  Future<void> _disposeRepository() async {
    _companionSync.stop();
    final repository = _repository;
    _repository = null;
    final telemetry = _telemetrySubscription;
    final config = _configSubscription;
    final rssi = _rssiSubscription;
    _telemetrySubscription = null;
    _configSubscription = null;
    _rssiSubscription = null;
    final repositoryDisposal = repository?.dispose();
    await telemetry?.cancel();
    await config?.cancel();
    await rssi?.cancel();
    await repositoryDisposal;
  }

  Future<void> _cancelLink() async {
    final link = _linkSubscription;
    _linkSubscription = null;
    await link?.cancel();
    await _transport.disconnect();
  }

  Future<void> _handleUnexpectedDisconnect() async {
    if (_explicitDisconnect || _handlingUnexpectedDisconnect) return;
    _handlingUnexpectedDisconnect = true;
    try {
      await _disposeRepository();
      await _cancelLink();
      state = state.copyWith(lastError: AppErrors.connectionLost);
      _scheduleReconnect();
    } finally {
      _handlingUnexpectedDisconnect = false;
    }
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

  Future<Result<void>> restoreFirmwareBackup() async {
    final backup = await _migrationStore.readBackup();
    if (backup == null) {
      return const Failure<void>(
        AppFailure(
          kind: AppErrorKind.validationFailed,
          message: 'Резервная копия не найдена',
        ),
      );
    }
    if (state.connection is! ConnectionReady || _repository == null) {
      return const Failure<void>(
        AppFailure(
          kind: AppErrorKind.connectionLost,
          message: 'Восстановление доступно только при готовом подключении',
        ),
      );
    }
    state = state.copyWith(commandInFlight: true, lastError: null);
    final configResult = await _repository!.writeConfig(backup.config);
    if (configResult case Failure<void>(:final error)) {
      state = state.copyWith(
        commandInFlight: false,
        lastError: error is AppError ? error : AppErrors.unknown(error),
      );
      return Failure<void>(error);
    }
    final odometerResult = await _repository!.setOdometerMeters(
      backup.odometerM,
    );
    switch (odometerResult) {
      case Success<void>():
        state = state.copyWith(
          commandInFlight: false,
          lastMessage: 'Конфиг и одометр восстановлены из резервной копии',
        );
        await refresh();
        return const Success<void>(null);
      case Failure<void>(:final error):
        state = state.copyWith(
          commandInFlight: false,
          lastError: error is AppError ? error : AppErrors.unknown(error),
        );
        return Failure<void>(error);
    }
  }

  Future<Result<void>> backupFirmwareData() async {
    final device = state.selectedDevice;
    final config = state.deviceConfig;
    final telemetry = state.telemetry;
    if (device == null || config == null || telemetry == null) {
      return const Failure<void>(
        AppFailure(
          kind: AppErrorKind.connectionLost,
          message:
              'Подключитесь и дождитесь телеметрии перед резервным копированием',
        ),
      );
    }
    await _migrationStore.writeBackup(
      FirmwareMigrationBackup(
        deviceId: device.deviceId,
        deviceName: device.name,
        config: config,
        odometerM: telemetry.odometerM,
        savedAt: DateTime.now(),
        fwVersion: state.deviceInfo?.fwVersion,
      ),
    );
    state = state.copyWith(
      lastMessage: 'Резервная копия сохранена (${telemetry.odometerM} м)',
    );
    return const Success<void>(null);
  }

  Future<FirmwareMigrationBackup?> readFirmwareBackup() =>
      _migrationStore.readBackup();

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

  Future<bool> syncCompanion() async {
    if (!_companionSupported) return false;
    final ok = await _companionSync.sync();
    if (ok) {
      state = state.copyWith(lastMessage: 'Время и погода отправлены на устройство');
    }
    return ok;
  }

  Future<void> disconnect({bool explicit = true}) async {
    _explicitDisconnect = explicit;
    _reconnectTimer?.cancel();
    await _disposeRepository();
    await _cancelLink();
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

  int get rideLogSampleCount => _rideLog.samples.length;

  Future<Result<String>> exportSessionLog() async {
    if (_repository == null) {
      return const Failure<String>(
        AppFailure(
          kind: AppErrorKind.connectionLost,
          message: 'Экспорт доступен только при подключении',
        ),
      );
    }
    final diagnosticResult = await _repository!.getDiagnostic();
    final errorLogResult = await _repository!.readErrorLog();
    if (diagnosticResult case Failure<DiagnosticSnapshot>(:final error)) {
      return Failure<String>(error);
    }
    if (errorLogResult case Failure<ErrorLogBatch>(:final error)) {
      return Failure<String>(error);
    }
    try {
      final file = await LogExporter.write(
        exportedAt: DateTime.now(),
        deviceId: state.selectedDevice?.deviceId,
        deviceInfo: state.deviceInfo,
        config: state.deviceConfig,
        diagnostic: (diagnosticResult as Success<DiagnosticSnapshot>).value,
        errorLog: (errorLogResult as Success<ErrorLogBatch>).value,
        samples: _rideLog.samples,
        rssi: state.rssi,
        connectionState: state.connection,
        sensorTestActive: state.sensorTestActive,
      );
      return Success(file.path);
    } on Object catch (error) {
      return Failure<String>(AppErrors.unknown(error));
    }
  }

  void _dispose() {
    _reconnectTimer?.cancel();
    unawaited(_adapterSubscription?.cancel());
    unawaited(_scanSubscription?.cancel());
    unawaited(_cancelLink());
    unawaited(_disposeRepository());
  }
}
