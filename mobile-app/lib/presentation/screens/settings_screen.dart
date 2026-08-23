import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../core/result.dart';
import '../../domain/entities/models.dart';
import '../../domain/services/tire_presets.dart';
import '../../l10n/app_localizations.dart';
import '../widgets/app_version_footer.dart';
import '../widgets/companion_settings_card.dart';
import '../widgets/csc_status_card.dart';
import '../widgets/page_order_editor.dart';

class SettingsScreen extends ConsumerWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final strings = AppLocalizations.of(context);
    final session = ref.watch(connectionControllerProvider);
    final state = ref.watch(configDraftControllerProvider);
    final controller = ref.read(configDraftControllerProvider.notifier);
    final draft = state.draft;

    if (draft == null) {
      return ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
        children: <Widget>[
          const SizedBox(height: 48),
          Center(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: <Widget>[
                  const Icon(Icons.tune, size: 64),
                  const SizedBox(height: 16),
                  Text(strings.connectToReadSettings),
                  if (session.deviceInfo != null) ...<Widget>[
                    const SizedBox(height: 12),
                    Text(
                      strings.deviceProtocol(
                        session.deviceInfo!.protoMajor,
                        session.deviceInfo!.protoMinor,
                        session.deviceInfo!.model,
                      ),
                    ),
                  ],
                ],
              ),
            ),
          ),
          const CompanionSettingsCard(),
          const AppVersionFooter(),
        ],
      );
    }

    final writable = session.connection is ConnectionReady;
    final canSave =
        writable &&
        state.isDirty &&
        state.isValid &&
        !state.isWriting &&
        !session.commandInFlight;

    return SingleChildScrollView(
      padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: <Widget>[
          Text(
            strings.settingsTab,
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            writable
                ? strings.draftDescription
                : strings.writeBlockedDescription,
          ),
          if (state.hasConflict) ...<Widget>[
            const SizedBox(height: 12),
            Card(
              color: Theme.of(context).colorScheme.errorContainer,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: <Widget>[
                    Text(strings.draftConflict),
                    const SizedBox(height: 8),
                    Wrap(
                      spacing: 8,
                      children: <Widget>[
                        FilledButton(
                          onPressed: () =>
                              controller.resolveConflict(keepDraft: true),
                          child: Text(strings.applyDraftAction),
                        ),
                        TextButton(
                          onPressed: () =>
                              controller.resolveConflict(keepDraft: false),
                          child: Text(strings.discardAction),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
          const SizedBox(height: 16),
          _WheelSection(draft: draft, state: state, controller: controller),
          const SizedBox(height: 12),
          _SensorSection(draft: draft, state: state, controller: controller),
          const SizedBox(height: 12),
          _DisplaySection(draft: draft, state: state, controller: controller),
          const SizedBox(height: 12),
          CscStatusCard(telemetry: session.telemetry),
          const SizedBox(height: 12),
          const CompanionSettingsCard(),
          const SizedBox(height: 12),
          _PowerSection(
            draft: draft,
            state: state,
            controller: controller,
            deepSleepSupported: session.deviceInfo?.deepSleepSupported ?? false,
          ),
          const SizedBox(height: 12),
          _DeviceSection(draft: draft, state: state, controller: controller),
          const SizedBox(height: 16),
          if (state.fieldErrors.isNotEmpty)
            Text(
              strings.fixErrors(state.fieldErrors.length),
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          const SizedBox(height: 8),
          Row(
            children: <Widget>[
              Expanded(
                child: FilledButton.icon(
                  onPressed: canSave
                      ? () async {
                          final result = await controller.save();
                          if (!context.mounted) return;
                          if (result is Failure<void>) {
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(content: Text(result.error.toString())),
                            );
                          }
                        }
                      : null,
                  icon: state.isWriting
                      ? const SizedBox.square(
                          dimension: 18,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.save_outlined),
                  label: Text(
                    state.isWriting ? strings.savingAction : strings.saveAction,
                  ),
                ),
              ),
              const SizedBox(width: 12),
              OutlinedButton(
                onPressed: () => _confirmDefaults(context, controller, draft),
                child: Text(strings.defaultsShortAction),
              ),
            ],
          ),
          if (!state.isDirty) ...<Widget>[
            const SizedBox(height: 8),
            Text(strings.noUnsavedChanges, textAlign: TextAlign.center),
          ],
          const AppVersionFooter(),
        ],
      ),
    );
  }

  Future<void> _confirmDefaults(
    BuildContext context,
    ConfigDraftController controller,
    DeviceConfig current,
  ) async {
    final strings = AppLocalizations.of(context);
    final changes = _defaultChanges(strings, current);
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(strings.loadDefaultsQuestion),
        content: SingleChildScrollView(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              Text(strings.localDraftOnly),
              const SizedBox(height: 8),
              ...changes.map((change) => Text('• $change')),
              const SizedBox(height: 8),
              Text(strings.odometerBondsUntouched),
            ],
          ),
        ),
        actions: <Widget>[
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: Text(strings.cancel),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: Text(strings.loadAction),
          ),
        ],
      ),
    );
    if (confirmed == true) controller.resetDefaults();
  }

  List<String> _defaultChanges(AppLocalizations strings, DeviceConfig current) {
    final defaults = DeviceConfig.defaults;
    final changes = <String>[];
    if (current.wheelCircumferenceMm != defaults.wheelCircumferenceMm) {
      changes.add(
        strings.defaultCircumferenceChange(
          current.wheelCircumferenceMm,
          defaults.wheelCircumferenceMm,
        ),
      );
    }
    if (current.unitsImperial != defaults.unitsImperial) {
      changes.add(strings.measurementUnitsChange);
    }
    if (current.brightnessPct != defaults.brightnessPct) {
      changes.add(
        strings.defaultBrightnessChange(
          current.brightnessPct,
          defaults.brightnessPct,
        ),
      );
    }
    if (current.stopTimeoutS != defaults.stopTimeoutS) {
      changes.add(strings.stopTimeoutChange);
    }
    if (current.displayTimeoutS != defaults.displayTimeoutS) {
      changes.add(strings.displayTimeoutChange);
    }
    if (current.deepSleepTimeoutS != defaults.deepSleepTimeoutS) {
      changes.add(strings.deepSleepTimeoutChange);
    }
    if (current.powerSaveMode != defaults.powerSaveMode) {
      changes.add(strings.powerSaveModeLabel);
    }
    if (current.deepSleepEnabled != defaults.deepSleepEnabled) {
      changes.add(strings.deepSleepEnabledLabel);
    }
    if (current.pageSwitchPeriodS != defaults.pageSwitchPeriodS) {
      changes.add(strings.pagePeriodChange);
    }
    if (current.deviceName != defaults.deviceName) {
      changes.add(strings.deviceNameLabel);
    }
    if (changes.isEmpty) {
      changes.add(strings.visibleSameHiddenRestored);
    }
    return changes;
  }
}

