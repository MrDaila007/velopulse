import '../protocol/ble_uuids.dart';

const _defaultBikeCompNamePrefix = 'bikecomp-';

/// Filters an unfiltered platform scan in Dart.
///
/// Some Android BLE stacks miss peripherals when a custom 128-bit service UUID
/// is passed to the native scan filter. The default name is a discovery fallback;
/// service discovery still validates the full BikeComp GATT contract on connect.
bool isBikeCompAdvertisement({
  required String name,
  required Iterable<String> serviceUuids,
}) {
  final advertisesService = serviceUuids.any(
    (uuid) => uuid.trim().toLowerCase() == BleUuids.service,
  );
  return advertisesService ||
      name.trim().toLowerCase().startsWith(_defaultBikeCompNamePrefix);
}
