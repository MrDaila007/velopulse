// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'models.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;

/// @nodoc
mixin _$DeviceInfo {

 int get structVersion; int get protoMajor; int get protoMinor; int get hwRevision; String get model; String get fwVersion; List<int> get serial; int get uptimeS; ResetReason get resetReason; int get bootCount; int get flags;
/// Create a copy of DeviceInfo
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$DeviceInfoCopyWith<DeviceInfo> get copyWith => _$DeviceInfoCopyWithImpl<DeviceInfo>(this as DeviceInfo, _$identity);

  /// Serializes this DeviceInfo to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is DeviceInfo&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.protoMajor, protoMajor) || other.protoMajor == protoMajor)&&(identical(other.protoMinor, protoMinor) || other.protoMinor == protoMinor)&&(identical(other.hwRevision, hwRevision) || other.hwRevision == hwRevision)&&(identical(other.model, model) || other.model == model)&&(identical(other.fwVersion, fwVersion) || other.fwVersion == fwVersion)&&const DeepCollectionEquality().equals(other.serial, serial)&&(identical(other.uptimeS, uptimeS) || other.uptimeS == uptimeS)&&(identical(other.resetReason, resetReason) || other.resetReason == resetReason)&&(identical(other.bootCount, bootCount) || other.bootCount == bootCount)&&(identical(other.flags, flags) || other.flags == flags));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,protoMajor,protoMinor,hwRevision,model,fwVersion,const DeepCollectionEquality().hash(serial),uptimeS,resetReason,bootCount,flags);

@override
String toString() {
  return 'DeviceInfo(structVersion: $structVersion, protoMajor: $protoMajor, protoMinor: $protoMinor, hwRevision: $hwRevision, model: $model, fwVersion: $fwVersion, serial: $serial, uptimeS: $uptimeS, resetReason: $resetReason, bootCount: $bootCount, flags: $flags)';
}


}

/// @nodoc
abstract mixin class $DeviceInfoCopyWith<$Res>  {
  factory $DeviceInfoCopyWith(DeviceInfo value, $Res Function(DeviceInfo) _then) = _$DeviceInfoCopyWithImpl;
@useResult
$Res call({
 int structVersion, int protoMajor, int protoMinor, int hwRevision, String model, String fwVersion, List<int> serial, int uptimeS, ResetReason resetReason, int bootCount, int flags
});




}
/// @nodoc
class _$DeviceInfoCopyWithImpl<$Res>
    implements $DeviceInfoCopyWith<$Res> {
  _$DeviceInfoCopyWithImpl(this._self, this._then);

  final DeviceInfo _self;
  final $Res Function(DeviceInfo) _then;

/// Create a copy of DeviceInfo
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? structVersion = null,Object? protoMajor = null,Object? protoMinor = null,Object? hwRevision = null,Object? model = null,Object? fwVersion = null,Object? serial = null,Object? uptimeS = null,Object? resetReason = null,Object? bootCount = null,Object? flags = null,}) {
  return _then(_self.copyWith(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,protoMajor: null == protoMajor ? _self.protoMajor : protoMajor // ignore: cast_nullable_to_non_nullable
as int,protoMinor: null == protoMinor ? _self.protoMinor : protoMinor // ignore: cast_nullable_to_non_nullable
as int,hwRevision: null == hwRevision ? _self.hwRevision : hwRevision // ignore: cast_nullable_to_non_nullable
as int,model: null == model ? _self.model : model // ignore: cast_nullable_to_non_nullable
as String,fwVersion: null == fwVersion ? _self.fwVersion : fwVersion // ignore: cast_nullable_to_non_nullable
as String,serial: null == serial ? _self.serial : serial // ignore: cast_nullable_to_non_nullable
as List<int>,uptimeS: null == uptimeS ? _self.uptimeS : uptimeS // ignore: cast_nullable_to_non_nullable
as int,resetReason: null == resetReason ? _self.resetReason : resetReason // ignore: cast_nullable_to_non_nullable
as ResetReason,bootCount: null == bootCount ? _self.bootCount : bootCount // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,
  ));
}

}


/// Adds pattern-matching-related methods to [DeviceInfo].
extension DeviceInfoPatterns on DeviceInfo {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _DeviceInfo value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _DeviceInfo() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _DeviceInfo value)  $default,){
final _that = this;
switch (_that) {
case _DeviceInfo():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _DeviceInfo value)?  $default,){
final _that = this;
switch (_that) {
case _DeviceInfo() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int structVersion,  int protoMajor,  int protoMinor,  int hwRevision,  String model,  String fwVersion,  List<int> serial,  int uptimeS,  ResetReason resetReason,  int bootCount,  int flags)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _DeviceInfo() when $default != null:
return $default(_that.structVersion,_that.protoMajor,_that.protoMinor,_that.hwRevision,_that.model,_that.fwVersion,_that.serial,_that.uptimeS,_that.resetReason,_that.bootCount,_that.flags);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int structVersion,  int protoMajor,  int protoMinor,  int hwRevision,  String model,  String fwVersion,  List<int> serial,  int uptimeS,  ResetReason resetReason,  int bootCount,  int flags)  $default,) {final _that = this;
switch (_that) {
case _DeviceInfo():
return $default(_that.structVersion,_that.protoMajor,_that.protoMinor,_that.hwRevision,_that.model,_that.fwVersion,_that.serial,_that.uptimeS,_that.resetReason,_that.bootCount,_that.flags);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int structVersion,  int protoMajor,  int protoMinor,  int hwRevision,  String model,  String fwVersion,  List<int> serial,  int uptimeS,  ResetReason resetReason,  int bootCount,  int flags)?  $default,) {final _that = this;
switch (_that) {
case _DeviceInfo() when $default != null:
return $default(_that.structVersion,_that.protoMajor,_that.protoMinor,_that.hwRevision,_that.model,_that.fwVersion,_that.serial,_that.uptimeS,_that.resetReason,_that.bootCount,_that.flags);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _DeviceInfo implements DeviceInfo {
  const _DeviceInfo({required this.structVersion, required this.protoMajor, required this.protoMinor, required this.hwRevision, required this.model, required this.fwVersion, required final  List<int> serial, required this.uptimeS, required this.resetReason, required this.bootCount, required this.flags}): _serial = serial;
  factory _DeviceInfo.fromJson(Map<String, dynamic> json) => _$DeviceInfoFromJson(json);

@override final  int structVersion;
@override final  int protoMajor;
@override final  int protoMinor;
@override final  int hwRevision;
@override final  String model;
@override final  String fwVersion;
 final  List<int> _serial;
@override List<int> get serial {
  if (_serial is EqualUnmodifiableListView) return _serial;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_serial);
}

@override final  int uptimeS;
@override final  ResetReason resetReason;
@override final  int bootCount;
@override final  int flags;

/// Create a copy of DeviceInfo
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$DeviceInfoCopyWith<_DeviceInfo> get copyWith => __$DeviceInfoCopyWithImpl<_DeviceInfo>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$DeviceInfoToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _DeviceInfo&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.protoMajor, protoMajor) || other.protoMajor == protoMajor)&&(identical(other.protoMinor, protoMinor) || other.protoMinor == protoMinor)&&(identical(other.hwRevision, hwRevision) || other.hwRevision == hwRevision)&&(identical(other.model, model) || other.model == model)&&(identical(other.fwVersion, fwVersion) || other.fwVersion == fwVersion)&&const DeepCollectionEquality().equals(other._serial, _serial)&&(identical(other.uptimeS, uptimeS) || other.uptimeS == uptimeS)&&(identical(other.resetReason, resetReason) || other.resetReason == resetReason)&&(identical(other.bootCount, bootCount) || other.bootCount == bootCount)&&(identical(other.flags, flags) || other.flags == flags));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,protoMajor,protoMinor,hwRevision,model,fwVersion,const DeepCollectionEquality().hash(_serial),uptimeS,resetReason,bootCount,flags);

@override
String toString() {
  return 'DeviceInfo(structVersion: $structVersion, protoMajor: $protoMajor, protoMinor: $protoMinor, hwRevision: $hwRevision, model: $model, fwVersion: $fwVersion, serial: $serial, uptimeS: $uptimeS, resetReason: $resetReason, bootCount: $bootCount, flags: $flags)';
}


}

