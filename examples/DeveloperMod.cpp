// Developer-facing sample for ANOTHER native mod's lifecycle.
// Do not export PLGetModRegistration from here; integrate these functions
// into the lifecycle of your own separately compiled mod.
#include "BadlionUI.h"
#include <android/log.h>
#include <atomic>
#include <dlfcn.h>

namespace {
std::atomic<bool> g_pvpEnabled{false};
std::atomic<bool> g_counterEnabled{false};
void* g_library=nullptr;
using RegisterCategoryFn=int(*)(const BadlionCategoryDescriptor*);
using UnregisterCategoryFn=int(*)(const char*);
using RegisterModuleFn=int(*)(const BadlionModuleDescriptor*);
using UnregisterModuleFn=int(*)(const char*);
using SetTemplatesVisibleFn=void(*)(int);
RegisterCategoryFn registerCategory=nullptr;
UnregisterCategoryFn unregisterCategory=nullptr;
RegisterModuleFn registerModule=nullptr;
UnregisterModuleFn unregisterModule=nullptr;
SetTemplatesVisibleFn setTemplatesVisible=nullptr;
bool g_categoryRegistered=false;
bool g_pvpRegistered=false;
bool g_hudRegistered=false;

void onPvPToggle(const char* id,int on,void*){
    g_pvpEnabled.store(on!=0,std::memory_order_release);
    __android_log_print(ANDROID_LOG_INFO,"MyMod","%s = %d",id,on);
}
void onCounterToggle(const char* id,int on,void*){
    g_counterEnabled.store(on!=0,std::memory_order_release);
    __android_log_print(ANDROID_LOG_INFO,"MyMod","%s = %d",id,on);
}
} // namespace

bool installBadlionUiModules(){
    // RTLD_NOLOAD: use the UI library already loaded by LeviLaunchroid;
    // it may not be enabled yet, so retry at a safe lifecycle checkpoint.
    g_library=dlopen("libbadlionmenuui.so",RTLD_NOW|RTLD_NOLOAD);
    if(!g_library)return false;
    registerCategory=reinterpret_cast<RegisterCategoryFn>(dlsym(g_library,"BadlionUI_RegisterCategory"));
    unregisterCategory=reinterpret_cast<UnregisterCategoryFn>(dlsym(g_library,"BadlionUI_UnregisterCategory"));
    registerModule=reinterpret_cast<RegisterModuleFn>(dlsym(g_library,"BadlionUI_RegisterModule"));
    unregisterModule=reinterpret_cast<UnregisterModuleFn>(dlsym(g_library,"BadlionUI_UnregisterModule"));
    setTemplatesVisible=reinterpret_cast<SetTemplatesVisibleFn>(dlsym(g_library,"BadlionUI_SetTemplatesVisible"));
    if(!registerCategory||!unregisterCategory||!registerModule||!unregisterModule){
        dlclose(g_library);g_library=nullptr;return false;
    }
    // 1. PvP is already a built-in category. No new category required.
    BadlionModuleDescriptor pvp{
        "my_mod.pvp_indicator", "PvP Indicator", "PvP", "⚔",
        "Developer-owned PvP switch", 0, &onPvPToggle, nullptr
    };
    g_pvpRegistered=registerModule(&pvp)!=0;

    // 2. Register your OWN custom category before registering a card inside it.
    BadlionCategoryDescriptor category{
        "my_mod.widgets", "My HUD Widgets", "◈", 100
    };
    g_categoryRegistered=registerCategory(&category)!=0;
    if(g_categoryRegistered){
        BadlionModuleDescriptor counter{
            "my_mod.counter", "Counter Widget", "my_mod.widgets", "▣",
            "Developer-owned counter toggle", 0, &onCounterToggle, nullptr
        };
        g_hudRegistered=registerModule(&counter)!=0;
    }
    // Optional: hide the three examples so the menu shows only real modules.
    // if(setTemplatesVisible)setTemplatesVisible(0);
    return g_pvpRegistered||g_hudRegistered;
}

// Called from YOUR verified game-thread hook. The callback itself may run
// on a WebView background thread; only read atomics there, never raw game pointers.
void yourVerifiedRenderHook(){
    if(g_pvpEnabled.load(std::memory_order_acquire)){
        // Your PvP UI implementation goes here.
    }
    if(g_counterEnabled.load(std::memory_order_acquire)){
        // Your own safe counter rendering goes here.
    }
}

void removeBadlionUiModules(){
    // Unregister modules BEFORE removing their category and unloading userdata.
    if(unregisterModule){
        if(g_hudRegistered)unregisterModule("my_mod.counter");
        if(g_pvpRegistered)unregisterModule("my_mod.pvp_indicator");
    }
    if(unregisterCategory&&g_categoryRegistered)unregisterCategory("my_mod.widgets");
    g_hudRegistered=g_pvpRegistered=g_categoryRegistered=false;
    registerCategory=nullptr;unregisterCategory=nullptr;
    registerModule=nullptr;unregisterModule=nullptr;setTemplatesVisible=nullptr;
    if(g_library){dlclose(g_library);g_library=nullptr;}
}
