import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/widgets/desktop_panel.dart';
import '../../../core/utils/backend_state_helpers.dart';
import '../../audio/presentation/audio_section.dart';
import '../../media/presentation/media_section.dart';
import '../../settings/domain/endpoint_settings.dart';
import '../../settings/presentation/settings_sheet.dart';
import '../../windows/presentation/windows_section.dart';
import '../application/app_providers.dart';
import '../application/dashboard_controller.dart';
import '../domain/connection_status.dart';

class DashboardScreen extends ConsumerStatefulWidget {
  const DashboardScreen({super.key});

  @override
  ConsumerState<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends ConsumerState<DashboardScreen>
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
    if (state == AppLifecycleState.resumed) {
      ref.read(dashboardControllerProvider.notifier).handleAppResumed();
    }
  }

  @override
  Widget build(BuildContext context) {
    final settingsState = ref.watch(settingsControllerProvider);
    final endpointSettings = settingsState.endpointSettings;
    if (endpointSettings == null) {
      return const SizedBox.shrink();
    }

    final state = ref.watch(dashboardControllerProvider);
    final controller = ref.read(dashboardControllerProvider.notifier);
    final sinks = audioDevicesWithSelectedFirst(
      state.backendState.audio?.sinks ?? const [],
      state.backendState.audio?.selectedSinkId,
    );
    final sources = audioDevicesWithSelectedFirst(
      state.backendState.audio?.sources ?? const [],
      state.backendState.audio?.selectedSourceId,
    );
    return Scaffold(
      appBar: AppBar(
        title: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Plasma Remote Toolbar'),
            Text(
              'Breeze tablet controls',
              style: TextStyle(fontSize: 12, fontWeight: FontWeight.w500),
            ),
          ],
        ),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 6),
            child: Center(
              child: _ConnectionPill(status: state.connectionStatus),
            ),
          ),
          IconButton(
            onPressed: () {
              showModalBottomSheet<void>(
                context: context,
                isScrollControlled: true,
                useSafeArea: true,
                showDragHandle: true,
                builder: (_) => SettingsSheet(
                  settings: endpointSettings,
                  dashboardState: state,
                ),
              );
            },
            icon: const Icon(Icons.settings_rounded),
          ),
        ],
      ),
      body: LayoutBuilder(
        builder: (context, constraints) {
          final wideLayout = constraints.maxWidth >= 900;
          final content = _DashboardBody(
            key: ValueKey(
              wideLayout ? 'dashboard-wide-layout' : 'dashboard-narrow-layout',
            ),
            wideLayout: wideLayout,
            state: state,
            httpBaseUrl: endpointSettings.httpBaseUrl,
            windowSortBy: endpointSettings.windowSortBy,
            windowSortDirection: endpointSettings.windowSortDirection,
            sinks: sinks,
            sources: sources,
            controller: controller,
          );

          return RefreshIndicator(
            onRefresh: controller.reconnect,
            child: SingleChildScrollView(
              physics: const AlwaysScrollableScrollPhysics(),
              padding: const EdgeInsets.all(DesktopMetrics.pagePadding),
              child: ConstrainedBox(
                constraints: BoxConstraints(
                  minHeight:
                      constraints.maxHeight - (DesktopMetrics.pagePadding * 2),
                ),
                child: content,
              ),
            ),
          );
        },
      ),
    );
  }
}

class _DashboardBody extends StatelessWidget {
  const _DashboardBody({
    super.key,
    required this.wideLayout,
    required this.state,
    required this.httpBaseUrl,
    required this.windowSortBy,
    required this.windowSortDirection,
    required this.sinks,
    required this.sources,
    required this.controller,
  });

  final bool wideLayout;
  final DashboardState state;
  final String httpBaseUrl;
  final WindowSortBy windowSortBy;
  final WindowSortDirection windowSortDirection;
  final List<AudioDeviceState> sinks;
  final List<AudioDeviceState> sources;
  final DashboardController controller;

