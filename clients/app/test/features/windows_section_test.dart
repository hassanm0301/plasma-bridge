import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_bridge_app/core/models/backend_models.dart';
import 'package:plasma_bridge_app/features/settings/domain/endpoint_settings.dart';
import 'package:plasma_bridge_app/features/windows/presentation/windows_section.dart';

WindowState _windowState(String id, String title, {bool isActive = false}) {
  return WindowState(
    id: id,
    title: title,
    appId: 'org.example.App',
    pid: 10,
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
    resourceName: 'example-app',
    iconUrl: null,
  );
}

void main() {
  testWidgets(
    'renders window tiles without vertical overflow on small screens',
    (tester) async {
      tester.view.physicalSize = const Size(360, 640);
      tester.view.devicePixelRatio = 1.0;
      addTearDown(tester.view.reset);

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: Padding(
              padding: const EdgeInsets.all(16),
              child: WindowsSection(
                expanded: false,
                snapshot: WindowSnapshot(
                  activeWindowId: 'window-1',
                  activeWindow: _windowState(
                    'window-1',
                    'Very long application title that used to overflow',
                    isActive: true,
                  ),
                  windows: [
                    _windowState(
                      'window-1',
                      'Very long application title that used to overflow',
                      isActive: true,
                    ),
                    _windowState(
                      'window-2',
                      'Second long title for another desktop window',
                    ),
                  ],
                ),
                httpBaseUrl: 'http://127.0.0.1:8080',
                sortBy: WindowSortBy.usage,
                sortDirection: WindowSortDirection.newestFirst,
                pendingActions: const {},
                errors: const {'window-2': 'Activation failed'},
                onActivateWindow: (_) async {},
                onToggleExpanded: () {},
              ),
            ),
          ),
        ),
      );

      await tester.pumpAndSettle();

      expect(tester.takeException(), isNull);
      expect(find.text('Windows'), findsOneWidget);
      expect(find.text('org.example.App'), findsWidgets);
    },
  );

  testWidgets(
    'renders all windows in an expanded grid without horizontal scrolling',
    (tester) async {
      tester.view.physicalSize = const Size(900, 640);
      tester.view.devicePixelRatio = 1.0;
      addTearDown(tester.view.reset);

      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: Padding(
              padding: const EdgeInsets.all(16),
              child: WindowsSection(
                expanded: true,
                snapshot: WindowSnapshot(
                  activeWindowId: 'window-1',
                  activeWindow: _windowState(
                    'window-1',
                    'Very long application title that used to overflow',
                    isActive: true,
                  ),
                  windows: [
                    _windowState(
                      'window-1',
                      'Very long application title that used to overflow',
                      isActive: true,
                    ),
                    _windowState('window-2', 'Second long title'),
                    _windowState('window-3', 'Third title'),
                    _windowState('window-4', 'Fourth title'),
                  ],
                ),
                httpBaseUrl: 'http://127.0.0.1:8080',
                sortBy: WindowSortBy.usage,
                sortDirection: WindowSortDirection.newestFirst,
                pendingActions: const {},
                errors: const {},
                onActivateWindow: (_) async {},
                onToggleExpanded: () {},
              ),
            ),
          ),
        ),
      );

      await tester.pumpAndSettle();

      expect(tester.takeException(), isNull);
      expect(
        find.byKey(const ValueKey('windows-section-expanded-grid')),
        findsOneWidget,
      );
      expect(
        find.byKey(const ValueKey('windows-section-compact-list')),
        findsNothing,
      );
      expect(find.text('Third title'), findsOneWidget);
      expect(find.text('Fourth title'), findsOneWidget);
    },
  );
}
