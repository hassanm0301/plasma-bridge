import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../../core/models/backend_models.dart';
import '../../../core/network/api_error.dart';
import '../../../core/utils/backend_state_helpers.dart';
import '../../settings/domain/endpoint_settings.dart';
import '../data/connection_repository.dart';
import '../data/control_repository.dart';
import '../domain/connection_status.dart';
import 'app_providers.dart';

class DashboardState {
  const DashboardState({
    required this.backendState,
    required this.favoriteApps,
    required this.appSearchResults,
    required this.favoriteAppsAddMode,
    required this.favoriteAppsEditMode,
    required this.appSearchQuery,
    required this.appSearchLoading,
    required this.connectionStatus,
    required this.connectionDetail,
    required this.httpStatus,
    required this.httpDetail,
    required this.pendingActions,
    required this.rowErrors,
    required this.volumeDrafts,
  });

  factory DashboardState.initial() {
    return const DashboardState(
      backendState: BackendState.empty(),
      favoriteApps: [],
      appSearchResults: [],
      favoriteAppsAddMode: false,
      favoriteAppsEditMode: false,
      appSearchQuery: '',
      appSearchLoading: false,
      connectionStatus: ConnectionStatus.connecting,
      connectionDetail: 'Opening WebSocket stream...',
      httpStatus: HttpCheckState.checking,
      httpDetail: 'Checking OpenAPI endpoint...',
      pendingActions: {},
      rowErrors: {},
      volumeDrafts: {},
    );
  }

  final BackendState backendState;
  final List<AppInfo> favoriteApps;
  final List<AppInfo> appSearchResults;
  final bool favoriteAppsAddMode;
  final bool favoriteAppsEditMode;
  final String appSearchQuery;
  final bool appSearchLoading;
  final ConnectionStatus connectionStatus;
  final String connectionDetail;
  final HttpCheckState httpStatus;
  final String httpDetail;
  final Map<String, bool> pendingActions;
  final Map<String, String> rowErrors;
  final Map<String, double> volumeDrafts;

  DashboardState copyWith({
    BackendState? backendState,
    List<AppInfo>? favoriteApps,
    List<AppInfo>? appSearchResults,
    bool? favoriteAppsAddMode,
    bool? favoriteAppsEditMode,
    String? appSearchQuery,
    bool? appSearchLoading,
    ConnectionStatus? connectionStatus,
    String? connectionDetail,
    HttpCheckState? httpStatus,
    String? httpDetail,
    Map<String, bool>? pendingActions,
    Map<String, String>? rowErrors,
    Map<String, double>? volumeDrafts,
  }) {
    return DashboardState(
      backendState: backendState ?? this.backendState,
      favoriteApps: favoriteApps ?? this.favoriteApps,
      appSearchResults: appSearchResults ?? this.appSearchResults,
      favoriteAppsAddMode: favoriteAppsAddMode ?? this.favoriteAppsAddMode,
      favoriteAppsEditMode: favoriteAppsEditMode ?? this.favoriteAppsEditMode,
      appSearchQuery: appSearchQuery ?? this.appSearchQuery,
      appSearchLoading: appSearchLoading ?? this.appSearchLoading,
      connectionStatus: connectionStatus ?? this.connectionStatus,
      connectionDetail: connectionDetail ?? this.connectionDetail,
      httpStatus: httpStatus ?? this.httpStatus,
      httpDetail: httpDetail ?? this.httpDetail,
      pendingActions: pendingActions ?? this.pendingActions,
      rowErrors: rowErrors ?? this.rowErrors,
      volumeDrafts: volumeDrafts ?? this.volumeDrafts,
    );
  }
}

class DashboardController extends Notifier<DashboardState> {
  late ConnectionRepository _connectionRepository;
  late ControlRepository _controlRepository;
  late EndpointSettings _settings;

  StateStreamHandle? _streamHandle;
  StreamSubscription<StateStreamEvent>? _streamSubscription;
  Timer? _appSearchDebounce;
  int _appSearchRequestId = 0;
  bool _closedByController = false;
  bool _hasState = false;