  @override
  Widget build(BuildContext context) {
    final windowsSection = WindowsSection(
      snapshot: state.backendState.windowState,
      httpBaseUrl: httpBaseUrl,
      sortBy: windowSortBy,
      sortDirection: windowSortDirection,
      pendingActions: state.pendingActions,
      errors: state.rowErrors,
      onActivateWindow: controller.activateWindow,
    );
    final mediaSection = MediaSection(
      player: state.backendState.media?.player,
      httpBaseUrl: httpBaseUrl,
      pendingActions: state.pendingActions,
      error: state.rowErrors['media'] ?? '',
      onPrevious: () => controller.performMediaAction('previous'),
      onTogglePlayPause: () => controller.performMediaAction('play-pause'),
      onNext: () => controller.performMediaAction('next'),
      onSeek: controller.seekCurrentMedia,
    );
    final sinksSection = AudioSection(
      title: 'Playback',
      devices: sinks,
      selectedId: state.backendState.audio?.selectedSinkId,
      volumeDrafts: state.volumeDrafts,
      pendingActions: state.pendingActions,
      errors: state.rowErrors,
      onVolumeDraftChange: controller.updateVolumeDraft,
      onVolumeCommit: controller.commitSinkVolume,
      onMuteToggle: controller.toggleSinkMuted,
    );
    final sourcesSection = AudioSection(
      title: 'Capture',
      devices: sources,
      selectedId: state.backendState.audio?.selectedSourceId,
      volumeDrafts: state.volumeDrafts,
      pendingActions: state.pendingActions,
      errors: state.rowErrors,
      volumeReadOnly: true,
      onVolumeDraftChange: controller.updateVolumeDraft,
      onMuteToggle: controller.toggleSourceMuted,
    );

    if (!wideLayout) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          windowsSection,
          const SizedBox(height: DesktopMetrics.sectionGap),
          mediaSection,
          const SizedBox(height: DesktopMetrics.sectionGap),
          _AudioGrid(
            wideLayout: false,
            sinksSection: sinksSection,
            sourcesSection: sourcesSection,
          ),
        ],
      );
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        windowsSection,
        const SizedBox(height: DesktopMetrics.sectionGap),
        mediaSection,
        const SizedBox(height: DesktopMetrics.sectionGap),
        _AudioGrid(
          wideLayout: true,
          sinksSection: sinksSection,
          sourcesSection: sourcesSection,
        ),
      ],
    );
  }
}

class _AudioGrid extends StatelessWidget {
  const _AudioGrid({
    required this.wideLayout,
    required this.sinksSection,
    required this.sourcesSection,
  });

  final bool wideLayout;
  final Widget sinksSection;
  final Widget sourcesSection;

  @override
  Widget build(BuildContext context) {
    if (!wideLayout) {
      return Column(
        children: [
          sinksSection,
          const SizedBox(height: DesktopMetrics.sectionGap),
          sourcesSection,
        ],
      );
    }

    return LayoutBuilder(
      builder: (context, constraints) {
        final sideBySide = constraints.maxWidth >= 620;
        if (!sideBySide) {
          return Column(
            children: [
              sinksSection,
              const SizedBox(height: DesktopMetrics.sectionGap),
              sourcesSection,
            ],
          );
        }

        return Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Expanded(child: sinksSection),
            const SizedBox(width: DesktopMetrics.sectionGap),
            Expanded(child: sourcesSection),
          ],
        );
      },
    );
  }
}

class _ConnectionPill extends StatelessWidget {
  const _ConnectionPill({required this.status});

  final ConnectionStatus status;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final color = switch (status) {
      ConnectionStatus.connected => theme.colorScheme.primary,
      ConnectionStatus.notReady => theme.colorScheme.tertiary,
      ConnectionStatus.connecting => theme.colorScheme.secondary,
      ConnectionStatus.disconnected ||
      ConnectionStatus.error => theme.colorScheme.error,
    };

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.14),
        borderRadius: BorderRadius.circular(999),
        border: Border.all(color: color.withValues(alpha: 0.2)),
      ),
      child: Text(
        status.label,
        style: TextStyle(
          color: color,
          fontWeight: FontWeight.w700,
          fontSize: 12,
        ),
      ),
    );
  }
}