/// @nodoc
abstract mixin class _$DeviceInfoCopyWith<$Res> implements $DeviceInfoCopyWith<$Res> {
  factory _$DeviceInfoCopyWith(_DeviceInfo value, $Res Function(_DeviceInfo) _then) = __$DeviceInfoCopyWithImpl;
@override @useResult
$Res call({
 int structVersion, int protoMajor, int protoMinor, int hwRevision, String model, String fwVersion, List<int> serial, int uptimeS, ResetReason resetReason, int bootCount, int flags
});




}
/// @nodoc
class __$DeviceInfoCopyWithImpl<$Res>
    implements _$DeviceInfoCopyWith<$Res> {
  __$DeviceInfoCopyWithImpl(this._self, this._then);

  final _DeviceInfo _self;
  final $Res Function(_DeviceInfo) _then;

/// Create a copy of DeviceInfo
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? structVersion = null,Object? protoMajor = null,Object? protoMinor = null,Object? hwRevision = null,Object? model = null,Object? fwVersion = null,Object? serial = null,Object? uptimeS = null,Object? resetReason = null,Object? bootCount = null,Object? flags = null,}) {
  return _then(_DeviceInfo(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,protoMajor: null == protoMajor ? _self.protoMajor : protoMajor // ignore: cast_nullable_to_non_nullable
as int,protoMinor: null == protoMinor ? _self.protoMinor : protoMinor // ignore: cast_nullable_to_non_nullable
as int,hwRevision: null == hwRevision ? _self.hwRevision : hwRevision // ignore: cast_nullable_to_non_nullable
as int,model: null == model ? _self.model : model // ignore: cast_nullable_to_non_nullable
as String,fwVersion: null == fwVersion ? _self.fwVersion : fwVersion // ignore: cast_nullable_to_non_nullable
as String,serial: null == serial ? _self._serial : serial // ignore: cast_nullable_to_non_nullable
as List<int>,uptimeS: null == uptimeS ? _self.uptimeS : uptimeS // ignore: cast_nullable_to_non_nullable
as int,resetReason: null == resetReason ? _self.resetReason : resetReason // ignore: cast_nullable_to_non_nullable
as ResetReason,bootCount: null == bootCount ? _self.bootCount : bootCount // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}


/// @nodoc
mixin _$Telemetry {

 int get structVersion; int get flags; int get speedX100; int get avgSpeedX100; int get maxSpeedX100; int get tripDistanceCm; int get movingTimeS; int get odometerM; int get batteryMv; int get batteryPct; RideState get rideState; int get revolutions; int get lastPulseAgeMs; int get seq; SensorState get sensorState; PowerState get powerState; int get cadenceX10; int get cscFlags; int get lastCrankEventAgeMs;
/// Create a copy of Telemetry
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$TelemetryCopyWith<Telemetry> get copyWith => _$TelemetryCopyWithImpl<Telemetry>(this as Telemetry, _$identity);

  /// Serializes this Telemetry to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is Telemetry&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.flags, flags) || other.flags == flags)&&(identical(other.speedX100, speedX100) || other.speedX100 == speedX100)&&(identical(other.avgSpeedX100, avgSpeedX100) || other.avgSpeedX100 == avgSpeedX100)&&(identical(other.maxSpeedX100, maxSpeedX100) || other.maxSpeedX100 == maxSpeedX100)&&(identical(other.tripDistanceCm, tripDistanceCm) || other.tripDistanceCm == tripDistanceCm)&&(identical(other.movingTimeS, movingTimeS) || other.movingTimeS == movingTimeS)&&(identical(other.odometerM, odometerM) || other.odometerM == odometerM)&&(identical(other.batteryMv, batteryMv) || other.batteryMv == batteryMv)&&(identical(other.batteryPct, batteryPct) || other.batteryPct == batteryPct)&&(identical(other.rideState, rideState) || other.rideState == rideState)&&(identical(other.revolutions, revolutions) || other.revolutions == revolutions)&&(identical(other.lastPulseAgeMs, lastPulseAgeMs) || other.lastPulseAgeMs == lastPulseAgeMs)&&(identical(other.seq, seq) || other.seq == seq)&&(identical(other.sensorState, sensorState) || other.sensorState == sensorState)&&(identical(other.powerState, powerState) || other.powerState == powerState)&&(identical(other.cadenceX10, cadenceX10) || other.cadenceX10 == cadenceX10)&&(identical(other.cscFlags, cscFlags) || other.cscFlags == cscFlags)&&(identical(other.lastCrankEventAgeMs, lastCrankEventAgeMs) || other.lastCrankEventAgeMs == lastCrankEventAgeMs));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,flags,speedX100,avgSpeedX100,maxSpeedX100,tripDistanceCm,movingTimeS,odometerM,batteryMv,batteryPct,rideState,revolutions,lastPulseAgeMs,seq,sensorState,powerState,cadenceX10,cscFlags,lastCrankEventAgeMs);

@override
String toString() {
  return 'Telemetry(structVersion: $structVersion, flags: $flags, speedX100: $speedX100, avgSpeedX100: $avgSpeedX100, maxSpeedX100: $maxSpeedX100, tripDistanceCm: $tripDistanceCm, movingTimeS: $movingTimeS, odometerM: $odometerM, batteryMv: $batteryMv, batteryPct: $batteryPct, rideState: $rideState, revolutions: $revolutions, lastPulseAgeMs: $lastPulseAgeMs, seq: $seq, sensorState: $sensorState, powerState: $powerState, cadenceX10: $cadenceX10, cscFlags: $cscFlags, lastCrankEventAgeMs: $lastCrankEventAgeMs)';
}


}

/// @nodoc
abstract mixin class $TelemetryCopyWith<$Res>  {
  factory $TelemetryCopyWith(Telemetry value, $Res Function(Telemetry) _then) = _$TelemetryCopyWithImpl;
@useResult
$Res call({
 int structVersion, int flags, int speedX100, int avgSpeedX100, int maxSpeedX100, int tripDistanceCm, int movingTimeS, int odometerM, int batteryMv, int batteryPct, RideState rideState, int revolutions, int lastPulseAgeMs, int seq, SensorState sensorState, PowerState powerState, int cadenceX10, int cscFlags, int lastCrankEventAgeMs
});




}
/// @nodoc
class _$TelemetryCopyWithImpl<$Res>
    implements $TelemetryCopyWith<$Res> {
  _$TelemetryCopyWithImpl(this._self, this._then);

  final Telemetry _self;
  final $Res Function(Telemetry) _then;

/// Create a copy of Telemetry
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? structVersion = null,Object? flags = null,Object? speedX100 = null,Object? avgSpeedX100 = null,Object? maxSpeedX100 = null,Object? tripDistanceCm = null,Object? movingTimeS = null,Object? odometerM = null,Object? batteryMv = null,Object? batteryPct = null,Object? rideState = null,Object? revolutions = null,Object? lastPulseAgeMs = null,Object? seq = null,Object? sensorState = null,Object? powerState = null,Object? cadenceX10 = null,Object? cscFlags = null,Object? lastCrankEventAgeMs = null,}) {
  return _then(_self.copyWith(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,speedX100: null == speedX100 ? _self.speedX100 : speedX100 // ignore: cast_nullable_to_non_nullable
as int,avgSpeedX100: null == avgSpeedX100 ? _self.avgSpeedX100 : avgSpeedX100 // ignore: cast_nullable_to_non_nullable
as int,maxSpeedX100: null == maxSpeedX100 ? _self.maxSpeedX100 : maxSpeedX100 // ignore: cast_nullable_to_non_nullable
as int,tripDistanceCm: null == tripDistanceCm ? _self.tripDistanceCm : tripDistanceCm // ignore: cast_nullable_to_non_nullable
as int,movingTimeS: null == movingTimeS ? _self.movingTimeS : movingTimeS // ignore: cast_nullable_to_non_nullable
as int,odometerM: null == odometerM ? _self.odometerM : odometerM // ignore: cast_nullable_to_non_nullable
as int,batteryMv: null == batteryMv ? _self.batteryMv : batteryMv // ignore: cast_nullable_to_non_nullable
as int,batteryPct: null == batteryPct ? _self.batteryPct : batteryPct // ignore: cast_nullable_to_non_nullable
as int,rideState: null == rideState ? _self.rideState : rideState // ignore: cast_nullable_to_non_nullable
as RideState,revolutions: null == revolutions ? _self.revolutions : revolutions // ignore: cast_nullable_to_non_nullable
as int,lastPulseAgeMs: null == lastPulseAgeMs ? _self.lastPulseAgeMs : lastPulseAgeMs // ignore: cast_nullable_to_non_nullable
as int,seq: null == seq ? _self.seq : seq // ignore: cast_nullable_to_non_nullable
as int,sensorState: null == sensorState ? _self.sensorState : sensorState // ignore: cast_nullable_to_non_nullable
as SensorState,powerState: null == powerState ? _self.powerState : powerState // ignore: cast_nullable_to_non_nullable
as PowerState,cadenceX10: null == cadenceX10 ? _self.cadenceX10 : cadenceX10 // ignore: cast_nullable_to_non_nullable
as int,cscFlags: null == cscFlags ? _self.cscFlags : cscFlags // ignore: cast_nullable_to_non_nullable
as int,lastCrankEventAgeMs: null == lastCrankEventAgeMs ? _self.lastCrankEventAgeMs : lastCrankEventAgeMs // ignore: cast_nullable_to_non_nullable
as int,
  ));
}

}


/// Adds pattern-matching-related methods to [Telemetry].
extension TelemetryPatterns on Telemetry {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _Telemetry value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _Telemetry() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _Telemetry value)  $default,){
final _that = this;
switch (_that) {
case _Telemetry():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _Telemetry value)?  $default,){
final _that = this;
switch (_that) {
case _Telemetry() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int structVersion,  int flags,  int speedX100,  int avgSpeedX100,  int maxSpeedX100,  int tripDistanceCm,  int movingTimeS,  int odometerM,  int batteryMv,  int batteryPct,  RideState rideState,  int revolutions,  int lastPulseAgeMs,  int seq,  SensorState sensorState,  PowerState powerState,  int cadenceX10,  int cscFlags,  int lastCrankEventAgeMs)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _Telemetry() when $default != null:
return $default(_that.structVersion,_that.flags,_that.speedX100,_that.avgSpeedX100,_that.maxSpeedX100,_that.tripDistanceCm,_that.movingTimeS,_that.odometerM,_that.batteryMv,_that.batteryPct,_that.rideState,_that.revolutions,_that.lastPulseAgeMs,_that.seq,_that.sensorState,_that.powerState,_that.cadenceX10,_that.cscFlags,_that.lastCrankEventAgeMs);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int structVersion,  int flags,  int speedX100,  int avgSpeedX100,  int maxSpeedX100,  int tripDistanceCm,  int movingTimeS,  int odometerM,  int batteryMv,  int batteryPct,  RideState rideState,  int revolutions,  int lastPulseAgeMs,  int seq,  SensorState sensorState,  PowerState powerState,  int cadenceX10,  int cscFlags,  int lastCrankEventAgeMs)  $default,) {final _that = this;
switch (_that) {
case _Telemetry():
return $default(_that.structVersion,_that.flags,_that.speedX100,_that.avgSpeedX100,_that.maxSpeedX100,_that.tripDistanceCm,_that.movingTimeS,_that.odometerM,_that.batteryMv,_that.batteryPct,_that.rideState,_that.revolutions,_that.lastPulseAgeMs,_that.seq,_that.sensorState,_that.powerState,_that.cadenceX10,_that.cscFlags,_that.lastCrankEventAgeMs);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int structVersion,  int flags,  int speedX100,  int avgSpeedX100,  int maxSpeedX100,  int tripDistanceCm,  int movingTimeS,  int odometerM,  int batteryMv,  int batteryPct,  RideState rideState,  int revolutions,  int lastPulseAgeMs,  int seq,  SensorState sensorState,  PowerState powerState,  int cadenceX10,  int cscFlags,  int lastCrankEventAgeMs)?  $default,) {final _that = this;
switch (_that) {
case _Telemetry() when $default != null:
return $default(_that.structVersion,_that.flags,_that.speedX100,_that.avgSpeedX100,_that.maxSpeedX100,_that.tripDistanceCm,_that.movingTimeS,_that.odometerM,_that.batteryMv,_that.batteryPct,_that.rideState,_that.revolutions,_that.lastPulseAgeMs,_that.seq,_that.sensorState,_that.powerState,_that.cadenceX10,_that.cscFlags,_that.lastCrankEventAgeMs);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _Telemetry extends Telemetry {
  const _Telemetry({required this.structVersion, required this.flags, required this.speedX100, required this.avgSpeedX100, required this.maxSpeedX100, required this.tripDistanceCm, required this.movingTimeS, required this.odometerM, required this.batteryMv, required this.batteryPct, required this.rideState, required this.revolutions, required this.lastPulseAgeMs, required this.seq, required this.sensorState, required this.powerState, this.cadenceX10 = 0, this.cscFlags = 0, this.lastCrankEventAgeMs = 4294967295}): super._();
  factory _Telemetry.fromJson(Map<String, dynamic> json) => _$TelemetryFromJson(json);

@override final  int structVersion;
@override final  int flags;
@override final  int speedX100;
@override final  int avgSpeedX100;
@override final  int maxSpeedX100;
@override final  int tripDistanceCm;
@override final  int movingTimeS;
@override final  int odometerM;
@override final  int batteryMv;
@override final  int batteryPct;
@override final  RideState rideState;
@override final  int revolutions;
@override final  int lastPulseAgeMs;
@override final  int seq;
@override final  SensorState sensorState;
@override final  PowerState powerState;
@override@JsonKey() final  int cadenceX10;
@override@JsonKey() final  int cscFlags;
@override@JsonKey() final  int lastCrankEventAgeMs;

/// Create a copy of Telemetry
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$TelemetryCopyWith<_Telemetry> get copyWith => __$TelemetryCopyWithImpl<_Telemetry>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$TelemetryToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _Telemetry&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.flags, flags) || other.flags == flags)&&(identical(other.speedX100, speedX100) || other.speedX100 == speedX100)&&(identical(other.avgSpeedX100, avgSpeedX100) || other.avgSpeedX100 == avgSpeedX100)&&(identical(other.maxSpeedX100, maxSpeedX100) || other.maxSpeedX100 == maxSpeedX100)&&(identical(other.tripDistanceCm, tripDistanceCm) || other.tripDistanceCm == tripDistanceCm)&&(identical(other.movingTimeS, movingTimeS) || other.movingTimeS == movingTimeS)&&(identical(other.odometerM, odometerM) || other.odometerM == odometerM)&&(identical(other.batteryMv, batteryMv) || other.batteryMv == batteryMv)&&(identical(other.batteryPct, batteryPct) || other.batteryPct == batteryPct)&&(identical(other.rideState, rideState) || other.rideState == rideState)&&(identical(other.revolutions, revolutions) || other.revolutions == revolutions)&&(identical(other.lastPulseAgeMs, lastPulseAgeMs) || other.lastPulseAgeMs == lastPulseAgeMs)&&(identical(other.seq, seq) || other.seq == seq)&&(identical(other.sensorState, sensorState) || other.sensorState == sensorState)&&(identical(other.powerState, powerState) || other.powerState == powerState)&&(identical(other.cadenceX10, cadenceX10) || other.cadenceX10 == cadenceX10)&&(identical(other.cscFlags, cscFlags) || other.cscFlags == cscFlags)&&(identical(other.lastCrankEventAgeMs, lastCrankEventAgeMs) || other.lastCrankEventAgeMs == lastCrankEventAgeMs));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,flags,speedX100,avgSpeedX100,maxSpeedX100,tripDistanceCm,movingTimeS,odometerM,batteryMv,batteryPct,rideState,revolutions,lastPulseAgeMs,seq,sensorState,powerState,cadenceX10,cscFlags,lastCrankEventAgeMs);

