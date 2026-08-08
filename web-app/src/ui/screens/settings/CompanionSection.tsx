import { useState } from 'react';
import { ConfigField } from '../../components/ConfigField';
import { weatherCities } from '../../../services/weatherCities';
import type { CompanionPreferences } from '../../../domain/types';

interface CompanionSectionProps {
  companionSupported: boolean;
  preferences: CompanionPreferences;
  onChange: (prefs: CompanionPreferences) => void;
  onSave: () => Promise<void>;
}

export function CompanionSection({
  companionSupported,
  preferences,
  onChange,
  onSave,
}: CompanionSectionProps) {
  const [saving, setSaving] = useState(false);

  return (
    <>
      {!companionSupported && (
        <p className="chip warn">Характеристика Companion Write не найдена на устройстве.</p>
      )}

      <ConfigField label="Часы на OLED">
        <label className="checkbox-inline">
          <input
            type="checkbox"
            checked={preferences.showClockOnDevice}
            onChange={(e) => onChange({ ...preferences, showClockOnDevice: e.target.checked })}
          />
          Показывать часы
        </label>
      </ConfigField>

      <ConfigField label="Погода на OLED">
        <label className="checkbox-inline">
          <input
            type="checkbox"
            checked={preferences.showWeatherOnDevice}
            onChange={(e) => onChange({ ...preferences, showWeatherOnDevice: e.target.checked })}
          />
          Показывать погоду
        </label>
      </ConfigField>

      <ConfigField label="Город для погоды">
        <select
          value={preferences.weatherCityId}
          onChange={(e) => onChange({ ...preferences, weatherCityId: e.target.value })}
        >
          {weatherCities.map((c) => (
            <option key={c.id} value={c.id}>
              {c.name}
            </option>
          ))}
        </select>
      </ConfigField>

      <button
        type="button"
        className="secondary"
        disabled={saving}
        onClick={() => {
          setSaving(true);
          void onSave().finally(() => setSaving(false));
        }}
      >
        {saving ? 'Синхронизация…' : 'Сохранить и синхронизировать'}
      </button>
    </>
  );
}
