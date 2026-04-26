import type { ThemeMode } from "../theme/theme";

interface ThemeToggleProps {
  mode: ThemeMode;
  onChange: (mode: ThemeMode) => void;
}

export function ThemeToggle({ mode, onChange }: ThemeToggleProps) {
  return (
    <div className="theme-toggle" role="group" aria-label="Theme">
      <button
        type="button"
        className={mode === "light" ? "selected" : ""}
        aria-pressed={mode === "light"}
        onClick={() => onChange("light")}
      >
        Light
      </button>
      <button
        type="button"
        className={mode === "dark" ? "selected" : ""}
        aria-pressed={mode === "dark"}
        onClick={() => onChange("dark")}
      >
        Dark
      </button>
    </div>
  );
}
