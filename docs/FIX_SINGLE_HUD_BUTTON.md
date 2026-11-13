# v1.1.5 — Single editable Levi HUD floating button

## What was wrong

The prior release created **two** distinct floating buttons with the same Badlion image:

1. `pl::modmenu::registerButton()` in `src/BadlionMenu.cpp` created a native Levi HUD button, which the Levi HUD Editor could move and resize (the white-arrow button in the screenshot).
2. `floatingButtonHtml()` + `g_floatWeb` in `src/AndroidAutoHost.cpp` created an independent 64dp Android WebView overlay at a fixed top-left margin. That view stayed on top of Levi's HUD Editor and was not editable there (the red-arrow button).

The HTML's own `#fallbackFloat` control was a **third** potential button when browsing the standalone menu.

## Fix

- **Keep** `pl::modmenu::registerModule()` and `pl::modmenu::registerButton()`, with the same icon template PNGs and native `ButtonEvent::Click` callback. This is the only floating launcher produced by the mod.
- **Remove** `g_floatWeb`, `floatingButtonHtml`, icon base64 generation, extra Android WebView creation, `ensureParented` and `bringToFront` handling for that second overlay.
- **Remove** `#fallbackFloat` from the HTML, CSS and JS. Only the native Levi button opens the installed menu, while developers may invoke the public native visibility API.
- **Remove** the `≡` button next to Search (and its JavaScript handler). The existing square tile size choices remain in **Settings → Module Square Size**.

## Verification checklist on Android

1. Uninstall/disable v1.1.4 before importing v1.1.5. Restart LeviLaunchroid. Do not leave duplicate versions with the same mod name.
2. Join a world, open **Levi HUD Editor**, select the Badlion icon, move it and change its size. Save the HUD layout.
3. Confirm only **one** Badlion floating button exists and no second icon overlaps the HUD Editor or other screens.
4. Exit the editor, tap the native Badlion button to open/hide the full menu.
5. Confirm the toolbar shows only **Mods**, **Settings**, **Profiles**, and **Search**, with no `≡` button.
6. In Settings, test **Standard** and **Square** module tile sizes.

The native Levi HUD button may not appear during startup until the HUD renderer has initialized. The startup menu WebView may still attach during startup. To get a button that is **editable in Levi HUD Editor**, do not create a separate Android overlay as a substitute.

## Extending the library

No C ABI changes were made. `BadlionUI_SetVisible(1)` and `BadlionUI_SetVisible(0)` still control the menu, and `BadlionUI_RegisterModule`, custom categories, profile state, and all title APIs remain available.
