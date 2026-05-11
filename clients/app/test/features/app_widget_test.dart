import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:plasma_bridge_app/app/app.dart';
import 'package:plasma_bridge_app/features/dashboard/application/app_providers.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('shows the setup screen when endpoints are unset', (
    tester,
  ) async {
    SharedPreferences.setMockInitialValues({});
    final sharedPreferences = await SharedPreferences.getInstance();

    await tester.pumpWidget(
      ProviderScope(
        overrides: [
          sharedPreferencesProvider.overrideWithValue(sharedPreferences),
        ],
        child: const PlasmaRemoteToolbarApp(),
      ),
    );

    await tester.pump();

    expect(find.text('Connect your Plasma desktop'), findsOneWidget);
    expect(find.text('Save and connect'), findsOneWidget);
    expect(find.text('Usage'), findsOneWidget);
    expect(find.text('Newest first'), findsOneWidget);
  });

  testWidgets(
    'setup screen swaps direction choices when name sorting is selected',
    (tester) async {
      SharedPreferences.setMockInitialValues({});
      final sharedPreferences = await SharedPreferences.getInstance();

      await tester.pumpWidget(
        ProviderScope(
          overrides: [
            sharedPreferencesProvider.overrideWithValue(sharedPreferences),
          ],
          child: const PlasmaRemoteToolbarApp(),
        ),
      );

      await tester.pumpAndSettle();
      await tester.tap(find.text('Name'));
      await tester.pumpAndSettle();

      expect(find.text('A-Z'), findsOneWidget);
      expect(find.text('Z-A'), findsOneWidget);
      expect(find.text('Newest first'), findsNothing);
    },
  );
}
