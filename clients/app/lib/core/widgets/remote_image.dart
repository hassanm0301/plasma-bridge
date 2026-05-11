import 'dart:convert';
import 'dart:typed_data';

import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_svg/flutter_svg.dart';

import '../../features/dashboard/application/app_providers.dart';
import '../utils/url_utils.dart';

class _RemoteImagePayload {
  const _RemoteImagePayload({required this.bytes, required this.isSvg});

  final Uint8List bytes;
  final bool isSvg;
}

final _backendIconProvider = FutureProvider.family<_RemoteImagePayload, String>(
  (ref, url) async {
    final response = await ref.watch(httpClientProvider).get(Uri.parse(url));
    final bodyBytes = response.bodyBytes;
    final contentType = response.headers['content-type'] ?? '';
    final decoded = utf8.decode(bodyBytes, allowMalformed: true).trimLeft();
    final isSvg =
        contentType.contains('svg') ||
        decoded.startsWith('<svg') ||
        decoded.startsWith('<?xml');
    return _RemoteImagePayload(bytes: bodyBytes, isSvg: isSvg);
  },
);

class RemoteImage extends ConsumerWidget {
  const RemoteImage({
    super.key,
    required this.url,
    required this.fit,
    this.borderRadius = 0,
    this.placeholder,
  });

  final String url;
  final BoxFit fit;
  final double borderRadius;
  final Widget? placeholder;

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    if (!looksLikeBackendIconUrl(url)) {
      return ClipRRect(
        borderRadius: BorderRadius.circular(borderRadius),
        child: CachedNetworkImage(
          imageUrl: url,
          fit: fit,
          placeholder: (_, _) => placeholder ?? const SizedBox.expand(),
          errorWidget: (_, _, _) => placeholder ?? const SizedBox.expand(),
        ),
      );
    }

    final payload = ref.watch(_backendIconProvider(url));
    return payload.when(
      data: (data) {
        if (data.isSvg) {
          return ClipRRect(
            borderRadius: BorderRadius.circular(borderRadius),
            child: SvgPicture.memory(data.bytes, fit: fit),
          );
        }

        return ClipRRect(
          borderRadius: BorderRadius.circular(borderRadius),
          child: Image.memory(data.bytes, fit: fit),
        );
      },
      error: (_, _) => placeholder ?? const SizedBox.expand(),
      loading: () => placeholder ?? const SizedBox.expand(),
    );
  }
}
