export interface WeatherCity {
  id: string;
  name: string;
  latitude: number;
  longitude: number;
}

export const weatherCities: WeatherCity[] = [
  { id: 'minsk', name: 'Минск', latitude: 53.9045, longitude: 27.5615 },
  { id: 'moscow', name: 'Москва', latitude: 55.7558, longitude: 37.6173 },
  { id: 'spb', name: 'Санкт-Петербург', latitude: 59.9343, longitude: 30.3351 },
  { id: 'kazan', name: 'Казань', latitude: 55.7887, longitude: 49.1221 },
  { id: 'ekb', name: 'Екатеринбург', latitude: 56.8389, longitude: 60.6057 },
  { id: 'novosibirsk', name: 'Новосибирск', latitude: 55.0084, longitude: 82.9357 },
  { id: 'krasnodar', name: 'Краснодар', latitude: 45.0355, longitude: 38.9753 },
  { id: 'sochi', name: 'Сочи', latitude: 43.6028, longitude: 39.7342 },
];

export function weatherCityById(id: string): WeatherCity | undefined {
  return weatherCities.find((c) => c.id === id);
}
