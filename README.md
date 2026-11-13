# BadlionMenuUI v1.1.5 — LeviLaunchroid Native UI Library

An independent Bedrock Android module-menu **library** inspired by the supplied Badlion Client menu screenshot. It supplies reusable cards, category filters, C++ callbacks, a Java/DEX WebView bridge and a Levi floating launcher button. It is not affiliated with Badlion.

**This release deliberately removes all 49 hardcoded imitation gameplay modules.** The initial screen has exactly **three functional template switches**, clearly marked `TEMPLATE` and showing the supplied Badlion icon:

| Card ID | Category | What it does |
| --- | --- | --- |
| `badlion.template.pvp` | PvP | A working toggle state and native logging callback; no gameplay effect |
| `badlion.template.hypixel` | Hypixel | A working toggle state and native logging callback; no gameplay effect |
| `badlion.template.visual` | Visual | A working toggle state and native logging callback; no gameplay effect |

The **All** filter shows all three. PvP, Hypixel and Visual each show only their own template. Developers can add any number of modules to these categories or create their own categories (e.g., `my_client.hud`, display name `HUD Widgets`). Custom categories appear as additional sidebar filters. The three starter cards can be hidden in the **Settings → Developer Template Cards** menu, or hidden by the native API `BadlionUI_SetTemplatesVisible(0)`; they are not fixed Minecraft features.

## Downloads and files

The import package includes `manifest.json`, one compiled ARM64 `libbadlionmenuui.so`, `resources/ui/index.html`, `resources/classes.dex`, the Badlion logo, two composited Levi-button images, and all Markdown documentation. The source ZIP contains the editable components:

- `include/BadlionUI.h` — stable C module ABI and custom-category registration API.
- `src/BadlionMenu.cpp` — category and module registries, template callbacks, persistence and Levi button lifecycle.
- `src/AndroidAutoHost.cpp` — Activity/WebView attachment, JavaScript bridge and overlay refresh.
- `web/src/menu.js`, `menu.css`, `index.tpl.html` — editable full menu UI.
- `web/index.html` — generated standalone HTML/CSS/JS (works offline in desktop browsers).
- `examples/DeveloperMod.cpp` — C++ mod integration for a built-in and a custom category.
- `docs/DEVELOPER_API.md`, `docs/WEB_MENU_DEVELOPMENT.md`, `docs/CUSTOM_CATEGORIES_AND_TEMPLATES.md` — tutorials and API reference.

Target defaults: Minecraft Bedrock `1.26.*`, `1.26.6*`; ARM64; LeviLaunchroid/Preloader 1.6.1 and Android API 26+. The supplied Minecraft 1.26.62.1 library and preloader are build/validation inputs. The hardcoded wildcard pattern remains a diagnostic fingerprint and does not alter the game binary.

## Rename the header — no font change

The **BADLION CLIENT** header is configurable. In **Settings → Menu Header Text**, enter a new title. The text updates immediately and is saved locally. **Use Developer Title** removes the local override and restores the C++-provided title (default `BADLION CLIENT`). The header retains the existing Impact/Arial Narrow condensed typography, letter spacing, weight, and logo.

Native developers can call `BadlionUI_SetTitle("MY CLIENT")` after the library loads. Query the effective native title with `BadlionUI_GetTitle(nullptr, 0)` followed by `BadlionUI_GetTitle(buffer, size)`. The title is included in `BadlionUI_Snapshot()` JSON. Browser-only integrations can call `BadlionUI.setTitle('MY CLIENT')` or `BadlionUI.resetTitle()`.

The Premium tab has been removed completely; the toolbar contains only **Mods**, **Settings**, **Profiles**, and **Search** (no three-line square-size shortcut). Square tile sizing remains available in Settings.

A GitHub Pages documentation site is generated in `docs/index.html`; setup instructions are in `docs/GITHUB_PAGES.md`. The repository layout follows the documentation style of WindUIBE while keeping independent content and code.

## Using the menu

Normal Mode uses a dark desktop-style panel; Compact Mode is smaller, and portrait mode automatically fits small screens. Search, switching, module details, profiles, themes, opacity, square sizing, UI scaling, and panel dragging are preserved. The HUD logo button is registered through Levi and is editable in Levi HUD Editor. The HTML WebView can attach during startup, but no additional fixed-position Android floating button is created. The native HUD button may appear only once Levi HUD rendering initializes.

The visible starter templates use real state updates. In an installed native mod, tapping a template invokes `onTemplateToggle`, which logs the state but intentionally does **not** invent a Minecraft mod feature. To implement gameplay behavior, register a `BadlionModuleDescriptor` with your own callback and safely dispatch its work to a verified game thread.

## Essential native API

```cpp
BadlionCategoryDescriptor cat{"my_mod.hud", "HUD Widgets", "◈", 100};
BadlionUI_RegisterCategory(&cat);

BadlionModuleDescriptor widget{
    "my_mod.fps", "FPS Widget", "my_mod.hud", "▣",
    "Draw FPS using my mod's verified render integration", 0,
    &onMyWidgetToggle, nullptr
};
BadlionUI_RegisterModule(&widget);
```

The module's `category` field refers to a **category ID**, not its display name. Register custom categories before modules; unregister modules before unregistering their categories. Existing `BadlionModuleDescriptor` layouts remain ABI-compatible with earlier 1.1.x releases. `BadlionUI_GetAbiVersion()` remains `1`.

## Rebuild and test

```bash
export ANDROID_NDK_HOME=/path/to/android-ndk-r28c
export PRELOADER_SO=/path/to/levilaunchroid-1.6.1/libpreloader.so
bash tools/build.sh
python3 tests/test_web.py
python3 tests/test_full_menu.py
python3 tests/test_category_templates.py
python3 tests/test_overlay_regression.py
python3 tests/test_dex.py
python3 tools/package.py /path/to/output
```

`tools/build.sh` regenerates the complete HTML and classes.dex locally before compiling the ARM64 `.so`. The package builder checks manifest fields, ELF CPU, DEX checksums, ZIP integrity and one-`.so` structure. Build/browser test success is not the same as physical Android gameplay validation; install and startup overlay behavior still depend on the device and matching preloader.

**Documentation:** Start with `docs/QUICK_START.md`, then `docs/CUSTOM_CATEGORIES_AND_TEMPLATES.md` and `docs/DEVELOPER_API.md`. The older startup and ABI repair guides are preserved as historical troubleshooting notes.

## v1.1.5: Single Levi HUD button and clean toolbar

Only the button registered with `pl::modmenu::registerButton()` is created. Its composited normal and pressed Badlion images are preserved, as are native click callbacks and Levi HUD Editor's movement/size control. The duplicate independent Android `WebView` icon from previous releases was removed: it could not be dragged/edited by the HUD Editor and stayed visible on unrelated screens. An HTML `fallbackFloat` control was also removed to avoid duplicating the same launcher.

The `≡` shortcut directly beside Search was removed from normal, compact and portrait layouts. In **Settings → Module Square Size**, developers/users can still select Standard or Square.

**Behavioral trade-off:** no separate floating launcher is drawn before Levi HUD initializes. The full HTML menu's startup-host path is unchanged, but the editable Levi button becomes available when the preloader displays native HUD buttons. Device-specific launch timing must be tested in Minecraft.
