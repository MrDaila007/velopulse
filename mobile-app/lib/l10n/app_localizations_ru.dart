// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Russian (`ru`).
class AppLocalizationsRu extends AppLocalizations {
  AppLocalizationsRu([String locale = 'ru']) : super(locale);

  @override
  String get accessRequiredTitle => 'Нужен доступ';

  @override
  String get ambientBrightnessHelper =>
      'Фоторезистор автоматически снижает яркость; ползунок задаёт доступный максимум.';

  @override
  String get appTitle => 'BikeComp';

  @override
  String get applyDraftAction => 'Применить черновик';

  @override
  String get averageLabel => 'Средняя';

  @override
  String get batteryLabel => 'Батарея';

  @override
  String batteryValue(int millivolts, int percent) {
    return '$percent% · $millivolts мВ';
  }

  @override
  String get bluetoothOff => 'Bluetooth выключен';

  @override
  String get bluetoothOffShort => 'Bluetooth выкл.';

  @override
  String get bondUnknown => 'сопряжение неизвестно';

  @override
  String get bonded => 'сопряжено';

  @override
  String brightnessValue(int percent) {
    return 'Максимальная яркость: $percent%';
  }

  @override
  String get cancel => 'Отмена';

  @override
  String get commandDeviceBody =>
      'Команда будет выполнена на подключённом устройстве.';

  @override
  String get commandsRequireReady =>
      'Для команд требуется готовое соединение с совместимым протоколом.';

  @override
  String get confirm => 'Подтвердить';

  @override
  String get connectAction => 'Подключить';

  @override
  String get connectToReadSettings =>
      'Подключите BikeComp, чтобы прочитать настройки.';

  @override
  String get connectToSeeRide =>
      'Подключите BikeComp, чтобы увидеть данные поездки.';

  @override
  String get connectedStatus => 'Подключено';

  @override
  String get connectingStatus => 'Подключение';

  @override
  String connectionLost(String lastSeen) {
    return 'Соединение потеряно. Последнее обновление: $lastSeen';
  }

  @override
  String connectionSemantics(String status) {
    return 'Состояние соединения: $status';
  }

  @override
  String get connectionTitle => 'Подключение';

  @override
  String get customValue => 'Пользовательский';

  @override
  String get dashboardTab => 'Показатели';

  @override
  String defaultBrightnessChange(int current, int defaults) {
    return 'макс. яркость: $current → $defaults%';
  }

  @override
  String defaultCircumferenceChange(int current, int defaults) {
    return 'окружность: $current → $defaults мм';
  }

  @override
  String get defaultsAction => 'Значения по умолчанию';

  @override
  String get defaultsShortAction => 'По умолчанию';

  @override
  String get deviceConnectedTitle => 'Устройство подключено';

  @override
  String deviceProtocol(int major, int minor, String model) {
    return '$model · протокол $major.$minor';
  }

  @override
  String deviceScanDetails(String bond, String deviceId, int rssi) {
    return '$deviceId\nRSSI $rssi dBm · $bond';
  }

  @override
  String get discardAction => 'Отбросить';

  @override
  String get disconnectedStatus => 'Не подключено';

  @override
  String get displayOff => 'Выключить OLED';

  @override
  String get displayOffShort => 'OLED выкл.';

  @override
  String get displayOn => 'Включить OLED';

  @override
  String get displayOnShort => 'OLED вкл.';

  @override
  String get displayPatternCheckerboard => 'Шахматная сетка';

  @override
  String get displayPatternFill => 'Заливка';

  @override
  String get displayPatternText => 'Текст';

  @override
  String get displayPatternTitle => 'Выберите тестовый паттерн';

  @override
  String get displayStateSubtitle => 'Временно изменить состояние экрана';

  @override
  String get displayStopSection => 'Дисплей и остановка';

  @override
  String get displayTest => 'Тест дисплея';

  @override
  String get displayTestSubtitle => 'Выбрать заливку, сетку или текст';

  @override
  String get displayTimeoutChange => 'таймаут дисплея';

  @override
  String get displayTimeoutLabel => 'Выключить дисплей через, с';

  @override
  String get draftConflict =>
      'Настройки устройства изменились после сохранения черновика.';

  @override
  String get draftDescription =>
      'Изменения хранятся как черновик до подтверждённой записи.';

  @override
  String get enableBluetoothBody => 'Включите Bluetooth, чтобы начать поиск.';