@override
String toString() {
  return 'Telemetry(structVersion: $structVersion, flags: $flags, speedX100: $speedX100, avgSpeedX100: $avgSpeedX100, maxSpeedX100: $maxSpeedX100, tripDistanceCm: $tripDistanceCm, movingTimeS: $movingTimeS, odometerM: $odometerM, batteryMv: $batteryMv, batteryPct: $batteryPct, rideState: $rideState, revolutions: $revolutions, lastPulseAgeMs: $lastPulseAgeMs, seq: $seq, sensorState: $sensorState, powerState: $powerState, cadenceX10: $cadenceX10, cscFlags: $cscFlags, lastCrankEventAgeMs: $lastCrankEventAgeMs)';
}


}

/// @nodoc
abstract mixin class _$TelemetryCopyWith<$Res> implements $TelemetryCopyWith<$Res> {
  factory _$TelemetryCopyWith(_Telemetry value, $Res Function(_Telemetry) _then) = __$TelemetryCopyWithImpl;
@override @useResult
$Res call({
 int structVersion, int flags, int speedX100, int avgSpeedX100, int maxSpeedX100, int tripDistanceCm, int movingTimeS, int odometerM, int batteryMv, int batteryPct, RideState rideState, int revolutions, int lastPulseAgeMs, int seq, SensorState sensorState, PowerState powerState, int cadenceX10, int cscFlags, int lastCrankEventAgeMs
});




}
/// @nodoc
class __$TelemetryCopyWithImpl<$Res>
    implements _$TelemetryCopyWith<$Res> {
  __$TelemetryCopyWithImpl(this._self, this._then);

  final _Telemetry _self;
  final $Res Function(_Telemetry) _then;

/// Create a copy of Telemetry
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? structVersion = null,Object? flags = null,Object? speedX100 = null,Object? avgSpeedX100 = null,Object? maxSpeedX100 = null,Object? tripDistanceCm = null,Object? movingTimeS = null,Object? odometerM = null,Object? batteryMv = null,Object? batteryPct = null,Object? rideState = null,Object? revolutions = null,Object? lastPulseAgeMs = null,Object? seq = null,Object? sensorState = null,Object? powerState = null,Object? cadenceX10 = null,Object? cscFlags = null,Object? lastCrankEventAgeMs = null,}) {
  return _then(_Telemetry(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,speedX100: null == speedX100 ? _self.speedX100 : speedX100 // ignore: cast_nullable_to_non_nullable
as int,avgSpeedX100: null == avgSpeedX100 ? _self.avgSpeedX100 : avgSpeedX100 // ignore: cast_nullable_to_non_nullable
as int,maxSpeedX100: null == maxSpeedX100 ? _self.maxSpeedX100 : maxSpeedX100 // ignore: cast_nullable_to_non_nullable
as int,tripDistanceCm: null == tripDistanceCm ? _self.tripDistanceCm : tripDistanceCm // ignore: cast_nullable_to_non_nullable
as int,movingTimeS: null == movingTimeS ? _self.movingTimeS : movingTimeS // ignore: cast_nullable_to_non_nullable
as int,odometerM: null == odometerM ? _self.odometerM : odometerM // ignore: cast_nullable_to_non_nullable
as int,batteryMv: null == batteryMv ? _self.batteryMv : batteryMv // ignore: cast_nullable_to_non_nullable
as int,batteryPct: null == batteryPct ? _self.batteryPct : batteryPct // ignore: cast_nullable_to_non_nullable
as int,rideState: null == rideState ? _self.rideState : rideState // ignore: cast_nullable_to_non_nullable
as RideState,revolutions: null == revolutions ? _self.revolutions : revolutions // ignore: cast_nullable_to_non_nullable
as int,lastPulseAgeMs: null == lastPulseAgeMs ? _self.lastPulseAgeMs : lastPulseAgeMs // ignore: cast_nullable_to_non_nullable
as int,seq: null == seq ? _self.seq : seq // ignore: cast_nullable_to_non_nullable
as int,sensorState: null == sensorState ? _self.sensorState : sensorState // ignore: cast_nullable_to_non_nullable
as SensorState,powerState: null == powerState ? _self.powerState : powerState // ignore: cast_nullable_to_non_nullable
as PowerState,cadenceX10: null == cadenceX10 ? _self.cadenceX10 : cadenceX10 // ignore: cast_nullable_to_non_nullable
as int,cscFlags: null == cscFlags ? _self.cscFlags : cscFlags // ignore: cast_nullable_to_non_nullable
as int,lastCrankEventAgeMs: null == lastCrankEventAgeMs ? _self.lastCrankEventAgeMs : lastCrankEventAgeMs // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}


/// @nodoc
mixin _$DeviceConfig {

 int get structVersion; int get flags; int get wheelCircumferenceMm; int get maxSpeedKmh; int get stopTimeoutS; int get displayTimeoutS; int get deepSleepTimeoutS; int get brightnessPct; int get pageSwitchPeriodS; int get enabledPagesMask; int get lowBatteryPct; int get odometerSaveIntervalM; int get smoothingWindow; int get debounceMs; int get activeEdge; int get pinnedPage; int get battCalScalePermille; int get battCalOffsetMv; List<int> get pageOrder; int get reservedPage; String get deviceName; int get reserved;
/// Create a copy of DeviceConfig
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<DeviceConfig> get copyWith => _$DeviceConfigCopyWithImpl<DeviceConfig>(this as DeviceConfig, _$identity);

  /// Serializes this DeviceConfig to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is DeviceConfig&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.flags, flags) || other.flags == flags)&&(identical(other.wheelCircumferenceMm, wheelCircumferenceMm) || other.wheelCircumferenceMm == wheelCircumferenceMm)&&(identical(other.maxSpeedKmh, maxSpeedKmh) || other.maxSpeedKmh == maxSpeedKmh)&&(identical(other.stopTimeoutS, stopTimeoutS) || other.stopTimeoutS == stopTimeoutS)&&(identical(other.displayTimeoutS, displayTimeoutS) || other.displayTimeoutS == displayTimeoutS)&&(identical(other.deepSleepTimeoutS, deepSleepTimeoutS) || other.deepSleepTimeoutS == deepSleepTimeoutS)&&(identical(other.brightnessPct, brightnessPct) || other.brightnessPct == brightnessPct)&&(identical(other.pageSwitchPeriodS, pageSwitchPeriodS) || other.pageSwitchPeriodS == pageSwitchPeriodS)&&(identical(other.enabledPagesMask, enabledPagesMask) || other.enabledPagesMask == enabledPagesMask)&&(identical(other.lowBatteryPct, lowBatteryPct) || other.lowBatteryPct == lowBatteryPct)&&(identical(other.odometerSaveIntervalM, odometerSaveIntervalM) || other.odometerSaveIntervalM == odometerSaveIntervalM)&&(identical(other.smoothingWindow, smoothingWindow) || other.smoothingWindow == smoothingWindow)&&(identical(other.debounceMs, debounceMs) || other.debounceMs == debounceMs)&&(identical(other.activeEdge, activeEdge) || other.activeEdge == activeEdge)&&(identical(other.pinnedPage, pinnedPage) || other.pinnedPage == pinnedPage)&&(identical(other.battCalScalePermille, battCalScalePermille) || other.battCalScalePermille == battCalScalePermille)&&(identical(other.battCalOffsetMv, battCalOffsetMv) || other.battCalOffsetMv == battCalOffsetMv)&&const DeepCollectionEquality().equals(other.pageOrder, pageOrder)&&(identical(other.reservedPage, reservedPage) || other.reservedPage == reservedPage)&&(identical(other.deviceName, deviceName) || other.deviceName == deviceName)&&(identical(other.reserved, reserved) || other.reserved == reserved));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hashAll([runtimeType,structVersion,flags,wheelCircumferenceMm,maxSpeedKmh,stopTimeoutS,displayTimeoutS,deepSleepTimeoutS,brightnessPct,pageSwitchPeriodS,enabledPagesMask,lowBatteryPct,odometerSaveIntervalM,smoothingWindow,debounceMs,activeEdge,pinnedPage,battCalScalePermille,battCalOffsetMv,const DeepCollectionEquality().hash(pageOrder),reservedPage,deviceName,reserved]);

@override
String toString() {
  return 'DeviceConfig(structVersion: $structVersion, flags: $flags, wheelCircumferenceMm: $wheelCircumferenceMm, maxSpeedKmh: $maxSpeedKmh, stopTimeoutS: $stopTimeoutS, displayTimeoutS: $displayTimeoutS, deepSleepTimeoutS: $deepSleepTimeoutS, brightnessPct: $brightnessPct, pageSwitchPeriodS: $pageSwitchPeriodS, enabledPagesMask: $enabledPagesMask, lowBatteryPct: $lowBatteryPct, odometerSaveIntervalM: $odometerSaveIntervalM, smoothingWindow: $smoothingWindow, debounceMs: $debounceMs, activeEdge: $activeEdge, pinnedPage: $pinnedPage, battCalScalePermille: $battCalScalePermille, battCalOffsetMv: $battCalOffsetMv, pageOrder: $pageOrder, reservedPage: $reservedPage, deviceName: $deviceName, reserved: $reserved)';
}


}

/// @nodoc
abstract mixin class $DeviceConfigCopyWith<$Res>  {
  factory $DeviceConfigCopyWith(DeviceConfig value, $Res Function(DeviceConfig) _then) = _$DeviceConfigCopyWithImpl;
@useResult
$Res call({
 int structVersion, int flags, int wheelCircumferenceMm, int maxSpeedKmh, int stopTimeoutS, int displayTimeoutS, int deepSleepTimeoutS, int brightnessPct, int pageSwitchPeriodS, int enabledPagesMask, int lowBatteryPct, int odometerSaveIntervalM, int smoothingWindow, int debounceMs, int activeEdge, int pinnedPage, int battCalScalePermille, int battCalOffsetMv, List<int> pageOrder, int reservedPage, String deviceName, int reserved
});




}
/// @nodoc
class _$DeviceConfigCopyWithImpl<$Res>
    implements $DeviceConfigCopyWith<$Res> {
  _$DeviceConfigCopyWithImpl(this._self, this._then);

  final DeviceConfig _self;
  final $Res Function(DeviceConfig) _then;

/// Create a copy of DeviceConfig
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? structVersion = null,Object? flags = null,Object? wheelCircumferenceMm = null,Object? maxSpeedKmh = null,Object? stopTimeoutS = null,Object? displayTimeoutS = null,Object? deepSleepTimeoutS = null,Object? brightnessPct = null,Object? pageSwitchPeriodS = null,Object? enabledPagesMask = null,Object? lowBatteryPct = null,Object? odometerSaveIntervalM = null,Object? smoothingWindow = null,Object? debounceMs = null,Object? activeEdge = null,Object? pinnedPage = null,Object? battCalScalePermille = null,Object? battCalOffsetMv = null,Object? pageOrder = null,Object? reservedPage = null,Object? deviceName = null,Object? reserved = null,}) {
  return _then(_self.copyWith(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,wheelCircumferenceMm: null == wheelCircumferenceMm ? _self.wheelCircumferenceMm : wheelCircumferenceMm // ignore: cast_nullable_to_non_nullable
as int,maxSpeedKmh: null == maxSpeedKmh ? _self.maxSpeedKmh : maxSpeedKmh // ignore: cast_nullable_to_non_nullable
as int,stopTimeoutS: null == stopTimeoutS ? _self.stopTimeoutS : stopTimeoutS // ignore: cast_nullable_to_non_nullable
as int,displayTimeoutS: null == displayTimeoutS ? _self.displayTimeoutS : displayTimeoutS // ignore: cast_nullable_to_non_nullable
as int,deepSleepTimeoutS: null == deepSleepTimeoutS ? _self.deepSleepTimeoutS : deepSleepTimeoutS // ignore: cast_nullable_to_non_nullable
as int,brightnessPct: null == brightnessPct ? _self.brightnessPct : brightnessPct // ignore: cast_nullable_to_non_nullable
as int,pageSwitchPeriodS: null == pageSwitchPeriodS ? _self.pageSwitchPeriodS : pageSwitchPeriodS // ignore: cast_nullable_to_non_nullable
as int,enabledPagesMask: null == enabledPagesMask ? _self.enabledPagesMask : enabledPagesMask // ignore: cast_nullable_to_non_nullable
as int,lowBatteryPct: null == lowBatteryPct ? _self.lowBatteryPct : lowBatteryPct // ignore: cast_nullable_to_non_nullable
as int,odometerSaveIntervalM: null == odometerSaveIntervalM ? _self.odometerSaveIntervalM : odometerSaveIntervalM // ignore: cast_nullable_to_non_nullable
as int,smoothingWindow: null == smoothingWindow ? _self.smoothingWindow : smoothingWindow // ignore: cast_nullable_to_non_nullable
as int,debounceMs: null == debounceMs ? _self.debounceMs : debounceMs // ignore: cast_nullable_to_non_nullable
as int,activeEdge: null == activeEdge ? _self.activeEdge : activeEdge // ignore: cast_nullable_to_non_nullable
as int,pinnedPage: null == pinnedPage ? _self.pinnedPage : pinnedPage // ignore: cast_nullable_to_non_nullable
as int,battCalScalePermille: null == battCalScalePermille ? _self.battCalScalePermille : battCalScalePermille // ignore: cast_nullable_to_non_nullable
as int,battCalOffsetMv: null == battCalOffsetMv ? _self.battCalOffsetMv : battCalOffsetMv // ignore: cast_nullable_to_non_nullable
as int,pageOrder: null == pageOrder ? _self.pageOrder : pageOrder // ignore: cast_nullable_to_non_nullable
as List<int>,reservedPage: null == reservedPage ? _self.reservedPage : reservedPage // ignore: cast_nullable_to_non_nullable
as int,deviceName: null == deviceName ? _self.deviceName : deviceName // ignore: cast_nullable_to_non_nullable
as String,reserved: null == reserved ? _self.reserved : reserved // ignore: cast_nullable_to_non_nullable
as int,
  ));
}

}


/// Adds pattern-matching-related methods to [DeviceConfig].
extension DeviceConfigPatterns on DeviceConfig {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _DeviceConfig value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _DeviceConfig() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _DeviceConfig value)  $default,){
final _that = this;
switch (_that) {
case _DeviceConfig():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _DeviceConfig value)?  $default,){
final _that = this;
switch (_that) {
case _DeviceConfig() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int structVersion,  int flags,  int wheelCircumferenceMm,  int maxSpeedKmh,  int stopTimeoutS,  int displayTimeoutS,  int deepSleepTimeoutS,  int brightnessPct,  int pageSwitchPeriodS,  int enabledPagesMask,  int lowBatteryPct,  int odometerSaveIntervalM,  int smoothingWindow,  int debounceMs,  int activeEdge,  int pinnedPage,  int battCalScalePermille,  int battCalOffsetMv,  List<int> pageOrder,  int reservedPage,  String deviceName,  int reserved)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _DeviceConfig() when $default != null:
return $default(_that.structVersion,_that.flags,_that.wheelCircumferenceMm,_that.maxSpeedKmh,_that.stopTimeoutS,_that.displayTimeoutS,_that.deepSleepTimeoutS,_that.brightnessPct,_that.pageSwitchPeriodS,_that.enabledPagesMask,_that.lowBatteryPct,_that.odometerSaveIntervalM,_that.smoothingWindow,_that.debounceMs,_that.activeEdge,_that.pinnedPage,_that.battCalScalePermille,_that.battCalOffsetMv,_that.pageOrder,_that.reservedPage,_that.deviceName,_that.reserved);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int structVersion,  int flags,  int wheelCircumferenceMm,  int maxSpeedKmh,  int stopTimeoutS,  int displayTimeoutS,  int deepSleepTimeoutS,  int brightnessPct,  int pageSwitchPeriodS,  int enabledPagesMask,  int lowBatteryPct,  int odometerSaveIntervalM,  int smoothingWindow,  int debounceMs,  int activeEdge,  int pinnedPage,  int battCalScalePermille,  int battCalOffsetMv,  List<int> pageOrder,  int reservedPage,  String deviceName,  int reserved)  $default,) {final _that = this;
switch (_that) {
case _DeviceConfig():
return $default(_that.structVersion,_that.flags,_that.wheelCircumferenceMm,_that.maxSpeedKmh,_that.stopTimeoutS,_that.displayTimeoutS,_that.deepSleepTimeoutS,_that.brightnessPct,_that.pageSwitchPeriodS,_that.enabledPagesMask,_that.lowBatteryPct,_that.odometerSaveIntervalM,_that.smoothingWindow,_that.debounceMs,_that.activeEdge,_that.pinnedPage,_that.battCalScalePermille,_that.battCalOffsetMv,_that.pageOrder,_that.reservedPage,_that.deviceName,_that.reserved);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int structVersion,  int flags,  int wheelCircumferenceMm,  int maxSpeedKmh,  int stopTimeoutS,  int displayTimeoutS,  int deepSleepTimeoutS,  int brightnessPct,  int pageSwitchPeriodS,  int enabledPagesMask,  int lowBatteryPct,  int odometerSaveIntervalM,  int smoothingWindow,  int debounceMs,  int activeEdge,  int pinnedPage,  int battCalScalePermille,  int battCalOffsetMv,  List<int> pageOrder,  int reservedPage,  String deviceName,  int reserved)?  $default,) {final _that = this;
switch (_that) {
case _DeviceConfig() when $default != null:
return $default(_that.structVersion,_that.flags,_that.wheelCircumferenceMm,_that.maxSpeedKmh,_that.stopTimeoutS,_that.displayTimeoutS,_that.deepSleepTimeoutS,_that.brightnessPct,_that.pageSwitchPeriodS,_that.enabledPagesMask,_that.lowBatteryPct,_that.odometerSaveIntervalM,_that.smoothingWindow,_that.debounceMs,_that.activeEdge,_that.pinnedPage,_that.battCalScalePermille,_that.battCalOffsetMv,_that.pageOrder,_that.reservedPage,_that.deviceName,_that.reserved);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _DeviceConfig extends DeviceConfig {
  const _DeviceConfig({this.structVersion = 1, this.flags = 15, this.wheelCircumferenceMm = 2100, this.maxSpeedKmh = 100, this.stopTimeoutS = 3, this.displayTimeoutS = 60, this.deepSleepTimeoutS = 900, this.brightnessPct = 60, this.pageSwitchPeriodS = 4, this.enabledPagesMask = 31, this.lowBatteryPct = 20, this.odometerSaveIntervalM = 500, this.smoothingWindow = 3, this.debounceMs = 3, this.activeEdge = 0, this.pinnedPage = 0, this.battCalScalePermille = 1000, this.battCalOffsetMv = 0, final  List<int> pageOrder = const <int>[0, 1, 2, 3, 4], this.reservedPage = 0, this.deviceName = 'BikeComp-XXXX', this.reserved = 0}): _pageOrder = pageOrder,super._();
  factory _DeviceConfig.fromJson(Map<String, dynamic> json) => _$DeviceConfigFromJson(json);

@override@JsonKey() final  int structVersion;
@override@JsonKey() final  int flags;
@override@JsonKey() final  int wheelCircumferenceMm;
@override@JsonKey() final  int maxSpeedKmh;
@override@JsonKey() final  int stopTimeoutS;
@override@JsonKey() final  int displayTimeoutS;
@override@JsonKey() final  int deepSleepTimeoutS;
@override@JsonKey() final  int brightnessPct;
@override@JsonKey() final  int pageSwitchPeriodS;
@override@JsonKey() final  int enabledPagesMask;
@override@JsonKey() final  int lowBatteryPct;
@override@JsonKey() final  int odometerSaveIntervalM;
@override@JsonKey() final  int smoothingWindow;
@override@JsonKey() final  int debounceMs;
@override@JsonKey() final  int activeEdge;
@override@JsonKey() final  int pinnedPage;
@override@JsonKey() final  int battCalScalePermille;
@override@JsonKey() final  int battCalOffsetMv;
 final  List<int> _pageOrder;
@override@JsonKey() List<int> get pageOrder {
  if (_pageOrder is EqualUnmodifiableListView) return _pageOrder;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_pageOrder);
}

@override@JsonKey() final  int reservedPage;
@override@JsonKey() final  String deviceName;
@override@JsonKey() final  int reserved;

/// Create a copy of DeviceConfig
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$DeviceConfigCopyWith<_DeviceConfig> get copyWith => __$DeviceConfigCopyWithImpl<_DeviceConfig>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$DeviceConfigToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _DeviceConfig&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.flags, flags) || other.flags == flags)&&(identical(other.wheelCircumferenceMm, wheelCircumferenceMm) || other.wheelCircumferenceMm == wheelCircumferenceMm)&&(identical(other.maxSpeedKmh, maxSpeedKmh) || other.maxSpeedKmh == maxSpeedKmh)&&(identical(other.stopTimeoutS, stopTimeoutS) || other.stopTimeoutS == stopTimeoutS)&&(identical(other.displayTimeoutS, displayTimeoutS) || other.displayTimeoutS == displayTimeoutS)&&(identical(other.deepSleepTimeoutS, deepSleepTimeoutS) || other.deepSleepTimeoutS == deepSleepTimeoutS)&&(identical(other.brightnessPct, brightnessPct) || other.brightnessPct == brightnessPct)&&(identical(other.pageSwitchPeriodS, pageSwitchPeriodS) || other.pageSwitchPeriodS == pageSwitchPeriodS)&&(identical(other.enabledPagesMask, enabledPagesMask) || other.enabledPagesMask == enabledPagesMask)&&(identical(other.lowBatteryPct, lowBatteryPct) || other.lowBatteryPct == lowBatteryPct)&&(identical(other.odometerSaveIntervalM, odometerSaveIntervalM) || other.odometerSaveIntervalM == odometerSaveIntervalM)&&(identical(other.smoothingWindow, smoothingWindow) || other.smoothingWindow == smoothingWindow)&&(identical(other.debounceMs, debounceMs) || other.debounceMs == debounceMs)&&(identical(other.activeEdge, activeEdge) || other.activeEdge == activeEdge)&&(identical(other.pinnedPage, pinnedPage) || other.pinnedPage == pinnedPage)&&(identical(other.battCalScalePermille, battCalScalePermille) || other.battCalScalePermille == battCalScalePermille)&&(identical(other.battCalOffsetMv, battCalOffsetMv) || other.battCalOffsetMv == battCalOffsetMv)&&const DeepCollectionEquality().equals(other._pageOrder, _pageOrder)&&(identical(other.reservedPage, reservedPage) || other.reservedPage == reservedPage)&&(identical(other.deviceName, deviceName) || other.deviceName == deviceName)&&(identical(other.reserved, reserved) || other.reserved == reserved));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hashAll([runtimeType,structVersion,flags,wheelCircumferenceMm,maxSpeedKmh,stopTimeoutS,displayTimeoutS,deepSleepTimeoutS,brightnessPct,pageSwitchPeriodS,enabledPagesMask,lowBatteryPct,odometerSaveIntervalM,smoothingWindow,debounceMs,activeEdge,pinnedPage,battCalScalePermille,battCalOffsetMv,const DeepCollectionEquality().hash(_pageOrder),reservedPage,deviceName,reserved]);

