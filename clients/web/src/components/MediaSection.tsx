import { useEffect, useState } from "react";

import type { MediaPlayerState } from "../api/models";
import { clampMediaPosition, formatDurationMs, mediaPrimaryActionLabel, mediaToggleAction } from "../api/state";

interface MediaSectionProps {
  player: MediaPlayerState | null;
  httpBaseUrl: string;
  pendingActions: Record<string, boolean>;
  error: string;
  onPrevious: () => void;
  onTogglePlayPause: () => void;
  onNext: () => void;
  onSeek: (positionMs: number) => void;
}

const progressTickMs = 250;

function resolveAssetUrl(httpBaseUrl: string, url: string | null): string | null {
  if (url === null || url.trim() === "") {
    return null;
  }

  return new URL(url, `${httpBaseUrl}/`).toString();
}

function fallbackLabel(player: MediaPlayerState): string {
  return (player.identity || player.desktopEntry || player.title || "Media").slice(0, 1).toUpperCase();
}

function subtitle(player: MediaPlayerState): string {
  if (player.artists.length > 0) {
    return player.artists.join(", ");
  }

  if (player.album) {
    return player.album;
  }

  return player.identity || player.desktopEntry || "Unknown source";
}

function playbackStatusLabel(player: MediaPlayerState): string {
  switch (player.playbackStatus) {
    case "playing":
      return "Playing";
    case "paused":
      return "Paused";
    case "stopped":
      return "Stopped";
    default:
      return "Unknown";
  }
}

function TransportIcon({ kind }: { kind: "previous" | "play" | "pause" | "next" }) {
  switch (kind) {
    case "previous":
      return (
        <svg aria-hidden="true" viewBox="0 0 24 24">
          <path d="M6 5h2v14H6zM18 6v12l-8-6z" fill="currentColor" />
        </svg>
      );
    case "play":
      return (
        <svg aria-hidden="true" viewBox="0 0 24 24">
          <path d="M8 6v12l10-6z" fill="currentColor" />
        </svg>
      );
    case "pause":
      return (
        <svg aria-hidden="true" viewBox="0 0 24 24">
          <path d="M7 6h4v12H7zM13 6h4v12h-4z" fill="currentColor" />
        </svg>
      );
    case "next":
      return (
        <svg aria-hidden="true" viewBox="0 0 24 24">
          <path d="M16 5h2v14h-2zM6 6v12l8-6z" fill="currentColor" />
        </svg>
      );
    default:
      return null;
  }
}

