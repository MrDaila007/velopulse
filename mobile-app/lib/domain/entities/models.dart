import 'package:freezed_annotation/freezed_annotation.dart';

part 'models.freezed.dart';
part 'models.g.dart';

enum RideState {
  idle(0),
  moving(1),
  paused(2),
  unknown(255);

  const RideState(this.code);
  final int code;
  static RideState fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum SensorState {
  ok(0),
  idle(1),
  stuck(2),
  noSignal(3),
  unknown(255);

  const SensorState(this.code);
  final int code;
  static SensorState fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum PowerState {
  active(0),
  shortStop(1),
  idleDisplayOff(2),
  deepSleepPending(3),
  bleConfig(4),
  charging(5),
  unknown(255);

  const PowerState(this.code);
  final int code;
  static PowerState fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum ResetReason {
  unknown(0),
  powerOn(1),
  pinReset(2),
  watchdog(3),
  softReset(4),
  lockup(5),
  wakeFromSleep(6),
  brownout(7);

  const ResetReason(this.code);
  final int code;
  static ResetReason fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum CommandStatus {
  ok(0),
  unknownCommand(1),
  invalidLength(2),
  invalidStructVersion(3),
  range(4),
  needsConfirm(5),
  tokenInvalid(6),
  tokenExpired(7),
  notPaired(8),
  busy(9),
  storage(10),
  hardware(11),
  notSupported(12),
  unknown(255);

  const CommandStatus(this.code);
  final int code;
  static CommandStatus fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum DeviceCommandId {
  resetTrip(0x01),
  resetMaxSpeed(0x02),
  forceSave(0x03),
  displayOn(0x04),
  displayOff(0x05),
  displayTest(0x06),
  sensorTestStart(0x07),
  sensorTestStop(0x08),
  batteryTest(0x09),
  startDiagnostic(0x0A),
  getDiagnostic(0x0B),
  resetOdometer(0x20),
  factoryReset(0x21),
  reboot(0x22),
  setBatteryCalibration(0x23),
  setOdometer(0x30),
  openPairingWindow(0x40),
  configWrite(0xF0),
  unknown(0xFF);

  const DeviceCommandId(this.code);
  final int code;
  static DeviceCommandId fromCode(int code) =>
      values.firstWhere((value) => value.code == code, orElse: () => unknown);
}

enum DisplayTestPattern {
  fill(0),
  checkerboard(1),
  text(2);

  const DisplayTestPattern(this.code);
  final int code;
}

DeviceCommand buildDisplayTestCommand(DisplayTestPattern pattern) =>
    DeviceCommand(
      id: DeviceCommandId.displayTest,
      payload: <int>[pattern.code],
    );

DeviceCommand buildSafeCommand(DeviceCommandId id) {
  final payload = switch (id) {
    DeviceCommandId.displayTest => <int>[DisplayTestPattern.checkerboard.code],
    DeviceCommandId.sensorTestStart => const <int>[60, 0],
    _ => const <int>[],
  };
  return DeviceCommand(id: id, payload: payload);
}

enum BondState { bonded, none, unknown }

@freezed
abstract class DeviceInfo with _$DeviceInfo {
  const factory DeviceInfo({
    required int structVersion,
    required int protoMajor,
    required int protoMinor,
    required int hwRevision,
    required String model,
    required String fwVersion,
    required List<int> serial,
    required int uptimeS,
    required ResetReason resetReason,
    required int bootCount,
    required int flags,
  }) = _DeviceInfo;

  factory DeviceInfo.fromJson(Map<String, Object?> json) =>
      _$DeviceInfoFromJson(json);
}

extension DeviceInfoFlags on DeviceInfo {
  bool get bonded => flags & 0x08 != 0;
  bool get pairingWindowOpen => flags & 0x10 != 0;
}

@freezed
abstract class Telemetry with _$Telemetry {
  const Telemetry._();

  const factory Telemetry({
    required int structVersion,
    required int flags,
    required int speedX100,
    required int avgSpeedX100,
    required int maxSpeedX100,
    required int tripDistanceCm,
    required int movingTimeS,
    required int odometerM,
    required int batteryMv,
    required int batteryPct,
    required RideState rideState,
    required int revolutions,
    required int lastPulseAgeMs,
    required int seq,
    required SensorState sensorState,
    required PowerState powerState,
  }) = _Telemetry;

  factory Telemetry.fromJson(Map<String, Object?> json) =>
      _$TelemetryFromJson(json);

  bool get displayOn => flags & 0x02 != 0;
  bool get charging => flags & 0x04 != 0;
  bool get lowBattery => flags & 0x10 != 0;
}

@freezed
abstract class DeviceConfig with _$DeviceConfig {
  const DeviceConfig._();

  const factory DeviceConfig({
    @Default(1) int structVersion,
    @Default(15) int flags,
    @Default(2100) int wheelCircumferenceMm,
    @Default(100) int maxSpeedKmh,
    @Default(3) int stopTimeoutS,
    @Default(60) int displayTimeoutS,
    @Default(900) int deepSleepTimeoutS,
    @Default(60) int brightnessPct,
    @Default(4) int pageSwitchPeriodS,
    @Default(31) int enabledPagesMask,
    @Default(20) int lowBatteryPct,
    @Default(500) int odometerSaveIntervalM,
    @Default(3) int smoothingWindow,
    @Default(3) int debounceMs,
    @Default(0) int activeEdge,
    @Default(0) int pinnedPage,
    @Default(1000) int battCalScalePermille,
    @Default(0) int battCalOffsetMv,
    @Default(<int>[0, 1, 2, 3, 4]) List<int> pageOrder,
    @Default(0) int reservedPage,
    @Default('BikeComp-XXXX') String deviceName,
    @Default(0) int reserved,
  }) = _DeviceConfig;

  factory DeviceConfig.fromJson(Map<String, Object?> json) =>
      _$DeviceConfigFromJson(json);

  bool get smoothingEnabled => flags & 0x01 != 0;
  bool get autoPageSwitch => flags & 0x02 != 0;
  bool get displayAutoOff => flags & 0x04 != 0;
  bool get bleAlwaysAdvertise => flags & 0x08 != 0;
  bool get unitsImperial => flags & 0x10 != 0;

  DeviceConfig withFlag(int mask, bool enabled) =>
      copyWith(flags: enabled ? flags | mask : flags & ~mask);

  static const defaults = DeviceConfig();
}

@freezed
abstract class DeviceCommand with _$DeviceCommand {
  const factory DeviceCommand({
    @Default(1) int structVersion,
    required DeviceCommandId id,
    @Default(false) bool hasToken,
    @Default(<int>[]) List<int> payload,
  }) = _DeviceCommand;

  factory DeviceCommand.fromJson(Map<String, Object?> json) =>
      _$DeviceCommandFromJson(json);
}

@freezed
abstract class CommandResult with _$CommandResult {
  const factory CommandResult({
    required int structVersion,
    required DeviceCommandId commandId,
    required CommandStatus status,
    required int detail,
    required int token,
    required List<int> payload,
  }) = _CommandResult;

  factory CommandResult.fromJson(Map<String, Object?> json) =>
      _$CommandResultFromJson(json);
}

class DiagnosticSnapshot {
  const DiagnosticSnapshot({
    required this.rawPulseCount,
    required this.rejectedDebounce,
    required this.rejectedOverspeed,
    required this.isrOverflow,
    required this.flashWriteCount,
    required this.freeHeapBytes,
    required this.i2cErrorCount,
    required this.selftestMask,
  });

  final int rawPulseCount;
  final int rejectedDebounce;
  final int rejectedOverspeed;
  final int isrOverflow;
  final int flashWriteCount;
  final int freeHeapBytes;
  final int i2cErrorCount;
  final int selftestMask;

  Map<String, Object?> toJson() => <String, Object?>{
    'rawPulseCount': rawPulseCount,
    'rejectedDebounce': rejectedDebounce,
    'rejectedOverspeed': rejectedOverspeed,
    'isrOverflow': isrOverflow,
    'flashWriteCount': flashWriteCount,
    'freeHeapBytes': freeHeapBytes,
    'i2cErrorCount': i2cErrorCount,
    'selftestMask': selftestMask,
  };
}

class ErrorLogEntry {
  const ErrorLogEntry({
    required this.uptimeS,
    required this.code,
    required this.severity,
    required this.detail,
  });

  final int uptimeS;
  final int code;
  final int severity;
  final int detail;

  Map<String, Object?> toJson() => <String, Object?>{
    'uptimeS': uptimeS,
    'code': code,
    'severity': severity,
    'detail': detail,
  };
}

class ErrorLogBatch {
  const ErrorLogBatch({required this.entries});

  final List<ErrorLogEntry> entries;

  Map<String, Object?> toJson() => <String, Object?>{
    'entries': entries.map((entry) => entry.toJson()).toList(growable: false),
  };
}
