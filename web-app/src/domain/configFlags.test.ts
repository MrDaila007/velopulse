import { describe, expect, it } from 'vitest';
import { ConfigFlagMask, readConfigFlags, withConfigFlag } from './configFlags';
import { defaultConfig } from './types';

describe('configFlags', () => {
  it('reads all flag bits', () => {
    const flags = readConfigFlags(0b11111111);
    expect(flags.smoothingEnabled).toBe(true);
    expect(flags.autoPageSwitch).toBe(true);
    expect(flags.displayAutoOff).toBe(true);
    expect(flags.bleAlwaysAdvertise).toBe(true);
    expect(flags.unitsImperial).toBe(true);
    expect(flags.sensorInvert).toBe(true);
    expect(flags.powerSaveMode).toBe(true);
    expect(flags.deepSleepEnabled).toBe(true);
  });

  it('toggles a single flag', () => {
    const base = defaultConfig();
    const next = withConfigFlag(base, 'unitsImperial', true);
    expect(next.flags & ConfigFlagMask.unitsImperial).toBe(ConfigFlagMask.unitsImperial);
    const cleared = withConfigFlag(next, 'unitsImperial', false);
    expect(cleared.flags & ConfigFlagMask.unitsImperial).toBe(0);
  });
});
