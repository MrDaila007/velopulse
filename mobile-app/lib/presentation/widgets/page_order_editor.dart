import 'package:flutter/material.dart';

import '../../domain/display_pages.dart';

class PageOrderUpdate {
  const PageOrderUpdate({
    required this.enabledMask,
    required this.pageOrder,
    required this.pinnedPage,
  });

  final int enabledMask;
  final List<int> pageOrder;
  final int pinnedPage;
}

class PageOrderEditor extends StatelessWidget {
  const PageOrderEditor({
    required this.enabledMask,
    required this.pageOrder,
    required this.pinnedPage,
    required this.onUpdate,
    required this.enabledPagesLabel,
    required this.enabledPagesHint,
    required this.tripPageOrderLabel,
    required this.pinnedPageLabel,
    super.key,
  });

  final int enabledMask;
  final List<int> pageOrder;
  final int pinnedPage;
  final ValueChanged<PageOrderUpdate> onUpdate;
  final String enabledPagesLabel;
  final String enabledPagesHint;
  final String tripPageOrderLabel;
  final String pinnedPageLabel;

  void _emit({int? mask, List<int>? order, int? pinned}) {
    onUpdate(
      PageOrderUpdate(
        enabledMask: mask ?? enabledMask,
        pageOrder: order ?? pageOrder,
        pinnedPage: pinned ?? pinnedPage,
      ),
    );
  }

  void _togglePage(int page, bool enabled) {
    final nextMask = DisplayPages.setPageEnabled(enabledMask, page, enabled);
    final nextOrder = DisplayPages.syncPageOrderWithMask(pageOrder, nextMask);
    _emit(
      mask: nextMask,
      order: nextOrder,
      pinned: DisplayPages.clampPinnedPage(pinnedPage, nextMask),
    );
  }

  void _setPosition(int position, int page) {
    final next = List<int>.from(pageOrder);
    final existing = next.indexOf(page);
    if (existing >= 0) {
      next[existing] = next[position];
    }
    next[position] = page;
    _emit(order: next);
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: <Widget>[
        Text(enabledPagesLabel),
        Text(enabledPagesHint, style: Theme.of(context).textTheme.bodySmall),
        const SizedBox(height: 8),
        Wrap(
          spacing: 4,
          runSpacing: 0,
          children: <Widget>[
            for (var page = 0; page < DisplayPages.displayPageCount; page++)
              FilterChip(
                label: Text(DisplayPages.pageLabel(page)),
                selected: DisplayPages.isPageEnabled(enabledMask, page),
                onSelected: (selected) {
                  if (!selected &&
                      DisplayPages.enabledPageCount(enabledMask) <= 1) {
                    return;
                  }
                  _togglePage(page, selected);
                },
              ),
          ],
        ),
        const SizedBox(height: 16),
        Text(tripPageOrderLabel),
        const SizedBox(height: 8),
        for (var position = 0; position < pageOrder.length; position++)
          Padding(
            padding: const EdgeInsets.only(bottom: 8),
            child: DropdownButtonFormField<int>(
              initialValue: DisplayPages.isTripPage(pageOrder[position])
                  ? pageOrder[position]
                  : position,
              decoration: InputDecoration(labelText: '#${position + 1}'),
              items: <DropdownMenuItem<int>>[
                for (
                  var option = 0;
                  option < DisplayPages.tripPageCount;
                  option++
                )
                  DropdownMenuItem<int>(
                    value: option,
                    child: Text(DisplayPages.tripPageLabels[option]),
                  ),
              ],
              onChanged: (value) {
                if (value != null) _setPosition(position, value);
              },
            ),
          ),
        DropdownButtonFormField<int>(
          initialValue: pinnedPage,
          decoration: InputDecoration(labelText: pinnedPageLabel),
          items: <DropdownMenuItem<int>>[
            for (var page = 0; page < DisplayPages.displayPageCount; page++)
              DropdownMenuItem<int>(
                value: page,
                enabled: DisplayPages.isPageEnabled(enabledMask, page),
                child: Text(DisplayPages.pageLabel(page)),
              ),
          ],
          onChanged: (value) {
            if (value != null) _emit(pinned: value);
          },
        ),
      ],
    );
  }
}
