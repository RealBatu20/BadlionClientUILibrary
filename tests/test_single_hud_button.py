from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
host=(root/'src/AndroidAutoHost.cpp').read_text()
menu=(root/'src/BadlionMenu.cpp').read_text()
html=(root/'web/index.html').read_text()
for banned in ('g_floatWeb','floatingButtonHtml','g_hudButtonActivated','startup floating button='):
    assert banned not in host,banned
assert 'registerButton(button)' in menu
assert 'badlion_button_pressed.png' in menu
assert 'badlion_button_normal.png' in menu
assert 'fallbackFloat' not in html and 'id="squares"' not in html
with sync_playwright() as pw:
 b=pw.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 for vp in ({'width':1536,'height':814},{'width':960,'height':540},{'width':430,'height':860}):
  p=b.new_page(viewport=vp)
  errors=[];p.on('pageerror',lambda err:errors.append(str(err)))
  p.set_content(html)
  assert p.locator('.toolbar .tab').all_text_contents()==['Mods','Settings','Profiles']
  assert p.locator('#search').count()==1
  assert p.locator('#fallbackFloat').count()==0
  assert p.locator('#squares').count()==0
  assert p.locator('.module').count()==3
  p.locator('[data-tab="Settings"]').click()
  p.locator('#sizeSquare').click()
  assert p.locator('body').evaluate('b=>b.classList.contains("square-grid")')
  p.locator('#sizeStandard').click()
  assert not p.locator('body').evaluate('b=>b.classList.contains("square-grid")')
  p.locator('[data-tab="Mods"]').click()
  p.locator('#close').click()
  assert not p.locator('#panel').is_visible()
  p.evaluate('window.BadlionUI.setVisible(true)')
  assert p.locator('#panel').is_visible()
  assert not errors,errors
  p.close()
 b.close()
 print('PASS: single Levi button source; no Android/HTML duplicate; toolbar; settings sizing; reopen across viewports')
