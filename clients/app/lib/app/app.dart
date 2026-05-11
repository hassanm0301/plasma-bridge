import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../features/dashboard/application/app_providers.dart';
import '../features/dashboard/presentation/dashboard_screen.dart';
import '../features/settings/presentation/setup_screen.dart';
import 'theme/app_theme.dart';

class PlasmaRemoteToolbarApp extends ConsumerWidget {
  const PlasmaRemoteToolbarApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settingsState = ref.watch(settingsControllerProvider);

    return MaterialApp(
      title: 'Plasma Remote Toolbar',
      debugShowCheckedModeBanner: false,
      themeMode: settingsState.themeMode.toThemeMode(),
      theme: AppTheme.lightTheme(),
      darkTheme: AppTheme.darkTheme(),
      home: settingsState.endpointSettings == null
          ? const SetupScreen()
          : const DashboardScreen(),
    );
  }
}
