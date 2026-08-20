// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'models.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_DeviceInfo _$DeviceInfoFromJson(Map<String, dynamic> json) => _DeviceInfo(
  structVersion: (json['structVersion'] as num).toInt(),
  protoMajor: (json['protoMajor'] as num).toInt(),
  protoMinor: (json['protoMinor'] as num).toInt(),
  hwRevision: (json['hwRevision'] as num).toInt(),
  model: json['model'] as String,
  fwVersion: json['fwVersion'] as String,
  serial: (json['serial'] as List<dynamic>)
      .map((e) => (e as num).toInt())
      .toList(),
  uptimeS: (json['uptimeS'] as num).toInt(),
  resetReason: $enumDecode(_$ResetReasonEnumMap, json['resetReason']),
  bootCount: (json['bootCount'] as num).toInt(),
  flags: (json['flags'] as num).toInt(),
);

Map<String, dynamic> _$DeviceInfoToJson(_DeviceInfo instance) =>
    <String, dynamic>{
      'structVersion': instance.structVersion,
      'protoMajor': instance.protoMajor,
      'protoMinor': instance.protoMinor,
      'hwRevision': instance.hwRevision,
      'model': instance.model,
      'fwVersion': instance.fwVersion,
      'serial': instance.serial,
      'uptimeS': instance.uptimeS,
      'resetReason': _$ResetReasonEnumMap[instance.resetReason]!,
      'bootCount': instance.bootCount,
      'flags': instance.flags,
    };

const _$ResetReasonEnumMap = {
  ResetReason.unknown: 'unknown',
  ResetReason.powerOn: 'powerOn',
  ResetReason.pinReset: 'pinReset',
  ResetReason.watchdog: 'watchdog',
  ResetReason.softReset: 'softReset',
  ResetReason.lockup: 'lockup',
  ResetReason.wakeFromSleep: 'wakeFromSleep',
  ResetReason.brownout: 'brownout',
};

_Telemetry _$TelemetryFromJson(Map<String, dynamic> json) => _Telemetry(
  structVersion: (json['structVersion'] as num).toInt(),
  flags: (json['flags'] as num).toInt(),
  speedX100: (json['speedX100'] as num).toInt(),
  avgSpeedX100: (json['avgSpeedX100'] as num).toInt(),
  maxSpeedX100: (json['maxSpeedX100'] as num).toInt(),
  tripDistanceCm: (json['tripDistanceCm'] as num).toInt(),
  movingTimeS: (json['movingTimeS'] as num).toInt(),
  odometerM: (json['odometerM'] as num).toInt(),
  batteryMv: (json['batteryMv'] as num).toInt(),
  batteryPct: (json['batteryPct'] as num).toInt(),
  rideState: $enumDecode(_$RideStateEnumMap, json['rideState']),
  revolutions: (json['revolutions'] as num).toInt(),
  lastPulseAgeMs: (json['lastPulseAgeMs'] as num).toInt(),
  seq: (json['seq'] as num).toInt(),
  sensorState: $enumDecode(_$SensorStateEnumMap, json['sensorState']),
  powerState: $enumDecode(_$PowerStateEnumMap, json['powerState']),
  cadenceX10: (json['cadenceX10'] as num?)?.toInt() ?? 0,
  cscFlags: (json['cscFlags'] as num?)?.toInt() ?? 0,
  lastCrankEventAgeMs: (json['lastCrankEventAgeMs'] as num?)?.toInt() ?? 4294967295,
);

Map<String, dynamic> _$TelemetryToJson(_Telemetry instance) =>
    <String, dynamic>{
      'structVersion': instance.structVersion,
      'flags': instance.flags,
      'speedX100': instance.speedX100,
      'avgSpeedX100': instance.avgSpeedX100,
      'maxSpeedX100': instance.maxSpeedX100,
      'tripDistanceCm': instance.tripDistanceCm,
      'movingTimeS': instance.movingTimeS,
      'odometerM': instance.odometerM,
      'batteryMv': instance.batteryMv,
      'batteryPct': instance.batteryPct,
      'rideState': _$RideStateEnumMap[instance.rideState]!,
      'revolutions': instance.revolutions,
      'lastPulseAgeMs': instance.lastPulseAgeMs,
      'seq': instance.seq,
      'sensorState': _$SensorStateEnumMap[instance.sensorState]!,
      'powerState': _$PowerStateEnumMap[instance.powerState]!,
      'cadenceX10': instance.cadenceX10,
      'cscFlags': instance.cscFlags,
      'lastCrankEventAgeMs': instance.lastCrankEventAgeMs,
    };

