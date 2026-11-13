#include "BadlionUI.h"
#include "SignatureProfile.hpp"
#include "PreloaderModMenu.hpp"
#include <jni.h>

void Badlion_AutoHostStart(const std::string& folder);
void Badlion_AutoHostSetJavaVM(JavaVM* vm);
void Badlion_AutoHostStop();
void Badlion_AutoHostRequestRefresh();
#include <jni.h>
#include <android/log.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"BadlionMenuUI",__VA_ARGS__)
namespace {
struct Module {
 std::string id,name,category,icon,description;
 bool enabled=false;
 BadlionToggleCallback callback=nullptr;
 void* userdata=nullptr;
 bool isTemplate=false;
};
struct Category {
 std::string id,name,icon;
 int sortOrder=100;
 bool builtin=false;
};
std::mutex g_lock;
std::map<std::string,Module> g_modules;
std::map<std::string,Category> g_categories;
std::map<std::string,bool> g_savedStates;
std::string g_title="BADLION CLIENT";
std::atomic<bool> g_visible{true};
std::atomic<bool> g_compact{false};
std::atomic<bool> g_templatesVisible{true};
std::atomic<int> g_opacity{95};
bool g_registered=false;
std::filesystem::path g_modDir;
constexpr std::string_view kModuleId="badlionmenuui.root";
constexpr std::string_view kButtonId="badlionmenuui.launcher";

bool validId(std::string_view s){return !s.empty()&&s.size()<96&&std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.';});}
bool validText(const char* value,size_t maxBytes){return value&&*value&&std::strlen(value)<=maxBytes;}
std::string jsonString(const std::string& v){std::string s="\"";for(unsigned char c:v){switch(c){case '"':s+="\\\"";break;case '\\':s+="\\\\";break;case '\n':s+="\\n";break;case '\r':s+="\\r";break;case '\t':s+="\\t";break;default:if(c<32){char buf[8];std::snprintf(buf,sizeof(buf),"\\u%04X",int(c));s+=buf;}else s+=char(c);}}return s+'"';}
void onTemplateToggle(const char* id,int enabled,void*) {
 LOG("template button callback: %s = %d (no Minecraft gameplay feature)", id,enabled);
}
void initCatalog(){
 std::lock_guard lk(g_lock);
 if(!g_categories.empty())return;
 for(const auto& category : {Category{"PvP","PvP","⚔",10,true},Category{"Hypixel","Hypixel","ϟ",20,true},Category{"Visual","Visual","◎",30,true}})
  g_categories.emplace(category.id,category);
 // Each actual template toggle invokes a native callback. No fake game modules.
 for(const auto& spec : {
  Module{"badlion.template.pvp","PvP Button Template","PvP","⚔","Replace this example with your PvP mod: register a C++ toggle callback for your own module ID.",false,onTemplateToggle,nullptr,true},
  Module{"badlion.template.hypixel","Hypixel Button Template","Hypixel","ϟ","Replace this example with your Hypixel mod: register a C++ toggle callback for your own module ID.",false,onTemplateToggle,nullptr,true},
  Module{"badlion.template.visual","Visual Button Template","Visual","◎","Replace this example with your visual mod: register a C++ toggle callback for your own module ID.",false,onTemplateToggle,nullptr,true}
 })g_modules.emplace(spec.id,spec);
}
std::filesystem::path statePath(){return g_modDir/"config"/"badlion_ui_state.txt";}
void saveState(){if(g_modDir.empty())return;std::error_code ec;std::filesystem::create_directories(g_modDir/"config",ec);if(ec)return;std::ostringstream text;{std::lock_guard lk(g_lock);for(const auto&[id,m]:g_modules)g_savedStates[id]=m.enabled;for(const auto&[id,on]:g_savedStates)text<<id<<'='<<(on?1:0)<<'\n';}text<<"$compact="<<(g_compact?1:0)<<"\n$opacity="<<g_opacity<<"\n$templates="<<(g_templatesVisible?1:0)<<"\n";auto target=statePath();auto tmp=target;tmp+=".tmp";{std::ofstream stream(tmp,std::ios::binary|std::ios::trunc);if(!stream)return;stream<<text.str();stream.flush();if(!stream)return;}std::filesystem::rename(tmp,target,ec);if(ec)std::filesystem::remove(tmp,ec);}
void loadState(){std::ifstream f(statePath());if(!f)return;std::lock_guard lk(g_lock);std::string line;while(std::getline(f,line)){auto p=line.find('=');if(p==std::string::npos)continue;auto key=line.substr(0,p);auto val=line.substr(p+1);if(key=="$compact")g_compact=(val=="1");else if(key=="$templates")g_templatesVisible=(val=="1");else if(key=="$opacity"){{char* end=nullptr;long n=std::strtol(val.c_str(),&end,10);if(end&&*end==0)g_opacity=std::clamp(int(n),40,100);}}else if(validId(key)){g_savedStates[key]=(val=="1");if(auto it=g_modules.find(key);it!=g_modules.end())it->second.enabled=(val=="1");}}}
std::filesystem::path resolveModDirectory(){Dl_info info{};if(!dladdr(reinterpret_cast<void*>(&BadlionUI_GetAbiVersion),&info)||!info.dli_fname)return {};return std::filesystem::path(info.dli_fname).parent_path();}
std::vector<unsigned char> readBytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)return {};return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
std::string snapshot(){
 std::ostringstream out;
 out<<"{\"abi\":1,\"title\":";
 {std::lock_guard lk(g_lock);out<<jsonString(g_title);}
 out<<",\"mode\":\""<<(g_compact?"compact":"normal")<<"\",\"visible\":"<<(g_visible?"true":"false")<<",\"opacity\":"<<g_opacity<<",\"templatesVisible\":"<<(g_templatesVisible?"true":"false")<<",\"categories\":[";
 std::lock_guard lk(g_lock);
 std::vector<Category> cats;
 for(const auto& [id,c] : g_categories)cats.push_back(c);
 std::sort(cats.begin(),cats.end(),[](const Category& a,const Category& b){if(a.sortOrder!=b.sortOrder)return a.sortOrder<b.sortOrder;return a.id<b.id;});
 bool first=true;
 for(const auto& c:cats){if(!first)out<<',';first=false;out<<"{\"id\":"<<jsonString(c.id)<<",\"name\":"<<jsonString(c.name)<<",\"icon\":"<<jsonString(c.icon)<<",\"builtin\":"<<(c.builtin?"true":"false")<<'}';}
 out<<"],\"modules\":[";first=true;
 for(const auto& [id,m]:g_modules){
  if(m.isTemplate&&!g_templatesVisible.load())continue;
  if(!first)out<<',';first=false;
  out<<"{\"id\":"<<jsonString(m.id)<<",\"name\":"<<jsonString(m.name)<<",\"category\":"<<jsonString(m.category)<<",\"icon\":"<<jsonString(m.icon)<<",\"description\":"<<jsonString(m.description)<<",\"enabled\":"<<(m.enabled?"true":"false")<<",\"template\":"<<(m.isTemplate?"true":"false")<<'}';
 }
 out<<"]}";
 return out.str();
}
void buttonClicked(std::string_view,pl::modmenu::ButtonEvent event,float){
 if(event==pl::modmenu::ButtonEvent::Click||event==pl::modmenu::ButtonEvent::StateChanged){
  bool next=!g_visible.load();g_visible.store(next);
  // Retry WebView initialization on a real user click if startup was too early.
  if(next && !g_modDir.empty())Badlion_AutoHostStart(g_modDir.string());
  Badlion_AutoHostRequestRefresh();
  LOG("floating button: menu %s",next?"shown":"hidden");
 }
}
// Only a diagnostic fingerprint. Never scan, hook, or patch an unknown game here.
[[maybe_unused]] constexpr auto kSignature=&badlion::profile::kPauseMenuOpenIdaPattern;
}
extern "C" __attribute__((visibility("default"))) const char* BadlionUI_KnownSignature(){return badlion::profile::kPauseMenuOpenIdaPattern;}
extern "C" int BadlionUI_GetAbiVersion(){return BADLION_UI_ABI_VERSION;}
extern "C" int BadlionUI_SetTitle(const char* title){
 if(!validText(title,96))return 0;
 for(const unsigned char* p=reinterpret_cast<const unsigned char*>(title);*p;++p){
  if(*p<0x20||*p==0x7f)return 0;
 }
 {std::lock_guard lk(g_lock);g_title=title;}
 Badlion_AutoHostRequestRefresh();return 1;
}
extern "C" size_t BadlionUI_GetTitle(char* output,size_t capacity){
 std::lock_guard lk(g_lock);
 const size_t n=g_title.size()+1;
 if(output&&capacity>=n)std::memcpy(output,g_title.c_str(),n);
 return n;
}
extern "C" int BadlionUI_RegisterCategory(const BadlionCategoryDescriptor* d){
 if(!d||!d->id||!validId(d->id)||std::strcmp(d->id,"All")==0||!validText(d->name,63)||
   (d->icon&&std::strlen(d->icon)>31))return 0;
 initCatalog();
 Category c{d->id,d->name,d->icon?d->icon:"◇",std::clamp(d->sort_order,40,10000),false};
 {std::lock_guard lk(g_lock);
  const auto it=g_categories.find(c.id);
  if(it!=g_categories.end()&&it->second.builtin)return 0;
  g_categories.insert_or_assign(c.id,std::move(c));
 }
 Badlion_AutoHostRequestRefresh();
 return 1;
}
extern "C" int BadlionUI_UnregisterCategory(const char* id){
 if(!id)return 0;
 initCatalog();
 {std::lock_guard lk(g_lock);
  auto it=g_categories.find(id);
  if(it==g_categories.end()||it->second.builtin)return 0;
  for(const auto& [mid,m]:g_modules)if(m.category==id)return 0;
  g_categories.erase(it);
 }
 Badlion_AutoHostRequestRefresh();
 return 1;
}
extern "C" int BadlionUI_RegisterModule(const BadlionModuleDescriptor* d){
 if(!d||!d->id||!d->name||!validId(d->id)||!validText(d->name,127)||
    (d->icon&&std::strlen(d->icon)>31)||
    (d->description&&std::strlen(d->description)>2048))return 0;
 initCatalog();
 Module m;m.id=d->id;m.name=d->name;m.category=d->category&&*d->category?d->category:"Visual";
 m.icon=d->icon?d->icon:"◇";m.description=d->description?d->description:"";
 m.enabled=d->initially_enabled!=0;m.callback=d->on_toggle;m.userdata=d->userdata;m.isTemplate=false;
 {std::lock_guard lk(g_lock);
  if(g_categories.find(m.category)==g_categories.end())return 0;
  if(auto saved=g_savedStates.find(m.id);saved!=g_savedStates.end())m.enabled=saved->second;
  g_modules.insert_or_assign(m.id,std::move(m));
 }
 saveState();Badlion_AutoHostRequestRefresh();
 return 1;
}
extern "C" int BadlionUI_UnregisterModule(const char* id){
 if(!id)return 0;
 {std::lock_guard lk(g_lock);
  auto it=g_modules.find(id);if(it==g_modules.end()||it->second.isTemplate)return 0;
  g_modules.erase(it);
 }
 saveState();Badlion_AutoHostRequestRefresh();
 return 1;
}
extern "C" void BadlionUI_SetTemplatesVisible(int visible){initCatalog();g_templatesVisible.store(visible!=0);saveState();Badlion_AutoHostRequestRefresh();}
extern "C" int BadlionUI_GetTemplatesVisible(){return int(g_templatesVisible.load());}
extern "C" int BadlionUI_SetEnabled(const char* id,int enabled){if(!id)return 0;BadlionToggleCallback cb=nullptr;void* userdata=nullptr;std::string owned;bool next=enabled!=0;{std::lock_guard lk(g_lock);auto it=g_modules.find(id);if(it==g_modules.end())return 0;if(it->second.enabled==next)return 1;it->second.enabled=next;g_savedStates[it->first]=next;cb=it->second.callback;userdata=it->second.userdata;owned=it->second.id;}saveState();if(cb)cb(owned.c_str(),next?1:0,userdata);return 1;}
extern "C" int BadlionUI_GetEnabled(const char* id){if(!id)return -1;std::lock_guard lk(g_lock);auto it=g_modules.find(id);return it==g_modules.end()?-1:int(it->second.enabled);}
extern "C" void BadlionUI_SetVisible(int visible){g_visible.store(visible!=0);Badlion_AutoHostRequestRefresh();}
extern "C" int BadlionUI_GetVisible(){return int(g_visible.load());}
extern "C" void BadlionUI_SetCompact(int compact){g_compact.store(compact!=0);saveState();}
extern "C" int BadlionUI_GetCompact(){return int(g_compact.load());}
extern "C" void BadlionUI_SetOpacity(int opacity){g_opacity.store(std::clamp(opacity,40,100));saveState();}
extern "C" size_t BadlionUI_Snapshot(char* output,size_t capacity){std::string json=snapshot();const auto n=json.size()+1;if(output&&capacity>=n)std::memcpy(output,json.c_str(),n);return n;}
namespace {
// ABI note: the official pl::mod::ModContext layout begins with JavaVM*.
// The preloader passes ModContext& as the second argument (pointer in AArch64 x1).
// This minimal lifecycle shim uses the first field instead of pulling the
// SDK's fmt-heavy Mod.hpp into this standalone build.
void captureLifecycleVm(void* context){
 if(context){auto vm=*reinterpret_cast<JavaVM* const*>(context);
  if(vm)Badlion_AutoHostSetJavaVM(vm);
 }
}
bool modLoad(void*,void* context){captureLifecycleVm(context);initCatalog();g_modDir=resolveModDirectory();loadState();Badlion_AutoHostStart(g_modDir.string());LOG("native library loaded; mod folder=%s",g_modDir.c_str());return true;}
bool modEnable(void*,void* context){
 captureLifecycleVm(context);
 LOG("enable begin; using corrected Levi ModMenu ABI");
 pl::modmenu::ModuleInfo module;module.moduleId=std::string(kModuleId);module.displayName="Badlion Client UI Library";module.description="Floating button and native module callback registry";module.modId=g_modDir.filename().string();module.defaultEnabled=true;module.hideInHudEditor=true;
 LOG("registering module %s",module.moduleId.c_str());
 if(!pl::modmenu::registerModule(module)){
  LOG("registerModule returned false; starting HTML host without external module");
  Badlion_AutoHostStart(g_modDir.string());
  return true; // Avoid disabling the native UI library for optional menu failure.
 }
 LOG("module registration succeeded");
 pl::modmenu::ButtonInfo button;button.buttonId=std::string(kButtonId);button.moduleId=std::string(kModuleId);button.displayName="Badlion Client Menu";button.modId=g_modDir.filename().string();button.label="Badlion";button.androidKeyCode=0;button.behavior=pl::modmenu::ButtonBehavior::Click;button.defaultVisible=true;button.stylePreset=pl::modmenu::ButtonStylePreset::Keycap;button.widthScale=1.0f;button.heightScale=1.0f;button.iconFormat=pl::modmenu::ButtonIconFormat::Png;button.hideLabelWhenIconPresent=true;
 button.iconData=readBytes(g_modDir/"resources"/"badlion_button_normal.png");
 button.activeIconData=readBytes(g_modDir/"resources"/"badlion_button_pressed.png");
 button.onEvent=buttonClicked;
 if(button.iconData.empty())LOG("Logo icon not found; using fallback label");
 LOG("registering floating button, icon bytes=%zu",button.iconData.size());
 if(!pl::modmenu::registerButton(button)){
  LOG("registerButton returned false; HTML host remains enabled");
  pl::modmenu::unregisterModule(kModuleId);
  Badlion_AutoHostStart(g_modDir.string());
  return true;
 }
 LOG("floating button registration succeeded");
 g_registered=true;Badlion_AutoHostStart(g_modDir.string());LOG("Levi HUD button registered; automatic WebView host attempted");return true;
 }
bool modDisable(void*,void*){Badlion_AutoHostStop();if(g_registered){pl::modmenu::unregisterButton(kButtonId);pl::modmenu::unregisterModule(kModuleId);g_registered=false;}saveState();return true;}
bool modUnload(void*,void*){modDisable(nullptr,nullptr);return true;}
struct ModRegistration{void* instance;bool(*load)(void*,void*);bool(*enable)(void*,void*);bool(*disable)(void*,void*);bool(*unload)(void*,void*);};
int modInstance=0;
ModRegistration registration{&modInstance,modLoad,modEnable,modDisable,modUnload};
}
extern "C" __attribute__((visibility("default"))) ModRegistration* PLGetModRegistration(){return &registration;}

