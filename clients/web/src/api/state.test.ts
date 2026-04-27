import { describe, expect, it } from "vitest";

import type { AudioDeviceState, BackendState, WindowSnapshot, WindowState } from "./models";
import {
  applyBackendMessage,
  audioDevicesWithSelectedFirst,
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

describe("backend state helpers", () => {
  it("stores full state messages", () => {
    const nextState = applyBackendMessage(emptyBackendState, {
      type: "fullState",
      payload: {
        audio: { sinks: [], selectedSinkId: null, sources: [], selectedSourceId: null },
        windowState: { activeWindowId: null, activeWindow: null, windows: [] }
      },
      error: null
    });

    expect(nextState.audio).not.toBeNull();
    expect(nextState.windowState).not.toBeNull();
  });

  it("applies top-level patch changes", () => {
    const state: BackendState = {
      audio: { sinks: [], selectedSinkId: null, sources: [], selectedSourceId: null },
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
});