  @override
  String get errorStatus => 'Ошибка';

  @override
  String get establishingConnection => 'Устанавливаем соединение…';

  @override
  String get exportLog => 'Сохранить лог';

  @override
  String get exportLogEmpty =>
      'Телеметрия ещё не записана — в лог попадут только данные устройства';

  @override
  String get exportLogSubtitle =>
      'Экспорт телеметрии, диагностики и журнала ошибок';

  @override
  String get exportLogSuccess => 'Лог сохранён — выберите, куда отправить';

  @override
  String fixErrors(int count) {
    String _temp0 = intl.Intl.pluralLogic(
      count,
      locale: localeName,
      other: 'Исправьте $count ошибки.',
      many: 'Исправьте $count ошибок.',
      few: 'Исправьте $count ошибки.',
      one: 'Исправьте $count ошибку.',
    );
    return '$_temp0';
  }

  @override
  String get forceSave => 'Сохранить во Flash';

  @override
  String get forceSaveSubtitle => 'Принудительно сохранить текущее состояние';

  @override
  String get forgetAction => 'Забыть';

  @override
  String get imperialUnits => 'мили / mph';

  @override
  String get incompatibleProtocolTitle => 'Несовместимый протокол';

  @override
  String get incompatibleStatus => 'Несовместимо';

  @override
  String get incompatibleTelemetry =>
      'Информация об устройстве доступна, но telemetry протокола несовместима.';

  @override
  String get linkAvailable => 'есть';

  @override
  String get linkLabel => 'Связь';

  @override
  String get linkUnavailable => 'нет';

  @override
  String get loadAction => 'Загрузить';

  @override
  String get loadDefaultsQuestion => 'Загрузить значения по умолчанию?';

  @override
  String get localDraftOnly => 'Будет изменён только локальный черновик:';

  @override
  String get locationOff =>
      'Для поиска на Android 11 и ниже включите геолокацию';

  @override
  String get maintenanceConfirmedOnly =>
      'Команда считается выполненной только после подтверждения устройства.';

  @override
  String get maintenanceTab => 'Обслуживание';

  @override
  String get maximumLabel => 'Максимальная';

  @override
  String get measurementUnitsChange => 'единицы измерения';

  @override
  String get metricUnits => 'км / км/ч';

  @override
  String get metricsLiveBody => 'Показатели обновляются в реальном времени.';

  @override
  String millisecondsShort(int milliseconds) {
    return '$milliseconds мс';
  }

  @override
  String get movementLabel => 'Движение';

  @override
  String get movingTimeLabel => 'В движении';

  @override
  String get neverHelper => '0 — никогда';

  @override
  String get noDevices =>
      'Велокомпьютеры не найдены. Прокрутите колесо, чтобы разбудить устройство.';

  @override
  String get noMetrics => 'Нет показателей';

  @override
  String get noUnsavedChanges => 'Нет несохранённых изменений.';

  @override
  String get notBonded => 'не сопряжено';

  @override
  String get notConnected => 'Нет подключения';

  @override
  String get odometerBondsUntouched =>
      'Одометр и Bluetooth-сопряжения не затрагиваются.';

  @override
  String get odometerLabel => 'Одометр';

  @override
  String get openSettingsAction => 'Открыть настройки';

  @override
  String get pagePeriodChange => 'период переключения страниц';

  @override
  String get pageSwitchLabel => 'Переключать страницу каждые, с';

  @override
  String get permissionRequired => 'Нужно разрешение на поиск устройств рядом';

  @override
  String get permissionsFirstScanBody =>
      'Разрешения будут запрошены при первом поиске.';

  @override
  String get preparingConnection => 'Подготовка подключения…';

  @override
  String get protocolIncompatible => 'Версия протокола несовместима';

  @override
  String get pulseAgeLabel => 'Возраст импульса';

  @override
  String get quickActions => 'Быстрые действия';

  @override
  String get readOnlyStatus => 'Только чтение';

  @override
  String get readOnlyTitle => 'Режим только для чтения';

  @override
  String get readyToScanTitle => 'Готово к поиску';

  @override
  String reconnectAttempt(int attempt, int seconds) {
    return 'Попытка $attempt через $seconds с';
  }

  @override
  String get reconnectingStatus => 'Переподключение';

  @override
  String get refreshAction => 'Обновить';

  @override
  String get resetCurrentTripQuestion => 'Сбросить текущую поездку?';

