// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'app_states.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// dart format off
T _$identity<T>(T value) => value;
/// @nodoc
mixin _$ConnectionState {





@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionState);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState()';
}


}

/// @nodoc
class $ConnectionStateCopyWith<$Res>  {
$ConnectionStateCopyWith(ConnectionState _, $Res Function(ConnectionState) __);
}


/// Adds pattern-matching-related methods to [ConnectionState].
extension ConnectionStatePatterns on ConnectionState {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>({TResult Function( ConnectionIdle value)?  idle,TResult Function( ConnectionScanning value)?  scanning,TResult Function( ConnectionConnecting value)?  connecting,TResult Function( ConnectionSynchronizing value)?  synchronizing,TResult Function( ConnectionReady value)?  ready,TResult Function( ConnectionReadOnly value)?  readOnly,TResult Function( ConnectionReconnecting value)?  reconnecting,TResult Function( ConnectionFailed value)?  failed,TResult Function( ConnectionBluetoothOff value)?  bluetoothOff,TResult Function( ConnectionPermissionRequired value)?  permissionRequired,TResult Function( ConnectionIncompatibleProtocol value)?  incompatibleProtocol,required TResult orElse(),}){
final _that = this;
switch (_that) {
case ConnectionIdle() when idle != null:
return idle(_that);case ConnectionScanning() when scanning != null:
return scanning(_that);case ConnectionConnecting() when connecting != null:
return connecting(_that);case ConnectionSynchronizing() when synchronizing != null:
return synchronizing(_that);case ConnectionReady() when ready != null:
return ready(_that);case ConnectionReadOnly() when readOnly != null:
return readOnly(_that);case ConnectionReconnecting() when reconnecting != null:
return reconnecting(_that);case ConnectionFailed() when failed != null:
return failed(_that);case ConnectionBluetoothOff() when bluetoothOff != null:
return bluetoothOff(_that);case ConnectionPermissionRequired() when permissionRequired != null:
return permissionRequired(_that);case ConnectionIncompatibleProtocol() when incompatibleProtocol != null:
return incompatibleProtocol(_that);case _:
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

@optionalTypeArgs TResult map<TResult extends Object?>({required TResult Function( ConnectionIdle value)  idle,required TResult Function( ConnectionScanning value)  scanning,required TResult Function( ConnectionConnecting value)  connecting,required TResult Function( ConnectionSynchronizing value)  synchronizing,required TResult Function( ConnectionReady value)  ready,required TResult Function( ConnectionReadOnly value)  readOnly,required TResult Function( ConnectionReconnecting value)  reconnecting,required TResult Function( ConnectionFailed value)  failed,required TResult Function( ConnectionBluetoothOff value)  bluetoothOff,required TResult Function( ConnectionPermissionRequired value)  permissionRequired,required TResult Function( ConnectionIncompatibleProtocol value)  incompatibleProtocol,}){
final _that = this;
switch (_that) {
case ConnectionIdle():
return idle(_that);case ConnectionScanning():
return scanning(_that);case ConnectionConnecting():
return connecting(_that);case ConnectionSynchronizing():
return synchronizing(_that);case ConnectionReady():
return ready(_that);case ConnectionReadOnly():
return readOnly(_that);case ConnectionReconnecting():
return reconnecting(_that);case ConnectionFailed():
return failed(_that);case ConnectionBluetoothOff():
return bluetoothOff(_that);case ConnectionPermissionRequired():
return permissionRequired(_that);case ConnectionIncompatibleProtocol():
return incompatibleProtocol(_that);}
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>({TResult? Function( ConnectionIdle value)?  idle,TResult? Function( ConnectionScanning value)?  scanning,TResult? Function( ConnectionConnecting value)?  connecting,TResult? Function( ConnectionSynchronizing value)?  synchronizing,TResult? Function( ConnectionReady value)?  ready,TResult? Function( ConnectionReadOnly value)?  readOnly,TResult? Function( ConnectionReconnecting value)?  reconnecting,TResult? Function( ConnectionFailed value)?  failed,TResult? Function( ConnectionBluetoothOff value)?  bluetoothOff,TResult? Function( ConnectionPermissionRequired value)?  permissionRequired,TResult? Function( ConnectionIncompatibleProtocol value)?  incompatibleProtocol,}){
final _that = this;
switch (_that) {
case ConnectionIdle() when idle != null:
return idle(_that);case ConnectionScanning() when scanning != null:
return scanning(_that);case ConnectionConnecting() when connecting != null:
return connecting(_that);case ConnectionSynchronizing() when synchronizing != null:
return synchronizing(_that);case ConnectionReady() when ready != null:
return ready(_that);case ConnectionReadOnly() when readOnly != null:
return readOnly(_that);case ConnectionReconnecting() when reconnecting != null:
return reconnecting(_that);case ConnectionFailed() when failed != null:
return failed(_that);case ConnectionBluetoothOff() when bluetoothOff != null:
return bluetoothOff(_that);case ConnectionPermissionRequired() when permissionRequired != null:
return permissionRequired(_that);case ConnectionIncompatibleProtocol() when incompatibleProtocol != null:
return incompatibleProtocol(_that);case _:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>({TResult Function()?  idle,TResult Function()?  scanning,TResult Function()?  connecting,TResult Function( SyncStage stage)?  synchronizing,TResult Function()?  ready,TResult Function( AppError reason)?  readOnly,TResult Function( int attempt,  int delaySeconds)?  reconnecting,TResult Function( AppError error)?  failed,TResult Function()?  bluetoothOff,TResult Function( AppError error)?  permissionRequired,TResult Function( AppError error)?  incompatibleProtocol,required TResult orElse(),}) {final _that = this;
switch (_that) {
case ConnectionIdle() when idle != null:
return idle();case ConnectionScanning() when scanning != null:
return scanning();case ConnectionConnecting() when connecting != null:
return connecting();case ConnectionSynchronizing() when synchronizing != null:
return synchronizing(_that.stage);case ConnectionReady() when ready != null:
return ready();case ConnectionReadOnly() when readOnly != null:
return readOnly(_that.reason);case ConnectionReconnecting() when reconnecting != null:
return reconnecting(_that.attempt,_that.delaySeconds);case ConnectionFailed() when failed != null:
return failed(_that.error);case ConnectionBluetoothOff() when bluetoothOff != null:
return bluetoothOff();case ConnectionPermissionRequired() when permissionRequired != null:
return permissionRequired(_that.error);case ConnectionIncompatibleProtocol() when incompatibleProtocol != null:
return incompatibleProtocol(_that.error);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>({required TResult Function()  idle,required TResult Function()  scanning,required TResult Function()  connecting,required TResult Function( SyncStage stage)  synchronizing,required TResult Function()  ready,required TResult Function( AppError reason)  readOnly,required TResult Function( int attempt,  int delaySeconds)  reconnecting,required TResult Function( AppError error)  failed,required TResult Function()  bluetoothOff,required TResult Function( AppError error)  permissionRequired,required TResult Function( AppError error)  incompatibleProtocol,}) {final _that = this;
switch (_that) {
case ConnectionIdle():
return idle();case ConnectionScanning():
return scanning();case ConnectionConnecting():
return connecting();case ConnectionSynchronizing():
return synchronizing(_that.stage);case ConnectionReady():
return ready();case ConnectionReadOnly():
return readOnly(_that.reason);case ConnectionReconnecting():
return reconnecting(_that.attempt,_that.delaySeconds);case ConnectionFailed():
return failed(_that.error);case ConnectionBluetoothOff():
return bluetoothOff();case ConnectionPermissionRequired():
return permissionRequired(_that.error);case ConnectionIncompatibleProtocol():
return incompatibleProtocol(_that.error);}
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>({TResult? Function()?  idle,TResult? Function()?  scanning,TResult? Function()?  connecting,TResult? Function( SyncStage stage)?  synchronizing,TResult? Function()?  ready,TResult? Function( AppError reason)?  readOnly,TResult? Function( int attempt,  int delaySeconds)?  reconnecting,TResult? Function( AppError error)?  failed,TResult? Function()?  bluetoothOff,TResult? Function( AppError error)?  permissionRequired,TResult? Function( AppError error)?  incompatibleProtocol,}) {final _that = this;
switch (_that) {
case ConnectionIdle() when idle != null:
return idle();case ConnectionScanning() when scanning != null:
return scanning();case ConnectionConnecting() when connecting != null:
return connecting();case ConnectionSynchronizing() when synchronizing != null:
return synchronizing(_that.stage);case ConnectionReady() when ready != null:
return ready();case ConnectionReadOnly() when readOnly != null:
return readOnly(_that.reason);case ConnectionReconnecting() when reconnecting != null:
return reconnecting(_that.attempt,_that.delaySeconds);case ConnectionFailed() when failed != null:
return failed(_that.error);case ConnectionBluetoothOff() when bluetoothOff != null:
return bluetoothOff();case ConnectionPermissionRequired() when permissionRequired != null:
return permissionRequired(_that.error);case ConnectionIncompatibleProtocol() when incompatibleProtocol != null:
return incompatibleProtocol(_that.error);case _:
  return null;

}
}

}

/// @nodoc


class ConnectionIdle implements ConnectionState {
  const ConnectionIdle();
  






@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionIdle);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState.idle()';
}


}




/// @nodoc


class ConnectionScanning implements ConnectionState {
  const ConnectionScanning();
  






@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionScanning);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState.scanning()';
}


}




/// @nodoc


class ConnectionConnecting implements ConnectionState {
  const ConnectionConnecting();
  






@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionConnecting);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState.connecting()';
}


}




/// @nodoc


class ConnectionSynchronizing implements ConnectionState {
  const ConnectionSynchronizing(this.stage);
  

