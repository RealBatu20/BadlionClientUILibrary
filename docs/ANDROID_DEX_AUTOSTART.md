# Android startup overlay, in-memory DEX and JNI integration

This file documents the **built-in automatic attempt** added in 1.1.1, which replaces the earlier Java-source-only integration. It requires no edit to the Minecraft APK. The fallback Java integration method remains available separately.

## Contents

| Component | Purpose |
|---|---|
| `src/AndroidAutoHost.cpp` | Creates JavaVM attachment, dynamic DEX loader, lifecycle callback registration and Android UI-thread WebView |
| `tools/dex/generate_minimal_dex.py` | Builds `dex/classes.dex` (DEX version 035) with a deterministic local Python generator |
| `dex/classes.dex` | Three executable Java classes with methods bound to C++ using `JNIEnv::RegisterNatives` |
| `web/index.html` | Bundled UI loaded into WebView with base URL `https://badlion.local/` |
| `src/BadlionMenu.cpp` | Preloader lifecycle and persistent logo floating button |

## Full initialization order

1. **`modLoad`**: initializes default modules, resolves the extracted folder from `dladdr`, restores persistent native config, then invokes `Badlion_AutoHostStart`.
2. **Java VM discovery**: calls `JNI_GetCreatedJavaVMs` from the Android runtime and attaches only if needed.
3. **Activity lookup**: on the supplied preloader, locates exported `Java_org_levimc_launcher_preloader_PreloaderInput_nativeSetActivity`. The hardcoded ARM64 pattern is `E1 03 02 AA ?? ?? ?? 14`. A bounds-checked decoder follows its branch, finds `ADRP x21` plus `LDR x1,[x21,#offset]`, and reads the previously registered global Activity reference. **This never modifies the preloader's code or its reference.**
4. **Get Application**: calls public `Activity.getApplication()`; if no preloader Activity exists yet, tries `ActivityThread.currentApplication()`. This fallback may be blocked on some OS versions.
5. **Load DEX**: reads `resources/classes.dex` (small, local, header-checked) and constructs `InMemoryDexClassLoader(ByteBuffer, parentClassLoader)` on API 26+.
6. **Connect JNI**: loads `HostApi`, `UiTask` and `ActivityObserver`; registers `HostApi.snapshot`, `setEnabled`, `setVisible`, `setMode`, `setOpacity`, `UiTask.run` and the lifecycle callbacks.
7. **Lifecycle**: registers `Application.registerActivityLifecycleCallbacks` and calls `Activity.runOnUiThread(UiTask)` if an Activity exists. Otherwise it can discover an Activity on creation/resume.
8. **WebView**: UI-thread task builds a transparent `FrameLayout` in the Activity decor, configures WebView with JavaScript and DOM storage, disables file/content access and multiple windows, adds annotated `BadlionHost` and loads HTML via `loadDataWithBaseURL()`.
9. **Sync**: the same UI task posts itself every 350ms. It keeps native visibility and module snapshots synchronized. The Preloader floating icon stays separately registered by `modEnable`.

## DEX generator details

All three DEX classes are real class definitions:

- `org.therealbatu.badlion.UiTask implements Runnable` with a native `run()` method. UI tasks execute on Android's main thread.
- `org.therealbatu.badlion.HostApi` with five public native methods carrying runtime-visible `android.webkit.JavascriptInterface` annotations (needed on Android API 17+).
- `org.therealbatu.badlion.ActivityObserver implements Application.ActivityLifecycleCallbacks` with seven native lifecycle methods.

Each class has a tiny actual constructor (`invoke-direct Object.<init>; return-void`). The generator emits DEX headers, string/type/prototype/method IDs, code items, annotation sets, class data, map list, SHA-1 and Adler32 checksum. It uses only Python's standard library. **The DEX is not a stub filename; it is built and embedded in the `.levipack`.**

```bash
python3 tools/dex/generate_minimal_dex.py
file dex/classes.dex   # 'Dalvik dex file version 035' on hosts with DEX-aware file magic
```

## Java source alternative

A separate readable implementation, `android/app/src/main/java/org/therealbatu/badlion/BadlionMenuBridge.java`, provides `install(Activity, File, String)` for app hosts that already own and can edit their Activity. This separate Java file requires Android SDK `android.jar` + D8 to build and is **not** used by the automatic DEX path. See `android/README.md` and `docs/ANDROID_DEX_STARTUP.md` for this optional adapter.

## Troubleshooting

Inspect `adb logcat -s BadlionUI.Host BadlionMenuUI` while launching the game. Expected events include `In-memory DEX, JavaScript bridge, Activity observer registered` and `Full HTML WebView attached to Activity`. If no UI appears, check the preloader build ID, mod folder resource paths, `libpreloader.so` version and Android WebView availability. An incompatible Activity lookup will not be force-patched.

Typical issues:

- **Floating logo shows but HTML does not**: native mod/menu registered, but DEX load, Activity lookup or WebView attach failed; inspect `BadlionUI.Host` logs.
- **HTML opens but toggles do nothing in Minecraft**: these are demo entries. Register an actual developer callback and implement the game behavior in your mod.
- **Old Android version**: this version requires API 26+, because it uses `InMemoryDexClassLoader`.
- **Different preloader build**: signature decoding is deliberately guarded and will fall back to lifecycle/reflection if available.
- **Game loading screen**: the overlay is attempted as soon as native loading and Android Activity access permit. Neither preloader HUD render timing nor WebView initialization latency can be guaranteed without on-device instrumentation.

## Security and crash prevention

All HTML is shipped locally. File/content access and WebView multiple windows are disabled. The JS bridge is exposed only to the bundled, known UI document; do not add remote navigation. The decoder checks instruction bitmasks and displacement bounds before reading the preloader's global Activity JNI reference. The game's 1.26.62.1 ELF is **not patched or hooked** by this UI library.

A WebView is created/destroyed only by tasks on the Android UI thread. Preloader disable/unload attempts to remove lifecycle callbacks and scheduled UI jobs. Hot-unloading during Activity destruction must still be tested on actual hardware.

## v1.1.3 custom categories and templates

The WebView JNI class layout and generated DEX **do not need to change** for this update. `HostApi.snapshot()` forwards the expanded JSON catalog: `templatesVisible`, `categories`, and `modules`. The three starter cards are functional native toggle examples with `template:true`; developer-owned cards have `template:false`. Users can hide starter cards from the HTML Settings page, while C++ developers can hide them in native snapshots using `BadlionUI_SetTemplatesVisible(0)`. The dynamic sidebar is implemented entirely in `web/src/menu.js`, so no hardcoded custom-category names are required in Java/DEX.

## v1.1.6 — one HUD-controlled launcher

The Android host attaches **only the full HTML menu**. It does not attach a small floating button WebView. The single launcher is a native Levi ModMenu `registerButton` widget, so HUD Editor can move/resize it. Startup HTML menu visibility is independent of when Levi draws its HUD controls.