  @override
  String get resetTrip => 'Сбросить поездку';

  @override
  String get resetTripQuestion => 'Сбросить поездку?';

  @override
  String get resetTripSubtitle =>
      'Обнулить trip, среднюю скорость и время поездки';

  @override
  String get retryAction => 'Повторить';

  @override
  String get returnToScan => 'Вернуться к поиску';

  @override
  String get revolutionsLabel => 'Обороты';

  @override
  String get rideIdle => 'остановка';

  @override
  String get rideMoving => 'движение';

  @override
  String get ridePaused => 'пауза';

  @override
  String get saveAction => 'Сохранить';

  @override
  String get savingAction => 'Сохраняем…';

  @override
  String get scanAction => 'Искать';

  @override
  String get scanDescription =>
      'BikeComp определяется по сервису или стандартному имени. Сохранённое устройство подключится автоматически.';

  @override
  String get scanTab => 'Поиск';

  @override
  String get scanTitle => 'Велокомпьютеры рядом';

  @override
  String get scanningStatus => 'Поиск';

  @override
  String secondsShort(int seconds) {
    return '$seconds с';
  }

  @override
  String get sensorIdle => 'ожидание';

  @override
  String get sensorLabel => 'Датчик';

  @override
  String get sensorNoSignal => 'нет сигнала';

  @override
  String get sensorOk => 'норма';

  @override
  String get sensorStuck => 'залипание';

  @override
  String get sensorTest => 'Тест датчика';

  @override
  String get sensorTestActiveBody =>
      'Вращайте колесо. Показатели обновляются с частотой 5 Гц.';

  @override
  String get sensorTestIdleBody =>
      'Базовый тест длится 60 секунд и не изменяет настройки.';

  @override
  String get settingsTab => 'Настройки';

  @override
  String get speedLabel => 'СКОРОСТЬ';

  @override
  String get startSensorTest => 'Начать тест';

  @override
  String get stateLabel => 'Состояние';

  @override
  String get stopScanAction => 'Остановить';

  @override
  String get stopSensorTest => 'Остановить тест';

  @override
  String get stopTimeoutChange => 'таймаут остановки';

  @override
  String get stopTimeoutLabel => 'Пауза после остановки, с';

  @override
  String get syncDeviceInfo => 'Чтение информации';

  @override
  String get syncDiscovery => 'Поиск BLE-сервиса';

  @override
  String get syncMtu => 'Согласование MTU';

  @override
  String get syncPairing => 'Сопряжение и конфигурация';

  @override
  String get syncSubscriptions => 'Подписка на данные';

  @override
  String tirePresetValue(int millimeters, String name) {
    return '$name · $millimeters мм';
  }

  @override
  String get tireSizeLabel => 'Размер покрышки';

  @override
  String get toScanAction => 'К поиску';

  @override
  String get tripLabel => 'Поездка';

  @override
  String get unableContinueTitle => 'Не удалось продолжить';

  @override
  String get unknownValue => 'неизвестно';

  @override
  String get visibleSameHiddenRestored =>
      'видимые значения уже совпадают; скрытые поля будут восстановлены';

  @override
  String get waitForConfirmation => 'Дождитесь подтверждения устройства.';

  @override
  String get wheelCircumferenceLabel => 'Окружность колеса, мм';

  @override
  String get wheelUnitsSection => 'Колесо и единицы';

  @override
  String get writeBlockedDescription =>
      'Просмотр доступен, запись заблокирована состоянием соединения.';

  @override
  String get firmwareBackupTitle => 'Резервная копия перед прошивкой';

  @override
  String get firmwareBackupSubtitle =>
      'Сохранить конфиг и одометр локально (Arduino → Zephyr)';

  @override
  String get firmwareBackupSuccess => 'Резервная копия сохранена на телефоне';

  @override
  String get firmwareRestoreTitle => 'Восстановить после прошивки';

  @override
  String get firmwareRestoreSubtitle =>
      'Записать сохранённые конфиг и одометр на устройство';

  @override
  String get firmwareRestoreConfirm =>
      'Восстановить конфиг и одометр из резервной копии?';

  @override
  String get firmwareRestoreSuccess => 'Данные восстановлены';

  @override
  String get firmwareBackupMissing =>
      'Резервная копия не найдена — сначала сохраните данные';

  @override
  String firmwareBackupSavedAt(Object timestamp) {
    return 'Сохранено: $timestamp';
  }
}
