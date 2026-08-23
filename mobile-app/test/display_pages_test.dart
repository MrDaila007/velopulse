import 'package:bikecomp_mobile/domain/display_pages.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('covers all eight firmware display pages', () {
    expect(DisplayPages.displayPageCount, 8);
    expect(DisplayPages.pageLabels, hasLength(8));
    expect(DisplayPages.pageLabels[0], 'Поездка');
    expect(DisplayPages.pageLabels[7], 'Каденс (CAD)');
    expect(DisplayPages.validEnabledPagesMask, 0xFF);
  });

  test('tracks enabled pages including weather and cadence bits', () {
    expect(DisplayPages.isPageEnabled(0x15, 0), isTrue);
    expect(DisplayPages.isPageEnabled(0x15, 1), isFalse);
    expect(DisplayPages.setPageEnabled(0x15, 1, true), 0x17);
    expect(DisplayPages.enabledPageCount(0x60), 2);
    expect(DisplayPages.enabledPageCount(0x80), 1);
    expect(DisplayPages.enabledPageCount(0xFF), 8);
  });

  test('syncs trip order without weather or cadence slots', () {
    final order = DisplayPages.syncPageOrderWithMask(<int>[
      0,
      1,
      2,
      3,
      4,
    ], 0x13);
    expect(order.take(3), <int>[0, 1, 4]);
    expect(
      DisplayPages.syncPageOrderWithMask(<int>[0, 1, 2, 3, 4], 0xFF),
      <int>[0, 1, 2, 3, 4],
    );
  });

  test('clamps pinned page onto an enabled page including cadence', () {
    expect(DisplayPages.clampPinnedPage(7, 0x80), 7);
    expect(DisplayPages.clampPinnedPage(7, 0x01), 0);
  });
}
