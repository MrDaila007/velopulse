import { ConfigField } from './ConfigField';

interface FlagToggleProps {
  label: string;
  hint?: string;
  checked: boolean;
  onChange: (checked: boolean) => void;
}

export function FlagToggle({ label, hint, checked, onChange }: FlagToggleProps) {
  return (
    <ConfigField label={label} hint={hint}>
      <label className="checkbox-inline">
        <input type="checkbox" checked={checked} onChange={(e) => onChange(e.target.checked)} />
        {checked ? 'Включено' : 'Выключено'}
      </label>
    </ConfigField>
  );
}
