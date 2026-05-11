import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../core/network/endpoints.dart';
import '../../../core/widgets/desktop_panel.dart';
import '../../dashboard/application/app_providers.dart';
import '../../dashboard/application/settings_controller.dart';
import '../../settings/domain/endpoint_settings.dart';
import '../../settings/domain/theme_mode.dart';

class SetupScreen extends ConsumerStatefulWidget {
  const SetupScreen({super.key});

  @override
  ConsumerState<SetupScreen> createState() => _SetupScreenState();
}

class _SetupScreenState extends ConsumerState<SetupScreen> {
  late final TextEditingController _httpController;
  late final TextEditingController _wsController;
  WindowSortBy _windowSortBy = WindowSortBy.usage;
  WindowSortDirection _windowSortDirection = WindowSortDirection.newestFirst;
  String? _error;

  @override
  void initState() {
    super.initState();
    _httpController = TextEditingController(text: defaultHttpBaseUrl);
    _wsController = TextEditingController(text: defaultWsUrl);
  }

  @override
  void dispose() {
    _httpController.dispose();
    _wsController.dispose();
    super.dispose();
  }

  void _save() {
    try {
      ref
          .read(settingsControllerProvider.notifier)
          .saveEndpointSettings(
            EndpointSettings(
              httpBaseUrl: _httpController.text,
              wsUrl: _wsController.text,
              windowSortBy: _windowSortBy,
              windowSortDirection: _windowSortDirection,
            ),
          );
    } on FormatException catch (error) {
      setState(() {
        _error = error.message;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final settingsState = ref.watch(settingsControllerProvider);

    return Scaffold(
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 920),
            child: LayoutBuilder(
              builder: (context, constraints) {
                final wideLayout = constraints.maxWidth >= 760;

                return SingleChildScrollView(
                  padding: const EdgeInsets.all(DesktopMetrics.pagePadding),
                  child: wideLayout
                      ? Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Expanded(child: _buildIntroPanel(theme)),
                            const SizedBox(width: DesktopMetrics.sectionGap),
                            Expanded(
                              flex: 2,
                              child: _buildFormPanel(theme, settingsState),
                            ),
                          ],
                        )
                      : Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            _buildIntroPanel(theme),
                            const SizedBox(height: DesktopMetrics.sectionGap),
                            _buildFormPanel(theme, settingsState),
                          ],
                        ),
                );
              },
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildIntroPanel(ThemeData theme) {
    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              color: theme.colorScheme.primaryContainer,
              borderRadius: BorderRadius.circular(14),
            ),
            child: Icon(
              Icons.phonelink_setup_rounded,
              color: theme.colorScheme.onPrimaryContainer,
            ),
          ),
          const SizedBox(height: 18),
          Text(
            'Connect your Plasma desktop',
            style: theme.textTheme.headlineSmall,
          ),
          const SizedBox(height: 8),
          Text(
            'Set the desktop backend endpoints and choose a compact Breeze-like theme for the tablet remote.',
            style: theme.textTheme.bodyMedium,
          ),
          const SizedBox(height: 14),
          const PanelMessage(
            icon: Icons.router_rounded,
            tone: PanelMessageTone.info,
            message:
                'Use the backend machine\'s LAN address. Android devices usually cannot reach localhost on the desktop.',
          ),
        ],
      ),
    );
  }

  Widget _buildFormPanel(ThemeData theme, SettingsState settingsState) {
    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          PanelSectionHeader(
            title: 'Connection Settings',
            subtitle: 'Enter the backend endpoints',
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _httpController,
            keyboardType: TextInputType.url,
            decoration: const InputDecoration(
              labelText: 'HTTP endpoint',
              hintText: 'http://192.168.1.20:8080',
            ),
          ),
          const SizedBox(height: 12),
          TextField(
            controller: _wsController,
            keyboardType: TextInputType.url,
            decoration: const InputDecoration(
              labelText: 'WebSocket endpoint',
              hintText: 'ws://192.168.1.20:8081/ws',
            ),
          ),
          const SizedBox(height: 16),
          Text('Theme', style: theme.textTheme.titleSmall),
          const SizedBox(height: 10),
          SegmentedButton<AppThemeMode>(
            selected: {settingsState.themeMode},
            onSelectionChanged: (selection) {
              ref
                  .read(settingsControllerProvider.notifier)
                  .saveThemeMode(selection.first);
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
                _windowSortDirection = normalizeWindowSortDirection(
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
          if (_error != null) ...[
            const SizedBox(height: 12),
            Text(
              _error!,
              style: theme.textTheme.bodySmall?.copyWith(
                color: theme.colorScheme.error,
              ),
            ),
          ],
          const SizedBox(height: 16),
          FilledButton.icon(
            onPressed: _save,
            icon: const Icon(Icons.wifi_find_rounded),
            label: const Text('Save and connect'),
          ),
        ],
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
