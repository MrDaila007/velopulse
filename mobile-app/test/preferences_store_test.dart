import 'package:bikecomp_mobile/data/local/preferences_store.dart';
import 'package:bikecomp_mobile/domain/entities/models.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    SharedPreferencesAsyncPlatform.instance =
        InMemorySharedPreferencesAsync.empty();
  });

  test('remembered device survives a store recreation', () async {
    final first = PreferencesStore();
    await first.rememberDevice('device-A', 'BikeComp-A');

    final restored = await PreferencesStore().readRememberedDevice();

    expect(restored?.id, 'device-A');
    expect(restored?.name, 'BikeComp-A');
  });

  test('dirty drafts are versioned and namespaced by device id', () async {
    final base = DeviceConfig.defaults;
    final draftA = base.copyWith(brightnessPct: 75);
    final draftB = base.copyWith(wheelCircumferenceMm: 2136);
    final store = PreferencesStore();
    await store.writeDraft(
      PersistedDraft(
        deviceId: 'device-A',
        base: base,
        draft: draftA,
        savedAt: DateTime.utc(2026, 7, 30),
      ),
    );
    await store.writeDraft(
      PersistedDraft(
        deviceId: 'device-B',
        base: base,
        draft: draftB,
        savedAt: DateTime.utc(2026, 7, 30, 1),
      ),
    );

    final restoredA = await PreferencesStore().readDraft('device-A');
    final restoredB = await PreferencesStore().readDraft('device-B');

    expect(restoredA?.base, base);
    expect(restoredA?.draft, draftA);
    expect(restoredB?.draft, draftB);
    expect(restoredA?.savedAt, DateTime.utc(2026, 7, 30));
  });
}
