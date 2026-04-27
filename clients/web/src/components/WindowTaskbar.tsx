import type { WindowSnapshot } from "../api/models";
import { displayWindowTitle, windowsForTaskbar } from "../api/state";

interface WindowTaskbarProps {
  snapshot: WindowSnapshot | null;
  pendingActions: Record<string, boolean>;
  errors: Record<string, string>;
  onWindowActivate: (windowId: string) => void;
}

export function WindowTaskbar({ snapshot, pendingActions, errors, onWindowActivate }: WindowTaskbarProps) {
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
            const isPending = pendingActions[`window-active:${window.id}`] ?? false;
            const appLabel = window.appId ?? window.resourceName ?? "Application";
            const content = (
              <>
                <span className="taskbar-title">{displayWindowTitle(window)}</span>
                <span className="taskbar-meta">{isPending ? "Focusing..." : appLabel}</span>
                {errors[window.id] ? <span className="taskbar-error">{errors[window.id]}</span> : null}
              </>
            );

            return (
              <div className="taskbar-list-item" key={window.id} role="listitem">
                {isActive ? (
                  <div className="taskbar-item active" aria-current="true">
                    {content}
                  </div>
                ) : (
                  <button
                    type="button"
                    className="taskbar-item"
                    disabled={isPending}
                    onClick={() => onWindowActivate(window.id)}
                  >
                    {content}
                  </button>
                )}
              </div>
            );
          })}
        </div>
      )}
    </section>
  );
}
