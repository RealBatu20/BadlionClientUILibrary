# Fix: Native Levi floating button missing on Minecraft startup (v1.1.6)

## What the screenshots show

The button appeared inside **Levi HUD Editor** at Minecraft's 56% startup loading bar but vanished when leaving the editor. Its icon, position and HUD-editor integration were working. This was not a failure to register `pl::modmenu::ButtonInfo`.

## Confirmed cause in LeviLaunchroid 1.6.1

The official `InbuiltOverlayManager.tick()` computes `inbuiltVisible = hudEditorMode || (isHudScreenOpen && !isShowingMenu && !isPauseOpen)`. In startup loading, the HUD screen is not active, so `inbuiltVisible` is false. Normally, the external button then gets set to `View.GONE`.

However, Levi also checks `manager.isOverlayShowEverywhere(overlay.getOverlayConfigKey())`. For `ExternalButtonOverlay`, `getOverlayConfigKey()` returns the button's position key: `"external_button:" + buttonId`. BadlionMenuUI's native button ID is `badlionmenuui.launcher`, so the exact key is:

```
external_button:badlionmenuui.launcher
```

## The fix

When Android calls the native host `UiTask.run()` on the Activity UI thread, `src/AndroidAutoHost.cpp` looks up Levi's public in-app Java class `org.levimc.launcher.core.mods.inbuilt.manager.InbuiltModManager`. It calls:

```java
InbuiltModManager manager = InbuiltModManager.getInstance(activity);
String key = "external_button:badlionmenuui.launcher";
if (!manager.isOverlayShowEverywhere(key)) {
    manager.setOverlayShowEverywhere(key, true);
    InbuiltOverlayManager overlays = InbuiltOverlayManager.getInstance();
    if (overlays != null) overlays.refreshRuntimeVisibility();
}
```

The actual implementation uses JNI rather than Java source compilation; the calls above are their equivalent. The setting is persistent in Levi's SharedPreferences and refreshed after changes. It is restricted to the Badlion launcher button.

There is still exactly **one** button. Levi owns its visual rendering, touch callbacks, stored position, saved size, and HUD Editor controls; there is **no** separate Android/WebView floating button. Normal/Compact menu modes and all developer APIs are unchanged.

## Caveats and diagnostics

- This uses the verified **LeviLaunchroid v1.6.1** Java methods and does not patch the game or launcher binary. Future launchers may rename them; JNI logs will then say where the lookup failed.
- If Levi has not yet created the button overlay, no mod can display that native view immediately. This fix makes it eligible to show as soon as Levi creates it.
- Levi's global **Hide launcher HUD** setting can still hide the button. Turn it off.
- In Logcat, filter `BadlionUI.Host`. Look for `Enabled startup visibility for Levi HUD button via Show Everywhere` or `Levi HUD button remains HUD-editor-managed and set to Show Everywhere`.
- To test: install v1.1.6, restart launcher, watch the Minecraft loading screen with HUD Editor closed, then open HUD Editor, move the icon, save, exit the editor and restart. Its position should persist and there must be only one Badlion button.
- If the button is still absent, collect Logcat lines containing `BadlionUI.Host`, `BadlionMenuUI`, and `InbuiltOverlayManager`. Do not infer UI visibility merely from successful button registration.

**Verification:** ARM64 compilation and source-level validation completed. No real Android device test was possible in this environment; runtime behavior depends on the installed LeviLaunchroid implementation.
