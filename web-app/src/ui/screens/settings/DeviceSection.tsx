import { ConfigField } from '../../components/ConfigField';
import { InfoCallout } from '../../components/InfoCallout';
import type { SettingsSectionProps } from './types';
import { patchDraft } from './types';

export function DeviceSection({ draft, fieldErrors, onChange }: SettingsSectionProps) {
  return (
    <>
      <InfoCallout tone="warn">
        Новое имя BLE вступит в силу после перезапуска рекламы (перезагрузка устройства или
        смена режима сна).
      </InfoCallout>

      <ConfigField
        label="Имя устройства (BLE)"
        hint="3–15 символов: A–Z, a–z, 0–9, пробел, - или _"
        error={fieldErrors.deviceName}
      >
        <input
          type="text"
          maxLength={15}
          value={draft.deviceName}
          onChange={(e) => onChange(patchDraft(draft, { deviceName: e.target.value }))}
        />
      </ConfigField>
    </>
  );
}
