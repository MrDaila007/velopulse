import 'dart:async';
import 'dart:typed_data';

import '../core/app_error.dart';
import '../core/result.dart';
import '../domain/entities/companion_models.dart';
import '../domain/entities/models.dart';
import '../domain/validators/config_validator.dart';
import 'ble/ble_transport.dart';
import 'protocol/ble_uuids.dart';
import 'protocol/protocol_codecs.dart';

abstract interface class BikeComputerRepository {
  Stream<BleLinkState> get connectionState;
  Stream<Telemetry> get telemetry;
  Stream<DeviceConfig> get configUpdates;
  Stream<int> get rssi;

  Future<Result<DeviceInfo>> readDeviceInfo();
  Future<Result<DeviceConfig>> readConfig();
  Future<Result<Telemetry>> readTelemetry();
  Future<Result<DiagnosticSnapshot>> getDiagnostic();
  Future<Result<ErrorLogBatch>> readErrorLog();
  Future<Result<void>> writeConfig(DeviceConfig config);
  Future<Result<void>> writeCompanionSnapshot(CompanionSnapshot snapshot);
  Future<Result<CommandResult>> sendCommand(DeviceCommand command);
  Future<Result<void>> setOdometerMeters(int odometerM);
  Future<void> setTelemetrySubscribed(bool value);
  Future<void> disconnect();
}

class BikeComputerRepositoryImpl implements BikeComputerRepository {
  BikeComputerRepositoryImpl(this._transport);

  final BleTransport _transport;
  final _connectionState = StreamController<BleLinkState>.broadcast();
  final _telemetry = StreamController<Telemetry>.broadcast();
  final _configUpdates = StreamController<DeviceConfig>.broadcast();
  final _rssi = StreamController<int>.broadcast();
  final _commandResults = StreamController<CommandResult>.broadcast();
  final List<StreamSubscription<List<int>>> _subscriptions = [];
  StreamSubscription<List<int>>? _telemetrySubscription;
  Timer? _rssiTimer;
  DeviceConfig? _latestConfig;
  Future<void> _operationTail = Future<void>.value();

  @override
  Stream<BleLinkState> get connectionState => _connectionState.stream;
  @override
  Stream<Telemetry> get telemetry => _telemetry.stream;
  @override
  Stream<DeviceConfig> get configUpdates => _configUpdates.stream;
  @override
  Stream<int> get rssi => _rssi.stream;

  void noteLinkState(BleLinkState value) => _connectionState.add(value);

  Future<void> start() async {
    final configSubscription = _transport
        .subscribe(BleUuids.configRead)
        .listen(_handleConfigBytes, onError: _configUpdates.addError);
    final resultSubscription = _transport
        .subscribe(BleUuids.commandResult)
        .listen(_handleResultBytes, onError: _commandResults.addError);
    _subscriptions.addAll([configSubscription, resultSubscription]);
    await setTelemetrySubscribed(true);
    _rssiTimer = Timer.periodic(const Duration(seconds: 1), (_) async {
      try {
        _rssi.add(await _transport.readRssi());
      } on Object {
        // RSSI is auxiliary; connection events remain the source of truth.
      }
    });
  }

  void _handleConfigBytes(List<int> bytes) {
    final config = ProtocolCodecs.decodeConfig(bytes);
    _latestConfig = config;
    _configUpdates.add(config);
  }

  void _handleResultBytes(List<int> bytes) {
    _commandResults.add(ProtocolCodecs.decodeCommandResult(bytes));
  }

  void _handleTelemetryBytes(List<int> bytes) {
    _telemetry.add(ProtocolCodecs.decodeTelemetry(bytes));
  }

  Future<Result<T>> _guard<T>(Future<T> Function() body) async {
    try {
      return Success(await body());
    } on AppError catch (error) {
      return Failure(error);
    } on ProtocolCodecException catch (error) {
      return Failure(AppErrors.malformed(error.message, error));
    } on Object catch (error) {
      return Failure(AppErrors.unknown(error));
    }
  }

  Future<Result<T>> _enqueue<T>(Future<Result<T>> Function() body) {
    final completer = Completer<Result<T>>();
    _operationTail = _operationTail.then((_) async {
      try {
        completer.complete(await body());
      } on Object catch (error) {
        completer.complete(Failure<T>(AppErrors.unknown(error)));
      }
    });
    return completer.future;
  }

  @override
  Future<Result<DeviceInfo>> readDeviceInfo() => _guard(
    () async => ProtocolCodecs.decodeDeviceInfo(
      await _transport.read(BleUuids.deviceInfo),
    ),
  );

