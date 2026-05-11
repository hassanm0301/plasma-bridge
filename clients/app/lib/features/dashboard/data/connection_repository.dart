import 'dart:async';
import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:web_socket_channel/web_socket_channel.dart';

import '../../../core/dto/backend_dtos.dart';
import '../../../core/models/backend_models.dart';
import '../../../core/network/endpoints.dart';

enum HttpCheckState { idle, checking, reachable, unreachable }

class HttpCheckResult {
  const HttpCheckResult({required this.state, required this.detail});

  final HttpCheckState state;
  final String detail;
}

sealed class StateStreamEvent {
  const StateStreamEvent();
}

class StateStreamMessageEvent extends StateStreamEvent {
  const StateStreamMessageEvent(this.message);

  final BackendMessage message;
}

class StateStreamTransportErrorEvent extends StateStreamEvent {
  const StateStreamTransportErrorEvent(this.message);

  final String message;
}

class StateStreamClosedEvent extends StateStreamEvent {
  const StateStreamClosedEvent();
}

class StateStreamHandle {
  StateStreamHandle({
    required this.events,
    required Future<void> Function() close,
  }) : _close = close;

  final Stream<StateStreamEvent> events;
  final Future<void> Function() _close;

  Future<void> close() => _close();
}

class ConnectionRepository {
  const ConnectionRepository(this._httpClient);

  final http.Client _httpClient;

  Future<HttpCheckResult> checkHttpEndpoint(
    String httpBaseUrl, {
    Duration timeout = const Duration(seconds: 5),
  }) async {
    try {
      final response = await _httpClient
          .get(
            Uri.parse(buildOpenApiUrl(httpBaseUrl)),
            headers: const {
              'Accept': 'application/yaml,text/yaml,text/plain,*/*',
            },
          )
          .timeout(timeout);

      if (response.statusCode < 200 || response.statusCode >= 300) {
        return HttpCheckResult(
          state: HttpCheckState.unreachable,
          detail: 'HTTP ${response.statusCode} ${response.reasonPhrase ?? ''}'
              .trim(),
        );
      }

      if (!response.body.contains('openapi:')) {
        return const HttpCheckResult(
          state: HttpCheckState.unreachable,
          detail:
              'The endpoint responded, but it did not look like the OpenAPI spec.',
        );
      }

      return const HttpCheckResult(
        state: HttpCheckState.reachable,
        detail: 'OpenAPI spec reached.',
      );
    } catch (error) {
      return HttpCheckResult(
        state: HttpCheckState.unreachable,
        detail: error is Exception
            ? error.toString().replaceFirst('Exception: ', '')
            : 'Unknown connection error.',
      );
    }
  }

  StateStreamHandle openStateStream(String wsUrl) {
    final controller = StreamController<StateStreamEvent>();
    final channel = WebSocketChannel.connect(
      Uri.parse(normalizeWebSocketUrl(wsUrl)),
    );

    controller.onCancel = () async {
      await channel.sink.close();
    };

    channel.sink.add(
      jsonEncode({
        'type': 'hello',
        'payload': {'protocolVersion': websocketProtocolVersion},
        'error': null,
      }),
    );

    final subscription = channel.stream.listen(
      (data) {
        try {
          final message = BackendMessageDto.parse(
            jsonDecode(data as String) as Map<String, Object?>,
          );
          controller.add(StateStreamMessageEvent(message));
        } catch (error) {
          controller.add(
            StateStreamTransportErrorEvent(
              error is Exception
                  ? error.toString().replaceFirst('Exception: ', '')
                  : 'Could not read WebSocket message.',
            ),
          );
        }
      },
      onError: (_) {
        controller.add(
          const StateStreamTransportErrorEvent('WebSocket connection failed.'),
        );
      },
      onDone: () {
        controller.add(const StateStreamClosedEvent());
        controller.close();
      },
      cancelOnError: false,
    );

    return StateStreamHandle(
      events: controller.stream,
      close: () async {
        await subscription.cancel();
        await channel.sink.close();
        await controller.close();
      },
    );
  }
}
