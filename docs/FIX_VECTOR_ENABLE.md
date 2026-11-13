# Fix for `Unhandled exception while enabling mod ...: vector` (original v1.1.1 repair)

## Reported device log

The native loader successfully loaded `libbadlionmenuui.so`, then reported:

```text
[BadlionUI.Host] No JavaVM yet; retry during enable
[Preloader] Unhandled exception while enabling mod BadlionMenuUI: vector
[Preloader] Failed to enable C++ lifecycle mod BadlionMenuUI
```

## Root cause and corrective work

The v1.1.0 `sdk_compat/PreloaderModMenu.hpp` omitted the runtime's
`ModuleInfo::onKeybind` callback and `ButtonInfo::activeIconData` vector.
The layout of the C++ objects therefore did not match the Levi 1.6.1
preloader. STL containers/callbacks may be interpreted at incorrect offsets,
leading to a `vector` exception or native memory corruption. The v1.1.1
compatibility definitions now include both fields in SDK order; it is
recompiled from the source, not binary-patched.

Second, `No JavaVM yet` was caused by obtaining the VM with an optional
`JNI_GetCreatedJavaVMs` lookup. The preloader actually provides `JavaVM*`
through its standard C++ lifecycle `pl::mod::ModContext`. The patched
mod copies this pointer into the auto-WebView host before initialization.
The icon click also retries the host initialization if it was too early.

Registration now logs success/failure at each stage and a refused ModMenu
registration no longer marks the whole library as failed.

## Reinstall and check

1. Remove the previously installed **BadlionMenuUI** mod from LeviLaunchroid
   (do not enable two copies).
2. Import the current **v1.1.5** `.levipack` and enable it.
3. Filter Logcat for `BadlionMenuUI`, `BadlionUI.Host`, and `Preloader`.
4. Look for `module registration succeeded` then
   `floating button registration succeeded`.
5. The WebView host should then log either successful attachment or a
   specific Android JNI failure.

**Verification scope:** ARM64 compile and static ABI checks can be
performed offline; device WebView startup and mod-menu compatibility
still require actual Android runtime testing. Do not treat passing an
ELF/build check as an on-device success report.

SDK reference: https://github.com/LiteLDev/preloader-android/blob/main/include/pl/ModMenu.hpp

## v1.1.3 note

The corrected Levi ModuleInfo and ButtonInfo ABI declarations remain unchanged. v1.1.3 modifies only the module data registry, adds a separate `BadlionCategoryDescriptor` C API and removes 49 fake cards. The old `BadlionModuleDescriptor` layout remains binary-compatible with v1.1.x.
