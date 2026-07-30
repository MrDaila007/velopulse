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

  /// No description provided for @appTitle.
  ///
  /// In ru, this message translates to:
  /// **'BikeComp'**
  String get appTitle;

  /// No description provided for @scanTab.
  ///
  /// In ru, this message translates to:
  /// **'Поиск'**
  String get scanTab;

  /// No description provided for @dashboardTab.
  ///
  /// In ru, this message translates to:
  /// **'Показатели'**
  String get dashboardTab;

  /// No description provided for @settingsTab.
  ///
  /// In ru, this message translates to:
  /// **'Настройки'**
  String get settingsTab;

  /// No description provided for @maintenanceTab.
  ///
  /// In ru, this message translates to:
  /// **'Обслуживание'**
  String get maintenanceTab;

  /// No description provided for @scanTitle.
  ///
  /// In ru, this message translates to:
  /// **'Велокомпьютеры рядом'**
  String get scanTitle;

  /// No description provided for @scanAction.
  ///
  /// In ru, this message translates to:
  /// **'Искать'**
  String get scanAction;

  /// No description provided for @stopScanAction.
  ///
  /// In ru, this message translates to:
  /// **'Остановить'**
  String get stopScanAction;

  /// No description provided for @forgetAction.
  ///
  /// In ru, this message translates to:
  /// **'Забыть'**
  String get forgetAction;

  /// No description provided for @connectAction.
  ///
  /// In ru, this message translates to:
  /// **'Подключить'**
  String get connectAction;

  /// No description provided for @saveAction.
  ///
  /// In ru, this message translates to:
  /// **'Сохранить'**
  String get saveAction;

  /// No description provided for @defaultsAction.
  ///
  /// In ru, this message translates to:
  /// **'Значения по умолчанию'**
  String get defaultsAction;

  /// No description provided for @retryAction.
  ///
  /// In ru, this message translates to:
  /// **'Повторить'**
  String get retryAction;

  /// No description provided for @openSettingsAction.
  ///
  /// In ru, this message translates to:
  /// **'Открыть настройки'**
  String get openSettingsAction;

  /// No description provided for @bluetoothOff.
  ///
  /// In ru, this message translates to:
  /// **'Bluetooth выключен'**
  String get bluetoothOff;

  /// No description provided for @permissionRequired.
  ///
  /// In ru, this message translates to:
  /// **'Нужно разрешение на поиск устройств рядом'**
  String get permissionRequired;

  /// No description provided for @locationOff.
  ///
  /// In ru, this message translates to:
  /// **'Для поиска на Android 11 и ниже включите геолокацию'**
  String get locationOff;

  /// No description provided for @noDevices.
  ///
  /// In ru, this message translates to:
  /// **'Велокомпьютеры не найдены. Прокрутите колесо, чтобы разбудить устройство.'**
  String get noDevices;

  /// No description provided for @notConnected.
  ///
  /// In ru, this message translates to:
  /// **'Нет подключения'**
  String get notConnected;

  /// No description provided for @resetTrip.
  ///
  /// In ru, this message translates to:
  /// **'Сбросить поездку'**
  String get resetTrip;

  /// No description provided for @displayOn.
  ///
  /// In ru, this message translates to:
  /// **'Включить OLED'**
  String get displayOn;

  /// No description provided for @displayOff.
  ///
  /// In ru, this message translates to:
  /// **'Выключить OLED'**
  String get displayOff;

  /// No description provided for @displayTest.
  ///
  /// In ru, this message translates to:
  /// **'Тест дисплея'**
  String get displayTest;

  /// No description provided for @forceSave.
  ///
  /// In ru, this message translates to:
  /// **'Сохранить во Flash'**
  String get forceSave;

  /// No description provided for @sensorTest.
  ///
  /// In ru, this message translates to:
  /// **'Тест датчика'**
  String get sensorTest;

  /// No description provided for @stopSensorTest.
  ///
  /// In ru, this message translates to:
  /// **'Остановить тест'**
  String get stopSensorTest;

  /// No description provided for @confirm.
  ///
  /// In ru, this message translates to:
  /// **'Подтвердить'**
  String get confirm;

  /// No description provided for @cancel.
  ///
  /// In ru, this message translates to:
  /// **'Отмена'**
  String get cancel;
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
