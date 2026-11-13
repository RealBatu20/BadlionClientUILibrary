from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
html=(ROOT/'web/index.html').read_text(encoding='utf-8')
with sync_playwright() as p:
 b=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox'])
 page=b.new_page(viewport={'width':1536,'height':814})
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html)
 assert page.locator('.module').count()==3
 page.locator('#search').fill('hypixel');assert page.locator('.module').count()==1
 page.locator('#search').fill('');assert page.locator('.module').count()==3
 button=page.locator('[data-id="badlion.template.pvp"]')
 assert button.get_attribute('aria-checked')=='false'
 button.click();assert page.locator('[data-id="badlion.template.pvp"]').get_attribute('aria-checked')=='true'
 page.locator('[data-tab="Settings"]').click();page.locator('#compact').click()
 assert page.locator('body').evaluate("e=>e.classList.contains('compact')")
 page.locator('[data-tab="Mods"]').click();assert page.locator('.module').count()==3
 page.set_viewport_size({'width':800,'height':360})
 page.wait_for_function("parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--scale'))>0")
 page.locator('#close').click();assert not page.locator('#panel').is_visible()
 page.evaluate('window.BadlionUI.setVisible(true)');assert page.locator('#panel').is_visible()
 assert not errors,errors
 b.close()
 print('PASS: three template cards, search, toggles, compact, scaling, close/reopen')
