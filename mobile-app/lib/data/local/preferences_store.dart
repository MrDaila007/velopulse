import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../../domain/entities/companion_models.dart';
import '../../domain/entities/models.dart';

class RememberedDevice {
  const RememberedDevice({required this.id, required this.name});
  final String id;
  final String name;
}

class PersistedDraft {
  const PersistedDraft({
    required this.deviceId,
    required this.base,
    required this.draft,
    required this.savedAt,
  });

  final String deviceId;
  final DeviceConfig base;
  final DeviceConfig draft;
  final DateTime savedAt;
}

class PreferencesStore {
  PreferencesStore({SharedPreferencesAsync? preferences})
    : _preferences = preferences ?? SharedPreferencesAsync();

  static const _rememberedIdKey = 'bikecomp.v1.remembered_device.id';
  static const _rememberedNameKey = 'bikecomp.v1.remembered_device.name';
  static const _draftPrefix = 'bikecomp.v1.config_draft.';
  static const _showClockKey = 'bikecomp.v1.companion.show_clock';
  static const _showWeatherKey = 'bikecomp.v1.companion.show_weather';
  static const _weatherCityKey = 'bikecomp.v1.companion.weather_city';
  static const _weatherFahrenheitKey = 'bikecomp.v1.companion.weather_f';

  final SharedPreferencesAsync _preferences;

  Future<RememberedDevice?> readRememberedDevice() async {
    final id = await _preferences.getString(_rememberedIdKey);
    if (id == null || id.isEmpty) return null;
    final name = await _preferences.getString(_rememberedNameKey) ?? 'BikeComp';
    return RememberedDevice(id: id, name: name);
  }

  Future<void> rememberDevice(String id, String name) async {
    await _preferences.setString(_rememberedIdKey, id);
    await _preferences.setString(_rememberedNameKey, name);
  }

  Future<void> forgetDevice() async {
    await _preferences.remove(_rememberedIdKey);
    await _preferences.remove(_rememberedNameKey);
  }

  String _draftKey(String deviceId) =>
      '$_draftPrefix${base64Url.encode(utf8.encode(deviceId))}';

  Future<PersistedDraft?> readDraft(String deviceId) async {
    final encoded = await _preferences.getString(_draftKey(deviceId));
    if (encoded == null) return null;
    try {
      final json = jsonDecode(encoded) as Map<String, Object?>;
      if (json['schema'] != 1 || json['deviceId'] != deviceId) return null;
      return PersistedDraft(
        deviceId: deviceId,
        base: DeviceConfig.fromJson(json['base']! as Map<String, Object?>),
        draft: DeviceConfig.fromJson(json['draft']! as Map<String, Object?>),
        savedAt: DateTime.parse(json['savedAt']! as String),
      );
    } on Object {
      await clearDraft(deviceId);
      return null;
    }
  }

  Future<void> writeDraft(PersistedDraft value) => _preferences.setString(
    _draftKey(value.deviceId),
    jsonEncode(<String, Object?>{
      'schema': 1,
      'deviceId': value.deviceId,
      'base': value.base.toJson(),
      'draft': value.draft.toJson(),
      'savedAt': value.savedAt.toUtc().toIso8601String(),
    }),
  );

  Future<void> clearDraft(String deviceId) =>
      _preferences.remove(_draftKey(deviceId));

  Future<CompanionPreferences> readCompanionPreferences() async {
    return CompanionPreferences(
      showClockOnDevice: await _preferences.getBool(_showClockKey) ?? true,
      showWeatherOnDevice: await _preferences.getBool(_showWeatherKey) ?? true,
      weatherCityId:
          await _preferences.getString(_weatherCityKey) ?? 'minsk',
      weatherUseFahrenheit:
          await _preferences.getBool(_weatherFahrenheitKey) ?? false,
    );
  }

  Future<void> writeCompanionPreferences(CompanionPreferences value) async {
    await _preferences.setBool(_showClockKey, value.showClockOnDevice);
    await _preferences.setBool(_showWeatherKey, value.showWeatherOnDevice);
    await _preferences.setString(_weatherCityKey, value.weatherCityId);
    await _preferences.setBool(_weatherFahrenheitKey, value.weatherUseFahrenheit);
  }
}
