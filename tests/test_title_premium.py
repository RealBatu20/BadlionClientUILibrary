from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'web/index.html').read_text(encoding='utf-8')
assert 'data-tab="Premium"' not in src
assert '.tab.premium' not in src
assert 'Premium services' not in src
assert '#brandTitle' in src or 'id="brandTitle"' in src
assert "font-family:Impact,'Arial Narrow',sans-serif" in src
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 pg=b.new_page(viewport={'width':1500,'height':900})
 errors=[];pg.on('pageerror',lambda e:errors.append(str(e)))
 pg.set_content(src)
 assert pg.locator('.toolbar .tab').count()==3
 assert pg.locator('#brandTitle').inner_text()=='BADLION CLIENT'
 assert pg.evaluate("BadlionUI.setTitle('BATU CLIENT')") is True
 assert pg.locator('#brandTitle').inner_text()=='BATU CLIENT'
 assert pg.evaluate("BadlionUI.getTitle()")=='BATU CLIENT'
 pg.locator('[data-tab="Settings"]').click()
 assert pg.locator('#menuTitle').input_value()=='BATU CLIENT'
 pg.locator('#menuTitle').fill('REALBATU CLIENT')
 assert pg.locator('#brandTitle').inner_text()=='REALBATU CLIENT'
 pg.locator('#resetTitle').click()
 assert pg.locator('#brandTitle').inner_text()=='BADLION CLIENT'
 pg.evaluate("window.simulated={title:'DEVELOPER CLIENT',mode:'normal',visible:true,opacity:95,templatesVisible:true,categories:[],modules:[]};window.BadlionHost={snapshot:()=>JSON.stringify(window.simulated),setVisible:()=>{},setMode:()=>{},setOpacity:()=>{}};BadlionUI.refresh();")
 assert pg.locator('#brandTitle').inner_text()=='DEVELOPER CLIENT'
 pg.evaluate("BadlionUI.setTitle('LOCAL TITLE');window.simulated.title='UPDATED DEVELOPER TITLE';BadlionUI.refresh()")
 assert pg.locator('#brandTitle').inner_text()=='LOCAL TITLE'
 pg.evaluate('BadlionUI.resetTitle()')
 assert pg.locator('#brandTitle').inner_text()=='UPDATED DEVELOPER TITLE'
 assert not errors,errors
 pg.close()
 # static docs link integrity + responsive
 for viewport in ({'width':1300,'height':800},{'width':390,'height':850}):
  page=b.new_page(viewport=viewport)
  page.set_content((ROOT/'docs/index.html').read_text());page.add_script_tag(content=(ROOT/'docs/assets/site.js').read_text())
  assert page.locator('.side-link').count()==11
  if viewport['width']<800:
   page.locator('#mob').click();assert page.locator('body').evaluate("el=>el.classList.contains('nav-open')")
  page.locator('#docSearch').fill('title')
  assert page.locator('.side-link:visible').count()==1
  page.close()
 b.close()
print('PASS: Premium removed, title CSS preserved, JS and Settings title changes, native title priority, docs responsive')
