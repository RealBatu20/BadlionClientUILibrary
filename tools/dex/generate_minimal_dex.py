#!/usr/bin/env python3
"""Offline, deterministic DEX035 emitter for the project JNI-only Android bridge.
No android.jar, D8 or Android Gradle plugin needed. These classes have no bytecode
besides Object.<init>; their annotated JS bridge methods are native JNI callbacks.
"""
from __future__ import annotations
import hashlib, struct, zlib, pathlib, collections

ROOT = pathlib.Path(__file__).resolve().parents[2]

CLASSES = {
 'Lorg/therealbatu/badlion/HostApi;': dict(base='Ljava/lang/Object;', interfaces=[], methods=[
  ('<init>', 'V', (), 'ctor'), ('snapshot', 'Ljava/lang/String;', (), 'bridge'),
  ('setEnabled', 'Z', ('Ljava/lang/String;', 'Z'), 'bridge'),
  ('setVisible', 'V', ('Z',), 'bridge'),
  ('setMode', 'V', ('Ljava/lang/String;',), 'bridge'),
  ('setOpacity', 'V', ('I',), 'bridge')]),
 'Lorg/therealbatu/badlion/UiTask;': dict(base='Ljava/lang/Object;', interfaces=['Ljava/lang/Runnable;'], methods=[
  ('<init>','V',(),'ctor'), ('run','V',(), 'native')]),
 'Lorg/therealbatu/badlion/ActivityObserver;': dict(base='Ljava/lang/Object;', interfaces=['Landroid/app/Application$ActivityLifecycleCallbacks;'], methods=[
  ('<init>','V',(),'ctor'),
  ('onActivityCreated','V',('Landroid/app/Activity;','Landroid/os/Bundle;'),'native'),
  ('onActivityStarted','V',('Landroid/app/Activity;',),'native'),
  ('onActivityResumed','V',('Landroid/app/Activity;',),'native'),
  ('onActivityPaused','V',('Landroid/app/Activity;',),'native'),
  ('onActivityStopped','V',('Landroid/app/Activity;',),'native'),
  ('onActivitySaveInstanceState','V',('Landroid/app/Activity;','Landroid/os/Bundle;'),'native'),
  ('onActivityDestroyed','V',('Landroid/app/Activity;',),'native')]),
}
ANNOTATION = 'Landroid/webkit/JavascriptInterface;'

def shorty(t): return 'L' if t.startswith('L') or t.startswith('[') else t

def uleb(n):
 assert n>=0
 out=bytearray()
 while True:
  b=n&127; n>>=7
  out.append(b|0x80 if n else b)
  if not n: return bytes(out)

def pack(fmt,*xs):return struct.pack('<'+fmt,*xs)
class Buffer:
 def __init__(self,seed=b''):self.b=bytearray(seed)
 def align(self,n=4):self.b.extend(b'\x00'*((-len(self.b))%n))
 def add(self,src):
  at=len(self.b);self.b.extend(src);return at
 def set(self,off,fmt,*values):struct.pack_into('<'+fmt,self.b,off,*values)
 def pos(self):return len(self.b)

strings={ANNOTATION,'<init>','V','I','Z','Ljava/lang/Object;'}
types={ANNOTATION,'V','I','Z','Ljava/lang/Object;'}
protos=set()
for c,entry in CLASSES.items():
 types.add(c);types.add(entry['base']);types.update(entry['interfaces'])
 for name,ret,args,kind in entry['methods']:
  strings.add(name);types.add(ret);types.update(args);protos.add((ret,args))
for tp in types:strings.add(tp)
for ret,args in protos:strings.add(shorty(ret)+''.join(shorty(x) for x in args))
strings=sorted(strings); sidx={s:i for i,s in enumerate(strings)}
types=sorted(types,key=sidx.__getitem__); tidx={t:i for i,t in enumerate(types)}
protos=sorted(protos,key=lambda p:(tidx[p[0]],tuple(tidx[x] for x in p[1])))
pidx={p:i for i,p in enumerate(protos)}
methods=[('Ljava/lang/Object;', '<init>', 'V', (), 'external')]
for c,entry in CLASSES.items():
 for name,ret,args,kind in entry['methods']:
  methods.append((c,name,ret,args,kind))
methods.sort(key=lambda m:(tidx[m[0]],sidx[m[1]],pidx[m[2],m[3]]))
midx={(c,name,ret,args):i for i,(c,name,ret,args,kind) in enumerate(methods)}
classes=sorted(CLASSES,key=tidx.__getitem__)

h=Buffer(b'\x00'*112)
sections={}
def reserve(typekey,count,width):
 h.align();start=h.pos();h.add(b'\x00'*count*width);sections[typekey]=(count,start);return start
str_off=reserve(0x0001,len(strings),4)
type_off=reserve(0x0002,len(types),4)
proto_off=reserve(0x0003,len(protos),12)
method_off=reserve(0x0005,len(methods),8)
class_off=reserve(0x0006,len(classes),32)
data_off=h.pos()
# string_data_item, encoded as minimal MUTF-8 (all identifiers here are ASCII)
for i,s in enumerate(strings):
 assert s.isascii();off=h.add(uleb(len(s))+s.encode()+b'\0');h.set(str_off+4*i,'I',off)
