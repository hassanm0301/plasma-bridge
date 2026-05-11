import 'package:flutter_test/flutter_test.dart';
import 'package:plasma_remote_toolbar_app/core/network/endpoints.dart';

void main() {
  group('endpoint helpers', () {
    test('normalizes HTTP base URLs before building the OpenAPI URL', () {
      expect(
        buildOpenApiUrl(' http://127.0.0.1:8080/ '),
        'http://127.0.0.1:8080/docs/openapi.yaml',
      );
    });

    test('keeps an HTTP path prefix when one is supplied', () {
      expect(
        buildOpenApiUrl('http://localhost:9000/plasma/'),
        'http://localhost:9000/plasma/docs/openapi.yaml',
      );
    });

    test('rejects non-HTTP base URLs', () {
      expect(
        () => normalizeHttpBaseUrl('ws://127.0.0.1:8081/ws'),
        throwsA(
          isA<FormatException>().having(
            (error) => error.message,
            'message',
            'HTTP endpoint must start with http:// or https://.',
          ),
        ),
      );
    });

    test('accepts websocket endpoints', () {
      expect(
        normalizeWebSocketUrl(' ws://127.0.0.1:8081/ws '),
        'ws://127.0.0.1:8081/ws',
      );
    });

    test('uses websocket protocol version 3', () {
      expect(websocketProtocolVersion, 3);
    });
  });
}