@override
String toString() {
  return 'DeviceConfig(structVersion: $structVersion, flags: $flags, wheelCircumferenceMm: $wheelCircumferenceMm, maxSpeedKmh: $maxSpeedKmh, stopTimeoutS: $stopTimeoutS, displayTimeoutS: $displayTimeoutS, deepSleepTimeoutS: $deepSleepTimeoutS, brightnessPct: $brightnessPct, pageSwitchPeriodS: $pageSwitchPeriodS, enabledPagesMask: $enabledPagesMask, lowBatteryPct: $lowBatteryPct, odometerSaveIntervalM: $odometerSaveIntervalM, smoothingWindow: $smoothingWindow, debounceMs: $debounceMs, activeEdge: $activeEdge, pinnedPage: $pinnedPage, battCalScalePermille: $battCalScalePermille, battCalOffsetMv: $battCalOffsetMv, pageOrder: $pageOrder, reservedPage: $reservedPage, deviceName: $deviceName, reserved: $reserved)';
}


}

/// @nodoc
abstract mixin class _$DeviceConfigCopyWith<$Res> implements $DeviceConfigCopyWith<$Res> {
  factory _$DeviceConfigCopyWith(_DeviceConfig value, $Res Function(_DeviceConfig) _then) = __$DeviceConfigCopyWithImpl;
@override @useResult
$Res call({
 int structVersion, int flags, int wheelCircumferenceMm, int maxSpeedKmh, int stopTimeoutS, int displayTimeoutS, int deepSleepTimeoutS, int brightnessPct, int pageSwitchPeriodS, int enabledPagesMask, int lowBatteryPct, int odometerSaveIntervalM, int smoothingWindow, int debounceMs, int activeEdge, int pinnedPage, int battCalScalePermille, int battCalOffsetMv, List<int> pageOrder, int reservedPage, String deviceName, int reserved
});




}
/// @nodoc
class __$DeviceConfigCopyWithImpl<$Res>
    implements _$DeviceConfigCopyWith<$Res> {
  __$DeviceConfigCopyWithImpl(this._self, this._then);

  final _DeviceConfig _self;
  final $Res Function(_DeviceConfig) _then;

/// Create a copy of DeviceConfig
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? structVersion = null,Object? flags = null,Object? wheelCircumferenceMm = null,Object? maxSpeedKmh = null,Object? stopTimeoutS = null,Object? displayTimeoutS = null,Object? deepSleepTimeoutS = null,Object? brightnessPct = null,Object? pageSwitchPeriodS = null,Object? enabledPagesMask = null,Object? lowBatteryPct = null,Object? odometerSaveIntervalM = null,Object? smoothingWindow = null,Object? debounceMs = null,Object? activeEdge = null,Object? pinnedPage = null,Object? battCalScalePermille = null,Object? battCalOffsetMv = null,Object? pageOrder = null,Object? reservedPage = null,Object? deviceName = null,Object? reserved = null,}) {
  return _then(_DeviceConfig(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,flags: null == flags ? _self.flags : flags // ignore: cast_nullable_to_non_nullable
as int,wheelCircumferenceMm: null == wheelCircumferenceMm ? _self.wheelCircumferenceMm : wheelCircumferenceMm // ignore: cast_nullable_to_non_nullable
as int,maxSpeedKmh: null == maxSpeedKmh ? _self.maxSpeedKmh : maxSpeedKmh // ignore: cast_nullable_to_non_nullable
as int,stopTimeoutS: null == stopTimeoutS ? _self.stopTimeoutS : stopTimeoutS // ignore: cast_nullable_to_non_nullable
as int,displayTimeoutS: null == displayTimeoutS ? _self.displayTimeoutS : displayTimeoutS // ignore: cast_nullable_to_non_nullable
as int,deepSleepTimeoutS: null == deepSleepTimeoutS ? _self.deepSleepTimeoutS : deepSleepTimeoutS // ignore: cast_nullable_to_non_nullable
as int,brightnessPct: null == brightnessPct ? _self.brightnessPct : brightnessPct // ignore: cast_nullable_to_non_nullable
as int,pageSwitchPeriodS: null == pageSwitchPeriodS ? _self.pageSwitchPeriodS : pageSwitchPeriodS // ignore: cast_nullable_to_non_nullable
as int,enabledPagesMask: null == enabledPagesMask ? _self.enabledPagesMask : enabledPagesMask // ignore: cast_nullable_to_non_nullable
as int,lowBatteryPct: null == lowBatteryPct ? _self.lowBatteryPct : lowBatteryPct // ignore: cast_nullable_to_non_nullable
as int,odometerSaveIntervalM: null == odometerSaveIntervalM ? _self.odometerSaveIntervalM : odometerSaveIntervalM // ignore: cast_nullable_to_non_nullable
as int,smoothingWindow: null == smoothingWindow ? _self.smoothingWindow : smoothingWindow // ignore: cast_nullable_to_non_nullable
as int,debounceMs: null == debounceMs ? _self.debounceMs : debounceMs // ignore: cast_nullable_to_non_nullable
as int,activeEdge: null == activeEdge ? _self.activeEdge : activeEdge // ignore: cast_nullable_to_non_nullable
as int,pinnedPage: null == pinnedPage ? _self.pinnedPage : pinnedPage // ignore: cast_nullable_to_non_nullable
as int,battCalScalePermille: null == battCalScalePermille ? _self.battCalScalePermille : battCalScalePermille // ignore: cast_nullable_to_non_nullable
as int,battCalOffsetMv: null == battCalOffsetMv ? _self.battCalOffsetMv : battCalOffsetMv // ignore: cast_nullable_to_non_nullable
as int,pageOrder: null == pageOrder ? _self._pageOrder : pageOrder // ignore: cast_nullable_to_non_nullable
as List<int>,reservedPage: null == reservedPage ? _self.reservedPage : reservedPage // ignore: cast_nullable_to_non_nullable
as int,deviceName: null == deviceName ? _self.deviceName : deviceName // ignore: cast_nullable_to_non_nullable
as String,reserved: null == reserved ? _self.reserved : reserved // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}


/// @nodoc
mixin _$DeviceCommand {

 int get structVersion; DeviceCommandId get id; bool get hasToken; List<int> get payload;
/// Create a copy of DeviceCommand
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$DeviceCommandCopyWith<DeviceCommand> get copyWith => _$DeviceCommandCopyWithImpl<DeviceCommand>(this as DeviceCommand, _$identity);

  /// Serializes this DeviceCommand to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is DeviceCommand&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.id, id) || other.id == id)&&(identical(other.hasToken, hasToken) || other.hasToken == hasToken)&&const DeepCollectionEquality().equals(other.payload, payload));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,id,hasToken,const DeepCollectionEquality().hash(payload));

@override
String toString() {
  return 'DeviceCommand(structVersion: $structVersion, id: $id, hasToken: $hasToken, payload: $payload)';
}


}

/// @nodoc
abstract mixin class $DeviceCommandCopyWith<$Res>  {
  factory $DeviceCommandCopyWith(DeviceCommand value, $Res Function(DeviceCommand) _then) = _$DeviceCommandCopyWithImpl;
@useResult
$Res call({
 int structVersion, DeviceCommandId id, bool hasToken, List<int> payload
});




}
/// @nodoc
class _$DeviceCommandCopyWithImpl<$Res>
    implements $DeviceCommandCopyWith<$Res> {
  _$DeviceCommandCopyWithImpl(this._self, this._then);

  final DeviceCommand _self;
  final $Res Function(DeviceCommand) _then;

/// Create a copy of DeviceCommand
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? structVersion = null,Object? id = null,Object? hasToken = null,Object? payload = null,}) {
  return _then(_self.copyWith(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,id: null == id ? _self.id : id // ignore: cast_nullable_to_non_nullable
as DeviceCommandId,hasToken: null == hasToken ? _self.hasToken : hasToken // ignore: cast_nullable_to_non_nullable
as bool,payload: null == payload ? _self.payload : payload // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}

}


/// Adds pattern-matching-related methods to [DeviceCommand].
extension DeviceCommandPatterns on DeviceCommand {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _DeviceCommand value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _DeviceCommand() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _DeviceCommand value)  $default,){
final _that = this;
switch (_that) {
case _DeviceCommand():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _DeviceCommand value)?  $default,){
final _that = this;
switch (_that) {
case _DeviceCommand() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int structVersion,  DeviceCommandId id,  bool hasToken,  List<int> payload)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _DeviceCommand() when $default != null:
return $default(_that.structVersion,_that.id,_that.hasToken,_that.payload);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int structVersion,  DeviceCommandId id,  bool hasToken,  List<int> payload)  $default,) {final _that = this;
switch (_that) {
case _DeviceCommand():
return $default(_that.structVersion,_that.id,_that.hasToken,_that.payload);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int structVersion,  DeviceCommandId id,  bool hasToken,  List<int> payload)?  $default,) {final _that = this;
switch (_that) {
case _DeviceCommand() when $default != null:
return $default(_that.structVersion,_that.id,_that.hasToken,_that.payload);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _DeviceCommand implements DeviceCommand {
  const _DeviceCommand({this.structVersion = 1, required this.id, this.hasToken = false, final  List<int> payload = const <int>[]}): _payload = payload;
  factory _DeviceCommand.fromJson(Map<String, dynamic> json) => _$DeviceCommandFromJson(json);

@override@JsonKey() final  int structVersion;
@override final  DeviceCommandId id;
@override@JsonKey() final  bool hasToken;
 final  List<int> _payload;
@override@JsonKey() List<int> get payload {
  if (_payload is EqualUnmodifiableListView) return _payload;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_payload);
}


/// Create a copy of DeviceCommand
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$DeviceCommandCopyWith<_DeviceCommand> get copyWith => __$DeviceCommandCopyWithImpl<_DeviceCommand>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$DeviceCommandToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _DeviceCommand&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.id, id) || other.id == id)&&(identical(other.hasToken, hasToken) || other.hasToken == hasToken)&&const DeepCollectionEquality().equals(other._payload, _payload));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,id,hasToken,const DeepCollectionEquality().hash(_payload));

