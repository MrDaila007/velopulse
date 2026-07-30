enum AppErrorKind {
  bluetoothOff,
  permissionDenied,
  locationServicesOff,
  deviceNotFound,
  connectionLost,
  serviceMissing,
  incompatibleProtocol,
  mtuTooSmall,
  validationFailed,
  deviceRejected,
  writeTimeout,
  storageError,
  notPaired,
  tokenExpired,
  busy,
  hardware,
  unsupported,
  malformedPacket,
  unknown,
}

sealed class AppError implements Exception {
  const AppError({
    required this.kind,
    required this.message,
    this.action,
    this.fieldId,
    this.cause,
  });

  final AppErrorKind kind;
  final String message;
  final String? action;
  final int? fieldId;
  final Object? cause;

  @override
  String toString() => message;
}

final class AppFailure extends AppError {
  const AppFailure({
    required super.kind,
    required super.message,
    super.action,
    super.fieldId,
    super.cause,
  });
}

abstract final class AppErrors {
  static const bluetoothOff = AppFailure(
    kind: AppErrorKind.bluetoothOff,
    message: 'Bluetooth выключен',
    action: 'Включить',
  );
  static const permissionDenied = AppFailure(
    kind: AppErrorKind.permissionDenied,
    message: 'Нужно разрешение на поиск устройств рядом',
    action: 'Разрешить',
  );
  static const locationServicesOff = AppFailure(
    kind: AppErrorKind.locationServicesOff,
    message: 'На Android 11 и ниже для поиска нужно включить геолокацию',
    action: 'Открыть настройки',
  );
  static const connectionLost = AppFailure(
    kind: AppErrorKind.connectionLost,
    message: 'Соединение потеряно, переподключение…',
  );
  static const serviceMissing = AppFailure(
    kind: AppErrorKind.serviceMissing,
    message: 'Устройство не поддерживает протокол велокомпьютера',
    action: 'Отключиться',
  );
  static const mtuTooSmall = AppFailure(
    kind: AppErrorKind.mtuTooSmall,
    message:
        'Телефон не поддерживает нужный размер пакета. Доступен только просмотр',
  );
  static const notPaired = AppFailure(
    kind: AppErrorKind.notPaired,
    message:
        'Требуется сопряжение. Перезапустите устройство и подключитесь в течение 5 минут',
    action: 'Повторить',
  );
  static const writeTimeout = AppFailure(
    kind: AppErrorKind.writeTimeout,
    message:
        'Устройство не подтвердило операцию. Фактическое состояние проверено повторно',
    action: 'Повторить',
  );

  static AppFailure incompatible(int deviceMajor, int appMajor) => AppFailure(
    kind: AppErrorKind.incompatibleProtocol,
    message:
        'Прошивка использует протокол $deviceMajor.x, приложение — $appMajor.x. Настройка недоступна',
    action: 'Обновить приложение',
  );

  static AppFailure validation(String message, {int? fieldId}) => AppFailure(
    kind: AppErrorKind.validationFailed,
    message: message,
    fieldId: fieldId,
  );

  static AppFailure malformed(String message, [Object? cause]) => AppFailure(
    kind: AppErrorKind.malformedPacket,
    message: message,
    cause: cause,
  );

  static AppFailure unknown(Object cause) => AppFailure(
    kind: AppErrorKind.unknown,
    message: 'Не удалось выполнить операцию',
    cause: cause,
  );
}
