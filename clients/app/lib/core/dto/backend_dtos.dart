import '../models/backend_models.dart';

Map<String, Object?> _asObjectMap(Object? value) {
  return (value as Map).cast<String, Object?>();
}

List<Object?> _asObjectList(Object? value) {
  return value is List ? List<Object?>.from(value) : const [];
}

class AudioDeviceStateDto {
  const AudioDeviceStateDto(this.json);

  final Map<String, Object?> json;

  AudioDeviceState toDomain() {
    return AudioDeviceState(
      id: json['id'] as String,
      label: json['label'] as String,
      volume: (json['volume'] as num).toDouble(),
      muted: json['muted'] as bool,
      available: json['available'] as bool,
      isDefault: json['isDefault'] as bool,
      isVirtual: json['isVirtual'] as bool,
      backendApi: json['backendApi'] as String?,
    );
  }
}

class AudioStateDto {
  const AudioStateDto(this.json);

  final Map<String, Object?> json;

  AudioState toDomain() {
    return AudioState(
      sinks: _asObjectList(json['sinks'])
          .map((item) => AudioDeviceStateDto(_asObjectMap(item)).toDomain())
          .toList(growable: false),
      selectedSinkId: json['selectedSinkId'] as String?,
      sources: _asObjectList(json['sources'])
          .map((item) => AudioDeviceStateDto(_asObjectMap(item)).toDomain())
          .toList(growable: false),
      selectedSourceId: json['selectedSourceId'] as String?,
    );
  }
}

class MediaPlayerStateDto {
  const MediaPlayerStateDto(this.json);

  final Map<String, Object?> json;

  MediaPlayerState toDomain() {
    return MediaPlayerState(
      playerId: json['playerId'] as String,
      identity: json['identity'] as String?,
      desktopEntry: json['desktopEntry'] as String?,
      playbackStatus: MediaPlaybackStatus.fromWireValue(
        json['playbackStatus'] as String,
      ),
      title: json['title'] as String?,
      artists: _asObjectList(
        json['artists'],
      ).map((item) => item as String).toList(growable: false),
      album: json['album'] as String?,
      trackLengthMs: (json['trackLengthMs'] as num?)?.toInt(),
      positionMs: (json['positionMs'] as num?)?.toInt(),
      canPlay: json['canPlay'] as bool,
      canPause: json['canPause'] as bool,
      canGoNext: json['canGoNext'] as bool,
      canGoPrevious: json['canGoPrevious'] as bool,
      canControl: json['canControl'] as bool,
      canSeek: json['canSeek'] as bool,
      appIconUrl: json['appIconUrl'] as String?,
      artworkUrl: json['artworkUrl'] as String?,
    );
  }
}

class MediaStateDto {
  const MediaStateDto(this.json);

  final Map<String, Object?> json;

  MediaState toDomain() {
    final playerValue = json['player'];
    return MediaState(
      player: playerValue == null
          ? null
          : MediaPlayerStateDto(_asObjectMap(playerValue)).toDomain(),
    );
  }
}

class WindowGeometryDto {
  const WindowGeometryDto(this.json);

  final Map<String, Object?> json;

  WindowGeometry toDomain() {
    return WindowGeometry(
      x: (json['x'] as num).toInt(),
      y: (json['y'] as num).toInt(),
      width: (json['width'] as num).toInt(),
      height: (json['height'] as num).toInt(),
    );
  }
}

class WindowStateDto {
  const WindowStateDto(this.json);

  final Map<String, Object?> json;

  WindowState toDomain() {
    return WindowState(
      id: json['id'] as String,
      title: json['title'] as String,
      appId: json['appId'] as String?,
      pid: (json['pid'] as num?)?.toInt(),
      isActive: json['isActive'] as bool,
      isMinimized: json['isMinimized'] as bool,
      isMaximized: json['isMaximized'] as bool,
      isFullscreen: json['isFullscreen'] as bool,
      isOnAllDesktops: json['isOnAllDesktops'] as bool,
      skipTaskbar: json['skipTaskbar'] as bool,
      skipSwitcher: json['skipSwitcher'] as bool,
      geometry: WindowGeometryDto(_asObjectMap(json['geometry'])).toDomain(),
      clientGeometry: WindowGeometryDto(
        _asObjectMap(json['clientGeometry']),
      ).toDomain(),
      virtualDesktopIds: _asObjectList(
        json['virtualDesktopIds'],
      ).map((item) => item as String).toList(growable: false),
      activityIds: _asObjectList(
        json['activityIds'],
      ).map((item) => item as String).toList(growable: false),
      parentId: json['parentId'] as String?,
      resourceName: json['resourceName'] as String?,
      iconUrl: json['iconUrl'] as String?,
    );
  }
}

class WindowSnapshotDto {
  const WindowSnapshotDto(this.json);

  final Map<String, Object?> json;

  WindowSnapshot toDomain() {
    final activeWindowValue = json['activeWindow'];

    return WindowSnapshot(
      activeWindowId: json['activeWindowId'] as String?,
      activeWindow: activeWindowValue == null
          ? null
          : WindowStateDto(_asObjectMap(activeWindowValue)).toDomain(),
      windows: _asObjectList(json['windows'])
          .map((item) => WindowStateDto(_asObjectMap(item)).toDomain())
          .toList(growable: false),
    );
  }
}

class BackendMessageDto {
  const BackendMessageDto._();

  static BackendMessage parse(Map<String, Object?> json) {
    final type = json['type'] as String?;
    if (type == 'error') {
      final error = _asObjectMap(json['error']);
      return ErrorMessage(
        code: error['code'] as String? ?? 'unknown_error',
        message: error['message'] as String? ?? 'Unknown backend error.',
      );
    }

    final payload = _asObjectMap(json['payload']);

    if (type == 'fullState') {
      final audioValue = payload['audio'];
      final mediaValue = payload['media'];
      final windowStateValue = payload['windowState'];
      return FullStateMessage(
        audio: audioValue == null
            ? null
            : AudioStateDto(_asObjectMap(audioValue)).toDomain(),
        media: mediaValue == null
            ? null
            : MediaStateDto(_asObjectMap(mediaValue)).toDomain(),
        windowState: windowStateValue == null
            ? null
            : WindowSnapshotDto(_asObjectMap(windowStateValue)).toDomain(),
      );
    }

    if (type == 'patch') {
      final changes = _asObjectList(payload['changes'])
          .map((item) {
            final change = _asObjectMap(item);
            final path = change['path'] as String;
            final value = change['value'];
            if (path == 'audio') {
              return PatchChange<Object>(
                path: path,
                value: AudioStateDto(_asObjectMap(value)).toDomain(),
              );
            }
            if (path == 'media') {
              return PatchChange<Object>(
                path: path,
                value: MediaStateDto(_asObjectMap(value)).toDomain(),
              );
            }
            return PatchChange<Object>(
              path: path,
              value: WindowSnapshotDto(_asObjectMap(value)).toDomain(),
            );
          })
          .toList(growable: false);

      return PatchMessage(
        playerId: payload['playerId'] as String?,
        changes: changes,
      );
    }

    return const ErrorMessage(
      code: 'invalid_message',
      message: 'Unexpected WebSocket message type.',
    );
  }
}
