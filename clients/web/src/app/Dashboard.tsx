import { useEffect, useMemo, useState } from "react";

import { checkHttpEndpoint, type CheckState } from "../api/connectivity";
import { activateWindow, controlMedia, seekMedia, setDeviceMuted, setSinkVolume } from "../api/controls";
import type { AudioDeviceState, MediaPlayerState } from "../api/models";
import { initialEndpointSettings, persistEndpointSettings, type EndpointSettings } from "../api/settings";
import { audioDevicesWithSelectedFirst } from "../api/state";
import { useStateStream, type StreamStatus } from "../api/useStateStream";
import { AudioSection } from "../components/AudioSection";
import { MediaSection } from "../components/MediaSection";
import { SettingsDialog } from "../components/SettingsDialog";
import { WindowTaskbar } from "../components/WindowTaskbar";
import type { ThemeMode } from "../theme/theme";

interface DashboardProps {
  themeMode: ThemeMode;
  onThemeModeChange: (mode: ThemeMode) => void;
}

interface EndpointStatus {
  state: CheckState;
  detail: string;
}

const checkingHttpStatus: EndpointStatus = {
  state: "checking",
  detail: "Checking OpenAPI endpoint..."
};

function streamStatusLabel(status: StreamStatus): string {
  switch (status) {
    case "connected":
      return "Connected";
    case "connecting":
      return "Connecting";
    case "not_ready":
      return "Not ready";
    case "disconnected":
      return "Disconnected";
    case "error":
      return "Error";
  }
}

