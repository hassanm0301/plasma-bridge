import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../core/widgets/desktop_panel.dart';
import '../../dashboard/application/app_providers.dart';
import '../../dashboard/application/dashboard_controller.dart';
import '../../dashboard/data/connection_repository.dart';
import '../../settings/domain/endpoint_settings.dart';
import '../../settings/domain/theme_mode.dart';

class SettingsSheet extends ConsumerStatefulWidget {
  const SettingsSheet({
    super.key,
    required this.settings,
    required this.dashboardState,
  });

  final EndpointSettings settings;
  final DashboardState dashboardState;

  @override
  ConsumerState<SettingsSheet> createState() => _SettingsSheetState();
}

class _SettingsSheetState extends ConsumerState<SettingsSheet> {
  late final TextEditingController _httpController;
  late final TextEditingController _wsController;
  late AppThemeMode _themeMode;
  late WindowSortBy _windowSortBy;
  late WindowSortDirection _windowSortDirection;
  late AppLaunchBehavior _appLaunchBehavior;
  String? _error;

  @override
  void initState() {
    super.initState();
    _httpController = TextEditingController(text: widget.settings.httpBaseUrl);
    _wsController = TextEditingController(text: widget.settings.wsUrl);
    _themeMode = ref.read(settingsControllerProvider).themeMode;
    _windowSortBy = widget.settings.windowSortBy;
    _windowSortDirection = widget.settings.windowSortDirection;
    _appLaunchBehavior = widget.settings.appLaunchBehavior;
  }

  @override
  void dispose() {
    _httpController.dispose();
    _wsController.dispose();
    super.dispose();
  }

  String _httpLabel(HttpCheckState state) {
    switch (state) {
      case HttpCheckState.idle:
        return 'Idle';
      case HttpCheckState.checking:
        return 'Checking';
      case HttpCheckState.reachable:
        return 'Reachable';
      case HttpCheckState.unreachable:
        return 'Unreachable';
    }
  }

