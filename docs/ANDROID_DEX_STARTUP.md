# Optional manually hosted Java WebView integration

The current v1.1.5 distribution includes an executable `resources/classes.dex` and an automatic JNI bootstrap in `src/AndroidAutoHost.cpp`; **you do not need this guide for the default installation attempt**. This document is only for Android launcher maintainers who want explicit ownership of the Activity and WebView lifecycle.

## Use the standalone Java source

Copy `android/app/src/main/java/org/therealbatu/badlion/BadlionMenuBridge.java` into an Android app or library module that owns the Minecraft Activity. Build it using Android Studio and an Android SDK platform (API >= 26; recommended compile SDK >= 28). If you require a standalone DEX/JAR for a custom classloader, set `ANDROID_JAR` and `D8` and execute `tools/build_dex.sh` (this optional script was not run in this environment).

Inside the actual Minecraft Activity, **on the UI thread**, call:

```java
BadlionMenuBridge overlay = BadlionMenuBridge.install(
    this,
    new java.io.File("/absolute/installed/BadlionMenuUI"),
    "/absolute/installed/BadlionMenuUI/libbadlionmenuui.so"
);
```

Pass the actual extracted mod directory and `.so` path. Store the adapter reference and call `overlay.dispose()` inside `onDestroy`. The JNI bridge loads local HTML and uses `nativeSnapshot`, `nativeSetEnabled`, `nativeSetVisible`, `nativeSetMode` and `nativeSetOpacity` symbols from the same `.so`. Avoid having both automatic and manual overlays active simultaneously; use one integration path per launcher process.

## Difference from the built-in DEX path

The automatic path uses a small generated `HostApi` and `ActivityObserver` DEX driven by native code, with no external Java host. The manual path uses the longer readable Java source, and the launcher must compile and initialize it itself. The automatic path is an attempt, not a guarantee across every Android version and custom launcher.

See `ANDROID_DEX_AUTOSTART.md` for automatic bootstrap details and `WEB_MENU_DEVELOPMENT.md` for HTML customization.

## Module category data

The DEX Java bridge requires no new native methods for custom categories. The existing `nativeSnapshot()`/`BadlionHost.snapshot()` JSON now includes `categories` with each `id`, `name`, `icon`, and `builtin` flag. Native mod developers call the exported `BadlionUI_RegisterCategory` and `BadlionUI_RegisterModule` C functions; the WebView automatically refreshes its category buttons. The manual Java bridge should pass the native snapshot through without trying to reconstruct a hardcoded module list.
