import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_remote_toolbar_app/core/models/backend_models.dart';
import 'package:plasma_remote_toolbar_app/core/utils/backend_state_helpers.dart';
import 'package:plasma_remote_toolbar_app/core/utils/media_helpers.dart';

AudioDeviceState device(String id, String label, {bool isDefault = false}) {
  return AudioDeviceState(
    id: id,
    label: label,
    volume: 0.5,
    muted: false,
    available: true,
    isDefault: isDefault,
    isVirtual: false,
    backendApi: 'alsa',
  );
}

WindowState windowState(
  String id,
  String title, {
  bool isActive = false,
  bool skipTaskbar = false,
}) {
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
    skipTaskbar: skipTaskbar,
    skipSwitcher: false,
    geometry: const WindowGeometry(x: 0, y: 0, width: 100, height: 100),
    clientGeometry: const WindowGeometry(x: 0, y: 0, width: 100, height: 100),
    virtualDesktopIds: const [],
    activityIds: const [],
    parentId: null,
    resourceName: null,
    iconUrl: null,
  );
}

MediaPlayerState mediaPlayer({
  MediaPlaybackStatus status = MediaPlaybackStatus.paused,
}) {
  return MediaPlayerState(
    playerId: 'org.mpris.MediaPlayer2.spotify',
    identity: 'Spotify',
    desktopEntry: 'spotify',
    playbackStatus: status,
    title: 'Track',
    artists: const ['Artist'],
    album: 'Album',
    trackLengthMs: 1000,
    positionMs: 200,
    canPlay: true,
    canPause: true,
    canGoNext: true,
    canGoPrevious: true,
    canControl: true,
    canSeek: true,
    appIconUrl: '/icons/apps/spotify.png',
    artworkUrl: 'https://example.test/artwork.jpg',
  );
}

void main() {
  test('stores full state messages', () {
    final nextState = applyBackendMessage(
      const BackendState.empty(),
      FullStateMessage(
        audio: const AudioState(
          sinks: [],
          selectedSinkId: null,
          sources: [],
          selectedSourceId: null,
        ),
        media: MediaState(player: mediaPlayer()),
        windowState: const WindowSnapshot(
          activeWindowId: null,
          activeWindow: null,
          windows: [],
        ),
      ),
    );

    expect(nextState.audio, isNotNull);
    expect(nextState.media?.player?.playerId, 'org.mpris.MediaPlayer2.spotify');
    expect(nextState.windowState, isNotNull);
  });

  test('applies top-level patch changes', () {
    final state = BackendState(
      audio: const AudioState(
        sinks: [],
        selectedSinkId: null,
        sources: [],
        selectedSourceId: null,
      ),
      media: null,
      windowState: null,
    );

    final nextState = applyBackendMessage(
      state,
      PatchMessage(
        playerId: null,
        changes: const [
          PatchChange<Object>(
            path: 'windowState',
            value: WindowSnapshot(
              activeWindowId: 'active',
              activeWindow: null,
              windows: [],
            ),
          ),
        ],
      ),
    );

    expect(nextState.audio, same(state.audio));
    expect(nextState.windowState?.activeWindowId, 'active');
  });

  test('sorts active taskbar windows first and hides skipped windows', () {
    final snapshot = WindowSnapshot(
      activeWindowId: 'active',
      activeWindow: windowState('active', 'Active', isActive: true),
      windows: [
        windowState('z', 'Zed'),
        windowState('hidden', 'Hidden', skipTaskbar: true),
        windowState('active', 'Active', isActive: true),
        windowState('a', 'Alpha'),
      ],
    );

    expect(windowsForTaskbar(snapshot).map((window) => window.id), [
      'active',
      'a',
      'z',
    ]);
  });

  test('sorts selected audio devices first', () {
    expect(
      audioDevicesWithSelectedFirst([
        device('b', 'Beta'),
        device('a', 'Alpha'),
      ], 'b').map((item) => item.id),
      ['b', 'a'],
    );
  });

  test('formats duration strings and clamps media positions', () {
    expect(formatDurationMs(62000), '1:02');
    expect(formatDurationMs(3723000), '1:02:03');
    expect(formatDurationMs(null), '--:--');
    expect(clampMediaPosition(42000, 60000), 42000);
    expect(clampMediaPosition(90000, 60000), 60000);
    expect(clampMediaPosition(-5, null), 0);
  });
}
