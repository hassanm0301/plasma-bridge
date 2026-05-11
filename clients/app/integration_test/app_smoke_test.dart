import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:integration_test/integration_test.dart';
import 'package:plasma_bridge_app/app/app.dart';
import 'package:plasma_bridge_app/features/dashboard/application/app_providers.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('shows first-run setup when no endpoints are stored', (
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

    await tester.pumpAndSettle();

    expect(find.text('Connect your Plasma desktop'), findsOneWidget);
  });
}
