import 'dart:convert';

import 'package:http/http.dart' as http;

class ApiException implements Exception {
  const ApiException(this.message);

  final String message;

  @override
  String toString() => message;
}

Future<void> assertResponseOk(http.Response response) async {
  if (response.statusCode >= 200 && response.statusCode < 300) {
    return;
  }

  var message = 'HTTP ${response.statusCode} ${response.reasonPhrase ?? ''}'
      .trim();
  try {
    final json = jsonDecode(response.body) as Map<String, Object?>;
    final error = json['error'];
    if (error is Map && error['message'] is String) {
      message = error['message'] as String;
    }
  } catch (_) {
    // Keep the status fallback.
  }

  throw ApiException(message);
}
