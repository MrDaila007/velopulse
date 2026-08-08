import { createContext, useContext, useEffect, useMemo, useRef, useState, type ReactNode } from 'react';
import { FakeBleTransport } from '../transport/fakeBleTransport';
import { WebBluetoothTransport } from '../transport/webBluetoothTransport';
import { WebSerialTransport } from '../transport/serialTransport';
import { ConnectionController, type ConnectionState } from '../services/connectionController';
import { CompanionSyncService } from '../services/companionSyncService';
import { PreferencesStore } from '../services/preferencesStore';
import { getWebBluetoothSupport } from '../transport/bleTransport';
import { FirmwareMigrationStore } from '../services/firmwareMigrationStore';
import { RideLogRecorder } from '../services/rideLogRecorder';

interface AppContextValue {
  connection: ConnectionController;
  connectionState: ConnectionState;
  companionSync: CompanionSyncService;
  preferences: PreferencesStore;
  migrationStore: FirmwareMigrationStore;
  rideLog: RideLogRecorder;
  serial: WebSerialTransport;
  connect: () => Promise<void>;
  reconnectLast: () => Promise<void>;
  disconnect: () => Promise<void>;
  forgetDevice: () => void;
}

const AppContext = createContext<AppContextValue | null>(null);

export function AppProvider({ children }: { children: ReactNode }) {
  const connection = useMemo(() => new ConnectionController(), []);
  const companionSync = useMemo(() => new CompanionSyncService(), []);
  const preferences = useMemo(() => new PreferencesStore(), []);
  const migrationStore = useMemo(() => new FirmwareMigrationStore(), []);
  const rideLog = useMemo(() => new RideLogRecorder(), []);
  const serial = useMemo(() => new WebSerialTransport(), []);
  const [connectionState, setConnectionState] = useState<ConnectionState>(connection.getState());
  const companionStarted = useRef(false);

  useEffect(() => {
    return connection.subscribe(setConnectionState);
  }, [connection]);

  useEffect(() => {
    if (connectionState.phase === 'ready' && connectionState.repository) {
      if (!companionStarted.current) {
        companionSync.configure(connectionState.companionSupported);
        companionSync.start(connectionState.repository);
        companionStarted.current = true;
      }
      if (connectionState.telemetry) {
        rideLog.record(connectionState.telemetry);
      }
      if (connectionState.deviceId && connectionState.deviceName) {
        preferences.writeLastDevice({
          deviceId: connectionState.deviceId,
          name: connectionState.deviceName,
        });
      }
    } else {
      companionStarted.current = false;
      companionSync.stop();
    }
  }, [connectionState, companionSync, preferences, rideLog]);

  const createTransport = () =>
    connectionState.useFakeBle ? new FakeBleTransport() : new WebBluetoothTransport();

  const connect = async () => {
    if (!connectionState.useFakeBle && !getWebBluetoothSupport().available) {
      return;
    }
    await connection.scanAndConnect(createTransport());
  };

  const reconnectLast = async () => {
    const last = preferences.readLastDevice();
    if (!last) return;
    if (connectionState.useFakeBle) {
      await connection.connectTo(last.deviceId, last.name, new FakeBleTransport());
      return;
    }
    const transport = new WebBluetoothTransport();
    const permitted = await transport.tryGetPermittedDevice(last.deviceId);
    if (permitted) {
      await connection.connectTo(last.deviceId, last.name, transport);
      return;
    }
    await connect();
  };

  const disconnect = async () => {
    await connection.disconnect();
    rideLog.reset();
  };

  const forgetDevice = () => {
    preferences.clearLastDevice();
  };

  const value: AppContextValue = {
    connection,
    connectionState,
    companionSync,
    preferences,
    migrationStore,
    rideLog,
    serial,
    connect,
    reconnectLast,
    disconnect,
    forgetDevice,
  };

  return <AppContext.Provider value={value}>{children}</AppContext.Provider>;
}

export function useApp() {
  const ctx = useContext(AppContext);
  if (!ctx) throw new Error('useApp must be used within AppProvider');
  return ctx;
}
