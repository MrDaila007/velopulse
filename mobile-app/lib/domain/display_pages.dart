/// OLED carousel pages matching firmware `DisplayPage` / `enabled_pages_mask`.
abstract final class DisplayPages {
  static const int tripPageCount = 5;
  static const int displayPageCount = 8;
  static const int validEnabledPagesMask = 0xFF;
  static const int cadencePage = 7;

  static const List<String> tripPageLabels = <String>[
    'Поездка',
    'Средняя',
    'Максимум',
    'Время',
    'Одометр',
  ];

  static const List<String> pageLabels = <String>[
    ...tripPageLabels,
    'Погода и часы',
    'Дождь',
    'Каденс (CAD)',
  ];

  static String pageLabel(int page) {
    if (page >= 0 && page < pageLabels.length) return pageLabels[page];
    return 'Стр. $page';
  }

  static bool isTripPage(int page) => page >= 0 && page < tripPageCount;

  static bool isPageEnabled(int mask, int page) => (mask & (1 << page)) != 0;

  static int setPageEnabled(int mask, int page, bool enabled) =>
      enabled ? mask | (1 << page) : mask & ~(1 << page);

  static int enabledPageCount(int mask) {
    var count = 0;
    for (var page = 0; page < displayPageCount; page++) {
      if (isPageEnabled(mask, page)) count++;
    }
    return count;
  }

  static List<int> syncPageOrderWithMask(List<int> pageOrder, int mask) {
    final enabled = pageOrder
        .where((page) => isTripPage(page) && isPageEnabled(mask, page))
        .toList();
    final disabled = <int>[
      for (var page = 0; page < tripPageCount; page++)
        if (!isPageEnabled(mask, page)) page,
    ];
    return <int>[...enabled, ...disabled].take(tripPageCount).toList();
  }

  static int clampPinnedPage(int pinned, int mask) {
    if (isPageEnabled(mask, pinned)) return pinned;
    for (var page = 0; page < displayPageCount; page++) {
      if (isPageEnabled(mask, page)) return page;
    }
    return 0;
  }
}
