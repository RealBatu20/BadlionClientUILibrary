# Custom categories and genuine module-button templates

## What changed in 1.1.3

The previous interface hardcoded 49 Badlion feature names that were only UI placeholders. Those are **removed from both the native C++ state registry and HTML/JS**. The independent library now ships only three clearly identified reusable template cards. They function as actual switch/callback demonstrations, not promised Minecraft features.

The built-in sidebar IDs are `All` (client-only aggregation), `PvP`, `Hypixel`, and `Visual`. Filter semantics are exact: All shows every visible module, while any named category shows only modules whose `category` ID matches.

A developer can make the card list completely their own: register feature modules via C++, then call `BadlionUI_SetTemplatesVisible(0)` to remove the three samples globally. Users can also hide template cards from the Settings panel; that setting is local to the HTML UI and does not suppress developer-owned cards.

## Native registration ordering

1. Resolve `BadlionUI_RegisterCategory`, `BadlionUI_RegisterModule`, `BadlionUI_UnregisterModule`, and `BadlionUI_UnregisterCategory` from the **same loaded UI `.so`** (using `RTLD_NOLOAD` and `dlsym`).
2. For a built-in category (`PvP`, `Hypixel`, `Visual`), skip category registration. For a custom category, register a stable ASCII ID and visible name first.
3. Register your mod descriptor with `.category` set to the category **ID**. The UI immediately includes it in **All** and in the matching filter after the next bridge refresh.
4. The toggle callback runs on the calling thread. Do not modify Minecraft state from an Android JavaScript interface thread. Instead store atomic config or enqueue work on your verified game render/tick thread.
5. Unregister all modules while your userdata/callback addresses remain valid. Then unregister their custom categories; a category in use cannot be removed.

### A complete example

See `examples/DeveloperMod.cpp` in the source archive. The pattern is:

```cpp
using RegisterCategoryFn=int (*)(const BadlionCategoryDescriptor*);
using RegisterModuleFn=int (*)(const BadlionModuleDescriptor*);
using UnregisterModuleFn=int (*)(const char*);
using UnregisterCategoryFn=int (*)(const char*);

BadlionCategoryDescriptor custom{"sample.widgets", "HUD Widgets", "◈", 100};
// registerCategory(&custom) => 1
BadlionModuleDescriptor fps{"sample.fps", "FPS HUD", "sample.widgets", "▣",
    "Owned by the developer", 0, &onToggle, nullptr};
// registerModule(&fps) => 1
// ... on mod unload: unregisterModule("sample.fps"); unregisterCategory("sample.widgets");
```

Categories are identified by 1–95 ASCII letters, numbers, dot, underscore, hyphen; `All` is reserved. A visible name is required (max 63 UTF-8 bytes), optional icon max 31 bytes, and sort order is clamped to 40–10000 (built-in categories have stable order 10/20/30). Duplicate custom category ID registration updates that category's display metadata; built-ins cannot be changed through this API.

The old module descriptor ABI is preserved: `{id,name,category,icon,description,initially_enabled,on_toggle,userdata}`. Custom category support adds **new exported functions**, not a changed module layout. `BadlionUI_GetAbiVersion()` remains 1 for compatibility.

### Template IDs

| Category | Initial card ID | Status |
| --- | --- | --- |
| PvP | `badlion.template.pvp` | Native example logs toggle |
| Hypixel | `badlion.template.hypixel` | Native example logs toggle |
| Visual | `badlion.template.visual` | Native example logs toggle |

Templates do not require your mod and are not permanent feature entries. `BadlionUI_SetTemplatesVisible(0)` hides all three and persists that preference; `BadlionUI_SetTemplatesVisible(1)` restores them. The HTML **Settings → Developer Template Cards** switch hides/shows only the browser-visible examples. Do not use template IDs as your own production module IDs; use a unique namespaced ID.

### What happens to state after unregistering?

The current saved switch state is associated with the module ID. If a developer unregisters and later registers the same ID, the saved enabled state may be reapplied. Initialize your callback/gameplay hooks safely and synchronize on enable if your mod depends on persisted initial state. The UI does not own callback userdata.

## Standalone JavaScript preview

In `web/index.html`, `window.BadlionUI` has `registerCategory`, `unregisterCategory`, `registerModule`, `unregisterModule`, and `setTemplatesVisible` for a full browser demo:

```js
BadlionUI.registerCategory({id:'client.hud',name:'HUD Tools',icon:'◈'});
BadlionUI.registerModule({
  id:'client.counter',name:'Counter',category:'client.hud',icon:'▣',
  onToggle(enabled, id) { console.log('Toggled',id,enabled); }
});
```

This browser registry is **not a native mod registration API**. When `window.BadlionHost` is present, JavaScript registration methods deliberately return `false`; a real mod registers with the C ABI so that the preloader owns the callback. Native snapshots provide `categories` and `modules` arrays to render sidebar and cards without hardcoded data.

## Quick verification

```bash
python3 web/src/build_web.py
python3 tests/test_category_templates.py
python3 tests/test_full_menu.py
```

Browser tests validate All, PvP, Hypixel, Visual, new custom categories, native snapshot replacement, callback invocation, visibility, and safe category removal. An ARM64 compile validates exported functions, but Android lifecycle/device behavior still requires an on-device test.
