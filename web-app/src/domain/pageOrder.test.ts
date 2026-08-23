import { describe, expect, it } from 'vitest';
import {
  DISPLAY_PAGE_COUNT,
  PAGE_LABELS,
  VALID_ENABLED_PAGES_MASK,
  clampPinnedPage,
  enabledPageCount,
  isPageEnabled,
  setPageEnabled,
  syncPageOrderWithMask,
} from './pageOrder';

describe('pageOrder', () => {
  it('tracks enabled pages in mask', () => {
    expect(isPageEnabled(0b10101, 0)).toBe(true);
    expect(isPageEnabled(0b10101, 1)).toBe(false);
    expect(setPageEnabled(0b10101, 1, true)).toBe(0b10111);
  });

  it('covers all eight firmware display pages', () => {
    expect(DISPLAY_PAGE_COUNT).toBe(8);
    expect(PAGE_LABELS).toHaveLength(8);
    expect(PAGE_LABELS[0]).toBe('Поездка');
    expect(PAGE_LABELS[7]).toBe('Каденс (CAD)');
    expect(VALID_ENABLED_PAGES_MASK).toBe(0xff);
  });

  it('counts enabled pages including weather and cadence bits', () => {
    expect(enabledPageCount(0b10101)).toBe(3);
    expect(enabledPageCount(0x60)).toBe(2);
    expect(enabledPageCount(0x80)).toBe(1);
    expect(enabledPageCount(0xff)).toBe(8);
  });

  it('syncs trip order when pages disabled', () => {
    const order = syncPageOrderWithMask([0, 1, 2, 3, 4], 0b10011);
    expect(order.slice(0, 3)).toEqual([0, 1, 4]);
    expect(enabledPageCount(0b10011)).toBe(3);
  });

  it('leaves weather and cadence bits out of page_order slots', () => {
    const order = syncPageOrderWithMask([0, 1, 2, 3, 4], 0xff);
    expect(order).toEqual([0, 1, 2, 3, 4]);
  });

  it('clamps pinned page onto an enabled page including cadence', () => {
    expect(clampPinnedPage(7, 0x80)).toBe(7);
    expect(clampPinnedPage(7, 0x01)).toBe(0);
  });
});
