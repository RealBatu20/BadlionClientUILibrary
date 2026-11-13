// Optional self-installing Android WebView host: loads the tiny bundled JNI DEX
// and schedules work onto the game's Activity UI thread without patching Minecraft.
// Supported: Android API 26+ with accessible ActivityThread on the target build.
// Fallback: Application.ActivityLifecycleCallbacks on subsequent lifecycle events.
#include "BadlionUI.h"
#include <jni.h>
#include <android/log.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#define LOGA(...) __android_log_print(ANDROID_LOG_INFO, "BadlionUI.Host", __VA_ARGS__)
namespace {
JavaVM* g_vm = nullptr;
std::recursive_mutex g_lock;
bool g_enabled = false;
bool g_dexReady = false;
bool g_observerRegistered = false;
std::string g_modDir;
// JNI global refs, accessed under the lock. UI widgets are mutated only on UI thread.
jobject g_app = nullptr, g_observer = nullptr, g_task = nullptr, g_activity = nullptr;
jobject g_root = nullptr, g_web = nullptr, g_decor = nullptr;
bool g_lastVisible = false;
bool g_loggedLeviShowEverywhere = false;

bool check(JNIEnv* e, const char* where) {
 if (!e->ExceptionCheck()) return true;
 LOGA("JNI exception at %s",where);e->ExceptionDescribe();e->ExceptionClear();return false;
}
void release(JNIEnv* e, jobject& r){if(r){e->DeleteGlobalRef(r);r=nullptr;}}
void replace(JNIEnv* e, jobject& old, jobject fresh){release(e,old);if(fresh)old=e->NewGlobalRef(fresh);}
struct EnvScope {
 JNIEnv* e=nullptr;bool attached=false;
 explicit EnvScope(JavaVM* vm) {
  if(!vm)return;
  if(vm->GetEnv((void**)&e,JNI_VERSION_1_6)!=JNI_OK){
   if(vm->AttachCurrentThread(&e,nullptr)==JNI_OK)attached=true;
  }
 }
 ~EnvScope(){if(attached&&g_vm)g_vm->DetachCurrentThread();}
};
JavaVM* findVM(){
 if(g_vm)return g_vm;
 auto f=reinterpret_cast<jint(*)(JavaVM**,jsize,jsize*)>(dlsym(RTLD_DEFAULT,"JNI_GetCreatedJavaVMs"));
 if(!f){auto art=dlopen("libart.so",RTLD_NOW|RTLD_NOLOAD);if(art)f=reinterpret_cast<jint(*)(JavaVM**,jsize,jsize*)>(dlsym(art,"JNI_GetCreatedJavaVMs"));}
 JavaVM* vm=nullptr;jsize count=0;
 if(f&&f(&vm,1,&count)==JNI_OK&&count>0)g_vm=vm;
 return g_vm;
}
std::string readHtml(){
 std::ifstream f(g_modDir+"/resources/ui/index.html",std::ios::binary);
 if(!f)return {};
 std::string s((std::istreambuf_iterator<char>(f)),{});
 if(s.size()>1024*1024)return {};
 return s;
}
std::vector<unsigned char> readDex(){
 std::ifstream f(g_modDir+"/resources/classes.dex",std::ios::binary);
 if(!f)return {};
 std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});
 if(bytes.size()>512*1024||bytes.size()<112)return {};
 if(std::memcmp(bytes.data(),"dex\n035\0",8)!=0)return {};
 return bytes;
}
// Returns object local reference or nullptr. No Java Activity API is patched.
// ARM64 Preloader 1.6.1 activity reference, read-only. Verify the exported JNI
// function's instruction sequence before decoding the ADRP/LDR slot. The
// recorded wildcard pattern is: E1 03 02 AA ?? ?? ?? 14 (nativeSetActivity).
// No hooking, memory patching, or blind fixed-address dereference is used.
jobject preloaderActivity(JNIEnv* e){
 auto symbol=dlsym(RTLD_DEFAULT,"Java_org_levimc_launcher_preloader_PreloaderInput_nativeSetActivity");
 if(!symbol)return nullptr;
 auto words=reinterpret_cast<const uint32_t*>(symbol);
 if(words[0]!=0xaa0203e1u||(words[1]&0xfc000000u)!=0x14000000u)return nullptr;
 int64_t disp=int32_t(words[1]<<6)>>6; // signed 26-bit branch displacement in words
 uintptr_t dest=reinterpret_cast<uintptr_t>(words+1)+disp*4;
 if(dest<reinterpret_cast<uintptr_t>(words)||dest>reinterpret_cast<uintptr_t>(words)+0x10000)return nullptr;
 auto target=reinterpret_cast<const uint32_t*>(dest);
 // Locate ADRP x21, followed shortly by LDR x1,[x21,#offset].
 for(int i=0;i<28;++i){
  uint32_t adrp=target[i];
  if((adrp&0x9f00001fu)!=0x90000015u)continue;
  for(int j=i+1;j<std::min(i+6,28);++j){
   uint32_t ldr=target[j];
   if((ldr&0xffc003ffu)!=0xf94002a1u)continue;
   int64_t imm21=int64_t(((adrp>>5)&0x7ffffu)<<2 | (adrp>>29&3));
   if(imm21&(1<<20))imm21-=1<<21;
   uintptr_t pc=reinterpret_cast<uintptr_t>(target+i);
   uintptr_t page=(pc&~uintptr_t(4095))+(imm21<<12);
   uintptr_t slot=page+(((ldr>>10)&0xfffu)*8);
   // The instruction pair must point inside the same known preloader image
   // near its writable state, not arbitrary executable or unmapped memory.
   if(slot<reinterpret_cast<uintptr_t>(words)||slot>reinterpret_cast<uintptr_t>(words)+0x200000)return nullptr;
   auto ref=__atomic_load_n(reinterpret_cast<jobject*>(slot),__ATOMIC_ACQUIRE);
   if(ref){auto local=e->NewLocalRef(ref);if(check(e,"preloader Activity ref"))return local;}
   return nullptr;
  }
 }
 return nullptr;
}
jobject currentApplication(JNIEnv* e){
 jclass t=e->FindClass("android/app/ActivityThread");
 if(!check(e,"ActivityThread")||!t)return nullptr;
 jmethodID cur=e->GetStaticMethodID(t,"currentApplication","()Landroid/app/Application;");
 if(!check(e,"currentApplication")||!cur)return nullptr;
 jobject app=e->CallStaticObjectMethod(t,cur);check(e,"Application");return app;
}
jobject getActivity(JNIEnv* e){
 if(auto fromPreloader=preloaderActivity(e))return fromPreloader;
 // Android SDK has no public 'current Activity' accessor. This optional path is
 // best-effort and subject to OEM hidden-API restrictions.
 jclass t=e->FindClass("android/app/ActivityThread");
 if(!check(e,"ActivityThread lookup")||!t)return nullptr;
 jmethodID current=e->GetStaticMethodID(t,"currentActivityThread","()Landroid/app/ActivityThread;");
 if(!check(e,"currentActivityThread")||!current)return nullptr;
 jobject thread=e->CallStaticObjectMethod(t,current);
 if(!check(e,"thread")||!thread)return nullptr;
 jfieldID field=e->GetFieldID(t,"mActivities","Landroid/util/ArrayMap;");
 if(!check(e,"mActivities")||!field)return nullptr;
 jobject map=e->GetObjectField(thread,field);
 if(!check(e,"activityMap")||!map)return nullptr;
 jclass mapClass=e->FindClass("java/util/Map");
 jmethodID vals=e->GetMethodID(mapClass,"values","()Ljava/util/Collection;");
 jobject values=e->CallObjectMethod(map,vals);
 if(!check(e,"values")||!values)return nullptr;
 jclass col=e->FindClass("java/util/Collection");
 jmethodID iterator=e->GetMethodID(col,"iterator","()Ljava/util/Iterator;");
 jobject it=e->CallObjectMethod(values,iterator);
 if(!check(e,"iterator")||!it)return nullptr;
 jclass itr=e->FindClass("java/util/Iterator");
 jmethodID has=e->GetMethodID(itr,"hasNext","()Z");
 jmethodID next=e->GetMethodID(itr,"next","()Ljava/lang/Object;");
 while(e->CallBooleanMethod(it,has)){
  jobject record=e->CallObjectMethod(it,next);
  if(!check(e,"activity record")||!record)break;
  jclass rec=e->GetObjectClass(record);
  jfieldID af=e->GetFieldID(rec,"activity","Landroid/app/Activity;");
  if(!check(e,"record.activity")||!af)break;
  jobject a=e->GetObjectField(record,af);
  if(a&&!e->IsSameObject(a,nullptr))return a;
 }
 return nullptr;
}
void schedule(JNIEnv* e){
 if(!g_enabled||!g_task||!g_activity)return;
 jclass act=e->FindClass("android/app/Activity");
 jmethodID run=e->GetMethodID(act,"runOnUiThread","(Ljava/lang/Runnable;)V");
 if(run)e->CallVoidMethod(g_activity,run,g_task);
 check(e,"schedule on UI");
}
void showWidget(JNIEnv* e,jobject view,bool shown){
 if(!view)return;
 jclass v=e->FindClass("android/view/View");
 jmethodID setter=e->GetMethodID(v,"setVisibility","(I)V");
 e->CallVoidMethod(view,setter,shown?0:8);
}
void detachWidget(JNIEnv* e){
 if(g_decor&&g_task){
  jclass view=e->FindClass("android/view/View");
  jmethodID remove=e->GetMethodID(view,"removeCallbacks","(Ljava/lang/Runnable;)Z");
  if(remove)e->CallBooleanMethod(g_decor,remove,g_task);
 }
 if(g_root&&g_decor){
  jclass parent=e->FindClass("android/view/ViewGroup");
  jmethodID remove=e->GetMethodID(parent,"removeView","(Landroid/view/View;)V");
  if(remove)e->CallVoidMethod(g_decor,remove,g_root);
 }
 if(g_web){
  jclass webCls=e->FindClass("android/webkit/WebView");
  jmethodID destroy=e->GetMethodID(webCls,"destroy","()V");
  if(destroy)e->CallVoidMethod(g_web,destroy);
 }
 check(e,"detach widgets");
 release(e,g_root);release(e,g_web);release(e,g_decor);
 g_lastVisible=false;
}
// There is intentionally no Android-owned floating button. The only launcher
// control is pl::modmenu::registerButton() and is therefore editable in the
// Levi HUD Editor. Creating a second WebView would bypass HUD positioning.
// Repeatable check: Minecraft may replace its content views at the world-loading
// transition without destroying the Activity. A non-null g_root is not proof
// the overlay is still parented or above the renderer.
jobject activityDecor(JNIEnv* e){
 jclass a=e->FindClass("android/app/Activity");
 jmethodID getWindow=e->GetMethodID(a,"getWindow","()Landroid/view/Window;");
 jobject win=e->CallObjectMethod(g_activity,getWindow);
 if(!check(e,"get current window")||!win)return nullptr;
 jclass w=e->FindClass("android/view/Window");
 jmethodID getDecor=e->GetMethodID(w,"getDecorView","()Landroid/view/View;");
 jobject decor=e->CallObjectMethod(win,getDecor);
 if(!check(e,"get current decor"))return nullptr;
 return decor;
}
bool ensureParented(JNIEnv* e,jobject view,jobject decor){
 if(!view||!decor)return false;
 jclass v=e->FindClass("android/view/View");
 jmethodID parentGetter=e->GetMethodID(v,"getParent","()Landroid/view/ViewParent;");
 jobject parent=e->CallObjectMethod(view,parentGetter);
 if(!check(e,"get overlay parent"))return false;
 if(parent&&e->IsSameObject(parent,decor))return true;
 if(parent){
  jclass grp=e->FindClass("android/view/ViewGroup");
  if(e->IsInstanceOf(parent,grp)){
   jmethodID remove=e->GetMethodID(grp,"removeView","(Landroid/view/View;)V");
   e->CallVoidMethod(parent,remove,view);
   if(!check(e,"remove stale overlay"))return false;
  }else return false;
 }
 jobject lp=nullptr;
 jclass lpCls=e->FindClass("android/view/ViewGroup$LayoutParams");
 jmethodID lpCtor=e->GetMethodID(lpCls,"<init>","(II)V");
 lp=e->NewObject(lpCls,lpCtor,-1,-1);
 if(!check(e,"create overlay layout"))return false;
 jclass grp=e->FindClass("android/view/ViewGroup");
 jmethodID add=e->GetMethodID(grp,"addView","(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V");
 e->CallVoidMethod(decor,add,view,lp);
 if(!check(e,"reattach overlay"))return false;
 LOGA("Reattached full HTML menu after game view transition");
 return true;
}
void raiseView(JNIEnv* e,jobject view){
 if(!view)return;
 jclass cls=e->FindClass("android/view/View");
 jmethodID front=e->GetMethodID(cls,"bringToFront","()V");
 jmethodID invalidate=e->GetMethodID(cls,"invalidate","()V");
 if(front)e->CallVoidMethod(view,front);
 jmethodID elevation=e->GetMethodID(cls,"setElevation","(F)V");
 if(elevation)e->CallVoidMethod(view,elevation,jfloat(128.0f));
 if(invalidate)e->CallVoidMethod(view,invalidate);
}
// LeviLaunchroid 1.6.1 normally hides external HUD buttons until the game HUD
// is active. Its built-in InbuiltOverlayManager checks the *same* button's
// persistent "show everywhere" preference when it is on a loading/title screen.
// The key is derived by ExternalModBridge.ExternalButton.positionKey():
// "external_button:" + buttonId. This is not a second Android overlay:
// registration, icon, saved position, size and HUD Editor all remain Levi-owned.
// Call on the Activity UI thread; no hidden API, memory patch or fake button.
void enableLeviButtonOnAllScreens(JNIEnv* e){
 if(!g_activity)return;
 jclass mgrClass=e->FindClass("org/levimc/launcher/core/mods/inbuilt/manager/InbuiltModManager");
 if(!check(e,"find Levi InbuiltModManager")||!mgrClass)return;
 jmethodID get=e->GetStaticMethodID(mgrClass,"getInstance",
    "(Landroid/content/Context;)Lorg/levimc/launcher/core/mods/inbuilt/manager/InbuiltModManager;");
 if(!check(e,"Levi InbuiltModManager.getInstance")||!get)return;
 jobject mgr=e->CallStaticObjectMethod(mgrClass,get,g_activity);
 if(!check(e,"Levi InbuiltModManager instance")||!mgr)return;
 jstring buttonKey=e->NewStringUTF("external_button:badlionmenuui.launcher");
 jmethodID isEverywhere=e->GetMethodID(mgrClass,"isOverlayShowEverywhere", "(Ljava/lang/String;)Z");
 jmethodID setEverywhere=e->GetMethodID(mgrClass,"setOverlayShowEverywhere", "(Ljava/lang/String;Z)V");
 if(!check(e,"Levi show-everywhere methods")||!isEverywhere||!setEverywhere){
  e->DeleteLocalRef(buttonKey);e->DeleteLocalRef(mgr);e->DeleteLocalRef(mgrClass);return;
 }
 bool wasEnabled=e->CallBooleanMethod(mgr,isEverywhere,buttonKey)==JNI_TRUE;
 if(!check(e,"read Levi show-everywhere")){
  e->DeleteLocalRef(buttonKey);e->DeleteLocalRef(mgr);e->DeleteLocalRef(mgrClass);return;
 }
 if(!wasEnabled){
  e->CallVoidMethod(mgr,setEverywhere,buttonKey,JNI_TRUE);
  if(check(e,"enable Levi show-everywhere")){
   LOGA("Enabled startup visibility for Levi HUD button via Show Everywhere: external_button:badlionmenuui.launcher");
   // Prompt a visibility recalculation now; do not wait for the game HUD.
   jclass overlays=e->FindClass("org/levimc/launcher/core/mods/inbuilt/overlay/InbuiltOverlayManager");
   if(overlays){
    jmethodID current=e->GetStaticMethodID(overlays,"getInstance",
     "()Lorg/levimc/launcher/core/mods/inbuilt/overlay/InbuiltOverlayManager;");
    if(current){
     jobject instance=e->CallStaticObjectMethod(overlays,current);
     if(instance){
      jmethodID refresh=e->GetMethodID(overlays,"refreshRuntimeVisibility","()V");
      if(refresh)e->CallVoidMethod(instance,refresh);
      e->DeleteLocalRef(instance);
     }
    }
    e->DeleteLocalRef(overlays);
   }
   check(e,"refresh Levi HUD visibility");
  }
 }
 if(!g_loggedLeviShowEverywhere){
  LOGA("Levi HUD button remains HUD-editor-managed and set to Show Everywhere");
  g_loggedLeviShowEverywhere=true;
 }
 e->DeleteLocalRef(buttonKey);e->DeleteLocalRef(mgr);e->DeleteLocalRef(mgrClass);
}
// Called exclusively by android's UI thread through UiTask.run() or a lifecycle callback.
void uiTick(JNIEnv* e){
 std::lock_guard guard(g_lock);
 if(!g_enabled){detachWidget(e);return;}
 if(!g_activity)return;
 // Keep the genuine Levi external button visible before the gameplay HUD is active.
 enableLeviButtonOnAllScreens(e);
 if(!g_root){
  auto html=readHtml();if(html.empty()){LOGA("Missing HTML resource");return;}
  jclass act=e->FindClass("android/app/Activity");
  jmethodID window=e->GetMethodID(act,"getWindow","()Landroid/view/Window;");
  jobject win=e->CallObjectMethod(g_activity,window);
  jclass winCls=e->FindClass("android/view/Window");
  jmethodID decorGetter=e->GetMethodID(winCls,"getDecorView","()Landroid/view/View;");
  jobject decor=e->CallObjectMethod(win,decorGetter);
  if(!check(e,"decor")||!decor)return;
  replace(e,g_decor,decor);
  jclass frameCls=e->FindClass("android/widget/FrameLayout");
  jmethodID frameCtor=e->GetMethodID(frameCls,"<init>","(Landroid/content/Context;)V");
  jobject root=e->NewObject(frameCls,frameCtor,g_activity);
  jclass webCls=e->FindClass("android/webkit/WebView");
  jmethodID webCtor=e->GetMethodID(webCls,"<init>","(Landroid/content/Context;)V");
  jobject web=e->NewObject(webCls,webCtor,g_activity);
  if(!check(e,"construct WebView")||!root||!web)return;
  jclass view=e->FindClass("android/view/View");
  jmethodID color=e->GetMethodID(view,"setBackgroundColor","(I)V");
  e->CallVoidMethod(root,color,0);e->CallVoidMethod(web,color,0);
  jmethodID getSettings=e->GetMethodID(webCls,"getSettings","()Landroid/webkit/WebSettings;");
  jobject cfg=e->CallObjectMethod(web,getSettings);
  jclass settings=e->FindClass("android/webkit/WebSettings");
  for(auto [name,flag]:{std::pair{"setJavaScriptEnabled",true},{"setDomStorageEnabled",true},
                        {"setAllowFileAccess",false},{"setAllowContentAccess",false},
                        {"setJavaScriptCanOpenWindowsAutomatically",false},
                        {"setSupportMultipleWindows",false}}){
   jmethodID m=e->GetMethodID(settings,name,"(Z)V");if(m)e->CallVoidMethod(cfg,m,flag);
  }
  // The DEX class implements native JS interface methods with a runtime visible
  // android.webkit.JavascriptInterface method annotation, required on API 17+.
  jclass hostCls=nullptr;
  // The HostApi class is cached as a GlobalRef via 'load dex' below.
  extern jclass g_hostClass;
  hostCls=g_hostClass;
  jmethodID ctor=e->GetMethodID(hostCls,"<init>","()V");
  jobject host=e->NewObject(hostCls,ctor);
  jmethodID bridge=e->GetMethodID(webCls,"addJavascriptInterface","(Ljava/lang/Object;Ljava/lang/String;)V");
  jstring bridgeName=e->NewStringUTF("BadlionHost");
  e->CallVoidMethod(web,bridge,host,bridgeName);
  jclass lpCls=e->FindClass("android/view/ViewGroup$LayoutParams");
  jmethodID lpCtor=e->GetMethodID(lpCls,"<init>","(II)V");
  jobject match=e->NewObject(lpCls,lpCtor,-1,-1);
  jclass group=e->FindClass("android/view/ViewGroup");
  jmethodID add=e->GetMethodID(group,"addView","(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V");
  e->CallVoidMethod(root,add,web,match);
  e->CallVoidMethod(decor,add,root,match);
  if(!check(e,"add overlay"))return;
  jmethodID load=e->GetMethodID(webCls,"loadDataWithBaseURL","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jstring base=e->NewStringUTF("https://badlion.local/");
  jstring source=e->NewStringUTF(html.c_str());
  jstring mime=e->NewStringUTF("text/html");jstring encoding=e->NewStringUTF("UTF-8");
  e->CallVoidMethod(web,load,base,source,mime,encoding,nullptr);
  if(!check(e,"load HTML"))return;
  replace(e,g_root,root);replace(e,g_web,web);
  LOGA("Full HTML WebView attached to Activity; launcher button owned by Levi HUD");
 }
 // Re-check current decor and parent EVERY tick. The Activity can survive
 // the loading -> gameplay transition while its child view tree is rebuilt.
 jobject current=activityDecor(e);
 if(current){
  if(g_root)ensureParented(e,g_root,current);
  if(!g_decor||!e->IsSameObject(current,g_decor))replace(e,g_decor,current);
 }
 bool shown=BadlionUI_GetVisible()!=0;
 showWidget(e,g_root,shown);
 if(shown){
  // A new renderer/SurfaceView may have been added after our original overlay.
  raiseView(e,g_root);
  if(g_web){
   jclass webCls=e->FindClass("android/webkit/WebView");
   jmethodID eval=e->GetMethodID(webCls,"evaluateJavascript","(Ljava/lang/String;Landroid/webkit/ValueCallback;)V");
   if(eval){
    jstring js=e->NewStringUTF("window.BadlionUI&&(BadlionUI.hostVisibility(true),BadlionUI.refresh())");
    e->CallVoidMethod(g_web,eval,js,nullptr);
   }
  }
 }
 if(g_lastVisible!=shown){
  LOGA("Menu visible=%d root=%p web=%p decor=%p",int(shown),(void*)g_root,(void*)g_web,(void*)g_decor);
  g_lastVisible=shown;
 }
 // A UI-owned recurrence keeps native model visibility and WebView display in sync.
 if(g_decor&&g_task){
  jclass view=e->FindClass("android/view/View");
  jmethodID delay=e->GetMethodID(view,"postDelayed","(Ljava/lang/Runnable;J)Z");
  if(delay)e->CallBooleanMethod(g_decor,delay,g_task,jlong(350));
 }
 check(e,"uiTick");
}
jclass g_hostClass=nullptr;
// Native callbacks registered with our InMemoryDexClassLoader, no System.load() needed.
jstring JNICALL hostSnapshot(JNIEnv* e,jobject){
 size_t n=BadlionUI_Snapshot(nullptr,0);
 std::vector<char> buf(n);BadlionUI_Snapshot(buf.data(),buf.size());
 return e->NewStringUTF(buf.data());
}
jboolean JNICALL hostSetEnabled(JNIEnv* e,jobject,jstring id,jboolean value){
 if(!id)return JNI_FALSE;
 const char* s=e->GetStringUTFChars(id,nullptr);
 int ok=BadlionUI_SetEnabled(s,value==JNI_TRUE);e->ReleaseStringUTFChars(id,s);
 return ok?JNI_TRUE:JNI_FALSE;
}
void JNICALL hostSetVisible(JNIEnv* e,jobject,jboolean value){
 BadlionUI_SetVisible(value==JNI_TRUE);
 std::lock_guard guard(g_lock);
 schedule(e);
}
void JNICALL hostSetMode(JNIEnv* e,jobject,jstring value){
 if(!value)return;
 const char* s=e->GetStringUTFChars(value,nullptr);
 BadlionUI_SetCompact(std::strcmp(s,"compact")==0);e->ReleaseStringUTFChars(value,s);
}
void JNICALL hostSetOpacity(JNIEnv*,jobject,jint value){BadlionUI_SetOpacity(value);}
void JNICALL taskRun(JNIEnv* e,jobject){uiTick(e);}
void setActivity(JNIEnv* e,jobject activity){
 std::lock_guard guard(g_lock);
 if(!g_enabled||!activity)return;
 if(g_activity&&e->IsSameObject(g_activity,activity))return;
 // Lifecycle transitions are on the UI thread; safely detach the old activity.
 if(g_activity)detachWidget(e);
 replace(e,g_activity,activity);
 g_loggedLeviShowEverywhere=false;
 schedule(e);
}
void JNICALL activityCreated(JNIEnv* e,jobject,jobject a,jobject){setActivity(e,a);}
void JNICALL activityStarted(JNIEnv* e,jobject,jobject a){setActivity(e,a);}
void JNICALL activityResumed(JNIEnv* e,jobject,jobject a){setActivity(e,a);}
void JNICALL activityPaused(JNIEnv*,jobject,jobject){}
void JNICALL activityStopped(JNIEnv*,jobject,jobject){}
void JNICALL activitySaved(JNIEnv*,jobject,jobject,jobject){}
void JNICALL activityDestroyed(JNIEnv* e,jobject,jobject a){
 std::lock_guard guard(g_lock);
 if(g_activity&&e->IsSameObject(a,g_activity)){detachWidget(e);release(e,g_activity);}
}
// Dynamic DEX host bootstrapping.
bool loadDex(JNIEnv* e){
 if(g_dexReady){
  if(!g_observerRegistered&&g_app&&g_observer){
   jclass appCls=e->FindClass("android/app/Application");
   jmethodID registerCb=e->GetMethodID(appCls,"registerActivityLifecycleCallbacks","(Landroid/app/Application$ActivityLifecycleCallbacks;)V");
   if(registerCb)e->CallVoidMethod(g_app,registerCb,g_observer);
   if(!check(e,"re-register Activity observer"))return false;
   g_observerRegistered=true;
  }
  return true;
 }
 auto bytes=readDex();if(bytes.empty()){LOGA("Missing resources/classes.dex");return false;}
 jclass bufferClass=e->FindClass("java/nio/ByteBuffer");
 jmethodID wrap=e->GetStaticMethodID(bufferClass,"wrap","([B)Ljava/nio/ByteBuffer;");
 jbyteArray a=e->NewByteArray(jint(bytes.size()));
 e->SetByteArrayRegion(a,0,jint(bytes.size()),reinterpret_cast<const jbyte*>(bytes.data()));
 jobject buffer=e->CallStaticObjectMethod(bufferClass,wrap,a);
 jclass loaderCls=e->FindClass("dalvik/system/InMemoryDexClassLoader");
 if(!check(e,"InMemoryDexClassLoader")||!loaderCls)return false;
 jobject app=nullptr;
 if(jobject a=preloaderActivity(e)){
  jclass act=e->FindClass("android/app/Activity");
  jmethodID getter=e->GetMethodID(act,"getApplication","()Landroid/app/Application;");
  if(getter)app=e->CallObjectMethod(a,getter);
  check(e,"Activity.getApplication");
 }
 if(!app)app=currentApplication(e);
 if(!app){LOGA("Cannot locate Android Application");return false;}
 replace(e,g_app,app);
 jclass context=e->FindClass("android/content/Context");
 jmethodID getClassLoader=e->GetMethodID(context,"getClassLoader","()Ljava/lang/ClassLoader;");
 jobject parent=e->CallObjectMethod(app,getClassLoader);
 jmethodID constructor=e->GetMethodID(loaderCls,"<init>","(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
 jobject loader=e->NewObject(loaderCls,constructor,buffer,parent);
 if(!check(e,"create in-memory DEX loader")||!loader)return false;
 jclass base=e->FindClass("java/lang/ClassLoader");
 jmethodID load=e->GetMethodID(base,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
 auto getClass=[&](const char* name)->jclass{
  jstring s=e->NewStringUTF(name);
  jclass c=(jclass)e->CallObjectMethod(loader,load,s);
  if(!check(e,name))return nullptr;
  return c;
 };
 jclass host=getClass("org.therealbatu.badlion.HostApi");
 jclass task=getClass("org.therealbatu.badlion.UiTask");
 jclass obs=getClass("org.therealbatu.badlion.ActivityObserver");
 if(!host||!task||!obs)return false;
 JNINativeMethod hostMethods[]={
  {const_cast<char*>("snapshot"),const_cast<char*>("()Ljava/lang/String;"),(void*)hostSnapshot},
  {const_cast<char*>("setEnabled"),const_cast<char*>("(Ljava/lang/String;Z)Z"),(void*)hostSetEnabled},
  {const_cast<char*>("setVisible"),const_cast<char*>("(Z)V"),(void*)hostSetVisible},
  {const_cast<char*>("setMode"),const_cast<char*>("(Ljava/lang/String;)V"),(void*)hostSetMode},
  {const_cast<char*>("setOpacity"),const_cast<char*>("(I)V"),(void*)hostSetOpacity},
 };
 JNINativeMethod taskMethods[]={{const_cast<char*>("run"),const_cast<char*>("()V"),(void*)taskRun}};
 JNINativeMethod obsMethods[]={
  {const_cast<char*>("onActivityCreated"),const_cast<char*>("(Landroid/app/Activity;Landroid/os/Bundle;)V"),(void*)activityCreated},
  {const_cast<char*>("onActivityStarted"),const_cast<char*>("(Landroid/app/Activity;)V"),(void*)activityStarted},
  {const_cast<char*>("onActivityResumed"),const_cast<char*>("(Landroid/app/Activity;)V"),(void*)activityResumed},
  {const_cast<char*>("onActivityPaused"),const_cast<char*>("(Landroid/app/Activity;)V"),(void*)activityPaused},
  {const_cast<char*>("onActivityStopped"),const_cast<char*>("(Landroid/app/Activity;)V"),(void*)activityStopped},
  {const_cast<char*>("onActivitySaveInstanceState"),const_cast<char*>("(Landroid/app/Activity;Landroid/os/Bundle;)V"),(void*)activitySaved},
  {const_cast<char*>("onActivityDestroyed"),const_cast<char*>("(Landroid/app/Activity;)V"),(void*)activityDestroyed},
 };
 if(e->RegisterNatives(host,hostMethods,5)<0||e->RegisterNatives(task,taskMethods,1)<0||e->RegisterNatives(obs,obsMethods,7)<0||!check(e,"RegisterNatives"))return false;
 jmethodID tCtor=e->GetMethodID(task,"<init>","()V");
 jmethodID oCtor=e->GetMethodID(obs,"<init>","()V");
 jobject runnable=e->NewObject(task,tCtor);jobject observer=e->NewObject(obs,oCtor);
 if(!check(e,"construct bridge")||!runnable||!observer)return false;
 replace(e,g_task,runnable);replace(e,g_observer,observer);g_hostClass=(jclass)e->NewGlobalRef(host);
 jclass appCls=e->FindClass("android/app/Application");
 jmethodID registerCb=e->GetMethodID(appCls,"registerActivityLifecycleCallbacks","(Landroid/app/Application$ActivityLifecycleCallbacks;)V");
 e->CallVoidMethod(g_app,registerCb,g_observer);
 if(!check(e,"register ActivityLifecycleCallbacks"))return false;
 g_dexReady=true;g_observerRegistered=true;
 LOGA("In-memory DEX, JavaScript bridge, Activity observer registered");
 return true;
}
} // namespace

// The preloader passes a JavaVM through the official ModContext lifecycle.
// Prefer that over libart symbol lookup, which is not available on all builds.
void Badlion_AutoHostSetJavaVM(JavaVM* vm){
 std::lock_guard guard(g_lock);
 if(vm)g_vm=vm;
}
void Badlion_AutoHostStart(const std::string& folder){
 std::lock_guard guard(g_lock);
 g_enabled=true;g_modDir=folder;
 auto vm=findVM();
 if(!vm){LOGA("JavaVM unavailable: native button will remain usable; retry on next click");return;}
 EnvScope scope(vm);auto e=scope.e;if(!e)return;
 if(!loadDex(e))return;
 if(!g_activity){jobject a=getActivity(e);if(a)setActivity(e,a);}
 else schedule(e);
}
void Badlion_AutoHostStop(){
 std::lock_guard guard(g_lock);g_enabled=false;
 if(!g_vm)return;
 EnvScope env(g_vm);auto e=env.e;if(!e)return;
 if(g_observerRegistered&&g_app&&g_observer){
  jclass appCls=e->FindClass("android/app/Application");
  jmethodID unregister=e->GetMethodID(appCls,"unregisterActivityLifecycleCallbacks","(Landroid/app/Application$ActivityLifecycleCallbacks;)V");
  if(unregister)e->CallVoidMethod(g_app,unregister,g_observer);
  g_observerRegistered=false;
 }
 if(g_decor&&g_task){
  jclass view=e->FindClass("android/view/View");
  jmethodID remove=e->GetMethodID(view,"removeCallbacks","(Ljava/lang/Runnable;)Z");
  if(remove)e->CallBooleanMethod(g_decor,remove,g_task);
 }
 // Android Views must only be destroyed on the UI thread; avoid touching them
 // off thread. If UI thread is current, dispose immediately; otherwise hide
 // via UI task (the native library must stay mapped until this runs).
 if(g_activity&&g_task)schedule(e);
 check(e,"auto host stop");
}

void Badlion_AutoHostRequestRefresh(){
 std::lock_guard guard(g_lock);
 if(!g_vm)return;
 EnvScope env(g_vm);
 if(env.e)schedule(env.e);
}
