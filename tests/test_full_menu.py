from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
html=(ROOT/'web/index.html').read_text(encoding='utf-8')
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for (width,height) in [(1536,814),(960,540),(430,860),(320,568),(2560,1440)]:
  page=b.new_page(viewport={'width':width,'height':height},device_scale_factor=1)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.set_content(html)
  assert page.locator('.module').count()==3
  panel=page.locator('#panel').bounding_box()
  assert panel and panel['x']>=-1 and panel['y']>=-1 and panel['x']+panel['width']<=width+1 and panel['y']+panel['height']<=height+1,(width,height,panel)
  if width==1536:
   page.screenshot(path=str(ROOT/'previews'/'Normal_Mode_Preview_v1.1.5.png'))
   page.locator('[data-module="badlion.template.visual"]').click()
   assert page.locator('#detail').is_visible()
   assert 'badlion.template.visual' in page.locator('#detail').inner_text()
   page.locator('#detailToggle').click()
   assert page.locator('[data-id="badlion.template.visual"]').get_attribute('aria-checked')=='true'
   page.locator('#detailClose').click()
   page.locator('[data-tab="Profiles"]').click()
   page.locator('#newProfile').fill('PvP Practice');page.locator('#createProfile').click()
   assert page.locator('#profile').input_value()=='PvP Practice'
   page.locator('#saveProfile').click();page.locator('#loadProfile').click();page.locator('#deleteProfile').click()
   assert page.locator('#profile').input_value()=='Default'
   page.locator('[data-tab="Settings"]').click()
   page.locator('#sizeSquare').click();assert page.locator('body').evaluate("e=>e.classList.contains('square-grid')")
   page.locator('#sizeStandard').click();page.locator('#uiScale').fill('85');assert page.locator('#scaleValue').inner_text()=='85%'
   page.locator('#normal').click();page.locator('#editPosition').click()
   assert page.locator('body').evaluate("e=>e.classList.contains('editing')")
   page.locator('#resetPosition').click();assert not page.locator('body').evaluate("e=>e.classList.contains('editing')")
  if width==960:
   page.locator('[data-tab="Settings"]').click();page.locator('#compact').click();page.locator('[data-tab="Mods"]').click()
   assert page.locator('.module').count()==3
   page.screenshot(path=str(ROOT/'previews'/'Compact_Mode_Preview_v1.1.5.png'))
  if width==430:
   page.screenshot(path=str(ROOT/'previews'/'Portrait_Mode_Preview_v1.1.5.png'))
  assert not errors,(width,height,errors)
  page.close()
 b.close()
 print('PASS: 3 templates, profile lifecycle, settings, sizing, normal/compact/portrait previews')