const _$RideStateEnumMap = {
  RideState.idle: 'idle',
  RideState.moving: 'moving',
  RideState.paused: 'paused',
  RideState.unknown: 'unknown',
};

const _$SensorStateEnumMap = {
  SensorState.ok: 'ok',
  SensorState.idle: 'idle',
  SensorState.stuck: 'stuck',
  SensorState.noSignal: 'noSignal',
  SensorState.unknown: 'unknown',
};

const _$PowerStateEnumMap = {
  PowerState.active: 'active',
  PowerState.shortStop: 'shortStop',
  PowerState.idleDisplayOff: 'idleDisplayOff',
  PowerState.deepSleepPending: 'deepSleepPending',
  PowerState.bleConfig: 'bleConfig',
  PowerState.charging: 'charging',
  PowerState.unknown: 'unknown',
};

_DeviceConfig _$DeviceConfigFromJson(
  Map<String, dynamic> json,
) => _DeviceConfig(
  structVersion: (json['structVersion'] as num?)?.toInt() ?? 1,
  flags: (json['flags'] as num?)?.toInt() ?? 15,
  wheelCircumferenceMm: (json['wheelCircumferenceMm'] as num?)?.toInt() ?? 2100,
  maxSpeedKmh: (json['maxSpeedKmh'] as num?)?.toInt() ?? 100,
  stopTimeoutS: (json['stopTimeoutS'] as num?)?.toInt() ?? 3,
  displayTimeoutS: (json['displayTimeoutS'] as num?)?.toInt() ?? 60,
  deepSleepTimeoutS: (json['deepSleepTimeoutS'] as num?)?.toInt() ?? 900,
  brightnessPct: (json['brightnessPct'] as num?)?.toInt() ?? 60,
  pageSwitchPeriodS: (json['pageSwitchPeriodS'] as num?)?.toInt() ?? 4,
  enabledPagesMask: (json['enabledPagesMask'] as num?)?.toInt() ?? 31,
  lowBatteryPct: (json['lowBatteryPct'] as num?)?.toInt() ?? 20,
  odometerSaveIntervalM:
      (json['odometerSaveIntervalM'] as num?)?.toInt() ?? 500,
  smoothingWindow: (json['smoothingWindow'] as num?)?.toInt() ?? 3,
  debounceMs: (json['debounceMs'] as num?)?.toInt() ?? 3,
  activeEdge: (json['activeEdge'] as num?)?.toInt() ?? 0,
  pinnedPage: (json['pinnedPage'] as num?)?.toInt() ?? 0,
  battCalScalePermille: (json['battCalScalePermille'] as num?)?.toInt() ?? 1000,
  battCalOffsetMv: (json['battCalOffsetMv'] as num?)?.toInt() ?? 0,
  pageOrder:
      (json['pageOrder'] as List<dynamic>?)
          ?.map((e) => (e as num).toInt())
          .toList() ??
      const <int>[0, 1, 2, 3, 4],
  reservedPage: (json['reservedPage'] as num?)?.toInt() ?? 0,
  deviceName: json['deviceName'] as String? ?? 'BikeComp-XXXX',
  reserved: (json['reserved'] as num?)?.toInt() ?? 0,
);

Map<String, dynamic> _$DeviceConfigToJson(_DeviceConfig instance) =>
    <String, dynamic>{
      'structVersion': instance.structVersion,
      'flags': instance.flags,
      'wheelCircumferenceMm': instance.wheelCircumferenceMm,
      'maxSpeedKmh': instance.maxSpeedKmh,
      'stopTimeoutS': instance.stopTimeoutS,
      'displayTimeoutS': instance.displayTimeoutS,
      'deepSleepTimeoutS': instance.deepSleepTimeoutS,
      'brightnessPct': instance.brightnessPct,
      'pageSwitchPeriodS': instance.pageSwitchPeriodS,
      'enabledPagesMask': instance.enabledPagesMask,
      'lowBatteryPct': instance.lowBatteryPct,
      'odometerSaveIntervalM': instance.odometerSaveIntervalM,
      'smoothingWindow': instance.smoothingWindow,
      'debounceMs': instance.debounceMs,
      'activeEdge': instance.activeEdge,
      'pinnedPage': instance.pinnedPage,
      'battCalScalePermille': instance.battCalScalePermille,
      'battCalOffsetMv': instance.battCalOffsetMv,
      'pageOrder': instance.pageOrder,
      'reservedPage': instance.reservedPage,
      'deviceName': instance.deviceName,
      'reserved': instance.reserved,
    };