class _WheelSection extends StatelessWidget {
  const _WheelSection({
    required this.draft,
    required this.state,
    required this.controller,
  });

  final DeviceConfig draft;
  final ConfigDraftState state;
  final ConfigDraftController controller;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    return Card(
      child: ExpansionTile(
        initiallyExpanded: true,
        title: Text(strings.wheelUnitsSection),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: <Widget>[
          DropdownButtonFormField<int>(
            initialValue:
                TirePresets.values.values.contains(draft.wheelCircumferenceMm)
                ? draft.wheelCircumferenceMm
                : null,
            decoration: InputDecoration(labelText: strings.tireSizeLabel),
            hint: Text(strings.customValue),
            items: TirePresets.values.entries
                .map(
                  (entry) => DropdownMenuItem<int>(
                    value: entry.value,
                    child: Text(
                      strings.tirePresetValue(entry.value, entry.key),
                    ),
                  ),
                )
                .toList(growable: false),
            onChanged: (value) {
              if (value != null) {
                controller.update(draft.copyWith(wheelCircumferenceMm: value));
              }
            },
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('wheel-${draft.wheelCircumferenceMm}'),
            label: strings.wheelCircumferenceLabel,
            value: draft.wheelCircumferenceMm,
            error: state.fieldErrors['wheelCircumferenceMm'],
            onChanged: (value) =>
                controller.update(draft.copyWith(wheelCircumferenceMm: value)),
          ),
          const SizedBox(height: 12),
          SegmentedButton<bool>(
            segments: <ButtonSegment<bool>>[
              ButtonSegment<bool>(
                value: false,
                label: Text(strings.metricUnits),
              ),
              ButtonSegment<bool>(
                value: true,
                label: Text(strings.imperialUnits),
              ),
            ],
            selected: <bool>{draft.unitsImperial},
            onSelectionChanged: (selection) =>
                controller.update(draft.withFlag(0x10, selection.first)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('max-speed-${draft.maxSpeedKmh}'),
            label: strings.maxSpeedLabel,
            value: draft.maxSpeedKmh,
            error: state.fieldErrors['maxSpeedKmh'],
            onChanged: (value) =>
                controller.update(draft.copyWith(maxSpeedKmh: value)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('smoothing-${draft.smoothingWindow}'),
            label: strings.smoothingWindowLabel,
            value: draft.smoothingWindow,
            error: state.fieldErrors['smoothingWindow'],
            onChanged: (value) =>
                controller.update(draft.copyWith(smoothingWindow: value)),
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.smoothingEnabledLabel),
            value: draft.smoothingEnabled,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x01, value)),
          ),
          _IntegerField(
            key: ValueKey('stop-${draft.stopTimeoutS}'),
            label: strings.stopTimeoutLabel,
            value: draft.stopTimeoutS,
            error: state.fieldErrors['stopTimeoutS'],
            onChanged: (value) =>
                controller.update(draft.copyWith(stopTimeoutS: value)),
          ),
        ],
      ),
    );
  }
}

