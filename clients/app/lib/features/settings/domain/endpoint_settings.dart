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
  });

  final String httpBaseUrl;
  final String wsUrl;
  final WindowSortBy windowSortBy;
  final WindowSortDirection windowSortDirection;

  EndpointSettings copyWith({
    String? httpBaseUrl,
    String? wsUrl,
    WindowSortBy? windowSortBy,
    WindowSortDirection? windowSortDirection,
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
    );
  }

  Map<String, Object?> toJson() {
    return {
      'httpBaseUrl': httpBaseUrl,
      'wsUrl': wsUrl,
      'windowSortBy': windowSortBy.storageValue,
      'windowSortDirection': windowSortDirection.storageValue,
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
    );
  }
}