_DeviceCommand _$DeviceCommandFromJson(Map<String, dynamic> json) =>
    _DeviceCommand(
      structVersion: (json['structVersion'] as num?)?.toInt() ?? 1,
      id: $enumDecode(_$DeviceCommandIdEnumMap, json['id']),
      hasToken: json['hasToken'] as bool? ?? false,
      payload:
          (json['payload'] as List<dynamic>?)
              ?.map((e) => (e as num).toInt())
              .toList() ??
          const <int>[],
    );

Map<String, dynamic> _$DeviceCommandToJson(_DeviceCommand instance) =>
    <String, dynamic>{
      'structVersion': instance.structVersion,
      'id': _$DeviceCommandIdEnumMap[instance.id]!,
      'hasToken': instance.hasToken,
      'payload': instance.payload,
    };

const _$DeviceCommandIdEnumMap = {
  DeviceCommandId.resetTrip: 'resetTrip',
  DeviceCommandId.resetMaxSpeed: 'resetMaxSpeed',
  DeviceCommandId.forceSave: 'forceSave',
  DeviceCommandId.displayOn: 'displayOn',
  DeviceCommandId.displayOff: 'displayOff',
  DeviceCommandId.displayTest: 'displayTest',
  DeviceCommandId.sensorTestStart: 'sensorTestStart',
  DeviceCommandId.sensorTestStop: 'sensorTestStop',
  DeviceCommandId.batteryTest: 'batteryTest',
  DeviceCommandId.startDiagnostic: 'startDiagnostic',
  DeviceCommandId.getDiagnostic: 'getDiagnostic',
  DeviceCommandId.resetOdometer: 'resetOdometer',
  DeviceCommandId.factoryReset: 'factoryReset',
  DeviceCommandId.reboot: 'reboot',
  DeviceCommandId.setBatteryCalibration: 'setBatteryCalibration',
  DeviceCommandId.setOdometer: 'setOdometer',
  DeviceCommandId.openPairingWindow: 'openPairingWindow',
  DeviceCommandId.configWrite: 'configWrite',
  DeviceCommandId.unknown: 'unknown',
};

_CommandResult _$CommandResultFromJson(Map<String, dynamic> json) =>
    _CommandResult(
      structVersion: (json['structVersion'] as num).toInt(),
      commandId: $enumDecode(_$DeviceCommandIdEnumMap, json['commandId']),
      status: $enumDecode(_$CommandStatusEnumMap, json['status']),
      detail: (json['detail'] as num).toInt(),
      token: (json['token'] as num).toInt(),
      payload: (json['payload'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$CommandResultToJson(_CommandResult instance) =>
    <String, dynamic>{
      'structVersion': instance.structVersion,
      'commandId': _$DeviceCommandIdEnumMap[instance.commandId]!,
      'status': _$CommandStatusEnumMap[instance.status]!,
      'detail': instance.detail,
      'token': instance.token,
      'payload': instance.payload,
    };

const _$CommandStatusEnumMap = {
  CommandStatus.ok: 'ok',
  CommandStatus.unknownCommand: 'unknownCommand',
  CommandStatus.invalidLength: 'invalidLength',
  CommandStatus.invalidStructVersion: 'invalidStructVersion',
  CommandStatus.range: 'range',
  CommandStatus.needsConfirm: 'needsConfirm',
  CommandStatus.tokenInvalid: 'tokenInvalid',
  CommandStatus.tokenExpired: 'tokenExpired',
  CommandStatus.notPaired: 'notPaired',
  CommandStatus.busy: 'busy',
  CommandStatus.storage: 'storage',
  CommandStatus.hardware: 'hardware',
  CommandStatus.notSupported: 'notSupported',
  CommandStatus.unknown: 'unknown',
};