  @override
  Future<Result<DeviceConfig>> readConfig() => _guard(() async {
    final value = ProtocolCodecs.decodeConfig(
      await _transport.read(BleUuids.configRead),
    );
    _latestConfig = value;
    _configUpdates.add(value);
    return value;
  });

  @override
  Future<Result<Telemetry>> readTelemetry() => _guard(() async {
    final value = ProtocolCodecs.decodeTelemetry(
      await _transport.read(BleUuids.telemetry),
    );
    _telemetry.add(value);
    return value;
  });

  @override
  Future<Result<DiagnosticSnapshot>> getDiagnostic() => _enqueue(() async {
    final response = _nextResult(DeviceCommandId.getDiagnostic);
    try {
      await _transport.writeWithResponse(
        BleUuids.command,
        ProtocolCodecs.encodeCommand(
          const DeviceCommand(id: DeviceCommandId.getDiagnostic),
        ),
      );
      final result = await response;
      if (result.status != CommandStatus.ok) {
        return Failure<DiagnosticSnapshot>(_resultError(result));
      }
      if (result.payload.length != ProtocolCodecs.diagnosticPayloadSize) {
        return Failure<DiagnosticSnapshot>(
          AppErrors.malformed(
            'Diagnostic payload: ожидалось '
            '${ProtocolCodecs.diagnosticPayloadSize} байт, '
            'получено ${result.payload.length}',
          ),
        );
      }
      return Success(ProtocolCodecs.decodeDiagnostic(result.payload));
    } on TimeoutException {
      return const Failure<DiagnosticSnapshot>(
        AppFailure(
          kind: AppErrorKind.writeTimeout,
          message: 'Устройство не ответило на GET_DIAGNOSTIC',
          action: 'Повторить',
        ),
      );
    } on Object catch (error) {
      response.ignore();
      return Failure<DiagnosticSnapshot>(AppErrors.unknown(error));
    }
  });

  @override
  Future<Result<ErrorLogBatch>> readErrorLog() => _guard(() async {
    try {
      return ProtocolCodecs.decodeErrorLog(
        await _transport.read(BleUuids.errorLog),
      );
    } on StateError {
      return const ErrorLogBatch(entries: <ErrorLogEntry>[]);
    }
  });

  Future<CommandResult> _nextResult(DeviceCommandId command) => _commandResults
      .stream
      .firstWhere((result) => result.commandId == command)
      .timeout(const Duration(seconds: 3));

  AppError _resultError(CommandResult result) => switch (result.status) {
    CommandStatus.range => AppFailure(
      kind: AppErrorKind.deviceRejected,
      message:
          'Устройство отклонило поле конфигурации (offset ${result.detail})',
      fieldId: result.detail,
    ),
    CommandStatus.notPaired => AppErrors.notPaired,
    CommandStatus.storage => const AppFailure(
      kind: AppErrorKind.storageError,
      message: 'Устройство не смогло сохранить настройки',
      action: 'Повторить',
    ),
    CommandStatus.tokenExpired => const AppFailure(
      kind: AppErrorKind.tokenExpired,
      message: 'Время подтверждения истекло',
      action: 'Начать заново',
    ),
    CommandStatus.hardware => const AppFailure(
      kind: AppErrorKind.hardware,
      message: 'Устройство сообщило об аппаратной ошибке',
    ),
    CommandStatus.notSupported => const AppFailure(
      kind: AppErrorKind.unsupported,
      message: 'Функция отсутствует в этой версии прошивки',
    ),
    _ => AppFailure(
      kind: AppErrorKind.unknown,
      message: 'Устройство отклонило операцию: ${result.status.name}',
    ),
  };

  @override
  Future<Result<void>> writeConfig(DeviceConfig config) => _enqueue(() async {
    final validation = ConfigValidator.validate(config);
    if (!validation.isValid) {
      final first = validation.issues.first;
      return Failure<void>(
        AppErrors.validation(first.message, fieldId: first.fieldId),
      );
    }

    for (var attempt = 0; attempt <= 3; attempt++) {
      final response = _nextResult(DeviceCommandId.configWrite);
      try {
        await _transport.writeWithResponse(
          BleUuids.configWrite,
          ProtocolCodecs.encodeConfig(config),
        );
        final result = await response;
        if (result.status == CommandStatus.busy && attempt < 3) {
          await Future<void>.delayed(const Duration(seconds: 1));
          continue;
        }
        if (result.status != CommandStatus.ok) {
          return Failure<void>(_resultError(result));
        }
        if (_latestConfig != config) {
          try {
            await _configUpdates.stream
                .firstWhere((value) => value == config)
                .timeout(const Duration(seconds: 3));
          } on TimeoutException {
            final reread = await readConfig();
            if (reread case Success<DeviceConfig>(
              value: final value,
            ) when value == config) {
              return const Success<void>(null);
            }
            return const Failure<void>(AppErrors.writeTimeout);
          }
        }
        return const Success<void>(null);
      } on TimeoutException {
        final reread = await readConfig();
        if (reread case Success<DeviceConfig>(
          value: final value,
        ) when value == config) {
          return const Success<void>(null);
        }
        return const Failure<void>(AppErrors.writeTimeout);
      } on Object catch (error) {
        response.ignore();
        return Failure<void>(AppErrors.unknown(error));
      }
    }
    return const Failure<void>(
      AppFailure(
        kind: AppErrorKind.busy,
        message: 'Устройство занято. Повторите операцию',
        action: 'Повторить',
      ),
    );
  });

