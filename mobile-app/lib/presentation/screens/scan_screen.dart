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
            strings.scanDescription,
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
                      strings.deviceScanDetails(
                        _bondLabel(strings, device.bondState),
                        device.deviceId,
                        device.rssi,
                      ),
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

  String _bondLabel(AppLocalizations strings, BondState state) =>
      switch (state) {
        BondState.bonded => strings.bonded,
        BondState.none => strings.notBonded,
        BondState.unknown => strings.bondUnknown,
      };
}

class _StateCard extends StatelessWidget {
  const _StateCard({required this.connection});

  final ConnectionState connection;

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    final (icon, title, body, color) = switch (connection) {
      ConnectionBluetoothOff() => (
        Icons.bluetooth_disabled,
        strings.bluetoothOff,
        strings.enableBluetoothBody,
        Colors.red,
      ),
      ConnectionPermissionRequired(:final error) => (
        Icons.admin_panel_settings_outlined,
        strings.accessRequiredTitle,
        error.message,
        Colors.orange,
      ),
      ConnectionFailed(:final error) => (
        Icons.error_outline,
        strings.unableContinueTitle,
        error.message,
        Colors.red,
      ),
      ConnectionReady() => (
        Icons.bluetooth_connected,
        strings.deviceConnectedTitle,
        strings.metricsLiveBody,
        Colors.green,
      ),
      ConnectionReadOnly(:final reason) => (
        Icons.visibility_outlined,
        strings.readOnlyTitle,
        reason.message,
        Colors.orange,
      ),
      ConnectionIncompatibleProtocol(:final error) => (
        Icons.system_update_alt,
        strings.incompatibleProtocolTitle,
        error.message,
        Colors.orange,
      ),
      _ => (
        Icons.bluetooth_searching,
        strings.readyToScanTitle,
        strings.permissionsFirstScanBody,
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
