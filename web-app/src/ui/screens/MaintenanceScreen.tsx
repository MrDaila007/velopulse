import { useState } from 'react';
import { useApp } from '../../context/AppContext';
import { buildSafeCommand } from '../../domain/types';
import { buildSessionLog, downloadJson } from '../../services/rideLogRecorder';

import { PageHeader } from '../components/PageHeader';

export function MaintenanceScreen() {
  const { connectionState, migrationStore, rideLog } = useApp();
  const [sensorTestLeft, setSensorTestLeft] = useState<number | null>(null);
  const [message, setMessage] = useState<string | null>(null);
  const repo = connectionState.repository;

  if (!repo || connectionState.phase !== 'ready') {
    return (
      <>
        <PageHeader title="Сервис" description="Диагностика, бэкап и сервисные команды." />
        <section className="panel">
          <p className="muted">Требуется подключение к устройству.</p>
        </section>
      </>
    );
  }

  const runCommand = async (label: string, fn: () => Promise<unknown>) => {
    try {
      const result = await fn();
      if (result && typeof result === 'object' && 'ok' in result) {
        const r = result as { ok: boolean; error?: { message: string } };
        setMessage(r.ok ? `${label}: OK` : `${label}: ${r.error?.message ?? 'ошибка'}`);
      } else {
        setMessage(`${label}: OK`);
      }
    } catch (error) {
      setMessage(`${label}: ${error instanceof Error ? error.message : String(error)}`);
    }
  };

  const startSensorTest = async () => {
    setSensorTestLeft(60);
    await repo.sendCommand(buildSafeCommand('sensorTestStart'));
    const timer = setInterval(() => {
      setSensorTestLeft((prev) => {
        if (prev === null || prev <= 1) {
          clearInterval(timer);
          void repo.sendCommand(buildSafeCommand('sensorTestStop'));
          return null;
        }
        return prev - 1;
      });
    }, 1000);
  };

  const backup = async () => {
    if (!connectionState.config || !connectionState.deviceInfo) return;
    migrationStore.writeBackup({
      deviceId: connectionState.deviceId ?? 'unknown',
      deviceName: connectionState.deviceName ?? 'BikeComp',
      config: connectionState.config,
      odometerM: connectionState.telemetry?.odometerM ?? 0,
      fwVersion: connectionState.deviceInfo.fwVersion,
      savedAt: new Date().toISOString(),
    });
    setMessage('Бэкап сохранён локально');
  };

  const restore = async () => {
    const saved = migrationStore.readBackup();
    if (!saved) {
      setMessage('Бэкап не найден');
      return;
    }
    if (!window.confirm('Восстановить конфигурацию и одометр?')) return;
    const configResult = await repo.writeConfig(saved.config);
    if (!configResult.ok) {
      setMessage(configResult.error.message);
      return;
    }
    const odoResult = await repo.setOdometerMeters(saved.odometerM);
    setMessage(odoResult.ok ? 'Восстановление завершено' : odoResult.error.message);
  };

  const exportLog = async () => {
    const diagnostic = await repo.getDiagnostic();
    const errorLog = await repo.readErrorLog();
    const log = buildSessionLog({
      deviceId: connectionState.deviceId,
      deviceName: connectionState.deviceName,
      deviceInfo: connectionState.deviceInfo,
      config: connectionState.config,
      diagnostic: diagnostic.ok ? diagnostic.value : null,
      errorLog: errorLog.ok ? errorLog.value : null,
      samples: rideLog.getSamples(),
    });
    downloadJson(`bikecomp-session-${Date.now()}.json`, log);
    setMessage('Журнал экспортирован');
  };

  return (
    <div className="stack">
      <PageHeader title="Сервис" description="Команды обслуживания, бэкап и экспорт журнала." />

      <section className="panel">
        <h3>Команды</h3>
        <div className="btn-row">
          <button
            type="button"
            className="secondary"
            onClick={() => void runCommand('Force save', () => repo.sendCommand(buildSafeCommand('forceSave')))}
          >
            Force save
          </button>
          <button type="button" className="secondary" onClick={() => void startSensorTest()}>
            Sensor test {sensorTestLeft !== null ? `(${sensorTestLeft}s)` : ''}
          </button>
          <button
            type="button"
            className="secondary"
            onClick={() =>
              void runCommand('Display test', () =>
                repo.sendCommand(buildSafeCommand('displayTest', 'checkerboard')),
              )
            }
          >
            Display test
          </button>
          <button
            type="button"
            className="danger"
            onClick={() => {
              if (window.confirm('Перезагрузить устройство?')) {
                void runCommand('Reboot', () => repo.rebootDevice());
              }
            }}
          >
            Reboot
          </button>
        </div>
      </section>

      <section className="panel">
        <h3>Бэкап / восстановление</h3>
        <div className="btn-row">
          <button type="button" className="secondary" onClick={() => void backup()}>
            Сохранить бэкап
          </button>
          <button type="button" className="secondary" onClick={() => void restore()}>
            Восстановить бэкап
          </button>
          <button type="button" className="secondary" onClick={() => void exportLog()}>
            Экспорт журнала сессии
          </button>
        </div>
        {migrationStore.readBackup() && (
          <p className="muted">
            Последний бэкап: {migrationStore.readBackup()!.savedAt} ·{' '}
            {migrationStore.readBackup()!.odometerM} m
          </p>
        )}
      </section>

      {connectionState.telemetry && (
        <section className="panel">
          <h3>Диагностика датчика</h3>
          <div className="grid-2">
            <div className="stat">
              <label>Об/мин (rev)</label>
              <strong>{connectionState.telemetry.revolutions}</strong>
            </div>
            <div className="stat">
              <label>Последний импульс</label>
              <strong>{connectionState.telemetry.lastPulseAgeMs} ms</strong>
            </div>
            <div className="stat">
              <label>Состояние</label>
              <strong>{connectionState.telemetry.sensorState}</strong>
            </div>
          </div>
        </section>
      )}

      {message && <p className="muted">{message}</p>}
    </div>
  );
}
