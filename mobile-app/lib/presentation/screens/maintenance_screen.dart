import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../core/result.dart';
import '../../domain/entities/models.dart';
import '../../l10n/app_localizations.dart';

class MaintenanceScreen extends ConsumerStatefulWidget {
  const MaintenanceScreen({super.key});

  @override
  ConsumerState<MaintenanceScreen> createState() => _MaintenanceScreenState();
}

class _MaintenanceScreenState extends ConsumerState<MaintenanceScreen> {
  Timer? _timer;
  int _secondsLeft = 60;

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  Future<void> _toggleSensorTest(bool active) async {
    if (active) {
      _timer?.cancel();
      final result = await _send(DeviceCommandId.sensorTestStop);
      if (result.isSuccess && mounted) setState(() => _secondsLeft = 60);
      return;
    }

    final result = await _send(DeviceCommandId.sensorTestStart);
    if (!result.isSuccess || !mounted) return;
    setState(() => _secondsLeft = 60);
    _timer?.cancel();
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) return;
      if (_secondsLeft <= 1) {
        timer.cancel();
        _send(DeviceCommandId.sensorTestStop);
        setState(() => _secondsLeft = 60);
      } else {
        setState(() => _secondsLeft--);
      }
    });
  }

  Future<Result<CommandResult>> _send(DeviceCommandId id) => ref
      .read(connectionControllerProvider.notifier)
      .sendCommand(DeviceCommand(id: id));

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    final session = ref.watch(connectionControllerProvider);
    final telemetry = session.telemetry;
    final ready = session.connection is ConnectionReady;
    final enabled = ready && !session.commandInFlight;

    return ListView(
      padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
      children: <Widget>[
        Text(
          strings.maintenanceTab,
          style: Theme.of(context).textTheme.headlineSmall,
        ),
        const SizedBox(height: 8),
        Text(strings.maintenanceConfirmedOnly),
        const SizedBox(height: 16),
        if (!ready)
          Card(
            color: Theme.of(context).colorScheme.surfaceContainerHighest,
            child: Padding(
              padding: EdgeInsets.all(16),
              child: Text(strings.commandsRequireReady),
            ),
          ),
        if (!ready) const SizedBox(height: 12),
        _ActionTile(
          icon: Icons.restart_alt,
          title: strings.resetTrip,
          subtitle: strings.resetTripSubtitle,
          enabled: enabled,
          onPressed: () => _confirm(
            title: strings.resetCurrentTripQuestion,
            command: DeviceCommandId.resetTrip,
          ),
        ),
        const SizedBox(height: 8),
        _ActionTile(
          icon: telemetry?.displayOn == true
              ? Icons.visibility_off
              : Icons.visibility,
          title: telemetry?.displayOn == true
              ? strings.displayOff
              : strings.displayOn,
          subtitle: strings.displayStateSubtitle,
          enabled: enabled,
          onPressed: () => _send(
            telemetry?.displayOn == true
                ? DeviceCommandId.displayOff
                : DeviceCommandId.displayOn,
          ),
        ),
        const SizedBox(height: 8),
        _ActionTile(
          icon: Icons.grid_view_outlined,
          title: strings.displayTest,
          subtitle: strings.displayTestSubtitle,
          enabled: enabled,
          onPressed: () => _send(DeviceCommandId.displayTest),
        ),
        const SizedBox(height: 8),
        _ActionTile(
          icon: Icons.save_outlined,
          title: strings.forceSave,
          subtitle: strings.forceSaveSubtitle,
          enabled: enabled,
          onPressed: () => _send(DeviceCommandId.forceSave),
        ),
        const SizedBox(height: 16),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Row(
                  children: <Widget>[
                    const Icon(Icons.sensors),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Text(
                        strings.sensorTest,
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                    ),
                    if (session.sensorTestActive)
                      Text(
                        strings.secondsShort(_secondsLeft),
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                  ],
                ),
                const SizedBox(height: 12),
                Text(
                  session.sensorTestActive
                      ? strings.sensorTestActiveBody
                      : strings.sensorTestIdleBody,
                ),
                const SizedBox(height: 16),
                Wrap(
                  spacing: 12,
                  runSpacing: 12,
                  children: <Widget>[
                    _SensorValue(
                      label: strings.stateLabel,
                      value: telemetry == null
                          ? '—'
                          : _sensorLabel(strings, telemetry.sensorState),
                    ),
                    _SensorValue(
                      label: strings.revolutionsLabel,
                      value: telemetry?.revolutions.toString() ?? '—',
                    ),
                    _SensorValue(
                      label: strings.pulseAgeLabel,
                      value: telemetry == null
                          ? '—'
                          : strings.millisecondsShort(telemetry.lastPulseAgeMs),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                FilledButton.icon(
                  onPressed: enabled || session.sensorTestActive
                      ? () => _toggleSensorTest(session.sensorTestActive)
                      : null,
                  icon: Icon(
                    session.sensorTestActive ? Icons.stop : Icons.play_arrow,
                  ),
                  label: Text(
                    session.sensorTestActive
                        ? strings.stopSensorTest
                        : strings.startSensorTest,
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Future<void> _confirm({
    required String title,
    required DeviceCommandId command,
  }) async {
    final strings = AppLocalizations.of(context);
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: Text(strings.waitForConfirmation),
        actions: <Widget>[
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: Text(strings.cancel),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: Text(strings.confirm),
          ),
        ],
      ),
    );
    if (confirmed == true) await _send(command);
  }

  String _sensorLabel(AppLocalizations strings, SensorState state) =>
      switch (state) {
        SensorState.ok => strings.sensorOk,
        SensorState.idle => strings.sensorIdle,
        SensorState.stuck => strings.sensorStuck,
        SensorState.noSignal => strings.sensorNoSignal,
        SensorState.unknown => strings.unknownValue,
      };
}

class _ActionTile extends StatelessWidget {
  const _ActionTile({
    required this.icon,
    required this.title,
    required this.subtitle,
    required this.enabled,
    required this.onPressed,
  });

  final IconData icon;
  final String title;
  final String subtitle;
  final bool enabled;
  final VoidCallback onPressed;

  @override
  Widget build(BuildContext context) => Card(
    child: ListTile(
      leading: Icon(icon),
      title: Text(title),
      subtitle: Text(subtitle),
      trailing: const Icon(Icons.chevron_right),
      enabled: enabled,
      onTap: enabled ? onPressed : null,
    ),
  );
}

class _SensorValue extends StatelessWidget {
  const _SensorValue({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) => Container(
    constraints: const BoxConstraints(minWidth: 130),
    padding: const EdgeInsets.all(12),
    decoration: BoxDecoration(
      color: Theme.of(context).colorScheme.surfaceContainerHighest,
      borderRadius: BorderRadius.circular(12),
    ),
    child: Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: <Widget>[
        Text(label, style: Theme.of(context).textTheme.bodySmall),
        const SizedBox(height: 4),
        Text(value, style: Theme.of(context).textTheme.titleMedium),
      ],
    ),
  );
}