 final  SyncStage stage;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionSynchronizingCopyWith<ConnectionSynchronizing> get copyWith => _$ConnectionSynchronizingCopyWithImpl<ConnectionSynchronizing>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionSynchronizing&&(identical(other.stage, stage) || other.stage == stage));
}


@override
int get hashCode => Object.hash(runtimeType,stage);

@override
String toString() {
  return 'ConnectionState.synchronizing(stage: $stage)';
}


}

/// @nodoc
abstract mixin class $ConnectionSynchronizingCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionSynchronizingCopyWith(ConnectionSynchronizing value, $Res Function(ConnectionSynchronizing) _then) = _$ConnectionSynchronizingCopyWithImpl;
@useResult
$Res call({
 SyncStage stage
});




}
/// @nodoc
class _$ConnectionSynchronizingCopyWithImpl<$Res>
    implements $ConnectionSynchronizingCopyWith<$Res> {
  _$ConnectionSynchronizingCopyWithImpl(this._self, this._then);

  final ConnectionSynchronizing _self;
  final $Res Function(ConnectionSynchronizing) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? stage = null,}) {
  return _then(ConnectionSynchronizing(
null == stage ? _self.stage : stage // ignore: cast_nullable_to_non_nullable
as SyncStage,
  ));
}


}

/// @nodoc


class ConnectionReady implements ConnectionState {
  const ConnectionReady();
  






@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionReady);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState.ready()';
}


}




/// @nodoc


class ConnectionReadOnly implements ConnectionState {
  const ConnectionReadOnly(this.reason);
  

 final  AppError reason;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionReadOnlyCopyWith<ConnectionReadOnly> get copyWith => _$ConnectionReadOnlyCopyWithImpl<ConnectionReadOnly>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionReadOnly&&(identical(other.reason, reason) || other.reason == reason));
}


