import { describe, expect, it } from 'vitest';
import { parseAmbientChunk, parseAmbientLine } from './ambientSerialParser';

describe('ambientSerialParser', () => {
  const sample =
    'Ambient: enabled=1, valid=1, raw=512, filtered=480, auto_pct=42, effective_pct=38';

  it('parses a single ambient line', () => {
    const parsed = parseAmbientLine(sample);
    expect(parsed).toEqual({
      enabled: 1,
      valid: 1,
      raw: 512,
      filtered: 480,
      autoPct: 42,
      effectivePct: 38,
    });
  });

  it('ignores unrelated serial output', () => {
    expect(parseAmbientLine('status: ok')).toBeNull();
  });

  it('parses multiline chunks', () => {
    const chunk = `${sample}\nnoise\n${sample}`;
    expect(parseAmbientChunk(chunk)).toHaveLength(2);
  });
});
