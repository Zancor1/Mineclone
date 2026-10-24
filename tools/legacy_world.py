#!/usr/bin/env python3
"""Convert the legacy NC arena into an embedded, compressed block-state resource."""
import argparse
import struct,gzip,zlib,pathlib,collections,json
class NBT:
 def __init__(self,b):self.b=b;self.i=0
 def read(self,n):v=self.b[self.i:self.i+n];self.i+=n;return v
 def num(self,f):return struct.unpack('>'+f,self.read(struct.calcsize(f)))[0]
 def text(self):return self.read(self.num('H')).decode('utf8')
 def tag(self,t):
  if t in [1,2,3,4,5,6]:return self.num({1:'b',2:'h',3:'i',4:'q',5:'f',6:'d'}[t])
  if t==7:return self.read(self.num('i'))
  if t==8:return self.text()
  if t==9:
   typ=self.num('B');n=self.num('i');return [self.tag(typ) for _ in range(n)]
  if t==10:
   d={}
   while True:
    typ=self.num('B')
    if typ==0:return d
    name=self.text();d[name]=self.tag(typ)
  if t in [11,12]:return [self.num('i' if t==11 else 'Q') for _ in range(self.num('i'))]
  raise ValueError(t)
 def root(self):t=self.num('B');self.text();return self.tag(t)
def chunks(source):
 for file in sorted((source/'region').glob('*.mca')):
  b=file.read_bytes()
  for i in range(1024):
   off=int.from_bytes(b[i*4:i*4+3],'big')*4096
   if not off:continue
   length=int.from_bytes(b[off:off+4],'big');data=b[off+5:off+4+length];yield NBT(zlib.decompress(data)).root()['Level']