@override
int get hashCode => Object.hash(runtimeType,reason);

@override
String toString() {
  return 'ConnectionState.readOnly(reason: $reason)';
}


}

/// @nodoc
abstract mixin class $ConnectionReadOnlyCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionReadOnlyCopyWith(ConnectionReadOnly value, $Res Function(ConnectionReadOnly) _then) = _$ConnectionReadOnlyCopyWithImpl;
@useResult
$Res call({
 AppError reason
});




}
/// @nodoc
class _$ConnectionReadOnlyCopyWithImpl<$Res>
    implements $ConnectionReadOnlyCopyWith<$Res> {
  _$ConnectionReadOnlyCopyWithImpl(this._self, this._then);

  final ConnectionReadOnly _self;
  final $Res Function(ConnectionReadOnly) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? reason = null,}) {
  return _then(ConnectionReadOnly(
null == reason ? _self.reason : reason // ignore: cast_nullable_to_non_nullable
as AppError,
  ));
}


}

/// @nodoc


class ConnectionReconnecting implements ConnectionState {
  const ConnectionReconnecting(this.attempt, this.delaySeconds);
  

 final  int attempt;
 final  int delaySeconds;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionReconnectingCopyWith<ConnectionReconnecting> get copyWith => _$ConnectionReconnectingCopyWithImpl<ConnectionReconnecting>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionReconnecting&&(identical(other.attempt, attempt) || other.attempt == attempt)&&(identical(other.delaySeconds, delaySeconds) || other.delaySeconds == delaySeconds));
}


@override
int get hashCode => Object.hash(runtimeType,attempt,delaySeconds);

@override
String toString() {
  return 'ConnectionState.reconnecting(attempt: $attempt, delaySeconds: $delaySeconds)';
}


}

/// @nodoc
abstract mixin class $ConnectionReconnectingCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionReconnectingCopyWith(ConnectionReconnecting value, $Res Function(ConnectionReconnecting) _then) = _$ConnectionReconnectingCopyWithImpl;
@useResult
$Res call({
 int attempt, int delaySeconds
});




}
/// @nodoc
class _$ConnectionReconnectingCopyWithImpl<$Res>
    implements $ConnectionReconnectingCopyWith<$Res> {
  _$ConnectionReconnectingCopyWithImpl(this._self, this._then);

  final ConnectionReconnecting _self;
  final $Res Function(ConnectionReconnecting) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? attempt = null,Object? delaySeconds = null,}) {
  return _then(ConnectionReconnecting(
null == attempt ? _self.attempt : attempt // ignore: cast_nullable_to_non_nullable
as int,null == delaySeconds ? _self.delaySeconds : delaySeconds // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class ConnectionFailed implements ConnectionState {
  const ConnectionFailed(this.error);
  

 final  AppError error;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionFailedCopyWith<ConnectionFailed> get copyWith => _$ConnectionFailedCopyWithImpl<ConnectionFailed>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionFailed&&(identical(other.error, error) || other.error == error));
}


@override
int get hashCode => Object.hash(runtimeType,error);

@override
String toString() {
  return 'ConnectionState.failed(error: $error)';
}


}

/// @nodoc
abstract mixin class $ConnectionFailedCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionFailedCopyWith(ConnectionFailed value, $Res Function(ConnectionFailed) _then) = _$ConnectionFailedCopyWithImpl;
@useResult
$Res call({
 AppError error
});




}
/// @nodoc
class _$ConnectionFailedCopyWithImpl<$Res>
    implements $ConnectionFailedCopyWith<$Res> {
  _$ConnectionFailedCopyWithImpl(this._self, this._then);

  final ConnectionFailed _self;
  final $Res Function(ConnectionFailed) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? error = null,}) {
  return _then(ConnectionFailed(
null == error ? _self.error : error // ignore: cast_nullable_to_non_nullable
as AppError,
  ));
}


}

/// @nodoc


class ConnectionBluetoothOff implements ConnectionState {
  const ConnectionBluetoothOff();
  






@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionBluetoothOff);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
  return 'ConnectionState.bluetoothOff()';
}


}




/// @nodoc


class ConnectionPermissionRequired implements ConnectionState {
  const ConnectionPermissionRequired(this.error);
  

 final  AppError error;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionPermissionRequiredCopyWith<ConnectionPermissionRequired> get copyWith => _$ConnectionPermissionRequiredCopyWithImpl<ConnectionPermissionRequired>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionPermissionRequired&&(identical(other.error, error) || other.error == error));
}


@override
int get hashCode => Object.hash(runtimeType,error);

@override
String toString() {
  return 'ConnectionState.permissionRequired(error: $error)';
}


}

/// @nodoc
abstract mixin class $ConnectionPermissionRequiredCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionPermissionRequiredCopyWith(ConnectionPermissionRequired value, $Res Function(ConnectionPermissionRequired) _then) = _$ConnectionPermissionRequiredCopyWithImpl;
@useResult
$Res call({
 AppError error
});




}
/// @nodoc
class _$ConnectionPermissionRequiredCopyWithImpl<$Res>
    implements $ConnectionPermissionRequiredCopyWith<$Res> {
  _$ConnectionPermissionRequiredCopyWithImpl(this._self, this._then);

  final ConnectionPermissionRequired _self;
  final $Res Function(ConnectionPermissionRequired) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? error = null,}) {
  return _then(ConnectionPermissionRequired(
null == error ? _self.error : error // ignore: cast_nullable_to_non_nullable
as AppError,
  ));
}


}

/// @nodoc


class ConnectionIncompatibleProtocol implements ConnectionState {
  const ConnectionIncompatibleProtocol(this.error);
  

