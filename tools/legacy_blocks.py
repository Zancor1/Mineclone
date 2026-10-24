"""Legacy block states shared by arena importers."""
import pathlib
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
shapes={'Cube':0,'Slab':1,'Stairs':2,'Fence':3,'Bars':4,'Carpet':5,'Torch':6,'Pot':7,'Plate':8,'Hopper':9,'Sign':10,'Portal':11,'Web':12,'Fire':13,'Barrier':14,'Wall':15,'Pane':16,'Trapdoor':17,'Door':18,'Gate':19,'Rail':20,'Vine':21,'Plant':22,'Anvil':23,'Skull':24,'Cocoa':25,'Hook':26,'PortalFrame':27,'Dispenser':28,'Jukebox':29,'RedstoneBlock':30,'Log':31,'Beacon':32,'Button':33}
def state(block,material=None,shape='Cube',data=0):return [ids[block],ids[material or block],shapes[shape],data]
def convert(t,m):
 if t==1:return state(['Stone','Granite','PolishedGranite','Diorite','PolishedDiorite','Andesite','PolishedAndesite'][m])
 if t==5:return state(planks[m])
 if t==17:return state('MapDecoration',['Wood','SpruceWood','BirchWood','JungleWood'][m&3],'Log',m)
 if t==162:return state('MapDecoration',['AcaciaWood','DarkOakWood'][m&1],'Log',m)
 if t==18:return state(['Leaves','SpruceLeaves','BirchLeaves','JungleLeaves'][m&3])
 if t==161:return state(['AcaciaLeaves','DarkOakLeaves'][m&1])
 if t==12:return state('RedSand' if m else 'Sand')
 if t==24:return state(['Sandstone','ChiseledSandstone','SmoothSandstone'][min(m,2)])
 if t in [8,9]:return state('Water')
 if t in [10,11]:return state('Lava')
 if t==6:return state('MapDecoration',['Leaves','SpruceLeaves','BirchLeaves','JungleLeaves','AcaciaLeaves','DarkOakLeaves'][min(m&7,5)],'Plant',m)
 if t in [31,32,38]:return state('MapDecoration','DeadBush' if t==32 else 'TallGrass' if t==31 else ['Rose','Orchid','Allium','Orchid','RedTulip','RedTulip','RedTulip','PinkTulip','Dandelion'][min(m,8)],'Plant',m)
 if t in [102,160]:return state('MapDecoration','Glass' if t==102 else wools[m].replace('Wool','StainedGlass'),'Pane',m)
 if t in [96,167]:return state('MapDecoration','BlockOfIron' if t==167 else 'Planks','Trapdoor',m)
 if t in [64,71,193,194,195,196,197]:return state('MapDecoration',{64:'Planks',71:'BlockOfIron',193:'SpruceWoodPlanks',194:'BirchWoodPlanks',195:'JungleWoodPlanks',196:'AcaciaPlanks',197:'DarkOakPlanks'}[t],'Door',m)
 if t in [107,183,184,185,186,187]:return state('MapDecoration',{107:'Planks',183:'SpruceWoodPlanks',184:'BirchWoodPlanks',185:'JungleWoodPlanks',186:'DarkOakPlanks',187:'AcaciaPlanks'}[t],'Gate',m)
 if t in [27,28,66,157]:return state('MapDecoration','BlockOfGold' if t in [27,28] else 'BlockOfIron','Rail',m)
 if t==106:return state('MapDecoration','Leaves','Vine',m)
 if t in [77,143]:return state('MapDecoration','Stone' if t==77 else 'Planks','Button',m)
 if t==121:return state('EndStone')
 if t==138:return state('MapDecoration','SeaLantern','Beacon',m)
 if t in [147,148]:return state('MapDecoration','BlockOfGold' if t==147 else 'BlockOfIron','Plate',m)
 if t==145:return state('MapDecoration','BlockOfIron','Anvil',m)
 if t==144:return state('MapDecoration','WhiteWool','Skull',m)
 if t==127:return state('MapDecoration','BrownWool','Cocoa',m)
 if t==131:return state('MapDecoration','BlockOfIron','Hook',m)
 if t==120:return state('MapDecoration','EndStone','PortalFrame',m)
 if t==23:return state('MapDecoration','Cobblestone','Dispenser',m)
 if t==84:return state('MapDecoration','Wood','Jukebox',m)
 if t==152:return state('MapDecoration','RedWool','RedstoneBlock',m)
 if t==168:return state(['Prismarine','PrismarineBricks','DarkPrismarine'][min(m,2)])
 if t==170:return state('MapDecoration','HayBale','Log',m)
 if t==35:return state(wools[m])
 if t==51:return state('MapDecoration','Lava','Fire',m)
 if t==166:return state('MapDecoration','Bedrock','Barrier',m)
 if t==26:return state('BedHead' if m&8 else 'Bed')
 if t in [43,44]:return state(slabs[m&7]) if t==43 else state('MapSlab',slabs[m&7],'Slab',m)
 if t in [125,126]:return state(planks[m&7]) if t==125 else state('MapSlab',planks[m&7],'Slab',m)
 if t==95:return state(wools[m].replace('Wool','StainedGlass'))
 if t==159:return state(wools[m].replace('Wool','StainedClay'))
 if t==98:return state(['StoneBricks','MossyStoneBricks','CrackedStoneBricks','CarvedStone'][m&3])
 if t==155:return state(['BlockOfQuartz','ChiseledQuartzBlock','PillarQuartzBlock'][min(2,m)])
 stairs={53:'Planks',108:'Bricks',114:'NetherBrick',136:'JungleWoodPlanks',163:'AcaciaPlanks',180:'RedSandstone',67:'Cobblestone',109:'StoneBricks',128:'Sandstone',134:'SpruceWoodPlanks',135:'BirchWoodPlanks',156:'BlockOfQuartz',164:'DarkOakPlanks'}
 if t in stairs:return state('MapStairs',stairs[t],'Stairs',m)
 if t==139:return state('MapFence','MossStone' if m else 'Cobblestone','Wall',m)
 if t in [85,113,188,189,190,191,192]:return state('MapFence',{85:'Planks',113:'NetherBrick',188:'SpruceWoodPlanks',189:'BirchWoodPlanks',190:'JungleWoodPlanks',191:'DarkOakPlanks',192:'AcaciaPlanks'}[t],'Fence',m)
 if t==101:return state('IronBars','BlockOfIron','Bars',m)
 if t==171:return state('MapCarpet',wools[m],'Carpet',m)
 deco={50:('Wood','Torch'),75:('Wood','Torch'),76:('LampOn','Torch'),90:('PurpleStainedGlass','Portal'),140:('Bricks','Pot'),55:('RedWool','Plate'),72:('Planks','Plate'),143:('Planks','Plate'),149:('Stone','Plate'),154:('BlockOfIron','Hopper'),118:('BlockOfIron','Hopper'),178:('Planks','Plate'),63:('Planks','Sign'),68:('Planks','Sign'),30:('WhiteWool','Web'),130:('Obsidian','Cube'),137:('CarvedStone','Cube')}
 if t in deco:return state('MapDecoration',*deco[t],m)
 simple={7:'Bedrock',13:'Gravel',22:'LapisLazuliBlock',41:'BlockOfGold',47:'Bookshelf',79:'Ice',80:'Snow',88:'SoulSand',89:'Glowstone',110:'Mycelium',112:'NetherBrick',124:'LampOn',148:'BlockOfIron',172:'HardenedClay',174:'PackedIce',42:'BlockOfIron',2:'Grass',3:'Dirt',4:'Cobblestone',20:'Glass',24:'Sandstone',45:'Bricks',49:'Obsidian',54:'Chest',57:'BlockOfDiamond',65:'Ladder',82:'Clay',97:'Stone',133:'BlockOfEmerald',152:'RedStainedClay',169:'SeaLantern',173:'BlockOfCoal'}
 if t in simple:return state(simple[t])
 raise ValueError('Unsupported block: '+str((t,m)))
