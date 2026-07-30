import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:bikecomp_mobile/domain/validators/config_validator.dart';
import 'package:flutter_test/flutter_test.dart';

void expectValid(DeviceConfig config) {
  expect(ConfigValidator.validate(config).issues, isEmpty);
}

void expectFieldInvalid(DeviceConfig config, String field) {
  expect(ConfigValidator.validate(config).fieldErrors, contains(field));
}

void main() {
  test('normative default config is valid', () {
    expectValid(DeviceConfig.defaults);
  });

  test('wheel circumference includes both boundaries', () {
    expectValid(DeviceConfig.defaults.copyWith(wheelCircumferenceMm: 500));
    expectValid(DeviceConfig.defaults.copyWith(wheelCircumferenceMm: 3000));
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(wheelCircumferenceMm: 499),
      'wheelCircumferenceMm',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(wheelCircumferenceMm: 3001),
      'wheelCircumferenceMm',
    );
  });

  test('zero timeout special values and ranges are enforced', () {
    expectValid(DeviceConfig.defaults.copyWith(displayTimeoutS: 0));
    expectValid(DeviceConfig.defaults.copyWith(deepSleepTimeoutS: 0));
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(displayTimeoutS: 9),
      'displayTimeoutS',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(deepSleepTimeoutS: 59),
      'deepSleepTimeoutS',
    );
  });

  test('full hidden config boundaries are checked', () {
    expectValid(
      DeviceConfig.defaults.copyWith(
        maxSpeedKmh: 20,
        stopTimeoutS: 1,
        brightnessPct: 1,
        pageSwitchPeriodS: 1,
        lowBatteryPct: 5,
        odometerSaveIntervalM: 100,
        smoothingWindow: 2,
        debounceMs: 0,
        activeEdge: 0,
        pinnedPage: 0,
        battCalScalePermille: 800,
        battCalOffsetMv: -500,
      ),
    );
    expectValid(
      DeviceConfig.defaults.copyWith(
        maxSpeedKmh: 200,
        stopTimeoutS: 30,
        brightnessPct: 100,
        pageSwitchPeriodS: 60,
        lowBatteryPct: 50,
        odometerSaveIntervalM: 5000,
        smoothingWindow: 5,
        debounceMs: 50,
        activeEdge: 2,
        pinnedPage: 4,
        battCalScalePermille: 1200,
        battCalOffsetMv: 500,
      ),
    );
  });

  test('page mask, order and reserved fields are strict', () {
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(enabledPagesMask: 0),
      'enabledPagesMask',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(enabledPagesMask: 0x20),
      'enabledPagesMask',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(pageOrder: <int>[0, 1, 2, 3, 3]),
      'pageOrder',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(reservedPage: 1),
      'reservedPage',
    );
    expectFieldInvalid(DeviceConfig.defaults.copyWith(reserved: 1), 'reserved');
  });

  test('device name is ASCII and 3 to 15 characters', () {
    expectValid(DeviceConfig.defaults.copyWith(deviceName: 'Bike 01-A'));
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(deviceName: 'Байк'),
      'deviceName',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(deviceName: 'ab'),
      'deviceName',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(deviceName: '1234567890123456'),
      'deviceName',
    );
  });

  test('all flag bits and signed battery offset encode valid extremes', () {
    expectValid(DeviceConfig.defaults.copyWith(flags: 0));
    expectValid(DeviceConfig.defaults.copyWith(flags: 0xFF));
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(battCalOffsetMv: -501),
      'battCalOffsetMv',
    );
    expectFieldInvalid(
      DeviceConfig.defaults.copyWith(battCalOffsetMv: 501),
      'battCalOffsetMv',
    );
  });
}
