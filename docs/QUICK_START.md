# Quick start — BadlionMenuUI 1.1.5

## 1. Install and launch

Requirements: Minecraft Bedrock Android ARM64 with manifest versions `1.26.*` / `1.26.6*`, compatible LeviLaunchroid 1.6.1 preloader, Android 8.0/API 26+ and working Android System WebView.

1. Remove the older `BadlionMenuUI` package (do not run two versions with duplicate native registrations).
2. Import `libBadlionMenuUI_1.1.5.levipack` in LeviLaunchroid's native-mod importer.
3. Enable the mod and restart the game. Use the single Levi HUD Editor-controlled Badlion icon to show or hide the full menu. The independent Android startup floating button has been removed; Levi controls when its native HUD button becomes visible.
4. Open **Mods → All**. Only three starter template cards should appear. They are **not** real Bedrock gameplay features.

## 2. Filter behavior

| Filter | Cards when no developer mods are registered |
| --- | --- |
| All | PvP Button Template, Hypixel Button Template, Visual Button Template |
| PvP | PvP Button Template |
| Hypixel | Hypixel Button Template |
| Visual | Visual Button Template |

Each template switch changes state and fires a small demo callback; it does not perform cheat or gameplay operations. Tap a card to see its ID and template status. Disable the sample cards in Settings → **Developer Template Cards** when adding real modules. Other Normal/Compact, profile, theme, position, resolution and search features remain available.

## 3. Add a module to an existing category

In your separately loaded C++ mod, dynamically resolve `BadlionUI_RegisterModule` from the installed UI library (see `examples/DeveloperMod.cpp`):

```cpp
void onToggle(const char* id, int enabled, void* userData) {
    // Save state, then dispatch verified gameplay operations on the game thread.
}
BadlionModuleDescriptor mod{
    "my_mod.pvp_item", "My PvP Module", "PvP", "⚔",
    "Feature provided by my own mod", 0, &onToggle, nullptr
};
// registerModule(&mod) using dlsym-resolved BadlionUI_RegisterModule
```

The new module appears in All and PvP. The same applies to `Hypixel` and `Visual`.

## 4. Add a custom category and its button

```cpp
BadlionCategoryDescriptor category{"my_mod.utilities", "Utilities", "✦", 100};
registerCategory(&category);          // resolve the native function first
BadlionModuleDescriptor mod{
    "my_mod.clock", "Real Clock Widget", "my_mod.utilities", "◷",
    "Developer-owned example", 0, &onToggle, nullptr
};
registerModule(&mod);
```

Now All and Utilities both show your module; PvP/Hypixel/Visual remain unchanged. Registration fails if the specified custom category has not been registered. Call `unregisterModule("my_mod.clock")` **before** `unregisterCategory("my_mod.utilities")`. No Minecraft feature is created automatically.

## 5. Rebuild

```bash
export ANDROID_NDK_HOME=/absolute/path/android-ndk-r28c
export PRELOADER_SO=/absolute/path/libpreloader.so
bash tools/build.sh
python3 tests/test_category_templates.py
python3 tests/test_full_menu.py
python3 tools/package.py /tmp/badlion-output
```

The full tutorial for callback threading, custom categories, and dynamic linking is in `docs/DEVELOPER_API.md`, with browser examples in `docs/WEB_MENU_DEVELOPMENT.md`.

## 6. Diagnose Android issues

`adb logcat -s BadlionMenuUI BadlionUI.Host Preloader` will show native enable and WebView activity. A successful ARM64 compile and browser preview do not guarantee that a particular Android Activity will accept the overlay at every screen transition. See `docs/FIX_STARTUP_WORLD_OVERLAY.md` for visibility debugging.
