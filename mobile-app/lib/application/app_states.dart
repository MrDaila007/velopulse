import 'package:freezed_annotation/freezed_annotation.dart';

import '../core/app_error.dart';
import '../data/ble/ble_transport.dart';
import '../domain/entities/models.dart';

part 'app_states.freezed.dart';

enum SyncStage {
  mtu('Согласование MTU'),
  discovery('Поиск BLE-сервиса'),
  deviceInfo('Чтение информации'),
  pairing('Сопряжение и конфигурация'),
  subscriptions('Подписка на данные');

  const SyncStage(this.label);
  final String label;
}

@freezed
sealed class ConnectionState with _$ConnectionState {
  const factory ConnectionState.idle() = ConnectionIdle;
  const factory ConnectionState.scanning() = ConnectionScanning;
  const factory ConnectionState.connecting() = ConnectionConnecting;
  const factory ConnectionState.synchronizing(SyncStage stage) =
      ConnectionSynchronizing;
  const factory ConnectionState.ready() = ConnectionReady;
  const factory ConnectionState.readOnly(AppError reason) = ConnectionReadOnly;
  const factory ConnectionState.reconnecting(int attempt, int delaySeconds) =
      ConnectionReconnecting;
  const factory ConnectionState.failed(AppError error) = ConnectionFailed;
  const factory ConnectionState.bluetoothOff() = ConnectionBluetoothOff;
  const factory ConnectionState.permissionRequired(AppError error) =
      ConnectionPermissionRequired;
  const factory ConnectionState.incompatibleProtocol(AppError error) =
      ConnectionIncompatibleProtocol;
}

@freezed
abstract class SessionState with _$SessionState {
  const factory SessionState({
    @Default(ConnectionState.idle()) ConnectionState connection,
    @Default(<BleScanResult>[]) List<BleScanResult> devices,
    BleScanResult? selectedDevice,
    DeviceInfo? deviceInfo,
    DeviceConfig? deviceConfig,
    Telemetry? telemetry,
    int? rssi,
    DateTime? lastTelemetryAt,
    String? lastMessage,
    AppError? lastError,
    @Default(false) bool commandInFlight,
    @Default(false) bool sensorTestActive,
  }) = _SessionState;
}

@freezed
abstract class ConfigDraftState with _$ConfigDraftState {
  const ConfigDraftState._();

  const factory ConfigDraftState({
    String? deviceId,
    DeviceConfig? deviceConfig,
    DeviceConfig? draft,
    @Default(<String, String>{}) Map<String, String> fieldErrors,
    @Default(false) bool isWriting,
    DateTime? lastSyncedAt,
    @Default(false) bool hasConflict,
  }) = _ConfigDraftState;

  bool get isDirty => draft != null && draft != deviceConfig;
  bool get isValid => fieldErrors.isEmpty;
}
