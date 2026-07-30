import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_ru.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations)!;
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[Locale('ru')];

  /// No description provided for @accessRequiredTitle.
  ///
  /// In ru, this message translates to:
  /// **'Нужен доступ'**
  String get accessRequiredTitle;

  /// No description provided for @appTitle.
  ///
  /// In ru, this message translates to:
  /// **'BikeComp'**
  String get appTitle;

  /// No description provided for @applyDraftAction.
  ///
  /// In ru, this message translates to:
  /// **'Применить черновик'**
  String get applyDraftAction;

  /// No description provided for @averageLabel.
  ///
  /// In ru, this message translates to:
  /// **'Средняя'**
  String get averageLabel;

  /// No description provided for @batteryLabel.
  ///
  /// In ru, this message translates to:
  /// **'Батарея'**
  String get batteryLabel;

  /// No description provided for @batteryValue.
  ///
  /// In ru, this message translates to:
  /// **'{percent}% · {millivolts} мВ'**
  String batteryValue(int millivolts, int percent);

  /// No description provided for @bluetoothOff.
  ///
  /// In ru, this message translates to:
  /// **'Bluetooth выключен'**
  String get bluetoothOff;

  /// No description provided for @bluetoothOffShort.
  ///
  /// In ru, this message translates to:
  /// **'Bluetooth выкл.'**
  String get bluetoothOffShort;

  /// No description provided for @bondUnknown.
  ///
  /// In ru, this message translates to:
  /// **'сопряжение неизвестно'**
  String get bondUnknown;

  /// No description provided for @bonded.
  ///
  /// In ru, this message translates to:
  /// **'сопряжено'**
  String get bonded;

  /// No description provided for @brightnessValue.
  ///
  /// In ru, this message translates to:
  /// **'Яркость: {percent}%'**
  String brightnessValue(int percent);

  /// No description provided for @cancel.
  ///
  /// In ru, this message translates to:
  /// **'Отмена'**
  String get cancel;

  /// No description provided for @commandDeviceBody.
  ///
  /// In ru, this message translates to:
  /// **'Команда будет выполнена на подключённом устройстве.'**
  String get commandDeviceBody;

  /// No description provided for @commandsRequireReady.
  ///
  /// In ru, this message translates to:
  /// **'Для команд требуется готовое соединение с совместимым протоколом.'**
  String get commandsRequireReady;

  /// No description provided for @confirm.
  ///
  /// In ru, this message translates to:
  /// **'Подтвердить'**
  String get confirm;

  /// No description provided for @connectAction.
  ///
  /// In ru, this message translates to:
  /// **'Подключить'**
  String get connectAction;

  /// No description provided for @connectToReadSettings.
  ///
  /// In ru, this message translates to:
  /// **'Подключите BikeComp, чтобы прочитать настройки.'**
  String get connectToReadSettings;

  /// No description provided for @connectToSeeRide.
  ///
  /// In ru, this message translates to:
  /// **'Подключите BikeComp, чтобы увидеть данные поездки.'**
  String get connectToSeeRide;

  /// No description provided for @connectedStatus.
  ///
  /// In ru, this message translates to:
  /// **'Подключено'**
  String get connectedStatus;

  /// No description provided for @connectingStatus.
  ///
  /// In ru, this message translates to:
  /// **'Подключение'**
  String get connectingStatus;

  /// No description provided for @connectionLost.
  ///
  /// In ru, this message translates to:
  /// **'Соединение потеряно. Последнее обновление: {lastSeen}'**
  String connectionLost(String lastSeen);

  /// No description provided for @connectionSemantics.
  ///
  /// In ru, this message translates to:
  /// **'Состояние соединения: {status}'**
  String connectionSemantics(String status);

  /// No description provided for @connectionTitle.
  ///
  /// In ru, this message translates to:
  /// **'Подключение'**
  String get connectionTitle;

  /// No description provided for @customValue.
  ///
  /// In ru, this message translates to:
  /// **'Пользовательский'**
  String get customValue;

  /// No description provided for @dashboardTab.
  ///
  /// In ru, this message translates to:
  /// **'Показатели'**
  String get dashboardTab;

  /// No description provided for @defaultBrightnessChange.
  ///
  /// In ru, this message translates to:
  /// **'яркость: {current} → {defaults}%'**
  String defaultBrightnessChange(int current, int defaults);

  /// No description provided for @defaultCircumferenceChange.
  ///
  /// In ru, this message translates to:
  /// **'окружность: {current} → {defaults} мм'**
  String defaultCircumferenceChange(int current, int defaults);

  /// No description provided for @defaultsAction.
  ///
  /// In ru, this message translates to:
  /// **'Значения по умолчанию'**
  String get defaultsAction;

  /// No description provided for @defaultsShortAction.
  ///
  /// In ru, this message translates to:
  /// **'По умолчанию'**
  String get defaultsShortAction;

  /// No description provided for @deviceConnectedTitle.
  ///
  /// In ru, this message translates to:
  /// **'Устройство подключено'**
  String get deviceConnectedTitle;

  /// No description provided for @deviceProtocol.
  ///
  /// In ru, this message translates to:
  /// **'{model} · протокол {major}.{minor}'**
  String deviceProtocol(int major, int minor, String model);

  /// No description provided for @deviceScanDetails.
  ///
  /// In ru, this message translates to:
  /// **'{deviceId}\nRSSI {rssi} dBm · {bond}'**
  String deviceScanDetails(String bond, String deviceId, int rssi);

  /// No description provided for @discardAction.
  ///
  /// In ru, this message translates to:
  /// **'Отбросить'**
  String get discardAction;

  /// No description provided for @disconnectedStatus.
  ///
  /// In ru, this message translates to:
  /// **'Не подключено'**
  String get disconnectedStatus;

  /// No description provided for @displayOff.
  ///
  /// In ru, this message translates to:
  /// **'Выключить OLED'**
  String get displayOff;

  /// No description provided for @displayOffShort.
  ///
  /// In ru, this message translates to:
  /// **'OLED выкл.'**
  String get displayOffShort;

  /// No description provided for @displayOn.
  ///
  /// In ru, this message translates to:
  /// **'Включить OLED'**
  String get displayOn;

  /// No description provided for @displayOnShort.
  ///
  /// In ru, this message translates to:
  /// **'OLED вкл.'**
  String get displayOnShort;

  /// No description provided for @displayStateSubtitle.
  ///
  /// In ru, this message translates to:
  /// **'Временно изменить состояние экрана'**
  String get displayStateSubtitle;

  /// No description provided for @displayStopSection.
  ///
  /// In ru, this message translates to:
  /// **'Дисплей и остановка'**
  String get displayStopSection;

  /// No description provided for @displayTest.
  ///
  /// In ru, this message translates to:
  /// **'Тест дисплея'**
  String get displayTest;

  /// No description provided for @displayTestSubtitle.
  ///
  /// In ru, this message translates to:
  /// **'Проверить сегменты и яркость OLED'**
  String get displayTestSubtitle;

  /// No description provided for @displayTimeoutChange.
  ///
  /// In ru, this message translates to:
  /// **'таймаут дисплея'**
  String get displayTimeoutChange;

  /// No description provided for @displayTimeoutLabel.
  ///
  /// In ru, this message translates to:
  /// **'Выключить дисплей через, с'**
  String get displayTimeoutLabel;

  /// No description provided for @draftConflict.
  ///
  /// In ru, this message translates to:
  /// **'Настройки устройства изменились после сохранения черновика.'**
  String get draftConflict;

  /// No description provided for @draftDescription.
  ///
  /// In ru, this message translates to:
  /// **'Изменения хранятся как черновик до подтверждённой записи.'**
  String get draftDescription;

  /// No description provided for @enableBluetoothBody.
  ///
  /// In ru, this message translates to:
  /// **'Включите Bluetooth, чтобы начать поиск.'**
  String get enableBluetoothBody;

  /// No description provided for @errorStatus.
  ///
  /// In ru, this message translates to:
  /// **'Ошибка'**
  String get errorStatus;

  /// No description provided for @establishingConnection.
  ///
  /// In ru, this message translates to:
  /// **'Устанавливаем соединение…'**
  String get establishingConnection;

  /// No description provided for @fixErrors.
  ///
  /// In ru, this message translates to:
  /// **'{count, plural, one{Исправьте {count} ошибку.} few{Исправьте {count} ошибки.} many{Исправьте {count} ошибок.} other{Исправьте {count} ошибки.}}'**
  String fixErrors(int count);

  /// No description provided for @forceSave.
  ///
  /// In ru, this message translates to:
  /// **'Сохранить во Flash'**
  String get forceSave;

  /// No description provided for @forceSaveSubtitle.
  ///
  /// In ru, this message translates to:
  /// **'Принудительно сохранить текущее состояние'**
  String get forceSaveSubtitle;

  /// No description provided for @forgetAction.
  ///
  /// In ru, this message translates to:
  /// **'Забыть'**
  String get forgetAction;

  /// No description provided for @imperialUnits.
  ///
  /// In ru, this message translates to:
  /// **'мили / mph'**
  String get imperialUnits;

  /// No description provided for @incompatibleProtocolTitle.
  ///
  /// In ru, this message translates to:
  /// **'Несовместимый протокол'**
  String get incompatibleProtocolTitle;

  /// No description provided for @incompatibleStatus.
  ///
  /// In ru, this message translates to:
  /// **'Несовместимо'**
  String get incompatibleStatus;

  /// No description provided for @incompatibleTelemetry.
  ///
  /// In ru, this message translates to:
  /// **'Информация об устройстве доступна, но telemetry протокола несовместима.'**
  String get incompatibleTelemetry;

  /// No description provided for @linkAvailable.
  ///
  /// In ru, this message translates to:
  /// **'есть'**
  String get linkAvailable;

  /// No description provided for @linkLabel.
  ///
  /// In ru, this message translates to:
  /// **'Связь'**
  String get linkLabel;

  /// No description provided for @linkUnavailable.
  ///
  /// In ru, this message translates to:
  /// **'нет'**
  String get linkUnavailable;

  /// No description provided for @loadAction.
  ///
  /// In ru, this message translates to:
  /// **'Загрузить'**
  String get loadAction;

  /// No description provided for @loadDefaultsQuestion.
  ///
  /// In ru, this message translates to:
  /// **'Загрузить значения по умолчанию?'**
  String get loadDefaultsQuestion;

  /// No description provided for @localDraftOnly.
  ///
  /// In ru, this message translates to:
  /// **'Будет изменён только локальный черновик:'**
  String get localDraftOnly;

  /// No description provided for @locationOff.
  ///
  /// In ru, this message translates to:
  /// **'Для поиска на Android 11 и ниже включите геолокацию'**
  String get locationOff;

  /// No description provided for @maintenanceConfirmedOnly.
  ///
  /// In ru, this message translates to:
  /// **'Команда считается выполненной только после подтверждения устройства.'**
  String get maintenanceConfirmedOnly;

  /// No description provided for @maintenanceTab.
  ///
  /// In ru, this message translates to:
  /// **'Обслуживание'**
  String get maintenanceTab;

  /// No description provided for @maximumLabel.
  ///
  /// In ru, this message translates to:
  /// **'Максимальная'**
  String get maximumLabel;

  /// No description provided for @measurementUnitsChange.
  ///
  /// In ru, this message translates to:
  /// **'единицы измерения'**
  String get measurementUnitsChange;

  /// No description provided for @metricUnits.
  ///
  /// In ru, this message translates to:
  /// **'км / км/ч'**
  String get metricUnits;

  /// No description provided for @metricsLiveBody.
  ///
  /// In ru, this message translates to:
  /// **'Показатели обновляются в реальном времени.'**
  String get metricsLiveBody;

  /// No description provided for @millisecondsShort.
  ///
  /// In ru, this message translates to:
  /// **'{milliseconds} мс'**
  String millisecondsShort(int milliseconds);

  /// No description provided for @movementLabel.
  ///
  /// In ru, this message translates to:
  /// **'Движение'**
  String get movementLabel;

  /// No description provided for @movingTimeLabel.
  ///
  /// In ru, this message translates to:
  /// **'В движении'**
  String get movingTimeLabel;

  /// No description provided for @neverHelper.
  ///
  /// In ru, this message translates to:
  /// **'0 — никогда'**
  String get neverHelper;

  /// No description provided for @noDevices.
  ///
  /// In ru, this message translates to:
  /// **'Велокомпьютеры не найдены. Прокрутите колесо, чтобы разбудить устройство.'**
  String get noDevices;

  /// No description provided for @noMetrics.
  ///
  /// In ru, this message translates to:
  /// **'Нет показателей'**
  String get noMetrics;

  /// No description provided for @noUnsavedChanges.
  ///
  /// In ru, this message translates to:
  /// **'Нет несохранённых изменений.'**
  String get noUnsavedChanges;

  /// No description provided for @notBonded.
  ///
  /// In ru, this message translates to:
  /// **'не сопряжено'**
  String get notBonded;

  /// No description provided for @notConnected.
  ///
  /// In ru, this message translates to:
  /// **'Нет подключения'**
  String get notConnected;

  /// No description provided for @odometerBondsUntouched.
  ///
  /// In ru, this message translates to:
  /// **'Одометр и Bluetooth-сопряжения не затрагиваются.'**
  String get odometerBondsUntouched;

  /// No description provided for @odometerLabel.
  ///
  /// In ru, this message translates to:
  /// **'Одометр'**
  String get odometerLabel;

  /// No description provided for @openSettingsAction.
  ///
  /// In ru, this message translates to:
  /// **'Открыть настройки'**
  String get openSettingsAction;

  /// No description provided for @pagePeriodChange.
  ///
  /// In ru, this message translates to:
  /// **'период переключения страниц'**
  String get pagePeriodChange;

  /// No description provided for @pageSwitchLabel.
  ///
  /// In ru, this message translates to:
  /// **'Переключать страницу каждые, с'**
  String get pageSwitchLabel;

  /// No description provided for @permissionRequired.
  ///
  /// In ru, this message translates to:
  /// **'Нужно разрешение на поиск устройств рядом'**
  String get permissionRequired;

  /// No description provided for @permissionsFirstScanBody.
  ///
  /// In ru, this message translates to:
  /// **'Разрешения будут запрошены при первом поиске.'**
  String get permissionsFirstScanBody;

  /// No description provided for @preparingConnection.
  ///
  /// In ru, this message translates to:
  /// **'Подготовка подключения…'**
  String get preparingConnection;

  /// No description provided for @protocolIncompatible.
  ///
  /// In ru, this message translates to:
  /// **'Версия протокола несовместима'**
  String get protocolIncompatible;

  /// No description provided for @pulseAgeLabel.
  ///
  /// In ru, this message translates to:
  /// **'Возраст импульса'**
  String get pulseAgeLabel;

  /// No description provided for @quickActions.
  ///
  /// In ru, this message translates to:
  /// **'Быстрые действия'**
  String get quickActions;

  /// No description provided for @readOnlyStatus.
  ///
  /// In ru, this message translates to:
  /// **'Только чтение'**
  String get readOnlyStatus;

  /// No description provided for @readOnlyTitle.
  ///
  /// In ru, this message translates to:
  /// **'Режим только для чтения'**
  String get readOnlyTitle;

  /// No description provided for @readyToScanTitle.
  ///
  /// In ru, this message translates to:
  /// **'Готово к поиску'**
  String get readyToScanTitle;

  /// No description provided for @reconnectAttempt.
  ///
  /// In ru, this message translates to:
  /// **'Попытка {attempt} через {seconds} с'**
  String reconnectAttempt(int attempt, int seconds);

  /// No description provided for @reconnectingStatus.
  ///
  /// In ru, this message translates to:
  /// **'Переподключение'**
  String get reconnectingStatus;

  /// No description provided for @refreshAction.
  ///
  /// In ru, this message translates to:
  /// **'Обновить'**
  String get refreshAction;

  /// No description provided for @resetCurrentTripQuestion.
  ///
  /// In ru, this message translates to:
  /// **'Сбросить текущую поездку?'**
  String get resetCurrentTripQuestion;

  /// No description provided for @resetTrip.
  ///
  /// In ru, this message translates to:
  /// **'Сбросить поездку'**
  String get resetTrip;

  /// No description provided for @resetTripQuestion.
  ///
  /// In ru, this message translates to:
  /// **'Сбросить поездку?'**
  String get resetTripQuestion;

  /// No description provided for @resetTripSubtitle.
  ///
  /// In ru, this message translates to:
  /// **'Обнулить trip, среднюю скорость и время поездки'**
  String get resetTripSubtitle;

  /// No description provided for @retryAction.
  ///
  /// In ru, this message translates to:
  /// **'Повторить'**
  String get retryAction;

  /// No description provided for @returnToScan.
  ///
  /// In ru, this message translates to:
  /// **'Вернуться к поиску'**
  String get returnToScan;

  /// No description provided for @revolutionsLabel.
  ///
  /// In ru, this message translates to:
  /// **'Обороты'**
  String get revolutionsLabel;

  /// No description provided for @rideIdle.
  ///
  /// In ru, this message translates to:
  /// **'остановка'**
  String get rideIdle;

  /// No description provided for @rideMoving.
  ///
  /// In ru, this message translates to:
  /// **'движение'**
  String get rideMoving;

  /// No description provided for @ridePaused.
  ///
  /// In ru, this message translates to:
  /// **'пауза'**
  String get ridePaused;

  /// No description provided for @saveAction.
  ///
  /// In ru, this message translates to:
  /// **'Сохранить'**
  String get saveAction;

  /// No description provided for @savingAction.
  ///
  /// In ru, this message translates to:
  /// **'Сохраняем…'**
  String get savingAction;

  /// No description provided for @scanAction.
  ///
  /// In ru, this message translates to:
  /// **'Искать'**
  String get scanAction;

  /// No description provided for @scanDescription.
  ///
  /// In ru, this message translates to:
  /// **'Показываются только устройства с сервисом BikeComp. Сохранённое устройство подключится автоматически.'**
  String get scanDescription;

  /// No description provided for @scanTab.
  ///
  /// In ru, this message translates to:
  /// **'Поиск'**
  String get scanTab;

  /// No description provided for @scanTitle.
  ///
  /// In ru, this message translates to:
  /// **'Велокомпьютеры рядом'**
  String get scanTitle;

  /// No description provided for @scanningStatus.
  ///
  /// In ru, this message translates to:
  /// **'Поиск'**
  String get scanningStatus;

  /// No description provided for @secondsShort.
  ///
  /// In ru, this message translates to:
  /// **'{seconds} с'**
  String secondsShort(int seconds);

  /// No description provided for @sensorIdle.
  ///
  /// In ru, this message translates to:
  /// **'ожидание'**
  String get sensorIdle;

  /// No description provided for @sensorLabel.
  ///
  /// In ru, this message translates to:
  /// **'Датчик'**
  String get sensorLabel;

  /// No description provided for @sensorNoSignal.
  ///
  /// In ru, this message translates to:
  /// **'нет сигнала'**
  String get sensorNoSignal;

  /// No description provided for @sensorOk.
  ///
  /// In ru, this message translates to:
  /// **'норма'**
  String get sensorOk;

  /// No description provided for @sensorStuck.
  ///
  /// In ru, this message translates to:
  /// **'залипание'**
  String get sensorStuck;

  /// No description provided for @sensorTest.
  ///
  /// In ru, this message translates to:
  /// **'Тест датчика'**
  String get sensorTest;

  /// No description provided for @sensorTestActiveBody.
  ///
  /// In ru, this message translates to:
  /// **'Вращайте колесо. Показатели обновляются с частотой 5 Гц.'**
  String get sensorTestActiveBody;

  /// No description provided for @sensorTestIdleBody.
  ///
  /// In ru, this message translates to:
  /// **'Базовый тест длится 60 секунд и не изменяет настройки.'**
  String get sensorTestIdleBody;

  /// No description provided for @settingsTab.
  ///
  /// In ru, this message translates to:
  /// **'Настройки'**
  String get settingsTab;

  /// No description provided for @speedLabel.
  ///
  /// In ru, this message translates to:
  /// **'СКОРОСТЬ'**
  String get speedLabel;

  /// No description provided for @startSensorTest.
  ///
  /// In ru, this message translates to:
  /// **'Начать тест'**
  String get startSensorTest;

  /// No description provided for @stateLabel.
  ///
  /// In ru, this message translates to:
  /// **'Состояние'**
  String get stateLabel;

  /// No description provided for @stopScanAction.
  ///
  /// In ru, this message translates to:
  /// **'Остановить'**
  String get stopScanAction;

  /// No description provided for @stopSensorTest.
  ///
  /// In ru, this message translates to:
  /// **'Остановить тест'**
  String get stopSensorTest;

  /// No description provided for @stopTimeoutChange.
  ///
  /// In ru, this message translates to:
  /// **'таймаут остановки'**
  String get stopTimeoutChange;

  /// No description provided for @stopTimeoutLabel.
  ///
  /// In ru, this message translates to:
  /// **'Пауза после остановки, с'**
  String get stopTimeoutLabel;

  /// No description provided for @syncDeviceInfo.
  ///
  /// In ru, this message translates to:
  /// **'Чтение информации'**
  String get syncDeviceInfo;

  /// No description provided for @syncDiscovery.
  ///
  /// In ru, this message translates to:
  /// **'Поиск BLE-сервиса'**
  String get syncDiscovery;

  /// No description provided for @syncMtu.
  ///
  /// In ru, this message translates to:
  /// **'Согласование MTU'**
  String get syncMtu;

  /// No description provided for @syncPairing.
  ///
  /// In ru, this message translates to:
  /// **'Сопряжение и конфигурация'**
  String get syncPairing;

  /// No description provided for @syncSubscriptions.
  ///
  /// In ru, this message translates to:
  /// **'Подписка на данные'**
  String get syncSubscriptions;

  /// No description provided for @tirePresetValue.
  ///
  /// In ru, this message translates to:
  /// **'{name} · {millimeters} мм'**
  String tirePresetValue(int millimeters, String name);

  /// No description provided for @tireSizeLabel.
  ///
  /// In ru, this message translates to:
  /// **'Размер покрышки'**
  String get tireSizeLabel;

  /// No description provided for @toScanAction.
  ///
  /// In ru, this message translates to:
  /// **'К поиску'**
  String get toScanAction;

  /// No description provided for @tripLabel.
  ///
  /// In ru, this message translates to:
  /// **'Поездка'**
  String get tripLabel;

  /// No description provided for @unableContinueTitle.
  ///
  /// In ru, this message translates to:
  /// **'Не удалось продолжить'**
  String get unableContinueTitle;

  /// No description provided for @unknownValue.
  ///
  /// In ru, this message translates to:
  /// **'неизвестно'**
  String get unknownValue;

  /// No description provided for @visibleSameHiddenRestored.
  ///
  /// In ru, this message translates to:
  /// **'видимые значения уже совпадают; скрытые поля будут восстановлены'**
  String get visibleSameHiddenRestored;

  /// No description provided for @waitForConfirmation.
  ///
  /// In ru, this message translates to:
  /// **'Дождитесь подтверждения устройства.'**
  String get waitForConfirmation;

  /// No description provided for @wheelCircumferenceLabel.
  ///
  /// In ru, this message translates to:
  /// **'Окружность колеса, мм'**
  String get wheelCircumferenceLabel;

  /// No description provided for @wheelUnitsSection.
  ///
  /// In ru, this message translates to:
  /// **'Колесо и единицы'**
  String get wheelUnitsSection;

  /// No description provided for @writeBlockedDescription.
  ///
  /// In ru, this message translates to:
  /// **'Просмотр доступен, запись заблокирована состоянием соединения.'**
  String get writeBlockedDescription;
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) =>
      <String>['ru'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'ru':
      return AppLocalizationsRu();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
