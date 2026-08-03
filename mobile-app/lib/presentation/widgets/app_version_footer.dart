import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/app_version.dart';
import '../../l10n/app_localizations.dart';

class AppVersionFooter extends ConsumerWidget {
  const AppVersionFooter({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final strings = AppLocalizations.of(context);
    final version = ref.watch(appVersionLabelProvider);
    return Padding(
      padding: const EdgeInsets.only(top: 24),
      child: Center(
        child: version.when(
          data: (label) => Text(
            strings.appVersion(label),
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: Theme.of(context).colorScheme.onSurfaceVariant,
            ),
            textAlign: TextAlign.center,
          ),
          loading: () => Text(
            strings.appVersion('…'),
            style: Theme.of(context).textTheme.bodySmall,
          ),
          error: (_, _) => const SizedBox.shrink(),
        ),
      ),
    );
  }
}