  @override
  DashboardState build() {
    _connectionRepository = ref.watch(connectionRepositoryProvider);
    _controlRepository = ref.watch(controlRepositoryProvider);
    _settings = ref.watch(settingsControllerProvider).endpointSettings!;

    ref.onDispose(() {
      _appSearchDebounce?.cancel();
      unawaited(_closeStream());
    });

    Future.microtask(connect);
    return DashboardState.initial();
  }

  Future<void> connect() async {
    _hasState = false;
    _closedByController = false;
    state = state.copyWith(
      backendState: const BackendState.empty(),
      favoriteApps: const [],
      appSearchResults: const [],
      favoriteAppsAddMode: false,
      favoriteAppsEditMode: false,
      appSearchQuery: '',
      appSearchLoading: false,
      connectionStatus: ConnectionStatus.connecting,
      connectionDetail: 'Opening WebSocket stream...',
      httpStatus: HttpCheckState.checking,
      httpDetail: 'Checking OpenAPI endpoint...',
      volumeDrafts: const {},
    );

    unawaited(_checkHttp());
    unawaited(_refreshFavoriteApps());
    await _openStream();
  }

  Future<void> reconnect() async {
    await _closeStream();
    await connect();
  }

  Future<void> handleAppResumed() async {
    await reconnect();
  }

  void updateVolumeDraft(String deviceId, double value) {
    final drafts = Map<String, double>.from(state.volumeDrafts);
    drafts[deviceId] = value;
    state = state.copyWith(volumeDrafts: drafts);
  }

  Future<void> commitSinkVolume(String sinkId, double value) async {
    await _runDeviceAction(
      deviceId: sinkId,
      actionKey: 'volume:$sinkId',
      action: () => _controlRepository.setSinkVolume(
        _settings.httpBaseUrl,
        sinkId,
        value,
      ),
    );
  }

  Future<void> toggleSinkMuted(AudioDeviceState device) async {
    await _runDeviceAction(
      deviceId: device.id,
      actionKey: 'mute:${device.id}',
      action: () => _controlRepository.setDeviceMuted(
        _settings.httpBaseUrl,
        'sinks',
        device.id,
        !device.muted,
      ),
    );
  }

  Future<void> toggleSourceMuted(AudioDeviceState device) async {
    await _runDeviceAction(
      deviceId: device.id,
      actionKey: 'mute:${device.id}',
      action: () => _controlRepository.setDeviceMuted(
        _settings.httpBaseUrl,
        'sources',
        device.id,
        !device.muted,
      ),
    );
  }

  Future<void> activateWindow(String windowId) async {
    await _runWindowAction(
      windowId: windowId,
      actionKey: 'window-active:$windowId',
      action: () =>
          _controlRepository.activateWindow(_settings.httpBaseUrl, windowId),
    );
  }

  Future<void> closeWindow(String windowId) async {
    await _runWindowAction(
      windowId: windowId,
      actionKey: 'window-close:$windowId',
      action: () =>
          _controlRepository.closeWindow(_settings.httpBaseUrl, windowId),
    );
  }

  Future<void> performMediaAction(String action) async {
    final player = state.backendState.media?.player;
    await _runMediaAction(
      player,
      action,
      () => _controlRepository.controlMedia(_settings.httpBaseUrl, action),
    );
  }

  Future<void> seekCurrentMedia(int positionMs) async {
    final player = state.backendState.media?.player;
    await _runMediaAction(
      player,
      'seek',
      () => _controlRepository.seekMedia(_settings.httpBaseUrl, positionMs),
    );
  }

  void showFavoriteAppsAddMode() {
    _appSearchDebounce?.cancel();
    _setRowError('apps', '');
    state = state.copyWith(
      favoriteAppsAddMode: true,
      favoriteAppsEditMode: false,
      appSearchQuery: '',
      appSearchResults: const [],
      appSearchLoading: false,
    );
  }

  void hideFavoriteAppsAddMode() {
    _appSearchDebounce?.cancel();
    state = state.copyWith(
      favoriteAppsAddMode: false,
      appSearchQuery: '',
      appSearchResults: const [],
      appSearchLoading: false,
    );
  }

