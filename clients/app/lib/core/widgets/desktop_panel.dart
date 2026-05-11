import 'package:flutter/material.dart';

class DesktopMetrics {
  const DesktopMetrics._();

  static const double pagePadding = 12;
  static const double sectionGap = 12;
  static const double panelPadding = 14;
  static const double panelRadius = 14;
  static const double itemRadius = 10;
}

class DesktopPanel extends StatelessWidget {
  const DesktopPanel({
    super.key,
    required this.child,
    this.padding = const EdgeInsets.all(DesktopMetrics.panelPadding),
  });

  final Widget child;
  final EdgeInsetsGeometry padding;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(padding: padding, child: child),
    );
  }
}

class PanelSectionHeader extends StatelessWidget {
  const PanelSectionHeader({
    super.key,
    required this.title,
    this.subtitle,
    this.count,
    this.trailing,
  });

  final String title;
  final String? subtitle;
  final int? count;
  final Widget? trailing;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(title, style: theme.textTheme.titleMedium),
              if ((subtitle ?? '').isNotEmpty) ...[
                const SizedBox(height: 2),
                Text(subtitle!, style: theme.textTheme.bodySmall),
              ],
            ],
          ),
        ),
        if (count != null)
          _HeaderBadge(
            label: '$count',
            backgroundColor: theme.colorScheme.secondaryContainer,
            foregroundColor: theme.colorScheme.onSecondaryContainer,
          ),
        if (count != null && trailing != null)
          const SizedBox(width: DesktopMetrics.sectionGap / 2),
        ?trailing,
      ],
    );
  }
}

class PanelExpandToggle extends StatelessWidget {
  const PanelExpandToggle({
    super.key,
    required this.expanded,
    required this.onPressed,
    required this.semanticLabel,
  });

  final bool expanded;
  final VoidCallback onPressed;
  final String semanticLabel;

  @override
  Widget build(BuildContext context) {
    return IconButton(
      onPressed: onPressed,
      tooltip: semanticLabel,
      visualDensity: VisualDensity.compact,
      icon: Icon(
        expanded ? Icons.unfold_less_rounded : Icons.unfold_more_rounded,
      ),
    );
  }
}

class PanelMessage extends StatelessWidget {
  const PanelMessage({
    super.key,
    required this.message,
    this.icon,
    this.tone = PanelMessageTone.neutral,
  });

  final String message;
  final IconData? icon;
  final PanelMessageTone tone;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final colors = switch (tone) {
      PanelMessageTone.neutral => (
        theme.colorScheme.surfaceContainerHighest,
        theme.colorScheme.onSurfaceVariant,
      ),
      PanelMessageTone.info => (
        theme.colorScheme.primaryContainer,
        theme.colorScheme.onPrimaryContainer,
      ),
      PanelMessageTone.error => (
        theme.colorScheme.errorContainer,
        theme.colorScheme.onErrorContainer,
      ),
    };

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: colors.$1.withValues(alpha: 0.88),
        borderRadius: BorderRadius.circular(DesktopMetrics.itemRadius),
      ),
      child: Row(
        children: [
          if (icon != null) ...[
            Icon(icon, size: 18, color: colors.$2),
            const SizedBox(width: 8),
          ],
          Expanded(
            child: Text(
              message,
              style: theme.textTheme.bodySmall?.copyWith(color: colors.$2),
            ),
          ),
        ],
      ),
    );
  }
}

enum PanelMessageTone { neutral, info, error }

class _HeaderBadge extends StatelessWidget {
  const _HeaderBadge({
    required this.label,
    required this.backgroundColor,
    required this.foregroundColor,
  });

  final String label;
  final Color backgroundColor;
  final Color foregroundColor;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: backgroundColor.withValues(alpha: 0.9),
        borderRadius: BorderRadius.circular(999),
      ),
      child: Text(
        label,
        style: Theme.of(context).textTheme.labelMedium?.copyWith(
          color: foregroundColor,
          fontWeight: FontWeight.w700,
        ),
      ),
    );
  }
}
