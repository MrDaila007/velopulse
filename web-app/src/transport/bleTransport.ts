import type { BondState } from '../domain/types';

export type BleAdapterState = 'unknown' | 'ready' | 'off' | 'unauthorized';
export type BleLinkState = 'disconnected' | 'connecting' | 'connected' | 'disconnecting';

export interface BleScanResult {
  deviceId: string;
  name: string;
  rssi: number;
  bondState: BondState;
}

export interface BleDiscovery {
  characteristics: Set<string>;
}

export interface BleTransport {
  readonly adapterState: BleAdapterState;
  scan(): AsyncIterable<BleScanResult>;
  stopScan(): Promise<void>;
  connect(deviceId: string): AsyncIterable<BleLinkState>;
  disconnect(): Promise<void>;
  requestMtu(mtu: number): Promise<number>;
  discoverServices(): Promise<BleDiscovery>;
  read(characteristicUuid: string): Promise<Uint8Array>;
  writeWithResponse(characteristicUuid: string, value: Uint8Array): Promise<void>;
  subscribe(characteristicUuid: string): AsyncIterable<Uint8Array>;
  readRssi(): Promise<number>;
}

export type WebBluetoothBlockReason =
  | 'available'
  | 'insecureContext'
  | 'unsupportedBrowser'
  | 'embeddedBrowser'
  | 'apiMissing';

export interface WebBluetoothSupport {
  available: boolean;
  reason: WebBluetoothBlockReason;
  message: string;
  hints: string[];
  secureContext: boolean;
  userAgent: string;
  embeddedBrowser: boolean;
  environmentLabel: string;
}

function isChromiumBrowser(ua: string): boolean {
  return /Chrome|Chromium|Edg\//.test(ua) && !/Firefox|OPR\//.test(ua);
}

function detectEmbeddedBrowser(ua: string): { embedded: boolean; label: string } {
  const lower = ua.toLowerCase();
  if (lower.includes('cursor')) return { embedded: true, label: 'Cursor' };
  if (lower.includes('electron')) return { embedded: true, label: 'Electron' };
  if (lower.includes('vscode') || lower.includes('code/')) {
    return { embedded: true, label: 'VS Code' };
  }
  if (typeof window !== 'undefined') {
    const w = window as Window & { electron?: unknown };
    if (w.electron) return { embedded: true, label: 'Electron' };
  }
  return { embedded: false, label: 'browser' };
}

export function getWebBluetoothSupport(): WebBluetoothSupport {
  const ua = typeof navigator !== 'undefined' ? navigator.userAgent : '';
  const secureContext = typeof window !== 'undefined' && window.isSecureContext;
  const { embedded, label } = detectEmbeddedBrowser(ua);
  const hints: string[] = [];

  if (!secureContext) {
    hints.push('Откройте приложение по адресу http://localhost:5173 (не по IP вроде 192.168.x.x).');
    hints.push('Либо используйте HTTPS.');
    return {
      available: false,
      reason: 'insecureContext',
      message: 'Web Bluetooth требует secure context (localhost или HTTPS).',
      hints,
      secureContext,
      userAgent: ua,
      embeddedBrowser: embedded,
      environmentLabel: label,
    };
  }

  if (!isChromiumBrowser(ua)) {
    hints.push('Используйте Google Chrome или Microsoft Edge на ПК.');
    hints.push('Firefox и Safari Web Bluetooth не поддерживают.');
    return {
      available: false,
      reason: 'unsupportedBrowser',
      message: 'Этот браузер не поддерживает Web Bluetooth.',
      hints,
      secureContext,
      userAgent: ua,
      embeddedBrowser: embedded,
      environmentLabel: label,
    };
  }

  const bluetooth = typeof navigator !== 'undefined' ? navigator.bluetooth : undefined;
  if (!bluetooth?.requestDevice) {
    if (embedded) {
      hints.push(
        `Встроенный браузер ${label} не даёт доступ к Bluetooth. Это ожидаемо.`,
      );
      hints.push('Запустите: cd web-app && npm run dev — затем npm run open');
      hints.push('Или вручную откройте http://localhost:5173/scan в Google Chrome / Edge.');
    } else {
      hints.push('Linux: в Chrome откройте chrome://flags и включите «Experimental Web Platform features».');
      hints.push('Перезапустите Chrome полностью, затем снова: npm run web:open');
      hints.push('Проверка в консоли: navigator.userAgent (не должно быть Cursor/Electron).');
      hints.push('Нужен BlueZ: systemctl status bluetooth');
    }
    hints.push('USB-отладка (вкладка «Отладка») тоже требует системный Chrome/Edge.');
    hints.push('Пока можно включить Fake BLE для проверки интерфейса.');
    return {
      available: false,
      reason: embedded ? 'embeddedBrowser' : 'apiMissing',
      message: embedded
        ? `Web Bluetooth недоступен во встроенном окне ${label}.`
        : 'API navigator.bluetooth недоступен.',
      hints,
      secureContext,
      userAgent: ua,
      embeddedBrowser: embedded,
      environmentLabel: label,
    };
  }

  return {
    available: true,
    reason: 'available',
    message: 'Web Bluetooth доступен.',
    hints: [],
    secureContext,
    userAgent: ua,
    embeddedBrowser: embedded,
    environmentLabel: label,
  };
}

export function isWebBluetoothAvailable(): boolean {
  return getWebBluetoothSupport().available;
}

export function assertWebBluetoothAvailable(): void {
  const support = getWebBluetoothSupport();
  if (!support.available) {
    throw new Error([support.message, ...support.hints].join(' '));
  }
}

export function isWebSerialAvailable(): boolean {
  return typeof navigator !== 'undefined' && 'serial' in navigator;
}
