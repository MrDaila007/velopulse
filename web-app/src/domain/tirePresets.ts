export interface TirePreset {
  id: string;
  label: string;
  circumferenceMm: number;
}

export const tirePresets: TirePreset[] = [
  { id: '700x23', label: '700×23', circumferenceMm: 2096 },
  { id: '700x25', label: '700×25', circumferenceMm: 2105 },
  { id: '700x28', label: '700×28', circumferenceMm: 2136 },
  { id: '700x32', label: '700×32', circumferenceMm: 2155 },
  { id: '700x35', label: '700×35', circumferenceMm: 2168 },
  { id: '700x38', label: '700×38', circumferenceMm: 2180 },
  { id: '29x2.0', label: '29×2.0', circumferenceMm: 2288 },
  { id: '27.5x2.1', label: '27.5×2.1', circumferenceMm: 2148 },
  { id: 'custom', label: 'Свой размер', circumferenceMm: 2100 },
];

export function presetForCircumference(mm: number): string {
  const match = tirePresets.find((p) => p.id !== 'custom' && p.circumferenceMm === mm);
  return match?.id ?? 'custom';
}
