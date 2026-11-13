"""Regression: 3 actual template cards, filters, custom category+callback lifecycle.
Tests the distributed generated HTML with Chromium; fake native host tests JSON contract.
"""
from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
html=(ROOT/'web/index.html').read_text(encoding='utf8')
with sync_playwright() as pw:
    b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
    page=b.new_page(viewport={'width':1536,'height':814})
    errors=[]
    page.on('pageerror',lambda err:errors.append(str(err)))
    page.set_content(html)
    assert page.locator('.module').count()==3
    assert page.locator('.template-card').count()==3
    assert page.locator('.filterBtn').all_text_contents()==['☰All','⚔PvP','ϟHypixel','◎Visual']
    for category,identifier in [('PvP','badlion.template.pvp'),('Hypixel','badlion.template.hypixel'),('Visual','badlion.template.visual')]:
        page.locator(f'[data-filter="{category}"]').click()
        assert page.locator('.module').count()==1
        assert page.locator('.module').get_attribute('data-module')==identifier
        page.locator(f'[data-id="{identifier}"]').click()
        assert page.locator('.switch.on').count()==1
    page.locator('[data-filter="All"]').click()
    assert page.locator('.module').count()==3
    # New categories can host real browser callbacks; never auto-create missing categories.
    assert page.evaluate("BadlionUI.registerModule({id:'invalid.m',name:'Missing',category:'custom.nonexistent'})") is False
    assert page.evaluate("BadlionUI.registerCategory({id:'custom.combat',name:'Combat Tools',icon:'✦'})") is True
    assert page.locator('[data-filter="custom.combat"]').count()==1
    assert page.evaluate("""() => {
       window.callbackLog=[];
       return BadlionUI.registerModule({id:'my.features.aim',name:'Aim Overlay',category:'custom.combat',icon:'⚙',
           description:'Developer-owned callback feature',onToggle:(enabled,id)=>window.callbackLog.push([id,enabled])});
    }""") is True
    page.locator('[data-filter="custom.combat"]').click()
    assert page.locator('.module').count()==1
    page.locator('[data-id="my.features.aim"]').click()
    assert page.evaluate('window.callbackLog')==[['my.features.aim',True]]
    assert page.evaluate("BadlionUI.unregisterCategory('custom.combat')") is False
    assert page.evaluate("BadlionUI.unregisterModule('my.features.aim')") is True
    assert page.evaluate("BadlionUI.unregisterCategory('custom.combat')") is True
    assert page.locator('[data-filter="custom.combat"]').count()==0
    assert page.evaluate('BadlionUI.getState().filter')=='All'
    # The 3 starter templates are removable from view without removing categories.
    page.locator('[data-tab="Settings"]').click()
    page.locator('#templatesToggle').click()
    page.locator('[data-tab="Mods"]').click()
    assert page.locator('.module').count()==0
    assert page.locator('.filterBtn').count()==4
    page.evaluate('BadlionUI.setTemplatesVisible(true)')
    assert page.locator('.module').count()==3
    assert not errors,errors
    # When running in native mode, snapshot owns both category and module catalog.
    page.evaluate("""() => {
       window.simulated={visible:true,mode:'normal',opacity:95,templatesVisible:true,
         categories:[{id:'PvP',name:'PvP',icon:'⚔',builtin:true},
           {id:'Hypixel',name:'Hypixel',icon:'ϟ',builtin:true},
           {id:'Visual',name:'Visual',icon:'◎',builtin:true},
           {id:'custom.hud',name:'My HUD Widgets',icon:'✶',builtin:false}],
         modules:[{id:'dev.widget',name:'Stats HUD',category:'custom.hud',icon:'▣',enabled:false,template:false}]};
       window.BadlionHost={snapshot:()=>JSON.stringify(simulated),
        setVisible:v=>simulated.visible=v,setMode:v=>simulated.mode=v,setOpacity:v=>simulated.opacity=v,
        setEnabled:(id,v)=>{let m=simulated.modules.find(m=>m.id===id);if(!m)return false;m.enabled=v;return true}};
       BadlionUI.refresh();
    }""")
    assert page.locator('.module').count()==1
    assert page.locator('[data-filter="custom.hud"]').count()==1
    assert page.evaluate("BadlionUI.registerCategory({id:'bad',name:'bad'})") is False
    assert page.evaluate("BadlionUI.registerModule({id:'bad.test',name:'bad'})") is False
    page.locator('[data-id="dev.widget"]').click()
    assert page.evaluate('simulated.modules[0].enabled') is True
    page.evaluate('simulated.modules=[];simulated.categories=simulated.categories.slice(0,3);BadlionUI.refresh()')
    assert page.locator('[data-filter="custom.hud"]').count()==0
    assert page.locator('.module').count()==0
    assert not errors,errors
    page.close();b.close()
    print('PASS: native and JS dynamic categories, 3 removable template cards, filters, callbacks, cleanup')
