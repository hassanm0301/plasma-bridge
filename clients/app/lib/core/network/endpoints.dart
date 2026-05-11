const defaultHttpBaseUrl = 'http://127.0.0.1:8080';
const defaultWsUrl = 'ws://127.0.0.1:8081/ws';
const websocketProtocolVersion = 3;

String normalizeHttpBaseUrl(String value) {
  final trimmed = value.trim();
  if (trimmed.isEmpty) {
    throw FormatException('HTTP endpoint is required.');
  }

  final uri = Uri.parse(trimmed);
  if (uri.scheme != 'http' && uri.scheme != 'https') {
    throw FormatException('HTTP endpoint must start with http:// or https://.');
  }

  final normalized = uri.replace(
    query: null,
    fragment: null,
    path: uri.path.replaceAll(RegExp(r'/+$'), ''),
  );
  final result = normalized.toString();
  return result.endsWith('/') ? result.substring(0, result.length - 1) : result;
}

String normalizeWebSocketUrl(String value) {
  final trimmed = value.trim();
  if (trimmed.isEmpty) {
    throw FormatException('WebSocket endpoint is required.');
  }

  final uri = Uri.parse(trimmed);
  if (uri.scheme != 'ws' && uri.scheme != 'wss') {
    throw FormatException(
      'WebSocket endpoint must start with ws:// or wss://.',
    );
  }

  return uri.replace(fragment: null).toString();
}

String buildOpenApiUrl(String httpBaseUrl) {
  return '${normalizeHttpBaseUrl(httpBaseUrl)}/docs/openapi.yaml';
}
