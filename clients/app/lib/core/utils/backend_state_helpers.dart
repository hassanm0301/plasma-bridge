import '../models/backend_models.dart';
import '../../features/settings/domain/endpoint_settings.dart';

BackendState applyBackendMessage(BackendState state, BackendMessage message) {
  if (message is FullStateMessage) {
    return BackendState(
      audio: message.audio ?? state.audio,
      media: message.media ?? state.media,
      windowState: message.windowState ?? state.windowState,
    );
  }

  if (message is! PatchMessage) {
    return state;
  }

  var nextState = state;
  for (final change in message.changes) {
    switch (change.path) {
      case 'audio':
        nextState = BackendState(
          audio: change.value as AudioState,
          media: nextState.media,
          windowState: nextState.windowState,
        );
      case 'media':
        nextState = BackendState(
          audio: nextState.audio,
          media: change.value as MediaState,
          windowState: nextState.windowState,
        );
      case 'windowState':
        nextState = BackendState(
          audio: nextState.audio,
          media: nextState.media,
          windowState: change.value as WindowSnapshot,
        );
    }
  }
  return nextState;
}

List<WindowState> windowsForTaskbar(
  WindowSnapshot? snapshot, {
  WindowSortBy sortBy = WindowSortBy.usage,
  WindowSortDirection sortDirection = WindowSortDirection.newestFirst,
}) {
  if (snapshot == null) {
    return const [];
  }

  final windows = <WindowState>[];
  final seenIds = <String>{};
  for (final window in snapshot.windows) {
    if (!window.skipTaskbar && seenIds.add(window.id)) {
      windows.add(window);
    }
  }
  final activeWindow = snapshot.activeWindow;
  if (activeWindow != null &&
      !activeWindow.skipTaskbar &&
      seenIds.add(activeWindow.id)) {
    windows.add(activeWindow);
  }

  final activeWindowId = snapshot.activeWindowId ?? activeWindow?.id;
  if (sortBy == WindowSortBy.name) {
    final ordered = [...windows];
    final normalizedDirection = normalizeWindowSortDirection(
      sortBy,
      sortDirection,
    );
    ordered.sort((left, right) {
      final titleComparison = displayWindowTitle(
        left,
      ).toLowerCase().compareTo(displayWindowTitle(right).toLowerCase());
      if (titleComparison != 0) {
        return normalizedDirection == WindowSortDirection.ascending
            ? titleComparison
            : -titleComparison;
      }
      final idComparison = left.id.toLowerCase().compareTo(
        right.id.toLowerCase(),
      );
      return normalizedDirection == WindowSortDirection.ascending
          ? idComparison
          : -idComparison;
    });
    return ordered;
  }

  final normalizedDirection = normalizeWindowSortDirection(
    sortBy,
    sortDirection,
  );
  final ordered = normalizedDirection == WindowSortDirection.newestFirst
      ? windows.reversed.toList()
      : List<WindowState>.from(windows);
  if (activeWindowId == null || ordered.isEmpty) {
    return ordered;
  }

  final activeIndex = ordered.indexWhere(
    (window) => window.id == activeWindowId,
  );
  if (activeIndex < 0) {
    return ordered;
  }

  final active = ordered.removeAt(activeIndex);
  if (normalizedDirection == WindowSortDirection.newestFirst) {
    ordered.insert(0, active);
  } else {
    ordered.add(active);
  }
  return ordered;
}

List<AudioDeviceState> audioDevicesWithSelectedFirst(
  List<AudioDeviceState> devices,
  String? selectedId,
) {
  final items = [...devices];
  items.sort((left, right) {
    final leftSelected = left.id == selectedId || left.isDefault;
    final rightSelected = right.id == selectedId || right.isDefault;
    if (leftSelected != rightSelected) {
      return leftSelected ? -1 : 1;
    }
    return left.label.compareTo(right.label);
  });
  return items;
}

String displayWindowTitle(WindowState window) {
  return window.title.isNotEmpty
      ? window.title
      : (window.appId ?? window.resourceName ?? 'Untitled window');
}