 final  AppError error;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConnectionIncompatibleProtocolCopyWith<ConnectionIncompatibleProtocol> get copyWith => _$ConnectionIncompatibleProtocolCopyWithImpl<ConnectionIncompatibleProtocol>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConnectionIncompatibleProtocol&&(identical(other.error, error) || other.error == error));
}


@override
int get hashCode => Object.hash(runtimeType,error);

@override
String toString() {
  return 'ConnectionState.incompatibleProtocol(error: $error)';
}


}

/// @nodoc
abstract mixin class $ConnectionIncompatibleProtocolCopyWith<$Res> implements $ConnectionStateCopyWith<$Res> {
  factory $ConnectionIncompatibleProtocolCopyWith(ConnectionIncompatibleProtocol value, $Res Function(ConnectionIncompatibleProtocol) _then) = _$ConnectionIncompatibleProtocolCopyWithImpl;
@useResult
$Res call({
 AppError error
});




}
/// @nodoc
class _$ConnectionIncompatibleProtocolCopyWithImpl<$Res>
    implements $ConnectionIncompatibleProtocolCopyWith<$Res> {
  _$ConnectionIncompatibleProtocolCopyWithImpl(this._self, this._then);

  final ConnectionIncompatibleProtocol _self;
  final $Res Function(ConnectionIncompatibleProtocol) _then;

/// Create a copy of ConnectionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? error = null,}) {
  return _then(ConnectionIncompatibleProtocol(
null == error ? _self.error : error // ignore: cast_nullable_to_non_nullable
as AppError,
  ));
}


}

/// @nodoc
mixin _$SessionState {

 ConnectionState get connection; List<BleScanResult> get devices; BleScanResult? get selectedDevice; DeviceInfo? get deviceInfo; DeviceConfig? get deviceConfig; Telemetry? get telemetry; int? get rssi; DateTime? get lastTelemetryAt; String? get lastMessage; AppError? get lastError; bool get commandInFlight; bool get sensorTestActive;
/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$SessionStateCopyWith<SessionState> get copyWith => _$SessionStateCopyWithImpl<SessionState>(this as SessionState, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is SessionState&&(identical(other.connection, connection) || other.connection == connection)&&const DeepCollectionEquality().equals(other.devices, devices)&&(identical(other.selectedDevice, selectedDevice) || other.selectedDevice == selectedDevice)&&(identical(other.deviceInfo, deviceInfo) || other.deviceInfo == deviceInfo)&&(identical(other.deviceConfig, deviceConfig) || other.deviceConfig == deviceConfig)&&(identical(other.telemetry, telemetry) || other.telemetry == telemetry)&&(identical(other.rssi, rssi) || other.rssi == rssi)&&(identical(other.lastTelemetryAt, lastTelemetryAt) || other.lastTelemetryAt == lastTelemetryAt)&&(identical(other.lastMessage, lastMessage) || other.lastMessage == lastMessage)&&(identical(other.lastError, lastError) || other.lastError == lastError)&&(identical(other.commandInFlight, commandInFlight) || other.commandInFlight == commandInFlight)&&(identical(other.sensorTestActive, sensorTestActive) || other.sensorTestActive == sensorTestActive));
}


@override
int get hashCode => Object.hash(runtimeType,connection,const DeepCollectionEquality().hash(devices),selectedDevice,deviceInfo,deviceConfig,telemetry,rssi,lastTelemetryAt,lastMessage,lastError,commandInFlight,sensorTestActive);

@override
String toString() {
  return 'SessionState(connection: $connection, devices: $devices, selectedDevice: $selectedDevice, deviceInfo: $deviceInfo, deviceConfig: $deviceConfig, telemetry: $telemetry, rssi: $rssi, lastTelemetryAt: $lastTelemetryAt, lastMessage: $lastMessage, lastError: $lastError, commandInFlight: $commandInFlight, sensorTestActive: $sensorTestActive)';
}


}