extern "C" JNIEXPORT jstring JNICALL Java_org_therealbatu_badlion_BadlionMenuBridge_nativeSnapshot(JNIEnv* env,jclass){const std::string s=snapshot();return env->NewStringUTF(s.c_str());}
extern "C" JNIEXPORT jboolean JNICALL Java_org_therealbatu_badlion_BadlionMenuBridge_nativeSetEnabled(JNIEnv* env,jclass,jstring id,jboolean value){if(!id)return JNI_FALSE;auto raw=env->GetStringUTFChars(id,nullptr);int ok=BadlionUI_SetEnabled(raw,value==JNI_TRUE);env->ReleaseStringUTFChars(id,raw);return ok?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT void JNICALL Java_org_therealbatu_badlion_BadlionMenuBridge_nativeSetVisible(JNIEnv*,jclass,jboolean v){BadlionUI_SetVisible(v==JNI_TRUE);}
extern "C" JNIEXPORT void JNICALL Java_org_therealbatu_badlion_BadlionMenuBridge_nativeSetMode(JNIEnv* env,jclass,jstring value){if(!value)return;const char* raw=env->GetStringUTFChars(value,nullptr);BadlionUI_SetCompact(std::strcmp(raw,"compact")==0);env->ReleaseStringUTFChars(value,raw);}
extern "C" JNIEXPORT void JNICALL Java_org_therealbatu_badlion_BadlionMenuBridge_nativeSetOpacity(JNIEnv*,jclass,jint value){BadlionUI_SetOpacity(value);}
