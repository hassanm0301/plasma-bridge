import 'dart:async';

import 'package:flutter/material.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/utils/media_helpers.dart';
import '../../../core/utils/url_utils.dart';
import '../../../core/widgets/desktop_panel.dart';
import '../../../core/widgets/remote_image.dart';

class MediaSection extends StatefulWidget {
  const MediaSection({
    super.key,
    required this.player,
    required this.httpBaseUrl,
    required this.pendingActions,
    required this.error,
    required this.onPrevious,
    required this.onTogglePlayPause,
    required this.onNext,
    required this.onSeek,
  });

  final MediaPlayerState? player;
  final String httpBaseUrl;
  final Map<String, bool> pendingActions;
  final String error;
  final Future<void> Function() onPrevious;
  final Future<void> Function() onTogglePlayPause;
  final Future<void> Function() onNext;
  final Future<void> Function(int positionMs) onSeek;

  @override
  State<MediaSection> createState() => _MediaSectionState();
}

class _MediaSectionState extends State<MediaSection> {
  static const progressTick = Duration(milliseconds: 250);

  Timer? _timer;
  int? _seekDraftMs;
  int? _displayPositionMs;

  @override
  void initState() {
    super.initState();
    _displayPositionMs = widget.player?.positionMs;
    _syncTimer();
  }

  @override
  void didUpdateWidget(covariant MediaSection oldWidget) {
    super.didUpdateWidget(oldWidget);
    final oldPlayer = oldWidget.player;
    final newPlayer = widget.player;
    final resetNeeded =
        oldPlayer?.playerId != newPlayer?.playerId ||
        oldPlayer?.title != newPlayer?.title ||
        oldPlayer?.trackLengthMs != newPlayer?.trackLengthMs;
    if (resetNeeded) {
      _seekDraftMs = null;
      _displayPositionMs = newPlayer?.positionMs;
    } else if (_seekDraftMs == null) {
      _displayPositionMs = newPlayer?.positionMs;
    }
    _syncTimer();
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  void _syncTimer() {
    _timer?.cancel();
    final player = widget.player;
    if (player == null ||
        player.positionMs == null ||
        _seekDraftMs != null ||
        player.playbackStatus != MediaPlaybackStatus.playing) {
      return;
    }

    _timer = Timer.periodic(progressTick, (_) {
      setState(() {
        final current = _displayPositionMs;
        if (current != null) {
          _displayPositionMs = clampMediaPosition(
            current + progressTick.inMilliseconds,
            player.trackLengthMs,
          );
        }
      });
    });
  }

  Future<void> _commitSeek(double value) async {
    final player = widget.player;
    if (player == null) {
      return;
    }

    final clamped = clampMediaPosition(value.round(), player.trackLengthMs);
    setState(() {
      _seekDraftMs = null;
      _displayPositionMs = clamped;
    });

    final hasTimeline =
        player.trackLengthMs != null && player.positionMs != null;
    final canSeek = player.canSeek && hasTimeline;
    if (!canSeek || (widget.pendingActions['media:seek'] ?? false)) {
      return;
    }

    await widget.onSeek(clamped);
  }

  @override
  Widget build(BuildContext context) {
    final player = widget.player;

    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          PanelSectionHeader(
            title: 'Current Media',
            count: player == null ? 0 : 1,
          ),
          const SizedBox(height: 8),
          if (player == null)
            const PanelMessage(message: 'No media session reported yet.')
          else
            _MediaCardContent(
              player: player,
              httpBaseUrl: widget.httpBaseUrl,
              pendingActions: widget.pendingActions,
              error: widget.error,
              displayPositionMs:
                  _seekDraftMs ?? _displayPositionMs ?? player.positionMs ?? 0,
              onPositionChanged: (value) {
                final clamped = clampMediaPosition(
                  value.round(),
                  player.trackLengthMs,
                );
                setState(() {
                  _seekDraftMs = clamped;
                  _displayPositionMs = clamped;
                });
              },
              onSeekCommitted: _commitSeek,
              onPrevious: widget.onPrevious,
              onTogglePlayPause: widget.onTogglePlayPause,
              onNext: widget.onNext,
            ),
        ],
      ),
    );
  }
}

class _MediaCardContent extends StatelessWidget {
  const _MediaCardContent({
    required this.player,
    required this.httpBaseUrl,
    required this.pendingActions,
    required this.error,
    required this.displayPositionMs,
    required this.onPositionChanged,
    required this.onSeekCommitted,
    required this.onPrevious,
    required this.onTogglePlayPause,
    required this.onNext,
  });

