import { useEffect, useState } from "react";

import type { CheckState } from "../api/connectivity";
import type { EndpointSettings } from "../api/settings";
import { EndpointField } from "./EndpointField";
import { ThemeToggle } from "./ThemeToggle";
import type { ThemeMode } from "../theme/theme";

interface SettingsDialogProps {
  open: boolean;
  settings: EndpointSettings;
  httpStatus: { state: CheckState; detail: string };
  streamStatus: { label: string; detail: string };
  themeMode: ThemeMode;
  onThemeModeChange: (mode: ThemeMode) => void;
  onClose: () => void;
  onSave: (settings: EndpointSettings) => void;
}

export function SettingsDialog({
  open,
  settings,
  httpStatus,
  streamStatus,
  themeMode,
  onThemeModeChange,
  onClose,
  onSave
}: SettingsDialogProps) {
  const [draftSettings, setDraftSettings] = useState(settings);

  useEffect(() => {
    setDraftSettings(settings);
  }, [settings, open]);

  if (!open) {
    return null;
  }

  return (
    <div className="modal-backdrop" role="presentation" onMouseDown={onClose}>
      <section
        className="settings-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="settings-title"
        onMouseDown={(event) => event.stopPropagation()}
      >
        <header className="settings-header">
          <h2 id="settings-title">Settings</h2>
          <button type="button" className="quiet-button" onClick={onClose}>
            Close
          </button>
        </header>

        <form
          className="settings-form"
          onSubmit={(event) => {
            event.preventDefault();
            onSave(draftSettings);
          }}
        >
          <EndpointField
            id="settings-http-endpoint"
            label="HTTP endpoint"
            value={draftSettings.httpBaseUrl}
            placeholder="http://127.0.0.1:8080"
            onChange={(httpBaseUrl) => setDraftSettings((current) => ({ ...current, httpBaseUrl }))}
          />
          <EndpointField
            id="settings-ws-endpoint"
            label="WebSocket endpoint"
            value={draftSettings.wsUrl}
            placeholder="ws://127.0.0.1:8081/ws"
            onChange={(wsUrl) => setDraftSettings((current) => ({ ...current, wsUrl }))}
          />

          <div className="settings-section">
            <span className="settings-label">Theme</span>
            <ThemeToggle mode={themeMode} onChange={onThemeModeChange} />
          </div>

          <div className="settings-status">
            <div>
              <span>HTTP</span>
              <strong>{httpStatus.state}</strong>
              <p>{httpStatus.detail || "No HTTP check has run yet."}</p>
            </div>
            <div>
              <span>WebSocket</span>
              <strong>{streamStatus.label}</strong>
              <p>{streamStatus.detail}</p>
            </div>
          </div>

          <div className="settings-actions">
            <button type="button" className="quiet-button" onClick={onClose}>
              Cancel
            </button>
            <button type="submit" className="primary-action">
              Save/Reconnect
            </button>
          </div>
        </form>
      </section>
    </div>
  );
}
