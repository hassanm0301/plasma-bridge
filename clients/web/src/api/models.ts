export interface AudioDeviceState {
  id: string;
  label: string;
  volume: number;
  muted: boolean;
  available: boolean;
  isDefault: boolean;
  isVirtual: boolean;
  backendApi: string | null;
}

export interface AudioState {
  sinks: AudioDeviceState[];
  selectedSinkId: string | null;
  sources: AudioDeviceState[];
  selectedSourceId: string | null;
}

export type MediaPlaybackStatus = "unknown" | "playing" | "paused" | "stopped";

export interface MediaPlayerState {
  playerId: string;
  identity: string | null;
  desktopEntry: string | null;
  playbackStatus: MediaPlaybackStatus;
  title: string | null;
  artists: string[];
  album: string | null;
  trackLengthMs: number | null;
  positionMs: number | null;
  canPlay: boolean;
  canPause: boolean;
  canGoNext: boolean;
  canGoPrevious: boolean;
  canControl: boolean;
  canSeek: boolean;
  appIconUrl: string | null;
  artworkUrl: string | null;
}

export interface MediaState {
  player: MediaPlayerState | null;
}

export interface WindowGeometry {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface WindowState {
  id: string;
  title: string;
  appId: string | null;
  pid: number | null;
  isActive: boolean;
  isMinimized: boolean;
  isMaximized: boolean;
  isFullscreen: boolean;
  isOnAllDesktops: boolean;
  skipTaskbar: boolean;
  skipSwitcher: boolean;
  geometry: WindowGeometry;
  clientGeometry: WindowGeometry;
  virtualDesktopIds: string[];
  activityIds: string[];
  parentId: string | null;
  resourceName: string | null;
  iconUrl: string | null;
}

export interface WindowSnapshot {
  activeWindowId: string | null;
  activeWindow: WindowState | null;
  windows: WindowState[];
}

export interface BackendState {
  audio: AudioState | null;
  media: MediaState | null;
  windowState: WindowSnapshot | null;
}

export interface FullStateMessage {
  type: "fullState";
  payload: {
    audio?: AudioState;
    media?: MediaState;
    windowState?: WindowSnapshot;
  };
  error: null;
}

export interface PatchMessage {
  type: "patch";
  payload: {
    reason?: string | null;
    sinkId?: string | null;
    sourceId?: string | null;
    playerId?: string | null;
    windowId?: string | null;
    changes: Array<
      | {
          path: "audio";
          value: AudioState;
        }
      | {
          path: "media";
          value: MediaState;
        }
      | {
          path: "windowState";
          value: WindowSnapshot;
        }
    >;
  };
  error: null;
}

export interface ErrorMessage {
  type: "error";
  payload: null;
  error: {
    code: string;
    message: string;
  };
}

export type BackendMessage = FullStateMessage | PatchMessage | ErrorMessage;
