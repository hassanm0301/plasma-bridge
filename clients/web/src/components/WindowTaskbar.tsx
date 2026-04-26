import type { WindowSnapshot } from "../api/models";
import { displayWindowTitle, windowsForTaskbar } from "../api/state";

interface WindowTaskbarProps {
  snapshot: WindowSnapshot | null;
}

export function WindowTaskbar({ snapshot }: WindowTaskbarProps) {
  const windows = windowsForTaskbar(snapshot);

  return (
    <section className="dashboard-section">
      <div className="section-heading">
        <h2>Windows</h2>
        <span>{windows.length}</span>
      </div>

      {windows.length === 0 ? (
        <div className="empty-state">No windows reported yet.</div>
      ) : (
        <div className="taskbar-strip" role="list">
          {windows.map((window) => {
            const isActive = window.id === snapshot?.activeWindowId || window.isActive;
            return (
              <div className={`taskbar-item ${isActive ? "active" : ""}`} key={window.id} role="listitem">
                <span className="taskbar-title">{displayWindowTitle(window)}</span>
                <span className="taskbar-meta">{window.appId ?? window.resourceName ?? "Application"}</span>
              </div>
            );
          })}
        </div>
      )}
    </section>
  );
}
