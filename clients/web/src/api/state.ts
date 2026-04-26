import type {
  AudioDeviceState,
  BackendMessage,
  BackendState,
  WindowSnapshot,
  WindowState
} from "./models";

export const emptyBackendState: BackendState = {
  audio: null,
  windowState: null
};

export function applyBackendMessage(state: BackendState, message: BackendMessage): BackendState {
  if (message.type === "fullState") {
    return {
      audio: message.payload.audio ?? state.audio,
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
