# BadlionMenuUI 1.1.5 — Single editable HUD button

- Removed the independent fixed-position Android/WebView floating button which could not be moved by Levi HUD Editor and persisted on unrelated screens.
- Kept the native `pl::modmenu::registerButton` icon, pressed icon, real callback and Levi HUD Editor positioning/resizing.
- Removed the HTML-only floating logo fallback; installed menu now opens through the Levi native launcher button or programmatically with `BadlionUI_SetVisible`.
- Removed the three-line `≡` shortcut beside Search in all layouts. Standard vs Square tile sizing remains in Settings.
- Fixed the 320px viewport centering regression caused by oversized unscaled CSS grid tracks.
- Preserved existing developer module callbacks and custom categories, three template cards, title customization, Normal/Compact modes, profiles, themes and WebView startup menu host.
- Updated the full source, tests and 12-page offline GitHub Pages docs. NDK compilation and Chromium tests passed; device verification is still needed.

**Note:** The sole Levi button may not render during early startup before the native HUD is ready. This is intentional to avoid a second non-editable overlay. Full HTML menu may still attach during startup.
