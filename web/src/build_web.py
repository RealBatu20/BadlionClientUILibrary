#!/usr/bin/env python3
from pathlib import Path
import base64
ROOT=Path(__file__).resolve().parent
WEB=ROOT.parent
html=(ROOT/'index.tpl.html').read_text()
css=(ROOT/'menu.css').read_text()
js=(ROOT/'menu.js').read_text()
logo=base64.b64encode((WEB.parent/'assets/badlion_logo.png').read_bytes()).decode('ascii')
js=js.replace('__LOGO_B64__',logo)
html=html.replace('__INLINE_CSS__',css).replace('__INLINE_JS__',js.replace('</script>',r'<\/script>'))
(WEB/'index.html').write_text(html,encoding='utf-8')
print('Generated',WEB/'index.html',len(html),'bytes')
