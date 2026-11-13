# Startup and world-overlay repair — historical v1.1.2 notes

**Latest update (v1.1.5):** The independent Android floating WebView described below was **removed**. Only the native Levi HUD button remains. See [`FIX_SINGLE_HUD_BUTTON.md`](FIX_SINGLE_HUD_BUTTON.md) for the current implementation. The reattachment and visible-menu WebView repairs remain in use. The instructions referring to the earlier Android fallback are historical, not applicable to v1.1.5.

## Symptoms reproduced

In the provided 31.6-second Android recording, the Badlion icon is visible in-game and its clicks are logged as `menu shown` / `menu hidden`, but no Badlion menu is drawn. The earlier Logcat screenshot confirms `Full HTML WebView attached to Activity`, `module registration succeeded`, and `floating button registration succeeded`.

This means native button registration succeeds. It **does not** establish that the WebView is still attached to the current decor view, above the game renderer, or that its HTML panel has switched out of the `is-hidden` CSS state.

## Fixes applied

1. **Immediate UI-thread wake:** after a native Levi button click or Android fallback click, schedule the UI task using `Activity.runOnUiThread()`, instead of relying solely on a delayed callback posted to a possibly replaced decor view.
2. **Parent recovery:** each UI tick obtains the Activity's **current** `getWindow().getDecorView()`. If game loading detached either WebView, the host removes it from its old parent if necessary and attaches it to the current decor.
3. **Raise layer:** while visible, the host calls `bringToFront()`, applies an elevation, and invalidates the root. This addresses reorder of sibling views when the game's view hierarchy is rebuilt.
4. **Native/HTML synchronization:** WebView receives `BadlionUI.hostVisibility(true)` and `BadlionUI.refresh()` on every visible tick. It cannot remain stuck at `body.is-hidden` after a native button restores visibility.
5. **Actual Levi button templates:** the assets `badlion_button_normal.png` and `badlion_button_pressed.png` composite the user-supplied `template_bfr.png` / `template_aft.png` with the Badlion logo. The same normal icon is used in the preloader's registered button and the Android fallback.
6. **Startup float:** an independent 64dp transparent WebView containing the template icon is attached on the Android UI thread. It remains accessible while the full-screen HTML menu is `GONE`, without making a full-screen view intercept Minecraft touches.
7. **No duplicate after first Levi click:** once the real Levi HUD button reports an event, the Android startup fallback is hidden for the rest of that Activity session.

## How to test on device

- Remove older installed BadlionMenuUI and import the current **v1.1.5** package in LeviLaunchroid.
- During initial startup/loading, confirm that both the complete HTML menu and the small Levi-template Badlion icon are visible.
- Close the menu, open it from the startup icon, and check that both transitions work.
- Enter a singleplayer world; tap the native Levi floating button to open and close the menu twice.
- Search for `BadlionUI.Host` in Logcat. New diagnostics include `Menu visible=1` and `Reattached full HTML menu overlay after game view transition` when reparenting was necessary.
- Confirm that movement, camera, and HUD inputs remain responsive when the panel is closed.

## Compatibility/limitations

The fix is compiled for Android ARM64 and the supplied LeviLaunchroid 1.6.1 Preloader. It has not been tested on the user's device. Some game renderers use Android subwindows or `SurfaceView` z-order modes that can outrank standard decor children; if that happens, collect the new Logcat and a recording. A WebView attached to an Android Activity cannot be guaranteed to appear before that Activity is created, so extremely early splash/loading stages remain dependent on the launcher lifecycle.

The signature in `src/SignatureProfile.hpp` remains a **diagnostic** wildcard fingerprint, not an installed hook or a guarantee of compatibility with unknown Minecraft builds.

## v1.1.3 note

The overlay bootstrap, floating logo template assets and device-view recovery remain unchanged by the new category registry. The **three category cards** inside the HTML menu are different controls from the **one floating launcher icon**; adding custom categories does not add extra floating launcher buttons. New categories and modules arrive via `BadlionUI_Snapshot` and render in the already attached WebView.
