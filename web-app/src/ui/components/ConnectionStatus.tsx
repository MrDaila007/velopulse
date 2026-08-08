import { useApp } from '../../context/AppContext';

export function ConnectionStatus() {
  const { connectionState, disconnect } = useApp();

  const tone =
    connectionState.phase === 'ready'
      ? 'ready'
      : connectionState.phase === 'failed' ||
          connectionState.phase === 'incompatible' ||
          connectionState.phase === 'pairingClosed'
        ? 'bad'
        : connectionState.phase === 'idle'
          ? 'idle'
          : 'busy';

  return (
    <div className="connection-status">
      <div className={`connection-pill ${tone}`}>
        <span className="connection-dot" aria-hidden="true" />
        <span className="connection-message">{connectionState.message}</span>
      </div>
      {connectionState.deviceName ? (
        <span className="connection-chip">{connectionState.deviceName}</span>
      ) : null}
      {connectionState.rssi !== null ? (
        <span className="connection-chip mono">{connectionState.rssi} dBm</span>
      ) : null}
      {connectionState.phase === 'ready' ? (
        <button type="button" className="btn btn-ghost btn-sm" onClick={() => void disconnect()}>
          Отключить
        </button>
      ) : null}
    </div>
  );
}
