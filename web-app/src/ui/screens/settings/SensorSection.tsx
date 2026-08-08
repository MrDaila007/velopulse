import { ConfigField } from '../../components/ConfigField';
import { FlagToggle } from '../../components/FlagToggle';
import { configFlagHelpers } from '../../../domain/configFlags';
import type { SettingsSectionProps } from './types';
import { patchDraft } from './types';

const EDGE_OPTIONS = [
  { value: 0, label: 'FALLING (по умолчанию)' },
  { value: 1, label: 'RISING' },
  { value: 2, label: 'CHANGE (оба фронта)' },
];

export function SensorSection({ draft, fieldErrors, onChange }: SettingsSectionProps) {
  const flags = configFlagHelpers(draft);

  return (
    <div className="settings-form-grid">
      <ConfigField
        label="Debounce (мс)"
        hint="Задержка после импульса датчика Холла"
        error={fieldErrors.debounceMs}
      >
        <input
          type="number"
          min={0}
          max={50}
          value={draft.debounceMs}
          onChange={(e) => onChange(patchDraft(draft, { debounceMs: Number(e.target.value) }))}
        />
      </ConfigField>

      <ConfigField
        label="Активный фронт"
        hint="Какой переход GPIO считается импульсом"
        error={fieldErrors.activeEdge}
      >
        <select
          value={draft.activeEdge}
          onChange={(e) => onChange(patchDraft(draft, { activeEdge: Number(e.target.value) }))}
        >
          {EDGE_OPTIONS.map((opt) => (
            <option key={opt.value} value={opt.value}>
              {opt.label}
            </option>
          ))}
        </select>
      </ConfigField>

      <FlagToggle
        label="Инвертировать датчик"
        hint="Меняет логику активного уровня"
        checked={flags.sensorInvert}
        onChange={(checked) => onChange(flags.withFlag('sensorInvert', checked))}
      />
    </div>
  );
}
