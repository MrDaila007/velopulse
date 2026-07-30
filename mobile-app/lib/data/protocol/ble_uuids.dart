abstract final class BleUuids {
  static const service = '7c9a0001-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const deviceInfo = '7c9a0002-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const telemetry = '7c9a0003-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const configRead = '7c9a0004-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const configWrite = '7c9a0005-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const command = '7c9a0006-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const commandResult = '7c9a0007-4b7d-4f2e-9c1a-2e6d5f8b31a4';
  static const errorLog = '7c9a0008-4b7d-4f2e-9c1a-2e6d5f8b31a4';

  static const requiredCharacteristics = <String>{
    deviceInfo,
    telemetry,
    configRead,
    configWrite,
    command,
    commandResult,
  };
}
