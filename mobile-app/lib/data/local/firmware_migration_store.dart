import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../../domain/entities/models.dart';

class FirmwareMigrationBackup {
  const FirmwareMigrationBackup({
    required this.deviceId,
    required this.deviceName,
    required this.config,
    required this.odometerM,
    required this.savedAt,
    this.fwVersion,
  });

  final String deviceId;
  final String deviceName;
  final DeviceConfig config;
  final int odometerM;
  final DateTime savedAt;
  final String? fwVersion;

  Map<String, Object?> toJson() => <String, Object?>{
        'schema': 1,
        'deviceId': deviceId,
        'deviceName': deviceName,
        'config': config.toJson(),
        'odometerM': odometerM,
        'savedAt': savedAt.toUtc().toIso8601String(),
        if (fwVersion != null) 'fwVersion': fwVersion,
      };

  static FirmwareMigrationBackup? fromJson(Map<String, Object?> json) {
    if (json['schema'] != 1) return null;
    final deviceId = json['deviceId'];
    if (deviceId is! String || deviceId.isEmpty) return null;
    return FirmwareMigrationBackup(
      deviceId: deviceId,
      deviceName: json['deviceName'] as String? ?? 'BikeComp',
      config: DeviceConfig.fromJson(json['config']! as Map<String, Object?>),
      odometerM: json['odometerM'] as int? ?? 0,
      savedAt: DateTime.parse(json['savedAt']! as String),
      fwVersion: json['fwVersion'] as String?,
    );
  }
}

class FirmwareMigrationStore {
  FirmwareMigrationStore({SharedPreferencesAsync? preferences})
      : _preferences = preferences ?? SharedPreferencesAsync();

  static const _backupKey = 'bikecomp.v1.firmware_migration.backup';

  final SharedPreferencesAsync _preferences;

  Future<FirmwareMigrationBackup?> readBackup() async {
    final encoded = await _preferences.getString(_backupKey);
    if (encoded == null) return null;
    try {
      final json = jsonDecode(encoded) as Map<String, Object?>;
      return FirmwareMigrationBackup.fromJson(json);
    } on Object {
      await clearBackup();
      return null;
    }
  }

  Future<void> writeBackup(FirmwareMigrationBackup value) =>
      _preferences.setString(_backupKey, jsonEncode(value.toJson()));

  Future<void> clearBackup() => _preferences.remove(_backupKey);
}