  Future<Result<CommandResult>> _sendCommandDirect(
    DeviceCommand command,
  ) async {
    for (var attempt = 0; attempt <= 3; attempt++) {
      final response = _nextResult(command.id);
      try {
        await _transport.writeWithResponse(
          BleUuids.command,
          ProtocolCodecs.encodeCommand(command),
        );
        final result = await response;
        if (result.status == CommandStatus.busy && attempt < 3) {
          await Future<void>.delayed(const Duration(seconds: 1));
          continue;
        }
        if (result.status == CommandStatus.ok ||
            result.status == CommandStatus.needsConfirm) {
          return Success(result);
        }
        return Failure<CommandResult>(_resultError(result));
      } on TimeoutException {
        return const Failure<CommandResult>(
          AppFailure(
            kind: AppErrorKind.writeTimeout,
            message: 'Устройство не ответило на команду',
            action: 'Повторить',
          ),
        );
      } on Object catch (error) {
        response.ignore();
        return Failure<CommandResult>(AppErrors.unknown(error));
      }
    }
    return const Failure<CommandResult>(
      AppFailure(
        kind: AppErrorKind.busy,
        message: 'Устройство занято. Повторите команду',
        action: 'Повторить',
      ),
    );
  }

  @override
  Future<Result<void>> writeCompanionSnapshot(CompanionSnapshot snapshot) =>
      _enqueue(() async {
        await _transport.writeWithResponse(
          BleUuids.companionWrite,
          ProtocolCodecs.encodeCompanion(snapshot),
        );
        return const Success<void>(null);
      });

  @override
  Future<Result<CommandResult>> sendCommand(DeviceCommand command) =>
      _enqueue(() => _sendCommandDirect(command));

  @override
  Future<Result<void>> setOdometerMeters(int odometerM) => _enqueue(() async {
    final requestPayload = ByteData(4)..setUint32(0, odometerM, Endian.little);
    final request = DeviceCommand(
      id: DeviceCommandId.setOdometer,
      payload: requestPayload.buffer.asUint8List().toList(),
    );
    final stepOne = await _sendCommandDirect(request);
    if (stepOne case Failure<CommandResult>(:final error)) {
      return Failure<void>(error);
    }
    final needs = (stepOne as Success<CommandResult>).value;
    if (needs.status != CommandStatus.needsConfirm) {
      return needs.status == CommandStatus.ok
          ? const Success<void>(null)
          : Failure<void>(_resultError(needs));
    }
    final tokenBytes = ByteData(4)..setUint32(0, needs.token, Endian.little);
    final confirmPayload = <int>[
      ...requestPayload.buffer.asUint8List(),
      ...tokenBytes.buffer.asUint8List(),
    ];
    final confirm = DeviceCommand(
      id: DeviceCommandId.setOdometer,
      hasToken: true,
      payload: confirmPayload,
    );
    final stepTwo = await _sendCommandDirect(confirm);
    if (stepTwo case Failure<CommandResult>(:final error)) {
      return Failure<void>(error);
    }
    final result = (stepTwo as Success<CommandResult>).value;
    if (result.status == CommandStatus.ok) {
      return const Success<void>(null);
    }
    return Failure<void>(_resultError(result));
  });

  @override
  Future<void> setTelemetrySubscribed(bool value) async {
    if (!value) {
      await _telemetrySubscription?.cancel();
      _telemetrySubscription = null;
      return;
    }
    if (_telemetrySubscription != null) return;
    _telemetrySubscription = _transport
        .subscribe(BleUuids.telemetry)
        .listen(_handleTelemetryBytes, onError: _telemetry.addError);
  }

  @override
  Future<void> disconnect() => _transport.disconnect();

  Future<void> dispose() async {
    _rssiTimer?.cancel();
    await _telemetrySubscription?.cancel();
    for (final subscription in _subscriptions) {
      await subscription.cancel();
    }
    await _connectionState.close();
    await _telemetry.close();
    await _configUpdates.close();
    await _rssi.close();
    await _commandResults.close();
  }
}
