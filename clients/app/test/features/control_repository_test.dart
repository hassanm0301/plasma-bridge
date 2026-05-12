import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:plasma_bridge_app/features/dashboard/data/control_repository.dart';

void main() {
  test('posts to the window close endpoint', () async {
    final repository = ControlRepository(
      MockClient((request) async {
        expect(request.method, 'POST');
        expect(
          request.url.toString(),
          'http://example.test/control/windows/window%20editor/close',
        );
        return http.Response(
          '{"payload":{"windowId":"window editor"},"error":null}',
          200,
        );
      }),
    );

    await repository.closeWindow('http://example.test', 'window editor');
  });

  test('searches apps using the q query parameter', () async {
    final repository = ControlRepository(
      MockClient((request) async {
        expect(request.method, 'GET');
        expect(
          request.url.toString(),
          'http://example.test/snapshot/apps?q=konsole',
        );
        return http.Response(
          '{"payload":{"apps":[{"appId":"org.kde.konsole.desktop","name":"Konsole","genericName":"Terminal Emulator","desktopEntryName":"org.kde.konsole","menuId":"org.kde.konsole.desktop","iconUrl":"/icons/apps/org.kde.konsole"}]},"error":null}',
          200,
        );
      }),
    );

    final apps = await repository.fetchAvailableApps(
      'http://example.test',
      query: 'konsole',
    );
    expect(apps.single.appId, 'org.kde.konsole.desktop');
  });

  test('opens an app with switchToExisting when requested', () async {
    final repository = ControlRepository(
      MockClient((request) async {
        expect(request.method, 'POST');
        expect(
          request.url.toString(),
          'http://example.test/control/apps/org.kde.kate.desktop/open?switchToExisting=true',
        );
        return http.Response(
          '{"payload":{"appId":"org.kde.kate.desktop"},"error":null}',
          200,
        );
      }),
    );

    await repository.openApp(
      'http://example.test',
      'org.kde.kate.desktop',
      switchToExisting: true,
    );
  });

  test('opens a new app instance by default', () async {
    final repository = ControlRepository(
      MockClient((request) async {
        expect(request.method, 'POST');
        expect(
          request.url.toString(),
          'http://example.test/control/apps/org.kde.kate.desktop/open',
        );
        return http.Response(
          '{"payload":{"appId":"org.kde.kate.desktop"},"error":null}',
          200,
        );
      }),
    );

    await repository.openApp('http://example.test', 'org.kde.kate.desktop');
  });
}
