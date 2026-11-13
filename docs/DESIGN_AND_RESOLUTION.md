# UI design, themes and screen-resolution scaling

Based on the user-provided screenshots: dark translucent Badlion-style module menu, a black/blue logo, a compact toolbar with Mods/Settings/Profiles, left category filter, a module grid with icons and on/off switches, and an Edit GUI action. This project is not affiliated with the original Badlion Client. Compact Mode is an added variation.

## Layouts

| Layout | Logical panel | Grid | Behavior |
|---|---:|---:|---|
| Normal | 820 × 460 CSS px | 6 columns | Closest to the provided image 1 reference |
| Compact | 570 × 475 CSS px | 4 columns | Dense rows of small icons, text and switches |
| Portrait adaptation | 380 × 690 CSS px | 2 columns | Auto-applies on narrow phones without changing saved Normal/Compact preference |

The toolbar and module grid are native **HTML/CSS**, not pixel screenshots. Module data comes from three native callback-backed template cards plus developer-registered cards; the category list comes from native `categories` snapshot data. The All filter aggregates every category, while PvP/Hypixel/Visual each initially contain exactly one template. The user-provided PNG logo appears on each starter template card, is embedded in the standalone HTML, and is composited into the supplied Levi HUD button templates; the launcher button is separate from module cards.

## Resolution adaptation

`updateScale()` reads `window.visualViewport` if available, otherwise `innerWidth/innerHeight`. It checks portrait orientation, selects a layout, and calculates a scale bounded by available CSS viewport space with 10px margins. User Scale can be adjusted between 70% and 130%; the panel still clamps to fit the viewport. `devicePixelRatio` is displayed as a diagnostic but doesn't need manual division since CSS pixel measurements already reflect Android density. Position drag is clamped to screen bounds.

Normal/Compact selection, panel coordinates, visual theme, square tile size and scale are persisted in UI localStorage. On Android, JNI exposes native module state, Compact mode and opacity from the native C++ registry; native switch states are saved under the mod's config folder.

For **All** with exactly the three starter cards visible, the grid uses three wider sample cards for clarity. When developers register additional modules, the regular six-column grid resumes; Compact uses four columns. When a developer registers a custom category, the sidebar gains a dynamically generated filter button without changing the screen's saved layout.

## Settings

- Normal and Compact mode buttons
- Regular, Opacity and Insider-inspired visual themes
- Opacity slider (40–100%)
- User size slider (70–130%) and automatic fit
- Standard vs square module tiles
- Accent color picker (Regular/Opacity)
- Device viewport and pixel density readout
- Edit GUI drag mode and reset panel position
- Template visibility (hide/show the 3 demo cards only)
- Dynamic sidebar category filters registered by developers
- Module details dialog and accessible keyboard activation

The three themes are independent visual variations, not Badlion Client purchases or membership flags. The Premium tab has been removed. The square-size shortcut next to Search has also been removed; module-tile size is controlled in Settings.

## Safe-area and touch behavior

The page has `viewport-fit=cover`, transparent background, browser touch and pointer support, and automatic orientation/viewport resize handlers. The Android host adds the WebView on the UI thread. When hidden, the Android WebView root is `GONE`, allowing touch input back to the game, while the independent native Levi floating button remains registered.

Test the rendered UI on devices with cutouts, unusual aspect ratios, high display density and accessibility font overrides. No desktop browser simulation can prove perfect handling of every OS status bar or manufacturer-specific window inset.

## Floating button ownership (v1.1.5)

Only the native Levi `registerButton` HUD widget exists. Adjust its size and position in Levi HUD Editor; the Android WebView owns only the module menu panel and never creates another fixed-position launcher. The native widget may not render on early startup screens before Levi initializes its HUD.