export function MediaSection({
  player,
  httpBaseUrl,
  pendingActions,
  error,
  onPrevious,
  onTogglePlayPause,
  onNext,
  onSeek
}: MediaSectionProps) {
  const toggleAction = player ? mediaToggleAction(player) : null;
  const artworkUrl = player ? resolveAssetUrl(httpBaseUrl, player.artworkUrl) : null;
  const appIconUrl = player ? resolveAssetUrl(httpBaseUrl, player.appIconUrl) : null;
  const [seekDraftMs, setSeekDraftMs] = useState<number | null>(null);
  const [displayPositionMs, setDisplayPositionMs] = useState<number | null>(player?.positionMs ?? null);
  const isDragging = seekDraftMs !== null;

  useEffect(() => {
    setSeekDraftMs(null);
    setDisplayPositionMs(player?.positionMs ?? null);
  }, [player?.playerId, player?.title, player?.trackLengthMs]);

  useEffect(() => {
    if (!isDragging) {
      setDisplayPositionMs(player?.positionMs ?? null);
    }
  }, [player?.playerId, player?.positionMs, player?.playbackStatus, player?.trackLengthMs, isDragging]);

  useEffect(() => {
    if (player === null || player.positionMs === null || isDragging || player.playbackStatus !== "playing") {
      return;
    }

    const interval = window.setInterval(() => {
      setDisplayPositionMs((current) => {
        if (current === null) {
          return null;
        }

        return clampMediaPosition(current + progressTickMs, player.trackLengthMs);
      });
    }, progressTickMs);

    return () => {
      window.clearInterval(interval);
    };
  }, [player?.playerId, player?.positionMs, player?.playbackStatus, player?.trackLengthMs, isDragging]);

  const sliderPositionMs = seekDraftMs ?? displayPositionMs ?? player?.positionMs ?? 0;
  const hasTimeline = player !== null && player.trackLengthMs !== null && player.positionMs !== null;
  const canSeek = player !== null && player.canSeek && hasTimeline;

  const commitSeek = (positionMs: number) => {
    if (player === null) {
      return;
    }

    const clampedValue = clampMediaPosition(positionMs, player.trackLengthMs);
    setSeekDraftMs(null);
    setDisplayPositionMs(clampedValue);

    if (!canSeek || pendingActions["media:seek"]) {
      return;
    }

    onSeek(clampedValue);
  };

  return (
    <section className="dashboard-section">
      <div className="section-heading">
        <h2>Current Media</h2>
        <span>{player === null ? 0 : 1}</span>
      </div>

      {player === null ? (
        <div className="empty-state">No media session reported yet.</div>
      ) : (
        <article className={`media-card media-status-${player.playbackStatus}`}>
          <div className="media-art-frame" aria-hidden="true">
            {artworkUrl ? (
              <img className="media-art" src={artworkUrl} alt="" loading="lazy" />
            ) : appIconUrl ? (
              <img className="media-app-icon" src={appIconUrl} alt="" loading="lazy" />
            ) : (
              <span className="media-art-fallback">{fallbackLabel(player)}</span>
            )}
          </div>

          <div className="media-copy">
            <div className="media-main">
              <div>
                <h3>{player.title || "Untitled track"}</h3>
                <p className="media-subtitle">{subtitle(player)}</p>
              </div>
              <span className="media-status-pill">{playbackStatusLabel(player)}</span>
            </div>

            <div className="media-meta-row">
              <span className="media-source">
                {appIconUrl ? <img className="media-source-icon" src={appIconUrl} alt="" loading="lazy" /> : null}
                <span>{player.identity || player.desktopEntry || player.playerId}</span>
              </span>
              <span className="media-meta">{player.album || "Unknown album"}</span>
            </div>

            <div className="media-progress">
              <span className="media-progress-time">{formatDurationMs(hasTimeline ? sliderPositionMs : null)}</span>
              <input
                aria-label="Seek current track"
                className="media-progress-slider"
                type="range"
                min="0"
                max={player.trackLengthMs ?? 0}
                step="1000"
                value={hasTimeline ? sliderPositionMs : 0}
                disabled={!canSeek || pendingActions["media:seek"]}
                onChange={(event) => {
                  const nextValue = clampMediaPosition(Number(event.target.value), player.trackLengthMs);
                  setSeekDraftMs(nextValue);
                  setDisplayPositionMs(nextValue);
                }}
                onPointerUp={(event) => commitSeek(Number(event.currentTarget.value))}
                onBlur={(event) => {
                  if (seekDraftMs !== null) {
                    commitSeek(Number(event.currentTarget.value));
                  }
                }}
                onKeyUp={(event) => {
                  if (
                    event.key === "ArrowLeft" ||
                    event.key === "ArrowRight" ||
                    event.key === "ArrowUp" ||
                    event.key === "ArrowDown" ||
                    event.key === "Home" ||
                    event.key === "End" ||
                    event.key === "PageUp" ||
                    event.key === "PageDown"
                  ) {
                    commitSeek(Number(event.currentTarget.value));
                  }
                }}
              />
              <span className="media-progress-time">{formatDurationMs(player.trackLengthMs)}</span>
            </div>

            <div className="media-controls">
              <button
                type="button"
                className="media-transport-button"
                aria-label="Previous track"
                title="Previous track"
                disabled={!player.canControl || !player.canGoPrevious || pendingActions["media:previous"]}
                onClick={onPrevious}
              >
                <TransportIcon kind="previous" />
              </button>
              <button
                type="button"
                className="media-transport-button media-transport-primary"
                aria-label={mediaPrimaryActionLabel(player)}
                title={mediaPrimaryActionLabel(player)}
                disabled={toggleAction === null || pendingActions["media:play-pause"]}
                onClick={onTogglePlayPause}
              >
                <TransportIcon kind={player.playbackStatus === "playing" ? "pause" : "play"} />
              </button>
              <button
                type="button"
                className="media-transport-button"
                aria-label="Next track"
                title="Next track"
                disabled={!player.canControl || !player.canGoNext || pendingActions["media:next"]}
                onClick={onNext}
              >
                <TransportIcon kind="next" />
              </button>
            </div>

            {error ? <p className="row-error">{error}</p> : null}
          </div>
        </article>
      )}
    </section>
  );
}
