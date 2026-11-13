# Developer API — C++ module cards, callback routing and custom categories

This is a **UI library**, not a collection of premade Minecraft modifications. The only bundled module cards are PvP/Hypixel/Visual **templates**. All mod behavior, verified game signatures, rendering and thread dispatch are supplied by the developer.

## Stable ABI and exported symbols

Include `include/BadlionUI.h` from the source ZIP. The module descriptor keeps the original ABI v1 layout, so old plugins that register properly named modules can still use it. This update adds a separate category descriptor and new functions, without changing existing structure fields:

| Native function | Meaning |
| --- | --- |
| `BadlionUI_SetTitle("MY CLIENT")` | Sets native menu header without changing its condensed typography; 1 on success |
| `BadlionUI_GetTitle(char*,capacity)` | Required size (including NUL), or copies title into a sufficient buffer |
| `BadlionUI_GetAbiVersion()` | Returns `1` (backward compatible) |
| `BadlionUI_RegisterCategory(&descriptor)` | Creates or updates a custom category; returns `1` on success |
| `BadlionUI_UnregisterCategory(id)` | Deletes empty, custom categories only |
| `BadlionUI_RegisterModule(&descriptor)` | Adds/replaces a callback-backed card under a **registered** category ID |
| `BadlionUI_UnregisterModule(id)` | Removes developer module; cannot remove built-in templates |
| `BadlionUI_SetEnabled(id, 0/1)` | Updates state and calls the descriptor callback when changed |
| `BadlionUI_GetEnabled(id)` | `0`, `1` or `-1` if ID unknown |
| `BadlionUI_SetTemplatesVisible(0/1)` | Hides/shows all 3 starter cards in native snapshots |
| `BadlionUI_GetTemplatesVisible()` | Returns native template visibility flag |
| `BadlionUI_SetVisible(0/1)` / `GetVisible()` | Android WebView visibility request/state |
| `BadlionUI_SetCompact(0/1)` / `GetCompact()` | Layout flag |
| `BadlionUI_SetOpacity(40..100)` | Persisted panel opacity |
| `BadlionUI_Snapshot(buffer,capacity)` | JSON with `abi`, `title`, `mode`, `visible`, `opacity`, `templatesVisible`, `categories`, and `modules` |

A `BadlionCategoryDescriptor` is `{const char* id, const char* name, const char* icon, int sort_order}`. Its `id` uses 1–95 ASCII alphanumeric/dot/underscore/hyphen, `All` is reserved, name length 1–63 bytes, optional icon <=31 bytes. Built-in `PvP`, `Hypixel`, `Visual` categories are permanent. Sort order: built-ins 10/20/30; custom values clamped to 40–10000.

A `BadlionModuleDescriptor` is `{id,name,category,icon,description,initially_enabled,on_toggle,userdata}`. Category is its stable **ID**, not a user-facing label. The `on_toggle` callback is `void(const char* moduleId, int enabled, void* userdata)` and is called **outside the library registry mutex**. String memory is valid for the duration of that callback; copy it for later use. Your `userdata` belongs to your mod, and must outlive all callbacks. The UI does not synthesize gameplay behavior.

## Two registration workflows

### Place a card under PvP, Hypixel or Visual

```cpp
static std::atomic<bool> g_enabled{false};
static void onSwitch(const char*,int enabled,void*){
    g_enabled.store(enabled!=0,std::memory_order_release);
}
BadlionModuleDescriptor desc{
   "my_mod.pvp_overlay", "PvP Overlay", "PvP", "⚔",
   "Overlay implemented by my mod", 0, &onSwitch, nullptr
};
// call the resolved BadlionUI_RegisterModule(&desc)
```

**All** will show this module, and **PvP** will show it alongside the PvP template if templates are still visible.

### Create your own custom category

```cpp
BadlionCategoryDescriptor category{"my_mod.hud", "HUD Widgets", "◈", 100};
// registerCategory(&category) before registering modules
BadlionModuleDescriptor desc{
    "my_mod.timer", "Stopwatch", "my_mod.hud", "◷",
    "Timer drawn by my mod", 0, &onSwitch, nullptr
};
// registerModule(&desc)
```

The sidebar now includes a new **HUD Widgets** filter and your card appears under it and All. Categories with no matching modules show an empty-state message. Unregister the module first, then the category. Attempting to remove a category still in use returns `0`; attempting to register a module with an unknown category returns `0`.

## Loading and unloading from a separately compiled mod

