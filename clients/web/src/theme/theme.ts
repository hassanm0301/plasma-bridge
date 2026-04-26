export type ThemeMode = "light" | "dark";

const STORAGE_KEY = "plasma-remote-toolbar.theme";

export function initialThemeMode(): ThemeMode {
  const stored = window.localStorage.getItem(STORAGE_KEY);
  if (stored === "light" || stored === "dark") {
    return stored;
  }

  return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

export function persistThemeMode(mode: ThemeMode): void {
  window.localStorage.setItem(STORAGE_KEY, mode);
}
