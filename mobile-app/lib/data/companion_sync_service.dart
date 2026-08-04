import 'dart:async';
import 'dart:developer' as developer;

import '../core/app_error.dart';
import '../core/result.dart';
import '../domain/entities/companion_models.dart';
import 'bike_computer_repository.dart';
import 'local/preferences_store.dart';
import 'weather/open_meteo_client.dart';
import 'weather/weather_cities.dart';

class CompanionSyncService {
  CompanionSyncService({
    required this._preferences,
    OpenMeteoClient? weatherClient,
  }) : _weatherClient = weatherClient ?? OpenMeteoClient();

  final PreferencesStore _preferences;
  final OpenMeteoClient _weatherClient;
  BikeComputerRepositoryImpl? _repository;
  bool _companionSupported = false;
  Timer? _timer;

  void configure({required bool companionSupported}) {
    _companionSupported = companionSupported;
  }

  void start(BikeComputerRepositoryImpl repository) {
    _repository = repository;
    unawaited(sync());
    _timer?.cancel();
    _timer = Timer.periodic(
      const Duration(minutes: 15),
      (_) => unawaited(sync()),
    );
  }

  void stop() {
    _timer?.cancel();
    _timer = null;
    _repository = null;
  }

  Future<bool> sync() async {
    final repository = _repository;
    if (repository == null || !_companionSupported) return false;

    try {
      final prefs = await _preferences.readCompanionPreferences();
      final nowUtc = DateTime.now().toUtc();
      var flags = 0;
      var tempCX10 = CompanionSnapshot.tempInvalid;
      var popPct = CompanionSnapshot.popInvalid;
      final validUntil =
          nowUtc.add(const Duration(minutes: 30)).millisecondsSinceEpoch ~/
          1000;

      if (prefs.showClockOnDevice) {
        flags |= CompanionSnapshot.flagTimeValid;
      }

      if (prefs.showWeatherOnDevice) {
        final city =
            weatherCityById(prefs.weatherCityId) ?? kWeatherCities.first;
        final weather = await _weatherClient.fetch(city);
        if (weather != null) {
          flags |= CompanionSnapshot.flagWeatherValid;
          tempCX10 = weather.tempCX10;
          popPct = weather.popPct;
          if (weather.rainNow) flags |= CompanionSnapshot.flagRainNow;
          if (weather.rainSoon) flags |= CompanionSnapshot.flagRainSoon;
          if (weather.stale) flags |= CompanionSnapshot.flagStale;
        }
      }

      if (flags == 0) return false;

      final snapshot = CompanionSnapshot(
        unixTime: nowUtc.millisecondsSinceEpoch ~/ 1000,
        tzOffsetMin: DateTime.now().timeZoneOffset.inMinutes,
        tempCX10: tempCX10,
        popPct: popPct,
        flags: flags,
        validUntil: validUntil,
      );
      final result = await repository.writeCompanionSnapshot(snapshot);
      if (result case Failure<void>(error: final AppError error)) {
        developer.log(
          'Companion sync failed: ${error.message}',
          name: 'CompanionSyncService',
        );
        return false;
      }
      developer.log('Companion sync ok', name: 'CompanionSyncService');
      return true;
    } on Object catch (error, stackTrace) {
      developer.log(
        'Companion sync error',
        name: 'CompanionSyncService',
        error: error,
        stackTrace: stackTrace,
      );
      return false;
    }
  }
}
