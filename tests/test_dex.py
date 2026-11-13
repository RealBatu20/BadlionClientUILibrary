"""Validate the generated minimal JNI DEX's binary layout and annotations."""
from pathlib import Path
import struct, hashlib, zlib
p=Path(__file__).resolve().parents[1]/'dex/classes.dex'
b=p.read_bytes()
assert b[:8]==b'dex\n035\x00'
u16=lambda o:struct.unpack_from('<H',b,o)[0]
u32=lambda o:struct.unpack_from('<I',b,o)[0]
assert u32(8)==zlib.adler32(b[12:])&0xffffffff
assert b[12:32]==hashlib.sha1(b[32:]).digest()
assert u32(32)==len(b) and u32(36)==112
nstr,sstr,ntyp,styp,nproto,sproto,nfields,sfields,nmeth,smeth,ncls,scls,szdat,odat=struct.unpack_from('<14I',b,56)
assert ncls==3 and nmeth==17 and nfields==0

def uleb(pos):
 val=0;shift=0
 while True:
  x=b[pos];pos+=1;val|=(x&127)<<shift;shift+=7
  if not x&128:return val,pos

strings=[]
for i in range(nstr):
 off=u32(sstr+4*i);length,loc=uleb(off);end=b.index(0,loc)
 val=b[loc:end].decode('ascii');assert len(val)==length
 strings.append(val)
types=[strings[u32(styp+4*i)] for i in range(ntyp)]
assert 'Landroid/webkit/JavascriptInterface;' in types
method_ids=[]
for i in range(nmeth):
 cls,proto,name=struct.unpack_from('<HHI',b,smeth+i*8)
 method_ids.append((types[cls],strings[name]))
assert ('Lorg/therealbatu/badlion/HostApi;','snapshot') in method_ids
assert ('Lorg/therealbatu/badlion/UiTask;','run') in method_ids
assert ('Lorg/therealbatu/badlion/ActivityObserver;','onActivityResumed') in method_ids
native={};constructors=0;annotations=[]
for i in range(ncls):
 cls,flags,supercls,interfaces,src,ann,dat,static=struct.unpack_from('<8I',b,scls+i*32)
 name=types[cls]
 assert flags&1 and src==0xffffffff and dat>odat and static==0
 pos=dat;stat,pos=uleb(pos);inst,pos=uleb(pos);direct,pos=uleb(pos);virtual,pos=uleb(pos)
 assert stat==inst==0 and direct==1
 for count in (direct,virtual):
  delta=0
  for _ in range(count):
   d,pos=uleb(pos);delta+=d;access,pos=uleb(pos);code,pos=uleb(pos)
   assert method_ids[delta][0]==name
   m=method_ids[delta][1]
   if m=='<init>':
    constructors+=1;assert access==0x10001 and code
    assert u16(code+16)==0x1070 and u16(code+22)==0x000e
   else:
    assert access==0x101 and code==0
    native[name+'.'+m]=delta
 if name.endswith('/HostApi;'):
  assert ann!=0
  clsa,nfields,nmethods,nparams=struct.unpack_from('<4I',b,ann)
  assert clsa==nfields==nparams==0 and nmethods==5
  for j in range(nmethods):
   method_idx,annset=struct.unpack_from('<II',b,ann+16+j*8)
   annotations.append(method_ids[method_idx][1]);assert u32(annset)==1
   annoff=u32(annset+4)
   assert b[annoff]==1 # runtime visibility
   tidx,pos=uleb(annoff+1)
   assert types[tidx]=='Landroid/webkit/JavascriptInterface;'
   count,pos=uleb(pos);assert count==0
assert constructors==3
assert set(annotations)=={'snapshot','setEnabled','setVisible','setMode','setOpacity'}
assert len(native)==13
print('PASS: DEX035 SHA-1/Adler32, 3 classes, all 17 method IDs, 13 JNI methods, 5 runtime @JavascriptInterface annotations')
