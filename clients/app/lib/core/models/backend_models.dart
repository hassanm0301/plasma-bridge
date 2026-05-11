class AudioDeviceState {
  const AudioDeviceState({
    required this.id,
    required this.label,
    required this.volume,
    required this.muted,
    required this.available,
    required this.isDefault,
    required this.isVirtual,
    required this.backendApi,
  });

  final String id;
  final String label;
  final double volume;
  final bool muted;
  final bool available;
  final bool isDefault;
  final bool isVirtual;
  final String? backendApi;
}

class AudioState {
  const AudioState({
    required this.sinks,
    required this.selectedSinkId,
    required this.sources,
    required this.selectedSourceId,
  });

  final List<AudioDeviceState> sinks;
  final String? selectedSinkId;
  final List<AudioDeviceState> sources;
  final String? selectedSourceId;
}

enum MediaPlaybackStatus {
  unknown,
  playing,
  paused,
  stopped;

  static MediaPlaybackStatus fromWireValue(String value) {
    return MediaPlaybackStatus.values.firstWhere(
      (status) => status.name == value,
      orElse: () => MediaPlaybackStatus.unknown,
    );
  }
}

class MediaPlayerState {
  const MediaPlayerState({
    required this.playerId,
    required this.identity,
    required this.desktopEntry,
    required this.playbackStatus,
    required this.title,
    required this.artists,
    required this.album,
    required this.trackLengthMs,
    required this.positionMs,
    required this.canPlay,
    required this.canPause,
    required this.canGoNext,
    required this.canGoPrevious,
    required this.canControl,
    required this.canSeek,
    required this.appIconUrl,
    required this.artworkUrl,
  });

  final String playerId;
  final String? identity;
  final String? desktopEntry;
  final MediaPlaybackStatus playbackStatus;
  final String? title;
  final List<String> artists;
  final String? album;
  final int? trackLengthMs;
  final int? positionMs;
  final bool canPlay;
  final bool canPause;
  final bool canGoNext;
  final bool canGoPrevious;
  final bool canControl;
  final bool canSeek;
  final String? appIconUrl;
  final String? artworkUrl;
}

class MediaState {
  const MediaState({required this.player});

  final MediaPlayerState? player;
}

class WindowGeometry {
  const WindowGeometry({
    required this.x,
    required this.y,
    required this.width,
    required this.height,
  });

  final int x;
  final int y;
  final int width;
  final int height;
}

class WindowState {
  const WindowState({
    required this.id,
    required this.title,
    required this.appId,
    required this.pid,
    required this.isActive,
    required this.isMinimized,
    required this.isMaximized,
    required this.isFullscreen,
    required this.isOnAllDesktops,
    required this.skipTaskbar,
    required this.skipSwitcher,
    required this.geometry,
    required this.clientGeometry,
    required this.virtualDesktopIds,
    required this.activityIds,
    required this.parentId,
    required this.resourceName,
    required this.iconUrl,
  });

  final String id;
  final String title;
  final String? appId;
  final int? pid;
  final bool isActive;
  final bool isMinimized;
  final bool isMaximized;
  final bool isFullscreen;
  final bool isOnAllDesktops;
  final bool skipTaskbar;
  final bool skipSwitcher;
  final WindowGeometry geometry;
  final WindowGeometry clientGeometry;
  final List<String> virtualDesktopIds;
  final List<String> activityIds;
  final String? parentId;
  final String? resourceName;
  final String? iconUrl;
}

class WindowSnapshot {
  const WindowSnapshot({
    required this.activeWindowId,
    required this.activeWindow,
    required this.windows,
  });

  final String? activeWindowId;
  final WindowState? activeWindow;
  final List<WindowState> windows;
}

class BackendState {
  const BackendState({
    required this.audio,
    required this.media,
    required this.windowState,
  });

  const BackendState.empty() : audio = null, media = null, windowState = null;

  final AudioState? audio;
  final MediaState? media;
  final WindowSnapshot? windowState;

  BackendState copyWith({
    AudioState? audio,
    MediaState? media,
    WindowSnapshot? windowState,
    bool keepAudio = true,
    bool keepMedia = true,
    bool keepWindowState = true,
  }) {
    return BackendState(
      audio: keepAudio ? (audio ?? this.audio) : audio,
      media: keepMedia ? (media ?? this.media) : media,
      windowState: keepWindowState
          ? (windowState ?? this.windowState)
          : windowState,
    );
  }
}

sealed class BackendMessage {
  const BackendMessage();
}

class FullStateMessage extends BackendMessage {
  const FullStateMessage({
    required this.audio,
    required this.media,
    required this.windowState,
  });

  final AudioState? audio;
  final MediaState? media;
  final WindowSnapshot? windowState;
}

class PatchChange<T> {
  const PatchChange({required this.path, required this.value});

  final String path;
  final T value;
}

class PatchMessage extends BackendMessage {
  const PatchMessage({required this.playerId, required this.changes});

  final String? playerId;
  final List<PatchChange<Object>> changes;
}

class ErrorMessage extends BackendMessage {
  const ErrorMessage({required this.code, required this.message});

  final String code;
  final String message;
}
