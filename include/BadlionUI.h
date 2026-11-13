#ifndef BADLION_UI_PUBLIC_H
#define BADLION_UI_PUBLIC_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Backward-compatible C ABI: v1 module descriptor is unchanged. */
#define BADLION_UI_ABI_VERSION 1
#if defined(__GNUC__)
#define BADLION_API __attribute__((visibility("default")))
#else
#define BADLION_API
#endif
/* Invoked outside the registry lock, on the caller's thread. Dispatch game operations
 * to your verified game thread; never touch an unloaded callback or userdata. */
typedef void (*BadlionToggleCallback)(const char* module_id, int enabled, void* userdata);

/* Built-in category IDs: "PvP", "Hypixel", "Visual". "All" is a UI-only filter.
 * For a custom category, call BadlionUI_RegisterCategory BEFORE RegisterModule.
 * Strings are copied into the library; order sorts custom categories numerically. */
typedef struct BadlionCategoryDescriptor {
    const char* id;           /* 1-95 ASCII [A-Za-z0-9._-], not "All" */
    const char* name;         /* visible title, UTF-8, 1-63 bytes */
    const char* icon;         /* optional icon/Unicode symbol, <=31 bytes */
    int sort_order;           /* 100..10000 recommended, after built-ins */
} BadlionCategoryDescriptor;

typedef struct BadlionModuleDescriptor {
    const char* id;           /* unique ID, 1-95 ASCII [A-Za-z0-9._-] */
    const char* name;         /* visible label */
    const char* category;     /* built-in ID or registered custom category ID */
    const char* icon;         /* optional, e.g. "⚙" */
    const char* description;
    int initially_enabled;
    BadlionToggleCallback on_toggle;
    void* userdata;
} BadlionModuleDescriptor;

BADLION_API int BadlionUI_GetAbiVersion(void);
/* Set an application-specific title (UTF-8 up to 96 bytes). Keeps the same
 * visual font and panel header style. Returns 1 on success, 0 otherwise.
 * Set the title after BadlionMenuUI is loaded; repeat on startup. */
BADLION_API int BadlionUI_SetTitle(const char* title);
/* Returns required bytes including NUL. If capacity is insufficient, writes
 * nothing. A WebView-local Settings override takes visual precedence. */
BADLION_API size_t BadlionUI_GetTitle(char* output, size_t capacity);
BADLION_API int BadlionUI_RegisterCategory(const BadlionCategoryDescriptor* descriptor);
/* Returns 0 when built-in, unknown, or a module still uses the category. */
BADLION_API int BadlionUI_UnregisterCategory(const char* id);
BADLION_API int BadlionUI_RegisterModule(const BadlionModuleDescriptor* descriptor);
BADLION_API int BadlionUI_UnregisterModule(const char* id);
BADLION_API int BadlionUI_SetEnabled(const char* id, int enabled);
BADLION_API int BadlionUI_GetEnabled(const char* id);
/* Remove the three starter cards without affecting developer-owned modules. */
BADLION_API void BadlionUI_SetTemplatesVisible(int visible);
BADLION_API int BadlionUI_GetTemplatesVisible(void);
BADLION_API void BadlionUI_SetVisible(int visible);
BADLION_API int BadlionUI_GetVisible(void);
BADLION_API void BadlionUI_SetCompact(int compact);
BADLION_API int BadlionUI_GetCompact(void);
BADLION_API void BadlionUI_SetOpacity(int percent);
/* JSON keys: abi, title, mode, visible, opacity, templatesVisible, categories, modules.
 * Each module has id/name/category/icon/description/enabled/template.
 * Return bytes needed INCLUDING NUL; if capacity is insufficient, do not copy. */
BADLION_API size_t BadlionUI_Snapshot(char* output, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
