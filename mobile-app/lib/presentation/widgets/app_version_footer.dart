import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/app_version.dart';
import '../../application/providers.dart';
import '../../l10n/app_localizations.dart';

String firmwareStackLabel(AppLocalizations strings, String fwVersion) {
  if (fwVersion.toLowerCase().contains('zephyr')) {
    return strings.stackZephyr;
  }
  return strings.stackArduino;
}

class AppVersionFooter extends ConsumerWidget {
  const AppVersionFooter({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final strings = AppLocalizations.of(context);
    final appVersion = ref.watch(appVersionLabelProvider);
    final deviceInfo = ref.watch(
      connectionControllerProvider.select((session) => session.deviceInfo),
    );
    final textStyle = Theme.of(context).textTheme.bodySmall?.copyWith(
      color: Theme.of(context).colorScheme.onSurfaceVariant,
    );

    Widget line(String text) => Padding(
      padding: const EdgeInsets.only(top: 4),
      child: Text(text, style: textStyle, textAlign: TextAlign.center),
    );

    return Padding(
      padding: const EdgeInsets.only(top: 24),
      child: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            appVersion.when(
              data: (label) => Text(
                strings.appVersion(label),
                style: textStyle,
                textAlign: TextAlign.center,
              ),
              loading: () => Text(
                strings.appVersion('…'),
                style: textStyle,
                textAlign: TextAlign.center,
              ),
              error: (_, _) => const SizedBox.shrink(),
            ),
            if (deviceInfo case final info?) ...<Widget>[
              line(
                strings.firmwareVersion(
                  info.fwVersion,
                  firmwareStackLabel(strings, info.fwVersion),
                ),
              ),
              line(
                strings.bleProtocolVersion(info.protoMajor, info.protoMinor),
              ),
            ] else
              line(strings.firmwareVersionPending),
          ],
        ),
      ),
    );
  }
}
