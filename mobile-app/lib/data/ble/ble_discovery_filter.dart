import '../protocol/ble_uuids.dart';

const _defaultBikeCompNamePrefix = 'bikecomp';

String _normalizeUuid(String uuid) => uuid.trim().toLowerCase();

bool _matchesBikeCompName(String name) {
  final normalized = name.trim().toLowerCase();
  return normalized.startsWith(_defaultBikeCompNamePrefix);
}

/// Filters an unfiltered platform scan in Dart.
///
/// Some Android BLE stacks miss peripherals when a custom 128-bit service UUID
/// is passed to the native scan filter. The default name is a discovery fallback;
/// service discovery still validates the full BikeComp GATT contract on connect.
bool isBikeCompAdvertisement({
  required String name,
  required Iterable<String> serviceUuids,
}) {
  final targetService = _normalizeUuid(BleUuids.service);
  final advertisesService = serviceUuids.any(
    (uuid) => _normalizeUuid(uuid) == targetService,
  );
  return advertisesService || _matchesBikeCompName(name);
}
