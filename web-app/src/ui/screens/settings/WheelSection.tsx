import { ConfigField } from '../../components/ConfigField';
import { FlagToggle } from '../../components/FlagToggle';
import { configFlagHelpers } from '../../../domain/configFlags';
import { presetForCircumference, tirePresets } from '../../../domain/tirePresets';
import type { SettingsSectionProps } from './types';
import { patchDraft } from './types';

export function WheelSection({ draft, fieldErrors, onChange }: SettingsSectionProps) {
  const flags = configFlagHelpers(draft);
  const presetId = presetForCircumference(draft.wheelCircumferenceMm);

  return (
    <div className="settings-form-grid">
      <ConfigField label="Покрышка">
        <select
          value={presetId}
          onChange={(e) => {
            const preset = tirePresets.find((p) => p.id === e.target.value);
            if (preset && preset.id !== 'custom') {
              onChange(patchDraft(draft, { wheelCircumferenceMm: preset.circumferenceMm }));
            }
          }}
        >
          {tirePresets.map((p) => (
            <option key={p.id} value={p.id}>
              {p.label}
            </option>
          ))}
        </select>
      </ConfigField>

      <ConfigField
        label="Окружность колеса (мм)"
        error={fieldErrors.wheelCircumferenceMm}
      >
        <input
          type="number"
          min={500}
          max={3000}
          value={draft.wheelCircumferenceMm}
          onChange={(e) =>
            onChange(patchDraft(draft, { wheelCircumferenceMm: Number(e.target.value) }))
          }
        />
      </ConfigField>

      <FlagToggle
        label="Имперские единицы"
        checked={flags.unitsImperial}
        onChange={(checked) => onChange(flags.withFlag('unitsImperial', checked))}
      />

      <ConfigField label="Макс. скорость (км/ч)" error={fieldErrors.maxSpeedKmh}>
        <input
          type="number"
          min={20}
          max={200}
          value={draft.maxSpeedKmh}
          onChange={(e) => onChange(patchDraft(draft, { maxSpeedKmh: Number(e.target.value) }))}
        />
      </ConfigField>

      <ConfigField label="Окно сглаживания" error={fieldErrors.smoothingWindow}>
        <input
          type="number"
          min={2}
          max={5}
          value={draft.smoothingWindow}
          onChange={(e) =>
            onChange(patchDraft(draft, { smoothingWindow: Number(e.target.value) }))
          }
        />
      </ConfigField>

      <FlagToggle
        label="Сглаживание скорости"
        checked={flags.smoothingEnabled}
        onChange={(checked) => onChange(flags.withFlag('smoothingEnabled', checked))}
      />

      <ConfigField label="Таймаут остановки (с)" error={fieldErrors.stopTimeoutS}>
        <input
          type="number"
          min={1}
          max={30}
          value={draft.stopTimeoutS}
          onChange={(e) => onChange(patchDraft(draft, { stopTimeoutS: Number(e.target.value) }))}
        />
      </ConfigField>
    </div>
  );
}
