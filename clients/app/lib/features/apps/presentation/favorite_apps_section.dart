import 'dart:async';

import 'package:flutter/material.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/utils/url_utils.dart';
import '../../../core/widgets/desktop_panel.dart';
import '../../../core/widgets/remote_image.dart';

class FavoriteAppsSection extends StatefulWidget {
  const FavoriteAppsSection({
    super.key,
    required this.favoriteApps,
    required this.searchResults,
    required this.addMode,
    required this.editMode,
    required this.searchQuery,
    required this.searchLoading,
    required this.error,
    required this.pendingActions,
    required this.httpBaseUrl,
    required this.onShowAddMode,
    required this.onHideAddMode,
    required this.onToggleEditMode,
    required this.onSearchChanged,
    required this.onOpenApp,
    required this.onAddFavorite,
    required this.onRemoveFavorite,
  });

  final List<AppInfo> favoriteApps;
  final List<AppInfo> searchResults;
  final bool addMode;
  final bool editMode;
  final String searchQuery;
  final bool searchLoading;
  final String error;
  final Map<String, bool> pendingActions;
  final String httpBaseUrl;
  final VoidCallback onShowAddMode;
  final VoidCallback onHideAddMode;
  final VoidCallback onToggleEditMode;
  final ValueChanged<String> onSearchChanged;
  final Future<void> Function(AppInfo app) onOpenApp;
  final Future<void> Function(AppInfo app) onAddFavorite;
  final Future<void> Function(AppInfo app) onRemoveFavorite;

  @override
  State<FavoriteAppsSection> createState() => _FavoriteAppsSectionState();
}

class _FavoriteAppsSectionState extends State<FavoriteAppsSection> {
  late final TextEditingController _searchController;

  @override
  void initState() {
    super.initState();
    _searchController = TextEditingController(text: widget.searchQuery);
  }

