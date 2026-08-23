import 'package:flutter/material.dart';

import '../../domain/entities/models.dart';
import '../../l10n/app_localizations.dart';

class CscStatusCard extends StatelessWidget {
  const CscStatusCard({required this.telemetry, super.key});

  final Telemetry? telemetry;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    final Telemetry? live = telemetry;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: <Widget>[
            Text(
              strings.cscSensorTitle,
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            if (live == null)
              Text(strings.cscNotConnected)
            else
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: <Widget>[
                  Chip(
                    label: Text(
                      live.cscConnected
                          ? strings.cscConnectedChip
                          : strings.cscNotConnected,
                    ),
                  ),
                  if (live.cscPairing)
                    Chip(label: Text(strings.cscPairingChip)),
                  if (live.cscCrankPresent)
                    Chip(label: Text(strings.cscCrankChip)),
                  if (live.cscWheelPresent || live.cscSpeedSource)
                    Chip(
                      label: Text(
                        live.cscSpeedSource
                            ? strings.cscWheelActiveChip
                            : strings.cscWheelChip,
                      ),
                    ),
                  Chip(
                    label: Text(
                      '${strings.cscSpeedSourceLabel}: ${live.cscSpeedSource ? strings.cscSpeedSourceS3 : strings.cscSpeedSourceHall}',
                    ),
                  ),
                  if (live.cadenceValid)
                    Chip(
                      label: Text(
                        strings.cadenceValue(
                          (live.cadenceX10 / 10).toStringAsFixed(1),
                        ),
                      ),
                    ),
                ],
              ),
            const SizedBox(height: 12),
            Text(
              strings.cscPairHint,
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
      ),
    );
  }
}
