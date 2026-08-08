import { useMemo, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useApp } from '../../context/AppContext';
import { PageHeader } from '../components/PageHeader';
import { getWebBluetoothSupport } from '../../transport/bleTransport';

export function ScanScreen() {
  const { connect, reconnectLast, forgetDevice, connection, connectionState, preferences } =
    useApp();
  const navigate = useNavigate();
  const [busy, setBusy] = useState(false);
  const last = preferences.readLastDevice();
  const bleSupport = useMemo(() => getWebBluetoothSupport(), []);
  const canUseBle = connectionState.useFakeBle || bleSupport.available;
  const appUrl = typeof window !== 'undefined' ? `${window.location.origin}/scan` : '';

  const copyUrl = async () => {
    if (!appUrl) return;
    await navigator.clipboard.writeText(appUrl);
  };

  const handleConnect = async () => {
    if (!canUseBle) return;
    setBusy(true);
    try {
      await connect();
      if (connection.getState().phase === 'ready') {
        navigate('/dashboard');
      }
    } finally {
      setBusy(false);
    }
  };

  const handleReconnect = async () => {
    if (!canUseBle) return;
    setBusy(true);
    try {
      await reconnectLast();
      if (connection.getState().phase === 'ready') navigate('/dashboard');
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="connect-card stack">
      <PageHeader
        title="Подключение"
        description="Выберите BikeComp через Web Bluetooth. Для разработки без железа включите Fake BLE внизу."
      />

      <section className="panel">
        <div className="connect-diagnostics">
          {typeof window !== 'undefined' ? window.location.href : '—'}
          {' · '}
          {bleSupport.secureContext ? 'secure' : 'не secure'}
          {bleSupport.embeddedBrowser ? ` · ${bleSupport.environmentLabel}` : ''}
          {' · bluetooth='}
          {typeof navigator !== 'undefined' && navigator.bluetooth ? 'да' : 'нет'}
        </div>

        {!connectionState.useFakeBle && !bleSupport.available && (
          <div className="panel panel-inset" style={{ marginTop: 'var(--space-4)' }}>
            <p className="chip bad">{bleSupport.message}</p>
            {bleSupport.embeddedBrowser && (
              <div className="btn-row">
                <a className="primary" href={appUrl} target="_blank" rel="noreferrer">
                  Открыть в Chrome / Edge
                </a>
                <button type="button" className="secondary" onClick={() => void copyUrl()}>
                  Скопировать ссылку
                </button>
              </div>
            )}
            <ul className="muted" style={{ margin: '0.5rem 0 0', paddingLeft: '1.2rem' }}>
              {bleSupport.hints.map((hint) => (
                <li key={hint}>{hint}</li>
              ))}
            </ul>
          </div>
        )}

        <label className="field" style={{ marginTop: 'var(--space-4)', marginBottom: 0 }}>
          <span className="checkbox-inline" style={{ margin: 0 }}>
            <input
              type="checkbox"
              checked={connectionState.useFakeBle}
              onChange={(e) => connection.setUseFakeBle(e.target.checked)}
            />
            Fake BLE (эмулятор устройства)
          </span>
        </label>

        <div className="btn-row">
          <button
            type="button"
            className="primary"
            disabled={busy || !canUseBle}
            onClick={() => void handleConnect()}
          >
            Найти и подключить
          </button>
          {last && (
            <button
              type="button"
              className="secondary"
              disabled={busy || !canUseBle}
              onClick={() => void handleReconnect()}
            >
              Переподключить {last.name}
            </button>
          )}
          {last && (
            <button type="button" className="danger" onClick={forgetDevice}>
              Забыть устройство
            </button>
          )}
        </div>

        {connectionState.error && <p className="chip bad">{connectionState.error}</p>}
        {connectionState.phase === 'pairingClosed' && (
          <p className="muted">
            Подключите USB, откройте «Отладка» и выполните <code>open-pairing</code>, либо
            перезагрузите велокомпьютер.
          </p>
        )}
      </section>
    </div>
  );
}
