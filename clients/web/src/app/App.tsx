import { useEffect, useState } from "react";

import { Dashboard } from "./Dashboard";
import { initialThemeMode, persistThemeMode, type ThemeMode } from "../theme/theme";

export function App() {
  const [themeMode, setThemeMode] = useState<ThemeMode>(() => initialThemeMode());

  useEffect(() => {
    document.documentElement.dataset.theme = themeMode;
    persistThemeMode(themeMode);
  }, [themeMode]);

  return <Dashboard themeMode={themeMode} onThemeModeChange={setThemeMode} />;
}
