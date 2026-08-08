import { ConfigField } from '../../components/ConfigField';
import { FlagToggle } from '../../components/FlagToggle';
import { InfoCallout } from '../../components/InfoCallout';
import { PageOrderEditor } from '../../components/PageOrderEditor';
import { RangeSlider } from '../../components/RangeSlider';
import { configFlagHelpers } from '../../../domain/configFlags';
import type { SettingsSectionProps } from './types';
import { patchDraft } from './types';

export function DisplaySection({ draft, fieldErrors, onChange }: SettingsSectionProps) {
  const flags = configFlagHelpers(draft);

  return (
    <>
      <InfoCallout tone="info">
        Яркость задаёт <strong>максимум</strong> для авто-режима по фотодиоду (LDR). Пороги
        калибровки LDR настраиваются через USB или{' '}
        <code>tools/ldr_calibrate.py</code>, не через BLE.
      </InfoCallout>

      <div className="settings-form-grid two-col">
        <div>
          <RangeSlider
            label="Яркость дисплея"
            value={draft.brightnessPct}
            min={1}
            max={100}
            unit="%"
            onChange={(value) => onChange(patchDraft(draft, { brightnessPct: value }))}
          />
          {fieldErrors.brightnessPct ? (
            <span className="field-error">{fieldErrors.brightnessPct}</span>
          ) : null}

          <ConfigField label="Таймаут дисплея (с)" error={fieldErrors.displayTimeoutS}>
            <input
              type="number"
              min={0}
              max={600}
              value={draft.displayTimeoutS}
              onChange={(e) =>
                onChange(patchDraft(draft, { displayTimeoutS: Number(e.target.value) }))
              }
            />
          </ConfigField>

          <FlagToggle
            label="Автоотключение дисплея"
            hint="0 с = никогда, если автоотключение выключено"
            checked={flags.displayAutoOff}
            onChange={(checked) => onChange(flags.withFlag('displayAutoOff', checked))}
          />
        </div>

        <div>
          <ConfigField label="Период смены страниц (с)" error={fieldErrors.pageSwitchPeriodS}>
            <input
              type="number"
              min={1}
              max={60}
              value={draft.pageSwitchPeriodS}
              onChange={(e) =>
                onChange(patchDraft(draft, { pageSwitchPeriodS: Number(e.target.value) }))
              }
            />
          </ConfigField>

          <FlagToggle
            label="Автосмена страниц"
            checked={flags.autoPageSwitch}
            onChange={(checked) => onChange(flags.withFlag('autoPageSwitch', checked))}
          />
        </div>
      </div>

      <InfoCallout tone="info">
        Страницы <strong>Погода и часы</strong> и <strong>Дождь</strong> — отдельные экраны карусели
        (биты 5–6 в <code>enabled_pages_mask</code>). Данные на них появятся после синхронизации
        Companion (вкладка Companion → часы и погода).
      </InfoCallout>

      <PageOrderEditor
        enabledMask={draft.enabledPagesMask}
        pageOrder={draft.pageOrder}
        pinnedPage={draft.pinnedPage}
        onUpdate={({ enabledMask, pageOrder, pinnedPage }) =>
          onChange(
            patchDraft(draft, {
              enabledPagesMask: enabledMask,
              pageOrder,
              pinnedPage,
            }),
          )
        }
      />
      {fieldErrors.enabledPagesMask ? (
        <span className="field-error">{fieldErrors.enabledPagesMask}</span>
      ) : null}
      {fieldErrors.pageOrder ? <span className="field-error">{fieldErrors.pageOrder}</span> : null}
      {fieldErrors.pinnedPage ? <span className="field-error">{fieldErrors.pinnedPage}</span> : null}
    </>
  );
}
