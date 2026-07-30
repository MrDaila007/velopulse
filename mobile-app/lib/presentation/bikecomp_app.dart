import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../application/app_states.dart';
import '../application/providers.dart';
import '../l10n/app_localizations.dart';
import 'screens/dashboard_screen.dart';
import 'screens/maintenance_screen.dart';
import 'screens/scan_screen.dart';
import 'screens/settings_screen.dart';

GoRouter _createRouter() => GoRouter(
  initialLocation: '/scan',
  routes: <RouteBase>[
    StatefulShellRoute.indexedStack(
      builder: (context, state, navigationShell) =>
          BikeCompShell(navigationShell: navigationShell),
      branches: <StatefulShellBranch>[
        StatefulShellBranch(
          routes: <RouteBase>[
            GoRoute(
              path: '/scan',
              builder: (context, state) => const ScanScreen(),
            ),
          ],
        ),
        StatefulShellBranch(
          routes: <RouteBase>[
            GoRoute(
              path: '/dashboard',
              builder: (context, state) => const DashboardScreen(),
            ),
          ],
        ),
        StatefulShellBranch(
          routes: <RouteBase>[
            GoRoute(
              path: '/settings',
              builder: (context, state) => const SettingsScreen(),
            ),
          ],
        ),
        StatefulShellBranch(
          routes: <RouteBase>[
            GoRoute(
              path: '/maintenance',
              builder: (context, state) => const MaintenanceScreen(),
            ),
          ],
        ),
      ],
    ),
    GoRoute(
      path: '/connecting',
      builder: (context, state) => const ConnectingScreen(),
    ),
  ],
);

class BikeCompApp extends StatefulWidget {
  const BikeCompApp({super.key});

  @override
  State<BikeCompApp> createState() => _BikeCompAppState();
}

class _BikeCompAppState extends State<BikeCompApp> {
  late final GoRouter _router = _createRouter();

  @override
  void dispose() {
    _router.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final colorScheme = ColorScheme.fromSeed(
      seedColor: const Color(0xFF006B5E),
      brightness: Brightness.light,
    );
    return MaterialApp.router(
      title: 'BikeComp',
      debugShowCheckedModeBanner: false,
      locale: const Locale('ru'),
      supportedLocales: AppLocalizations.supportedLocales,
      localizationsDelegates: AppLocalizations.localizationsDelegates,
      theme: ThemeData(
        colorScheme: colorScheme,
        useMaterial3: true,
        scaffoldBackgroundColor: const Color(0xFFF6F8F7),
        cardTheme: const CardThemeData(margin: EdgeInsets.zero),
        inputDecorationTheme: const InputDecorationTheme(
          border: OutlineInputBorder(),
        ),
      ),
      routerConfig: _router,
    );
  }
}

class BikeCompShell extends ConsumerStatefulWidget {
  const BikeCompShell({required this.navigationShell, super.key});

  final StatefulNavigationShell navigationShell;

  @override
  ConsumerState<BikeCompShell> createState() => _BikeCompShellState();
}

class _BikeCompShellState extends ConsumerState<BikeCompShell>
    with WidgetsBindingObserver {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    final foreground = state == AppLifecycleState.resumed;
    ref.read(connectionControllerProvider.notifier).setForeground(foreground);
  }

  @override
  Widget build(BuildContext context) {
    final strings = AppLocalizations.of(context);
    ref.listen(connectionControllerProvider, (previous, next) {
      if (!mounted) return;
      final message = next.lastMessage;
      final error = next.lastError;
      if (message != null && message != previous?.lastMessage) {
        ScaffoldMessenger.of(
          context,
        ).showSnackBar(SnackBar(content: Text(message)));
      } else if (error != null && error != previous?.lastError) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(error.message),
            action: error.action == null
                ? null
                : SnackBarAction(
                    label: error.action!,
                    onPressed: () => ref
                        .read(connectionControllerProvider.notifier)
                        .openSettings(),
                  ),
          ),
        );
      }
    });

    return Scaffold(
      appBar: AppBar(
        title: const Text('BikeComp'),
        actions: const <Widget>[
          Padding(
            padding: EdgeInsets.only(right: 16),
            child: ConnectionBadge(),
          ),
        ],
      ),
      body: SafeArea(child: widget.navigationShell),
      bottomNavigationBar: NavigationBar(
        selectedIndex: widget.navigationShell.currentIndex,
        onDestinationSelected: (index) => widget.navigationShell.goBranch(
          index,
          initialLocation: index == widget.navigationShell.currentIndex,
        ),
        destinations: <NavigationDestination>[
          NavigationDestination(
            icon: const Icon(Icons.bluetooth_searching),
            label: strings.scanTab,
          ),
          NavigationDestination(
            icon: const Icon(Icons.speed),
            label: strings.dashboardTab,
          ),
          NavigationDestination(
            icon: const Icon(Icons.tune),
            label: strings.settingsTab,
          ),
          NavigationDestination(
            icon: const Icon(Icons.build_outlined),
            label: strings.maintenanceTab,
          ),
        ],
      ),
    );
  }
}

