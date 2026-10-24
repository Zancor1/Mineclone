#!/usr/bin/env python3
"""Import local legacy BedWars arenas and validate their service locations."""
import argparse,collections,json,math,pathlib,zlib
from legacy_world import chunks
from legacy_blocks import convert,shapes

ROOT=pathlib.Path(__file__).resolve().parents[1]
NON_SOLID={0,6,8,9,10,11,27,28,31,32,37,38,39,40,50,51,55,59,63,65,66,68,69,70,72,75,76,77,78,90,93,94,104,105,106,111,115,119,127,131,132,140,141,142,143,147,148,149,150,157,171}
NO_FLOOR=NON_SOLID|{26,44,53,54,64,67,71,85,96,101,102,107,109,113,114,118,120,126,128,130,134,135,136,139,144,145,146,154,156,160,163,164,166,167,180,183,184,185,186,187,188,189,190,191,192,193}
COLORS=[14,11,5,4,9,0,6,7]

def solid(value):return value[0] not in NON_SOLID

def stand(cells,x,z,near_y,radius=5,marker=None):
    best=None
    for xx in range(round(x)-radius,round(x)+radius+1):
        for zz in range(round(z)-radius,round(z)+radius+1):
            for y in range(max(0,round(near_y)-24),min(254,round(near_y)+30)):
                ground=cells.get((xx,y-1,zz),(0,0))
                if ground[0] in NO_FLOOR:continue
                if cells.get((xx,y,zz),(0,0))[0] or cells.get((xx,y+1,zz),(0,0))[0]:continue
                score=(xx-x)**2+(zz-z)**2+((y-near_y)*.25)**2
                if marker is not None and ground[0]==marker:score-=4
                candidate=(score,(xx,y,zz))
                if best is None or candidate<best:best=candidate
    if best is None:raise ValueError(f'No safe floor near {(x,near_y,z)}')
    return best[1]

def vendor_stand(cells,x,z,near_y,dx,dz):
    candidates=[]
    back=(round(dx),round(dz))
    if abs(dx)>abs(dz):back=(1 if dx>0 else -1,0)
    else:back=(0,1 if dz>0 else -1)
    for xx in range(round(x)-5,round(x)+6):
        for zz in range(round(z)-5,round(z)+6):
            for y in range(round(near_y)-1,round(near_y)+2):
                if cells.get((xx,y-1,zz),(0,0))[0] in NO_FLOOR:continue
                if any(cells.get((xx,y+h,zz),(0,0))[0] for h in (0,1)):continue
                # Keep an open approach and favor recessed counters with a back wall.
                if any(cells.get((xx-back[0],y+h,zz-back[1]),(0,0))[0] for h in (0,1)):continue
                walls=sum(solid(cells.get((xx+back[0]*d,y+1,zz+back[1]*d),(0,0))) for d in (1,2))
                roof=solid(cells.get((xx,y+3,zz),(0,0)))
                score=(xx-x)**2+(zz-z)**2+4*(y-near_y)**2-9*walls-3*roof
                candidates.append((score,(xx,y,zz)))
    return min(candidates)[1] if candidates else stand(cells,x,z,near_y,3)

def assign_colors(cells,beds):
    scores=[]
    for x,y,z,_ in beds:
        counter=collections.Counter()
        for (a,b,c),(t,m) in cells.items():
            if t in (35,159,95) and abs(a-x)<=10 and abs(c-z)<=10 and abs(b-y)<=5:counter[m]+=1
        scores.append([counter[c]+(counter[13] if c==5 else counter[3] if c==9 else 0) for c in COLORS])
    dp={0:(0,[])}
    for row in scores:
        following={}
        for mask,(score,path) in dp.items():
            for color in range(8):
                if mask>>color&1:continue
                key=mask|1<<color;value=(score+row[color],path+[color])
                if key not in following or value[0]>following[key][0]:following[key]=value
        dp=following
    path=dp[255][1];result=[None]*8
    for i,color in enumerate(path):result[color]=beds[i]
    return result

