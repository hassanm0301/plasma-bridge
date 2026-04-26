import { useState } from "react";

import {
  checkHttpEndpoint,
  checkWebSocketEndpoint,
  type CheckState
} from "../api/connectivity";
import { DEFAULT_HTTP_BASE_URL, DEFAULT_WS_URL } from "../api/endpoints";
import { EndpointField } from "../components/EndpointField";
import { StatusPanel } from "../components/StatusPanel";
import { ThemeToggle } from "../components/ThemeToggle";
import type { ThemeMode } from "../theme/theme";

interface EndpointStatus {
  state: CheckState;
  detail: string;
}

interface ConnectionScreenProps {
  themeMode: ThemeMode;
  onThemeModeChange: (mode: ThemeMode) => void;
}

const idleStatus: EndpointStatus = {
  state: "idle",
  detail: ""
};

const checkingStatus: EndpointStatus = {
  state: "checking",
  detail: "Contacting endpoint..."
};

export function ConnectionScreen({ themeMode, onThemeModeChange }: ConnectionScreenProps) {
  const [httpEndpoint, setHttpEndpoint] = useState(DEFAULT_HTTP_BASE_URL);
  const [wsEndpoint, setWsEndpoint] = useState(DEFAULT_WS_URL);
  const [httpStatus, setHttpStatus] = useState<EndpointStatus>(idleStatus);
  const [wsStatus, setWsStatus] = useState<EndpointStatus>(idleStatus);
  const [isChecking, setIsChecking] = useState(false);

  const connect = async () => {
    setIsChecking(true);
    setHttpStatus(checkingStatus);
    setWsStatus(checkingStatus);

    const [httpResult, wsResult] = await Promise.all([
      checkHttpEndpoint(httpEndpoint),
      checkWebSocketEndpoint(wsEndpoint)
    ]);

    setHttpStatus(httpResult);
    setWsStatus(wsResult);
    setIsChecking(false);
  };

  return (
    <main className="app-shell">
      <header className="top-bar">
        <div>
          <p>Plasma Remote Toolbar</p>
          <h1>Connect to plasma_bridge</h1>
        </div>
        <ThemeToggle mode={themeMode} onChange={onThemeModeChange} />
      </header>

      <section className="connection-panel">
        <div className="endpoint-grid">
          <EndpointField
            id="http-endpoint"
            label="HTTP endpoint"
            value={httpEndpoint}
            placeholder={DEFAULT_HTTP_BASE_URL}
            onChange={setHttpEndpoint}
          />
          <EndpointField
            id="ws-endpoint"
            label="WebSocket endpoint"
            value={wsEndpoint}
            placeholder={DEFAULT_WS_URL}
            onChange={setWsEndpoint}
          />
        </div>

        <div className="actions">
          <button className="primary-action" type="button" disabled={isChecking} onClick={connect}>
            {isChecking ? "Connecting" : "Connect"}
          </button>
        </div>
      </section>

      <StatusPanel http={httpStatus} websocket={wsStatus} />
    </main>
  );
}
