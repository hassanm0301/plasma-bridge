import { describe, expect, it } from "vitest";

import type { AudioDeviceState, BackendState, MediaPlayerState, WindowSnapshot, WindowState } from "./models";
import {
  applyBackendMessage,
  audioDevicesWithSelectedFirst,
  clampMediaPosition,
  formatDurationMs,
  mediaToggleAction,
  mediaPrimaryActionLabel,
  emptyBackendState,
  windowsForTaskbar
} from "./state";

const device = (id: string, label: string, isDefault = false): AudioDeviceState => ({
  id,
  label,
  volume: 0.5,
  muted: false,
  available: true,
  isDefault,
  isVirtual: false,
  backendApi: "alsa"
});

const windowState = (id: string, title: string, isActive = false, skipTaskbar = false): WindowState => ({
  id,
  title,
  appId: "org.example.App",
  pid: 10,
  isActive,
  isMinimized: false,
  isMaximized: false,
  isFullscreen: false,
  isOnAllDesktops: false,
  skipTaskbar,
  skipSwitcher: false,
  geometry: { x: 0, y: 0, width: 100, height: 100 },
  clientGeometry: { x: 0, y: 0, width: 100, height: 100 },
  virtualDesktopIds: [],
  activityIds: [],
  parentId: null,
  resourceName: null,
  iconUrl: null
});

const mediaPlayer = (overrides: Partial<MediaPlayerState> = {}): MediaPlayerState => ({
  playerId: "org.mpris.MediaPlayer2.spotify",
  identity: "Spotify",
  desktopEntry: "spotify",
  playbackStatus: "paused",
  title: "Track",
  artists: ["Artist"],
  album: "Album",
  trackLengthMs: 1000,
  positionMs: 200,
  canPlay: true,
  canPause: true,
  canGoNext: true,
  canGoPrevious: true,
  canControl: true,
  canSeek: true,
  appIconUrl: "/icons/apps/spotify.png",
  artworkUrl: "https://i.scdn.co/image/example",
  ...overrides
});

describe("backend state helpers", () => {
  it("stores full state messages", () => {
    const nextState = applyBackendMessage(emptyBackendState, {
      type: "fullState",
      payload: {
        audio: { sinks: [], selectedSinkId: null, sources: [], selectedSourceId: null },
        media: { player: mediaPlayer() },
        windowState: { activeWindowId: null, activeWindow: null, windows: [] }
      },
      error: null
    });

    expect(nextState.audio).not.toBeNull();
    expect(nextState.media?.player?.playerId).toBe("org.mpris.MediaPlayer2.spotify");
    expect(nextState.windowState).not.toBeNull();
  });

  it("applies top-level patch changes", () => {
    const state: BackendState = {
      audio: { sinks: [], selectedSinkId: null, sources: [], selectedSourceId: null },
      media: null,
      windowState: null
    };

    const nextState = applyBackendMessage(state, {
      type: "patch",
      payload: {
        changes: [
          {
            path: "windowState",
            value: { activeWindowId: "active", activeWindow: null, windows: [] }
          }
        ]
      },
      error: null
    });

    expect(nextState.audio).toBe(state.audio);
    expect(nextState.windowState?.activeWindowId).toBe("active");
  });

  it("applies media patch changes", () => {
    const state: BackendState = {
      audio: null,
      media: null,
      windowState: null
    };

    const nextState = applyBackendMessage(state, {
      type: "patch",
      payload: {
        playerId: "org.mpris.MediaPlayer2.spotify",
        changes: [
          {
            path: "media",
            value: { player: mediaPlayer({ playbackStatus: "playing" }) }
          }
        ]
      },
      error: null
    });

    expect(nextState.media?.player?.playbackStatus).toBe("playing");
    expect(nextState.audio).toBeNull();
    expect(nextState.windowState).toBeNull();
  });

  it("sorts active taskbar windows first and hides skipped windows", () => {
    const snapshot: WindowSnapshot = {
      activeWindowId: "active",
      activeWindow: windowState("active", "Active", true),
      windows: [
        windowState("z", "Zed"),
        windowState("hidden", "Hidden", false, true),
        windowState("active", "Active", true),
        windowState("a", "Alpha")
      ]
    };

    expect(windowsForTaskbar(snapshot).map((window) => window.id)).toEqual(["active", "a", "z"]);
  });

  it("sorts selected audio devices first", () => {
    expect(audioDevicesWithSelectedFirst([device("b", "Beta"), device("a", "Alpha")], "b").map((item) => item.id)).toEqual([
      "b",
      "a"
    ]);
  });

  it("uses a play-pause toggle action for the primary media control", () => {
    expect(mediaToggleAction(mediaPlayer({ playbackStatus: "playing" }))).toBe("play-pause");
    expect(mediaPrimaryActionLabel(mediaPlayer({ playbackStatus: "playing" }))).toBe("Pause");
  });

  it("returns no toggle action when media control is unavailable", () => {
    expect(mediaToggleAction(mediaPlayer({ canControl: false }))).toBeNull();
  });

  it("formats duration strings", () => {
    expect(formatDurationMs(62000)).toBe("1:02");
    expect(formatDurationMs(3723000)).toBe("1:02:03");
    expect(formatDurationMs(null)).toBe("--:--");
  });

  it("clamps media positions against track length", () => {
    expect(clampMediaPosition(42000.8, 60000)).toBe(42000);
    expect(clampMediaPosition(90000, 60000)).toBe(60000);
    expect(clampMediaPosition(-5, null)).toBe(0);
  });
});
