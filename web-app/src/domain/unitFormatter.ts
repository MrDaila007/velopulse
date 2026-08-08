export function formatSpeed(speedX100: number, imperial: boolean): string {
  const kmh = speedX100 / 100;
  if (imperial) {
    return `${(kmh * 0.621371).toFixed(1)} mph`;
  }
  return `${kmh.toFixed(1)} km/h`;
}

export function formatDistanceMeters(meters: number, imperial: boolean): string {
  if (imperial) {
    const miles = meters / 1609.344;
    return miles >= 10 ? `${miles.toFixed(1)} mi` : `${(miles * 5280).toFixed(0)} ft`;
  }
  if (meters >= 1000) return `${(meters / 1000).toFixed(2)} km`;
  return `${meters} m`;
}

export function formatDistanceCm(cm: number, imperial: boolean): string {
  return formatDistanceMeters(cm / 100, imperial);
}

export function formatDuration(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
  return `${m}:${String(s).padStart(2, '0')}`;
}

export function formatBattery(mv: number, pct: number): string {
  return `${pct}% (${mv} mV)`;
}
