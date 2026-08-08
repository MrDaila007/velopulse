interface LiveMetricProps {
  label: string;
  value: string | number;
  unit?: string;
}

export function LiveMetric({ label, value, unit }: LiveMetricProps) {
  return (
    <div className="live-metric">
      <span className="live-metric-label">{label}</span>
      <span className="live-metric-value mono">
        {value}
        {unit ? <span className="live-metric-unit"> {unit}</span> : null}
      </span>
    </div>
  );
}
