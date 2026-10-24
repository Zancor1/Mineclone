#include "bedwars_maps.h"
#include "world.h"
#include "map_blocks.h"
#include <zlib.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <vector>
namespace game::bw {
namespace {
#include "nc_map_data.inc"
const std::vector<unsigned char>& NcCells(){static const auto cells=[](){std::vector<unsigned char> result(NcCellCount*7);uLongf length=result.size();if(uncompress(result.data(),&length,NcMapData,sizeof(NcMapData))!=Z_OK||length!=result.size())throw std::runtime_error("Mapa NC corrompido");return result;}();return cells;}
}
int MapTeams(int map){return map==3?2:4;}
std::array<Vector3,2> MapBeds(int map,int team,const Vector3& base){if(map==3)return team==0?std::array<Vector3,2>{{{86,26,156},{85,26,156}}}:std::array<Vector3,2>{{{170,26,156},{171,26,156}}};return {{{float(int(base.x)),26,float(int(base.z)+3)},{float(int(base.x)),26,float(int(base.z)+4)}}};}

const char* MapName(int map){constexpr const char* names[]={"Fortaleza","Ruinas","Jardins","NC"};return names[std::clamp(map,0,MapCount-1)];}
const char* MapDescription(int map){constexpr const char* descriptions[]={"Torres e muralhas nas ilhas","Colinas de pedra e arcos antigos","Terracos, arvores e pavilhoes","Arena original importada - 2 times"};return descriptions[std::clamp(map,0,MapCount-1)];}
std::array<Vector3,4> ArenaBases(int gap,int map){if(map==3)return {{{75.5f,26,156.5f},{181.5f,26,156.5f},{128.5f,26,128.5f},{128.5f,26,128.5f}}};int low=128-(23+gap)/2,high=low+23+gap;return {{{low+.5f,26,low+.5f},{high+.5f,26,low+.5f},{low+.5f,26,high+.5f},{high+.5f,26,high+.5f}}};}
void BuildArenaTerrain(int map,int gap,const std::function<void(int,int,int,Block)>& put,bool lobby){
    if(map==3){const auto& cells=NcCells();for(size_t i=0;i<cells.size();i+=7){int x=cells[i],y=cells[i+1],z=cells[i+2];Block block=Block(cells[i+3]);put(x,y,z,block);if(lobby&&block>=MapSlab)SetMapBlockState(x,y,z,{block,Block(cells[i+4]),MapShape(cells[i+5]),cells[i+6]});}
        for(int team=0;team<2;++team){auto beds=MapBeds(map,team,{});for(int i=0;i<2;++i)put(int(beds[i].x),int(beds[i].y),int(beds[i].z),i?BedHead:Bed);}
        if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,65,z,Glass);if(x==119||x==137||z==119||z==137){put(x,66,z,Glass);put(x,67,z,Glass);}}
        return;
    }
    const auto bases=ArenaBases(gap,map);constexpr Block wool[]={RedWool,BlueWool,GreenWool,YellowWool};
    auto island=[&](int cx,int cz,int radius,Block top){for(int dx=-radius;dx<=radius;++dx)for(int dz=-radius;dz<=radius;++dz){float d=std::sqrt(float(dx*dx+dz*dz));if(d>radius+.5f)continue;int depth=std::max(2,int((radius-d)*.65f)+3);for(int y=25-depth;y<=25;++y)put(cx+dx,y,cz+dz,y==25?top:Stone);}};
    island(128,128,std::min(10,gap/2-4),map==2?Grass:EndStone);put(128,25,128,BlockOfEmerald);
    for(int team=0;team<4;++team){int x=int(bases[team].x),z=int(bases[team].z);island(x,z,8,map==1?StoneBricks:Grass);
        for(int dx=-5;dx<=5;++dx)for(int dz=-5;dz<=5;++dz)put(x+dx,25,z+dz,wool[team]);
        for(int dx=-5;dx<=5;++dx)for(int y=26;y<=29;++y)put(x+dx,y,z-5,map==1?StoneBricks:Planks);
        for(int dx=-5;dx<=5;++dx)put(x+dx,30,z-5,wool[team]);
        put(x,26,z+3,Bed);put(x,26,z+4,BedHead);put(x+4,26,z-2,Chest);put(x-4,25,z-2,LampOn);
        if(map==0){for(int side:{-1,1}){int tx=x+side*6,tz=z-4;for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz)for(int y=26;y<=33;++y)if(std::abs(dx)==1||std::abs(dz)==1)put(tx+dx,y,tz+dz,StoneBricks);for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz)if(std::abs(dx)<2||std::abs(dz)<2)put(tx+dx,34,tz+dz,wool[team]);}}
        if(map==1){for(int dx=-8;dx<=8;++dx)for(int dz=-8;dz<=8;++dz)if(dx*dx+dz*dz<=64&&(std::abs(dx)>6||std::abs(dz)>6)){int height=int(2+1.5f*std::sin(dx*.7f)+1.5f*std::cos(dz*.6f));for(int y=26;y<26+height;++y)put(x+dx,y,z+dz,StoneBricks);}
            for(int side:{-1,1})for(int y=26;y<=34;++y)put(x+side*4,y,z-6,CarvedStone);
            for(int dx=-4;dx<=4;++dx)put(x+dx,34,z-6,StoneBricks);
        }
        if(map==2){for(int side:{-1,1}){int tx=x+side*6,tz=z+1;for(int y=26;y<=31;++y)put(tx,y,tz,Wood);for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz)for(int dy=0;dy<3;++dy)if(dx*dx+dz*dz+dy*dy<8)put(tx+dx,31+dy,tz+dz,PinkWool);}
            for(int dx=-5;dx<=5;++dx)for(int dz=-6;dz<=-4;++dz)put(x+dx,32,z+dz,DarkOakPlanks);
            for(int side:{-1,1})for(int y=26;y<=31;++y)put(x+side*5,y,z-6,Wood);
        }
    }
    for(int i=0;i<4;++i){int x=i<2?int(bases[i].x):128,z=i<2?128:int(bases[i].z)-7;if(i==3)z=int(bases[0].z)+7;island(x,z,2,map==2?Grass:StoneBricks);put(x,25,z,BlockOfDiamond);
        if(map==0){for(int side:{-1,1}){put(x+side*2,26,z,StoneBricks);put(x+side*2,27,z,CarvedStone);put(x+side*2,28,z,LampOn);}}
        if(map==1){for(int y=26;y<=29;++y)put(x-2,y,z,CarvedStone);for(int dx=-2;dx<=1;++dx)put(x+dx,30,z,StoneBricks);put(x+2,26,z,StoneBricks);}
        if(map==2){for(int side:{-1,1}){put(x+side*2,26,z,Wood);put(x+side*2,27,z,PinkWool);put(x+side*2,28,z,Leaves);}}
    }
    int r=std::min(10,gap/2-4);
    if(map==1)for(int dx=-r;dx<=r;++dx)for(int dz=-r;dz<=r;++dz)if(dx*dx+dz*dz<=r*r&&std::abs(dx)>2&&std::abs(dz)>2){int h=1+(std::abs(dx)+std::abs(dz))%4;for(int y=26;y<26+h;++y)put(128+dx,y,128+dz,StoneBricks);}
    if(map==2)for(int side:{-1,1})for(int y=26;y<=33;++y)put(128+side*3,y,128-3,Wood);
    if(map==0){
        for(int sx:{-1,1})for(int sz:{-1,1})for(int y=26;y<=32;++y)put(128+sx*5,y,128+sz*5,y==32?LampOn:StoneBricks);
        for(int dx=-5;dx<=5;++dx)for(int side:{-1,1})if(std::abs(dx)>2)put(128+dx,29,128+side*5,CarvedStone);
    }
    if(map==1){for(int side:{-1,1})for(int y=26;y<=33;++y)put(128+side*4,y,124,CarvedStone);for(int dx=-4;dx<=4;++dx)put(128+dx,34,124,StoneBricks);}
    if(map==2){for(int side:{-1,1}){int tx=128+side*6;for(int y=26;y<=31;++y)put(tx,y,131,Wood);for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz)if(dx*dx+dz*dz<6){put(tx+dx,31,131+dz,PinkWool);put(tx+dx,32,131+dz,Leaves);}}}
    if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,65,z,Glass);if(x==119||x==137||z==119||z==137){put(x,66,z,Glass);put(x,67,z,Glass);}}
}
RenderTexture2D CreateMapPreview(int map,int gap){
    std::vector<unsigned char> cells(size_t(Width)*Height*Width);
    auto index=[](int x,int y,int z){return (size_t(x)*Height+y)*Width+z;};
    BuildArenaTerrain(map,gap,[&](int x,int y,int z,Block b){if(Inside(x,y,z))cells[index(x,y,z)]=b;},false);
    auto at=[&](int x,int y,int z){return Inside(x,y,z)?cells[index(x,y,z)]:0;};
    RenderTexture2D image=LoadRenderTexture(360,210);if(!image.id)return image;
    Camera3D camera{{250,180,250},{128,25,128},{0,1,0},float(map==3?160:gap+65),CAMERA_ORTHOGRAPHIC};
    BeginTextureMode(image);ClearBackground({117,174,207,255});BeginMode3D(camera);
    for(int x=0;x<Width;++x)for(int z=0;z<Width;++z)for(int y=0;y<(map==3?74:36);++y){int b=at(x,y,z);if(!b||(at(x-1,y,z)&&at(x+1,y,z)&&at(x,y-1,z)&&at(x,y+1,z)&&at(x,y,z-1)&&at(x,y,z+1)))continue;Color c{148,145,132,255};if(Blocks[b].color)c={static_cast<unsigned char>(Blocks[b].color>>16),static_cast<unsigned char>(Blocks[b].color>>8),static_cast<unsigned char>(Blocks[b].color),255};if(b==Grass)c={106,156,63,255};if(b==Stone||b==StoneBricks)c={126,129,135,255};if(b==Planks||b==Wood)c={137,103,60,255};if(b==RedWool)c=RED;if(b==BlueWool)c=BLUE;if(b==GreenWool)c=GREEN;if(b==YellowWool)c=YELLOW;if(b==PinkWool)c=PINK;if(b==EndStone)c={207,204,152,255};DrawCube({x+.5f,y+.5f,z+.5f},1,1,1,c);}
    EndMode3D();EndTextureMode();SetTextureFilter(image.texture,TEXTURE_FILTER_BILINEAR);return image;
}
}
