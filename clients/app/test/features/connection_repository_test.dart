import 'package:flutter_test/flutter_test.dart';
import 'package:http/http.dart' as http;
import 'package:http/testing.dart';
import 'package:plasma_bridge_app/features/dashboard/data/connection_repository.dart';

void main() {
  test('reports a reachable OpenAPI endpoint', () async {
    final repository = ConnectionRepository(
      MockClient((request) async {
        expect(request.url.toString(), 'http://example.test/docs/openapi.yaml');
        return http.Response('openapi: 3.2.0', 200);
      }),
    );

    final result = await repository.checkHttpEndpoint('http://example.test');

    expect(result.state, HttpCheckState.reachable);
  });

  test('reports an unexpected body as unreachable', () async {
    final repository = ConnectionRepository(
      MockClient((_) async => http.Response('hello', 200)),
    );

    final result = await repository.checkHttpEndpoint('http://example.test');

    expect(result.state, HttpCheckState.unreachable);
    expect(result.detail, contains('did not look like the OpenAPI spec'));
  });
}
