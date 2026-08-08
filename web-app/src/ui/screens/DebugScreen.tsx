import { useEffect, useState } from 'react';
import { useApp } from '../../context/AppContext';
import { bytesToHex } from '../../protocol/codecs';
import { BleUuids } from '../../protocol/uuids';
import { parseAmbientLine } from '../../services/ambientSerialParser';
import type { AmbientSample } from '../../services/ambientSerialParser';
import { serialCommandPresets } from '../../transport/serialTransport';
import { InfoCallout } from '../components/InfoCallout';
import { LiveMetric } from '../components/LiveMetric';

import { PageHeader } from '../components/PageHeader';

export function DebugScreen() {
  const { serial, connectionState } = useApp();
  const [lines, setLines] = useState(serial.history);
  const [customCmd, setCustomCmd] = useState('');
  const [rawHex, setRawHex] = useState('');
  const [gattLog, setGattLog] = useState<string[]>([]);
  const [ambientSample, setAmbientSample] = useState<AmbientSample | null>(null);
  const [ambientStreaming, setAmbientStreaming] = useState(false);

  useEffect(() => {
    return serial.onLine((line) => {
      setLines([...serial.history]);
      const sample = parseAmbientLine(line.text);
      if (sample) setAmbientSample(sample);
    });
  }, [serial]);

  const connectSerial = async () => {
    try {
      await serial.connect();
      setLines([...serial.history]);
    } catch (error) {
      alert(error instanceof Error ? error.message : String(error));
    }
  };

  const sendSerial = async (cmd: string) => {
    try {
      await serial.send(cmd);
      if (cmd === 'ambient-raw') setAmbientStreaming(true);
      if (cmd === 'ambient-stop') setAmbientStreaming(false);
    } catch (error) {
      alert(error instanceof Error ? error.message : String(error));
    }
  };

  const writeRawGatt = async () => {
    const repo = connectionState.repository;
    if (!repo) {
      alert('Нет BLE-подключения');
      return;
    }
    const cleaned = rawHex.replace(/\s+/g, '');
    if (!/^[0-9a-fA-F]*$/.test(cleaned) || cleaned.length % 2 !== 0) {
      alert('Некорректный hex');
      return;
    }
    const bytes = new Uint8Array(cleaned.length / 2);
    for (let i = 0; i < bytes.length; i++) {
      bytes[i] = parseInt(cleaned.slice(i * 2, i * 2 + 2), 16);
    }
    try {
      await connectionState.repository!.sendCommand({
        structVersion: 1,
        id: 'resetTrip',
        hasToken: false,
        payload: Array.from(bytes),
      });
      setGattLog((prev) => [`TX command ${bytesToHex(bytes)}`, ...prev].slice(0, 100));
    } catch (error) {
      alert(error instanceof Error ? error.message : String(error));
    }
  };

  return (
    <div className="stack">
      <PageHeader
        title="Отладка"
        description="USB Serial для прошивки и сырые BLE-команды для разработки."
      />

      <section className="panel">
        <h2>Фотодиод (LDR)</h2>
        <p className="muted">Live-данные через USB-команду <code>ambient-raw</code>.</p>
        <InfoCallout tone="info">
          Пороги <code>raw_dark</code> / <code>raw_bright</code> задаются в прошивке (
          <code>platformio.ini</code>) или через <code>tools/ldr_calibrate.py</code>. В приложении
          они не редактируются.
        </InfoCallout>
        <div className="btn-row" style={{ marginTop: 0 }}>
          <button
            type="button"
            className="primary"
            disabled={!serial.isConnected || ambientStreaming}
            onClick={() => void sendSerial('ambient-raw')}
          >
            Start ambient-raw
          </button>
          <button
            type="button"
            className="secondary"
            disabled={!serial.isConnected || !ambientStreaming}
            onClick={() => void sendSerial('ambient-stop')}
          >
            Stop
          </button>
        </div>
        {ambientSample ? (
          <div className="live-metrics">
            <LiveMetric label="raw" value={ambientSample.raw} />
            <LiveMetric label="filtered" value={ambientSample.filtered} />
            <LiveMetric label="auto_pct" value={ambientSample.autoPct} unit="%" />
            <LiveMetric label="effective_pct" value={ambientSample.effectivePct} unit="%" />
            <LiveMetric label="enabled" value={ambientSample.enabled} />
            <LiveMetric label="valid" value={ambientSample.valid} />
          </div>
        ) : (
          <p className="muted">Нет данных. Подключите USB и запустите поток.</p>
        )}
      </section>

      <section className="panel">
        <h2>USB Serial (Web Serial)</h2>
        <p className="muted">CDC 115200 · команды как в firmware serial console.</p>
        <InfoCallout tone="info">
          На <strong>Seeed XIAO</strong> (USB 2886:xxxx) DTR не используется (иначе сброс). При первом
          подключении USB CDC может на секунду «мигнуть» — приложение подождёт и переподключится
          само (до 2 раз). Закройте PlatformIO Monitor на этом COM-порту.
        </InfoCallout>
        <div className="btn-row">
          <button type="button" className="primary" onClick={() => void connectSerial()}>
            {serial.isConnected ? 'Порт выбран' : 'Выбрать COM-порт'}
          </button>
          <button
            type="button"
            className="secondary"
            disabled={!serial.selectedPortInfo}
            onClick={async () => {
              try {
                await serial.reconnect();
                setLines([...serial.history]);
              } catch (error) {
                alert(error instanceof Error ? error.message : String(error));
              }
            }}
          >
            Переподключить
          </button>
          <button
            type="button"
            className="secondary"
            disabled={!serial.isConnected}
            onClick={() => void serial.disconnect()}
          >
            Отключить
          </button>
        </div>
        <div className="btn-row">
          {serialCommandPresets.map((cmd) => (
            <button
              key={cmd}
              type="button"
              className="secondary"
              disabled={!serial.isConnected}
              onClick={() => void sendSerial(cmd)}
            >
              {cmd}
            </button>
          ))}
        </div>
        <div className="field">
          <label>Произвольная команда</label>
          <input value={customCmd} onChange={(e) => setCustomCmd(e.target.value)} />
        </div>
        <button
          type="button"
          className="secondary"
          disabled={!serial.isConnected || !customCmd.trim()}
          onClick={() => void sendSerial(customCmd)}
        >
          Отправить
        </button>
        <div className="serial-log" style={{ marginTop: '0.75rem' }}>
          {lines.map((line, index) => (
            <div key={`${line.at}-${index}`} className={line.direction}>
              [{line.direction}] {line.text}
            </div>
          ))}
        </div>
      </section>

      <section className="panel">
        <h2>BLE Raw</h2>
        <p className="muted">
          UUID command: {BleUuids.command}. Для отладки отправки произвольного payload.
        </p>
        <div className="field">
          <label>Hex payload (будет отправлен как команда resetTrip с custom payload)</label>
          <input
            value={rawHex}
            onChange={(e) => setRawHex(e.target.value)}
            placeholder="01 00"
          />
        </div>
        <button
          type="button"
          className="secondary"
          disabled={connectionState.phase !== 'ready'}
          onClick={() => void writeRawGatt()}
        >
          Отправить raw
        </button>
        <div className="serial-log" style={{ marginTop: '0.75rem' }}>
          {gattLog.map((line, index) => (
            <div key={index}>{line}</div>
          ))}
        </div>
      </section>
    </div>
  );
}