  void toggleFavoriteAppsEditMode() {
    _appSearchDebounce?.cancel();
    state = state.copyWith(
      favoriteAppsEditMode: !state.favoriteAppsEditMode,
      favoriteAppsAddMode: false,
      appSearchQuery: '',
      appSearchResults: const [],
      appSearchLoading: false,
    );
  }

  void updateAppSearchQuery(String query) {
    _appSearchDebounce?.cancel();
    _setRowError('apps', '');
    state = state.copyWith(
      appSearchQuery: query,
      appSearchResults: const [],
      appSearchLoading: query.trim().isNotEmpty,
    );

    if (query.trim().isEmpty) {
      return;
    }

    final requestId = ++_appSearchRequestId;
    _appSearchDebounce = Timer(const Duration(milliseconds: 250), () {
      unawaited(_searchAvailableApps(query, requestId));
    });
  }

  Future<void> addFavoriteApp(AppInfo app) async {
    final actionKey = 'app-favorite:${app.appId}';
    _setPending(actionKey, true);
    _setRowError('apps', '');
    try {
      await _controlRepository.favoriteApp(_settings.httpBaseUrl, app.appId);
      hideFavoriteAppsAddMode();
      await _refreshFavoriteApps();
    } catch (error) {
      _setRowError('apps', _messageFromError(error));
    } finally {
      _setPending(actionKey, false);
    }
  }

  Future<void> removeFavoriteApp(AppInfo app) async {
    final actionKey = 'app-unfavorite:${app.appId}';
    _setPending(actionKey, true);
    _setRowError('apps', '');
    try {
      await _controlRepository.unfavoriteApp(_settings.httpBaseUrl, app.appId);
      await _refreshFavoriteApps();
    } catch (error) {
      _setRowError('apps', _messageFromError(error));
    } finally {
      _setPending(actionKey, false);
    }
  }

  Future<void> openFavoriteApp(AppInfo app) async {
    final actionKey = 'app-open:${app.appId}';
    _setPending(actionKey, true);
    _setRowError('apps', '');
    try {
      await _controlRepository.openApp(
        _settings.httpBaseUrl,
        app.appId,
        switchToExisting: _shouldSwitchToExisting(),
      );
    } catch (error) {
      _setRowError('apps', _messageFromError(error));
    } finally {
      _setPending(actionKey, false);
    }
  }

  Future<void> _searchAvailableApps(String query, int requestId) async {
    try {
      final apps = await _controlRepository.fetchAvailableApps(
        _settings.httpBaseUrl,
        query: query,
      );
      if (requestId != _appSearchRequestId || query != state.appSearchQuery) {
        return;
      }

      final favoriteIds = state.favoriteApps.map((app) => app.appId).toSet();
      state = state.copyWith(
        appSearchResults: apps
            .where((app) => !favoriteIds.contains(app.appId))
            .toList(growable: false),
        appSearchLoading: false,
      );
    } catch (error) {
      if (requestId != _appSearchRequestId) {
        return;
      }
      _setRowError('apps', _messageFromError(error));
      state = state.copyWith(
        appSearchLoading: false,
        appSearchResults: const [],
      );
    }
  }

  Future<void> _refreshFavoriteApps() async {
    try {
      final apps = await _controlRepository.fetchFavoriteApps(
        _settings.httpBaseUrl,
      );
      state = state.copyWith(favoriteApps: apps);
    } catch (error) {
      _setRowError('apps', _messageFromError(error));
    }
  }

  Future<void> _checkHttp() async {
    final result = await _connectionRepository.checkHttpEndpoint(
      _settings.httpBaseUrl,
    );
    state = state.copyWith(httpStatus: result.state, httpDetail: result.detail);
  }

  Future<void> _openStream() async {
    try {
      final handle = _connectionRepository.openStateStream(_settings.wsUrl);
      _streamHandle = handle;
      _streamSubscription = handle.events.listen(_onStreamEvent);
    } catch (error) {
      state = state.copyWith(
        connectionStatus: ConnectionStatus.error,
        connectionDetail: error is Exception
            ? error.toString().replaceFirst('Exception: ', '')
            : 'Invalid WebSocket endpoint.',
      );
    }
  }

