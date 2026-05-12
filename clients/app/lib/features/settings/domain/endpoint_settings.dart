enum WindowSortBy {
  usage('usage'),
  name('name');

  const WindowSortBy(this.storageValue);

  final String storageValue;

  static WindowSortBy fromStorageValue(String? value) {
    return WindowSortBy.values.firstWhere(
      (sortBy) => sortBy.storageValue == value,
      orElse: () => WindowSortBy.usage,
    );
  }
}

enum WindowSortDirection {
  newestFirst('newest_first'),
  oldestFirst('oldest_first'),
  ascending('asc'),
  descending('desc');

  const WindowSortDirection(this.storageValue);

  final String storageValue;

  static WindowSortDirection? tryFromStorageValue(String? value) {
    for (final direction in WindowSortDirection.values) {
      if (direction.storageValue == value) {
        return direction;
      }
    }
    return null;
  }

  bool supports(WindowSortBy sortBy) {
    switch (sortBy) {
      case WindowSortBy.usage:
        return this == WindowSortDirection.newestFirst ||
            this == WindowSortDirection.oldestFirst;
      case WindowSortBy.name:
        return this == WindowSortDirection.ascending ||
            this == WindowSortDirection.descending;
    }
  }
}

enum AppLaunchBehavior {
  openNewInstance('open_new_instance'),
  switchToExisting('switch_to_existing');

  const AppLaunchBehavior(this.storageValue);

  final String storageValue;

  static AppLaunchBehavior fromStorageValue(String? value) {
    return AppLaunchBehavior.values.firstWhere(
      (behavior) => behavior.storageValue == value,
      orElse: () => AppLaunchBehavior.openNewInstance,
    );
  }
}

WindowSortDirection defaultWindowSortDirection(WindowSortBy sortBy) {
  switch (sortBy) {
    case WindowSortBy.usage:
      return WindowSortDirection.newestFirst;
    case WindowSortBy.name:
      return WindowSortDirection.ascending;
  }
}

WindowSortDirection normalizeWindowSortDirection(
  WindowSortBy sortBy,
  WindowSortDirection? direction,
) {
  if (direction != null && direction.supports(sortBy)) {
    return direction;
  }
  return defaultWindowSortDirection(sortBy);
}

class EndpointSettings {
  const EndpointSettings({
    required this.httpBaseUrl,
    required this.wsUrl,
    required this.windowSortBy,
    required this.windowSortDirection,
    required this.appLaunchBehavior,
  });

  final String httpBaseUrl;
  final String wsUrl;
  final WindowSortBy windowSortBy;
  final WindowSortDirection windowSortDirection;
  final AppLaunchBehavior appLaunchBehavior;

  EndpointSettings copyWith({
    String? httpBaseUrl,
    String? wsUrl,
    WindowSortBy? windowSortBy,
    WindowSortDirection? windowSortDirection,
    AppLaunchBehavior? appLaunchBehavior,
  }) {
    final nextSortBy = windowSortBy ?? this.windowSortBy;
    return EndpointSettings(
      httpBaseUrl: httpBaseUrl ?? this.httpBaseUrl,
      wsUrl: wsUrl ?? this.wsUrl,
      windowSortBy: nextSortBy,
      windowSortDirection: normalizeWindowSortDirection(
        nextSortBy,
        windowSortDirection ?? this.windowSortDirection,
      ),
      appLaunchBehavior: appLaunchBehavior ?? this.appLaunchBehavior,
    );
  }

  Map<String, Object?> toJson() {
    return {
      'httpBaseUrl': httpBaseUrl,
      'wsUrl': wsUrl,
      'windowSortBy': windowSortBy.storageValue,
      'windowSortDirection': windowSortDirection.storageValue,
      'appLaunchBehavior': appLaunchBehavior.storageValue,
    };
  }

  factory EndpointSettings.fromJson(Map<String, Object?> json) {
    final String? sortByValue = json['windowSortBy'] as String?;
    final WindowSortBy sortBy;
    final WindowSortDirection direction;

    if (sortByValue != null) {
      sortBy = WindowSortBy.fromStorageValue(sortByValue);
      direction = normalizeWindowSortDirection(
        sortBy,
        WindowSortDirection.tryFromStorageValue(
          json['windowSortDirection'] as String?,
        ),
      );
    } else {
      final legacySortMode = json['windowSortMode'] as String?;
      sortBy = legacySortMode == 'name'
          ? WindowSortBy.name
          : WindowSortBy.usage;
      direction = defaultWindowSortDirection(sortBy);
    }

    return EndpointSettings(
      httpBaseUrl: json['httpBaseUrl'] as String,
      wsUrl: json['wsUrl'] as String,
      windowSortBy: sortBy,
      windowSortDirection: direction,
      appLaunchBehavior: AppLaunchBehavior.fromStorageValue(
        json['appLaunchBehavior'] as String?,
      ),
    );
  }
}
