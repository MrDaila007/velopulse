import 'dart:async';

import 'package:flutter/material.dart' hide ConnectionState;
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../domain/entities/models.dart';
import '../../l10n/app_localizations.dart';

class ScanScreen extends ConsumerWidget {
  const ScanScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final strings = AppLocalizations.of(context);
    final session = ref.watch(connectionControllerProvider);
    final controller = ref.read(connectionControllerProvider.notifier);
    final scanning = session.connection is ConnectionScanning;

    return RefreshIndicator(
      onRefresh: controller.scan,
      child: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
        children: <Widget>[
          Text(
            strings.scanTitle,
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            'Показываются только устройства с сервисом BikeComp. Сохранённое устройство подключится автоматически.',
            style: Theme.of(context).textTheme.bodyMedium,
          ),
          const SizedBox(height: 16),
          _StateCard(connection: session.connection),
          const SizedBox(height: 16),
          Row(
            children: <Widget>[
              Expanded(
                child: FilledButton.icon(
                  onPressed: scanning ? controller.stopScan : controller.scan,
                  icon: Icon(scanning ? Icons.stop : Icons.radar),
                  label: Text(
                    scanning ? strings.stopScanAction : strings.scanAction,
                  ),
                ),
              ),
              if (session.selectedDevice != null) ...<Widget>[
                const SizedBox(width: 12),
                OutlinedButton(
                  onPressed: controller.forgetDevice,
                  child: Text(strings.forgetAction),
                ),
              ],
            ],
          ),
          const SizedBox(height: 16),
          if (scanning) const LinearProgressIndicator(),
          if (session.devices.isEmpty) ...<Widget>[
            const SizedBox(height: 40),
            Icon(Icons.directions_bike, size: 56, color: Colors.grey.shade500),
            const SizedBox(height: 12),
            Text(
              strings.noDevices,
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodyLarge,
            ),
          ] else
            ...session.devices.map(
              (device) => Padding(
                padding: const EdgeInsets.only(top: 12),
                child: Card(
                  child: ListTile(
                    leading: const CircleAvatar(child: Icon(Icons.pedal_bike)),
                    title: Text(device.name.isEmpty ? 'BikeComp' : device.name),
                    subtitle: Text(
                      '${device.deviceId}\nRSSI ${device.rssi} dBm · ${_bondLabel(device.bondState)}',
                    ),
                    isThreeLine: true,
                    trailing: FilledButton.tonal(
                      onPressed: () {
                        unawaited(controller.connectDevice(device));
                        unawaited(context.push<void>('/connecting'));
                      },
                      child: Text(strings.connectAction),
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  String _bondLabel(BondState state) => switch (state) {
    BondState.bonded => 'сопряжено',
    BondState.none => 'не сопряжено',
    BondState.unknown => 'сопряжение неизвестно',
  };
}

class _StateCard extends StatelessWidget {
  const _StateCard({required this.connection});

  final ConnectionState connection;

  @override
  Widget build(BuildContext context) {
    final (icon, title, body, color) = switch (connection) {
      ConnectionBluetoothOff() => (
        Icons.bluetooth_disabled,
        'Bluetooth выключен',
        'Включите Bluetooth, чтобы начать поиск.',
        Colors.red,
      ),
      ConnectionPermissionRequired(:final error) => (
        Icons.admin_panel_settings_outlined,
        'Нужен доступ',
        error.message,
        Colors.orange,
      ),
      ConnectionFailed(:final error) => (
        Icons.error_outline,
        'Не удалось продолжить',
        error.message,
        Colors.red,
      ),
      ConnectionReady() => (
        Icons.bluetooth_connected,
        'Устройство подключено',
        'Показатели обновляются в реальном времени.',
        Colors.green,
      ),
      ConnectionReadOnly(:final reason) => (
        Icons.visibility_outlined,
        'Режим только для чтения',
        reason.message,
        Colors.orange,
      ),
      ConnectionIncompatibleProtocol(:final error) => (
        Icons.system_update_alt,
        'Несовместимый протокол',
        error.message,
        Colors.orange,
      ),
      _ => (
        Icons.bluetooth_searching,
        'Готово к поиску',
        'Разрешения будут запрошены при первом поиске.',
        Theme.of(context).colorScheme.primary,
      ),
    };
    return Card(
      color: color.withValues(alpha: 0.08),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: <Widget>[
            Icon(icon, color: color),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: <Widget>[
                  Text(title, style: Theme.of(context).textTheme.titleMedium),
                  const SizedBox(height: 4),
                  Text(body),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
