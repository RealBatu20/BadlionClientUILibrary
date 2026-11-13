from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / 'web/index.html').read_text(encoding='utf-8')
with sync_playwright() as pw:
    browser=pw.chromium.launch(headless=True, executable_path='/usr/bin/chromium', args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1536,'height':814})
    errors=[]
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.set_content(html)
    assert page.locator('#panel').is_visible()
    page.evaluate("""() => {
       window.simulated = {visible:true,mode:'normal',opacity:95,modules:[
          {id:'test.switch',name:'Real callback',category:'Popular',enabled:false,placeholder:false,icon:'✓'}]};
       window.BadlionHost={snapshot:()=>JSON.stringify(simulated),setVisible:v=>simulated.visible=!!v,
           setEnabled:(id,v)=>{simulated.modules[0].enabled=v;return true},
           setOpacity:v=>simulated.opacity=v,setMode:v=>simulated.mode=v};
       window.BadlionUI.refresh();
    }""")
    assert page.locator('.module').count()==1
    # Close from JS (native state becomes false). This simulates v1.1.1 world entry.
    page.locator('#close').click()
    assert page.evaluate('window.simulated.visible') is False
    assert not page.locator('#panel').is_visible()
    # Native floating button changes native visibility while HTML remains hidden.
    page.evaluate('window.simulated.visible=true;')
    assert not page.locator('#panel').is_visible()
    # JNI evaluateJavascript now explicitly restores the panel on every show tick.
    page.evaluate('window.BadlionUI.hostVisibility(true);window.BadlionUI.refresh();')
    assert page.locator('#panel').is_visible()
    page.locator('[data-id="test.switch"]').click()
    assert page.evaluate('window.simulated.modules[0].enabled') is True
    page.evaluate('window.simulated.visible=false;window.BadlionUI.hostVisibility(false);')
    assert not page.locator('#panel').is_visible()
    page.evaluate('window.simulated.visible=true;window.BadlionUI.hostVisibility(true);')
    assert page.locator('#panel').is_visible()
    assert not errors,errors
    print('PASS: native button close/reopen and HTML visibility after simulated world transition')
    browser.close()