  Future<void> _closeStream() async {
    _closedByController = true;
    _appSearchDebounce?.cancel();
    await _streamSubscription?.cancel();
    _streamSubscription = null;
    await _streamHandle?.close();
    _streamHandle = null;
  }

  void _onStreamEvent(StateStreamEvent event) {
    if (event is StateStreamTransportErrorEvent) {
      state = state.copyWith(
        connectionStatus: ConnectionStatus.error,
        connectionDetail: event.message,
      );
      return;
    }

    if (event is StateStreamClosedEvent) {
      if (_closedByController) {
        return;
      }
      state = state.copyWith(
        connectionStatus: _hasState
            ? ConnectionStatus.disconnected
            : ConnectionStatus.error,
        connectionDetail: _hasState
            ? 'WebSocket disconnected.'
            : 'WebSocket closed before state was received.',
      );
      return;
    }

    final message = (event as StateStreamMessageEvent).message;
    if (message is ErrorMessage) {
      state = state.copyWith(
        connectionStatus: message.code == 'not_ready'
            ? ConnectionStatus.notReady
            : ConnectionStatus.error,
        connectionDetail: message.message,
      );
      return;
    }

    final previousPlayerId = state.backendState.media?.player?.playerId;
    final nextBackendState = applyBackendMessage(state.backendState, message);
    final nextRowErrors = Map<String, String>.from(state.rowErrors);
    if (previousPlayerId != nextBackendState.media?.player?.playerId) {
      nextRowErrors['media'] = '';
    }

    _hasState = true;
    state = state.copyWith(
      backendState: nextBackendState,
      connectionStatus: ConnectionStatus.connected,
      connectionDetail: 'Live state stream connected.',
      rowErrors: nextRowErrors,
      volumeDrafts: const {},
    );
  }

  Future<void> _runDeviceAction({
    required String deviceId,
    required String actionKey,
    required Future<void> Function() action,
  }) async {
    _setPending(actionKey, true);
    _setRowError(deviceId, '');
    try {
      await action();
    } catch (error) {
      _setRowError(deviceId, _messageFromError(error));
    } finally {
      _setPending(actionKey, false);
    }
  }

  Future<void> _runWindowAction({
    required String windowId,
    required String actionKey,
    required Future<void> Function() action,
  }) async {
    _setPending(actionKey, true);
    _setRowError(windowId, '');
    try {
      await action();
    } catch (error) {
      _setRowError(windowId, _messageFromError(error));
    } finally {
      _setPending(actionKey, false);
    }
  }

  Future<void> _runMediaAction(
    MediaPlayerState? player,
    String actionName,
    Future<void> Function() action,
  ) async {
    final actionKey = 'media:$actionName';
    _setPending(actionKey, true);
    _setRowError('media', '');
    try {
      await action();
    } catch (error) {
      final prefix =
          player?.identity ??
          player?.desktopEntry ??
          player?.playerId ??
          'Current player';
      _setRowError('media', '$prefix: ${_messageFromError(error)}');
    } finally {
      _setPending(actionKey, false);
    }
  }

  void _setPending(String key, bool value) {
    final nextPending = Map<String, bool>.from(state.pendingActions);
    nextPending[key] = value;
    state = state.copyWith(pendingActions: nextPending);
  }

  void _setRowError(String key, String value) {
    final nextErrors = Map<String, String>.from(state.rowErrors);
    nextErrors[key] = value;
    state = state.copyWith(rowErrors: nextErrors);
  }

  String _messageFromError(Object error) {
    if (error is ApiException) {
      return error.message;
    }
    return error is Exception
        ? error.toString().replaceFirst('Exception: ', '')
        : 'Request failed.';
  }

  bool _shouldSwitchToExisting() {
    return !kIsWeb &&
        defaultTargetPlatform == TargetPlatform.android &&
        _settings.appLaunchBehavior == AppLaunchBehavior.switchToExisting;
  }
}