class _SensorSection extends StatelessWidget {
  const _SensorSection({
    required this.draft,
    required this.state,
    required this.controller,
  });

  final DeviceConfig draft;
  final ConfigDraftState state;
  final ConfigDraftController controller;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    return Card(
      child: ExpansionTile(
        title: Text(strings.sensorSection),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: <Widget>[
          _IntegerField(
            key: ValueKey('debounce-${draft.debounceMs}'),
            label: strings.debounceLabel,
            value: draft.debounceMs,
            error: state.fieldErrors['debounceMs'],
            onChanged: (value) =>
                controller.update(draft.copyWith(debounceMs: value)),
          ),
          const SizedBox(height: 12),
          DropdownButtonFormField<int>(
            initialValue: draft.activeEdge,
            decoration: InputDecoration(
              labelText: strings.activeEdgeLabel,
              errorText: state.fieldErrors['activeEdge'],
            ),
            items: <DropdownMenuItem<int>>[
              DropdownMenuItem<int>(
                value: 0,
                child: Text(strings.activeEdgeFalling),
              ),
              DropdownMenuItem<int>(
                value: 1,
                child: Text(strings.activeEdgeRising),
              ),
              DropdownMenuItem<int>(
                value: 2,
                child: Text(strings.activeEdgeChange),
              ),
            ],
            onChanged: (value) {
              if (value != null) {
                controller.update(draft.copyWith(activeEdge: value));
              }
            },
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.sensorInvertLabel),
            value: draft.sensorInvert,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x20, value)),
          ),
        ],
      ),
    );
  }
}

class _DisplaySection extends StatelessWidget {
  const _DisplaySection({
    required this.draft,
    required this.state,
    required this.controller,
  });

  final DeviceConfig draft;
  final ConfigDraftState state;
  final ConfigDraftController controller;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    return Card(
      child: ExpansionTile(
        title: Text(strings.displaySection),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: <Widget>[
          Text(strings.brightnessValue(draft.brightnessPct)),
          const SizedBox(height: 4),
          Text(
            strings.ambientBrightnessHelper,
            style: Theme.of(context).textTheme.bodySmall,
          ),
          Slider(
            value: draft.brightnessPct.clamp(1, 100).toDouble(),
            min: 1,
            max: 100,
            divisions: 99,
            label: '${draft.brightnessPct}%',
            onChanged: (value) =>
                controller.update(draft.copyWith(brightnessPct: value.round())),
          ),
          if (state.fieldErrors['brightnessPct'] case final error?)
            Text(
              error,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          _IntegerField(
            key: ValueKey('display-${draft.displayTimeoutS}'),
            label: strings.displayTimeoutLabel,
            helper: strings.neverHelper,
            value: draft.displayTimeoutS,
            error: state.fieldErrors['displayTimeoutS'],
            onChanged: (value) =>
                controller.update(draft.copyWith(displayTimeoutS: value)),
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.displayAutoOffLabel),
            value: draft.displayAutoOff,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x04, value)),
          ),
          _IntegerField(
            key: ValueKey('page-${draft.pageSwitchPeriodS}'),
            label: strings.pageSwitchLabel,
            value: draft.pageSwitchPeriodS,
            error: state.fieldErrors['pageSwitchPeriodS'],
            onChanged: (value) =>
                controller.update(draft.copyWith(pageSwitchPeriodS: value)),
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.autoPageSwitchLabel),
            value: draft.autoPageSwitch,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x02, value)),
          ),
          PageOrderEditor(
            enabledMask: draft.enabledPagesMask,
            pageOrder: draft.pageOrder,
            pinnedPage: draft.pinnedPage,
            enabledPagesLabel: strings.enabledPagesLabel,
            enabledPagesHint: strings.enabledPagesHint,
            tripPageOrderLabel: strings.tripPageOrderLabel,
            pinnedPageLabel: strings.pinnedPageLabel,
            onUpdate: (update) => controller.update(
              draft.copyWith(
                enabledPagesMask: update.enabledMask,
                pageOrder: update.pageOrder,
                pinnedPage: update.pinnedPage,
              ),
            ),
          ),
          if (state.fieldErrors['enabledPagesMask'] case final error?)
            Text(
              error,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
          if (state.fieldErrors['pageOrder'] case final error?)
            Text(
              error,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
        ],
      ),
    );
  }
}