export function Dashboard({ themeMode, onThemeModeChange }: DashboardProps) {
  const [settings, setSettings] = useState<EndpointSettings>(() => initialEndpointSettings());
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [reconnectToken, setReconnectToken] = useState(0);
  const [httpStatus, setHttpStatus] = useState<EndpointStatus>(checkingHttpStatus);
  const [pendingActions, setPendingActions] = useState<Record<string, boolean>>({});
  const [rowErrors, setRowErrors] = useState<Record<string, string>>({});
  const [volumeDrafts, setVolumeDrafts] = useState<Record<string, number>>({});
  const stream = useStateStream(settings.wsUrl, reconnectToken);

  useEffect(() => {
    let cancelled = false;
    setHttpStatus(checkingHttpStatus);
    checkHttpEndpoint(settings.httpBaseUrl).then((result) => {
      if (!cancelled) {
        setHttpStatus(result);
      }
    });

    return () => {
      cancelled = true;
    };
  }, [settings.httpBaseUrl, reconnectToken]);

  useEffect(() => {
    setVolumeDrafts({});
  }, [stream.state.audio]);

  useEffect(() => {
    setRowErrors((current) => ({ ...current, media: "" }));
  }, [stream.state.media?.player?.playerId]);

  const sinks = useMemo(
    () => audioDevicesWithSelectedFirst(stream.state.audio?.sinks ?? [], stream.state.audio?.selectedSinkId ?? null),
    [stream.state.audio]
  );
  const sources = useMemo(
    () => audioDevicesWithSelectedFirst(stream.state.audio?.sources ?? [], stream.state.audio?.selectedSourceId ?? null),
    [stream.state.audio]
  );
  const showConnectionSummary = stream.status !== "connected" || httpStatus.state === "unreachable";
  const currentMediaPlayer = stream.state.media?.player ?? null;

  const runDeviceAction = async (deviceId: string, actionKey: string, action: () => Promise<void>) => {
    setPendingActions((current) => ({ ...current, [actionKey]: true }));
    setRowErrors((current) => ({ ...current, [deviceId]: "" }));
    try {
      await action();
    } catch (error) {
      setRowErrors((current) => ({
        ...current,
        [deviceId]: error instanceof Error ? error.message : "Request failed."
      }));
    } finally {
      setPendingActions((current) => ({ ...current, [actionKey]: false }));
    }
  };

  const runWindowAction = async (windowId: string, actionKey: string, action: () => Promise<void>) => {
    setPendingActions((current) => ({ ...current, [actionKey]: true }));
    setRowErrors((current) => ({ ...current, [windowId]: "" }));
    try {
      await action();
    } catch (error) {
      setRowErrors((current) => ({
        ...current,
        [windowId]: error instanceof Error ? error.message : "Request failed."
      }));
    } finally {
      setPendingActions((current) => ({ ...current, [actionKey]: false }));
    }
  };

  const runMediaAction = async (
    player: MediaPlayerState | null,
    actionName: "play" | "pause" | "play-pause" | "next" | "previous" | "seek",
    action: () => Promise<void>
  ) => {
    const actionKey = `media:${actionName}`;
    const errorKey = "media";
    setPendingActions((current) => ({ ...current, [actionKey]: true }));
    setRowErrors((current) => ({ ...current, [errorKey]: "" }));
    try {
      await action();
    } catch (error) {
      const prefix = player?.identity || player?.desktopEntry || player?.playerId || "Current player";
      setRowErrors((current) => ({
        ...current,
        [errorKey]: `${prefix}: ${error instanceof Error ? error.message : "Request failed."}`
      }));
    } finally {
      setPendingActions((current) => ({ ...current, [actionKey]: false }));
    }
  };

  const commitSinkVolume = (sinkId: string, value: number) => {
    void runDeviceAction(sinkId, `volume:${sinkId}`, () => setSinkVolume(settings.httpBaseUrl, sinkId, value));
  };

  const toggleSinkMuted = (device: AudioDeviceState) => {
    void runDeviceAction(device.id, `mute:${device.id}`, () =>
      setDeviceMuted(settings.httpBaseUrl, "sinks", device.id, !device.muted)
    );
  };

  const toggleSourceMuted = (device: AudioDeviceState) => {
    void runDeviceAction(device.id, `mute:${device.id}`, () =>
      setDeviceMuted(settings.httpBaseUrl, "sources", device.id, !device.muted)
    );
  };

  const activateTaskbarWindow = (windowId: string) => {
    void runWindowAction(windowId, `window-active:${windowId}`, () => activateWindow(settings.httpBaseUrl, windowId));
  };

  const performMediaAction = (actionName: "play" | "pause" | "play-pause" | "next" | "previous") => {
    void runMediaAction(currentMediaPlayer, actionName, () => controlMedia(settings.httpBaseUrl, actionName));
  };

  const togglePlayback = () => {
    if (currentMediaPlayer === null) {
      return;
    }

    if (!currentMediaPlayer.canControl) {
      return;
    }

    performMediaAction("play-pause");
  };

  const seekCurrentMedia = (positionMs: number) => {
    void runMediaAction(currentMediaPlayer, "seek", () => seekMedia(settings.httpBaseUrl, positionMs));
  };

  const saveSettings = (nextSettings: EndpointSettings) => {
    const normalized = persistEndpointSettings(nextSettings);
    setSettings(normalized);
    setSettingsOpen(false);
    setReconnectToken((current) => current + 1);
  };

  return (
    <main className="app-shell">
      <header className="top-bar">
        <div>
          <p>Plasma Remote Toolbar</p>
          <h1>Desktop controls</h1>
        </div>
        <div className="top-actions">
          <span className={`connection-pill status-${stream.status}`}>
            {streamStatusLabel(stream.status)}
          </span>
          <button type="button" className="settings-button" onClick={() => setSettingsOpen(true)}>
            Settings
          </button>
        </div>
      </header>

      {showConnectionSummary ? (
        <section className={`connection-summary summary-${stream.status}`} aria-live="polite">
          <div>
            <strong>{streamStatusLabel(stream.status)}</strong>
            <p>{stream.detail}</p>
          </div>
          {httpStatus.state === "unreachable" ? (
            <div>
              <strong>HTTP unreachable</strong>
              <p>{httpStatus.detail}</p>
            </div>
          ) : null}
        </section>
      ) : null}

      <WindowTaskbar
        snapshot={stream.state.windowState}
        httpBaseUrl={settings.httpBaseUrl}
        pendingActions={pendingActions}
        errors={rowErrors}
        onWindowActivate={activateTaskbarWindow}
      />

      <MediaSection
        player={currentMediaPlayer}
        httpBaseUrl={settings.httpBaseUrl}
        pendingActions={pendingActions}
        error={rowErrors.media ?? ""}
        onPrevious={() => performMediaAction("previous")}
        onTogglePlayPause={togglePlayback}
        onNext={() => performMediaAction("next")}
        onSeek={seekCurrentMedia}
      />

      <div className="audio-grid">
        <AudioSection
          title="Sinks"
          devices={sinks}
          selectedId={stream.state.audio?.selectedSinkId ?? null}
          volumeDrafts={volumeDrafts}
          pendingActions={pendingActions}
          errors={rowErrors}
          onVolumeDraftChange={(deviceId, value) => setVolumeDrafts((current) => ({ ...current, [deviceId]: value }))}
          onVolumeCommit={commitSinkVolume}
          onMuteToggle={toggleSinkMuted}
        />
        <AudioSection
          title="Sources"
          devices={sources}
          selectedId={stream.state.audio?.selectedSourceId ?? null}
          volumeDrafts={volumeDrafts}
          pendingActions={pendingActions}
          errors={rowErrors}
          volumeReadOnly
          onVolumeDraftChange={(deviceId, value) => setVolumeDrafts((current) => ({ ...current, [deviceId]: value }))}
          onMuteToggle={toggleSourceMuted}
        />
      </div>

      <SettingsDialog
        open={settingsOpen}
        settings={settings}
        httpStatus={httpStatus}
        streamStatus={{
          label: streamStatusLabel(stream.status),
          detail: stream.detail
        }}
        themeMode={themeMode}
        onThemeModeChange={onThemeModeChange}
        onClose={() => setSettingsOpen(false)}
        onSave={saveSettings}
      />
    </main>
  );
}
