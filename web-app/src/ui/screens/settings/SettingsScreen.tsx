import { useEffect, useMemo, useState } from 'react';
import { useApp } from '../../../context/AppContext';
import { validateConfig } from '../../../domain/configValidator';
import { configEquals, defaultConfig, deviceInfoFlags, type DeviceConfig } from '../../../domain/types';
import { ConfigDraftStore } from '../../../services/configDraftStore';
import { DirtyBar } from '../../components/DirtyBar';
import { PageHeader } from '../../components/PageHeader';
import { SectionTabs } from '../../components/SectionTabs';
import { CompanionSection } from './CompanionSection';
import { DeviceSection } from './DeviceSection';
import { DisplaySection } from './DisplaySection';
import { PowerSection } from './PowerSection';
import { SensorSection } from './SensorSection';
import { WheelSection } from './WheelSection';

const TABS = [
  { id: 'wheel', label: 'Колесо' },
  { id: 'sensor', label: 'Датчик' },
  { id: 'display', label: 'Дисплей' },
  { id: 'power', label: 'Питание' },
  { id: 'device', label: 'Устройство' },
  { id: 'companion', label: 'Companion' },
] as const;

type TabId = (typeof TABS)[number]['id'];

export function SettingsScreen() {
  const { connectionState, companionSync, preferences } = useApp();
  const draftStore = useMemo(() => new ConfigDraftStore(), []);
  const [activeTab, setActiveTab] = useState<TabId>('wheel');
  const [draft, setDraft] = useState<DeviceConfig>(defaultConfig());
  const [deviceConfig, setDeviceConfig] = useState<DeviceConfig | null>(null);
  const [conflictConfig, setConflictConfig] = useState<DeviceConfig | null>(null);
  const [saving, setSaving] = useState(false);
  const [pulling, setPulling] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);
  const [syncMessage, setSyncMessage] = useState<string | null>(null);
  const [companionPrefs, setCompanionPrefs] = useState(preferences.readCompanionPreferences());

  const deviceId = connectionState.deviceId;

  useEffect(() => {
    if (!connectionState.config) return;
    const incoming = connectionState.config;

    if (deviceConfig && !configEquals(incoming, deviceConfig)) {
      const localDirty = draftStore.hasLocalChanges(draft, deviceConfig);
      if (localDirty) {
        setConflictConfig(incoming);
        return;
      }
    }

    setDeviceConfig(incoming);
    setConflictConfig(null);

    if (deviceId) {
      const stored = draftStore.read(deviceId);
      if (stored && draftStore.hasLocalChanges(stored.config, incoming)) {
        setDraft(stored.config);
        return;
      }
    }
    setDraft(incoming);
  }, [connectionState.config, deviceId, draftStore]);

  useEffect(() => {
    if (!deviceId) return;
    if (draftStore.hasLocalChanges(draft, deviceConfig)) {
      draftStore.write(deviceId, draft);
    } else {
      draftStore.clear(deviceId);
    }
  }, [draft, deviceId, deviceConfig, draftStore]);

  if (connectionState.phase !== 'ready') {
    return (
      <>
        <PageHeader
          title="Настройки"
          description="Конфигурация BLE и параметры Companion."
        />
        <section className="panel">
          <p className="muted">Требуется подключение к устройству.</p>
        </section>
      </>
    );
  }

  const validation = validateConfig(draft);
  const dirty = draftStore.hasLocalChanges(draft, deviceConfig);
  const deepSleepSupported = connectionState.deviceInfo
    ? deviceInfoFlags(connectionState.deviceInfo).deepSleepSupported
    : false;

  const updateDraft = (next: DeviceConfig) => {
    setDraft(next);
    setSaveError(null);
    setSyncMessage(null);
  };

  const syncing = saving || pulling;

  const pullFromDevice = async () => {
    if (
      dirty &&
      !window.confirm('Несохранённые изменения будут заменены конфигурацией с устройства. Продолжить?')
    ) {
      return;
    }
    setPulling(true);
    setSaveError(null);
    setSyncMessage(null);
    const result = await connectionState.repository!.readConfig();
    setPulling(false);
    if (!result.ok) {
      setSaveError(result.error.message);
      return;
    }
    setDraft(result.value);
    setDeviceConfig(result.value);
    setConflictConfig(null);
    if (deviceId) draftStore.clear(deviceId);
    setSyncMessage('Конфигурация загружена с устройства');
  };

  const save = async () => {
    if (!validation.isValid) {
      setSaveError(validation.issues[0]?.message ?? 'Ошибка валидации');
      return;
    }
    setSaving(true);
    setSaveError(null);
    const result = await connectionState.repository!.writeConfig(draft);
    setSaving(false);
    if (!result.ok) {
      setSaveError(result.error.message);
      return;
    }
    setDeviceConfig(draft);
    if (deviceId) draftStore.clear(deviceId);
    setConflictConfig(null);
    setSyncMessage('Конфигурация записана на устройство');
  };

  const resetDraft = () => {
    if (deviceConfig) setDraft(deviceConfig);
    setSaveError(null);
    if (deviceId) draftStore.clear(deviceId);
  };

  const applyDeviceConfig = () => {
    if (conflictConfig) {
      setDraft(conflictConfig);
      setDeviceConfig(conflictConfig);
      setConflictConfig(null);
      if (deviceId) draftStore.clear(deviceId);
    }
  };

  const sectionProps = {
    draft,
    fieldErrors: validation.fieldErrors,
    deepSleepSupported,
    onChange: updateDraft,
  };

  return (
    <div className="settings-shell">
      <PageHeader
        title="Настройки"
        description="Конфигурация устройства по секциям. Загрузите с устройства или синхронизируйте черновик на велокомпьютер."
        actions={
          activeTab !== 'companion' ? (
            <button
              type="button"
              className="secondary btn-sm"
              disabled={syncing}
              onClick={() => void pullFromDevice()}
            >
              Обновить с устройства
            </button>
          ) : undefined
        }
      />

      {conflictConfig && (
        <div className="conflict-banner">
          <span>На устройстве новая конфигурация. Применить или оставить черновик?</span>
          <div className="btn-row" style={{ marginTop: 0 }}>
            <button type="button" className="secondary" onClick={() => setConflictConfig(null)}>
              Оставить черновик
            </button>
            <button type="button" className="primary" onClick={applyDeviceConfig}>
              Применить с устройства
            </button>
          </div>
        </div>
      )}

      <section className="panel">
        <SectionTabs
          tabs={[...TABS]}
          active={activeTab}
          onChange={(id: string) => setActiveTab(id as TabId)}
        />

        {activeTab === 'wheel' && (
          <>
            <h3>Колесо и скорость</h3>
            <p className="muted">Окружность, единицы, сглаживание и остановка.</p>
            <WheelSection {...sectionProps} />
          </>
        )}
        {activeTab === 'sensor' && (
          <>
            <h3>Датчик Холла</h3>
            <p className="muted">Debounce, фронт и инверсия сигнала.</p>
            <SensorSection {...sectionProps} />
          </>
        )}
        {activeTab === 'display' && (
          <>
            <h3>Дисплей</h3>
            <p className="muted">Яркость, таймауты и страницы OLED.</p>
            <DisplaySection {...sectionProps} />
          </>
        )}
        {activeTab === 'power' && (
          <>
            <h3>Питание</h3>
            <p className="muted">АКБ, сон и сохранение одометра.</p>
            <PowerSection {...sectionProps} />
          </>
        )}
        {activeTab === 'device' && (
          <>
            <h3>Устройство</h3>
            <p className="muted">Имя в BLE-рекламе.</p>
            <DeviceSection {...sectionProps} />
          </>
        )}
        {activeTab === 'companion' && (
          <>
            <h3>Companion</h3>
            <p className="muted">Часы и погода на OLED (не входит в BLE Config).</p>
            <CompanionSection
              companionSupported={connectionState.companionSupported}
              preferences={companionPrefs}
              onChange={setCompanionPrefs}
              onSave={async () => {
                preferences.writeCompanionPreferences(companionPrefs);
                await companionSync.sync();
              }}
            />
          </>
        )}
      </section>

      {activeTab !== 'companion' && (
        <DirtyBar
          dirty={dirty}
          syncing={syncing}
          error={saveError}
          statusMessage={syncMessage}
          saveDisabled={!validation.isValid}
          onSave={() => void save()}
          onPullFromDevice={() => void pullFromDevice()}
          onReset={resetDraft}
        />
      )}
    </div>
  );
}
