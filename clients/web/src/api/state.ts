import type {
  AudioDeviceState,
  BackendMessage,
  BackendState,
  MediaPlayerState,
  WindowSnapshot,
  WindowState
} from "./models";

export const emptyBackendState: BackendState = {
  audio: null,
  media: null,
  windowState: null
};

export function applyBackendMessage(state: BackendState, message: BackendMessage): BackendState {
  if (message.type === "fullState") {
    return {
      audio: message.payload.audio ?? state.audio,
      media: message.payload.media ?? state.media,
      windowState: message.payload.windowState ?? state.windowState
    };
  }

  if (message.type !== "patch") {
    return state;
  }

  return message.payload.changes.reduce<BackendState>((nextState, change) => {
    if (change.path === "audio") {
      return { ...nextState, audio: change.value };
    }
    if (change.path === "media") {
      return { ...nextState, media: change.value };
    }
    return { ...nextState, windowState: change.value };
  }, state);
}

export function windowsForTaskbar(snapshot: WindowSnapshot | null): WindowState[] {
  if (snapshot === null) {
    return [];
  }

  const byId = new Map<string, WindowState>();
  for (const window of snapshot.windows) {
    if (!window.skipTaskbar) {
      byId.set(window.id, window);
    }
  }

  if (snapshot.activeWindow !== null && !snapshot.activeWindow.skipTaskbar) {
    byId.set(snapshot.activeWindow.id, snapshot.activeWindow);
  }

  return [...byId.values()].sort((left, right) => {
    const leftActive = left.id === snapshot.activeWindowId || left.isActive;
    const rightActive = right.id === snapshot.activeWindowId || right.isActive;
    if (leftActive !== rightActive) {
      return leftActive ? -1 : 1;
    }
    return displayWindowTitle(left).localeCompare(displayWindowTitle(right));
  });
}

export function audioDevicesWithSelectedFirst(
  devices: AudioDeviceState[],
  selectedId: string | null
): AudioDeviceState[] {
  return [...devices].sort((left, right) => {
    const leftSelected = left.id === selectedId || left.isDefault;
    const rightSelected = right.id === selectedId || right.isDefault;
    if (leftSelected !== rightSelected) {
      return leftSelected ? -1 : 1;
    }
    return left.label.localeCompare(right.label);
  });
}

export function displayWindowTitle(window: WindowState): string {
  return window.title || window.appId || window.resourceName || "Untitled window";
}

export function mediaToggleAction(player: MediaPlayerState): "play-pause" | null {
  return player.canControl ? "play-pause" : null;
}

export function mediaPrimaryActionLabel(player: MediaPlayerState): string {
  return player.playbackStatus === "playing" ? "Pause" : "Play";
}

export function formatDurationMs(value: number | null): string {
  if (value === null || !Number.isFinite(value) || value < 0) {
    return "--:--";
  }

  const totalSeconds = Math.floor(value / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;

  if (hours > 0) {
    return `${hours}:${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
  }

  return `${minutes}:${String(seconds).padStart(2, "0")}`;
}

export function clampMediaPosition(positionMs: number, trackLengthMs: number | null): number {
  const minimum = Math.max(0, Math.floor(positionMs));
  if (trackLengthMs === null || !Number.isFinite(trackLengthMs) || trackLengthMs < 0) {
    return minimum;
  }

  return Math.min(minimum, Math.floor(trackLengthMs));
}
