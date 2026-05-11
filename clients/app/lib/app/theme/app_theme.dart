import 'package:flutter/material.dart';

class AppTheme {
  const AppTheme._();

  static ThemeData lightTheme() {
    const colorScheme = ColorScheme(
      brightness: Brightness.light,
      primary: Color(0xFF2A74A7),
      onPrimary: Color(0xFFFFFFFF),
      primaryContainer: Color(0xFFD8EBF9),
      onPrimaryContainer: Color(0xFF16364E),
      secondary: Color(0xFF5E7387),
      onSecondary: Color(0xFFFFFFFF),
      secondaryContainer: Color(0xFFDCE5EE),
      onSecondaryContainer: Color(0xFF243746),
      tertiary: Color(0xFF4F6E86),
      onTertiary: Color(0xFFFFFFFF),
      tertiaryContainer: Color(0xFFD6E6F3),
      onTertiaryContainer: Color(0xFF173446),
      error: Color(0xFFB33E3E),
      onError: Color(0xFFFFFFFF),
      errorContainer: Color(0xFFF8DADA),
      onErrorContainer: Color(0xFF4C1616),
      surface: Color(0xFFF1F4F7),
      onSurface: Color(0xFF1D242B),
      onSurfaceVariant: Color(0xFF586470),
      outline: Color(0xFFB8C4D0),
      outlineVariant: Color(0xFFD5DDE5),
      shadow: Color(0x33000000),
      scrim: Color(0x66000000),
      inverseSurface: Color(0xFF262F38),
      onInverseSurface: Color(0xFFEAF0F4),
      inversePrimary: Color(0xFF90CCF3),
      surfaceTint: Color(0xFF2A74A7),
      surfaceDim: Color(0xFFE2E7EC),
      surfaceBright: Color(0xFFF9FBFC),
      surfaceContainerLowest: Color(0xFFFFFFFF),
      surfaceContainerLow: Color(0xFFF6F8FA),
      surfaceContainer: Color(0xFFEEF2F6),
      surfaceContainerHigh: Color(0xFFE8EDF2),
      surfaceContainerHighest: Color(0xFFDEE6EE),
    );

    return _buildTheme(colorScheme);
  }

  static ThemeData darkTheme() {
    const colorScheme = ColorScheme(
      brightness: Brightness.dark,
      primary: Color(0xFF74B9E4),
      onPrimary: Color(0xFF0F2E42),
      primaryContainer: Color(0xFF214864),
      onPrimaryContainer: Color(0xFFD4ECFB),
      secondary: Color(0xFFB1C3D3),
      onSecondary: Color(0xFF22313F),
      secondaryContainer: Color(0xFF334556),
      onSecondaryContainer: Color(0xFFDDE6EE),
      tertiary: Color(0xFF9FC5E1),
      onTertiary: Color(0xFF0E3048),
      tertiaryContainer: Color(0xFF26465E),
      onTertiaryContainer: Color(0xFFD9EAF7),
      error: Color(0xFFFF8F8F),
      onError: Color(0xFF591616),
      errorContainer: Color(0xFF742727),
      onErrorContainer: Color(0xFFFFDADA),
      surface: Color(0xFF252A31),
      onSurface: Color(0xFFE8EDF2),
      onSurfaceVariant: Color(0xFFB6C0CA),
      outline: Color(0xFF54606C),
      outlineVariant: Color(0xFF39424B),
      shadow: Color(0x99000000),
      scrim: Color(0x99000000),
      inverseSurface: Color(0xFFE7ECF0),
      onInverseSurface: Color(0xFF1D252C),
      inversePrimary: Color(0xFF2C77AB),
      surfaceTint: Color(0xFF74B9E4),
      surfaceDim: Color(0xFF1A1F25),
      surfaceBright: Color(0xFF313841),
      surfaceContainerLowest: Color(0xFF181C22),
      surfaceContainerLow: Color(0xFF1F242B),
      surfaceContainer: Color(0xFF252A31),
      surfaceContainerHigh: Color(0xFF2B3138),
      surfaceContainerHighest: Color(0xFF343B44),
    );

    return _buildTheme(colorScheme);
  }