@override
String toString() {
  return 'DeviceCommand(structVersion: $structVersion, id: $id, hasToken: $hasToken, payload: $payload)';
}


}

/// @nodoc
abstract mixin class _$DeviceCommandCopyWith<$Res> implements $DeviceCommandCopyWith<$Res> {
  factory _$DeviceCommandCopyWith(_DeviceCommand value, $Res Function(_DeviceCommand) _then) = __$DeviceCommandCopyWithImpl;
@override @useResult
$Res call({
 int structVersion, DeviceCommandId id, bool hasToken, List<int> payload
});




}
/// @nodoc
class __$DeviceCommandCopyWithImpl<$Res>
    implements _$DeviceCommandCopyWith<$Res> {
  __$DeviceCommandCopyWithImpl(this._self, this._then);

  final _DeviceCommand _self;
  final $Res Function(_DeviceCommand) _then;

/// Create a copy of DeviceCommand
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? structVersion = null,Object? id = null,Object? hasToken = null,Object? payload = null,}) {
  return _then(_DeviceCommand(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,id: null == id ? _self.id : id // ignore: cast_nullable_to_non_nullable
as DeviceCommandId,hasToken: null == hasToken ? _self.hasToken : hasToken // ignore: cast_nullable_to_non_nullable
as bool,payload: null == payload ? _self._payload : payload // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}


}


/// @nodoc
mixin _$CommandResult {

 int get structVersion; DeviceCommandId get commandId; CommandStatus get status; int get detail; int get token; List<int> get payload;
/// Create a copy of CommandResult
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$CommandResultCopyWith<CommandResult> get copyWith => _$CommandResultCopyWithImpl<CommandResult>(this as CommandResult, _$identity);

  /// Serializes this CommandResult to a JSON map.
  Map<String, dynamic> toJson();


@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is CommandResult&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.commandId, commandId) || other.commandId == commandId)&&(identical(other.status, status) || other.status == status)&&(identical(other.detail, detail) || other.detail == detail)&&(identical(other.token, token) || other.token == token)&&const DeepCollectionEquality().equals(other.payload, payload));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,commandId,status,detail,token,const DeepCollectionEquality().hash(payload));

