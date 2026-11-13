#!/usr/bin/env python3
"""Package BadlionMenuUI, then enforce the 8-key and one-SO invariants."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import zipfile
import sys

ROOT=Path(__file__).resolve().parents[1]
DEST=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT/'dist'
DEST.mkdir(parents=True,exist_ok=True)
manifest=json.loads((ROOT/'manifest.json').read_text())
keys=['type','name','author','version','entry','minecraft_versions','overwrite_files','overwrite_folders']
assert list(manifest)==keys, list(manifest)
assert manifest['name']=='BadlionMenuUI'
assert manifest['author']=='TheRealBatu'
assert manifest['minecraft_versions']==['1.26.*','1.26.6*']
assert manifest['type']=='preload-native'
assert manifest['overwrite_files']==manifest['overwrite_folders']==[]
assert manifest['entry']=='libbadlionmenuui.so'

stem=f"lib{manifest['name']}_{manifest['version']}"
so=ROOT/'build/arm64-v8a'/manifest['entry']
assert so.is_file(), 'Build library before packaging'
blob=so.read_bytes()
assert blob[:4]==b'\x7fELF' and blob[4]==2 and struct.unpack_from('<H',blob,18)[0]==183
outso=DEST/(stem+'_arm64.so')
shutil.copyfile(so,outso)
assert outso.read_bytes()==blob

zpath=DEST/(stem+'.levipack')
resource_paths=[
 ('assets/badlion_logo.png','resources/badlion_logo.png'),
 ('assets/badlion_button_normal.png','resources/badlion_button_normal.png'),
 ('assets/badlion_button_pressed.png','resources/badlion_button_pressed.png'),
 ('assets/levi_template_aft.png','resources/levi_template_aft.png'),
 ('assets/levi_template_bfr.png','resources/levi_template_bfr.png'),
 ('web/index.html','resources/ui/index.html'),
 ('dex/classes.dex','resources/classes.dex'),
]
with zipfile.ZipFile(zpath,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9,allowZip64=True) as z:
 z.write(ROOT/'manifest.json','manifest.json')
 z.write(so,manifest['entry'])
 for src,target in resource_paths:z.write(ROOT/src,target)
 for p in sorted((ROOT/'docs').glob('*.md')):z.write(p,'docs/'+p.name)
 z.write(ROOT/'README.md','README.md')
with zipfile.ZipFile(zpath,'r') as z:
 assert z.testzip() is None,'CRC failure'
 names=z.namelist()
 assert len(names)==len(set(names)), 'Duplicate entries'
 assert all(not x.startswith('/') and '..' not in Path(x).parts for x in names)
 assert [x for x in names if x.endswith('.so')]==[manifest['entry']]
 assert json.loads(z.read('manifest.json'))==manifest
 assert z.read(manifest['entry'])==blob
 assert z.read('resources/ui/index.html').startswith(b'<!DOCTYPE html>')
 html=z.read('resources/ui/index.html').decode('utf8')
 assert all(f'badlion.template.{cat}' in html for cat in ('pvp','hypixel','visual'))
 assert 'categoryFilters' in html and 'registerCategory' in html
 assert '49 named demo modules' not in html
 dex=z.read('resources/classes.dex')
 assert dex[:8]==b'dex\n035\x00', 'DEX header'
 import zlib
 assert zlib.adler32(dex[12:]) & 0xffffffff == struct.unpack_from('<I',dex,8)[0], 'DEX checksum'
 assert hashlib.sha1(dex[32:]).digest() == dex[12:32], 'DEX SHA-1'
 assert all(z.getinfo(x).compress_type==zipfile.ZIP_DEFLATED for x in names)

source=DEST/(stem+'_src.zip')
with zipfile.ZipFile(source,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in sorted(ROOT.rglob('*')):
  if not p.is_file():continue
  rel=p.relative_to(ROOT)
  if rel.parts[0] in ('build','dist') or '__pycache__' in rel.parts or p.suffix in ('.so','.pyc'):continue
  z.write(p,'BadlionMenuUI/'+str(rel))
with zipfile.ZipFile(source,'r') as z:
 assert z.testzip() is None
 required=['BadlionMenuUI/CMakeLists.txt','BadlionMenuUI/tools/build.sh','BadlionMenuUI/tools/build_dex.sh','BadlionMenuUI/android/app/src/main/java/org/therealbatu/badlion/BadlionMenuBridge.java','BadlionMenuUI/src/BadlionMenu.cpp','BadlionMenuUI/include/BadlionUI.h','BadlionMenuUI/docs/DEVELOPER_API.md','BadlionMenuUI/docs/CUSTOM_CATEGORIES_AND_TEMPLATES.md','BadlionMenuUI/tests/test_category_templates.py','BadlionMenuUI/web/index.html','BadlionMenuUI/dex/classes.dex','BadlionMenuUI/src/AndroidAutoHost.cpp','BadlionMenuUI/tools/dex/generate_minimal_dex.py']
 assert all(f in z.namelist() for f in required)
 assert not any(f.endswith('.so') for f in z.namelist())

report={'manifest':manifest,'arm64_sha256':hashlib.sha256(blob).hexdigest(),
 'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [zpath,outso,source]},
 'package_entries':names,'source_file_count':len(zipfile.ZipFile(source).namelist()),
 'notes':['ARM64 compiled and ELF checked','ZIP CRC and one-so/manifest rules passed','3 real toggle templates / dynamically rendered category UI checked','Only native Levi editable HUD button; no second WebView launcher','Offline-generated JNI DEX bundled and integrity-checked','Automatic Activity bootstrap compiled; v1.1.6 unverified on physical device']}
(DEST/'BadlionMenuUI_build_report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