  static ThemeData _buildTheme(ColorScheme colorScheme) {
    final base = ThemeData(
      useMaterial3: true,
      colorScheme: colorScheme,
      visualDensity: VisualDensity.compact,
      scaffoldBackgroundColor: colorScheme.surface,
      cardTheme: CardThemeData(
        elevation: 0,
        color: colorScheme.surfaceContainerLow,
        margin: EdgeInsets.zero,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: BorderSide(
            color: colorScheme.outlineVariant.withValues(alpha: 0.8),
          ),
        ),
      ),
      appBarTheme: AppBarTheme(
        elevation: 0,
        scrolledUnderElevation: 0,
        centerTitle: false,
        backgroundColor: colorScheme.surface,
        foregroundColor: colorScheme.onSurface,
        surfaceTintColor: Colors.transparent,
        toolbarHeight: 58,
        titleTextStyle: TextStyle(
          color: colorScheme.onSurface,
          fontSize: 18,
          fontWeight: FontWeight.w700,
          letterSpacing: 0.2,
        ),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        isDense: true,
        fillColor: colorScheme.surfaceContainerLowest,
        contentPadding: const EdgeInsets.symmetric(
          horizontal: 12,
          vertical: 12,
        ),
        border: _inputBorder(colorScheme.outlineVariant),
        enabledBorder: _inputBorder(colorScheme.outlineVariant),
        focusedBorder: _inputBorder(colorScheme.primary),
        errorBorder: _inputBorder(colorScheme.error),
        focusedErrorBorder: _inputBorder(colorScheme.error),
      ),
    );

    return base.copyWith(
      dividerTheme: DividerThemeData(
        color: colorScheme.outlineVariant.withValues(alpha: 0.85),
        thickness: 1,
        space: 1,
      ),
      iconTheme: IconThemeData(color: colorScheme.onSurfaceVariant, size: 20),
      iconButtonTheme: IconButtonThemeData(
        style: ButtonStyle(
          visualDensity: VisualDensity.compact,
          padding: WidgetStateProperty.all(EdgeInsets.zero),
          minimumSize: WidgetStateProperty.all(const Size(36, 36)),
          shape: WidgetStateProperty.all(
            RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          ),
        ),
      ),
      chipTheme: base.chipTheme.copyWith(
        backgroundColor: colorScheme.surfaceContainerHighest.withValues(
          alpha: 0.72,
        ),
        side: BorderSide.none,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(999)),
        labelStyle: TextStyle(
          color: colorScheme.onSurfaceVariant,
          fontSize: 12,
          fontWeight: FontWeight.w600,
        ),
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: ButtonStyle(
          minimumSize: WidgetStateProperty.all(const Size(0, 40)),
          padding: WidgetStateProperty.all(
            const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
          ),
          shape: WidgetStateProperty.all(
            RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          ),
          textStyle: WidgetStateProperty.all(
            const TextStyle(fontSize: 14, fontWeight: FontWeight.w600),
          ),
        ),
      ),
      textButtonTheme: TextButtonThemeData(
        style: ButtonStyle(
          minimumSize: WidgetStateProperty.all(const Size(0, 34)),
          padding: WidgetStateProperty.all(
            const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
          ),
          shape: WidgetStateProperty.all(
            RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          ),
          textStyle: WidgetStateProperty.all(
            const TextStyle(fontSize: 13, fontWeight: FontWeight.w600),
          ),
        ),
      ),
      segmentedButtonTheme: SegmentedButtonThemeData(
        style: ButtonStyle(
          visualDensity: VisualDensity.compact,
          padding: WidgetStateProperty.all(
            const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
          ),
          shape: WidgetStateProperty.all(
            RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
          ),
        ),
      ),
      bottomSheetTheme: BottomSheetThemeData(
        backgroundColor: colorScheme.surfaceContainerLow,
        surfaceTintColor: Colors.transparent,
        shape: const RoundedRectangleBorder(
          borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
        ),
      ),
      sliderTheme: base.sliderTheme.copyWith(
        trackHeight: 4,
        thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 7),
        overlayShape: const RoundSliderOverlayShape(overlayRadius: 14),
        inactiveTrackColor: colorScheme.surfaceContainerHighest,
        activeTrackColor: colorScheme.primary,
        thumbColor: colorScheme.primary,
      ),
      listTileTheme: const ListTileThemeData(
        dense: true,
        visualDensity: VisualDensity.compact,
        contentPadding: EdgeInsets.zero,
      ),
    );
  }

  static OutlineInputBorder _inputBorder(Color color) {
    return OutlineInputBorder(
      borderRadius: BorderRadius.circular(10),
      borderSide: BorderSide(color: color),
    );
  }
}