class ConnectionBadge extends ConsumerWidget {
  const ConnectionBadge({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final connection = ref.watch(
      connectionControllerProvider.select((value) => value.connection),
    );
    final (label, color) = switch (connection) {
      ConnectionReady() => ('Подключено', Colors.green),
      ConnectionReadOnly() => ('Только чтение', Colors.orange),
      ConnectionIncompatibleProtocol() => ('Несовместимо', Colors.orange),
      ConnectionScanning() => ('Поиск', Colors.blue),
      ConnectionConnecting() ||
      ConnectionSynchronizing() => ('Подключение', Colors.blue),
      ConnectionReconnecting() => ('Переподключение', Colors.orange),
      ConnectionBluetoothOff() => ('Bluetooth выкл.', Colors.red),
      ConnectionFailed() ||
      ConnectionPermissionRequired() => ('Ошибка', Colors.red),
      _ => ('Не подключено', Colors.grey),
    };
    return Semantics(
      label: 'Состояние соединения: $label',
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: <Widget>[
          Icon(Icons.circle, size: 10, color: color),
          const SizedBox(width: 6),
          Text(label, style: Theme.of(context).textTheme.labelMedium),
        ],
      ),
    );
  }
}

class ConnectingScreen extends ConsumerWidget {
  const ConnectingScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final session = ref.watch(connectionControllerProvider);
    if (session.connection is ConnectionReady ||
        session.connection is ConnectionReadOnly) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (context.mounted) context.go('/dashboard');
      });
    }
    ref.listen(
      connectionControllerProvider.select((value) => value.connection),
      (previous, next) {
        if (next is ConnectionReady || next is ConnectionReadOnly) {
          WidgetsBinding.instance.addPostFrameCallback((_) {
            if (context.mounted) context.go('/dashboard');
          });
        }
      },
    );
    final label = switch (session.connection) {
      ConnectionConnecting() => 'Устанавливаем соединение…',
      ConnectionSynchronizing(:final stage) => stage.label,
      ConnectionReconnecting(:final attempt, :final delaySeconds) =>
        'Попытка $attempt через $delaySeconds с',
      ConnectionIncompatibleProtocol() => 'Версия протокола несовместима',
      ConnectionFailed(:final error) => error.message,
      _ => 'Подготовка подключения…',
    };
    final failed =
        session.connection is ConnectionFailed ||
        session.connection is ConnectionIncompatibleProtocol;
    return Scaffold(
      appBar: AppBar(title: const Text('Подключение')),
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 420),
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: <Widget>[
                if (!failed) const CircularProgressIndicator(),
                if (!failed) const SizedBox(height: 24),
                Text(label, textAlign: TextAlign.center),
                if (session.deviceInfo != null) ...<Widget>[
                  const SizedBox(height: 16),
                  Text(
                    '${session.deviceInfo!.model} · ${session.deviceInfo!.fwVersion}',
                    style: Theme.of(context).textTheme.bodySmall,
                  ),
                ],
                const SizedBox(height: 24),
                if (failed)
                  FilledButton(
                    onPressed: context.pop,
                    child: const Text('Вернуться к поиску'),
                  )
                else
                  TextButton(
                    onPressed: () async {
                      await ref
                          .read(connectionControllerProvider.notifier)
                          .disconnect();
                      if (context.mounted) context.pop();
                    },
                    child: const Text('Отмена'),
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