/// @nodoc
abstract mixin class $SessionStateCopyWith<$Res>  {
  factory $SessionStateCopyWith(SessionState value, $Res Function(SessionState) _then) = _$SessionStateCopyWithImpl;
@useResult
$Res call({
 ConnectionState connection, List<BleScanResult> devices, BleScanResult? selectedDevice, DeviceInfo? deviceInfo, DeviceConfig? deviceConfig, Telemetry? telemetry, int? rssi, DateTime? lastTelemetryAt, String? lastMessage, AppError? lastError, bool commandInFlight, bool sensorTestActive
});


$ConnectionStateCopyWith<$Res> get connection;$DeviceInfoCopyWith<$Res>? get deviceInfo;$DeviceConfigCopyWith<$Res>? get deviceConfig;$TelemetryCopyWith<$Res>? get telemetry;

}
/// @nodoc
class _$SessionStateCopyWithImpl<$Res>
    implements $SessionStateCopyWith<$Res> {
  _$SessionStateCopyWithImpl(this._self, this._then);

  final SessionState _self;
  final $Res Function(SessionState) _then;

/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? connection = null,Object? devices = null,Object? selectedDevice = freezed,Object? deviceInfo = freezed,Object? deviceConfig = freezed,Object? telemetry = freezed,Object? rssi = freezed,Object? lastTelemetryAt = freezed,Object? lastMessage = freezed,Object? lastError = freezed,Object? commandInFlight = null,Object? sensorTestActive = null,}) {
  return _then(_self.copyWith(
connection: null == connection ? _self.connection : connection // ignore: cast_nullable_to_non_nullable
as ConnectionState,devices: null == devices ? _self.devices : devices // ignore: cast_nullable_to_non_nullable
as List<BleScanResult>,selectedDevice: freezed == selectedDevice ? _self.selectedDevice : selectedDevice // ignore: cast_nullable_to_non_nullable
as BleScanResult?,deviceInfo: freezed == deviceInfo ? _self.deviceInfo : deviceInfo // ignore: cast_nullable_to_non_nullable
as DeviceInfo?,deviceConfig: freezed == deviceConfig ? _self.deviceConfig : deviceConfig // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,telemetry: freezed == telemetry ? _self.telemetry : telemetry // ignore: cast_nullable_to_non_nullable
as Telemetry?,rssi: freezed == rssi ? _self.rssi : rssi // ignore: cast_nullable_to_non_nullable
as int?,lastTelemetryAt: freezed == lastTelemetryAt ? _self.lastTelemetryAt : lastTelemetryAt // ignore: cast_nullable_to_non_nullable
as DateTime?,lastMessage: freezed == lastMessage ? _self.lastMessage : lastMessage // ignore: cast_nullable_to_non_nullable
as String?,lastError: freezed == lastError ? _self.lastError : lastError // ignore: cast_nullable_to_non_nullable
as AppError?,commandInFlight: null == commandInFlight ? _self.commandInFlight : commandInFlight // ignore: cast_nullable_to_non_nullable
as bool,sensorTestActive: null == sensorTestActive ? _self.sensorTestActive : sensorTestActive // ignore: cast_nullable_to_non_nullable
as bool,
  ));
}
/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$ConnectionStateCopyWith<$Res> get connection {
  
  return $ConnectionStateCopyWith<$Res>(_self.connection, (value) {
    return _then(_self.copyWith(connection: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceInfoCopyWith<$Res>? get deviceInfo {
    if (_self.deviceInfo == null) {
    return null;
  }

  return $DeviceInfoCopyWith<$Res>(_self.deviceInfo!, (value) {
    return _then(_self.copyWith(deviceInfo: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get deviceConfig {
    if (_self.deviceConfig == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.deviceConfig!, (value) {
    return _then(_self.copyWith(deviceConfig: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$TelemetryCopyWith<$Res>? get telemetry {
    if (_self.telemetry == null) {
    return null;
  }

  return $TelemetryCopyWith<$Res>(_self.telemetry!, (value) {
    return _then(_self.copyWith(telemetry: value));
  });
}
}


/// Adds pattern-matching-related methods to [SessionState].
extension SessionStatePatterns on SessionState {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _SessionState value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _SessionState() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _SessionState value)  $default,){
final _that = this;
switch (_that) {
case _SessionState():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _SessionState value)?  $default,){
final _that = this;
switch (_that) {
case _SessionState() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( ConnectionState connection,  List<BleScanResult> devices,  BleScanResult? selectedDevice,  DeviceInfo? deviceInfo,  DeviceConfig? deviceConfig,  Telemetry? telemetry,  int? rssi,  DateTime? lastTelemetryAt,  String? lastMessage,  AppError? lastError,  bool commandInFlight,  bool sensorTestActive)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _SessionState() when $default != null:
return $default(_that.connection,_that.devices,_that.selectedDevice,_that.deviceInfo,_that.deviceConfig,_that.telemetry,_that.rssi,_that.lastTelemetryAt,_that.lastMessage,_that.lastError,_that.commandInFlight,_that.sensorTestActive);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( ConnectionState connection,  List<BleScanResult> devices,  BleScanResult? selectedDevice,  DeviceInfo? deviceInfo,  DeviceConfig? deviceConfig,  Telemetry? telemetry,  int? rssi,  DateTime? lastTelemetryAt,  String? lastMessage,  AppError? lastError,  bool commandInFlight,  bool sensorTestActive)  $default,) {final _that = this;
switch (_that) {
case _SessionState():
return $default(_that.connection,_that.devices,_that.selectedDevice,_that.deviceInfo,_that.deviceConfig,_that.telemetry,_that.rssi,_that.lastTelemetryAt,_that.lastMessage,_that.lastError,_that.commandInFlight,_that.sensorTestActive);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( ConnectionState connection,  List<BleScanResult> devices,  BleScanResult? selectedDevice,  DeviceInfo? deviceInfo,  DeviceConfig? deviceConfig,  Telemetry? telemetry,  int? rssi,  DateTime? lastTelemetryAt,  String? lastMessage,  AppError? lastError,  bool commandInFlight,  bool sensorTestActive)?  $default,) {final _that = this;
switch (_that) {
case _SessionState() when $default != null:
return $default(_that.connection,_that.devices,_that.selectedDevice,_that.deviceInfo,_that.deviceConfig,_that.telemetry,_that.rssi,_that.lastTelemetryAt,_that.lastMessage,_that.lastError,_that.commandInFlight,_that.sensorTestActive);case _:
  return null;

}
}

}

/// @nodoc


class _SessionState implements SessionState {
  const _SessionState({this.connection = const ConnectionState.idle(), final  List<BleScanResult> devices = const <BleScanResult>[], this.selectedDevice, this.deviceInfo, this.deviceConfig, this.telemetry, this.rssi, this.lastTelemetryAt, this.lastMessage, this.lastError, this.commandInFlight = false, this.sensorTestActive = false}): _devices = devices;
  

@override@JsonKey() final  ConnectionState connection;
 final  List<BleScanResult> _devices;
@override@JsonKey() List<BleScanResult> get devices {
  if (_devices is EqualUnmodifiableListView) return _devices;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_devices);
}

@override final  BleScanResult? selectedDevice;
@override final  DeviceInfo? deviceInfo;
@override final  DeviceConfig? deviceConfig;
@override final  Telemetry? telemetry;
@override final  int? rssi;
@override final  DateTime? lastTelemetryAt;
@override final  String? lastMessage;
@override final  AppError? lastError;
@override@JsonKey() final  bool commandInFlight;
@override@JsonKey() final  bool sensorTestActive;

/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$SessionStateCopyWith<_SessionState> get copyWith => __$SessionStateCopyWithImpl<_SessionState>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _SessionState&&(identical(other.connection, connection) || other.connection == connection)&&const DeepCollectionEquality().equals(other._devices, _devices)&&(identical(other.selectedDevice, selectedDevice) || other.selectedDevice == selectedDevice)&&(identical(other.deviceInfo, deviceInfo) || other.deviceInfo == deviceInfo)&&(identical(other.deviceConfig, deviceConfig) || other.deviceConfig == deviceConfig)&&(identical(other.telemetry, telemetry) || other.telemetry == telemetry)&&(identical(other.rssi, rssi) || other.rssi == rssi)&&(identical(other.lastTelemetryAt, lastTelemetryAt) || other.lastTelemetryAt == lastTelemetryAt)&&(identical(other.lastMessage, lastMessage) || other.lastMessage == lastMessage)&&(identical(other.lastError, lastError) || other.lastError == lastError)&&(identical(other.commandInFlight, commandInFlight) || other.commandInFlight == commandInFlight)&&(identical(other.sensorTestActive, sensorTestActive) || other.sensorTestActive == sensorTestActive));
}


@override
int get hashCode => Object.hash(runtimeType,connection,const DeepCollectionEquality().hash(_devices),selectedDevice,deviceInfo,deviceConfig,telemetry,rssi,lastTelemetryAt,lastMessage,lastError,commandInFlight,sensorTestActive);

@override
String toString() {
  return 'SessionState(connection: $connection, devices: $devices, selectedDevice: $selectedDevice, deviceInfo: $deviceInfo, deviceConfig: $deviceConfig, telemetry: $telemetry, rssi: $rssi, lastTelemetryAt: $lastTelemetryAt, lastMessage: $lastMessage, lastError: $lastError, commandInFlight: $commandInFlight, sensorTestActive: $sensorTestActive)';
}


}

/// @nodoc
abstract mixin class _$SessionStateCopyWith<$Res> implements $SessionStateCopyWith<$Res> {
  factory _$SessionStateCopyWith(_SessionState value, $Res Function(_SessionState) _then) = __$SessionStateCopyWithImpl;
@override @useResult
$Res call({
 ConnectionState connection, List<BleScanResult> devices, BleScanResult? selectedDevice, DeviceInfo? deviceInfo, DeviceConfig? deviceConfig, Telemetry? telemetry, int? rssi, DateTime? lastTelemetryAt, String? lastMessage, AppError? lastError, bool commandInFlight, bool sensorTestActive
});


@override $ConnectionStateCopyWith<$Res> get connection;@override $DeviceInfoCopyWith<$Res>? get deviceInfo;@override $DeviceConfigCopyWith<$Res>? get deviceConfig;@override $TelemetryCopyWith<$Res>? get telemetry;

}
/// @nodoc
class __$SessionStateCopyWithImpl<$Res>
    implements _$SessionStateCopyWith<$Res> {
  __$SessionStateCopyWithImpl(this._self, this._then);

  final _SessionState _self;
  final $Res Function(_SessionState) _then;

/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? connection = null,Object? devices = null,Object? selectedDevice = freezed,Object? deviceInfo = freezed,Object? deviceConfig = freezed,Object? telemetry = freezed,Object? rssi = freezed,Object? lastTelemetryAt = freezed,Object? lastMessage = freezed,Object? lastError = freezed,Object? commandInFlight = null,Object? sensorTestActive = null,}) {
  return _then(_SessionState(
connection: null == connection ? _self.connection : connection // ignore: cast_nullable_to_non_nullable
as ConnectionState,devices: null == devices ? _self._devices : devices // ignore: cast_nullable_to_non_nullable
as List<BleScanResult>,selectedDevice: freezed == selectedDevice ? _self.selectedDevice : selectedDevice // ignore: cast_nullable_to_non_nullable
as BleScanResult?,deviceInfo: freezed == deviceInfo ? _self.deviceInfo : deviceInfo // ignore: cast_nullable_to_non_nullable
as DeviceInfo?,deviceConfig: freezed == deviceConfig ? _self.deviceConfig : deviceConfig // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,telemetry: freezed == telemetry ? _self.telemetry : telemetry // ignore: cast_nullable_to_non_nullable
as Telemetry?,rssi: freezed == rssi ? _self.rssi : rssi // ignore: cast_nullable_to_non_nullable
as int?,lastTelemetryAt: freezed == lastTelemetryAt ? _self.lastTelemetryAt : lastTelemetryAt // ignore: cast_nullable_to_non_nullable
as DateTime?,lastMessage: freezed == lastMessage ? _self.lastMessage : lastMessage // ignore: cast_nullable_to_non_nullable
as String?,lastError: freezed == lastError ? _self.lastError : lastError // ignore: cast_nullable_to_non_nullable
as AppError?,commandInFlight: null == commandInFlight ? _self.commandInFlight : commandInFlight // ignore: cast_nullable_to_non_nullable
as bool,sensorTestActive: null == sensorTestActive ? _self.sensorTestActive : sensorTestActive // ignore: cast_nullable_to_non_nullable
as bool,
  ));
}

/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$ConnectionStateCopyWith<$Res> get connection {
  
  return $ConnectionStateCopyWith<$Res>(_self.connection, (value) {
    return _then(_self.copyWith(connection: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceInfoCopyWith<$Res>? get deviceInfo {
    if (_self.deviceInfo == null) {
    return null;
  }

  return $DeviceInfoCopyWith<$Res>(_self.deviceInfo!, (value) {
    return _then(_self.copyWith(deviceInfo: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get deviceConfig {
    if (_self.deviceConfig == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.deviceConfig!, (value) {
    return _then(_self.copyWith(deviceConfig: value));
  });
}/// Create a copy of SessionState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$TelemetryCopyWith<$Res>? get telemetry {
    if (_self.telemetry == null) {
    return null;
  }

  return $TelemetryCopyWith<$Res>(_self.telemetry!, (value) {
    return _then(_self.copyWith(telemetry: value));
  });
}
}

/// @nodoc
mixin _$ConfigDraftState {

 String? get deviceId; DeviceConfig? get deviceConfig; DeviceConfig? get draft; Map<String, String> get fieldErrors; bool get isWriting; DateTime? get lastSyncedAt; bool get hasConflict;
/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$ConfigDraftStateCopyWith<ConfigDraftState> get copyWith => _$ConfigDraftStateCopyWithImpl<ConfigDraftState>(this as ConfigDraftState, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is ConfigDraftState&&(identical(other.deviceId, deviceId) || other.deviceId == deviceId)&&(identical(other.deviceConfig, deviceConfig) || other.deviceConfig == deviceConfig)&&(identical(other.draft, draft) || other.draft == draft)&&const DeepCollectionEquality().equals(other.fieldErrors, fieldErrors)&&(identical(other.isWriting, isWriting) || other.isWriting == isWriting)&&(identical(other.lastSyncedAt, lastSyncedAt) || other.lastSyncedAt == lastSyncedAt)&&(identical(other.hasConflict, hasConflict) || other.hasConflict == hasConflict));
}


@override
int get hashCode => Object.hash(runtimeType,deviceId,deviceConfig,draft,const DeepCollectionEquality().hash(fieldErrors),isWriting,lastSyncedAt,hasConflict);

@override
String toString() {
  return 'ConfigDraftState(deviceId: $deviceId, deviceConfig: $deviceConfig, draft: $draft, fieldErrors: $fieldErrors, isWriting: $isWriting, lastSyncedAt: $lastSyncedAt, hasConflict: $hasConflict)';
}


}

/// @nodoc
abstract mixin class $ConfigDraftStateCopyWith<$Res>  {
  factory $ConfigDraftStateCopyWith(ConfigDraftState value, $Res Function(ConfigDraftState) _then) = _$ConfigDraftStateCopyWithImpl;
@useResult
$Res call({
 String? deviceId, DeviceConfig? deviceConfig, DeviceConfig? draft, Map<String, String> fieldErrors, bool isWriting, DateTime? lastSyncedAt, bool hasConflict
});


$DeviceConfigCopyWith<$Res>? get deviceConfig;$DeviceConfigCopyWith<$Res>? get draft;

}
/// @nodoc
class _$ConfigDraftStateCopyWithImpl<$Res>
    implements $ConfigDraftStateCopyWith<$Res> {
  _$ConfigDraftStateCopyWithImpl(this._self, this._then);

  final ConfigDraftState _self;
  final $Res Function(ConfigDraftState) _then;

/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') @override $Res call({Object? deviceId = freezed,Object? deviceConfig = freezed,Object? draft = freezed,Object? fieldErrors = null,Object? isWriting = null,Object? lastSyncedAt = freezed,Object? hasConflict = null,}) {
  return _then(_self.copyWith(
deviceId: freezed == deviceId ? _self.deviceId : deviceId // ignore: cast_nullable_to_non_nullable
as String?,deviceConfig: freezed == deviceConfig ? _self.deviceConfig : deviceConfig // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,draft: freezed == draft ? _self.draft : draft // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,fieldErrors: null == fieldErrors ? _self.fieldErrors : fieldErrors // ignore: cast_nullable_to_non_nullable
as Map<String, String>,isWriting: null == isWriting ? _self.isWriting : isWriting // ignore: cast_nullable_to_non_nullable
as bool,lastSyncedAt: freezed == lastSyncedAt ? _self.lastSyncedAt : lastSyncedAt // ignore: cast_nullable_to_non_nullable
as DateTime?,hasConflict: null == hasConflict ? _self.hasConflict : hasConflict // ignore: cast_nullable_to_non_nullable
as bool,
  ));
}
/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get deviceConfig {
    if (_self.deviceConfig == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.deviceConfig!, (value) {
    return _then(_self.copyWith(deviceConfig: value));
  });
}/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get draft {
    if (_self.draft == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.draft!, (value) {
    return _then(_self.copyWith(draft: value));
  });
}
}


/// Adds pattern-matching-related methods to [ConfigDraftState].
extension ConfigDraftStatePatterns on ConfigDraftState {
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

@optionalTypeArgs TResult maybeMap<TResult extends Object?>(TResult Function( _ConfigDraftState value)?  $default,{required TResult orElse(),}){
final _that = this;
switch (_that) {
case _ConfigDraftState() when $default != null:
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

@optionalTypeArgs TResult map<TResult extends Object?>(TResult Function( _ConfigDraftState value)  $default,){
final _that = this;
switch (_that) {
case _ConfigDraftState():
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

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>(TResult? Function( _ConfigDraftState value)?  $default,){
final _that = this;
switch (_that) {
case _ConfigDraftState() when $default != null:
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

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>(TResult Function( String? deviceId,  DeviceConfig? deviceConfig,  DeviceConfig? draft,  Map<String, String> fieldErrors,  bool isWriting,  DateTime? lastSyncedAt,  bool hasConflict)?  $default,{required TResult orElse(),}) {final _that = this;
switch (_that) {
case _ConfigDraftState() when $default != null:
return $default(_that.deviceId,_that.deviceConfig,_that.draft,_that.fieldErrors,_that.isWriting,_that.lastSyncedAt,_that.hasConflict);case _:
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

@optionalTypeArgs TResult when<TResult extends Object?>(TResult Function( String? deviceId,  DeviceConfig? deviceConfig,  DeviceConfig? draft,  Map<String, String> fieldErrors,  bool isWriting,  DateTime? lastSyncedAt,  bool hasConflict)  $default,) {final _that = this;
switch (_that) {
case _ConfigDraftState():
return $default(_that.deviceId,_that.deviceConfig,_that.draft,_that.fieldErrors,_that.isWriting,_that.lastSyncedAt,_that.hasConflict);case _:
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

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>(TResult? Function( String? deviceId,  DeviceConfig? deviceConfig,  DeviceConfig? draft,  Map<String, String> fieldErrors,  bool isWriting,  DateTime? lastSyncedAt,  bool hasConflict)?  $default,) {final _that = this;
switch (_that) {
case _ConfigDraftState() when $default != null:
return $default(_that.deviceId,_that.deviceConfig,_that.draft,_that.fieldErrors,_that.isWriting,_that.lastSyncedAt,_that.hasConflict);case _:
  return null;

}
}

}

/// @nodoc


class _ConfigDraftState extends ConfigDraftState {
  const _ConfigDraftState({this.deviceId, this.deviceConfig, this.draft, final  Map<String, String> fieldErrors = const <String, String>{}, this.isWriting = false, this.lastSyncedAt, this.hasConflict = false}): _fieldErrors = fieldErrors,super._();
  

@override final  String? deviceId;
@override final  DeviceConfig? deviceConfig;
@override final  DeviceConfig? draft;
 final  Map<String, String> _fieldErrors;
@override@JsonKey() Map<String, String> get fieldErrors {
  if (_fieldErrors is EqualUnmodifiableMapView) return _fieldErrors;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableMapView(_fieldErrors);
}

@override@JsonKey() final  bool isWriting;
@override final  DateTime? lastSyncedAt;
@override@JsonKey() final  bool hasConflict;

/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override @JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
_$ConfigDraftStateCopyWith<_ConfigDraftState> get copyWith => __$ConfigDraftStateCopyWithImpl<_ConfigDraftState>(this, _$identity);



@override
bool operator ==(Object other) {
  return identical(this, other) || (other.runtimeType == runtimeType&&other is _ConfigDraftState&&(identical(other.deviceId, deviceId) || other.deviceId == deviceId)&&(identical(other.deviceConfig, deviceConfig) || other.deviceConfig == deviceConfig)&&(identical(other.draft, draft) || other.draft == draft)&&const DeepCollectionEquality().equals(other._fieldErrors, _fieldErrors)&&(identical(other.isWriting, isWriting) || other.isWriting == isWriting)&&(identical(other.lastSyncedAt, lastSyncedAt) || other.lastSyncedAt == lastSyncedAt)&&(identical(other.hasConflict, hasConflict) || other.hasConflict == hasConflict));
}


@override
int get hashCode => Object.hash(runtimeType,deviceId,deviceConfig,draft,const DeepCollectionEquality().hash(_fieldErrors),isWriting,lastSyncedAt,hasConflict);

@override
String toString() {
  return 'ConfigDraftState(deviceId: $deviceId, deviceConfig: $deviceConfig, draft: $draft, fieldErrors: $fieldErrors, isWriting: $isWriting, lastSyncedAt: $lastSyncedAt, hasConflict: $hasConflict)';
}


}

/// @nodoc
abstract mixin class _$ConfigDraftStateCopyWith<$Res> implements $ConfigDraftStateCopyWith<$Res> {
  factory _$ConfigDraftStateCopyWith(_ConfigDraftState value, $Res Function(_ConfigDraftState) _then) = __$ConfigDraftStateCopyWithImpl;
@override @useResult
$Res call({
 String? deviceId, DeviceConfig? deviceConfig, DeviceConfig? draft, Map<String, String> fieldErrors, bool isWriting, DateTime? lastSyncedAt, bool hasConflict
});


@override $DeviceConfigCopyWith<$Res>? get deviceConfig;@override $DeviceConfigCopyWith<$Res>? get draft;

}
/// @nodoc
class __$ConfigDraftStateCopyWithImpl<$Res>
    implements _$ConfigDraftStateCopyWith<$Res> {
  __$ConfigDraftStateCopyWithImpl(this._self, this._then);

  final _ConfigDraftState _self;
  final $Res Function(_ConfigDraftState) _then;

/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override @pragma('vm:prefer-inline') $Res call({Object? deviceId = freezed,Object? deviceConfig = freezed,Object? draft = freezed,Object? fieldErrors = null,Object? isWriting = null,Object? lastSyncedAt = freezed,Object? hasConflict = null,}) {
  return _then(_ConfigDraftState(
deviceId: freezed == deviceId ? _self.deviceId : deviceId // ignore: cast_nullable_to_non_nullable
as String?,deviceConfig: freezed == deviceConfig ? _self.deviceConfig : deviceConfig // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,draft: freezed == draft ? _self.draft : draft // ignore: cast_nullable_to_non_nullable
as DeviceConfig?,fieldErrors: null == fieldErrors ? _self._fieldErrors : fieldErrors // ignore: cast_nullable_to_non_nullable
as Map<String, String>,isWriting: null == isWriting ? _self.isWriting : isWriting // ignore: cast_nullable_to_non_nullable
as bool,lastSyncedAt: freezed == lastSyncedAt ? _self.lastSyncedAt : lastSyncedAt // ignore: cast_nullable_to_non_nullable
as DateTime?,hasConflict: null == hasConflict ? _self.hasConflict : hasConflict // ignore: cast_nullable_to_non_nullable
as bool,
  ));
}

/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get deviceConfig {
    if (_self.deviceConfig == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.deviceConfig!, (value) {
    return _then(_self.copyWith(deviceConfig: value));
  });
}/// Create a copy of ConfigDraftState
/// with the given fields replaced by the non-null parameter values.
@override
@pragma('vm:prefer-inline')
$DeviceConfigCopyWith<$Res>? get draft {
    if (_self.draft == null) {
    return null;
  }

  return $DeviceConfigCopyWith<$Res>(_self.draft!, (value) {
    return _then(_self.copyWith(draft: value));
  });
}
}

// dart format on