class _PowerSection extends StatelessWidget {
  const _PowerSection({
    required this.draft,
    required this.state,
    required this.controller,
    required this.deepSleepSupported,
  });

  final DeviceConfig draft;
  final ConfigDraftState state;
  final ConfigDraftController controller;
  final bool deepSleepSupported;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    return Card(
      child: ExpansionTile(
        title: Text(strings.powerSection),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: <Widget>[
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.powerSaveModeLabel),
            value: draft.powerSaveMode,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x40, value)),
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: Text(strings.bleAlwaysAdvertiseLabel),
            value: draft.bleAlwaysAdvertise,
            onChanged: (value) =>
                controller.update(draft.withFlag(0x08, value)),
          ),
          if (deepSleepSupported)
            SwitchListTile(
              contentPadding: EdgeInsets.zero,
              title: Text(strings.deepSleepEnabledLabel),
              value: draft.deepSleepEnabled,
              onChanged: (value) =>
                  controller.update(draft.withFlag(0x80, value)),
            ),
          _IntegerField(
            key: ValueKey('deep-sleep-${draft.deepSleepTimeoutS}'),
            label: strings.deepSleepTimeoutLabel,
            helper: strings.neverHelper,
            value: draft.deepSleepTimeoutS,
            error: state.fieldErrors['deepSleepTimeoutS'],
            onChanged: (value) =>
                controller.update(draft.copyWith(deepSleepTimeoutS: value)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('low-batt-${draft.lowBatteryPct}'),
            label: strings.lowBatteryPctLabel,
            value: draft.lowBatteryPct,
            error: state.fieldErrors['lowBatteryPct'],
            onChanged: (value) =>
                controller.update(draft.copyWith(lowBatteryPct: value)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('odo-save-${draft.odometerSaveIntervalM}'),
            label: strings.odometerSaveIntervalLabel,
            value: draft.odometerSaveIntervalM,
            error: state.fieldErrors['odometerSaveIntervalM'],
            onChanged: (value) =>
                controller.update(draft.copyWith(odometerSaveIntervalM: value)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('batt-scale-${draft.battCalScalePermille}'),
            label: strings.battCalScaleLabel,
            value: draft.battCalScalePermille,
            error: state.fieldErrors['battCalScalePermille'],
            onChanged: (value) =>
                controller.update(draft.copyWith(battCalScalePermille: value)),
          ),
          const SizedBox(height: 12),
          _IntegerField(
            key: ValueKey('batt-offset-${draft.battCalOffsetMv}'),
            label: strings.battCalOffsetLabel,
            value: draft.battCalOffsetMv,
            error: state.fieldErrors['battCalOffsetMv'],
            onChanged: (value) =>
                controller.update(draft.copyWith(battCalOffsetMv: value)),
          ),
        ],
      ),
    );
  }
}

class _DeviceSection extends StatelessWidget {
  const _DeviceSection({
    required this.draft,
    required this.state,
    required this.controller,
  });

  final DeviceConfig draft;
  final ConfigDraftState state;
  final ConfigDraftController controller;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    return Card(
      child: ExpansionTile(
        title: Text(strings.deviceSection),
        childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
        children: <Widget>[
          Text(
            strings.deviceNameRestartHint,
            style: Theme.of(context).textTheme.bodySmall,
          ),
          const SizedBox(height: 8),
          TextFormField(
            initialValue: draft.deviceName,
            maxLength: 15,
            decoration: InputDecoration(
              labelText: strings.deviceNameLabel,
              helperText: strings.deviceNameHelper,
              errorText: state.fieldErrors['deviceName'],
            ),
            onChanged: (value) =>
                controller.update(draft.copyWith(deviceName: value)),
          ),
        ],
      ),
    );
  }
}

class _IntegerField extends StatelessWidget {
  const _IntegerField({
    required this.label,
    required this.value,
    required this.onChanged,
    this.helper,
    this.error,
    super.key,
  });

  final String label;
  final int value;
  final ValueChanged<int> onChanged;
  final String? helper;
  final String? error;

  @override
  Widget build(BuildContext context) => TextFormField(
    initialValue: value.toString(),
    keyboardType: TextInputType.number,
    decoration: InputDecoration(
      labelText: label,
      helperText: helper,
      errorText: error,
    ),
    onChanged: (raw) {
      final parsed = int.tryParse(raw);
      if (parsed != null) onChanged(parsed);
    },
  );
}