The companion source file `examples/DeveloperMod.cpp` includes dynamic linking to the **already loaded** `libbadlionmenuui.so` using `dlopen(..., RTLD_NOLOAD)` and `dlsym` for all four registration functions. Do not embed a second copy of the library inside your mod. Preloader mod ordering is not guaranteed: if the dependency is not loaded yet, retry at a later lifecycle checkpoint rather than busy-looping.

Before your mod unloads, call `BadlionUI_UnregisterModule` for each module, then `BadlionUI_UnregisterCategory` for your custom categories, then release the dynamic library handle. Ensure no concurrently executing callback can outlive the code or userdata it uses; synchronize with your own shutdown lifecycle. Callbacks can originate from Java `@JavascriptInterface` threads, not necessarily the Minecraft render/game thread. Use atomics or a safe task queue and execute gameplay operations only from your own verified game-thread integration.

Do not rely on the starter templates to execute game logic. They are functional button **examples** whose callback logs a diagnostic message. Their IDs are `badlion.template.pvp`, `badlion.template.hypixel`, `badlion.template.visual`. They are optional, user-hideable UI elements. They do not appear as fake gameplay entries.

## Snapshot JSON

```json
{
  "abi": 1,
  "mode": "normal",
  "visible": true,
  "opacity": 95,
  "templatesVisible": true,
  "categories": [
    {"id":"PvP","name":"PvP","icon":"⚔","builtin":true},
    {"id":"Hypixel","name":"Hypixel","icon":"ϟ","builtin":true},
    {"id":"Visual","name":"Visual","icon":"◎","builtin":true},
    {"id":"my_mod.hud","name":"HUD Widgets","icon":"◈","builtin":false}
  ],
  "modules": [
    {"id":"my_mod.timer","name":"Stopwatch","category":"my_mod.hud","icon":"◷",
     "description":"Timer drawn by my mod","enabled":false,"template":false}
  ]
}
```

This JSON example omits other visible template cards for clarity. Exact catalog order is a library detail; use stable IDs. First query `BadlionUI_Snapshot(nullptr,0)` for the required byte count (including NUL), allocate that many bytes, then call it again. The registry can change between calls: handle a second returned required length if the buffer is too small.

The WebView bridge reads the snapshot every refresh cycle. On the UI side, only the **All** aggregation filter is synthetic; all other sidebar buttons use the native `categories` array. There is no hardcoded 49-card game-feature catalog.

## Persistence

Native module flags, compact mode, panel opacity and template visibility are persisted under `config/badlion_ui_state.txt`; the UI also stores profiles, scale, theme and its local template-filter preference in localStorage. A re-registered module ID can inherit a previously saved toggle state. If you require initial callbacks, check the state and initialize your gameplay system accordingly; registration alone does not guarantee an initial toggle event.

## Related guides

- `CUSTOM_CATEGORIES_AND_TEMPLATES.md` — entire category lifecycle and browser demo.
- `WEB_MENU_DEVELOPMENT.md` — HTML/JavaScript-side registration for offline previews.
- `ANDROID_DEX_AUTOSTART.md` — how native state crosses into the WebView.
- `COMPATIBILITY_AND_TESTING.md` — ABI, loader and device validation limits.

## Customize menu header text (added v1.1.4)

The visible header preserves the original condensed `.brand` font (`Impact`, fallback `Arial Narrow`), size and weight; only the **characters** change.

```cpp
using SetTitleFn = int (*)(const char*);
auto setTitle = reinterpret_cast<SetTitleFn>(dlsym(uiHandle, "BadlionUI_SetTitle"));
if (setTitle) setTitle("BATU CLIENT");
```

`BadlionUI_SetTitle` copies a nonempty UTF-8 string of at most 96 bytes and rejects ASCII control characters. It is per-process: call on every mod start. `BadlionUI_GetTitle` returns the required byte count including the terminating NUL, like the snapshot method. A user-specific override chosen under **Settings → Menu Header Text** lives in WebView localStorage and intentionally takes precedence until the user presses **Use Developer Title**. The standalone web API is `BadlionUI.setTitle('BATU CLIENT')`, `BadlionUI.getTitle()` and `BadlionUI.resetTitle()`.

The Premium tab and its markup, logic and style were removed. Only `Mods`, `Settings`, and `Profiles` are present.

## Launcher ownership (v1.1.5)

The launcher button is a registered Levi `pl::modmenu::ButtonInfo` and can be edited in Levi HUD Editor. Do not create a second Android floating WebView button. The square-size shortcut in the top toolbar has been removed, but Settings → Module Square Size still provides Standard/Square tiles. See `docs/FIX_SINGLE_HUD_BUTTON.md`.
