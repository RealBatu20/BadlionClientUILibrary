from pathlib import Path
p=Path(__file__).resolve().parents[1]
a=(p/'src/AndroidAutoHost.cpp').read_text()
b=(p/'src/BadlionMenu.cpp').read_text()
assert 'enableLeviButtonOnAllScreens(e)' in a
assert 'external_button:badlionmenuui.launcher' in a
assert 'setOverlayShowEverywhere' in a and 'isOverlayShowEverywhere' in a
assert 'refreshRuntimeVisibility' in a
assert 'g_floatWeb' not in a and 'floatingButtonHtml' not in a
assert b.count('pl::modmenu::registerButton(button)') == 1
assert 'badlion_button_normal.png' in b and 'badlion_button_pressed.png' in b
assert (p/'manifest.json').read_text().find('"version": "1.1.6"') >= 0
print('PASS native Levi startup Show Everywhere, one HUD-editable button, 1.1.6 manifest')