  @override
  void didUpdateWidget(covariant FavoriteAppsSection oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (_searchController.text != widget.searchQuery) {
      _searchController.value = TextEditingValue(
        text: widget.searchQuery,
        selection: TextSelection.collapsed(offset: widget.searchQuery.length),
      );
    }
  }

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          PanelSectionHeader(
            title: 'Favorite Apps',
            count: widget.favoriteApps.length,
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                IconButton(
                  key: const ValueKey('favorite-apps-add-button'),
                  tooltip: widget.addMode
                      ? 'Close add apps'
                      : 'Add favorite app',
                  visualDensity: VisualDensity.compact,
                  onPressed: widget.addMode
                      ? widget.onHideAddMode
                      : widget.onShowAddMode,
                  icon: Icon(
                    widget.addMode ? Icons.close_rounded : Icons.add_rounded,
                  ),
                ),
                const SizedBox(width: 4),
                IconButton(
                  key: const ValueKey('favorite-apps-edit-button'),
                  tooltip: widget.editMode
                      ? 'Done editing favorites'
                      : 'Edit favorites',
                  visualDensity: VisualDensity.compact,
                  onPressed: widget.favoriteApps.isEmpty && !widget.editMode
                      ? null
                      : widget.onToggleEditMode,
                  icon: Icon(
                    widget.editMode ? Icons.check_rounded : Icons.edit_rounded,
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 8),
          if (widget.error.isNotEmpty) ...[
            PanelMessage(message: widget.error, tone: PanelMessageTone.error),
            const SizedBox(height: 8),
          ],
          if (widget.addMode) ...[
            TextField(
              key: const ValueKey('favorite-apps-search-field'),
              controller: _searchController,
              autofocus: true,
              decoration: const InputDecoration(
                hintText: 'Search KDE apps',
                prefixIcon: Icon(Icons.search_rounded),
              ),
              onChanged: widget.onSearchChanged,
            ),
            const SizedBox(height: 10),
            _SearchResults(
              searchQuery: widget.searchQuery,
              searchLoading: widget.searchLoading,
              results: widget.searchResults,
              pendingActions: widget.pendingActions,
              httpBaseUrl: widget.httpBaseUrl,
              onAddFavorite: widget.onAddFavorite,
            ),
            if (widget.favoriteApps.isNotEmpty) const SizedBox(height: 12),
          ],
          if (widget.favoriteApps.isEmpty)
            const PanelMessage(
              message: 'Add your most-used KDE apps so they stay one tap away.',
              tone: PanelMessageTone.info,
              icon: Icons.apps_rounded,
            )
          else
            Wrap(
              spacing: 10,
              runSpacing: 10,
              children: widget.favoriteApps
                  .map(
                    (app) => _FavoriteAppTile(
                      key: ValueKey('favorite-app-tile:${app.appId}'),
                      app: app,
                      httpBaseUrl: widget.httpBaseUrl,
                      opening:
                          widget.pendingActions['app-open:${app.appId}'] ??
                          false,
                      removing:
                          widget
                              .pendingActions['app-unfavorite:${app.appId}'] ??
                          false,
                      editMode: widget.editMode,
                      onOpenApp: () {
                        if (widget.editMode) {
                          return;
                        }
                        unawaited(widget.onOpenApp(app));
                      },
                      onRemoveFavorite: () async {
                        final confirmed = await showDialog<bool>(
                          context: context,
                          builder: (context) {
                            return AlertDialog(
                              title: const Text('Remove favorite?'),
                              content: Text(
                                'Remove ${app.name} from your favorite apps?',
                              ),
                              actions: [
                                TextButton(
                                  onPressed: () =>
                                      Navigator.of(context).pop(false),
                                  child: const Text('Cancel'),
                                ),
                                FilledButton(
                                  onPressed: () =>
                                      Navigator.of(context).pop(true),
                                  child: const Text('Remove'),
                                ),
                              ],
                            );
                          },
                        );
                        if (confirmed == true && context.mounted) {
                          unawaited(widget.onRemoveFavorite(app));
                        }
                      },
                    ),
                  )
                  .toList(growable: false),
            ),
          if (widget.editMode && widget.favoriteApps.isNotEmpty) ...[
            const SizedBox(height: 10),
            Text(
              'Tap the remove button on any tile to update favorites.',
              style: theme.textTheme.bodySmall,
            ),
          ],
        ],
      ),
    );
  }
}

class _SearchResults extends StatelessWidget {
  const _SearchResults({
    required this.searchQuery,
    required this.searchLoading,
    required this.results,
    required this.pendingActions,
    required this.httpBaseUrl,
    required this.onAddFavorite,
  });

  final String searchQuery;
  final bool searchLoading;
  final List<AppInfo> results;
  final Map<String, bool> pendingActions;
  final String httpBaseUrl;
  final Future<void> Function(AppInfo app) onAddFavorite;

  @override
  Widget build(BuildContext context) {
    if (searchQuery.trim().isEmpty) {
      return const PanelMessage(
        message: 'Start typing to search the apps KDE can launch.',
      );
    }

    if (searchLoading) {
      return const Center(
        child: Padding(
          padding: EdgeInsets.symmetric(vertical: 12),
          child: CircularProgressIndicator(),
        ),
      );
    }

    if (results.isEmpty) {
      return const PanelMessage(message: 'No apps found.');
    }

    return Column(
      children: results
          .map(
            (app) => Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: _SearchResultTile(
                app: app,
                httpBaseUrl: httpBaseUrl,
                pending: pendingActions['app-favorite:${app.appId}'] ?? false,
                onTap: () {
                  unawaited(onAddFavorite(app));
                },
              ),
            ),
          )
          .toList(growable: false),
    );
  }
}

class _SearchResultTile extends StatelessWidget {
  const _SearchResultTile({
    required this.app,
    required this.httpBaseUrl,
    required this.pending,
    required this.onTap,
  });

  final AppInfo app;
  final String httpBaseUrl;
  final bool pending;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final iconUrl = resolveAssetUrl(httpBaseUrl, app.iconUrl);