sections[0x2002]=(len(strings),data_off)

def emit_type_list(items):
 if not items:return 0
 h.align();off=h.pos();h.add(pack('I',len(items))+b''.join(pack('H',tidx[t]) for t in items));h.align();return off
lists={}
for ret,args in protos:
 if args and args not in lists:lists[args]=emit_type_list(args)
for c in classes:
 items=tuple(CLASSES[c]['interfaces'])
 if items and items not in lists:lists[items]=emit_type_list(items)
if lists:sections[0x1001]=(len(lists),min(lists.values()))
# Constructor bytecode: invoke-direct {v0}, java.lang.Object.<init>(); return-void
obj_ctor=midx.get(('Ljava/lang/Object;','<init>','V',()))
assert obj_ctor is not None
h.align()
code_offsets={}
first_code=h.pos()
for c in classes:
 h.align(); off=h.pos();code_offsets[c]=off
 h.add(pack('HHHHII',1,1,1,0,0,4)+pack('HHHH',0x1070,obj_ctor,0,0x000e))
sections[0x2001]=(len(code_offsets),first_code)
# A runtime-visible, zero-parameter @JavascriptInterface marker
ann_off=h.add(bytes((1,))+uleb(tidx[ANNOTATION])+uleb(0))
sections[0x2004]=(1,ann_off)
h.align(); ann_set_off=h.add(pack('II',1,ann_off));sections[0x1003]=(1,ann_set_off)
# HostApi annotated_methods[] entries sorted by method_idx
annotated=sorted(midx[(c,name,ret,args)] for c,entry in CLASSES.items() for name,ret,args,kind in entry['methods'] if kind=='bridge')
h.align(); dir_off=h.add(pack('IIII',0,0,len(annotated),0)+b''.join(pack('II',i,ann_set_off) for i in annotated))
sections[0x2006]=(1,dir_off)
# emit class_data items: methods index deltas reset for direct/virtual groups
class_data_offsets={}
for c in classes:
 ms=[(midx[(c,name,ret,args)],kind) for name,ret,args,kind in CLASSES[c]['methods']]
 direct=sorted((i,k) for i,k in ms if k=='ctor')
 virtual=sorted((i,k) for i,k in ms if k!='ctor')
 data=bytearray(uleb(0)+uleb(0)+uleb(len(direct))+uleb(len(virtual)))
 for group in (direct,virtual):
  prev=0
  for idx,kind in group:
   flags=0x10001 if kind=='ctor' else 0x101
   data.extend(uleb(idx-prev)+uleb(flags)+uleb(code_offsets[c] if kind=='ctor' else 0))
   prev=idx
 class_data_offsets[c]=h.add(data)
sections[0x2000]=(len(class_data_offsets),min(class_data_offsets.values()))
# Fill the fixed-size index arrays.
for i,t in enumerate(types):h.set(type_off+4*i,'I',sidx[t])
for i,(ret,args) in enumerate(protos):
 short=shorty(ret)+''.join(shorty(x) for x in args)
 h.set(proto_off+i*12,'III',sidx[short],tidx[ret],lists.get(args,0))
for i,(c,name,ret,args,kind) in enumerate(methods):
 h.set(method_off+8*i,'HHI',tidx[c],pidx[(ret,args)],sidx[name])
for i,c in enumerate(classes):
 entry=CLASSES[c]
 h.set(class_off+32*i,'IIIIIIII',tidx[c],1,tidx[entry['base']],lists.get(tuple(entry['interfaces']),0),0xffffffff,dir_off if c.endswith('/HostApi;') else 0,class_data_offsets[c],0)
# map list. All offsets refer to first item of a given section.
h.align();map_off=h.pos()
mapsections={0x0000:(1,0), 0x1000:(1,map_off), **sections}
mapitems=sorted(mapsections.items(),key=lambda x:x[1][1])
h.add(pack('I',len(mapitems)))
for tag,(count,off) in mapitems: h.add(pack('HHII',tag,0,count,off))
# DEX035 header
h.set(0,'8s',b'dex\n035\x00')
h.set(32,'IIIIIIIIIIIIIIIIIIII',h.pos(),112,0x12345678,0,0,map_off,
 len(strings),str_off,len(types),type_off,len(protos),proto_off,0,0,
 len(methods),method_off,len(classes),class_off,h.pos()-data_off,data_off)
h.b[12:32]=hashlib.sha1(h.b[32:]).digest()
h.set(8,'I',zlib.adler32(h.b[12:]) & 0xffffffff)
out=ROOT/'dex'/'classes.dex';out.parent.mkdir(exist_ok=True)
out.write_bytes(h.b)
print('Generated',out, 'bytes:',len(h.b),'classes:',len(classes),'methods:',len(methods),'map entries:',len(mapitems))
for c in classes: print(' -',c)
assert h.b[:8]==b'dex\n035\x00'
assert zlib.adler32(h.b[12:]) & 0xffffffff==struct.unpack_from('<I',h.b,8)[0]
assert hashlib.sha1(h.b[32:]).digest()==h.b[12:32]

