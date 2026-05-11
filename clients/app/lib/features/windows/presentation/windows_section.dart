import 'package:flutter/material.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/utils/backend_state_helpers.dart';
import '../../../core/utils/url_utils.dart';
import '../../../core/widgets/desktop_panel.dart';
import '../../../core/widgets/remote_image.dart';

const _windowTileWidth = 146.0;
const _windowTileHeight = 88.0;

class WindowsSection extends StatelessWidget {
  const WindowsSection({
    super.key,
    required this.snapshot,
    required this.httpBaseUrl,
    required this.pendingActions,
    required this.errors,
    required this.onActivateWindow,
  });

  final WindowSnapshot? snapshot;
  final String httpBaseUrl;
  final Map<String, bool> pendingActions;
  final Map<String, String> errors;
  final Future<void> Function(String windowId) onActivateWindow;

  @override
  Widget build(BuildContext context) {
    final windows = windowsForTaskbar(snapshot);

    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          PanelSectionHeader(title: 'Windows', count: windows.length),
          const SizedBox(height: 8),
          if (windows.isEmpty)
            const PanelMessage(message: 'No windows reported yet.')
          else
            SizedBox(
              height: _windowTileHeight,
              child: ListView.separated(
                scrollDirection: Axis.horizontal,
                itemBuilder: (context, index) {
                  final window = windows[index];
                  final isActive =
                      window.id == snapshot?.activeWindowId || window.isActive;
                  final isPending =
                      pendingActions['window-active:${window.id}'] ?? false;
                  final appLabel =
                      window.appId ?? window.resourceName ?? 'Application';
                  final iconUrl = resolveAssetUrl(httpBaseUrl, window.iconUrl);
                  final content = _WindowTileContent(
                    title: displayWindowTitle(window),
                    subtitle: isPending ? 'Focusing...' : appLabel,
                    error: errors[window.id],
                    iconUrl: iconUrl,
                    fallbackLabel: appLabel,
                    isActive: isActive,
                  );

                  return SizedBox(
                    width: _windowTileWidth,
                    child: isActive
                        ? content
                        : InkWell(
                            borderRadius: BorderRadius.circular(
                              DesktopMetrics.itemRadius,
                            ),
                            onTap: isPending
                                ? null
                                : () => onActivateWindow(window.id),
                            child: content,
                          ),
                  );
                },
                separatorBuilder: (_, _) =>
                    const SizedBox(width: DesktopMetrics.sectionGap - 2),
                itemCount: windows.length,
              ),
            ),
        ],
      ),
    );
  }
}

class _WindowTileContent extends StatelessWidget {
  const _WindowTileContent({
    required this.title,
    required this.subtitle,
    required this.error,
    required this.iconUrl,
    required this.fallbackLabel,
    required this.isActive,
  });

  final String title;
  final String subtitle;
  final String? error;
  final String? iconUrl;
  final String fallbackLabel;
  final bool isActive;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final activeColor = isActive
        ? theme.colorScheme.primaryContainer.withValues(alpha: 0.82)
        : theme.colorScheme.surfaceContainerLowest.withValues(alpha: 0.72);
    final hasError = error != null && error!.isNotEmpty;

    return AnimatedContainer(
      duration: const Duration(milliseconds: 180),
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: activeColor,
        borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
        border: Border.all(
          color: isActive
              ? theme.colorScheme.primary.withValues(alpha: 0.3)
              : theme.colorScheme.outlineVariant.withValues(alpha: 0.8),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            width: 26,
            height: 26,
            decoration: BoxDecoration(
              color: theme.colorScheme.surfaceContainerHighest,
              borderRadius: BorderRadius.circular(8),
            ),
            clipBehavior: Clip.antiAlias,
            child: iconUrl == null
                ? Center(
                    child: Text(
                      fallbackLabel.characters.first.toUpperCase(),
                      style: theme.textTheme.titleSmall,
                    ),
                  )
                : RemoteImage(
                    url: iconUrl!,
                    fit: BoxFit.cover,
                    borderRadius: 8,
                    placeholder: Center(
                      child: Text(
                        fallbackLabel.characters.first.toUpperCase(),
                        style: theme.textTheme.titleSmall,
                      ),
                    ),
                  ),
          ),
          const SizedBox(height: 6),
          Text(
            title,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: theme.textTheme.titleSmall,
          ),
          const SizedBox(height: 2),
          Text(
            hasError ? error! : subtitle,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: hasError
                ? theme.textTheme.bodySmall?.copyWith(
                    color: theme.colorScheme.error,
                  )
                : theme.textTheme.bodySmall,
          ),
        ],
      ),
    );
  }
}
