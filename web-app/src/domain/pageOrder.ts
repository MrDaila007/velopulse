/** Trip metrics pages (ordered via `page_order[5]`). Matches firmware DisplayPage 0–4. */
export const TRIP_PAGE_LABELS = [
  'Поездка',
  'Средняя',
  'Максимум',
  'Время',
  'Одометр',
] as const;

/** Companion carousel pages (`enabled_pages_mask` bits 5–6). */
export const WEATHER_PAGE_LABELS = ['Погода и часы', 'Дождь'] as const;

/** Cadence page (`enabled_pages_mask` bit 7). */
export const CADENCE_PAGE_LABEL = 'Каденс (CAD)';

export const TRIP_PAGE_COUNT = TRIP_PAGE_LABELS.length;
export const DISPLAY_PAGE_COUNT = 8;
export const VALID_ENABLED_PAGES_MASK = 0xff;

export const PAGE_LABELS = [
  ...TRIP_PAGE_LABELS,
  ...WEATHER_PAGE_LABELS,
  CADENCE_PAGE_LABEL,
] as const;

export function pageLabel(page: number): string {
  if (page >= 0 && page < PAGE_LABELS.length) return PAGE_LABELS[page]!;
  return `Стр. ${page}`;
}

export function isTripPage(page: number): boolean {
  return page >= 0 && page < TRIP_PAGE_COUNT;
}

export function isPageEnabled(mask: number, page: number): boolean {
  return (mask & (1 << page)) !== 0;
}

export function setPageEnabled(mask: number, page: number, enabled: boolean): number {
  return enabled ? mask | (1 << page) : mask & ~(1 << page);
}

export function enabledPageCount(mask: number): number {
  let count = 0;
  for (let page = 0; page < DISPLAY_PAGE_COUNT; page++) {
    if (isPageEnabled(mask, page)) count++;
  }
  return count;
}

export function syncPageOrderWithMask(pageOrder: number[], mask: number): number[] {
  const tripPages = Array.from({ length: TRIP_PAGE_COUNT }, (_, page) => page);
  const enabled = pageOrder.filter((page) => isTripPage(page) && isPageEnabled(mask, page));
  const disabled = tripPages.filter((page) => !isPageEnabled(mask, page));
  return [...enabled, ...disabled].slice(0, TRIP_PAGE_COUNT);
}

export function clampPinnedPage(pinned: number, mask: number): number {
  if (isPageEnabled(mask, pinned)) return pinned;
  for (let page = 0; page < DISPLAY_PAGE_COUNT; page++) {
    if (isPageEnabled(mask, page)) return page;
  }
  return 0;
}
