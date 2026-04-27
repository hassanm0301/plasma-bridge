import type { WindowSnapshot } from "../api/models";
import { displayWindowTitle, windowsForTaskbar } from "../api/state";

interface WindowTaskbarProps {
  snapshot: WindowSnapshot | null;
  httpBaseUrl: string;
  pendingActions: Record<string, boolean>;
  errors: Record<string, string>;
  onWindowActivate: (windowId: string) => void;
}

function resolveIconUrl(httpBaseUrl: string, iconUrl: string | null): string | null {
  if (iconUrl === null || iconUrl.trim() === "") {
    return null;
  }

  return new URL(iconUrl, `${httpBaseUrl}/`).toString();
}

export function WindowTaskbar({ snapshot, httpBaseUrl, pendingActions, errors, onWindowActivate }: WindowTaskbarProps) {
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
            const resolvedIconUrl = resolveIconUrl(httpBaseUrl, window.iconUrl);
            const content = (
              <>
                <span className="taskbar-icon-frame" aria-hidden="true">
                  {resolvedIconUrl === null ? (
                    <span className="taskbar-icon-fallback">{appLabel.slice(0, 1).toUpperCase()}</span>
                  ) : (
                    <img className="taskbar-icon" src={resolvedIconUrl} alt="" loading="lazy" />
                  )}
                </span>
                <span className="taskbar-copy">
                  <span className="taskbar-title">{displayWindowTitle(window)}</span>
                  <span className="taskbar-meta">{isPending ? "Focusing..." : appLabel}</span>
                  {errors[window.id] ? <span className="taskbar-error">{errors[window.id]}</span> : null}
                </span>
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
