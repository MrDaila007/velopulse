import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/providers.dart';
import '../../data/weather/weather_cities.dart';
import '../../domain/entities/companion_models.dart';
import '../../l10n/app_localizations.dart';

class CompanionSettingsCard extends ConsumerStatefulWidget {
  const CompanionSettingsCard({super.key});

  @override
  ConsumerState<CompanionSettingsCard> createState() =>
      _CompanionSettingsCardState();
}

class _CompanionSettingsCardState extends ConsumerState<CompanionSettingsCard> {
  CompanionPreferences? _prefs;
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _load());
  }

  Future<void> _load() async {
    final prefs = await ref
        .read(preferencesStoreProvider)
        .readCompanionPreferences();
    if (!mounted) return;
    setState(() {
      _prefs = prefs;
      _loading = false;
    });
  }

  Future<void> _save(CompanionPreferences value) async {
    setState(() => _prefs = value);
    await ref.read(preferencesStoreProvider).writeCompanionPreferences(value);
    await ref.read(connectionControllerProvider.notifier).syncCompanion();
  }

  String _resolvedCityId(CompanionPreferences prefs) {
    if (weatherCityById(prefs.weatherCityId) != null) {
      return prefs.weatherCityId;
    }
    return kWeatherCities.first.id;
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    if (_loading || _prefs == null) {
      return const Card(
        child: Padding(
          padding: EdgeInsets.all(16),
          child: Center(child: CircularProgressIndicator()),
        ),
      );
    }
    final prefs = _prefs!;
    final cityId = _resolvedCityId(prefs);
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: <Widget>[
            Text(
              strings.companionSection,
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            SwitchListTile(
              contentPadding: EdgeInsets.zero,
              title: Text(strings.companionShowClockLabel),
              value: prefs.showClockOnDevice,
              onChanged: (value) =>
                  _save(prefs.copyWith(showClockOnDevice: value)),
            ),
            SwitchListTile(
              contentPadding: EdgeInsets.zero,
              title: Text(strings.companionShowWeatherLabel),
              value: prefs.showWeatherOnDevice,
              onChanged: (value) =>
                  _save(prefs.copyWith(showWeatherOnDevice: value)),
            ),
            DropdownMenu<String>(
              key: ValueKey<String>(cityId),
              initialSelection: cityId,
              label: Text(strings.companionWeatherCityLabel),
              dropdownMenuEntries: <DropdownMenuEntry<String>>[
                for (final city in kWeatherCities)
                  DropdownMenuEntry<String>(value: city.id, label: city.name),
              ],
              onSelected: (value) {
                if (value == null) return;
                _save(prefs.copyWith(weatherCityId: value));
              },
            ),
          ],
        ),
      ),
    );
  }
}
