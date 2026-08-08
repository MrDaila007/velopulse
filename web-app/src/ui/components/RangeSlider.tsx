interface RangeSliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  onChange: (value: number) => void;
}

export function RangeSlider({
  label,
  value,
  min,
  max,
  step = 1,
  unit,
  onChange,
}: RangeSliderProps) {
  return (
    <div className="range-slider">
      <div className="range-slider-header">
        <span>{label}</span>
        <span className="mono range-slider-value">
          {value}
          {unit ? ` ${unit}` : ''}
        </span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
      />
    </div>
  );
}
