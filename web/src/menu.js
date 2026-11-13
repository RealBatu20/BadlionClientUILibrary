
'use strict';
const BASE_CATEGORIES=[
 {id:'PvP',name:'PvP',icon:'⚔',builtin:true},
 {id:'Hypixel',name:'Hypixel',icon:'ϟ',builtin:true},
 {id:'Visual',name:'Visual',icon:'◎',builtin:true}
];
// Exactly three replaceable developer-facing templates, not 49 fake gameplay mods.
const DEFAULTS=[
 {id:'badlion.template.pvp',name:'PvP Button Template',category:'PvP',icon:'⚔',enabled:false,template:true,description:'Example native toggle callback. Register your own PvP module to implement actual mod functionality.'},
 {id:'badlion.template.hypixel',name:'Hypixel Button Template',category:'Hypixel',icon:'ϟ',enabled:false,template:true,description:'Example native toggle callback. Register your own Hypixel module to implement actual mod functionality.'},
 {id:'badlion.template.visual',name:'Visual Button Template',category:'Visual',icon:'◎',enabled:false,template:true,description:'Example native toggle callback. Register your own Visual module to implement actual mod functionality.'}
];
const state={
 items:DEFAULTS.map(x=>({...x})),categories:BASE_CATEGORIES.map(x=>({...x})),
 filter:'All',tab:'Mods',mode:'normal',opacity:95,templatesVisible:true,nativeTemplatesVisible:true,
 accent:'#2ca9d0',theme:'Regular',visible:true,square:false,userScale:100,
 profiles:{Default:{}},currentProfile:'Default',editMode:false,dragX:0,dragY:0,
 selectedModule:null,nativeTitle:'BADLION CLIENT',titleOverride:null
};
const $=id=>document.getElementById(id);
const native=()=>!!(window.BadlionHost&&typeof window.BadlionHost.snapshot==='function');
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const safeTitle=v=>{const t=String(v??'').trim().replace(/[\x00-\x1f\x7f]/g,'');return t.slice(0,64)||'BADLION CLIENT';};
const safeColor=v=>/^#[0-9a-f]{6}$/i.test(v)?v:'#2ca9d0';
const profileKey='badlion-ui-v2'; // Keep saved profiles and themes across upgrades.
let lastNativeSnapshot='';
function sync(){
 if(!native())return false;
 try{
  const raw=window.BadlionHost.snapshot();
  if(raw===lastNativeSnapshot)return false;
  lastNativeSnapshot=raw;
  const d=JSON.parse(raw);
  if(d&&Array.isArray(d.modules)){
   const previous=new Map(state.items.map(x=>[x.id,x]));
   state.items=d.modules.map(x=>({...previous.get(x.id),...x,
    icon:x.icon||previous.get(x.id)?.icon||'◇',
    category:x.category||previous.get(x.id)?.category||'Visual',
    template:!!x.template}));
  }
  if(d&&Array.isArray(d.categories)){
   const unique=new Map();
   d.categories.forEach(c=>{if(c&&typeof c.id==='string'&&c.id!=='All')
    unique.set(c.id,{id:c.id,name:c.name||c.id,icon:c.icon||'◇',builtin:!!c.builtin});});
   state.categories=Array.from(unique.values());
  }else if(d&&Array.isArray(d.modules)){
   // Older native host snapshots may not have a categories key.
   const known=new Map(BASE_CATEGORIES.map(c=>[c.id,{...c}]));
   d.modules.forEach(m=>{if(m.category&&!known.has(m.category))known.set(m.category,{id:m.category,name:m.category,icon:'◇',builtin:false});});
   state.categories=Array.from(known.values());
  }
  if(typeof d.templatesVisible==='boolean')state.nativeTemplatesVisible=d.templatesVisible;
  if(typeof d.title==='string')state.nativeTitle=safeTitle(d.title);
  if(d.mode)state.mode=d.mode;
  if(typeof d.visible==='boolean')state.visible=d.visible;
  if(typeof d.opacity==='number')state.opacity=d.opacity;
  if(state.filter!=='All'&&!state.categories.some(c=>c.id===state.filter))state.filter='All';
  return true;
 }catch(e){console.warn('Native synchronization failed:',e);return false;}
}
function persist(){
 try{
  const snapshot={
   mode:state.mode,opacity:state.opacity,accent:state.accent,theme:state.theme,
   templatesVisible:state.templatesVisible,titleOverride:state.titleOverride,
   square:state.square,userScale:state.userScale,profiles:state.profiles,
   currentProfile:state.currentProfile,dragX:state.dragX,dragY:state.dragY,
   items:state.items.map(x=>({id:x.id,enabled:x.enabled}))
  };
  localStorage.setItem(profileKey,JSON.stringify(snapshot));
 }catch(e){/* Storage unavailable: the C++ module registry is still authoritative. */}
}
function restore(){
 try{
  const d=JSON.parse(localStorage.getItem(profileKey)||'{}');
  if(typeof d.titleOverride==='string' && d.titleOverride.trim())state.titleOverride=safeTitle(d.titleOverride);
  if(d.mode==='normal'||d.mode==='compact')state.mode=d.mode;
  if(Number.isInteger(d.opacity))state.opacity=Math.max(40,Math.min(100,d.opacity));
  if(d.accent)state.accent=safeColor(d.accent);
  if(['Regular','Opacity','Insider'].includes(d.theme))state.theme=d.theme;
  state.square=!!d.square;
  if(typeof d.templatesVisible==='boolean')state.templatesVisible=d.templatesVisible;
  if(Number.isInteger(d.userScale))state.userScale=Math.max(70,Math.min(130,d.userScale));
  if(d.profiles&&typeof d.profiles==='object'&&!Array.isArray(d.profiles))state.profiles={...d.profiles,Default:d.profiles.Default||{}};
  if(d.currentProfile&&state.profiles[d.currentProfile])state.currentProfile=d.currentProfile;
  if(Number.isFinite(d.dragX))state.dragX=d.dragX;
  if(Number.isFinite(d.dragY))state.dragY=d.dragY;
  if(!native()){
   const map=new Map((d.items||[]).map(x=>[x.id,!!x.enabled]));
   state.items.forEach(x=>{if(map.has(x.id))x.enabled=map.get(x.id);});
  }
 }catch(e){console.warn('Saved settings could not be restored:',e);}
}
function applyTitle(){const t=state.titleOverride||state.nativeTitle||'BADLION CLIENT';$('brandTitle').textContent=t;document.title=t+' · Native UI Library';}
function setTitle(title){state.titleOverride=safeTitle(title);applyTitle();persist();return true;}
function resetTitle(){state.titleOverride=null;applyTitle();persist();}
function updateScale(){
 const vp=window.visualViewport;
 const width=vp?vp.width:innerWidth, height=vp?vp.height:innerHeight;
 const portrait=height>width&&width<680;
 const compact=state.mode==='compact'||portrait;
 const design=portrait?{w:380,h:690}:compact?{w:570,h:475}:{w:820,h:460};
 const margin=10;
 const availableW=(width-margin*2)/design.w, availableH=(height-margin*2)/design.h;
 const base=Math.min(1,availableW,availableH);
 const scale=Math.max(.25,Math.min(availableW,availableH,base*(state.userScale/100)));
 const roomX=Math.max(0,(width-design.w*scale)/2);
 const roomY=Math.max(0,(height-design.h*scale)/2);
 state.dragX=Math.max(-roomX,Math.min(roomX,state.dragX));
 state.dragY=Math.max(-roomY,Math.min(roomY,state.dragY));
 const css=document.documentElement.style;
 css.setProperty('--scale',scale.toFixed(4));
 css.setProperty('--offset-x',state.dragX.toFixed(1)+'px');
 css.setProperty('--offset-y',state.dragY.toFixed(1)+'px');
 css.setProperty('--opacity',(state.opacity/100).toFixed(2));
 css.setProperty('--accent',state.theme==='Insider'?'#45bb4a':safeColor(state.accent));
 document.body.classList.toggle('compact',compact);
 document.body.classList.toggle('portrait',portrait);
 document.body.classList.toggle('square-grid',state.square);
 document.body.classList.toggle('is-hidden',!state.visible);
 document.body.classList.toggle('native-host',native());
 document.body.classList.toggle('editing',state.editMode);
 const viewport=$('viewportReadout');
 if(viewport)viewport.textContent=`${Math.round(width)} × ${Math.round(height)} CSS px · ${Math.round(devicePixelRatio*100)/100} DPR`;
}
function setVisible(next){
 state.visible=!!next;
 try{if(native())window.BadlionHost.setVisible(state.visible);}catch(e){console.warn(e);}
 updateScale();persist();
}
function setMode(next){
 state.mode=next==='compact'?'compact':'normal';
 try{if(native())window.BadlionHost.setMode(state.mode);}catch(e){console.warn(e);}
 updateScale();render();persist();
}
function toggle(id){
 const m=state.items.find(x=>x.id===id);if(!m)return false;
 const next=!m.enabled;
 try{
  if(native()){
   if(!window.BadlionHost.setEnabled(id,next))return false;
  }else if(typeof m.onToggle==='function'){
   // A functional browser-side developer callback, not fake gameplay logic.
   m.onToggle(next,id);
  }else if(m.template){
   console.info('[BadlionUI] Template callback',id,next);
  }
 }catch(e){console.warn('Module callback error',e);return false;}
 m.enabled=next;render();persist();return true;
}
function visibleModules(){
 return state.items.filter(m=>!(m.template&&(!state.templatesVisible||!state.nativeTemplatesVisible)));
}
function filteredModules(){
 const q=$('search').value.toLowerCase().trim();
 return visibleModules().filter(m=>(state.filter==='All'||m.category===state.filter)&&
  (!q||[m.name,m.id,m.description||'',m.category].some(x=>String(x).toLowerCase().includes(q))));
}
function renderFilters(){
 const all=[{id:'All',name:'All',icon:'☰',builtin:true},...state.categories];
 const holder=$('categoryFilters');
 holder.innerHTML=all.map(c=>`<button class="filterBtn ${state.filter===c.id?'active':''}"
  data-filter="${esc(c.id)}" title="${esc(c.name)}" aria-pressed="${state.filter===c.id}"><span class="glyph">${esc(c.icon||'◇')}</span><span class="filter-name">${esc(c.name)}</span></button>`).join('');
 holder.querySelectorAll('[data-filter]').forEach(b=>b.onclick=()=>{state.filter=b.dataset.filter;state.tab='Mods';hideDetails();render();});
}
function showDetails(id){
 const m=state.items.find(x=>x.id===id);if(!m)return;
 state.selectedModule=id;
 const d=$('detail');d.hidden=false;
 d.innerHTML=`<div class="detail-box" role="dialog" aria-modal="true" aria-label="Module details">
 <div class="detail-heading"><span class="detail-symbol">${esc(m.icon||'⬡')}</span><div><strong>${esc(m.name)}</strong><small>${esc(state.categories.find(c=>c.id===m.category)?.name||m.category||'Visual')}</small></div><button class="detail-close" id="detailClose" aria-label="Close details">×</button></div>
 <p>${esc(m.description||'No description provided by the mod developer.')}</p>
 <p class="module-id">${esc(m.id)}</p>
 <p class="detail-status">${m.template?'Developer starter template · callback enabled; no gameplay effect':native()?'Registered native module':'Developer module · callback controlled by developer'}</p>
 <div class="detail-actions"><button id="detailToggle" class="tab ${m.enabled?'active':''}">${m.enabled?'Disable':'Enable'} Module</button><button id="detailDone" class="tab">Done</button></div></div>`;
 $('detailClose').onclick=hideDetails;
 $('detailDone').onclick=hideDetails;
 $('detailToggle').onclick=()=>{if(toggle(id))showDetails(id);};
 d.onclick=e=>{if(e.target===d)hideDetails();};
}
function hideDetails(){state.selectedModule=null;$('detail').hidden=true;$('detail').innerHTML='';}
function renderModules(){
 const filtered=filteredModules();
 const grid=$('modules');
 grid.classList.toggle('templates-only',state.filter==='All'&&filtered.length===3&&filtered.every(m=>m.template));
 grid.innerHTML=filtered.map(m=>`<div class="module ${m.template?'template-card':''}" title="${esc(m.description||'')}" data-module="${esc(m.id)}" tabindex="0" role="group" aria-label="${esc(m.name)}">
 <span class="modtitle">${esc(m.name)}</span>${m.template?'<span class="template-mark">TEMPLATE</span>':''}<span class="symbol" aria-hidden="true">${m.template?`<img class="template-logo" src="${$('brandLogo').src}" alt="">`:esc(m.icon||'◇')}</span>
 <button class="switch ${m.enabled?'on':''}" role="switch" aria-checked="${m.enabled}" aria-label="Toggle ${esc(m.name)}" data-id="${esc(m.id)}"></button></div>`).join('')||'<p class="empty">No modules in this category. Register one from your mod.</p>';
 grid.querySelectorAll('[data-id]').forEach(b=>b.addEventListener('click',e=>{e.stopPropagation();toggle(b.dataset.id);}));
 grid.querySelectorAll('[data-module]').forEach(m=>{
  m.onclick=()=>showDetails(m.dataset.module);
  m.onkeydown=e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();showDetails(m.dataset.module);}};
 });
}
function settings(){
 return `<div class="page"><h2>Settings</h2><div class="prefs">
 <section class="pref"><label for="menuTitle">Menu Header Text</label><input id="menuTitle" type="text" maxlength="64" autocomplete="off" value="${esc(state.titleOverride||state.nativeTitle)}"><button class="action" id="resetTitle">Use Developer Title</button><p class="custom">Changes the words only. The original condensed Badlion-style font and sizing are preserved.</p></section>
 <section class="pref"><label>GUI Layout</label><div class="segment"><button id="normal" class="${state.mode==='normal'?'active':''}">Normal Mode</button><button id="compact" class="${state.mode==='compact'?'active':''}">Compact Mode</button></div></section>
 <section class="pref"><label>Theme</label><div class="segment">${['Regular','Opacity','Insider'].map(s=>`<button data-theme="${s}" class="${state.theme===s?'active':''}">${s}</button>`).join('')}</div></section>
 <section class="pref"><label>Opacity <span id="opacityValue">${state.opacity}%</span></label><input id="opacity" aria-label="Opacity" type="range" min="40" max="100" value="${state.opacity}"></section>
 <section class="pref"><label>UI Size <span id="scaleValue">${state.userScale}%</span></label><input id="uiScale" aria-label="UI size" type="range" min="70" max="130" value="${state.userScale}"></section>
 <section class="pref"><label>Module Square Size</label><div class="segment"><button id="sizeStandard" class="${!state.square?'active':''}">Standard</button><button id="sizeSquare" class="${state.square?'active':''}">Square</button></div></section>
 <section class="pref"><label>Accent Color</label><input id="accent" type="color" aria-label="Accent color" value="${safeColor(state.accent)}" ${state.theme==='Insider'?'disabled':''}><p class="custom">Insider uses a green accent.</p></section>
 <section class="pref"><label>Screen / Resolution</label><p id="viewportReadout"></p><p class="custom">Automatic orientation and safe-area fitting is enabled.</p></section>
 <section class="pref"><label>Panel Position</label><div class="segment"><button id="editPosition">${state.editMode?'Finish Dragging':'Drag Panel'}</button><button id="resetPosition">Reset</button></div><p class="custom">Drag the header while editing.</p></section>
 <section class="pref"><label>Developer Template Cards</label><div class="segment"><button id="templatesToggle">${state.templatesVisible?'Hide 3 Templates':'Show 3 Templates'}</button></div><p class="custom">Only examples. Hide them after registering your own modules. Custom categories remain available.</p></section>
 <section class="pref"><label>Startup Overlay</label><p>Enabled when the Android host is attached; opening is controlled by the native button.</p></section>
 </div><p class="footnote">Native UI library · TheRealBatu</p></div>`;
}
function profiles(){
 return `<div class="page"><h2>Profiles</h2><p>Save, load and manage module switch combinations.</p><div class="prefs">
 <section class="pref"><label for="profile">Current Profile</label><select id="profile">${Object.keys(state.profiles).map(name=>`<option value="${esc(name)}" ${state.currentProfile===name?'selected':''}>${esc(name)}</option>`).join('')}</select></section>
 <section class="pref"><label>Create Profile</label><input id="newProfile" type="text" maxlength="36" placeholder="Profile name" autocomplete="off"><button class="action" id="createProfile">Create</button></section>
 <section class="pref"><label>Manage Profile</label><div class="segment"><button id="saveProfile">Save</button><button id="loadProfile">Load</button><button id="deleteProfile" ${state.currentProfile==='Default'?'disabled':''}>Delete</button></div></section>
 <section class="pref"><label>Mod Function Integration</label><p>Profile loading calls the same registered C++ toggle callbacks as the module switches.</p></section>
 </div><p class="footnote" id="profileMessage">Profiles save locally in the UI storage.</p></div>`;
}
function render(){
 applyTitle();updateScale();
 document.querySelectorAll('[data-tab]').forEach(b=>b.classList.toggle('active',b.dataset.tab===state.tab));
 renderFilters();
 $('search').style.visibility=state.tab==='Mods'?'visible':'hidden';
 $('content').innerHTML=state.tab==='Mods'?'<div class="module-grid" id="modules"></div>':
 state.tab==='Settings'?settings():state.tab==='Profiles'?profiles():
 '<div class="page"><h2>Modules</h2></div>';
 if(state.tab==='Mods')renderModules();
 else if(state.tab==='Settings'){
  $('menuTitle').oninput=e=>setTitle(e.target.value);$('resetTitle').onclick=()=>{resetTitle();$('menuTitle').value=state.nativeTitle;};
  $('normal').onclick=()=>setMode('normal');$('compact').onclick=()=>setMode('compact');
  $('templatesToggle').onclick=()=>setTemplatesVisible(!state.templatesVisible);
  $('opacity').oninput=e=>{state.opacity=+e.target.value;$('opacityValue').textContent=state.opacity+'%';try{if(native())window.BadlionHost.setOpacity(state.opacity);}catch(err){console.warn(err);}updateScale();persist();};
  $('uiScale').oninput=e=>{state.userScale=+e.target.value;$('scaleValue').textContent=state.userScale+'%';updateScale();persist();};
  $('accent').oninput=e=>{state.accent=safeColor(e.target.value);updateScale();persist();};
  $('sizeStandard').onclick=()=>{state.square=false;render();persist();};
  $('sizeSquare').onclick=()=>{state.square=true;render();persist();};
  document.querySelectorAll('[data-theme]').forEach(b=>b.onclick=()=>{state.theme=b.dataset.theme;render();persist();});
  $('editPosition').onclick=()=>{state.editMode=!state.editMode;render();};
  $('resetPosition').onclick=()=>{state.dragX=state.dragY=0;state.editMode=false;render();persist();};
 }
 else if(state.tab==='Profiles'){
  $('profile').onchange=e=>{state.currentProfile=e.target.value;render();persist();};
  $('createProfile').onclick=()=>{
   const name=$('newProfile').value.trim().slice(0,36);
   if(!name)return message('Enter a profile name.');
   if(state.profiles[name])return message('That profile already exists.');
   if(Object.keys(state.profiles).length>=24)return message('Maximum 24 profiles.');
   state.profiles[name]=Object.fromEntries(state.items.map(x=>[x.id,x.enabled]));
   state.currentProfile=name;render();persist();message('Profile created: '+name);
  };
  $('saveProfile').onclick=()=>{
   state.profiles[state.currentProfile]=Object.fromEntries(state.items.map(x=>[x.id,x.enabled]));
   persist();message('Saved: '+state.currentProfile);
  };
  $('loadProfile').onclick=()=>{
   const p=state.profiles[state.currentProfile];if(!p)return;
   let count=0;
   for(const [id,value] of Object.entries(p)){
    const m=state.items.find(x=>x.id===id);
    if(m&&m.enabled!==value&&toggle(id))count++;
   }
   persist();message('Loaded '+state.currentProfile+' · '+count+' switches updated');
  };
  $('deleteProfile').onclick=()=>{
   if(state.currentProfile==='Default')return;
   delete state.profiles[state.currentProfile];state.currentProfile='Default';render();persist();message('Profile deleted.');
  };
 }
 updateScale();
}
function message(msg){const el=$('profileMessage');if(el)el.textContent=msg;}
$('close').onclick=()=>setVisible(false);
$('search').oninput=()=>{if(state.tab==='Mods')renderModules();};
document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>{state.tab=b.dataset.tab;hideDetails();render();});
$('edit').onclick=()=>{state.tab='Settings';state.editMode=true;render();};
window.addEventListener('resize',updateScale);
window.addEventListener('orientationchange',updateScale);
if(window.visualViewport)window.visualViewport.addEventListener('resize',updateScale);
window.addEventListener('keydown',e=>{if(e.key==='Escape'){if(!$('detail').hidden)hideDetails();else setVisible(false);}});
// Touch and mouse drag, enabled by the Edit GUI action or Settings.
let pointer=null;
$('panelHeader').addEventListener('pointerdown',e=>{
 if(!state.editMode||e.target.closest('button'))return;
 pointer={x:e.clientX,y:e.clientY,dx:state.dragX,dy:state.dragY};
 $('panelHeader').setPointerCapture(e.pointerId);
});
$('panelHeader').addEventListener('pointermove',e=>{
 if(!pointer)return;
 state.dragX=pointer.dx+(e.clientX-pointer.x);
 state.dragY=pointer.dy+(e.clientY-pointer.y);
 updateScale();
});
$('panelHeader').addEventListener('pointerup',()=>{if(pointer){pointer=null;persist();}});
$('panelHeader').addEventListener('pointercancel',()=>{pointer=null;});
function setTemplatesVisible(visible){
 state.templatesVisible=!!visible;
 render();persist();
}
const validId=value=>typeof value==='string'&&/^[a-zA-Z0-9._-]{1,95}$/.test(value);
window.BadlionUI={
 registerCategory(spec){
  if(native())return false; // On Android, use BadlionUI_RegisterCategory from C++.
  if(!spec||!validId(spec.id)||spec.id==='All'||typeof spec.name!=='string'||
     !spec.name.trim()||spec.name.length>63||state.categories.some(x=>x.id===spec.id))return false;
  state.categories.push({id:spec.id,name:spec.name.trim(),icon:String(spec.icon||'◇').slice(0,16),builtin:false});
  render();return true;
 },
 unregisterCategory(id){
  if(native())return false;
  const index=state.categories.findIndex(x=>x.id===id&&!x.builtin);
  if(index<0||state.items.some(m=>m.category===id))return false;
  state.categories.splice(index,1);
  if(state.filter===id)state.filter='All';
  render();return true;
 },
 registerModule(spec){
  if(native())return false; // Register via the native C API instead.
  if(!spec||!validId(spec.id)||typeof spec.name!=='string'||!spec.name.trim()||
   state.items.some(x=>x.id===spec.id))return false;
  const category=spec.category||'Visual';
  if(!state.categories.some(c=>c.id===category))return false;
  state.items.push({...spec,name:spec.name.trim(),icon:spec.icon||'◇',enabled:!!spec.enabled,category,template:false});
  render();return true;
 },
 unregisterModule(id){
  if(native())return false;
  const index=state.items.findIndex(x=>x.id===id&&!x.template);
  if(index<0)return false;
  state.items.splice(index,1);
  render();return true;
 },
 setTemplatesVisible,
 setTitle,resetTitle,getTitle:()=>state.titleOverride||state.nativeTitle,
 setVisible,setMode,
 hostVisibility(shown){
  if(state.visible!==!!shown){state.visible=!!shown;updateScale();render();}
 },
 refresh(){if(sync())render();},
 getState(){return state;},
 getCategories(){return state.categories.map(x=>({...x}));},
 showDetails,hideDetails,
};

$('brandLogo').src='data:image/png;base64,__LOGO_B64__';restore();sync();updateScale();render();

