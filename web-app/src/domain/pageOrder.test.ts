import { describe, expect, it } from 'vitest';
import {
  DISPLAY_PAGE_COUNT,
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

  it('counts enabled pages including weather bits', () => {
    expect(enabledPageCount(0b10101)).toBe(3);
    expect(enabledPageCount(0x60)).toBe(2);
    expect(DISPLAY_PAGE_COUNT).toBe(7);
  });

  it('syncs trip order when pages disabled', () => {
    const order = syncPageOrderWithMask([0, 1, 2, 3, 4], 0b10011);
    expect(order.slice(0, 3)).toEqual([0, 1, 4]);
    expect(enabledPageCount(0b10011)).toBe(3);
  });

  it('leaves weather bits out of page_order slots', () => {
    const order = syncPageOrderWithMask([0, 1, 2, 3, 4], 0x7f);
    expect(order).toEqual([0, 1, 2, 3, 4]);
  });
});
