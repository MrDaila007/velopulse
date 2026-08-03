class CompanionSnapshot {
  const CompanionSnapshot({
    this.structVersion = 1,
    this.unixTime = 0,
    this.tzOffsetMin = 0,
    this.tempCX10 = CompanionSnapshot.tempInvalid,
    this.popPct = CompanionSnapshot.popInvalid,
    this.flags = 0,
    this.validUntil = 0,
  });

  static const int size = 15;
  static const int tempInvalid = 0x7FFF;
  static const int popInvalid = 0xFF;
  static const int flagTimeValid = 1 << 0;
  static const int flagWeatherValid = 1 << 1;
  static const int flagRainNow = 1 << 2;
  static const int flagRainSoon = 1 << 3;
  static const int flagStale = 1 << 4;

  final int structVersion;
  final int unixTime;
  final int tzOffsetMin;
  final int tempCX10;
  final int popPct;
  final int flags;
  final int validUntil;

  bool get timeValid => (flags & flagTimeValid) != 0;
  bool get weatherValid => (flags & flagWeatherValid) != 0;
}

class CompanionPreferences {
  const CompanionPreferences({
    this.showClockOnDevice = true,
    this.showWeatherOnDevice = true,
    this.weatherCityId = 'minsk',
    this.weatherUseFahrenheit = false,
  });

  final bool showClockOnDevice;
  final bool showWeatherOnDevice;
  final String weatherCityId;
  final bool weatherUseFahrenheit;

  CompanionPreferences copyWith({
    bool? showClockOnDevice,
    bool? showWeatherOnDevice,
    String? weatherCityId,
    bool? weatherUseFahrenheit,
  }) {
    return CompanionPreferences(
      showClockOnDevice: showClockOnDevice ?? this.showClockOnDevice,
      showWeatherOnDevice: showWeatherOnDevice ?? this.showWeatherOnDevice,
      weatherCityId: weatherCityId ?? this.weatherCityId,
      weatherUseFahrenheit: weatherUseFahrenheit ?? this.weatherUseFahrenheit,
    );
  }
}

class WeatherReading {
  const WeatherReading({
    required this.tempCX10,
    required this.popPct,
    required this.rainNow,
    required this.rainSoon,
    required this.stale,
  });

  final int tempCX10;
  final int popPct;
  final bool rainNow;
  final bool rainSoon;
  final bool stale;
}
