import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import '../../../core/network/endpoints.dart';
import '../domain/endpoint_settings.dart';
import '../domain/theme_mode.dart';

class SettingsRepository {
  const SettingsRepository(this._sharedPreferences);

  static const endpointStorageKey = 'plasma_remote_toolbar.endpoints';
  static const themeStorageKey = 'plasma_remote_toolbar.theme_mode';

  final SharedPreferences _sharedPreferences;

  EndpointSettings? loadEndpointSettings() {
    final stored = _sharedPreferences.getString(endpointStorageKey);
    if (stored == null) {
      return null;
    }

    try {
      final parsed = jsonDecode(stored) as Map<String, Object?>;
      final settings = EndpointSettings.fromJson(parsed);
      return EndpointSettings(
        httpBaseUrl: normalizeHttpBaseUrl(settings.httpBaseUrl),
        wsUrl: normalizeWebSocketUrl(settings.wsUrl),
      );
    } catch (_) {
      return null;
    }
  }

  EndpointSettings saveEndpointSettings(EndpointSettings settings) {
    final normalized = EndpointSettings(
      httpBaseUrl: normalizeHttpBaseUrl(settings.httpBaseUrl),
      wsUrl: normalizeWebSocketUrl(settings.wsUrl),
    );
    _sharedPreferences.setString(
      endpointStorageKey,
      jsonEncode(normalized.toJson()),
    );
    return normalized;
  }

  AppThemeMode loadThemeMode() {
    return AppThemeMode.fromStorageValue(
      _sharedPreferences.getString(themeStorageKey),
    );
  }

  void saveThemeMode(AppThemeMode mode) {
    _sharedPreferences.setString(themeStorageKey, mode.storageValue);
  }
}
