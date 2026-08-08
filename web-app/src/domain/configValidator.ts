import type { DeviceConfig } from './types';

export interface ValidationIssue {
  field: string;
  fieldId: number;
  message: string;
}

export interface ConfigValidationResult {
  issues: ValidationIssue[];
  isValid: boolean;
  fieldErrors: Record<string, string>;
}

export function validateConfig(config: DeviceConfig): ConfigValidationResult {
  const issues: ValidationIssue[] = [];

  const range = (field: string, id: number, value: number, min: number, max: number) => {
    if (value < min || value > max) {
      issues.push({ field, fieldId: id, message: `Допустимый диапазон: ${min}–${max}` });
    }
  };

  const zeroOrRange = (field: string, id: number, value: number, min: number, max: number) => {
    if (value !== 0 && (value < min || value > max)) {
      issues.push({ field, fieldId: id, message: `Допустимо 0 либо ${min}–${max}` });
    }
  };

  if (config.structVersion !== 1) {
    issues.push({ field: 'structVersion', fieldId: 0, message: 'Поддерживается структура версии 1' });
  }
  range('flags', 1, config.flags, 0, 255);
  range('wheelCircumferenceMm', 2, config.wheelCircumferenceMm, 500, 3000);
  range('maxSpeedKmh', 4, config.maxSpeedKmh, 20, 200);
  range('stopTimeoutS', 5, config.stopTimeoutS, 1, 30);
  zeroOrRange('displayTimeoutS', 6, config.displayTimeoutS, 10, 600);
  zeroOrRange('deepSleepTimeoutS', 8, config.deepSleepTimeoutS, 60, 3600);
  range('brightnessPct', 10, config.brightnessPct, 1, 100);
  range('pageSwitchPeriodS', 11, config.pageSwitchPeriodS, 1, 60);
  if (config.enabledPagesMask <= 0 || (config.enabledPagesMask & ~0x7f) !== 0) {
    issues.push({
      field: 'enabledPagesMask',
      fieldId: 12,
      message: 'Нужно включить минимум одну страницу (биты 0–6)',
    });
  }
  range('lowBatteryPct', 13, config.lowBatteryPct, 5, 50);
  range('odometerSaveIntervalM', 14, config.odometerSaveIntervalM, 100, 5000);
  range('smoothingWindow', 16, config.smoothingWindow, 2, 5);
  range('debounceMs', 17, config.debounceMs, 0, 50);
  range('activeEdge', 18, config.activeEdge, 0, 2);
  range('pinnedPage', 19, config.pinnedPage, 0, 6);
  range('battCalScalePermille', 20, config.battCalScalePermille, 800, 1200);
  range('battCalOffsetMv', 22, config.battCalOffsetMv, -500, 500);

  const order = config.pageOrder;
  const unique = new Set(order);
  if (order.length !== 5 || unique.size !== 5 || order.some((v) => v < 0 || v > 4)) {
    issues.push({
      field: 'pageOrder',
      fieldId: 24,
      message: 'Порядок должен содержать страницы 0–4 без повторов',
    });
  }
  if (config.reservedPage !== 0) {
    issues.push({ field: 'reservedPage', fieldId: 29, message: 'Зарезервированное поле должно быть 0' });
  }
  const name = config.deviceName;
  if (!/^[A-Za-z0-9\-_ ]{3,15}$/.test(name)) {
    issues.push({
      field: 'deviceName',
      fieldId: 30,
      message: 'Имя: 3–15 символов A–Z, 0–9, пробел, - или _',
    });
  }
  if (config.reserved !== 0) {
    issues.push({ field: 'reserved', fieldId: 46, message: 'Зарезервированное поле должно быть 0' });
  }

  return {
    issues,
    isValid: issues.length === 0,
    fieldErrors: Object.fromEntries(issues.map((i) => [i.field, i.message])),
  };
}
