# Optional readable Java WebView host adapter

`app/src/main/java/org/therealbatu/badlion/BadlionMenuBridge.java` is an alternative standalone Activity integration example. It is useful when maintaining a launcher or an Android app that **already owns** the game Activity, and you prefer Java code over the built-in JNI-created WebView.

**The current v1.1.5 `.levipack` already bundles a working-format, auto-generated DEX** (`resources/classes.dex`) containing three small Java classes with native methods and runtime-visible `@JavascriptInterface` annotations. It also bundles `AndroidAutoHost.cpp`, which attempts to attach the WebView automatically. You do not need to compile this longer Java source for the default install.

For manual use, add the Java source to an Android application project, compile with a real Android SDK, then invoke `BadlionMenuBridge.install(Activity, File, String)` on the Activity UI thread with the installed mod folder and absolute `.so` path. Call `dispose()` on Activity destruction. `tools/build_dex.sh` provides an optional D8 conversion step for a launcher-specific dynamic loader. Do not install the manual adapter and automatic overlay in parallel.

The automatic bootstrap is tested at build/static-code level only. On-device startup visibility and unsupported OEM Activity behavior need device verification. Details in `docs/ANDROID_DEX_AUTOSTART.md`.

The v1.1.5 module registry adds dynamic categories and a three-template starter catalog; the `BadlionHost.snapshot()` JavaScript bridge method is unchanged but returns an expanded JSON object. Do not add duplicate 49-module lists in Java or Android XML. Register your own modules and custom category IDs through the C ABI (see `docs/DEVELOPER_API.md`).
