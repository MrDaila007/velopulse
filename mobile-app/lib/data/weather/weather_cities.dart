class WeatherCity {
  const WeatherCity({
    required this.id,
    required this.name,
    required this.latitude,
    required this.longitude,
  });

  final String id;
  final String name;
  final double latitude;
  final double longitude;
}

const List<WeatherCity> kWeatherCities = <WeatherCity>[
  WeatherCity(
    id: 'minsk',
    name: 'Минск',
    latitude: 53.9045,
    longitude: 27.5615,
  ),
  WeatherCity(
    id: 'moscow',
    name: 'Москва',
    latitude: 55.7558,
    longitude: 37.6173,
  ),
  WeatherCity(
    id: 'spb',
    name: 'Санкт-Петербург',
    latitude: 59.9343,
    longitude: 30.3351,
  ),
  WeatherCity(
    id: 'kazan',
    name: 'Казань',
    latitude: 55.7887,
    longitude: 49.1221,
  ),
  WeatherCity(
    id: 'ekb',
    name: 'Екатеринбург',
    latitude: 56.8389,
    longitude: 60.6057,
  ),
  WeatherCity(
    id: 'novosibirsk',
    name: 'Новосибирск',
    latitude: 55.0084,
    longitude: 82.9357,
  ),
  WeatherCity(
    id: 'krasnodar',
    name: 'Краснодар',
    latitude: 45.0355,
    longitude: 38.9753,
  ),
  WeatherCity(
    id: 'sochi',
    name: 'Сочи',
    latitude: 43.6028,
    longitude: 39.7342,
  ),
];

WeatherCity? weatherCityById(String id) {
  for (final city in kWeatherCities) {
    if (city.id == id) return city;
  }
  return null;
}
