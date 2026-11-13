# Customize the topbar title

BadlionMenuUI defaults to **BADLION CLIENT** in its title bar. Developers can change the label without changing the original condensed font: the CSS continues to use `Impact, 'Arial Narrow', sans-serif` at the same weight, size, and spacing. The `BADLION CLIENT` logo image and floating launcher buttons are not replaced by this option.

## In the HTML menu

Open **Settings → Menu Header Text** and enter your new title. The text changes immediately and the WebView stores this override locally. Press **Use Developer Title** to clear it. This is not a different theme, and does not add a new tab.

In an offline browser preview, or from other approved JavaScript modules within the same page:

```js
BadlionUI.setTitle('BATU CLIENT');
console.log(BadlionUI.getTitle());
BadlionUI.resetTitle();
```

These methods change only the `textContent` of `#brandTitle`, never raw `innerHTML`. Names are trimmed, control characters are removed and display text is escaped in Settings. Overly long titles display an ellipsis rather than overlapping the close button.

## In a native C++ mod

The native library exports a C ABI function that other mods can resolve with `dlsym`:

```cpp
using SetTitle = int (*)(const char* title);
auto change = reinterpret_cast<SetTitle>(dlsym(uiHandle, "BadlionUI_SetTitle"));
if (change) change("BATU CLIENT");
```

`BadlionUI_SetTitle` returns 1 if a nonempty UTF-8 title up to 96 bytes is accepted. It returns 0 for invalid data. The title is copied; callers can free their temporary string. Call on each mod start. The native snapshot now includes a `title` string. `BadlionUI_GetTitle(char*,size_t)` can copy it out: pass `nullptr,0` first to query the needed byte count including NUL.

When the user has set a local WebView title override, that override takes precedence over the developer's native title. The user can reset to the native value without restarting. No C++ ABI descriptor layouts were changed; ABI remains v1.

## Premium removed

The toolbar now has exactly three tabs: **Mods**, **Settings**, and **Profiles**. The Premium tab, its HTML element, JavaScript page, and CSS styling have been deleted.
