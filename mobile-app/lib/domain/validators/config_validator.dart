import '../entities/models.dart';

class ValidationIssue {
  const ValidationIssue(this.field, this.fieldId, this.message);

  final String field;
  final int fieldId;
  final String message;
}

class ConfigValidationResult {
  const ConfigValidationResult(this.issues);

  final List<ValidationIssue> issues;
  bool get isValid => issues.isEmpty;
  Map<String, String> get fieldErrors => {
    for (final issue in issues) issue.field: issue.message,
  };
}

abstract final class ConfigValidator {
  static ConfigValidationResult validate(DeviceConfig config) {
    final issues = <ValidationIssue>[];

    void range(String field, int id, int value, int min, int max) {
      if (value < min || value > max) {
        issues.add(
          ValidationIssue(field, id, 'Допустимый диапазон: $min–$max'),
        );
      }
    }

    void zeroOrRange(String field, int id, int value, int min, int max) {
      if (value != 0 && (value < min || value > max)) {
        issues.add(ValidationIssue(field, id, 'Допустимо 0 либо $min–$max'));
      }
    }

    if (config.structVersion != 1) {
      issues.add(
        const ValidationIssue(
          'structVersion',
          0,
          'Поддерживается структура версии 1',
        ),
      );
    }
    range('flags', 1, config.flags, 0, 255);
    range('wheelCircumferenceMm', 2, config.wheelCircumferenceMm, 500, 3000);
    range('maxSpeedKmh', 4, config.maxSpeedKmh, 20, 200);
    range('stopTimeoutS', 5, config.stopTimeoutS, 1, 30);
    zeroOrRange('displayTimeoutS', 6, config.displayTimeoutS, 10, 600);
    zeroOrRange('deepSleepTimeoutS', 8, config.deepSleepTimeoutS, 60, 3600);
    range('brightnessPct', 10, config.brightnessPct, 1, 100);
    range('pageSwitchPeriodS', 11, config.pageSwitchPeriodS, 1, 60);
    if (config.enabledPagesMask <= 0 || config.enabledPagesMask & ~0x1F != 0) {
      issues.add(
        const ValidationIssue(
          'enabledPagesMask',
          12,
          'Нужно включить минимум одну из пяти страниц',
        ),
      );
    }
    range('lowBatteryPct', 13, config.lowBatteryPct, 5, 50);
    range('odometerSaveIntervalM', 14, config.odometerSaveIntervalM, 100, 5000);
    range('smoothingWindow', 16, config.smoothingWindow, 2, 5);
    range('debounceMs', 17, config.debounceMs, 0, 50);
    range('activeEdge', 18, config.activeEdge, 0, 2);
    range('pinnedPage', 19, config.pinnedPage, 0, 4);
    range('battCalScalePermille', 20, config.battCalScalePermille, 800, 1200);
    range('battCalOffsetMv', 22, config.battCalOffsetMv, -500, 500);

    final order = config.pageOrder;
    if (order.length != 5 ||
        order.toSet().length != 5 ||
        order.any((value) => value < 0 || value > 4)) {
      issues.add(
        const ValidationIssue(
          'pageOrder',
          24,
          'Порядок должен содержать страницы 0–4 без повторов',
        ),
      );
    }
    if (config.reservedPage != 0) {
      issues.add(
        const ValidationIssue(
          'reservedPage',
          29,
          'Зарезервированное поле должно быть 0',
        ),
      );
    }
    final name = config.deviceName;
    final nameValid =
        name.length >= 3 &&
        name.length <= 15 &&
        RegExp(r'^[A-Za-z0-9\-_ ]+$').hasMatch(name);
    if (!nameValid) {
      issues.add(
        const ValidationIssue(
          'deviceName',
          30,
          'Имя: 3–15 символов A–Z, 0–9, пробел, - или _',
        ),
      );
    }
    if (config.reserved != 0) {
      issues.add(
        const ValidationIssue(
          'reserved',
          46,
          'Зарезервированное поле должно быть 0',
        ),
      );
    }

    return ConfigValidationResult(List.unmodifiable(issues));
  }
}
