import '../../domain/entities/models.dart';

class RideLogSample {
  const RideLogSample({required this.at, required this.telemetry, this.rssi});

  final DateTime at;
  final Telemetry telemetry;
  final int? rssi;

  Map<String, Object?> toJson() => <String, Object?>{
    't': at.toUtc().toIso8601String(),
    'speedX100': telemetry.speedX100,
    'revolutions': telemetry.revolutions,
    'lastPulseAgeMs': telemetry.lastPulseAgeMs,
    'sensorState': telemetry.sensorState.code,
    if (rssi != null) 'rssi': rssi,
  };
}

class RideLogRecorder {
  RideLogRecorder({this.maxSamples = 3600});

  final int maxSamples;
  final List<RideLogSample> _samples = <RideLogSample>[];

  List<RideLogSample> get samples => List<RideLogSample>.unmodifiable(_samples);

  void record(Telemetry telemetry, {int? rssi}) {
    _samples.add(
      RideLogSample(
        at: DateTime.now().toUtc(),
        telemetry: telemetry,
        rssi: rssi,
      ),
    );
    if (_samples.length > maxSamples) {
      _samples.removeRange(0, _samples.length - maxSamples);
    }
  }

  void clear() => _samples.clear();
}
