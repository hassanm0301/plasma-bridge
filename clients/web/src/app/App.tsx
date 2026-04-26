import { useEffect, useState } from "react";

import { ConnectionScreen } from "./ConnectionScreen";
import { initialThemeMode, persistThemeMode, type ThemeMode } from "../theme/theme";

export function App() {
  const [themeMode, setThemeMode] = useState<ThemeMode>(() => initialThemeMode());

  useEffect(() => {
    document.documentElement.dataset.theme = themeMode;
    persistThemeMode(themeMode);
  }, [themeMode]);

  return <ConnectionScreen themeMode={themeMode} onThemeModeChange={setThemeMode} />;
}
