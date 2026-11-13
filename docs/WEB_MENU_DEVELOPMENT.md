# Full HTML/CSS/JavaScript menu — template and category development

Edit `web/src/menu.js`, `web/src/menu.css`, or `web/src/index.tpl.html`, **not** the generated HTML output directly. Regenerate with `python3 web/src/build_web.py` and open `web/index.html` in a browser for an offline preview. The generated page inlines all CSS, JS and the provided Badlion logo; the same HTML is packaged in `resources/ui/index.html` in `.levipack`.

## Catalog and category rendering

The source now starts with **exactly three** `DEFAULTS` (PvP/Hypixel/Visual button templates). The built-in category list has IDs `PvP`, `Hypixel`, and `Visual`. `All` is an aggregate filter created by the sidebar renderer. There are no hardcoded Armor Status, CPS, Bedwars, etc. cards. Clicking template switches invokes a real logging callback on the native build; browser preview produces a developer console message. No Minecraft gameplay effects are present.

`renderFilters()` renders `#categoryFilters` dynamically from `state.categories`. Native snapshot `categories` and `modules` replace local browser data when `window.BadlionHost` exists. Each registered module's `.category` is compared exactly to the selected category ID. Search additionally searches name, ID, description and category. `renderModules()` creates module cards and the `TEMPLATE` label only when `module.template===true`.

## Browser-side public API

In standalone mode (without `window.BadlionHost`), the JavaScript API is useful for previewing a mod's custom menu design:

```js
BadlionUI.registerCategory({
  id: 'my_mod.utilities',
  name: 'Utilities',
  icon: '◇'
});
BadlionUI.registerModule({
  id: 'my_mod.clock', name: 'Clock', category: 'my_mod.utilities', icon: '◷',
  description: 'A button with a browser callback', enabled: false,
  onToggle(enabled, id) { console.log(id, enabled); }
});
BadlionUI.getCategories();
BadlionUI.setTemplatesVisible(false); // hide starter cards, keep developer cards
BadlionUI.setMode('compact');
// When finished: unregisterModule first, then unregisterCategory.
BadlionUI.unregisterModule('my_mod.clock');
BadlionUI.unregisterCategory('my_mod.utilities');
```

`registerCategory` fails on duplicate IDs, invalid IDs or reserved `All`; `registerModule` fails on unknown categories or duplicate IDs. `unregisterCategory` fails while it still has modules. Browser callbacks execute synchronously with successful toggles; if the callback throws, the state change is rejected. Built-in category and template records are not removable through these JavaScript methods. The Settings tab can hide all three templates.

**Important:** On Android with a live `window.BadlionHost`, these JavaScript registration methods intentionally return false. Developers must register through the exported native C API from their own mod so callbacks can safely be tracked by the preloader. `BadlionHost.snapshot()` provides serialized data; `BadlionHost.setEnabled()` invokes `BadlionUI_SetEnabled()` via JNI, which fires your registered native callback.

## Existing UI features preserved

- Normal layout: 820 × 460 CSS design pixels, with a six-column grid for developer modules; when only three templates are present in All, they are displayed as a three-column example row.
- Compact layout: 570 × 475 with denser cards; portrait mode: 380 × 690 reference design with two columns and automatic scaling.
- Themes Regular, Opacity, Insider; theme accent and opacity controls.
- Profile save/load/delete; each profile stores per-module IDs and enabled states and applies toggles through real callbacks when available.
- Edit GUI / drag header; bounded offsets, UI Size slider and auto-fit based on `visualViewport`/`innerWidth`/`innerHeight`.
- Optional module-detail dialog; search; square sizes through Settings. There is no browser-only floating logo because only the native Levi HUD launcher should appear on-device.

The native Levi floating button is created by `src/BadlionMenu.cpp`; `src/AndroidAutoHost.cpp` owns only the full HTML menu WebView, not a separate floating button. It cannot create a duplicate uneditable button.

## Native state vs. local storage

The UI persists appearance, profiles, mode and local `templatesVisible` in `localStorage` (`badlion-ui-v2` key for upgrade continuity). Native snapshot also supplies `templatesVisible`. A template card is displayed only if **both** the native flag and the local UI setting allow it. The developer can hide the three templates globally through `BadlionUI_SetTemplatesVisible(0)`. UI users can hide them in Settings without impacting native callback registration or custom modules.

## Test the result

```bash
python3 web/src/build_web.py
python3 tests/test_web.py
python3 tests/test_full_menu.py
python3 tests/test_category_templates.py
python3 tests/test_overlay_regression.py
```

`test_category_templates.py` tests all four built-in filters, custom categories, callback invocations, category removal constraints, native snapshot updates and suppression of the three examples. Tests run in desktop Chromium; actual WebView and device behavior must still be validated on Android.

## Customize the topbar title (v1.1.5)

The markup is `<span class="brand" id="brandTitle">BADLION CLIENT</span>`. `menu.js` updates its `textContent` (not `innerHTML`), avoiding markup injection. Change it from Settings → Menu Header Text or call `BadlionUI.setTitle('MY UI LIBRARY')`. The Reset button returns to the developer's native `BadlionUI_SetTitle()` value. Keep the `.header .brand` CSS rules intact to preserve the condensed Impact / Arial Narrow style. An ellipsis prevents overly long titles from obscuring the close button.

There is no Premium UI: the toolbar contains only Mods, Settings, and Profiles.

## v1.1.5 launcher and topbar

`#fallbackFloat` was removed from the standalone HTML and is no longer created by the Android host; installed in-game menu reopening relies on the native Levi HUD button. `#squares` was removed from the topbar and from the JS event wiring; use Settings → Module Square Size for square mode.
