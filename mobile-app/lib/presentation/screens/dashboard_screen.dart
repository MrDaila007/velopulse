import 'package:flutter/material.dart' hide ConnectionState;
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../domain/entities/models.dart';
import '../../domain/services/unit_formatter.dart';
import '../../l10n/app_localizations.dart';

class DashboardScreen extends ConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final strings = AppLocalizations.of(context);
    final session = ref.watch(connectionControllerProvider);
    final telemetry = session.telemetry;
    final imperial = session.deviceConfig?.unitsImperial ?? false;
    final live =
        session.connection is ConnectionReady ||
        session.connection is ConnectionReadOnly;

    if (telemetry == null) {
      return _EmptyDashboard(
        onConnect: () => context.go('/scan'),
        connection: session.connection,
      );
    }

    return RefreshIndicator(
      onRefresh: ref.read(connectionControllerProvider.notifier).refresh,
      child: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
        children: <Widget>[
          if (!live)
            Card(
              color: Theme.of(context).colorScheme.surfaceContainerHighest,
              child: Padding(
                padding: const EdgeInsets.all(12),
                child: Row(
                  children: <Widget>[
                    const Icon(Icons.cloud_off_outlined),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Text(
                        strings.connectionLost(
                          _lastSeen(strings, session.lastTelemetryAt),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          if (!live) const SizedBox(height: 12),
          Opacity(
            opacity: live ? 1 : 0.5,
            child: Column(
              children: <Widget>[
                _SpeedCard(telemetry: telemetry, imperial: imperial),
                const SizedBox(height: 12),
                GridView.count(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  crossAxisCount: MediaQuery.sizeOf(context).width > 600
                      ? 3
                      : 2,
                  childAspectRatio: 1.45,
                  crossAxisSpacing: 12,
                  mainAxisSpacing: 12,
                  children: <Widget>[
                    _MetricCard(
                      label: strings.tripLabel,
                      value: UnitFormatter.distanceCm(
                        telemetry.tripDistanceCm,
                        imperial: imperial,
                      ),
                      icon: Icons.route,
                    ),
                    _MetricCard(
                      label: strings.averageLabel,
                      value: UnitFormatter.speed(
                        telemetry.avgSpeedX100,
                        imperial: imperial,
                      ),
                      icon: Icons.show_chart,
                    ),
                    _MetricCard(
                      label: strings.maximumLabel,
                      value: UnitFormatter.speed(
                        telemetry.maxSpeedX100,
                        imperial: imperial,
                      ),
                      icon: Icons.trending_up,
                    ),
                    _MetricCard(
                      label: strings.movingTimeLabel,
                      value: UnitFormatter.duration(telemetry.movingTimeS),
                      icon: Icons.timer_outlined,
                    ),
                    _MetricCard(
                      label: strings.odometerLabel,
                      value: UnitFormatter.odometerM(
                        telemetry.odometerM,
                        imperial: imperial,
                      ),
                      icon: Icons.straighten,
                    ),
                    _MetricCard(
                      label: strings.batteryLabel,
                      value: strings.batteryValue(
                        telemetry.batteryMv,
                        telemetry.batteryPct,
                      ),
                      icon: telemetry.lowBattery
                          ? Icons.battery_alert
                          : Icons.battery_full,
                    ),
                  ],
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Wrap(
                spacing: 16,
                runSpacing: 8,
                children: <Widget>[
                  _StatusChip(
                    label: strings.movementLabel,
                    value: _ride(strings, telemetry.rideState),
                  ),
                  _StatusChip(
                    label: strings.sensorLabel,
                    value: _sensor(strings, telemetry.sensorState),
                  ),
                  _StatusChip(
                    label: strings.linkLabel,
                    value: live
                        ? strings.linkAvailable
                        : strings.linkUnavailable,
                  ),
                  _StatusChip(
                    label: 'RSSI',
                    value: session.rssi == null ? '—' : '${session.rssi} dBm',
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),
          Text(
            strings.quickActions,
            style: Theme.of(context).textTheme.titleMedium,
          ),
          const SizedBox(height: 8),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: <Widget>[
              FilledButton.tonalIcon(
                onPressed: live && !session.commandInFlight
                    ? () => _confirmCommand(
                        context,
                        ref,
                        title: strings.resetTripQuestion,
                        command: DeviceCommandId.resetTrip,
                      )
                    : null,
                icon: const Icon(Icons.restart_alt),
                label: Text(strings.resetTrip),
              ),
              FilledButton.tonalIcon(
                onPressed: live && !session.commandInFlight
                    ? () => _send(
                        ref,
                        telemetry.displayOn
                            ? DeviceCommandId.displayOff
                            : DeviceCommandId.displayOn,
                      )
                    : null,
                icon: Icon(
                  telemetry.displayOn ? Icons.visibility_off : Icons.visibility,
                ),
                label: Text(
                  telemetry.displayOn
                      ? strings.displayOffShort
                      : strings.displayOnShort,
                ),
              ),
              OutlinedButton.icon(
                onPressed: live && !session.commandInFlight
                    ? ref.read(connectionControllerProvider.notifier).refresh
                    : null,
                icon: const Icon(Icons.refresh),
                label: Text(strings.refreshAction),
              ),
              OutlinedButton.icon(
                onPressed: () => context.go('/settings'),
                icon: const Icon(Icons.tune),
                label: Text(strings.settingsTab),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Future<void> _confirmCommand(
    BuildContext context,
    WidgetRef ref, {
    required String title,
    required DeviceCommandId command,
  }) async {
    final strings = AppLocalizations.of(context);
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: Text(strings.commandDeviceBody),
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
    if (confirmed == true) await _send(ref, command);
  }

  Future<void> _send(WidgetRef ref, DeviceCommandId id) => ref
      .read(connectionControllerProvider.notifier)
      .sendCommand(DeviceCommand(id: id));

  String _ride(AppLocalizations strings, RideState state) => switch (state) {
    RideState.idle => strings.rideIdle,
    RideState.moving => strings.rideMoving,
    RideState.paused => strings.ridePaused,
    RideState.unknown => strings.unknownValue,
  };

  String _sensor(AppLocalizations strings, SensorState state) =>
      switch (state) {
        SensorState.ok => strings.sensorOk,
        SensorState.idle => strings.sensorIdle,
        SensorState.stuck => strings.sensorStuck,
        SensorState.noSignal => strings.sensorNoSignal,
        SensorState.unknown => strings.unknownValue,
      };

  String _lastSeen(AppLocalizations strings, DateTime? value) {
    if (value == null) return strings.unknownValue;
    final local = value.toLocal();
    return '${local.hour.toString().padLeft(2, '0')}:${local.minute.toString().padLeft(2, '0')}:${local.second.toString().padLeft(2, '0')}';
  }
}

class _EmptyDashboard extends StatelessWidget {
  const _EmptyDashboard({required this.onConnect, required this.connection});

  final VoidCallback onConnect;
  final ConnectionState connection;

  @override
  Widget build(BuildContext context) => Builder(
    builder: (context) {
      final strings = AppLocalizations.of(context);
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              const Icon(Icons.speed, size: 72),
              const SizedBox(height: 16),
              Text(
                strings.noMetrics,
                style: Theme.of(context).textTheme.headlineSmall,
              ),
              const SizedBox(height: 8),
              Text(
                connection is ConnectionIncompatibleProtocol
                    ? strings.incompatibleTelemetry
                    : strings.connectToSeeRide,
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 16),
              FilledButton(
                onPressed: onConnect,
                child: Text(strings.toScanAction),
              ),
            ],
          ),
        ),
      );
    },
  );
}

class _SpeedCard extends StatelessWidget {
  const _SpeedCard({required this.telemetry, required this.imperial});

  final Telemetry telemetry;
  final bool imperial;

  @override
  Widget build(BuildContext context) => Card(
    color: Theme.of(context).colorScheme.primaryContainer,
    child: Padding(
      padding: const EdgeInsets.symmetric(vertical: 28, horizontal: 20),
      child: Column(
        children: <Widget>[
          Text(
            AppLocalizations.of(context).speedLabel,
            style: Theme.of(context).textTheme.labelLarge,
          ),
          const SizedBox(height: 8),
          FittedBox(
            child: Text(
              UnitFormatter.speed(telemetry.speedX100, imperial: imperial),
              style: Theme.of(
                context,
              ).textTheme.displayMedium?.copyWith(fontWeight: FontWeight.w700),
            ),
          ),
        ],
      ),
    ),
  );
}

class _MetricCard extends StatelessWidget {
  const _MetricCard({
    required this.label,
    required this.value,
    required this.icon,
  });

  final String label;
  final String value;
  final IconData icon;

  @override
  Widget build(BuildContext context) => Card(
    child: Padding(
      padding: const EdgeInsets.all(12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: <Widget>[
          Icon(icon, color: Theme.of(context).colorScheme.primary),
          Text(value, style: Theme.of(context).textTheme.titleMedium),
          Text(label, style: Theme.of(context).textTheme.bodySmall),
        ],
      ),
    ),
  );
}

class _StatusChip extends StatelessWidget {
  const _StatusChip({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) => Chip(label: Text('$label: $value'));
}