def centers(points,count):
    points=[(p[0],p[2]) for p in points]
    if not points:raise ValueError('Missing resource markers')
    selected=[min(points)]
    while len(selected)<count:selected.append(max(points,key=lambda p:min((p[0]-q[0])**2+(p[1]-q[1])**2 for q in selected)))
    for _ in range(30):
        groups=[[] for _ in selected]
        for p in points:groups[min(range(count),key=lambda i:(p[0]-selected[i][0])**2+(p[1]-selected[i][1])**2)].append(p)
        updated=[(sum(p[0] for p in g)/len(g),sum(p[1] for p in g)/len(g)) if g else selected[i] for i,g in enumerate(groups)]
        if updated==selected:break
        selected=updated
    return selected

def import_map(source):
    cells={};bed_feet=[];chests=[];villagers=[];resources={57:[],133:[]}
    for chunk in chunks(source):
        if not(-8<=chunk['xPos']<=7 and -8<=chunk['zPos']<=7):continue
        villagers.extend(e['Pos'] for e in chunk.get('Entities',[]) if e.get('id')=='Villager' and 'Pos' in e)
        for section in chunk.get('Sections',[]):
            for i,t in enumerate(section.get('Blocks',b'')):
                if not t:continue
                x=chunk['xPos']*16+i%16;y=section['Y']*16+i//256;z=chunk['zPos']*16+(i//16)%16
                if not(-124<=x<=124 and -124<=z<=124):continue
                m=(section['Data'][i//2]>>(4*(i%2)))&15;cells[x,y,z]=(t,m)
                if t==26 and not m&8 and math.hypot(x,z)>40:bed_feet.append((x,y,z,m))
                if t in (54,130):chests.append((x,y,z,t))
                if t in resources:resources[t].append((x,y,z))
    if len(bed_feet)!=8:raise ValueError(f'{source.name}: expected eight base beds, found {len(bed_feet)}')
    bounds=[min(p[i] for p in cells) for i in range(3)]+[max(p[i] for p in cells) for i in range(3)]
    offset_y=8-bounds[1]
    if bounds[4]+offset_y>178:raise ValueError(f'{source.name}: world height insufficient')
    def pos(p,center=True):return [p[0]+128+(.5 if center else 0),p[1]+offset_y,p[2]+128+(.5 if center else 0)]
    teams=[]
    for bed in assign_colors(cells,bed_feet):
        x,y,z,m=bed;normal=min((c for c in chests if c[3]==54),key=lambda c:(c[0]-x)**2+(c[2]-z)**2)
        ender=min((c for c in chests if c[3]==130),key=lambda c:(c[0]-x)**2+(c[2]-z)**2)
        if math.hypot(normal[0]-x,normal[2]-z)>25:raise ValueError('Base chest too far from bed')
        mx,mz=(normal[0]+ender[0])/2,(normal[2]+ender[2])/2;dx,dz=mx-x,mz-z;length=math.hypot(dx,dz);dx/=length;dz/=length
        forge=stand(cells,mx,mz,normal[1],4);spawn=stand(cells,mx-2*dx,mz-2*dz,forge[1],3)
        vendors=[vendor_stand(cells,mx+dx*2-dz*side,mz+dz*2+dx*side,forge[1],dx,dz) for side in [-3,3]]
        original=sorted((v for v in villagers if math.hypot(v[0]-x,v[2]-z)<20 and abs(v[1]-y)<4),key=lambda v:(v[0]-x)**2+(v[2]-z)**2)
        if len(original)>=2:
            vendors=[stand(cells,v[0]-.5,v[2]-.5,v[1],1) for v in original[:2]]
        if vendors[0]==vendors[1]:raise ValueError('Overlapping vendors')
        direction={0:(0,1),1:(-1,0),2:(0,-1),3:(1,0)}[m&3];head=(x+direction[0],y,z+direction[1])
        if cells.get(head,(0,0))[0]!=26:raise ValueError('Missing bed head')
        teams.append({'spawn':pos(spawn),'beds':[pos((x,y,z),False),pos(head,False)],'vendors':[pos(v) for v in vendors],'forge':pos(forge),'chest':pos(normal[:3])})
    stations={}
    for block in [57,133]:
        count=2 if block==133 and source.name in ['Dragonstar','Easter Basket','Lighthouse','RoofTop','Speedway','Zarzul'] else 4
        points=resources[block];near_y=sum(p[1] for p in bed_feet)/8
        exposed=[p for p in points if not cells.get((p[0],p[1]+1,p[2]),(0,0))[0] and not cells.get((p[0],p[1]+2,p[2]),(0,0))[0]]
        if not exposed:raise ValueError('No exposed resource marker')
        stations[block]=[pos((p[0],p[1]+1,p[2])) for x,z in centers(points,count) for p in [min(exposed,key=lambda p:(p[0]-x)**2+(p[2]-z)**2+.01*(p[1]-near_y)**2)]]
        if len({tuple(p) for p in stations[block]})!=count:raise ValueError('Duplicate resource generators')
    states=collections.Counter(cells.values());records=[]
    for (x,y,z),(t,m) in sorted(cells.items()):
        try:converted=convert(t,m)
        except Exception as error:raise ValueError(f'{source.name}: block {t}:{m} at {(x,y,z)}: {error}') from error
        records.append(bytes([x+128,y+offset_y,z+128]+converted))
    packed=zlib.compress(b''.join(records),9)
    report={'name':source.name,'cells':len(records),'compressed_bytes':len(packed),'bounds':bounds,'source_offset':[128,offset_y,128],'lobby_y':max(65,bounds[4]+offset_y+10),'teams':teams,'diamonds':stations[57],'emeralds':stations[133],'states':[{'id':t,'data':m,'count':v} for (t,m),v in sorted(states.items())]}
    return report,packed

def cpp_vector(p):return '{'+','.join(f'{v:.3f}f' for v in p)+'}'
def export(sources):
    reports=[];arrays=[];definitions=[]
    for index,source in enumerate(sources):
        report,packed=import_map(source);reports.append(report);symbol='ExternalBytes'+str(index)
        arrays.append('static const unsigned char '+symbol+'[]={\n'+''.join(','.join(str(v) for v in packed[i:i+32])+',\n' for i in range(0,len(packed),32))+'};\n')
        teams=[]
        for t in report['teams']:
            teams.append('{'+','.join([cpp_vector(t['spawn']),'{'+','.join(cpp_vector(v) for v in t['beds'])+'}','{'+','.join(cpp_vector(v) for v in t['vendors'])+'}',cpp_vector(t['forge']),cpp_vector(t['chest'])])+'}')
        definitions.append('{'+','.join([json.dumps(source.name),str(report['cells']),symbol,'sizeof('+symbol+')',str(report['lobby_y']),'{'+','.join(teams)+'}','{'+','.join(cpp_vector(v) for v in report['diamonds'])+'}','{'+','.join(cpp_vector(v) for v in report['emeralds'])+'}'])+'}')
        print(source.name,report['cells'],'blocks,',len(report['states']),'states, lobby',report['lobby_y'],flush=True)
    (ROOT/'include/external_maps.inc').write_text('// Generated by tools/import_bedwars_maps.py.\n'+''.join(arrays)+'static const ExternalArena ExternalArenas[]={\n'+',\n'.join(definitions)+'\n};\n')
    (ROOT/'data/external-maps.json').write_text(json.dumps(reports,indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory',type=pathlib.Path,nargs='?',default=ROOT/'mapas externos');args=parser.parse_args()
    export([p for p in sorted(args.directory.iterdir()) if (p/'level.dat').exists() and p.name not in ['Airshow','NC Bedwars 1v1']])
