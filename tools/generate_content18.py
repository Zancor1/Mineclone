import json,re
from pathlib import Path
# Run from the repository root. IDs in the generated files are persistent save data.
items=json.load(open('data/minecraft-1.8/items.json'));blocks=json.load(open('data/minecraft-1.8/blocks.json'));bd={b['id']:b for b in blocks}
old={(1,0):'Stone',(2,0):'Grass',(3,0):'Dirt',(4,0):'Cobblestone',(5,0):'Planks',(5,4):'AcaciaPlanks',(5,5):'DarkOakPlanks',(7,0):'Bedrock',(8,0):'Water',(9,0):'Water',(10,0):'Lava',(11,0):'Lava',(12,0):'Sand',(13,0):'Gravel',(15,0):'IronOre',(16,0):'CoalOre',(17,0):'Wood',(18,0):'Leaves',(18,1):'SpruceLeaves',(20,0):'Glass',(24,0):None,(31,1):'TallGrass',(32,0):'DeadBush',(37,0):'Dandelion',(38,0):'Rose',(38,1):'Orchid',(38,2):'Allium',(38,4):'RedTulip',(38,7):'PinkTulip',(45,0):'Bricks',(50,0):None,(54,0):'Chest',(58,0):'CraftingTable',(61,0):'Furnace',(79,0):None,(80,0):'Snow',(81,0):'Cactus',(82,0):'Clay',(98,0):'StoneBricks',(98,3):'CarvedStone',(123,0):'LampOff',(124,0):'LampOn',(165,0):'Slime',(263,0):'Coal',(265,0):'IronIngot',(268,0):'WoodenSword',(272,0):'StoneSword',(267,0):'IronSword',(270,0):'WoodenPickaxe',(274,0):'StonePickaxe',(257,0):'IronPickaxe',(280,0):'Stick'}
wools=['WhiteWool','OrangeWool','MagentaWool','LightBlueWool','YellowWool','LimeWool','PinkWool','GrayWool','LightGrayWool','CyanWool','PurpleWool','BlueWool','BrownWool','GreenWool','RedWool','BlackWool'];old.update({(35,i):v for i,v in enumerate(wools)})
old.update({(368, 0): 'EnderPearl', (385, 0): 'Fireball', (46, 0): 'Tnt', (322, 0): 'GoldenApple', (325, 0): 'Bucket', (326, 0): 'WaterBucket', (327, 0): 'LavaBucket', (261, 0): 'Bow', (262, 0): 'Arrow', (332, 0): 'Snowball', (344, 0): 'Egg', (259, 0): 'FlintAndSteel', (65, 0): 'Ladder', (355, 0): 'Bed', (287, 0): 'String', (289, 0): 'Gunpowder', (260, 0): 'Apple', (288, 0): 'Feather', (334, 0): 'Leather', (298, 0): 'LeatherHelmet', (299, 0): 'LeatherChestplate', (300, 0): 'LeatherLeggings', (301, 0): 'LeatherBoots', (302, 0): 'ChainmailHelmet', (303, 0): 'ChainmailChestplate', (304, 0): 'ChainmailLeggings', (305, 0): 'ChainmailBoots', (306, 0): 'IronHelmet', (307, 0): 'IronChestplate', (308, 0): 'IronLeggings', (309, 0): 'IronBoots', (310, 0): 'DiamondHelmet', (311, 0): 'DiamondChestplate', (312, 0): 'DiamondLeggings', (313, 0): 'DiamondBoots', (314, 0): 'GoldenHelmet', (315, 0): 'GoldenChestplate', (316, 0): 'GoldenLeggings', (317, 0): 'GoldenBoots'})
allowed={1,3,5,12,14,17,18,21,22,24,35,41,42,47,48,49,56,57,73,79,87,88,89,95,98,110,112,121,129,133,153,155,159,161,162,168,169,170,172,173,174,179}
materials={264,266,331,388,318,336,337,348,351,353,405,406} # dye variants are materials; dyeing recipe added below
extras=[]
colorwords={'white':0xe5e4dc,'orange':0xea7e28,'magenta':0xb74ab4,'light blue':0x65afd7,'yellow':0xe3bf2e,'lime':0x80be2d,'pink':0xde89a4,'gray':0x484c4f,'light gray':0xa3a9a9,'cyan':0x208f9a,'purple':0x7c3cae,'blue':0x344ea6,'brown':0x734e30,'green':0x408435,'red':0xb02b27,'black':0x1e1e23}
def color(name,id):
 n=name.lower();base=0x8c8b85
 if 'wood' in n or 'log' in n or 'plank' in n: base=0x98713e
 if 'sand' in n:base=0xdccc92
 if 'red sand' in n:base=0xb96535
 if 'dirt' in n or 'podzol' in n:base=0x816040
 if 'granite' in n:base=0xa87561
 if 'diorite' in n:base=0xc4c4bf
 if 'prismarine' in n:base=0x619e8b
 if 'nether' in n:base=0x713c39
 if 'obsidian' in n:base=0x30283d
 if 'ice' in n:base=0x98bfdf
 if 'end stone' in n:base=0xd9dba9
 if 'soul' in n:base=0x65503b
 if 'clay' in n:base=0x9b735b
 if 'leaves' in n:base=0x47763e
 if 'gold' in n:base=0xe4c64e
 if 'diamond' in n:base=0x58c8c7
 if 'emerald' in n:base=0x46b873
 if 'lapis' in n:base=0x3455ac
 if 'coal' in n:base=0x303234
 if 'redstone' in n:base=0xb0352a
 if 'quartz' in n:base=0xe3dfd4
 if 'glowstone' in n or 'lantern' in n:base=0xe4d697
 if 'glass' in n or 'clay' in n or 'wool' in n:
  for key in sorted(colorwords,key=len,reverse=True):
   if n.startswith(key):base=colorwords[key];break
 return base
