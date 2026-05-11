import '../models/backend_models.dart';

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

List<WindowState> windowsForTaskbar(WindowSnapshot? snapshot) {
  if (snapshot == null) {
    return const [];
  }

  final byId = <String, WindowState>{};
  for (final window in snapshot.windows) {
    if (!window.skipTaskbar) {
      byId[window.id] = window;
    }
  }
  final activeWindow = snapshot.activeWindow;
  if (activeWindow != null && !activeWindow.skipTaskbar) {
    byId[activeWindow.id] = activeWindow;
  }

  final windows = byId.values.toList(growable: false);
  windows.sort((left, right) {
    final leftActive = left.id == snapshot.activeWindowId || left.isActive;
    final rightActive = right.id == snapshot.activeWindowId || right.isActive;
    if (leftActive != rightActive) {
      return leftActive ? -1 : 1;
    }
    return displayWindowTitle(left).compareTo(displayWindowTitle(right));
  });
  return windows;
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
