String? resolveAssetUrl(String httpBaseUrl, String? url) {
  if (url == null || url.trim().isEmpty) {
    return null;
  }

  final parsed = Uri.parse(url);
  if (parsed.hasScheme) {
    return parsed.toString();
  }

  return Uri.parse('$httpBaseUrl/').resolve(url).toString();
}

bool looksLikeBackendIconUrl(String url) {
  return Uri.parse(url).path.contains('/icons/apps/');
}
