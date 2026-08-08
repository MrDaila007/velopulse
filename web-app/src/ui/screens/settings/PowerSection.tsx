import { ConfigField } from '../../components/ConfigField';
import { FlagToggle } from '../../components/FlagToggle';
import { RangeSlider } from '../../components/RangeSlider';
import { configFlagHelpers } from '../../../domain/configFlags';
import type { SettingsSectionProps } from './types';
import { patchDraft } from './types';

export function PowerSection({ draft, fieldErrors, deepSleepSupported, onChange }: SettingsSectionProps) {
  const flags = configFlagHelpers(draft);

  return (
    <div className="settings-form-grid two-col">
      <div>
        <RangeSlider
          label="Низкий заряд АКБ"
          value={draft.lowBatteryPct}
          min={5}
          max={50}
          unit="%"
          onChange={(value) => onChange(patchDraft(draft, { lowBatteryPct: value }))}
        />
        {fieldErrors.lowBatteryPct ? (
          <span className="field-error">{fieldErrors.lowBatteryPct}</span>
        ) : null}

        <FlagToggle
          label="Режим энергосбережения"
          checked={flags.powerSaveMode}
          onChange={(checked) => onChange(flags.withFlag('powerSaveMode', checked))}
        />

        <FlagToggle
          label="Всегда рекламировать BLE"
          hint="Устройство остаётся видимым даже в режиме сна"
          checked={flags.bleAlwaysAdvertise}
          onChange={(checked) => onChange(flags.withFlag('bleAlwaysAdvertise', checked))}
        />

        <ConfigField
          label="Интервал сохранения одометра (м)"
          error={fieldErrors.odometerSaveIntervalM}
        >
          <input
            type="number"
            min={100}
            max={5000}
            value={draft.odometerSaveIntervalM}
            onChange={(e) =>
              onChange(patchDraft(draft, { odometerSaveIntervalM: Number(e.target.value) }))
            }
          />
        </ConfigField>
      </div>

      <div>
        {deepSleepSupported && (
          <>
            <FlagToggle
              label="Глубокий сон"
              checked={flags.deepSleepEnabled}
              onChange={(checked) => onChange(flags.withFlag('deepSleepEnabled', checked))}
            />

            <ConfigField label="Таймаут глубокого сна (с)" error={fieldErrors.deepSleepTimeoutS}>
              <input
                type="number"
                min={60}
                max={3600}
                value={draft.deepSleepTimeoutS}
                onChange={(e) =>
                  onChange(patchDraft(draft, { deepSleepTimeoutS: Number(e.target.value) }))
                }
              />
            </ConfigField>
          </>
        )}

        <details className="advanced">
          <summary>Расширенные: калибровка АКБ</summary>
          <ConfigField label="Масштаб (‰)" error={fieldErrors.battCalScalePermille}>
            <input
              type="number"
              min={800}
              max={1200}
              value={draft.battCalScalePermille}
              onChange={(e) =>
                onChange(patchDraft(draft, { battCalScalePermille: Number(e.target.value) }))
              }
            />
          </ConfigField>
          <ConfigField label="Смещение (мВ)" error={fieldErrors.battCalOffsetMv}>
            <input
              type="number"
              min={-500}
              max={500}
              value={draft.battCalOffsetMv}
              onChange={(e) =>
                onChange(patchDraft(draft, { battCalOffsetMv: Number(e.target.value) }))
              }
            />
          </ConfigField>
        </details>
      </div>
    </div>
  );
}
