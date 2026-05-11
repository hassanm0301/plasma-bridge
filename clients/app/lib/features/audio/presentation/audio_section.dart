import 'package:flutter/material.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/widgets/desktop_panel.dart';

class AudioSection extends StatelessWidget {
  const AudioSection({
    super.key,
    required this.expanded,
    required this.title,
    required this.devices,
    required this.selectedId,
    required this.volumeDrafts,
    required this.pendingActions,
    required this.errors,
    required this.onVolumeDraftChange,
    required this.onMuteToggle,
    required this.onToggleExpanded,
    this.onVolumeCommit,
    this.volumeReadOnly = false,
  });

  final bool expanded;
  final String title;
  final List<AudioDeviceState> devices;
  final String? selectedId;
  final Map<String, double> volumeDrafts;
  final Map<String, bool> pendingActions;
  final Map<String, String> errors;
  final void Function(String deviceId, double value) onVolumeDraftChange;
  final Future<void> Function(String deviceId, double value)? onVolumeCommit;
  final Future<void> Function(AudioDeviceState device) onMuteToggle;
  final VoidCallback onToggleExpanded;
  final bool volumeReadOnly;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final activeDevice = devices.where(
      (device) => device.id == selectedId || device.isDefault,
    );
    final visibleDevices = expanded || devices.isEmpty
        ? devices
        : [activeDevice.isNotEmpty ? activeDevice.first : devices.first];

    return DesktopPanel(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          PanelSectionHeader(
            title: title,
            subtitle: volumeReadOnly
                ? 'Monitor levels and mute capture devices'
                : 'Adjust output levels and mute devices',
            count: devices.length,
            trailing: PanelExpandToggle(
              key: ValueKey(
                title == 'Playback'
                    ? 'playback-section-toggle'
                    : 'capture-section-toggle',
              ),
              expanded: expanded,
              onPressed: onToggleExpanded,
              semanticLabel: expanded
                  ? 'Compact $title devices'
                  : 'Expand $title devices',
            ),
          ),
          const SizedBox(height: DesktopMetrics.sectionGap),
          if (devices.isEmpty)
            const PanelMessage(message: 'No devices reported yet.')
          else
            ...visibleDevices.map((device) {
              final volume = volumeDrafts[device.id] ?? device.volume;
              final volumePercent = (volume.clamp(0, 1) * 100).round();
              final isSelected = device.id == selectedId || device.isDefault;
              final volumePending =
                  pendingActions['volume:${device.id}'] ?? false;
              final mutePending = pendingActions['mute:${device.id}'] ?? false;

              return Padding(
                padding: const EdgeInsets.only(bottom: 10),
                child: Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: isSelected
                        ? theme.colorScheme.primaryContainer.withValues(
                            alpha: 0.52,
                          )
                        : theme.colorScheme.surfaceContainerHighest.withValues(
                            alpha: 0.2,
                          ),
                    borderRadius: BorderRadius.circular(
                      DesktopMetrics.itemRadius,
                    ),
                    border: Border.all(
                      color: isSelected
                          ? theme.colorScheme.primary.withValues(alpha: 0.26)
                          : theme.colorScheme.outlineVariant.withValues(
                              alpha: 0.8,
                            ),
                    ),
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  device.label.isEmpty
                                      ? device.id
                                      : device.label,
                                  maxLines: 1,
                                  overflow: TextOverflow.ellipsis,
                                  style: theme.textTheme.titleSmall,
                                ),
                                const SizedBox(height: 2),
                                Text(
                                  isSelected
                                      ? 'Default device'
                                      : device.available
                                      ? 'Available'
                                      : 'Unavailable',
                                  style: theme.textTheme.bodySmall,
                                ),
                              ],
                            ),
                          ),
                          const SizedBox(width: 8),
                          Text(
                            '$volumePercent%',
                            style: theme.textTheme.labelLarge,
                          ),
                          const SizedBox(width: 4),
                          TextButton(
                            onPressed: mutePending
                                ? null
                                : () => onMuteToggle(device),
                            child: Text(device.muted ? 'Unmute' : 'Mute'),
                          ),
                        ],
                      ),
                      Row(
                        children: [
                          Icon(
                            device.muted
                                ? Icons.volume_off_rounded
                                : volumeReadOnly
                                ? Icons.mic_none_rounded
                                : Icons.volume_up_rounded,
                            size: 18,
                          ),
                          Expanded(
                            child: Slider(
                              value: volumePercent.toDouble(),
                              max: 100,
                              onChanged: volumeReadOnly || volumePending
                                  ? null
                                  : (value) => onVolumeDraftChange(
                                      device.id,
                                      value / 100,
                                    ),
                              onChangeEnd:
                                  volumeReadOnly || onVolumeCommit == null
                                  ? null
                                  : (value) =>
                                        onVolumeCommit!(device.id, value / 100),
                            ),
                          ),
                        ],
                      ),
                      if ((errors[device.id] ?? '').isNotEmpty)
                        Text(
                          errors[device.id]!,
                          style: theme.textTheme.bodySmall?.copyWith(
                            color: theme.colorScheme.error,
                          ),
                        ),
                    ],
                  ),
                ),
              );
            }),
        ],
      ),
    );
  }
}
