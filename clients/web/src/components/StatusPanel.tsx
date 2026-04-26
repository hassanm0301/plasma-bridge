import type { CheckState } from "../api/connectivity";

interface EndpointStatus {
  state: CheckState;
  detail: string;
}

interface StatusPanelProps {
  http: EndpointStatus;
  websocket: EndpointStatus;
}

const stateLabel: Record<CheckState, string> = {
  idle: "Idle",
  checking: "Checking",
  reachable: "Reachable",
  unreachable: "Unreachable"
};

function StatusRow({ label, status }: { label: string; status: EndpointStatus }) {
  return (
    <div className="status-row">
      <div>
        <span className={`status-dot status-${status.state}`} />
        <span className="status-label">{label}</span>
      </div>
      <div>
        <strong>{stateLabel[status.state]}</strong>
        {status.detail ? <p>{status.detail}</p> : null}
      </div>
    </div>
  );
}

export function StatusPanel({ http, websocket }: StatusPanelProps) {
  return (
    <section className="status-panel" aria-live="polite">
      <StatusRow label="HTTP" status={http} />
      <StatusRow label="WebSocket" status={websocket} />
    </section>
  );
}
