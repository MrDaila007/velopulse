import 'package:bikecomp_mobile/data/ble/ble_discovery_filter.dart';
import 'package:bikecomp_mobile/data/protocol/ble_uuids.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('BikeComp advertisement filter', () {
    test('accepts the advertised service UUID', () {
      expect(
        isBikeCompAdvertisement(
          name: 'Custom name',
          serviceUuids: const [BleUuids.service],
        ),
        isTrue,
      );
    });

    test('matches service UUID without case sensitivity', () {
      expect(
        isBikeCompAdvertisement(
          name: '',
          serviceUuids: [BleUuids.service.toUpperCase()],
        ),
        isTrue,
      );
    });

    test('accepts default name when Android omits service UUID', () {
      expect(
        isBikeCompAdvertisement(name: 'BikeComp-D210', serviceUuids: const []),
        isTrue,
      );
    });

    test('rejects unrelated advertisements', () {
      expect(
        isBikeCompAdvertisement(
          name: 'Headphones',
          serviceUuids: const ['0000180f-0000-1000-8000-00805f9b34fb'],
        ),
        isFalse,
      );
    });

    test('does not accept an unnamed advertisement', () {
      expect(
        isBikeCompAdvertisement(name: '', serviceUuids: const []),
        isFalse,
      );
    });
  });
}