for item in items:
 id=item['id'];name=item['name'];kind=1 if name.endswith('_sword') else 2 if name.endswith('_pickaxe') else 3 if name.endswith('_axe') else 4 if name.endswith('_shovel') else 5 if name.endswith('_hoe') else 0
 if not (id in allowed or id in materials or kind):continue
 for variant in item.get('variations',[{'metadata':0,'displayName':item['displayName']}]):
  meta=variant['metadata'];key=(id,meta)
  if old.get(key):continue
  display=variant['displayName'];token=''.join(x.capitalize() for x in re.findall('[A-Za-z0-9]+',display))
  # Use familiar tool spelling while keeping all generated names distinct.
  if kind:token=''.join(x.capitalize() for x in name.split('_'))
  token={337:'ClayBall',405:'NetherBrickItem',406:'Quartz',153:'NetherQuartzOre'}.get(id,token)
  if token in [x['token'] for x in extras]:token+=str(meta)
  b=bd.get(id,{});place=id<256;tier=1 if 'wooden' in name or 'golden' in name else 2 if 'stone_' in name else 3 if 'iron_' in name else 4 if 'diamond_' in name else 0
  material={'rock':1,'wood':2,'dirt':3,'wool':4,'plant':5}.get(b.get('material'),0)
  rgb=color(display,id);alpha=100 if 'glass' in name else 175 if name=='ice' else 255
  e=dict(id=id,meta=meta,token=token,name=display,place=place,solid=place,transparent=b.get('transparent',not place),stack=item['stackSize'],durability=item.get('maxDurability',0),kind=kind,tier=tier,material=material,light=min(24,round(b.get('emitLight',0)*24/15)),rgb=rgb,alpha=alpha,hardness=b.get('hardness',.5) or .5)
  extras.append(e);old[key]=token
assert 65+len(extras)<256,len(extras)
Path('include/content18_ids.inc').write_text(''.join(f"    {e['token']} = {65+i},\n" for i,e in enumerate(extras)))
Path('include/content18_properties.inc').write_text(''.join('    {'+json.dumps(e['name'])+f",{str(e['solid']).lower()},{str(e['transparent']).lower()},{str(e['place']).lower()},{e['stack']},{e['durability']},{e['kind']},{e['tier']},{e['material']},{e['light']},0x{e['rgb']:06x},{e['alpha']},{float(e['hardness'])}f"+'},\n' for e in extras))
Path('include/content18_colors.inc').write_text(''.join(f"    {{{e['rgb']>>16},{e['rgb']>>8&255},{e['rgb']&255},{e['alpha']}}},\n" for e in extras))
records=[]
for item in items:
 for var in item.get('variations',[{'metadata':0,'displayName':item['displayName']}]):
  if item['id']==17 and var['metadata']>3:continue
  key=(item['id'],var['metadata']);records.append(dict(id=item['id'],metadata=var['metadata'],name=item['name'],displayName=var['displayName'],stackSize=item['stackSize'],mineclone=old.get(key),kind='item'))
for b in blocks:
 if not any(r['id']==b['id'] for r in records):records.append(dict(id=b['id'],metadata=0,name=b['name'],displayName=b['displayName'],stackSize=b['stackSize'],mineclone=old.get((b['id'],0)),kind='world_block'))
Path('data/minecraft-1.8/catalog.json').write_text(json.dumps({'target':'Java 1.8.x / 1.8.9','source':'PrismarineJS/minecraft-data data/pc/1.8','sourceCommit':'1df9432c07c81b47326e4fb1e06d3c8d723bf4aa','entries':records},indent=2)+'\n')
Path('include/content18_catalog.inc').write_text(''.join('    {'+f"{r['id']},{r['metadata']},{json.dumps(r['displayName'])},{r['mineclone'] or 'Air'},{str(bool(r['mineclone'])).lower()}"+'},\n' for r in records))
Path('data/minecraft-1.8/added.json').write_text(json.dumps(extras,indent=2)+'\n')
print('New functional basic entries:',len(extras),'total:',65+len(extras),'reference catalogue:',len(records))
