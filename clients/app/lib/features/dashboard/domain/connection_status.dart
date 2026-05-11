enum ConnectionStatus {
  connecting,
  connected,
  notReady,
  disconnected,
  error;

  String get label {
    switch (this) {
      case ConnectionStatus.connected:
        return 'Connected';
      case ConnectionStatus.connecting:
        return 'Connecting';
      case ConnectionStatus.notReady:
        return 'Not ready';
      case ConnectionStatus.disconnected:
        return 'Disconnected';
      case ConnectionStatus.error:
        return 'Error';
    }
  }
}