@override
String toString() {
  return 'CommandResult(structVersion: $structVersion, commandId: $commandId, status: $status, detail: $detail, token: $token, payload: $payload)';
}


}

/// @nodoc
abstract mixin class $CommandResultCopyWith<$Res>  {
  factory $CommandResultCopyWith(CommandResult value, $Res Function(CommandResult) _then) = _$CommandResultCopyWithImpl;
@useResult
$Res call({
 int structVersion, DeviceCommandId commandId, CommandStatus status, int detail, int token, List<int> payload
});




}
/// @nodoc
class _$CommandResultCopyWithImpl<$Res>
    implements $CommandResultCopyWith<$Res> {
  _$CommandResultCopyWithImpl(this._self, this._then);

  final CommandResult _self;
  final $Res Function(CommandResult) _then;

/// Create a copy of CommandResult
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? structVersion = null,Object? commandId = null,Object? status = null,Object? detail = null,Object? token = null,Object? payload = null,}) {
  return _then(_self.copyWith(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,commandId: null == commandId ? _self.commandId : commandId // ignore: cast_nullable_to_non_nullable
as DeviceCommandId,status: null == status ? _self.status : status // ignore: cast_nullable_to_non_nullable
as CommandStatus,detail: null == detail ? _self.detail : detail // ignore: cast_nullable_to_non_nullable
as int,token: null == token ? _self.token : token // ignore: cast_nullable_to_non_nullable
as int,payload: null == payload ? _self.payload : payload // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}

}


/// Adds pattern-matching-related methods to [CommandResult].
extension CommandResultPatterns on CommandResult {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _CommandResult value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _CommandResult() when $default != null:
return $default(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _CommandResult value)  $default,){
final _that = this;
switch (_that) {
case _CommandResult():
return $default(_that);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _CommandResult value)?  $default,){
final _that = this;
switch (_that) {
case _CommandResult() when $default != null:
return $default(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( int structVersion,  DeviceCommandId commandId,  CommandStatus status,  int detail,  int token,  List<int> payload)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _CommandResult() when $default != null:
return $default(_that.structVersion,_that.commandId,_that.status,_that.detail,_that.token,_that.payload);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( int structVersion,  DeviceCommandId commandId,  CommandStatus status,  int detail,  int token,  List<int> payload)  $default,) {final _that = this;
switch (_that) {
case _CommandResult():
return $default(_that.structVersion,_that.commandId,_that.status,_that.detail,_that.token,_that.payload);case _:
  throw StateError('Unexpected subclass');

}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( int structVersion,  DeviceCommandId commandId,  CommandStatus status,  int detail,  int token,  List<int> payload)?  $default,) {final _that = this;
switch (_that) {
case _CommandResult() when $default != null:
return $default(_that.structVersion,_that.commandId,_that.status,_that.detail,_that.token,_that.payload);case _:
  return null;

}
}

}

/// @nodoc
@JsonSerializable()

class _CommandResult implements CommandResult {
  const _CommandResult({required this.structVersion, required this.commandId, required this.status, required this.detail, required this.token, required final  List<int> payload}): _payload = payload;
  factory _CommandResult.fromJson(Map<String, dynamic> json) => _$CommandResultFromJson(json);

@override final  int structVersion;
@override final  DeviceCommandId commandId;
@override final  CommandStatus status;
@override final  int detail;
@override final  int token;
 final  List<int> _payload;
@override List<int> get payload {
  if (_payload is EqualUnmodifiableListView) return _payload;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_payload);
}


/// Create a copy of CommandResult
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$CommandResultCopyWith<_CommandResult> get copyWith => __$CommandResultCopyWithImpl<_CommandResult>(this, _$identity);

@override
Map<String, dynamic> toJson() {
  return _$CommandResultToJson(this, );
}

@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _CommandResult&&(identical(other.structVersion, structVersion) || other.structVersion == structVersion)&&(identical(other.commandId, commandId) || other.commandId == commandId)&&(identical(other.status, status) || other.status == status)&&(identical(other.detail, detail) || other.detail == detail)&&(identical(other.token, token) || other.token == token)&&const DeepCollectionEquality().equals(other._payload, _payload));
}

@JsonKey(includeFromJson: false, includeToJson: false)
@override
int get hashCode => Object.hash(runtimeType,structVersion,commandId,status,detail,token,const DeepCollectionEquality().hash(_payload));

@override
String toString() {
  return 'CommandResult(structVersion: $structVersion, commandId: $commandId, status: $status, detail: $detail, token: $token, payload: $payload)';
}


}

