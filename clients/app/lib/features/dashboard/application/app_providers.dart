import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';

import '../../settings/data/settings_repository.dart';
import '../data/connection_repository.dart';
import '../data/control_repository.dart';
import 'dashboard_controller.dart';
import 'settings_controller.dart';

final sharedPreferencesProvider = Provider<SharedPreferences>((ref) {
  throw UnimplementedError(
    'sharedPreferencesProvider must be overridden at app startup.',
  );
});

final httpClientProvider = Provider<http.Client>((ref) {
  final client = http.Client();
  ref.onDispose(client.close);
  return client;
});

final settingsRepositoryProvider = Provider<SettingsRepository>((ref) {
  return SettingsRepository(ref.watch(sharedPreferencesProvider));
});

final settingsControllerProvider =
    NotifierProvider<SettingsController, SettingsState>(SettingsController.new);

final connectionRepositoryProvider = Provider<ConnectionRepository>((ref) {
  return ConnectionRepository(ref.watch(httpClientProvider));
});

final controlRepositoryProvider = Provider<ControlRepository>((ref) {
  return ControlRepository(ref.watch(httpClientProvider));
});

final dashboardControllerProvider =
    NotifierProvider.autoDispose<DashboardController, DashboardState>(
      DashboardController.new,
    );
