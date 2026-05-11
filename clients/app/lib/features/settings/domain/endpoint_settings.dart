class EndpointSettings {
  const EndpointSettings({required this.httpBaseUrl, required this.wsUrl});

  final String httpBaseUrl;
  final String wsUrl;

  EndpointSettings copyWith({String? httpBaseUrl, String? wsUrl}) {
    return EndpointSettings(
      httpBaseUrl: httpBaseUrl ?? this.httpBaseUrl,
      wsUrl: wsUrl ?? this.wsUrl,
    );
  }

  Map<String, Object?> toJson() {
    return {'httpBaseUrl': httpBaseUrl, 'wsUrl': wsUrl};
  }

  factory EndpointSettings.fromJson(Map<String, Object?> json) {
    return EndpointSettings(
      httpBaseUrl: json['httpBaseUrl'] as String,
      wsUrl: json['wsUrl'] as String,
    );
  }
}
