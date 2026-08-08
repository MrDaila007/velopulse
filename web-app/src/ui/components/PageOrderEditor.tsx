import { ConfigField } from './ConfigField';
import {
  PAGE_LABELS,
  TRIP_PAGE_LABELS,
  isPageEnabled,
  isTripPage,
  setPageEnabled,
  syncPageOrderWithMask,
} from '../../domain/pageOrder';

interface PageOrderEditorProps {
  enabledMask: number;
  pageOrder: number[];
  pinnedPage: number;
  onUpdate: (update: {
    enabledMask: number;
    pageOrder: number[];
    pinnedPage: number;
  }) => void;
}

export function PageOrderEditor({
  enabledMask,
  pageOrder,
  pinnedPage,
  onUpdate,
}: PageOrderEditorProps) {
  const togglePage = (page: number, enabled: boolean) => {
    const nextMask = setPageEnabled(enabledMask, page, enabled);
    const nextOrder = syncPageOrderWithMask(pageOrder, nextMask);
    let nextPinned = pinnedPage;
    if (!isPageEnabled(nextMask, nextPinned)) {
      for (let p = 0; p < PAGE_LABELS.length; p++) {
        if (isPageEnabled(nextMask, p)) {
          nextPinned = p;
          break;
        }
      }
    }
    onUpdate({ enabledMask: nextMask, pageOrder: nextOrder, pinnedPage: nextPinned });
  };

  const setPosition = (position: number, page: number) => {
    const next = [...pageOrder];
    const existing = next.indexOf(page);
    if (existing >= 0) {
      next[existing] = next[position];
    }
    next[position] = page;
    onUpdate({ enabledMask, pageOrder: next, pinnedPage });
  };

  return (
    <div className="page-order-editor">
      <ConfigField
        label="Включённые страницы"
        hint="Страницы погоды дублируют Companion в нижней строке карусели (биты 5–6)"
      >
        <div className="page-checkboxes">
          {PAGE_LABELS.map((label, page) => (
            <label key={page} className="checkbox-inline">
              <input
                type="checkbox"
                checked={isPageEnabled(enabledMask, page)}
                onChange={(e) => togglePage(page, e.target.checked)}
              />
              {label}
            </label>
          ))}
        </div>
      </ConfigField>

      <ConfigField label="Порядок страниц поездки" hint="Позиции 1–5; погода/дождь идут после них">
        <div className="page-order-grid">
          {pageOrder.map((page, position) => (
            <label key={position}>
              <span className="muted">#{position + 1}</span>
              <select
                value={isTripPage(page) ? page : position}
                onChange={(e) => setPosition(position, Number(e.target.value))}
              >
                {TRIP_PAGE_LABELS.map((label, option) => (
                  <option key={option} value={option}>
                    {label}
                  </option>
                ))}
              </select>
            </label>
          ))}
        </div>
      </ConfigField>

      <ConfigField label="Закреплённая страница">
        <select
          value={pinnedPage}
          onChange={(e) =>
            onUpdate({
              enabledMask,
              pageOrder,
              pinnedPage: Number(e.target.value),
            })
          }
        >
          {PAGE_LABELS.map((label, page) => (
            <option key={page} value={page} disabled={!isPageEnabled(enabledMask, page)}>
              {label}
            </option>
          ))}
        </select>
      </ConfigField>
    </div>
  );
}
