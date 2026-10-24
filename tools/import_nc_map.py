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
def chunks():
 for file in sorted((source/'region').glob('*.mca')):
  b=file.read_bytes()
  for i in range(1024):
   off=int.from_bytes(b[i*4:i*4+3],'big')*4096
   if not off:continue
   length=int.from_bytes(b[off:off+4],'big');data=b[off+5:off+4+length];yield NBT(zlib.decompress(data)).root()['Level']

parser=argparse.ArgumentParser()
parser.add_argument('source',type=pathlib.Path)
parser.add_argument('--output',type=pathlib.Path,default=pathlib.Path('include/nc_map_data.inc'))
args=parser.parse_args();source=args.source
registry=pathlib.Path('include/block.h').read_text()
import re
ids={};value=0
# Preserve the registry order, including the generated ID lists.
registry=registry[registry.index('enum Block'):registry.index('struct BlockProperties')]
for name in ['content18_ids.inc','minigame_ids.inc']:
 registry=registry.replace('#include "'+name+'"',pathlib.Path('include/'+name).read_text())
body=registry[registry.index('{')+1:registry.index('}')]
for token in body.split(','):
 token=token.strip()
 if not token:continue
 if '=' in token:name,v=token.split('=');value=int(v.strip());name=name.strip()
 else:name=token
 ids[name]=value;value+=1
wools=['WhiteWool','OrangeWool','MagentaWool','LightBlueWool','YellowWool','LimeWool','PinkWool','GrayWool','LightGrayWool','CyanWool','PurpleWool','BlueWool','BrownWool','GreenWool','RedWool','BlackWool']
planks=['Planks','SpruceWoodPlanks','BirchWoodPlanks','JungleWoodPlanks','AcaciaPlanks','DarkOakPlanks']
slabs=['Stone','Sandstone','Planks','Cobblestone','Bricks','StoneBricks','NetherBrick','BlockOfQuartz']
shapes={'Cube':0,'Slab':1,'Stairs':2,'Fence':3,'Bars':4,'Carpet':5,'Torch':6,'Pot':7,'Plate':8,'Hopper':9,'Sign':10,'Portal':11,'Web':12}
def state(block,material=None,shape='Cube',data=0):return [ids[block],ids[material or block],shapes[shape],data]
def convert(t,m):
 if t==1:return state(['Stone','Granite','PolishedGranite','Diorite','PolishedDiorite','Andesite','PolishedAndesite'][m])
 if t==5:return state(planks[m])
 if t==17:return state(['Wood','SpruceWood','BirchWood','JungleWood'][m&3])
 if t==26:return state('BedHead' if m&8 else 'Bed')
 if t in [43,44]:return state(slabs[m&7]) if t==43 else state('MapSlab',slabs[m&7],'Slab',m)
 if t in [125,126]:return state(planks[m&7]) if t==125 else state('MapSlab',planks[m&7],'Slab',m)
 if t==95:return state(wools[m].replace('Wool','StainedGlass'))
 if t==159:return state(wools[m].replace('Wool','StainedClay'))
 if t==98:return state(['StoneBricks','MossyStoneBricks','CrackedStoneBricks','CarvedStone'][m&3])
 if t==155:return state(['BlockOfQuartz','ChiseledQuartzBlock','PillarQuartzBlock'][min(2,m)])
 stairs={109:'StoneBricks',128:'Sandstone',134:'SpruceWoodPlanks',135:'BirchWoodPlanks',156:'BlockOfQuartz',164:'DarkOakPlanks'}
 if t in stairs:return state('MapStairs',stairs[t],'Stairs',m)
 if t in [139,188,190]:return state('MapFence',{139:'Cobblestone',188:'SpruceWoodPlanks',190:'DarkOakPlanks'}[t],'Fence',m)
 if t==101:return state('IronBars','BlockOfIron','Bars',m)
 if t==171:return state('MapCarpet',wools[m],'Carpet',m)
 deco={50:('Wood','Torch'),75:('Wood','Torch'),76:('LampOn','Torch'),90:('PurpleStainedGlass','Portal'),140:('Bricks','Pot'),55:('RedWool','Plate'),72:('Planks','Plate'),143:('Planks','Plate'),149:('Stone','Plate'),154:('BlockOfIron','Hopper'),118:('BlockOfIron','Hopper'),178:('Planks','Plate'),63:('Planks','Sign'),68:('Planks','Sign'),30:('WhiteWool','Web'),130:('Obsidian','Cube'),137:('CarvedStone','Cube')}
 if t in deco:return state('MapDecoration',*deco[t],m)
 simple={42:'BlockOfIron',2:'Grass',3:'Dirt',4:'Cobblestone',20:'Glass',24:'Sandstone',45:'Bricks',49:'Obsidian',54:'Chest',57:'BlockOfDiamond',65:'Ladder',82:'Clay',97:'Stone',133:'BlockOfEmerald',152:'RedStainedClay',169:'SeaLantern',173:'BlockOfCoal'}
 if t in simple:return state(simple[t])
 raise ValueError('Unsupported block: '+str((t,m)))
records=[];counts=collections.Counter()
for chunk in chunks():
 cx,cz=chunk['xPos'],chunk['zPos']
 if not(-5<=cx<=5 and -4<=cz<=5):continue
 for section in chunk.get('Sections',[]):
  if section['Y']*16>=80:continue
  for i,t in enumerate(section['Blocks']):
   if not t or t==166:continue
   x=cx*16+i%16;z=cz*16+(i//16)%16;y=section['Y']*16+i//256
   if not(-75<=x<=75 and -64<=z<=80 and 5<=y<80):continue
   m=(section['Data'][i//2]>>(4*(i%2)))&15
   records.append(bytes([x+128,y-5,z+128]+convert(t,m)));counts[t,m]+=1
raw=b''.join(records);compressed=zlib.compress(raw,9)
args.output.write_text('// NC arena: embedded legacy block states, generated by tools/import_nc_map.py.\n'+f'constexpr unsigned int NcCellCount={len(records)};\nconstexpr unsigned char NcMapData[]={{\n'+''.join('    '+','.join(str(v) for v in compressed[i:i+24])+',\n' for i in range(0,len(compressed),24))+'};\n')
pathlib.Path('data').mkdir(exist_ok=True)
pathlib.Path('data/nc-import.json').write_text(json.dumps({'name':'NC','source':'NC Bedwars 1v1','cells':len(records),'compressed_bytes':len(compressed),'source_offset':[128,-5,128],'legacy_states':[{'id':k[0],'data':k[1],'count':v} for k,v in sorted(counts.items())]},indent=2)+'\n')
print(f'Imported {len(records)} blocks, {len(counts)} legacy states, {len(compressed)} compressed bytes')
