import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../domain/entities/companion_models.dart';
import 'weather_cities.dart';

class OpenMeteoClient {
  OpenMeteoClient({http.Client? client}) : _client = client ?? http.Client();

  final http.Client _client;
  DateTime? _lastFetchAt;
  WeatherReading? _cached;

  Future<WeatherReading?> fetch(WeatherCity city) async {
    final now = DateTime.now().toUtc();
    if (_cached != null &&
        _lastFetchAt != null &&
        now.difference(_lastFetchAt!) < const Duration(minutes: 15)) {
      return _cached;
    }

    final uri =
        Uri.https('api.open-meteo.com', '/v1/forecast', <String, String>{
          'latitude': city.latitude.toString(),
          'longitude': city.longitude.toString(),
          'current': 'temperature_2m,precipitation',
          'hourly': 'precipitation_probability',
          'forecast_hours': '1',
          'timezone': 'GMT',
        });

    try {
      final response = await _client
          .get(uri)
          .timeout(const Duration(seconds: 8));
      if (response.statusCode != 200) {
        return _staleOrNull();
      }
      final json = jsonDecode(response.body) as Map<String, Object?>;
      final current = json['current'] as Map<String, Object?>?;
      if (current == null) return _staleOrNull();

      final temp = (current['temperature_2m'] as num?)?.toDouble();
      final precipitation = (current['precipitation'] as num?)?.toDouble() ?? 0;
      final hourly = json['hourly'] as Map<String, Object?>?;
      final popList = hourly?['precipitation_probability'] as List<Object?>?;
      final pop = popList == null || popList.isEmpty
          ? CompanionSnapshot.popInvalid
          : (popList.first as num).round().clamp(0, 100);

      if (temp == null) return _staleOrNull();

      final reading = WeatherReading(
        tempCX10: (temp * 10).round(),
        popPct: pop,
        rainNow: precipitation > 0,
        rainSoon: pop >= 30,
        stale: false,
      );
      _cached = reading;
      _lastFetchAt = now;
      return reading;
    } on Object {
      return _staleOrNull();
    }
  }

  WeatherReading? _staleOrNull() {
    if (_cached == null) return null;
    return WeatherReading(
      tempCX10: _cached!.tempCX10,
      popPct: _cached!.popPct,
      rainNow: _cached!.rainNow,
      rainSoon: _cached!.rainSoon,
      stale: true,
    );
  }
}
