import type { ReactNode } from 'react';

interface ConfigFieldProps {
  label: string;
  hint?: string;
  error?: string;
  children: ReactNode;
}

export function ConfigField({ label, hint, error, children }: ConfigFieldProps) {
  return (
    <div className={`field config-field${error ? ' has-error' : ''}`}>
      <label>{label}</label>
      {children}
      {hint ? <span className="field-hint muted">{hint}</span> : null}
      {error ? <span className="field-error">{error}</span> : null}
    </div>
  );
}
