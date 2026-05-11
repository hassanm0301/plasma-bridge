import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_bridge_app/core/models/backend_models.dart';
import 'package:plasma_bridge_app/features/audio/presentation/audio_section.dart';

AudioDeviceState _device(
  String id,
  String label, {
  bool isDefault = false,
  bool muted = false,
}) {
  return AudioDeviceState(
    id: id,
    label: label,
    volume: 0.5,
    muted: muted,
    available: true,
    isDefault: isDefault,
    isVirtual: false,
    backendApi: 'pulse',
  );
}

Widget _buildSection({
  required bool expanded,
  required List<AudioDeviceState> devices,
  String? selectedId,
}) {
  return MaterialApp(
    home: Scaffold(
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: AudioSection(
          expanded: expanded,
          title: 'Playback',
          devices: devices,
          selectedId: selectedId,
          volumeDrafts: const {},
          pendingActions: const {},
          errors: const {},
          onVolumeDraftChange: (_, _) {},
          onVolumeCommit: (_, _) async {},
          onMuteToggle: (_) async {},
          onToggleExpanded: () {},
        ),
      ),
    ),
  );
}

void main() {
  testWidgets('compact mode only shows the active playback device', (
    tester,
  ) async {
    await tester.pumpWidget(
      _buildSection(
        expanded: false,
        selectedId: 'sink-2',
        devices: [
          _device('sink-1', 'External Speakers'),
          _device('sink-2', 'Internal Speakers'),
        ],
      ),
    );

    await tester.pumpAndSettle();

    expect(find.text('Internal Speakers'), findsOneWidget);
    expect(find.text('External Speakers'), findsNothing);
  });

  testWidgets('expanded mode shows all playback devices', (tester) async {
    await tester.pumpWidget(
      _buildSection(
        expanded: true,
        selectedId: 'sink-2',
        devices: [
          _device('sink-1', 'External Speakers'),
          _device('sink-2', 'Internal Speakers'),
        ],
      ),
    );

    await tester.pumpAndSettle();

    expect(find.text('Internal Speakers'), findsOneWidget);
    expect(find.text('External Speakers'), findsOneWidget);
  });

  testWidgets('empty state stays unchanged in compact mode', (tester) async {
    await tester.pumpWidget(_buildSection(expanded: false, devices: const []));

    await tester.pumpAndSettle();

    expect(find.text('No devices reported yet.'), findsOneWidget);
  });
}
