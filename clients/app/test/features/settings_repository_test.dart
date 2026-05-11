import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_remote_toolbar_app/features/settings/data/settings_repository.dart';
import 'package:plasma_remote_toolbar_app/features/settings/domain/endpoint_settings.dart';
import 'package:plasma_remote_toolbar_app/features/settings/domain/theme_mode.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    SharedPreferences.setMockInitialValues({});
  });

  test('uses null endpoints when nothing is stored', () async {
    final prefs = await SharedPreferences.getInstance();
    final repository = SettingsRepository(prefs);

    expect(repository.loadEndpointSettings(), isNull);
    expect(repository.loadThemeMode(), AppThemeMode.light);
  });

  test('persists normalized endpoints and theme mode', () async {
    final prefs = await SharedPreferences.getInstance();
    final repository = SettingsRepository(prefs);

    final saved = repository.saveEndpointSettings(
      const EndpointSettings(
        httpBaseUrl: 'http://localhost:8080/',
        wsUrl: 'ws://localhost:8081/ws',
        windowSortBy: WindowSortBy.name,
        windowSortDirection: WindowSortDirection.descending,
      ),
    );
    repository.saveThemeMode(AppThemeMode.dark);

    expect(saved.httpBaseUrl, 'http://localhost:8080');
    expect(repository.loadEndpointSettings()?.httpBaseUrl, saved.httpBaseUrl);
    expect(repository.loadEndpointSettings()?.windowSortBy, WindowSortBy.name);
    expect(
      repository.loadEndpointSettings()?.windowSortDirection,
      WindowSortDirection.descending,
    );
    expect(repository.loadThemeMode(), AppThemeMode.dark);
  });

  test(
    'defaults missing sort settings to usage newest first for older saved settings',
    () async {
      SharedPreferences.setMockInitialValues({
        SettingsRepository.endpointStorageKey:
            '{"httpBaseUrl":"http://localhost:8080","wsUrl":"ws://localhost:8081/ws"}',
      });
      final prefs = await SharedPreferences.getInstance();
      final repository = SettingsRepository(prefs);

      expect(
        repository.loadEndpointSettings()?.windowSortBy,
        WindowSortBy.usage,
      );
      expect(
        repository.loadEndpointSettings()?.windowSortDirection,
        WindowSortDirection.newestFirst,
      );
    },
  );

  test(
    'migrates the legacy single sort mode field into the new settings pair',
    () async {
      SharedPreferences.setMockInitialValues({
        SettingsRepository.endpointStorageKey:
            '{"httpBaseUrl":"http://localhost:8080","wsUrl":"ws://localhost:8081/ws","windowSortMode":"name"}',
      });
      final prefs = await SharedPreferences.getInstance();
      final repository = SettingsRepository(prefs);

      expect(
        repository.loadEndpointSettings()?.windowSortBy,
        WindowSortBy.name,
      );
      expect(
        repository.loadEndpointSettings()?.windowSortDirection,
        WindowSortDirection.ascending,
      );
    },
  );
}
