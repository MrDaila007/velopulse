import 'package:bikecomp_mobile/application/providers.dart';
import 'package:bikecomp_mobile/data/ble/fake_ble_transport.dart';
import 'package:bikecomp_mobile/presentation/bikecomp_app.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

Future<void> ensureFakeDeviceVisible(WidgetTester tester) async {
  await tester.pump(const Duration(milliseconds: 150));
  if (find.text('BikeComp-FAKE').evaluate().isEmpty) {
    if (find.text('Искать').evaluate().isNotEmpty) {
      await tester.tap(find.text('Искать'));
    }
    await tester.pump(const Duration(milliseconds: 100));
  }
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    SharedPreferencesAsyncPlatform.instance =
        InMemorySharedPreferencesAsync.empty();
  });

  testWidgets('renders four MVP navigation destinations', (tester) async {
    final fake = FakeBleTransport();
    addTearDown(fake.dispose);

    await tester.pumpWidget(
      ProviderScope(
        overrides: [bleTransportProvider.overrideWithValue(fake)],
        child: const BikeCompApp(),
      ),
    );
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 200));

    expect(find.text('Поиск'), findsOneWidget);
    expect(find.text('Показатели'), findsOneWidget);
    expect(find.text('Настройки'), findsOneWidget);
    expect(find.text('Обслуживание'), findsOneWidget);
    expect(
      find.byWidgetPredicate(
        (widget) =>
            widget is Text &&
            (widget.data == 'Искать' || widget.data == 'Остановить'),
      ),
      findsOneWidget,
    );
  });

  testWidgets('fake scan connects and shows dashboard metrics', (tester) async {
    final fake = FakeBleTransport();
    addTearDown(fake.dispose);

    await tester.pumpWidget(
      ProviderScope(
        overrides: [bleTransportProvider.overrideWithValue(fake)],
        child: const BikeCompApp(),
      ),
    );
    await ensureFakeDeviceVisible(tester);

    expect(find.text('BikeComp-FAKE'), findsOneWidget);
    await tester.tap(find.text('Подключить'));
    for (var frame = 0; frame < 10; frame++) {
      await tester.pump(const Duration(milliseconds: 100));
    }

    expect(find.text('СКОРОСТЬ'), findsOneWidget);
    expect(find.text('Поездка'), findsOneWidget);
    expect(find.text('Батарея'), findsOneWidget);

    await tester.pumpWidget(const SizedBox.shrink());
    await fake.dispose();
  });

  testWidgets('maintenance offers all three OLED test patterns', (
    tester,
  ) async {
    final fake = FakeBleTransport();
    addTearDown(fake.dispose);

    await tester.pumpWidget(
      ProviderScope(
        overrides: [bleTransportProvider.overrideWithValue(fake)],
        child: const BikeCompApp(),
      ),
    );
    await ensureFakeDeviceVisible(tester);
    await tester.tap(find.text('Подключить'));
    for (var frame = 0; frame < 10; frame++) {
      await tester.pump(const Duration(milliseconds: 100));
    }

    await tester.tap(find.text('Обслуживание'));
    await tester.pump();
    await tester.ensureVisible(find.text('Тест дисплея'));
    await tester.tap(find.text('Тест дисплея'));
    await tester.pump();

    expect(find.text('Выберите тестовый паттерн'), findsOneWidget);
    expect(find.text('Заливка'), findsOneWidget);
    expect(find.text('Шахматная сетка'), findsOneWidget);
    expect(find.text('Текст'), findsOneWidget);

    await tester.tap(find.text('Текст'));
    await tester.pump(const Duration(milliseconds: 100));
    expect(find.text('Выберите тестовый паттерн'), findsNothing);

    await tester.pumpWidget(const SizedBox.shrink());
    await fake.dispose();
  });

  testWidgets('settings and maintenance are write-blocked while disconnected', (
    tester,
  ) async {
    final fake = FakeBleTransport();
    addTearDown(fake.dispose);

    await tester.pumpWidget(
      ProviderScope(
        overrides: [bleTransportProvider.overrideWithValue(fake)],
        child: const BikeCompApp(),
      ),
    );
    await tester.pump(const Duration(milliseconds: 50));

    await tester.tap(find.text('Настройки'));
    await tester.pump();
    expect(
      find.text('Подключите BikeComp, чтобы прочитать настройки.'),
      findsOneWidget,
    );

    await tester.tap(find.text('Обслуживание'));
    await tester.pump();
    expect(
      find.text(
        'Для команд требуется готовое соединение с совместимым протоколом.',
      ),
      findsOneWidget,
    );
    final action = tester.widget<ListTile>(
      find.widgetWithText(ListTile, 'Сбросить поездку'),
    );
    expect(action.enabled, isFalse);
  });
}
