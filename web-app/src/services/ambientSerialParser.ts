export interface AmbientSample {
  enabled: number;
  valid: number;
  raw: number;
  filtered: number;
  autoPct: number;
  effectivePct: number;
}

const AMBIENT_RE =
  /Ambient: enabled=(\d), valid=(\d), raw=(\d+), filtered=(\d+), auto_pct=(\d+), effective_pct=(\d+)/;

export function parseAmbientLine(text: string): AmbientSample | null {
  const match = AMBIENT_RE.exec(text);
  if (!match) return null;
  return {
    enabled: Number(match[1]),
    valid: Number(match[2]),
    raw: Number(match[3]),
    filtered: Number(match[4]),
    autoPct: Number(match[5]),
    effectivePct: Number(match[6]),
  };
}

export function parseAmbientChunk(text: string): AmbientSample[] {
  const samples: AmbientSample[] = [];
  for (const line of text.split(/\r?\n/)) {
    const sample = parseAmbientLine(line);
    if (sample) samples.push(sample);
  }
  return samples;
}
