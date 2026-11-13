#!/usr/bin/env python3
"""Offline verification of the exact user-uploaded ARM64 Minecraft ELF fingerprint.
This is a diagnostic only: it never patches or calls game code.
"""
from pathlib import Path
import hashlib, mmap, re, subprocess, sys
if len(sys.argv)!=2:
 raise SystemExit('Usage: python tools/check_signature.py /path/to/1.26.62.1/libminecraftpe.so')
file=Path(sys.argv[1]); source=Path(__file__).resolve().parents[1]/'src/SignatureProfile.hpp'
text=source.read_text()
pattern=re.search(r'kPauseMenuOpenIdaPattern\s*=\s*"([^"]+)"',text).group(1)
expected_id=re.search(r'kMinecraftBuildId\s*=\s*"([0-9a-f]+)"',text).group(1)
sha=hashlib.sha256(file.read_bytes()).hexdigest()
assert sha=='2a915cf07bd165c5f939b02b36ee756878a63c4489f4971fb14e6e7bdf39216b', 'Wrong game ELF hash; do not reuse this signature profile'
note=subprocess.run(['readelf','-n',str(file)],capture_output=True,text=True,check=True).stdout
assert expected_id in note, 'Build ID mismatch'
arr=[None if x in ('?','??') else int(x,16) for x in pattern.split()]
segments=[]; start=None
for i,v in enumerate(arr):
 if v is not None and start is None:start=i
 if v is None and start is not None:segments.append((start,bytes(arr[start:i])));start=None
if start is not None:segments.append((start,bytes(arr[start:])))
offset,anchor=max(segments,key=lambda x:len(x[1]))
with file.open('rb') as f,mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as m:
 matches=[]; pos=0
 while True:
  at=m.find(anchor,pos)
  if at==-1:break
  beginning=at-offset
  if beginning>=0 and beginning+len(arr)<=len(m) and all(v is None or m[beginning+j]==v for j,v in enumerate(arr)):
   matches.append(beginning)
  pos=at+1
assert len(matches)==1, 'Expected exactly one match, got '+str(len(matches))
print('PASS: 1.26.62.1 exact ELF SHA256, build ID, one wildcard signature at '+hex(matches[0]))
