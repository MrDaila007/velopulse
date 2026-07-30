import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../application/app_states.dart';
import '../../application/providers.dart';
import '../../core/result.dart';
import '../../domain/entities/models.dart';
import '../../domain/services/tire_presets.dart';

class SettingsScreen extends ConsumerWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final session = ref.watch(connectionControllerProvider);
    final state = ref.watch(configDraftControllerProvider);
    final controller = ref.read(configDraftControllerProvider.notifier);
    final draft = state.draft;

    if (draft == null) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              const Icon(Icons.tune, size: 64),
              const SizedBox(height: 16),
              const Text('Подключите BikeComp, чтобы прочитать настройки.'),
              if (session.deviceInfo != null) ...<Widget>[
                const SizedBox(height: 12),
                Text(
                  '${session.deviceInfo!.model} · протокол ${session.deviceInfo!.protoMajor}.${session.deviceInfo!.protoMinor}',
                ),
              ],
            ],
          ),
        ),
      );
    }

    final writable = session.connection is ConnectionReady;
    final canSave =
        writable &&
        state.isDirty &&
        state.isValid &&
        !state.isWriting &&
        !session.commandInFlight;

    return ListView(
      padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
      children: <Widget>[
        Text('Настройки', style: Theme.of(context).textTheme.headlineSmall),
        const SizedBox(height: 8),
        Text(
          writable
              ? 'Изменения хранятся как черновик до подтверждённой записи.'
              : 'Просмотр доступен, запись заблокирована состоянием соединения.',
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
                  const Text(
                    'Настройки устройства изменились после сохранения черновика.',
                  ),
                  const SizedBox(height: 8),
                  Wrap(
                    spacing: 8,
                    children: <Widget>[
                      FilledButton(
                        onPressed: () =>
                            controller.resolveConflict(keepDraft: true),
                        child: const Text('Применить черновик'),
                      ),
                      TextButton(
                        onPressed: () =>
                            controller.resolveConflict(keepDraft: false),
                        child: const Text('Отбросить'),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ],
        const SizedBox(height: 16),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Text(
                  'Колесо и единицы',
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                const SizedBox(height: 16),
                DropdownButtonFormField<int>(
                  initialValue:
                      TirePresets.values.values.contains(
                        draft.wheelCircumferenceMm,
                      )
                      ? draft.wheelCircumferenceMm
                      : null,
                  decoration: const InputDecoration(
                    labelText: 'Размер покрышки',
                  ),
                  hint: const Text('Пользовательский'),
                  items: TirePresets.values.entries
                      .map(
                        (entry) => DropdownMenuItem<int>(
                          value: entry.value,
                          child: Text('${entry.key} · ${entry.value} мм'),
                        ),
                      )
                      .toList(growable: false),
                  onChanged: (value) {
                    if (value != null) {
                      controller.update(
                        draft.copyWith(wheelCircumferenceMm: value),
                      );
                    }
                  },
                ),
                const SizedBox(height: 12),
                _IntegerField(
                  key: ValueKey('wheel-${draft.wheelCircumferenceMm}'),
                  label: 'Окружность колеса, мм',
                  value: draft.wheelCircumferenceMm,
                  error: state.fieldErrors['wheelCircumferenceMm'],
                  onChanged: (value) => controller.update(
                    draft.copyWith(wheelCircumferenceMm: value),
                  ),
                ),
                const SizedBox(height: 12),
                SegmentedButton<bool>(
                  segments: const <ButtonSegment<bool>>[
                    ButtonSegment<bool>(value: false, label: Text('км / км/ч')),
                    ButtonSegment<bool>(value: true, label: Text('мили / mph')),
                  ],
                  selected: <bool>{draft.unitsImperial},
                  onSelectionChanged: (selection) =>
                      controller.update(draft.withFlag(0x10, selection.first)),
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Text(
                  'Дисплей и остановка',
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                const SizedBox(height: 8),
                Text('Яркость: ${draft.brightnessPct}%'),
                Slider(
                  value: draft.brightnessPct.clamp(0, 100).toDouble(),
                  min: 0,
                  max: 100,
                  divisions: 20,
                  label: '${draft.brightnessPct}%',
                  onChanged: (value) => controller.update(
                    draft.copyWith(brightnessPct: value.round()),
                  ),
                ),
                if (state.fieldErrors['brightnessPct'] case final error?)
                  Text(
                    error,
                    style: TextStyle(
                      color: Theme.of(context).colorScheme.error,
                    ),
                  ),
                const SizedBox(height: 8),
                _IntegerField(
                  key: ValueKey('stop-${draft.stopTimeoutS}'),
                  label: 'Пауза после остановки, с',
                  value: draft.stopTimeoutS,
                  error: state.fieldErrors['stopTimeoutS'],
                  onChanged: (value) =>
                      controller.update(draft.copyWith(stopTimeoutS: value)),
                ),
                const SizedBox(height: 12),
                _IntegerField(
                  key: ValueKey('display-${draft.displayTimeoutS}'),
                  label: 'Выключить дисплей через, с',
                  helper: '0 — никогда',
                  value: draft.displayTimeoutS,
                  error: state.fieldErrors['displayTimeoutS'],
                  onChanged: (value) =>
                      controller.update(draft.copyWith(displayTimeoutS: value)),
                ),
                const SizedBox(height: 12),
                _IntegerField(
                  key: ValueKey('page-${draft.pageSwitchPeriodS}'),
                  label: 'Переключать страницу каждые, с',
                  value: draft.pageSwitchPeriodS,
                  error: state.fieldErrors['pageSwitchPeriodS'],
                  onChanged: (value) => controller.update(
                    draft.copyWith(pageSwitchPeriodS: value),
                  ),
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 16),
        if (state.fieldErrors.isNotEmpty)
          Text(
            'Исправьте ${state.fieldErrors.length} ${_errorWord(state.fieldErrors.length)}.',
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
                label: Text(state.isWriting ? 'Сохраняем…' : 'Сохранить'),
              ),
            ),
            const SizedBox(width: 12),
            OutlinedButton(
              onPressed: () => _confirmDefaults(context, controller, draft),
              child: const Text('По умолчанию'),
            ),
          ],
        ),
        if (!state.isDirty) ...<Widget>[
          const SizedBox(height: 8),
          const Text(
            'Нет несохранённых изменений.',
            textAlign: TextAlign.center,
          ),
        ],
      ],
    );
  }

  Future<void> _confirmDefaults(
    BuildContext context,
    ConfigDraftController controller,
    DeviceConfig current,
  ) async {
    final changes = _defaultChanges(current);
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Загрузить значения по умолчанию?'),
        content: SingleChildScrollView(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              const Text('Будет изменён только локальный черновик:'),
              const SizedBox(height: 8),
              ...changes.map((change) => Text('• $change')),
              const SizedBox(height: 8),
              const Text('Одометр и Bluetooth-сопряжения не затрагиваются.'),
            ],
          ),
        ),
        actions: <Widget>[
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Отмена'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Загрузить'),
          ),
        ],
      ),
    );
    if (confirmed == true) controller.resetDefaults();
  }

  List<String> _defaultChanges(DeviceConfig current) {
    final defaults = DeviceConfig.defaults;
    final changes = <String>[];
    if (current.wheelCircumferenceMm != defaults.wheelCircumferenceMm) {
      changes.add(
        'окружность: ${current.wheelCircumferenceMm} → ${defaults.wheelCircumferenceMm} мм',
      );
    }
    if (current.unitsImperial != defaults.unitsImperial) {
      changes.add('единицы измерения');
    }
    if (current.brightnessPct != defaults.brightnessPct) {
      changes.add(
        'яркость: ${current.brightnessPct} → ${defaults.brightnessPct}%',
      );
    }
    if (current.stopTimeoutS != defaults.stopTimeoutS) {
      changes.add('таймаут остановки');
    }
    if (current.displayTimeoutS != defaults.displayTimeoutS) {
      changes.add('таймаут дисплея');
    }
    if (current.pageSwitchPeriodS != defaults.pageSwitchPeriodS) {
      changes.add('период переключения страниц');
    }
    if (changes.isEmpty) {
      changes.add(
        'видимые значения уже совпадают; скрытые поля будут восстановлены',
      );
    }
    return changes;
  }

  String _errorWord(int count) => count == 1 ? 'ошибку' : 'ошибки';
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