  void _save() {
    try {
      ref.read(settingsControllerProvider.notifier).saveThemeMode(_themeMode);
      ref
          .read(settingsControllerProvider.notifier)
          .saveEndpointSettings(
            EndpointSettings(
              httpBaseUrl: _httpController.text,
              wsUrl: _wsController.text,
              windowSortBy: _windowSortBy,
              windowSortDirection: _windowSortDirection,
              appLaunchBehavior: _appLaunchBehavior,
            ),
          );
      Navigator.of(context).pop();
    } on FormatException catch (error) {
      setState(() {
        _error = error.message;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final showAndroidLaunchBehavior =
        !kIsWeb && defaultTargetPlatform == TargetPlatform.android;

    return SafeArea(
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 760),
          child: Padding(
            padding: EdgeInsets.only(
              left: 16,
              right: 16,
              top: 8,
              bottom: 16 + MediaQuery.of(context).viewInsets.bottom,
            ),
            child: SingleChildScrollView(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                mainAxisSize: MainAxisSize.min,
                children: [
                  Row(
                    children: [
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'Settings',
                              style: theme.textTheme.headlineSmall,
                            ),
                            const SizedBox(height: 2),
                            Text(
                              'Tablet dashboard configuration',
                              style: theme.textTheme.bodySmall,
                            ),
                          ],
                        ),
                      ),
                      IconButton(
                        onPressed: () => Navigator.of(context).pop(),
                        icon: const Icon(Icons.close_rounded),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  DesktopPanel(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const PanelSectionHeader(
                          title: 'Endpoints',
                          subtitle: 'Reconnect the dashboard after saving',
                        ),
                        const SizedBox(height: 12),
                        TextField(
                          controller: _httpController,
                          keyboardType: TextInputType.url,
                          decoration: const InputDecoration(
                            labelText: 'HTTP endpoint',
                          ),
                        ),
                        const SizedBox(height: 12),
                        TextField(
                          controller: _wsController,
                          keyboardType: TextInputType.url,
                          decoration: const InputDecoration(
                            labelText: 'WebSocket endpoint',
                          ),
                        ),
                        const SizedBox(height: 16),
                        Text('Theme', style: theme.textTheme.titleSmall),
                        const SizedBox(height: 10),
                        SegmentedButton<AppThemeMode>(
                          selected: {_themeMode},
                          onSelectionChanged: (selection) {
                            setState(() {
                              _themeMode = selection.first;
                            });
                          },
                          segments: const [
                            ButtonSegment(
                              value: AppThemeMode.light,
                              label: Text('Light'),
                              icon: Icon(Icons.light_mode_rounded),
                            ),
                            ButtonSegment(
                              value: AppThemeMode.dark,
                              label: Text('Dark'),
                              icon: Icon(Icons.dark_mode_rounded),
                            ),
                          ],
                        ),
                        const SizedBox(height: 16),
                        Text('Window Sort', style: theme.textTheme.titleSmall),
                        const SizedBox(height: 10),
                        SegmentedButton<WindowSortBy>(
                          selected: {_windowSortBy},
                          onSelectionChanged: (selection) {
                            setState(() {
                              _windowSortBy = selection.first;
                              _windowSortDirection =
                                  normalizeWindowSortDirection(
                                    _windowSortBy,
                                    _windowSortDirection,
                                  );
                            });
                          },
                          segments: const [
                            ButtonSegment(
                              value: WindowSortBy.usage,
                              label: Text('Usage'),
                              icon: Icon(Icons.history_rounded),
                            ),
                            ButtonSegment(
                              value: WindowSortBy.name,
                              label: Text('Name'),
                              icon: Icon(Icons.sort_by_alpha_rounded),
                            ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        Text('Direction', style: theme.textTheme.titleSmall),
                        const SizedBox(height: 10),
                        SegmentedButton<WindowSortDirection>(
                          selected: {_windowSortDirection},
                          onSelectionChanged: (selection) {
                            setState(() {
                              _windowSortDirection = selection.first;
                            });
                          },
                          segments: _directionSegments(_windowSortBy),
                        ),
                        if (showAndroidLaunchBehavior) ...[
                          const SizedBox(height: 16),
                          Text(
                            'Favorite App Launch',
                            style: theme.textTheme.titleSmall,
                          ),
                          const SizedBox(height: 10),
                          SegmentedButton<AppLaunchBehavior>(
                            selected: {_appLaunchBehavior},
                            onSelectionChanged: (selection) {
                              setState(() {
                                _appLaunchBehavior = selection.first;
                              });
                            },
                            segments: const [
                              ButtonSegment(
                                value: AppLaunchBehavior.openNewInstance,
                                label: Text('New instance'),
                                icon: Icon(Icons.open_in_new_rounded),
                              ),
                              ButtonSegment(
                                value: AppLaunchBehavior.switchToExisting,
                                label: Text('Switch existing'),
                                icon: Icon(Icons.flip_to_front_rounded),
                              ),
                            ],
                          ),
                        ],
                      ],
                    ),
                  ),
                  const SizedBox(height: 12),
                  LayoutBuilder(
                    builder: (context, constraints) {
                      final stacked = constraints.maxWidth < 520;
                      final httpCard = _StatusCard(
                        label: 'HTTP',
                        value: _httpLabel(widget.dashboardState.httpStatus),
                        detail: widget.dashboardState.httpDetail,
                      );
                      final webSocketCard = _StatusCard(
                        label: 'WebSocket',
                        value: widget.dashboardState.connectionStatus.label,
                        detail: widget.dashboardState.connectionDetail,
                      );

                      if (stacked) {
                        return Column(
                          children: [
                            httpCard,
                            const SizedBox(height: 12),
                            webSocketCard,
                          ],
                        );
                      }

                      return Row(
                        children: [
                          Expanded(child: httpCard),
                          const SizedBox(width: 12),
                          Expanded(child: webSocketCard),
                        ],
                      );
                    },
                  ),
                  if (_error != null) ...[
                    const SizedBox(height: 12),
                    Text(
                      _error!,
                      style: theme.textTheme.bodySmall?.copyWith(
                        color: theme.colorScheme.error,
                      ),
                    ),
                  ],
                  const SizedBox(height: 14),
                  SizedBox(
                    width: double.infinity,
                    child: FilledButton(
                      onPressed: _save,
                      child: const Text('Save/Reconnect'),
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  List<ButtonSegment<WindowSortDirection>> _directionSegments(
    WindowSortBy sortBy,
  ) {
    switch (sortBy) {
      case WindowSortBy.usage:
        return const [
          ButtonSegment(
            value: WindowSortDirection.newestFirst,
            label: Text('Newest first'),
            icon: Icon(Icons.keyboard_double_arrow_left_rounded),
          ),
          ButtonSegment(
            value: WindowSortDirection.oldestFirst,
            label: Text('Oldest first'),
            icon: Icon(Icons.keyboard_double_arrow_right_rounded),
          ),
        ];
      case WindowSortBy.name:
        return const [
          ButtonSegment(
            value: WindowSortDirection.ascending,
            label: Text('A-Z'),
            icon: Icon(Icons.arrow_upward_rounded),
          ),
          ButtonSegment(
            value: WindowSortDirection.descending,
            label: Text('Z-A'),
            icon: Icon(Icons.arrow_downward_rounded),
          ),
        ];
    }
  }
}

class _StatusCard extends StatelessWidget {
  const _StatusCard({
    required this.label,
    required this.value,
    required this.detail,
  });

  final String label;
  final String value;
  final String detail;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return DesktopPanel(
      padding: const EdgeInsets.all(12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(label, style: theme.textTheme.labelLarge),
          const SizedBox(height: 6),
          Text(value, style: theme.textTheme.titleSmall),
          const SizedBox(height: 4),
          Text(detail, style: theme.textTheme.bodySmall),
        ],
      ),
    );
  }
}
