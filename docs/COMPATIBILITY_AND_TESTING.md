# Compatibility, signatures, verification and test matrix

**Main target**: MCBE 1.26.62.1 ARM64 (`libminecraftpe.so` from the supplied dependency archive), LeviLaunchroid 1.6.1 Preloader and Android API 26+. The manifest uses `minecraft_versions: ["1.26.*", "1.26.6*"]`. These are version declarations; they are not equivalent to successful testing on every version.

## Native ELF and reproducible build

The `.so` is compiled using Android NDK r28c's `aarch64-linux-android26-clang++`, linked to the provided `libpreloader.so` and the platform `libc++_shared.so`. Check it with:

```bash
file build/arm64-v8a/libbadlionmenuui.so
llvm-readelf -d build/arm64-v8a/libbadlionmenuui.so
llvm-nm -D --defined-only build/arm64-v8a/libbadlionmenuui.so
```

Expected exported functions include `PLGetModRegistration`, `BadlionUI_RegisterModule`, `BadlionUI_RegisterCategory`, `BadlionUI_UnregisterCategory`, `BadlionUI_SetTemplatesVisible`, `BadlionUI_Snapshot`, and visibility/configuration APIs. There must be exactly one `.so` inside the import package and the manifest `entry` must point to it.

## Hardcoded signatures with wildcards

The supplied Minecraft 1.26.62.1 ELF's Build ID was identified as `c60fa32908019eecae39ebfe7bac5755dc35cde2`. The diagnostic pause-menu wildcard signature is in `src/SignatureProfile.hpp` and can be checked with `tools/check_signature.py`; it has one match in that supplied binary. It is **not** used for a game hook or memory patch by this UI library.

The supplied LeviLaunchroid 1.6.1 `libpreloader.so` has Build ID `b4fb680b6be8da236e5068b7a237f5311c7f4ed8`. The guarded native Activity lookup anchors on the exported `PreloaderInput.nativeSetActivity` function and checks the hardcoded pattern `E1 03 02 AA ?? ?? ?? 14` before decoding `B`, `ADRP x21` and `LDR x1,[x21,#imm]` to recover a **read-only** JNI global Activity reference. No patch, trampoline, manual function execution or unknown game address is used. In the supplied file, the decoded reference lives at relative address `0x1023A0`, but the runtime code *does not hardcode this offset*.

A different preloader sequence is not trusted and is rejected. A fallback through Activity lifecycle callbacks or Android's ActivityThread may work but may also be restricted. Do not disable this safety check merely to force the UI to open.

## Test results and what was not tested

| Check | Result |
|---|---|
| ARM64 ELF build against supplied NDK and Preloader .so | Pass |
| ELF architecture, export table, matching 1 .so package | Pass |
| Generated DEX035 header, SHA-1, Adler32, class metadata and annotation table | Pass (static) |
| Desktop Chromium switches, search, 3 templates, custom category registration, settings | Pass |
| Normal/Compact, 430px and 320px portrait, 2560px desktop fitting | Pass |
| Profiles, template details, simulated native custom-category callback integration | Pass |
| ZIP integrity, manifest keys/versions and embedded resources | Pass |
| On-device Android DEX loading and Activity bootstrap | **Not tested** |
| WebView on the actual Minecraft startup/loading screen | **Not tested** |
| Cross-OEM compatibility and hot-unload after Activity destroy | **Not tested** |

No emulator or Android phone was connected to this build environment. Compilation and browser tests cannot guarantee successful execution inside Minecraft.

## User-facing scope

Only **three** starter buttons are built in (`PvP`, `Hypixel`, `Visual`). Their native callbacks log toggle changes but implement no Bedrock gameplay effects. Developers own real features and can register custom categories through the new exported C ABI. The All filter is synthetic; it displays all visible native modules in all registered categories. Browser and ARM64 compilation tests have been rerun for v1.1.5; this is not a substitute for physical Android runtime validation.

## v1.1.2 visible-world overlay regression

`python3 tests/test_overlay_regression.py` tests the native-show / stale-HTML-hidden state and ensures the panel reappears. Android runtime checks and startup-button controls are documented in `FIX_STARTUP_WORLD_OVERLAY.md`.

## v1.1.3 category and template regression

Run `python3 tests/test_category_templates.py` to verify that All contains all three template cards, each built-in category contains only its own, developer-owned custom categories can be added and removed, and native snapshots dynamically replace the sidebar and cards. Run `python3 tests/test_full_menu.py` for Normal/Compact/portrait previews.

## HUD button regression

Run `python3 tests/test_single_hud_button.py` to check that the native Levi button is still registered and that no independent Android/Web fallback launcher is emitted. This source/browser test cannot verify the HUD editor on a real device.
