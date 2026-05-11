import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:plasma_remote_toolbar_app/app/app.dart';
import 'package:plasma_remote_toolbar_app/features/dashboard/application/app_providers.dart';
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
  });
}
