import '../models/backend_models.dart';

String formatDurationMs(int? value) {
  if (value == null || value < 0) {
    return '--:--';
  }

  final totalSeconds = value ~/ 1000;
  final hours = totalSeconds ~/ 3600;
  final minutes = (totalSeconds % 3600) ~/ 60;
  final seconds = totalSeconds % 60;

  if (hours > 0) {
    return '$hours:${minutes.toString().padLeft(2, '0')}:${seconds.toString().padLeft(2, '0')}';
  }

  return '$minutes:${seconds.toString().padLeft(2, '0')}';
}

int clampMediaPosition(int positionMs, int? trackLengthMs) {
  final minimum = positionMs < 0 ? 0 : positionMs;
  if (trackLengthMs == null || trackLengthMs < 0) {
    return minimum;
  }
  return minimum > trackLengthMs ? trackLengthMs : minimum;
}

String mediaPrimaryActionLabel(MediaPlayerState player) {
  return player.playbackStatus == MediaPlaybackStatus.playing
      ? 'Pause'
      : 'Play';
}

String mediaSubtitle(MediaPlayerState player) {
  if (player.artists.isNotEmpty) {
    return player.artists.join(', ');
  }
  if (player.album != null && player.album!.isNotEmpty) {
    return player.album!;
  }
  return player.identity ?? player.desktopEntry ?? 'Unknown source';
}

String mediaPlaybackStatusLabel(MediaPlayerState player) {
  switch (player.playbackStatus) {
    case MediaPlaybackStatus.playing:
      return 'Playing';
    case MediaPlaybackStatus.paused:
      return 'Paused';
    case MediaPlaybackStatus.stopped:
      return 'Stopped';
    case MediaPlaybackStatus.unknown:
      return 'Unknown';
  }
}
