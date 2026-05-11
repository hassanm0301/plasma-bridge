import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../settings/data/settings_repository.dart';
import '../../settings/domain/endpoint_settings.dart';
import '../../settings/domain/theme_mode.dart';
import 'app_providers.dart';

class SettingsState {
  const SettingsState({
    required this.endpointSettings,
    required this.themeMode,
  });

  final EndpointSettings? endpointSettings;
  final AppThemeMode themeMode;

  SettingsState copyWith({
    EndpointSettings? endpointSettings,
    bool clearEndpointSettings = false,
    AppThemeMode? themeMode,
  }) {
    return SettingsState(
      endpointSettings: clearEndpointSettings
          ? null
          : (endpointSettings ?? this.endpointSettings),
      themeMode: themeMode ?? this.themeMode,
    );
  }
}

class SettingsController extends Notifier<SettingsState> {
  late final SettingsRepository _repository;

  @override
  SettingsState build() {
    _repository = ref.watch(settingsRepositoryProvider);
    return SettingsState(
      endpointSettings: _repository.loadEndpointSettings(),
      themeMode: _repository.loadThemeMode(),
    );
  }

  void saveEndpointSettings(EndpointSettings settings) {
    state = state.copyWith(
      endpointSettings: _repository.saveEndpointSettings(settings),
    );
  }

  void saveThemeMode(AppThemeMode mode) {
    _repository.saveThemeMode(mode);
    state = state.copyWith(themeMode: mode);
  }
}
