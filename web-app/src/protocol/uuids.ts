export const BleUuids = {
  service: '7c9a0001-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  deviceInfo: '7c9a0002-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  telemetry: '7c9a0003-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  configRead: '7c9a0004-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  configWrite: '7c9a0005-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  command: '7c9a0006-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  commandResult: '7c9a0007-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  errorLog: '7c9a0008-4b7d-4f2e-9c1a-2e6d5f8b31a4',
  companionWrite: '7c9a000b-4b7d-4f2e-9c1a-2e6d5f8b31a4',
} as const;

export const requiredCharacteristics = new Set<string>([
  BleUuids.deviceInfo,
  BleUuids.telemetry,
  BleUuids.configRead,
  BleUuids.configWrite,
  BleUuids.command,
  BleUuids.commandResult,
]);

export const allOptionalServices = [BleUuids.service];
