import { useApp } from '../../context/AppContext';
import {
  formatBattery,
  formatDistanceCm,
  formatDistanceMeters,
  formatDuration,
  formatSpeed,
} from '../../domain/unitFormatter';
import { buildSafeCommand, configFlagHelpers, telemetryFlags } from '../../domain/types';
import { PageHeader } from '../components/PageHeader';

export function DashboardScreen() {
  const { connectionState } = useApp();
  const { telemetry, config, repository, phase } = connectionState;

  if (phase !== 'ready' || !telemetry) {
    return (
      <>
        <PageHeader
          title="Панель"
          description="Телеметрия в реальном времени после подключения к устройству."
        />
        <section className="panel">
          <p className="muted">Подключите устройство в разделе «Скан».</p>
        </section>
      </>
    );
  }

  const imperial = config ? configFlagHelpers(config).unitsImperial : false;
  const flags = telemetryFlags(telemetry);

  const send = async (id: Parameters<typeof buildSafeCommand>[0]) => {
    if (!repository) return;
    await repository.sendCommand(buildSafeCommand(id));
  };

  return (
    <div className="stack">
      <PageHeader title="Панель" description="Скорость, поездка и быстрые команды." />

      <section className="panel panel-hero dashboard-hero">
        <span className="hero-label">Текущая скорость</span>
        <div className="hero-speed">{formatSpeed(telemetry.speedX100, imperial)}</div>
        <div className="chips">
          <span className={`chip ${telemetry.rideState === 'moving' ? 'ok' : ''}`}>
            {telemetry.rideState}
          </span>
          <span className="chip">{telemetry.sensorState}</span>
          <span className={`chip ${flags.lowBattery ? 'warn' : ''}`}>
            {flags.charging ? 'зарядка' : 'АКБ'}
          </span>
          <span className="chip">{flags.displayOn ? 'дисплей вкл' : 'дисплей выкл'}</span>
          <span className={`chip ${flags.cscSpeedSource ? 'ok' : ''}`}>
            {flags.cscSpeedSource ? 'S3 скорость' : 'Hall скорость'}
          </span>
        </div>
      </section>

      <section className="panel">
        <h3>Поездка</h3>
        <div className="grid-2">
          <div className="stat">
            <label>Дистанция</label>
            <strong>{formatDistanceCm(telemetry.tripDistanceCm, imperial)}</strong>
          </div>
          <div className="stat">
            <label>Средняя</label>
            <strong>{formatSpeed(telemetry.avgSpeedX100, imperial)}</strong>
          </div>
          <div className="stat">
            <label>Максимум</label>
            <strong>{formatSpeed(telemetry.maxSpeedX100, imperial)}</strong>
          </div>
          <div className="stat">
            <label>В движении</label>
            <strong>{formatDuration(telemetry.movingTimeS)}</strong>
          </div>
          <div className="stat">
            <label>Одометр</label>
            <strong>{formatDistanceMeters(telemetry.odometerM, imperial)}</strong>
          </div>
          <div className="stat">
            <label>АКБ</label>
            <strong>{formatBattery(telemetry.batteryMv, telemetry.batteryPct)}</strong>
          </div>
          <div className="stat">
            <label>Каденс</label>
            <strong>
              {flags.cadenceValid ? `${(telemetry.cadenceX10 / 10).toFixed(1)} об/мин` : '—'}
            </strong>
          </div>
        </div>
      </section>

      <section className="panel">
        <h3>BLE-датчик CSC</h3>
        <div className="chips">
          <span className={`chip ${flags.cscConnected ? 'ok' : ''}`}>
            {flags.cscConnected ? 'подключён' : 'нет связи'}
          </span>
          {flags.cscPairing && <span className="chip">поиск</span>}
          {flags.cscCrankPresent && <span className="chip ok">C3 каденс</span>}
          {(flags.cscWheelPresent || flags.cscSpeedSource) && (
            <span className={`chip ${flags.cscSpeedSource ? 'ok' : ''}`}>
              {flags.cscSpeedSource ? 'S3 скорость' : 'S3 колесо'}
            </span>
          )}
          <span className={`chip ${flags.cscSpeedSource ? 'ok' : ''}`}>
            источник скорости: {flags.cscSpeedSource ? 'S3' : 'Hall'}
          </span>
        </div>
        <p className="muted" style={{ marginTop: 8 }}>
          Сопряжение датчика — на устройстве: USB-команды <code>csc-pair</code> /{' '}
          <code>csc-forget</code> во вкладке «Отладка», либо кнопка на датчике.
        </p>
      </section>

      <section className="panel">
        <div className="btn-row">
          <button
            type="button"
            className="secondary"
            onClick={() => {
              if (window.confirm('Сбросить поездку?')) void send('resetTrip');
            }}
          >
            Сброс поездки
          </button>
          <button type="button" className="secondary" onClick={() => void send('displayOn')}>
            Дисплей вкл
          </button>
          <button type="button" className="secondary" onClick={() => void send('displayOff')}>
            Дисплей выкл
          </button>
          <button type="button" className="secondary" onClick={() => repository?.readTelemetry()}>
            Обновить
          </button>
        </div>
      </section>
    </div>
  );
}
