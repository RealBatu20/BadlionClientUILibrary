from pathlib import Path
from PIL import Image
p=Path(__file__).resolve().parents[1]
s=(p/'src/BadlionMenu.cpp').read_text()
assert s.count('pl::modmenu::registerButton(button)')==1
assert 'button.normalBgColor=0x01000000u' in s
assert 'button.activeBgColor=0x01000000u' in s
assert 'button.borderColor=0x01000000u' in s
assert 'button.iconData=readBytes(g_modDir/"resources"/"badlion_button_normal.png")' in s
assert 'button.activeIconData=readBytes(g_modDir/"resources"/"badlion_button_pressed.png")' in s
for nm in ('badlion_button_normal.png','badlion_button_pressed.png'):
  im=Image.open(p/'assets'/nm).convert('RGBA')
  assert im.size==(512,512) and im.getpixel((0,0))[3]==0
assert 'setOverlayShowEverywhere' in (p/'src/AndroidAutoHost.cpp').read_text()
print('PASS one native Levi HUD button: transparent wrapper, original PNGs and startup visibility intact')
