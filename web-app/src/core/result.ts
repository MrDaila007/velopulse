export type Result<T> = { ok: true; value: T } | { ok: false; error: AppError };

export function success<T>(value: T): Result<T> {
  return { ok: true, value };
}

export function failure<T>(error: AppError): Result<T> {
  return { ok: false, error };
}

export type AppErrorKind =
  | 'unknown'
  | 'bluetoothUnavailable'
  | 'notPaired'
  | 'writeTimeout'
  | 'deviceRejected'
  | 'validation'
  | 'malformed'
  | 'storageError'
  | 'tokenExpired'
  | 'hardware'
  | 'unsupported'
  | 'busy'
  | 'incompatibleProtocol'
  | 'pairingClosed';

export interface AppError {
  kind: AppErrorKind;
  message: string;
  action?: string;
  fieldId?: number;
}

export const AppErrors = {
  bluetoothUnavailable: {
    kind: 'bluetoothUnavailable' as const,
    message: 'Web Bluetooth недоступен. Используйте Chrome или Edge на ПК.',
    action: 'Проверить браузер',
  },
  notPaired: {
    kind: 'notPaired' as const,
    message: 'Требуется сопряжение с устройством',
    action: 'Открыть окно сопряжения через USB: open-pairing',
  },
  writeTimeout: {
    kind: 'writeTimeout' as const,
    message: 'Устройство не ответило вовремя',
    action: 'Повторить',
  },
  incompatibleProtocol: (major: number) => ({
    kind: 'incompatibleProtocol' as const,
    message: `Несовместимая версия протокола (major=${major})`,
    action: 'Обновите приложение или прошивку',
  }),
  pairingClosed: {
    kind: 'pairingClosed' as const,
    message: 'Окно сопряжения закрыто',
    action: 'Перезагрузите устройство или выполните open-pairing через USB',
  },
  validation: (message: string, fieldId?: number): AppError => ({
    kind: 'validation',
    message,
    fieldId,
  }),
  malformed: (message: string): AppError => ({
    kind: 'malformed',
    message,
  }),
  unknown: (error: unknown): AppError => ({
    kind: 'unknown',
    message: error instanceof Error ? error.message : String(error),
  }),
};
