import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../../core/dto/backend_dtos.dart';
import '../../../core/models/backend_models.dart';
import '../../../core/network/api_error.dart';
import '../../../core/network/endpoints.dart';

class ControlRepository {
  const ControlRepository(this._httpClient);

  final http.Client _httpClient;

  String _controlPath(String kind, String deviceId, String action) {
    return '/control/audio/$kind/${Uri.encodeComponent(deviceId)}/$action';
  }

  Uri _buildUri(
    String httpBaseUrl,
    String path, {
    Map<String, String>? queryParameters,
  }) {
    return Uri.parse(
      '${normalizeHttpBaseUrl(httpBaseUrl)}$path',
    ).replace(queryParameters: queryParameters);
  }

  Map<String, Object?> _readPayload(http.Response response) {
    final parsed = jsonDecode(response.body) as Map<String, Object?>;
    final payload = parsed['payload'];
    if (payload is! Map) {
      throw const ApiException('Backend response did not include a payload.');
    }
    return payload.cast<String, Object?>();
  }

  List<AppInfo> _readApps(http.Response response) {
    final payload = _readPayload(response);
    final apps = payload['apps'];
    if (apps is! List) {
      return const [];
    }

    return apps
        .map(
          (item) =>
              AppInfoDto((item as Map).cast<String, Object?>()).toDomain(),
        )
        .toList(growable: false);
  }

  Future<void> _postEmpty(String httpBaseUrl, String path) async {
    final response = await _httpClient.post(_buildUri(httpBaseUrl, path));
    await assertResponseOk(response);
  }

  Future<void> _postJson(
    String httpBaseUrl,
    String path,
    Map<String, Object?> body,
  ) async {
    final response = await _httpClient.post(
      _buildUri(httpBaseUrl, path),
      headers: const {'Content-Type': 'application/json'},
      body: jsonEncode(body),
    );
    await assertResponseOk(response);
  }

  Future<List<AppInfo>> fetchAvailableApps(
    String httpBaseUrl, {
    String query = '',
  }) async {
    final queryParameters = query.trim().isEmpty ? null : {'q': query.trim()};
    final response = await _httpClient.get(
      _buildUri(
        httpBaseUrl,
        '/snapshot/apps',
        queryParameters: queryParameters,
      ),
    );
    await assertResponseOk(response);
    return _readApps(response);
  }

  Future<List<AppInfo>> fetchFavoriteApps(String httpBaseUrl) async {
    final response = await _httpClient.get(
      _buildUri(httpBaseUrl, '/snapshot/apps/favorites'),
    );
    await assertResponseOk(response);
    return _readApps(response);
  }

  Future<void> favoriteApp(String httpBaseUrl, String appId) {
    return _postEmpty(
      httpBaseUrl,
      '/control/apps/${Uri.encodeComponent(appId)}/favorite',
    );
  }

  Future<void> unfavoriteApp(String httpBaseUrl, String appId) {
    return _postEmpty(
      httpBaseUrl,
      '/control/apps/${Uri.encodeComponent(appId)}/unfavorite',
    );
  }

  Future<void> openApp(
    String httpBaseUrl,
    String appId, {
    bool switchToExisting = false,
  }) async {
    final response = await _httpClient.post(
      _buildUri(
        httpBaseUrl,
        '/control/apps/${Uri.encodeComponent(appId)}/open',
        queryParameters: switchToExisting
            ? const {'switchToExisting': 'true'}
            : null,
      ),
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

  Future<void> closeWindow(String httpBaseUrl, String windowId) {
    return _postEmpty(
      httpBaseUrl,
      '/control/windows/${Uri.encodeComponent(windowId)}/close',
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
