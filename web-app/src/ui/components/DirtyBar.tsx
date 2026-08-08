interface DirtyBarProps {
  dirty: boolean;
  syncing: boolean;
  error?: string | null;
  statusMessage?: string | null;
  onSave: () => void;
  onPullFromDevice: () => void;
  onReset: () => void;
  saveDisabled?: boolean;
}

export function DirtyBar({
  dirty,
  syncing,
  error,
  statusMessage,
  onSave,
  onPullFromDevice,
  onReset,
  saveDisabled,
}: DirtyBarProps) {
  return (
    <div className="dirty-bar">
      <div className="dirty-bar-status">
        {error ? (
          <span className="dirty-bar-error">{error}</span>
        ) : statusMessage ? (
          <span className="dirty-bar-ok">{statusMessage}</span>
        ) : dirty ? (
          <span className="dirty-bar-dirty">Есть несохранённые изменения</span>
        ) : (
          <span className="muted">Синхронизировано с устройством</span>
        )}
      </div>
      <div className="btn-row dirty-bar-actions">
        <button
          type="button"
          className="secondary"
          disabled={syncing}
          onClick={onPullFromDevice}
          title="Прочитать конфигурацию по BLE и обновить форму"
        >
          {syncing ? 'Синхронизация…' : 'Загрузить с устройства'}
        </button>
        <button type="button" className="secondary" disabled={!dirty || syncing} onClick={onReset}>
          Сбросить черновик
        </button>
        <button
          type="button"
          className="primary"
          disabled={saveDisabled || syncing || !dirty}
          onClick={onSave}
          title="Записать черновик на устройство (write-then-verify)"
        >
          Синхронизировать на устройство
        </button>
      </div>
    </div>
  );
}
