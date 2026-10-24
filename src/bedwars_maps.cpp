#include "bedwars_maps.h"
#include "world.h"
#include "map_blocks.h"
#include "external_arenas.h"
#include <zlib.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <vector>
namespace game::bw {
namespace {
#include "nc_map_data.inc"
#include "airshow_map_data.inc"
const std::vector<unsigned char>& NcCells(){static const auto cells=[](){std::vector<unsigned char> result(NcCellCount*7);uLongf length=result.size();if(uncompress(result.data(),&length,NcMapData,sizeof(NcMapData))!=Z_OK||length!=result.size())throw std::runtime_error("Mapa NC corrompido");return result;}();return cells;}
const std::vector<unsigned char>& AirshowCells(){static const auto cells=[](){std::vector<unsigned char> result(AirshowCellCount*7);uLongf length=result.size();if(uncompress(result.data(),&length,AirshowMapData,sizeof(AirshowMapData))!=Z_OK||length!=result.size())throw std::runtime_error("Mapa Airshow corrompido");return result;}();return cells;}
constexpr int AirBeds[8][4]={{-40,63,-76,2},{40,67,-76,2},{76,67,-40,3},{76,63,40,3},{40,63,76,0},{-40,67,76,0},{-76,67,40,1},{-76,63,-40,1}};
Vector3 AirPoint(int team,float back,float side=0){auto b=AirBeds[std::clamp(team,0,7)];const int dx[]={0,-1,0,1},dz[]={1,0,-1,0};int d=b[3];return {b[0]+128.5f+dx[d]*back-dz[d]*side,b[1]-20.f,b[2]+128.5f+dz[d]*back+dx[d]*side};}

}
int MapTeams(int map){return map==3?2:8;}
int MapLobbyY(int map){return map>=FirstExternalArena?ExternalArenaAt(map).lobbyY:map==4?115:65;}
std::array<Vector3,2> MapBeds(int map,int team,const Vector3& base){if(map>=FirstExternalArena){auto& teamData=ExternalArenaAt(map).teams[std::clamp(team,0,7)];return {{teamData.beds[0],teamData.beds[1]}};}if(map==4){auto p=AirPoint(team,0),q=AirPoint(team,1);return {{{std::floor(p.x),p.y,std::floor(p.z)},{std::floor(q.x),q.y,std::floor(q.z)}}};}if(map==3)return team==0?std::array<Vector3,2>{{{86,26,156},{85,26,156}}}:std::array<Vector3,2>{{{170,26,156},{171,26,156}}};return {{{float(int(base.x)),26,float(int(base.z)+3)},{float(int(base.x)),26,float(int(base.z)+4)}}};}

const char* MapName(int map){if(map>=FirstExternalArena)return ExternalArenaAt(map).name;constexpr const char* names[]={"Fortaleza","Ruinas","Jardins","NC","Airshow"};return names[std::clamp(map,0,MapCount-1)];}
const char* MapDescription(int map){if(map>=FirstExternalArena)return "Arena original importada - 8 times";constexpr const char* descriptions[]={"Torres e muralhas nas ilhas","Colinas de pedra e arcos antigos","Terracos, arvores e pavilhoes","Arena original importada - 2 times","Baloes e avioes - 8 times"};return descriptions[std::clamp(map,0,MapCount-1)];}
std::array<Vector3,8> ArenaBases(int gap,int map){
    if(map>=FirstExternalArena){std::array<Vector3,8> result{};for(int i=0;i<8;++i)result[i]=ExternalArenaAt(map).teams[i].spawn;return result;}
    if(map==3)return {{{75.5f,26,156.5f},{181.5f,26,156.5f},{128.5f,26,128.5f},{128.5f,26,128.5f}}};
    if(map==4){std::array<Vector3,8> result{};for(int i=0;i<8;++i)result[i]=AirPoint(i,6);return result;}
    int radius=gap+30;
    return {{{128.5f,26,128.5f-radius},{128.5f+radius,26,128.5f},{128.5f,26,128.5f+radius},{128.5f-radius,26,128.5f},{128.5f+radius,26,128.5f-radius},{128.5f+radius,26,128.5f+radius},{128.5f-radius,26,128.5f+radius},{128.5f-radius,26,128.5f-radius}}};
}
std::array<Vector3,8> DiamondIslands(int gap,int map){
    auto bases=ArenaBases(gap,map);std::array<Vector3,8> positions{};
    constexpr int ring[]={0,4,1,5,2,6,3,7};
    for(int i=0;i<8;++i){auto a=bases[ring[i]],b=bases[ring[(i+1)%8]];positions[i]={float(int((a.x+b.x)/2))+.5f,26,float(int((a.z+b.z)/2))+.5f};}
    return positions;
}
Vector3 RuinsVendor(int team,int kind,int gap){auto base=ArenaBases(gap,1)[std::clamp(team,0,7)];return {base.x+(kind?8.f:-8.f),26,base.z+5};}
Vector3 AirshowSpawn(int team){return AirPoint(team,6);}
Vector3 AirshowVendor(int team,int kind){return AirPoint(team,8,kind?3.f:-3.f);}
Vector3 AirshowForge(int team){auto p=AirPoint(team,10.5f);p.y-=.5f;return p;}
Vector3 AirshowChest(int team){constexpr int positions[8][3]={{-36,63,-81},{44,67,-81},{81,67,-36},{81,63,44},{36,63,81},{-44,67,81},{-81,67,36},{-81,63,-44}};auto p=positions[std::clamp(team,0,7)];return {p[0]+128.5f,p[1]-20.f,p[2]+128.5f};}
std::vector<Vector3> AirshowResources(bool emerald){std::vector<Vector3> result;int radius=emerald?12:40;for(int x:{-1,1})for(int z:{-1,1}){bool high=x!=z;result.push_back({128.5f+x*radius,float(high?53:49),128.5f+z*radius});}return result;}
void BuildArenaTerrain(int map,int gap,const std::function<void(int,int,int,Block)>& put,bool lobby){
    if(map>=FirstExternalArena){const auto& cells=ExternalCells(map);for(size_t i=0;i<cells.size();i+=7){int x=cells[i],y=cells[i+1],z=cells[i+2];Block b=Block(cells[i+3]);put(x,y,z,b);if(lobby&&b>=MapSlab)SetMapBlockState(x,y,z,{b,Block(cells[i+4]),MapShape(cells[i+5]),cells[i+6]});}
        int floor=MapLobbyY(map);if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,floor,z,Glass);if(x==119||x==137||z==119||z==137){put(x,floor+1,z,Glass);put(x,floor+2,z,Glass);}}return;
    }
    if(map==4){const auto& cells=AirshowCells();for(size_t i=0;i<cells.size();i+=7){int x=cells[i],y=cells[i+1],z=cells[i+2];Block b=Block(cells[i+3]);put(x,y,z,b);if(lobby&&b>=MapSlab)SetMapBlockState(x,y,z,{b,Block(cells[i+4]),MapShape(cells[i+5]),cells[i+6]});}
        if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,115,z,Glass);if(x==119||x==137||z==119||z==137){put(x,116,z,Glass);put(x,117,z,Glass);}}
        return;
    }
    if(map==3){const auto& cells=NcCells();for(size_t i=0;i<cells.size();i+=7){int x=cells[i],y=cells[i+1],z=cells[i+2];Block block=Block(cells[i+3]);put(x,y,z,block);if(lobby&&block>=MapSlab)SetMapBlockState(x,y,z,{block,Block(cells[i+4]),MapShape(cells[i+5]),cells[i+6]});}
        for(int team=0;team<2;++team){auto beds=MapBeds(map,team,{});for(int i=0;i<2;++i)put(int(beds[i].x),int(beds[i].y),int(beds[i].z),i?BedHead:Bed);}
        if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,65,z,Glass);if(x==119||x==137||z==119||z==137){put(x,66,z,Glass);put(x,67,z,Glass);}}
        return;
    }
    const auto bases=ArenaBases(gap,map);constexpr Block wool[]={RedWool,BlueWool,GreenWool,YellowWool,CyanWool,WhiteWool,PinkWool,GrayWool};
    auto island=[&](int cx,int cz,int radius,Block top,int style){
        for(int dx=-radius;dx<=radius;++dx)for(int dz=-radius;dz<=radius;++dz){
            float d=std::hypot(float(dx),float(dz));bool inside=false;
            if(style==0)inside=std::max(std::abs(dx),std::abs(dz))<=radius-1&&std::abs(dx)+std::abs(dz)<=radius+radius/2;
            else if(style==1)inside=d<=radius-1+.8f*std::sin(dx*.8f)+.7f*std::cos(dz*.9f);
            else inside=std::pow(std::abs(float(dx))/radius,4)+std::pow(std::abs(float(dz))/radius,4)<=1;
            if(!inside)continue;
            int depth=std::max(3,int((radius-d)*.8f)+4);
            for(int y=25-depth;y<=25;++y)put(cx+dx,y,cz+dz,y==25?top:y>=23&&style==2?Dirt:Stone);
        }
    };
    auto tree=[&](int x,int z,bool blossom){
        for(int y=26;y<=31;++y)put(x,y,z,Wood);
        for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz)for(int dy=0;dy<4;++dy)if(dx*dx+dz*dz+(dy-1)*(dy-1)<=7)put(x+dx,31+dy,z+dz,blossom?PinkWool:Leaves);
    };
    auto pillar=[&](int x,int z,int height,Block block){for(int y=26;y<26+height;++y)put(x,y,z,block);};
    island(128,128,11,map==2?Grass:map==1?StoneBricks:EndStone,map);
    put(128,25,128,BlockOfEmerald);
    for(int team=0;team<8;++team){
        int x=int(bases[team].x),z=int(bases[team].z);
        island(x,z,map==1?12:11,map==1?StoneBricks:Grass,map);
        for(int dx=-5;dx<=5;++dx)for(int dz=-5;dz<=5;++dz)put(x+dx,25,z+dz,wool[team]);
        put(x,26,z+3,Bed);put(x,26,z+4,BedHead);put(x+4,26,z-2,Chest);if(map!=1)put(x-4,25,z-2,LampOn);
        if(map==0){
            for(int dx=-8;dx<=8;++dx)for(int y=26;y<=30;++y)if(std::abs(dx)>2)put(x+dx,y,z-7,StoneBricks);
            for(int side:{-1,1}){
                int tx=x+side*8,tz=z-5;
                for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz)for(int y=26;y<=35;++y)if(std::abs(dx)==1||std::abs(dz)==1)put(tx+dx,y,tz+dz,StoneBricks);
                for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz){put(tx+dx,36,tz+dz,CarvedStone);if((dx+dz)%2==0)put(tx+dx,37,tz+dz,wool[team]);}
                for(int dz=0;dz<=6;++dz){put(x+side*9,26,z+dz,StoneBricks);if(dz%2==0)put(x+side*9,27,z+dz,StoneBricks);}
            }
            for(int dx=-2;dx<=2;++dx)put(x+dx,31,z-7,wool[team]);
        }else if(map==1){
            for(int side:{-1,1}){
                pillar(x+side*7,z-5,side<0?9:6,CarvedStone);
                for(int dx=-3;dx<=3;++dx)put(x+side*7+dx,side<0?35:32,z-5,StoneBricks);
                for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz){int h=1+(dx*dx+dz*dz)%3;for(int y=26;y<26+h;++y)if(dx*dx+dz*dz<6)put(x+side*8+dx,y,z+5+dz,StoneBricks);}
            }
            for(int side:{-1,1}){
                int sx=x+side*8,sz=z+5;
                for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=1;++dz)put(sx+dx,25,sz+dz,StoneBricks);
                put(sx,25,sz,LampOn);
                for(int y=26;y<=28;++y){
                    for(int dx=-1;dx<=1;++dx)for(int dz=-2;dz<=0;++dz)put(sx+dx,y,sz+dz,Air);
                    put(sx-2,y,sz,StoneBricks);put(sx+2,y,sz,StoneBricks);
                    for(int dx=-1;dx<=1;++dx)put(sx+dx,y,sz+1,StoneBricks);
                }
                for(int dx=-2;dx<=2;++dx)for(int dz=-1;dz<=1;++dz)put(sx+dx,29,sz+dz,CarvedStone);
            }
            for(int dz=-9;dz<=-6;++dz)for(int dx=-4;dx<=4;++dx)if((dx*dx+dz)%3)put(x+dx,26,z+dz,CarvedStone);
            for(int dx=-3;dx<=3;++dx)put(x+dx,32,z-7,wool[team]);
        }else{
            tree(x-8,z+3,true);tree(x+8,z+3,true);
            for(int dx=-6;dx<=6;++dx)for(int dz=-9;dz<=-6;++dz)put(x+dx,26,z+dz,Planks);
            for(int sx:{-1,1})for(int sz:{-9,-6})pillar(x+sx*5,z+sz,5,Wood);
            for(int dx=-7;dx<=7;++dx)for(int dz=-10;dz<=-5;++dz){put(x+dx,32,z+dz,DarkOakPlanks);if(std::abs(dx)<=5&&dz>=-9&&dz<=-6)put(x+dx,33,z+dz,PinkWool);}
            for(int side:{-1,1})for(int dz=5;dz<=8;++dz)for(int dx=6;dx<=8;++dx)put(x+side*dx,26,z+dz,Grass);
        }
    }
    // Compact market stalls leave the central forge, chest and bed accessible.
    for(int team=0;team<8;++team)for(int kind=0;kind<2;++kind){
        auto base=bases[team];auto p=map==1?RuinsVendor(team,kind,gap):Vector3{base.x+(kind?4.f:-4.f),26,base.z+(kind?2.f:-2.f)};
        int x=int(p.x),z=int(p.z),front=map==1?-1:kind?-1:1;
        for(int dx=-2;dx<=2;++dx)for(int dz=-1;dz<=1;++dz)put(x+dx,25,z+dz,map==1?StoneBricks:Planks);
        put(x,25,z,LampOn);
        for(int side:{-1,1})for(int y=26;y<=28;++y)put(x+side*2,y,z,Wood);
        for(int dx=-2;dx<=2;++dx)for(int dz=-1;dz<=1;++dz)put(x+dx,map==1?30:29,z+dz,(dx+2)%2?WhiteWool:wool[team]);
        for(int dx=-1;dx<=1;++dx){put(x+dx,26,z-front,Planks);put(x+dx,27,z-front,Planks);}
        put(x-2,28,z+front,LampOn);put(x+2,28,z+front,LampOn);
    }
    for(auto pos:DiamondIslands(gap,map)){
        int x=int(pos.x),z=int(pos.z);island(x,z,4,map==2?Grass:StoneBricks,map);put(x,25,z,BlockOfDiamond);
        if(map==0)for(int side:{-1,1}){pillar(x+side*3,z,3,CarvedStone);put(x+side*3,29,z,LampOn);}
        if(map==1){pillar(x-3,z+1,5,CarvedStone);for(int dx=-3;dx<=0;++dx)put(x+dx,31,z+1,StoneBricks);}
        if(map==2)for(int side:{-1,1}){pillar(x+side*3,z+1,2,Wood);put(x+side*3,28,z+1,Leaves);}
    }
    if(map==0){
        for(int sx:{-1,1})for(int sz:{-1,1}){pillar(128+sx*7,128+sz*7,8,StoneBricks);put(128+sx*7,34,128+sz*7,LampOn);}
        for(int side:{-1,1})for(int d=-7;d<=7;++d)if(std::abs(d)>2){put(128+d,30,128+side*7,CarvedStone);put(128+side*7,30,128+d,CarvedStone);}
    }else if(map==1){
        for(int sx:{-1,1})for(int sz:{-1,1})pillar(128+sx*6,128+sz*6,(sx==sz)?10:6,CarvedStone);
        for(int dx=-6;dx<=6;++dx)put(128+dx,36,122,StoneBricks);
        for(int dx=-9;dx<=9;++dx)for(int dz=-9;dz<=9;++dz)if(std::abs(dx)>5&&std::abs(dz)>5&&dx*dx+dz*dz<95)put(128+dx,26+(std::abs(dx)+std::abs(dz))%2,128+dz,StoneBricks);
    }else{
        for(int sx:{-1,1})for(int sz:{-1,1})pillar(128+sx*5,128+sz*5,6,Wood);
        for(int dx=-7;dx<=7;++dx)for(int dz=-7;dz<=7;++dz){put(128+dx,33,128+dz,DarkOakPlanks);if(std::abs(dx)<=5&&std::abs(dz)<=5)put(128+dx,34,128+dz,PinkWool);}
        tree(120,128,true);tree(136,128,true);
    }
    if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,65,z,Glass);if(x==119||x==137||z==119||z==137){put(x,66,z,Glass);put(x,67,z,Glass);}}
}
RenderTexture2D CreateMapPreview(int map,int gap){
    std::vector<unsigned char> cells(size_t(Width)*Height*Width);
    std::vector<unsigned char> barriers(cells.size());if(map==4){const auto& records=AirshowCells();for(size_t i=0;i<records.size();i+=7)if(records[i+5]==14)barriers[(size_t(records[i])*Height+records[i+1])*Width+records[i+2]]=1;}
    int lowY=Height,highY=0;
    auto index=[](int x,int y,int z){return (size_t(x)*Height+y)*Width+z;};
    BuildArenaTerrain(map,gap,[&](int x,int y,int z,Block b){if(Inside(x,y,z)){cells[index(x,y,z)]=b;if(b!=Air){lowY=std::min(lowY,y);highY=std::max(highY,y);}}},false);
    if(map>=FirstExternalArena){const auto& records=ExternalCells(map);for(size_t i=0;i<records.size();i+=7){size_t key=index(records[i],records[i+1],records[i+2]);if(records[i+5]==14)barriers[key]=1;else if(records[i+3]>=MapSlab)cells[key]=records[i+4];}}
    auto at=[&](int x,int y,int z){return Inside(x,y,z)?cells[index(x,y,z)]:0;};
    RenderTexture2D image=LoadRenderTexture(360,210);if(!image.id)return image;
    float centerY=map>=FirstExternalArena?float(lowY+highY)*.5f:map==4?48.f:25.f;
    Camera3D camera{{250,centerY+155,250},{128,centerY,128},{0,1,0},float(map==3?160:map>=4?260:2*(gap+50)),CAMERA_ORTHOGRAPHIC};
    BeginTextureMode(image);ClearBackground({117,174,207,255});BeginMode3D(camera);
    for(int x=0;x<Width;++x)for(int z=0;z<Width;++z)for(int y=0;y<(map>=FirstExternalArena?Height:map==4?110:map==3?74:40);++y){int b=at(x,y,z);if(!b||barriers[index(x,y,z)]||(at(x-1,y,z)&&at(x+1,y,z)&&at(x,y-1,z)&&at(x,y+1,z)&&at(x,y,z-1)&&at(x,y,z+1)))continue;Color c{148,145,132,255};if(Blocks[b].color)c={static_cast<unsigned char>(Blocks[b].color>>16),static_cast<unsigned char>(Blocks[b].color>>8),static_cast<unsigned char>(Blocks[b].color),255};if(b==Grass)c={106,156,63,255};if(b==Stone||b==StoneBricks)c={126,129,135,255};if(b==Planks||b==Wood)c={137,103,60,255};if(b==RedWool)c=RED;if(b==BlueWool)c=BLUE;if(b==GreenWool)c=GREEN;if(b==YellowWool)c=YELLOW;if(b==PinkWool)c=PINK;if(b==CyanWool)c={55,215,215,255};if(b==WhiteWool)c=RAYWHITE;if(b==GrayWool)c=GRAY;if(b==EndStone)c={207,204,152,255};DrawCube({x+.5f,y+.5f,z+.5f},1,1,1,c);}
    EndMode3D();EndTextureMode();SetTextureFilter(image.texture,TEXTURE_FILTER_BILINEAR);return image;
}
}
