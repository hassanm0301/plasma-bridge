import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../../core/network/api_error.dart';
import '../../../core/network/endpoints.dart';

class ControlRepository {
  const ControlRepository(this._httpClient);

  final http.Client _httpClient;

  String _controlPath(String kind, String deviceId, String action) {
    return '/control/audio/$kind/${Uri.encodeComponent(deviceId)}/$action';
  }

  Future<void> _postEmpty(String httpBaseUrl, String path) async {
    final response = await _httpClient.post(
      Uri.parse('${normalizeHttpBaseUrl(httpBaseUrl)}$path'),
    );
    await assertResponseOk(response);
  }

  Future<void> _postJson(
    String httpBaseUrl,
    String path,
    Map<String, Object?> body,
  ) async {
    final response = await _httpClient.post(
      Uri.parse('${normalizeHttpBaseUrl(httpBaseUrl)}$path'),
      headers: const {'Content-Type': 'application/json'},
      body: jsonEncode(body),
    );
    await assertResponseOk(response);
  }

  Future<void> setSinkVolume(String httpBaseUrl, String sinkId, double value) {
    return _postJson(httpBaseUrl, _controlPath('sinks', sinkId, 'volume'), {
      'value': value,
    });
  }

  Future<void> setDeviceMuted(
    String httpBaseUrl,
    String kind,
    String deviceId,
    bool muted,
  ) {
    return _postJson(httpBaseUrl, _controlPath(kind, deviceId, 'mute'), {
      'muted': muted,
    });
  }

  Future<void> activateWindow(String httpBaseUrl, String windowId) {
    return _postEmpty(
      httpBaseUrl,
      '/control/windows/${Uri.encodeComponent(windowId)}/active',
    );
  }

  Future<void> controlMedia(String httpBaseUrl, String action) {
    return _postEmpty(httpBaseUrl, '/control/media/current/$action');
  }

  Future<void> seekMedia(String httpBaseUrl, int positionMs) {
    return _postJson(httpBaseUrl, '/control/media/current/seek', {
      'positionMs': positionMs,
    });
  }
}
