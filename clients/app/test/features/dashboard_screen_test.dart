import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_bridge_app/app/app.dart';
import 'package:plasma_bridge_app/core/models/backend_models.dart';
import 'package:plasma_bridge_app/features/dashboard/application/app_providers.dart';
import 'package:plasma_bridge_app/features/dashboard/application/dashboard_controller.dart';
import 'package:plasma_bridge_app/features/dashboard/application/settings_controller.dart';
import 'package:plasma_bridge_app/features/dashboard/data/connection_repository.dart';
import 'package:plasma_bridge_app/features/dashboard/domain/connection_status.dart';
import 'package:plasma_bridge_app/features/settings/domain/endpoint_settings.dart';
import 'package:plasma_bridge_app/features/settings/domain/theme_mode.dart';

class _FakeSettingsController extends SettingsController {
  @override
  SettingsState build() {
    return const SettingsState(
      endpointSettings: EndpointSettings(
        httpBaseUrl: 'http://192.168.1.20:8080',
        wsUrl: 'ws://192.168.1.20:8081/ws',
        windowSortBy: WindowSortBy.usage,
        windowSortDirection: WindowSortDirection.newestFirst,
      ),
      themeMode: AppThemeMode.light,
    );
  }

  @override
  void saveEndpointSettings(EndpointSettings settings) {
    state = state.copyWith(endpointSettings: settings);
  }

  @override
  void saveThemeMode(AppThemeMode mode) {
    state = state.copyWith(themeMode: mode);
  }
}

class _FakeDashboardController extends DashboardController {
  @override
  DashboardState build() {
    return DashboardState(
      backendState: BackendState(
        audio: AudioState(
          sinks: const [
            AudioDeviceState(
              id: 'sink-1',
              label: 'Internal Speakers',
              volume: 0.72,
              muted: false,
              available: true,
              isDefault: true,
              isVirtual: false,
              backendApi: 'pulse',
            ),
          ],
          selectedSinkId: 'sink-1',
          sources: const [
            AudioDeviceState(
              id: 'source-1',
              label: 'Built-in Microphone',
              volume: 0.48,
              muted: false,
              available: true,
              isDefault: true,
              isVirtual: false,
              backendApi: 'pulse',
            ),
          ],
          selectedSourceId: 'source-1',
        ),
        media: MediaState(
          player: const MediaPlayerState(
            playerId: 'player-1',
            identity: 'Elisa',
            desktopEntry: 'org.kde.elisa',
            playbackStatus: MediaPlaybackStatus.playing,
            title: 'Breeze Theme',
            artists: ['KDE Demo Artist'],
            album: 'Tablet Session',
            trackLengthMs: 240000,
            positionMs: 42000,
            canPlay: true,
            canPause: true,
            canGoNext: true,
            canGoPrevious: true,
            canControl: true,
            canSeek: true,
            appIconUrl: null,
            artworkUrl: null,
          ),
        ),
        windowState: WindowSnapshot(
          activeWindowId: 'window-1',
          activeWindow: _windowState('window-1', 'Konsole', isActive: true),
          windows: [
            _windowState('window-1', 'Konsole', isActive: true),
            _windowState('window-2', 'System Settings'),
          ],
        ),
      ),
      connectionStatus: ConnectionStatus.connected,
      connectionDetail: 'Live state stream connected.',
      httpStatus: HttpCheckState.reachable,
      httpDetail: 'OpenAPI endpoint reachable.',
      pendingActions: const {},
      rowErrors: const {},
      volumeDrafts: const {},
    );
  }

  @override
  Future<void> reconnect() async {}

  @override
  Future<void> handleAppResumed() async {}

  @override
  Future<void> activateWindow(String windowId) async {}

  @override
  Future<void> performMediaAction(String action) async {}

  @override
  Future<void> seekCurrentMedia(int positionMs) async {}

  @override
  void updateVolumeDraft(String deviceId, double value) {}

  @override
  Future<void> commitSinkVolume(String sinkId, double value) async {}

  @override
  Future<void> toggleSinkMuted(AudioDeviceState device) async {}

  @override
  Future<void> toggleSourceMuted(AudioDeviceState device) async {}
}

WindowState _windowState(String id, String title, {bool isActive = false}) {
  return WindowState(
    id: id,
    title: title,
    appId: 'org.kde.demo',
    pid: 42,
    isActive: isActive,
    isMinimized: false,
    isMaximized: false,
    isFullscreen: false,
    isOnAllDesktops: false,
    skipTaskbar: false,
    skipSwitcher: false,
    geometry: const WindowGeometry(x: 0, y: 0, width: 100, height: 100),
    clientGeometry: const WindowGeometry(x: 0, y: 0, width: 100, height: 100),
    virtualDesktopIds: const [],
    activityIds: const [],
    parentId: null,
    resourceName: 'demo-app',
    iconUrl: null,
  );
}

void main() {
  Future<void> pumpDashboard(WidgetTester tester, Size size) async {
    tester.view.physicalSize = size;
    tester.view.devicePixelRatio = 1.0;
    addTearDown(tester.view.reset);

    await tester.pumpWidget(
      ProviderScope(
        overrides: [
          settingsControllerProvider.overrideWith(_FakeSettingsController.new),
          dashboardControllerProvider.overrideWith(
            _FakeDashboardController.new,
          ),
        ],
        child: const PlasmaRemoteToolbarApp(),
      ),
    );

    await tester.pumpAndSettle();
  }

  testWidgets('uses stacked dashboard layout on narrow widths', (tester) async {
    await pumpDashboard(tester, const Size(700, 1200));

    expect(
      find.byKey(const ValueKey('dashboard-narrow-layout')),
      findsOneWidget,
    );
    expect(find.text('Windows'), findsOneWidget);
    expect(find.text('Current Media'), findsOneWidget);
    expect(find.text('Playback'), findsOneWidget);
    expect(tester.takeException(), isNull);
  });

  testWidgets('uses two-column dashboard layout on tablet landscape', (
    tester,
  ) async {
    await pumpDashboard(tester, const Size(1280, 800));

    expect(find.byKey(const ValueKey('dashboard-wide-layout')), findsOneWidget);
    expect(find.text('Capture'), findsOneWidget);
    expect(find.text('Connection'), findsNothing);
    expect(find.text('Konsole'), findsWidgets);
    expect(find.text('Connected'), findsOneWidget);
    expect(tester.takeException(), isNull);
  });
}