    return Material(
      color: theme.colorScheme.surfaceContainerLow,
      borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
      child: InkWell(
        key: ValueKey('favorite-app-search-result:${app.appId}'),
        borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
        onTap: pending ? null : onTap,
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
          child: Row(
            children: [
              _AppIcon(iconUrl: iconUrl, fallbackLabel: app.name, size: 36),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      app.name,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: theme.textTheme.titleSmall,
                    ),
                    Text(
                      app.genericName.isNotEmpty
                          ? app.genericName
                          : app.desktopEntryName,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: theme.textTheme.bodySmall,
                    ),
                  ],
                ),
              ),
              const SizedBox(width: 8),
              pending
                  ? const SizedBox(
                      width: 18,
                      height: 18,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.add_circle_outline_rounded),
            ],
          ),
        ),
      ),
    );
  }
}

class _FavoriteAppTile extends StatelessWidget {
  static const _tileWidth = 172.0;

  const _FavoriteAppTile({
    super.key,
    required this.app,
    required this.httpBaseUrl,
    required this.opening,
    required this.removing,
    required this.editMode,
    required this.onOpenApp,
    required this.onRemoveFavorite,
  });

  final AppInfo app;
  final String httpBaseUrl;
  final bool opening;
  final bool removing;
  final bool editMode;
  final VoidCallback onOpenApp;
  final VoidCallback onRemoveFavorite;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final iconUrl = resolveAssetUrl(httpBaseUrl, app.iconUrl);
    final subtitle = opening
        ? 'Opening...'
        : app.genericName.isNotEmpty
        ? app.genericName
        : app.desktopEntryName;

    return SizedBox(
      width: _tileWidth,
      child: Material(
        color: theme.colorScheme.surfaceContainerLowest.withValues(alpha: 0.92),
        borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
        child: InkWell(
          borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
          onTap: opening || removing || editMode ? null : onOpenApp,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                _AppIcon(iconUrl: iconUrl, fallbackLabel: app.name, size: 36),
                const SizedBox(width: 10),
                Expanded(
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        app.name,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: theme.textTheme.titleSmall,
                      ),
                      const SizedBox(height: 2),
                      Text(
                        subtitle,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: theme.textTheme.bodySmall,
                      ),
                    ],
                  ),
                ),
                if (editMode) ...[
                  const SizedBox(width: 4),
                  IconButton(
                    key: ValueKey('favorite-app-remove:${app.appId}'),
                    tooltip: 'Remove ${app.name}',
                    visualDensity: VisualDensity.compact,
                    padding: EdgeInsets.zero,
                    constraints: const BoxConstraints.tightFor(
                      width: 28,
                      height: 28,
                    ),
                    onPressed: removing ? null : onRemoveFavorite,
                    icon: removing
                        ? const SizedBox(
                            width: 16,
                            height: 16,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(
                            Icons.remove_circle_outline_rounded,
                            size: 18,
                          ),
                  ),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _AppIcon extends StatelessWidget {
  const _AppIcon({
    required this.iconUrl,
    required this.fallbackLabel,
    required this.size,
  });

  final String? iconUrl;
  final String fallbackLabel;
  final double size;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final placeholder = Container(
      width: size,
      height: size,
      decoration: BoxDecoration(
        color: theme.colorScheme.primaryContainer,
        borderRadius: BorderRadius.circular(12),
      ),
      alignment: Alignment.center,
      child: Text(
        fallbackLabel.isEmpty ? '?' : fallbackLabel.substring(0, 1),
        style: theme.textTheme.titleMedium?.copyWith(
          color: theme.colorScheme.onPrimaryContainer,
          fontWeight: FontWeight.w700,
        ),
      ),
    );

    if (iconUrl == null) {
      return placeholder;
    }

    return SizedBox(
      width: size,
      height: size,
      child: RemoteImage(
        url: iconUrl!,
        fit: BoxFit.contain,
        borderRadius: 12,
        placeholder: placeholder,
      ),
    );
  }
}
