import 'dart:io';

import 'package:flutter/services.dart';
import 'package:permission_handler/permission_handler.dart';

import '../core/app_error.dart';
import '../domain/entities/models.dart';

class AndroidBlePlatform {
  const AndroidBlePlatform();

  static const _channel = MethodChannel('app.bikecomp.mobile/ble_platform');

  Future<AppError?> ensureScanPermissions() async {
    if (!Platform.isAndroid) return null;
    final sdk = await _channel.invokeMethod<int>('sdkInt') ?? 31;
    if (sdk >= 31) {
      final statuses = await <Permission>[
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
      ].request();
      if (statuses.values.any((status) => !status.isGranted)) {
        return AppErrors.permissionDenied;
      }
      return null;
    }

    final status = await Permission.locationWhenInUse.request();
    if (!status.isGranted) return AppErrors.permissionDenied;
    final enabled = await locationServicesEnabled();
    return enabled ? null : AppErrors.locationServicesOff;
  }

  Future<bool> locationServicesEnabled() async {
    if (!Platform.isAndroid) return true;
    return await _channel.invokeMethod<bool>('locationServicesEnabled') ??
        false;
  }

  Future<void> requestEnableBluetooth() async {
    if (Platform.isAndroid) {
      await _channel.invokeMethod<void>('requestEnableBluetooth');
    }
  }

  Future<BondState> bondState(String deviceId) async {
    if (!Platform.isAndroid) return BondState.unknown;
    final value = await _channel.invokeMethod<String>(
      'bondState',
      <String, Object?>{'deviceId': deviceId},
    );
    return switch (value) {
      'bonded' => BondState.bonded,
      'none' => BondState.none,
      _ => BondState.unknown,
    };
  }

  Future<bool> openSettings() => openAppSettings();
}
