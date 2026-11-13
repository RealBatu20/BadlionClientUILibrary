# v1.1.6 artwork hotfix — remove Levi's extra square backdrop

The Badlion launcher uses the two supplied template compositions:

- `assets/badlion_button_normal.png`: original Levi *before* (`levi_template_bfr.png`) plus Badlion logo.
- `assets/badlion_button_pressed.png`: original Levi *after* (`levi_template_aft.png`) plus Badlion logo.

Both PNGs already include the pixel-art square frame. **Do not add another Android square behind them.** On LeviLaunchroid v1.6.1, `ExternalButtonOverlay.createButtonBackground()` adds an independent `GradientDrawable` background and stroke to every C++ registered button. Its default `Keycap` styling produces the broad gray outer square visible in the screenshot even when the PNG itself looks correct.

## Native fix

In `src/BadlionMenu.cpp`, when registering the one `pl::modmenu::ButtonInfo`:

```cpp
button.stylePreset = pl::modmenu::ButtonStylePreset::Keycap;
button.normalBgColor = 0x01000000u;
button.activeBgColor = 0x01000000u;
button.borderColor = 0x01000000u;
button.iconFormat = pl::modmenu::ButtonIconFormat::Png;
button.iconData = readBytes(modDir / "resources" / "badlion_button_normal.png");
button.activeIconData = readBytes(modDir / "resources" / "badlion_button_pressed.png");
```

The alpha channel is **1/255**, so the wrapper fill and border are visually imperceptible. This is intentional: Levi uses an `override != 0` check, so `0x00000000` would wrongly fall back to its opaque default. The PNG artwork itself is not altered.

This keeps the genuine Levi on-screen control (one button), its click callback, native HUD Editor drag/resize, and the v1.1.6 `Show Everywhere` loading-screen setting. No second fixed-position or WebView floating button is created. Startup availability still depends on Levi initializing its native button overlay.

## Confirm on Android

1. Remove or disable the old 1.1.6 build, install the updated `.levipack`, and restart LeviLaunchroid.
2. Inspect the Badlion launcher icon on the loading screen. It should show **just the normal template PNG**, without the larger plain gray background panel.
3. Tap it to open/close the HTML menu; it must still work.
4. Open Levi HUD Editor, drag/resize the **same** button, save, and verify its position persists.
5. Confirm the pressed state uses `badlion_button_pressed.png` and that there is only one Badlion button.

This patch was compiled and checked against Levi's v1.6.1 Java source. It has not been confirmed on a physical Android device in this environment.