  final MediaPlayerState player;
  final String httpBaseUrl;
  final Map<String, bool> pendingActions;
  final String error;
  final int displayPositionMs;
  final ValueChanged<double> onPositionChanged;
  final ValueChanged<double> onSeekCommitted;
  final Future<void> Function() onPrevious;
  final Future<void> Function() onTogglePlayPause;
  final Future<void> Function() onNext;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final artworkUrl = resolveAssetUrl(httpBaseUrl, player.artworkUrl);
    final appIconUrl = resolveAssetUrl(httpBaseUrl, player.appIconUrl);
    final hasTimeline =
        player.trackLengthMs != null && player.positionMs != null;
    final canSeek = player.canSeek && hasTimeline;

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: theme.colorScheme.surfaceContainerHighest.withValues(
          alpha: 0.26,
        ),
        borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
        border: Border.all(
          color: theme.colorScheme.outlineVariant.withValues(alpha: 0.86),
        ),
      ),
      child: LayoutBuilder(
        builder: (context, constraints) {
          final compactTransport = constraints.maxWidth < 720;
          final ultraCompact = constraints.maxWidth < 560;

          return Column(
            children: [
              Row(
                crossAxisAlignment: CrossAxisAlignment.center,
                children: [
                  SizedBox(
                    width: 52,
                    height: 52,
                    child: ClipRRect(
                      borderRadius: BorderRadius.circular(10),
                      child: artworkUrl != null
                          ? RemoteImage(
                              url: artworkUrl,
                              fit: BoxFit.cover,
                              placeholder: _MediaFallback(
                                label: _fallbackLabel(player),
                              ),
                            )
                          : appIconUrl != null
                          ? RemoteImage(
                              url: appIconUrl,
                              fit: BoxFit.contain,
                              placeholder: _MediaFallback(
                                label: _fallbackLabel(player),
                              ),
                            )
                          : _MediaFallback(label: _fallbackLabel(player)),
                    ),
                  ),
                  const SizedBox(width: 10),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          player.title ?? 'Untitled track',
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: theme.textTheme.titleSmall,
                        ),
                        const SizedBox(height: 2),
                        Text(
                          mediaSubtitle(player),
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: theme.textTheme.bodySmall,
                        ),
                        const SizedBox(height: 4),
                        Wrap(
                          spacing: 4,
                          runSpacing: 4,
                          children: [
                            _InfoChip(label: mediaPlaybackStatusLabel(player)),
                            if (!ultraCompact)
                              _InfoChip(label: player.album ?? 'Unknown album'),
                            _InfoChip(
                              label:
                                  player.identity ??
                                  player.desktopEntry ??
                                  player.playerId,
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 10),
                  Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      _TransportButton(
                        filled: false,
                        enabled:
                            player.canControl &&
                            player.canGoPrevious &&
                            !(pendingActions['media:previous'] ?? false),
                        icon: Icons.skip_previous_rounded,
                        size: 34,
                        iconSize: 18,
                        onPressed: onPrevious,
                      ),
                      const SizedBox(width: 6),
                      _TransportButton(
                        filled: true,
                        enabled:
                            player.canControl &&
                            !(pendingActions['media:play-pause'] ?? false),
                        icon:
                            player.playbackStatus == MediaPlaybackStatus.playing
                            ? Icons.pause_rounded
                            : Icons.play_arrow_rounded,
                        size: compactTransport ? 38 : 42,
                        iconSize: compactTransport ? 20 : 22,
                        onPressed: onTogglePlayPause,
                        tooltip: mediaPrimaryActionLabel(player),
                      ),
                      const SizedBox(width: 6),
                      _TransportButton(
                        filled: false,
                        enabled:
                            player.canControl &&
                            player.canGoNext &&
                            !(pendingActions['media:next'] ?? false),
                        icon: Icons.skip_next_rounded,
                        size: 34,
                        iconSize: 18,
                        onPressed: onNext,
                      ),
                    ],
                  ),
                ],
              ),
              const SizedBox(height: 2),
              Row(
                children: [
                  Text(
                    formatDurationMs(hasTimeline ? displayPositionMs : null),
                    style: theme.textTheme.bodySmall,
                  ),
                  Expanded(
                    child: Slider(
                      value: hasTimeline ? displayPositionMs.toDouble() : 0,
                      max: (player.trackLengthMs ?? 0).toDouble(),
                      onChanged:
                          (!canSeek || (pendingActions['media:seek'] ?? false))
                          ? null
                          : onPositionChanged,
                      onChangeEnd:
                          (!canSeek || (pendingActions['media:seek'] ?? false))
                          ? null
                          : onSeekCommitted,
                    ),
                  ),
                  Text(
                    formatDurationMs(player.trackLengthMs),
                    style: theme.textTheme.bodySmall,
                  ),
                ],
              ),
              if (error.isNotEmpty) ...[
                const SizedBox(height: 4),
                Text(
                  error,
                  style: theme.textTheme.bodySmall?.copyWith(
                    color: theme.colorScheme.error,
                  ),
                ),
              ],
            ],
          );
        },
      ),
    );
  }

  String _fallbackLabel(MediaPlayerState player) {
    final value =
        player.identity ?? player.desktopEntry ?? player.title ?? 'Media';
    return value.characters.first.toUpperCase();
  }
}

class _MediaFallback extends StatelessWidget {
  const _MediaFallback({required this.label});

  final String label;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Theme.of(context).colorScheme.surfaceContainerHighest,
      child: Center(
        child: Text(label, style: Theme.of(context).textTheme.headlineSmall),
      ),
    );
  }
}

class _InfoChip extends StatelessWidget {
  const _InfoChip({required this.label});

  final String label;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface.withValues(alpha: 0.7),
        borderRadius: BorderRadius.circular(999),
      ),
      child: Text(label, style: Theme.of(context).textTheme.labelMedium),
    );
  }
}

class _TransportButton extends StatelessWidget {
  const _TransportButton({
    required this.filled,
    required this.enabled,
    required this.icon,
    required this.onPressed,
    this.size = 40,
    this.iconSize = 20,
    this.tooltip,
  });

  final bool filled;
  final bool enabled;
  final IconData icon;
  final Future<void> Function() onPressed;
  final double size;
  final double iconSize;
  final String? tooltip;

  @override
  Widget build(BuildContext context) {
    final button = filled
        ? IconButton.filled(
            onPressed: enabled ? onPressed : null,
            icon: Icon(icon, size: iconSize),
            tooltip: tooltip,
          )
        : IconButton.filledTonal(
            onPressed: enabled ? onPressed : null,
            icon: Icon(icon, size: iconSize),
            tooltip: tooltip,
          );

    return SizedBox.square(dimension: size, child: button);
  }
}
