import type { AudioDeviceState } from "../api/models";

interface AudioSectionProps {
  title: string;
  devices: AudioDeviceState[];
  selectedId: string | null;
  volumeDrafts: Record<string, number>;
  pendingActions: Record<string, boolean>;
  errors: Record<string, string>;
  volumeReadOnly?: boolean;
  onVolumeDraftChange: (deviceId: string, value: number) => void;
  onVolumeCommit?: (deviceId: string, value: number) => void;
  onMuteToggle: (device: AudioDeviceState) => void;
}

function percentValue(value: number): number {
  return Math.round(Math.max(0, Math.min(1, value)) * 100);
}

export function AudioSection({
  title,
  devices,
  selectedId,
  volumeDrafts,
  pendingActions,
  errors,
  volumeReadOnly = false,
  onVolumeDraftChange,
  onVolumeCommit,
  onMuteToggle
}: AudioSectionProps) {
  return (
    <section className="dashboard-section">
      <div className="section-heading">
        <h2>{title}</h2>
        <span>{devices.length}</span>
      </div>

      {devices.length === 0 ? (
        <div className="empty-state">No devices reported yet.</div>
      ) : (
        <div className="device-list">
          {devices.map((device) => {
            const volume = volumeDrafts[device.id] ?? device.volume;
            const volumePercent = percentValue(volume);
            const isSelected = device.id === selectedId || device.isDefault;
            const volumePending = pendingActions[`volume:${device.id}`] ?? false;
            const mutePending = pendingActions[`mute:${device.id}`] ?? false;

            return (
              <article className={`device-row ${isSelected ? "selected" : ""}`} key={device.id}>
                <div className="device-main">
                  <div>
                    <h3>{device.label || device.id}</h3>
                    <p>{isSelected ? "Default" : device.available ? "Available" : "Unavailable"}</p>
                  </div>
                  <span>{volumePercent}%</span>
                </div>

                <div className="device-controls">
                  <input
                    aria-label={`${device.label} volume`}
                    type="range"
                    min="0"
                    max="100"
                    value={volumePercent}
                    disabled={volumeReadOnly || volumePending}
                    onChange={(event) => onVolumeDraftChange(device.id, Number(event.target.value) / 100)}
                    onPointerUp={(event) => {
                      if (!volumeReadOnly) {
                        onVolumeCommit?.(device.id, Number(event.currentTarget.value) / 100);
                      }
                    }}
                    onBlur={(event) => {
                      if (!volumeReadOnly) {
                        onVolumeCommit?.(device.id, Number(event.currentTarget.value) / 100);
                      }
                    }}
                  />
                  <button
                    type="button"
                    className="mute-button"
                    disabled={mutePending}
                    aria-pressed={device.muted}
                    onClick={() => onMuteToggle(device)}
                  >
                    {device.muted ? "Unmute" : "Mute"}
                  </button>
                </div>

                {volumeReadOnly ? <p className="row-note">Volume is read-only for sources.</p> : null}
                {errors[device.id] ? <p className="row-error">{errors[device.id]}</p> : null}
              </article>
            );
          })}
        </div>
      )}
    </section>
  );
}
