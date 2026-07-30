// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Russian (`ru`).
class AppLocalizationsRu extends AppLocalizations {
  AppLocalizationsRu([String locale = 'ru']) : super(locale);

  @override
  String get appTitle => 'BikeComp';

  @override
  String get scanTab => 'Поиск';

  @override
  String get dashboardTab => 'Показатели';

  @override
  String get settingsTab => 'Настройки';

  @override
  String get maintenanceTab => 'Обслуживание';

  @override
  String get scanTitle => 'Велокомпьютеры рядом';

  @override
  String get scanAction => 'Искать';

  @override
  String get stopScanAction => 'Остановить';

  @override
  String get forgetAction => 'Забыть';

  @override
  String get connectAction => 'Подключить';

  @override
  String get saveAction => 'Сохранить';

  @override
  String get defaultsAction => 'Значения по умолчанию';

  @override
  String get retryAction => 'Повторить';

  @override
  String get openSettingsAction => 'Открыть настройки';

  @override
  String get bluetoothOff => 'Bluetooth выключен';

  @override
  String get permissionRequired => 'Нужно разрешение на поиск устройств рядом';

  @override
  String get locationOff =>
      'Для поиска на Android 11 и ниже включите геолокацию';

  @override
  String get noDevices =>
      'Велокомпьютеры не найдены. Прокрутите колесо, чтобы разбудить устройство.';

  @override
  String get notConnected => 'Нет подключения';

  @override
  String get resetTrip => 'Сбросить поездку';

  @override
  String get displayOn => 'Включить OLED';

  @override
  String get displayOff => 'Выключить OLED';

  @override
  String get displayTest => 'Тест дисплея';

  @override
  String get forceSave => 'Сохранить во Flash';

  @override
  String get sensorTest => 'Тест датчика';

  @override
  String get stopSensorTest => 'Остановить тест';

  @override
  String get confirm => 'Подтвердить';

  @override
  String get cancel => 'Отмена';
}
