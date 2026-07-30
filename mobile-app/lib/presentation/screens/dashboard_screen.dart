import 'package:flutter/material.dart' hide ConnectionState;
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../domain/entities/models.dart';
import '../../domain/services/unit_formatter.dart';

class DashboardScreen extends ConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
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
                        'Соединение потеряно. Последнее обновление: ${_lastSeen(session.lastTelemetryAt)}',
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
                      label: 'Поездка',
                      value: UnitFormatter.distanceCm(
                        telemetry.tripDistanceCm,
                        imperial: imperial,
                      ),
                      icon: Icons.route,
                    ),
                    _MetricCard(
                      label: 'Средняя',
                      value: UnitFormatter.speed(
                        telemetry.avgSpeedX100,
                        imperial: imperial,
                      ),
                      icon: Icons.show_chart,
                    ),
                    _MetricCard(
                      label: 'Максимальная',
                      value: UnitFormatter.speed(
                        telemetry.maxSpeedX100,
                        imperial: imperial,
                      ),
                      icon: Icons.trending_up,
                    ),
                    _MetricCard(
                      label: 'В движении',
                      value: UnitFormatter.duration(telemetry.movingTimeS),
                      icon: Icons.timer_outlined,
                    ),
                    _MetricCard(
                      label: 'Одометр',
                      value: UnitFormatter.odometerM(
                        telemetry.odometerM,
                        imperial: imperial,
                      ),
                      icon: Icons.straighten,
                    ),
                    _MetricCard(
                      label: 'Батарея',
                      value:
                          '${telemetry.batteryPct}% · ${telemetry.batteryMv} мВ',
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
                    label: 'Движение',
                    value: _ride(telemetry.rideState),
                  ),
                  _StatusChip(
                    label: 'Датчик',
                    value: _sensor(telemetry.sensorState),
                  ),
                  _StatusChip(label: 'Связь', value: live ? 'есть' : 'нет'),
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
            'Быстрые действия',
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
                        title: 'Сбросить поездку?',
                        command: DeviceCommandId.resetTrip,
                      )
                    : null,
                icon: const Icon(Icons.restart_alt),
                label: const Text('Сбросить поездку'),
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
                label: Text(telemetry.displayOn ? 'OLED выкл.' : 'OLED вкл.'),
              ),
              OutlinedButton.icon(
                onPressed: live && !session.commandInFlight
                    ? ref.read(connectionControllerProvider.notifier).refresh
                    : null,
                icon: const Icon(Icons.refresh),
                label: const Text('Обновить'),
              ),
              OutlinedButton.icon(
                onPressed: () => context.go('/settings'),
                icon: const Icon(Icons.tune),
                label: const Text('Настройки'),
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
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: const Text(
          'Команда будет выполнена на подключённом устройстве.',
        ),
        actions: <Widget>[
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Отмена'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Подтвердить'),
          ),
        ],
      ),
    );
    if (confirmed == true) await _send(ref, command);
  }

  Future<void> _send(WidgetRef ref, DeviceCommandId id) => ref
      .read(connectionControllerProvider.notifier)
      .sendCommand(DeviceCommand(id: id));

  String _ride(RideState state) => switch (state) {
    RideState.idle => 'остановка',
    RideState.moving => 'движение',
    RideState.paused => 'пауза',
    RideState.unknown => 'неизвестно',
  };

  String _sensor(SensorState state) => switch (state) {
    SensorState.ok => 'норма',
    SensorState.idle => 'ожидание',
    SensorState.stuck => 'залипание',
    SensorState.noSignal => 'нет сигнала',
    SensorState.unknown => 'неизвестно',
  };

  String _lastSeen(DateTime? value) {
    if (value == null) return 'неизвестно';
    final local = value.toLocal();
    return '${local.hour.toString().padLeft(2, '0')}:${local.minute.toString().padLeft(2, '0')}:${local.second.toString().padLeft(2, '0')}';
  }
}

class _EmptyDashboard extends StatelessWidget {
  const _EmptyDashboard({required this.onConnect, required this.connection});

  final VoidCallback onConnect;
  final ConnectionState connection;

  @override
  Widget build(BuildContext context) => Center(
    child: Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: <Widget>[
          const Icon(Icons.speed, size: 72),
          const SizedBox(height: 16),
          Text(
            'Нет показателей',
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            connection is ConnectionIncompatibleProtocol
                ? 'Информация об устройстве доступна, но telemetry протокола несовместима.'
                : 'Подключите BikeComp, чтобы увидеть данные поездки.',
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 16),
          FilledButton(onPressed: onConnect, child: const Text('К поиску')),
        ],
      ),
    ),
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
          Text('СКОРОСТЬ', style: Theme.of(context).textTheme.labelLarge),
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