/// @nodoc
abstract mixin class _$CommandResultCopyWith<$Res> implements $CommandResultCopyWith<$Res> {
  factory _$CommandResultCopyWith(_CommandResult value, $Res Function(_CommandResult) _then) = __$CommandResultCopyWithImpl;
@override @useResult
$Res call({
 int structVersion, DeviceCommandId commandId, CommandStatus status, int detail, int token, List<int> payload
});




}
/// @nodoc
class __$CommandResultCopyWithImpl<$Res>
    implements _$CommandResultCopyWith<$Res> {
  __$CommandResultCopyWithImpl(this._self, this._then);

  final _CommandResult _self;
  final $Res Function(_CommandResult) _then;

/// Create a copy of CommandResult
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? structVersion = null,Object? commandId = null,Object? status = null,Object? detail = null,Object? token = null,Object? payload = null,}) {
  return _then(_CommandResult(
structVersion: null == structVersion ? _self.structVersion : structVersion // ignore: cast_nullable_to_non_nullable
as int,commandId: null == commandId ? _self.commandId : commandId // ignore: cast_nullable_to_non_nullable
as DeviceCommandId,status: null == status ? _self.status : status // ignore: cast_nullable_to_non_nullable
as CommandStatus,detail: null == detail ? _self.detail : detail // ignore: cast_nullable_to_non_nullable
as int,token: null == token ? _self.token : token // ignore: cast_nullable_to_non_nullable
as int,payload: null == payload ? _self._payload : payload // ignore: cast_nullable_to_non_nullable
as List<int>,
  ));
}


}

// dart format on
