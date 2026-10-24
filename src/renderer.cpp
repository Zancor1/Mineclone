#include "projectile_models.h"
#include "renderer.h"
#include "map_blocks.h"
#include "bedwars.h"
#include "cosmetics.h"
#include "lighting.h"
#include "settings.h"
#include "items.h"
#include "bow_model.h"
#include "network.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
namespace game {
int BlockTile(Block b,int f) {
    if(b==Chest) return f==2?BaseTileCount-2:(f==4?16+int(Chest)-14:BaseTileCount-1);
    switch(b) {
        case Grass:return f==2?0:(f==3?1:6);
        case Stone:return 2;
        case Wood:return f==2||f==3?4:3;
        case Leaves:return 5;
        case Sand:return 7;
        case Water:return 8;
        case Glass:return 9;
        case Cactus:return f==2||f==3?11:10;
        case Bricks:return 12;
        case CoalOre:return 13;
        case WhiteWool:return 14;
        case Planks:return 15;
        default:return b>=Cobblestone?16+(int(b)-int(Cobblestone)):1;
    }
}
namespace {

const Color Palette[]={{0,0,0,0},{100,153,56,255},{133,93,62,255},{134,134,131,255},
    {112,82,47,255},{64,117,44,255},{220,204,146,255},{58,121,196,145},{173,214,228,70},
    {51,129,58,255},{164,78,58,255},{134,134,131,255},{229,228,220,255},{173,135,79,255},
    {110,111,108,255},{130,131,128,255},{144,146,141,255},{45,85,57,255},
    {185,105,56,255},{67,46,29,255},{255,213,113,255},{91,72,47,255},
    {176,43,39,255},{64,132,53,255},{52,78,166,255},{227,191,46,255},
    {130,122,111,255},{155,165,177,255},{239,246,248,255},{101,190,80,160},
    {237,92,22,220},{52,54,52,255},{79,142,47,255},{125,88,44,255},
    {49,120,62,255},{172,85,187,255},{115,162,221,255},{211,53,46,255},
    {239,127,165,255},{190,34,49,255},{242,208,44,255},
    {234,126,40,255},{183,74,180,255},{101,175,215,255},{128,190,45,255},
    {222,137,164,255},{72,76,79,255},{163,169,169,255},{32,143,154,255},
    {124,60,174,255},{115,78,48,255},{30,30,35,255},{157,103,44,255},{150,100,50,255},{136,135,127,255},{105,107,108,255},
    {124,84,40,255},{216,218,213,255},{35,35,36,255},
    {157,109,54,255},{143,147,147,255},{226,231,233,255},{148,104,52,255},{140,145,146,255},{218,224,226,255},
#include "content18_colors.inc"
#include "minigame_colors.inc"
    {89,191,232,255},{100,214,90,255},{206,145,220,255},
    {153,155,157,255},{153,155,157,255},{152,115,70,255},{169,173,177,255},{245,181,71,255},{238,238,238,255},
};
static_assert(sizeof(Palette)/sizeof(*Palette)==BlockCount,"Every block needs an atlas color");
Color Shade(Color c,float light) {
    return {static_cast<unsigned char>(std::clamp(c.r*light,0.f,255.f)),
        static_cast<unsigned char>(std::clamp(c.g*light,0.f,255.f)),
        static_cast<unsigned char>(std::clamp(c.b*light,0.f,255.f)),c.a};
}
Texture2D MakeAtlas() {
    Image image=GenImageColor(TileSize*TileCount,TileSize,BLANK);
    for(int tile=0;tile<TileCount;++tile) for(int y=0;y<TileSize;++y) for(int x=0;x<TileSize;++x) {
        const Block types[16]={Grass,Dirt,Stone,Wood,Wood,Leaves,Dirt,Sand,Water,Glass,Cactus,Cactus,Bricks,CoalOre,WhiteWool,Planks};
        Block type=tile<16?types[tile]:(tile>=BaseTileCount?Stone:tile>=BaseTileCount-2?Chest:static_cast<Block>(int(Cobblestone)+tile-16));
        Color c=Palette[type];
        float variation=.85f+float(Hash(x,y,tile)%31)*.01f;
        if(tile==6&&y<3+int(Hash(x,0,60)%3)) c=Palette[Grass];
        if(tile==2&&((x+int(Hash(0,y,2)%4))%11==0)) variation*=.78f;
        if(tile==3&&(x%5==0||(x%5==1&&y%8<5))) variation*=.64f;
        if(tile==4) variation=std::max(std::abs(x-7),std::abs(y-7))%3==0?.7f:1.2f;
        if(tile==5&&Hash(x,y,5)%7<2) variation*=.65f;
        if(tile==8) { variation=.97f+.09f*std::sin(x*.8f+y*.6f); c.a=145; }
        if(tile==9) {
            bool edge=x==0||x==15||y==0||y==15;
            bool glint=(x-y==3||x-y==4)&&x>4&&x<12;
            c=edge?Color{143,192,205,200}:(glint?Color{226,247,255,145}:Color{180,222,235,38});
            variation=1;
        }
        if(tile==10) {
            variation=x%4==0?.64f:1.1f;
            if(x%4==2&&y%5==1) c={204,205,126,255};
        }
        if(tile==11) variation=std::max(std::abs(x-7),std::abs(y-7))>5?.7f:1.25f;
        if(tile==12) {
            bool mortar=y%5==0 || (x+(y/5%2)*8)%8==0;
            if(mortar) {c={174,165,148,255};variation=1;}
        }
        if(tile==13 && Hash(x/2,y/2,133)%5==0) {c={38,37,35,255};variation=.8f;}
        if(Wool(type)) {
            variation=.92f+float(Hash(x,y,144)%9)*.01f;
            if((x+y)%4==0) variation*=.96f;
        }
        if(tile==15) {
            bool seam=y%4==0 || (x+(y/4%2)*7)%16==0;
            if(seam) variation*=.62f;
            else if(y%4==2) variation*=.9f;
        }
        if(type==LampOn||type==LampOff) {
            bool frame=x<2||y<2||x>13||y>13||x==y||x+y==15;
            if(frame) c={76,59,37,255};
            else if(type==LampOn) c={255,228,142,255};
        }
        if(type==StoneBricks||type==CarvedStone||type==Cobblestone) {
            int size=type==Cobblestone?5:8;
            if(y%size==0||(x+(y/size%2)*4)%size==0) variation*=.55f;
        }
        if(type==AcaciaPlanks||type==DarkOakPlanks) if(y%4==0) variation*=.6f;
        if(type==Bedrock) variation=.4f+float(Hash(x/2,y/2,200)%70)*.01f;
        if(type==IronOre&&Hash(x/2,y/2,803)%5==0) {c={186,141,110,255};variation=1;}
        if(type==CraftingTable) {
            if(x%5==0||y%5==0) c={69,43,22,255};
            else if(y>11) c={103,65,30,255};
        }
        if(type==Furnace) {
            if(x==0||y==0||x==15||y==15) c={66,67,67,255};
            if(x>=4&&x<=11&&((y>=8&&y<=13)||(y>=3&&y<=5))) c={26,25,23,255};
        }
        if(Extended(type)&&Placeable(type)) {
            if(Blocks[type].material==2&&y%4==0) variation*=.62f;
            if(Blocks[type].material==4) variation=.92f+float(Hash(x,y,144)%9)*.01f;
            if(type==Bookshelf) {if(y%7<2) c={137,101,52,255};else c=Shade(Palette[RedWool+int(Hash(x/2,0,77)%4)],.85f);}
            if(type==GoldOre||type==DiamondOre||type==EmeraldOre||type==LapisLazuliOre||type==RedstoneOre||type==NetherQuartzOre) {
                Color ore=c;c=type==NetherQuartzOre?Color{113,60,57,255}:Palette[Stone];
                if(Hash(x/2,y/2,803)%5==0) c=ore;
            }
            if(type==Ice||type==PackedIce) variation=.9f+float((x+y)%7)/50.f;
        }
        if(Extended(type)&&!Placeable(type)&&!Tool(type)) {
            if((x-8)*(x-8)+(y-8)*(y-8)>40) c=BLANK;
        }
        if(Extended(type)&&Tool(type)&&!Sword(type)&&!Pickaxe(type)) {
            bool handle=std::abs(x+y-16)<=1&&x>=3&&x<=10&&y>=6;
            bool head=Blocks[type].toolKind==3?(x>=8&&x<=13&&y>=2&&y<=7):
                (Blocks[type].toolKind==4?(x>=9&&x<=13&&y>=2&&y<=6):(x>=4&&x<=12&&y>=3&&y<=5));
            if(handle) c={114,76,35,255};else if(!head)c=BLANK;
        }
        if(Sword(type)) {
            // Mirror both edges around the blade's center line.
            Color metal=Palette[type];variation=1.f;c=BLANK;
            int edge=std::min(x,15-x);
            if(y==0&&edge==7)c=Shade(metal,.3f);
            else if(y>=1&&y<=9&&edge>=6){
                if(edge==6)c=Shade(metal,.48f);
                else c=Shade(metal,y==1?.8f:1.12f);
            }else if((y==10||y==11)&&edge>=4){
                c=Shade(metal,(edge==4||(y==11&&edge<6))?.28f:.65f);
            }else if(y>=12&&y<=14&&edge>=6){
                c=edge==6?Color{62,44,25,255}:(y==13?Color{105,72,34,255}:Color{159,116,60,255});
            }else if(y==15&&edge>=5)c=Shade(metal,edge==5?.28f:.65f);
        }
        if(Pickaxe(type)) {
            bool handle=std::abs(x+y-16)<=1&&x>=3&&x<=10&&y>=6;
            bool head=(y>=3&&y<=5&&x>=4&&x<=12)||(x>=11&&x<=13&&y>=5&&y<=8);
            if(handle) c={114,76,35,255};else if(head) variation=y==3?1.12f:.82f;else c=BLANK;
        }
        if(type==Stick&&!(x>=3&&x<=12&&std::abs(x+y-15)<=1)) c=BLANK;
        if(type==IronIngot) {
            if(x<3||x>13||y<6||y>11) c=BLANK;
            else if(y<8) variation=1.13f;else variation=.82f;
        }
        if(type==Coal&&((x-8)*(x-8)+(y-8)*(y-8)>31)) c=BLANK;
        if(type==Chest) {
            bool top=tile==BlockTile(Chest,2),front=tile==BlockTile(Chest,4);
            if(x<2||x>13||y<2||y>13||(!top&&y==5)) c={67,43,22,255};
            else {variation=.93f+float(Hash(x/2,y,781)%12)*.01f;if(y%4==0) variation*=.86f;}
            if(front&&x>=7&&x<=9&&y>=5&&y<=10) {c={207,200,166,255};variation=x==7?.65f:1.f;}
        }
        if(Plant(type)) {
            bool stem=(x==7||x==8)&&y>5;
            bool leaf=(y==10&&x>=3&&x<=12)||(y==12&&x>=5&&x<=10);
            bool crown=(x-8)*(x-8)+(y-4)*(y-4)<14;
            if(type==TallGrass||type==DeadBush||type==Kelp) {
                if(!(stem||leaf||(x==y/2+2&&y>4)||(x==14-y/2&&y>4))) c=BLANK;
            } else if(stem||leaf) c={54,128,39,255};
            else if(!crown) c=BLANK;
        }
        if(type>=EnderPearl) {
            c=Palette[type];variation=1;
            float circle=(x-7.5f)*(x-7.5f)+(y-7.5f)*(y-7.5f);
            if(type==EnderPearl||type==Snowball||type==Egg||type==Fireball) {
                if(circle>(type==Egg?30:40))c=BLANK;
                else if(x<7&&y<7)c=Shade(c,1.3f);
                else if(x>9||y>10)c=Shade(c,.7f);
                if(type==Fireball&&circle<13)c=YELLOW;
            }
            if(type>=SpeedPotion&&type<=InvisibilityPotion){bool neck=x>=6&&x<=9&&y>=2&&y<=6;bool body=(x-8)*(x-8)+(y-10)*(y-10)<=25;c=neck||body?(y<5?BEIGE:y<8?Color{205,221,228,255}:Palette[type]):BLANK;}
            if(type==EnderPearl||type==Fireball)c=ProjectilePixel(type,x,y);
            if(type==GoldenApple) {
                bool body=((x-5)*(x-5)+(y-8)*(y-8)<23)||((x-10)*(x-10)+(y-8)*(y-8)<23);
                if(!body)c=BLANK;
                if(x==8&&y>=1&&y<=4)c=DARKBROWN;
                if(y==2&&x>=9&&x<=12)c=GREEN;
            }
            if(type==Bucket||type==WaterBucket||type==LavaBucket) {
                if(y<4||y>13||x<3+(y-4)/5||x>12-(y-4)/5)c=BLANK;
                else if(y==4||x==3+(y-4)/5||x==12-(y-4)/5||y==13)c=LIGHTGRAY;
                else c=type==WaterBucket?BLUE:(type==LavaBucket?ORANGE:DARKGRAY);
            }
            if(type==FilledMap){c=(x<2||x>13||y<2||y>13)?BROWN:BEIGE;if(x>=4&&x<=10&&y>=4&&y<=10)c=(x+y)%5<2?BLUE:GREEN;}
            if(type==Bow) {
                int edge=y<=3?9:y<=5?6:y<=10?4:y<=12?6:9;
                bool arc=y>=2&&y<=13&&x>=edge&&x<=edge+1;
                c=arc?(x==edge?Color{101,66,32,255}:Color{191,142,77,255}):((x==11&&y>=2&&y<=13)?Color{222,212,184,255}:BLANK);
                if(arc&&y>=7&&y<=9)c={91,67,44,255};
            }
            if(type==Arrow) {c=std::abs(x+y-15)<=1&&x>=2&&x<=13?BROWN:BLANK;if(x>=10&&x<=13&&y>=1&&y<=4)c=LIGHTGRAY;}
            if(type==FlintAndSteel)c=(x>=3&&x<=11&&y>=4&&y<=12&&(x<6||y>9))?GRAY:((x>=9&&x<=13&&y>=2&&y<=6)?DARKGRAY:BLANK);
            if(type==Tnt) {if(y>=5&&y<=10)c=RAYWHITE;if(y>=6&&y<=9&&(x==4||x==8||x==12))c=BLACK;}
            if(type==Ladder&&!(x==3||x==12||(y>=3&&y<=13&&y%4==1&&x>=3&&x<=12)))c=BLANK;
            if(type==Bed||type==BedHead) {if(y>11)c=BROWN;if(type==BedHead&&y<6)c=RAYWHITE;}
            if(Armor(type)) {
                int slot=ArmorSlot(type);bool shape=slot==0?(x>=3&&x<=12&&y>=3&&y<=11&&!(y>=8&&x>=6&&x<=9)):
                    slot==1?((x>=2&&x<=13&&y>=3&&y<=6)||(x>=5&&x<=10&&y>=6&&y<=13)):
                    slot==2?(x>=4&&x<=11&&y>=2&&y<=13&&(y<6||x<=6||x>=9)):
                    ((x>=3&&x<=6&&y>=4&&y<=11)||(x>=9&&x<=12&&y>=4&&y<=11)||(y>=10&&y<=13&&((x>=1&&x<=6)||(x>=9&&x<=14))));
                if(!shape)c=BLANK;else if(x%4==0)c=Shade(c,.75f);
            }
        }
        if(tile>=BaseTileCount){
            int kind=tile-BaseTileCount;variation=.92f+float(Hash(x,y,kind+380)%16)*.01f;
            if(kind==0){c=x>=6&&x<=9?(y<6?Color{255,200,45,255}:Color{130,85,39,255}):BLANK;}
            if(kind==1){bool flame=y>4+int(Hash(x/2,0,991)%9)&&std::abs(x-7)<7-y/5;c=flame?(y>10?ORANGE:YELLOW):BLANK;}
            if(kind==2){bool stem=x==7||(x+y)%9==0;bool leaf=(Hash(x/2,y/3,720)%4==0);c=stem||leaf?Color{63,117,42,255}:BLANK;}
            if(kind==3||kind==12){bool rail=x==3||x==4||x==11||x==12;bool tie=y%4<=1;c=rail?(kind==12?GOLD:Color{178,180,178,255}):tie?Color{119,85,48,255}:BLANK;}
            if(kind==4||kind==5){c=kind==4?Color{145,105,58,255}:Color{177,183,185,255};if(x==0||x==15||y==0||y==15||x==7)c=Shade(c,.55f);if(y>=3&&y<=8&&((x>=3&&x<=5)||(x>=10&&x<=12)))c=BLANK;if(x==12&&y==12)c=GOLD;}
            if(kind==6){c={193,193,181,255};if((y>=5&&y<=8&&((x>=3&&x<=6)||(x>=10&&x<=13)))||(y>=10&&y<=13&&x>=6&&x<=10))c={46,47,42,255};}
            if(kind==7){c={131,134,132,255};if(std::abs(x-8)+std::abs(y-8)<5)c={29,30,29,255};if(x==0||y==0||x==15||y==15)c={72,74,73,255};}
            if(kind==8){c={119,78,44,255};if(x<2||x>13||y<2||y>13)c={62,40,24,255};if(y>=6&&y<=8&&x>=3&&x<=12)c={25,24,21,255};}
            if(kind==9){c={176,24,20,255};if((x+y)%5==0)c={226,52,31,255};}
            if(kind==10||kind==11){c={78,111,72,255};if(x>3&&x<12&&y>3&&y<12)c=kind==11?Color{25,36,28,255}:Color{177,183,131,255};}
        }
        ImageDrawPixel(&image,tile*TileSize+x,y,Shade(c,variation));
    }
    Texture2D texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(texture,TEXTURE_FILTER_POINT); SetTextureWrap(texture,TEXTURE_WRAP_CLAMP);
    return texture;
}
const int Normals[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
// Counterclockwise winding viewed from outside.
const float Corners[6][4][3]={
    {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
    {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
    {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}
};
}

WaterFace BuildFluidFace(int x, int y, int z, int direction) {
    WaterFace face;
    if (direction < 0 || direction >= 6 || !Liquid(At(x,y,z))) return face;
    const Block fluid=At(x,y,z);
    const float height = FluidHeight(x, y, z);
    if (height <= 0.0f) return face;
    const int nx = x + Normals[direction][0];
    const int ny = y + Normals[direction][1];
    const int nz = z + Normals[direction][2];
    const Block neighbor = At(nx, ny, nz);
    float bottom = 0.0f;

    if (direction == 2) {
        if (neighbor == fluid || (Solid(neighbor) && height >= 1.0f)) return face;
    } else if (direction == 3) {
        if (Solid(neighbor) || (neighbor == fluid && FluidHeight(nx, ny, nz) >= 1.0f)) return face;
    } else if (neighbor == fluid) {
        bottom = FluidHeight(nx, ny, nz);
        if (bottom >= height) return face;
    } else if (neighbor != Air) {
        if(!Liquid(neighbor)||fluid<neighbor) return face;
    }

    face.normal = {float(Normals[direction][0]), float(Normals[direction][1]), float(Normals[direction][2])};
    for (int i = 0; i < 4; ++i) {
        const float localY = Corners[direction][i][1] > 0 ? height : bottom;
        face.vertices[i] = {x + Corners[direction][i][0], y + localY, z + Corners[direction][i][2]};
    }
    face.visible = true;
    return face;
}

WaterFace BuildWaterFace(int x,int y,int z,int direction) {
    return At(x,y,z)==Water?BuildFluidFace(x,y,z,direction):WaterFace{};
}

namespace {
struct Face { int x,y,z,direction; Block block;int box=-1; };
Vector3 Center(const Face& f) {
    if(f.box>=0){auto box=MapShapeBoxes(f.x,f.y,f.z).boxes[f.box];return Vector3Scale(Vector3Add(box.min,box.max),.5f);}
    if(Liquid(f.block)) {
        const WaterFace face=BuildFluidFace(f.x,f.y,f.z,f.direction);
        Vector3 center{};
        for(Vector3 vertex:face.vertices) center=Vector3Add(center,vertex);
        return Vector3Scale(center,.25f);
    }
    return {f.x+.5f+.5f*Normals[f.direction][0],
            f.y+.5f+.5f*Normals[f.direction][1],
            f.z+.5f+.5f*Normals[f.direction][2]};
}
int MapFaceTile(const Face& face){
    auto state=MapStateAt(face.x,face.y,face.z);int direction=face.direction;
    switch(state.shape){
    case MapShape::Torch:return BaseTileCount;
    case MapShape::Fire:return BaseTileCount+1;
    case MapShape::Vine:return BaseTileCount+2;
    case MapShape::Rail:return BaseTileCount+(state.material==BlockOfGold?12:3);
    case MapShape::Door:case MapShape::Trapdoor:return BaseTileCount+(state.material==BlockOfIron?5:4);
    case MapShape::Skull:if(direction==4)return BaseTileCount+6;break;
    case MapShape::Dispenser:if(direction==(state.data==2?5:state.data==3?4:state.data==4?1:0))return BaseTileCount+7;break;
    case MapShape::Jukebox:if(direction==2)return BaseTileCount+8;break;
    case MapShape::RedstoneBlock:return BaseTileCount+9;
    case MapShape::PortalFrame:if(direction==2)return BaseTileCount+(face.box>0?11:10);break;
    case MapShape::Beacon:return BlockTile(face.box==0?Glass:face.box==1?Obsidian:SeaLantern,direction);
    case MapShape::Log:{int axis=state.data&12;bool end=axis==4?(direction<2):axis==8?(direction>=4):direction==2||direction==3;return BlockTile(state.material,end?2:0);}
    default:break;
    }
    return BlockTile(state.material,direction);
}
struct Geometry {
    std::vector<float> positions,uvs,normals;
    std::vector<unsigned char> colors;
    void Clear() { positions.clear(); uvs.clear(); normals.clear(); colors.clear(); }
    int Count() const {return int(positions.size()/3);}
    void Add(const Face& f,const BoundingBox* customBox=nullptr) {
        int tile=f.box>=0?MapFaceTile(f):BlockTile(f.block,f.direction);
        WaterFace water;
        if(Liquid(f.block)) {
            water=BuildFluidFace(f.x,f.y,f.z,f.direction);
            if(!water.visible) return;
        }
        Vector3 n{float(Normals[f.direction][0]),float(Normals[f.direction][1]),float(Normals[f.direction][2])};
        const int lx=f.x+Normals[f.direction][0],ly=f.y+Normals[f.direction][1],lz=f.z+Normals[f.direction][2];
        int normalAxis=Normals[f.direction][0]?0:(Normals[f.direction][1]?1:2);
        int tangentA=(normalAxis+1)%3,tangentB=(normalAxis+2)%3;
        BoundingBox shapeBox{};if(f.box>=0)shapeBox=MapShapeBoxes(f.x,f.y,f.z).boxes[f.box];
        int emission=BlockEmissionAt(f.x,f.y,f.z);
        Color vertexColors[4];
        for(int i=0;i<4;++i) {
            int sa=Corners[f.direction][i][tangentA]>.5f?1:-1;
            int sb=Corners[f.direction][i][tangentB]>.5f?1:-1;
            VertexLight light=SampleCornerLighting(lx,ly,lz,normalAxis,sa,sb);
            if(emission) {light.block=emission/24.f;light.ambient=1;}
            if(Liquid(f.block)||Plant(f.block)) light.ambient=1;
            auto channel=[](float value) {return static_cast<unsigned char>(std::clamp(value,0.f,1.f)*255+.5f);};
            vertexColors[i]={channel(light.ambient),channel(light.sky),channel(light.block),255};
        }
        constexpr int normalOrder[6]={0,1,2,0,2,3},flippedOrder[6]={0,1,3,1,2,3};
        auto brightness=[&](int i) {return vertexColors[i].r*(vertexColors[i].g+vertexColors[i].b+1);};
        const int* order=brightness(0)+brightness(2)>brightness(1)+brightness(3)?flippedOrder:normalOrder;
        constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
        for(int v=0;v<6;++v) {
            const int index=order[v];
            float px=f.x+Corners[f.direction][index][0];
            float pz=f.z+Corners[f.direction][index][2];
            float py=Corners[f.direction][index][1];
            Vector3 vertex=Liquid(f.block)?water.vertices[index]:Vector3{px,f.y+py*CollisionHeight(f.block),pz};
            if(f.box>=0){const auto& box=shapeBox;vertex={Lerp(box.min.x,box.max.x,Corners[f.direction][index][0]),Lerp(box.min.y,box.max.y,Corners[f.direction][index][1]),Lerp(box.min.z,box.max.z,Corners[f.direction][index][2])};}
            if(customBox){const auto& box=*customBox;vertex={Lerp(box.min.x,box.max.x,Corners[f.direction][index][0]),Lerp(box.min.y,box.max.y,Corners[f.direction][index][1]),Lerp(box.min.z,box.max.z,Corners[f.direction][index][2])};}
            if(f.block==Ladder)vertex.z=f.z+.97f;
            if(Plant(f.block)) {
                float t=Corners[f.direction][index][0];
                // Two crossed planes, submitted on faces +Z and -Z.
                if(f.direction==4) vertex={f.x+t,f.y+py,f.z+t};
                else vertex={f.x+t,f.y+py,f.z+1-t};
            }
            positions.insert(positions.end(),{vertex.x,vertex.y,vertex.z});
            normals.insert(normals.end(),{n.x,n.y,n.z});
            uvs.push_back((tile*TileSize+.5f+uv[index][0]*(TileSize-1))/(TileSize*TileCount));
            uvs.push_back((.5f+uv[index][1]*(TileSize-1))/TileSize);
            Color color=vertexColors[index];
            colors.insert(colors.end(),{color.r,color.g,color.b,255});
        }
    }
};
// Bed details are baked into the chunk mesh, including the sleeping penguin.
void AddBed(Geometry& geometry,int x,int y,int z,Block bed){
    int dx=0,dz=1;bool paired=false;
    const int neighbors[4][2]={{0,1},{0,-1},{1,0},{-1,0}};
    for(const auto& offset:neighbors)if(At(x+offset[0],y,z+offset[1])==(bed==Bed?BedHead:Bed)){
        int sign=bed==Bed?1:-1;dx=offset[0]*sign;dz=offset[1]*sign;paired=true;break;
    }
    auto point=[&](Vector3 p){return Vector3{x+.5f+dz*(p.x-.5f)+dx*(p.z-.5f),y+p.y,z+.5f-dx*(p.x-.5f)+dz*(p.z-.5f)};};
    auto box=[&](Vector3 low,Vector3 high,Block material){
        auto a=point(low),b=point(high);
        BoundingBox bounds{Vector3Min(a,b),Vector3Max(a,b)};
        for(int face=0;face<6;++face)geometry.Add({x,y,z,face,material},&bounds);
    };
    box({.03f,.19f,0},{.97f,.31f,1},Planks);
    float end=bed==Bed?.04f:.83f;
    for(float side:{.07f,.79f})box({side,0,end},{side+.14f,.26f,end+.13f},Wood);
    box({.04f,.31f,.01f},{.96f,.39f,.99f},WhiteWool);
    box({.04f,.39f,0},{.96f,.55f,1},RedWool);
    // A stitched seam on each side of the blanket.
    for(float side:{.07f,.90f})box({side,.55f,.03f},{side+.025f,.556f,.97f},RedWool);
    if(bed!=BedHead)return;
    box({.14f,.55f,.51f},{.86f,.62f,.92f},WhiteWool);
    if(!paired)return;
    box({.27f,.55f,-.54f},{.73f,.78f,.22f},GrayWool);
    box({.34f,.78f,-.43f},{.66f,.805f,.13f},WhiteWool);
    for(float side:{.19f,.73f})box({side,.57f,-.40f},{side+.08f,.68f,.08f},GrayWool);
    for(float side:{.32f,.55f})box({side,.56f,-.66f},{side+.13f,.64f,-.52f},OrangeWool);
    box({.30f,.62f,.19f},{.70f,.89f,.60f},BlackWool);
    box({.35f,.89f,.25f},{.65f,.905f,.55f},WhiteWool);
    // Closed eyes and a small beak, facing upward on the pillow.
    for(float side:{.38f,.55f})box({side,.905f,.44f},{side+.07f,.914f,.46f},BlackWool);
    box({.465f,.905f,.30f},{.535f,.95f,.37f},OrangeWool);
}
void AllocateMesh(Mesh& mesh,int capacity,bool dynamic) {
    mesh={}; mesh.vertexCount=capacity; mesh.triangleCount=capacity/3;
    mesh.vertices=static_cast<float*>(MemAlloc(capacity*3*sizeof(float)));
    mesh.normals=static_cast<float*>(MemAlloc(capacity*3*sizeof(float)));
    mesh.texcoords=static_cast<float*>(MemAlloc(capacity*2*sizeof(float)));
    mesh.colors=static_cast<unsigned char*>(MemAlloc(capacity*4));
    std::memset(mesh.vertices,0,capacity*3*sizeof(float));
    std::memset(mesh.normals,0,capacity*3*sizeof(float));
    std::memset(mesh.texcoords,0,capacity*2*sizeof(float));
    std::memset(mesh.colors,255,capacity*4);
    UploadMesh(&mesh,dynamic);
}
void UploadGeometry(Mesh& mesh,const Geometry& g) {

    UpdateMeshBuffer(mesh,0,g.positions.data(),int(g.positions.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,1,g.uvs.data(),int(g.uvs.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,2,g.normals.data(),int(g.normals.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,3,g.colors.data(),int(g.colors.size()),0);
    mesh.vertexCount=g.Count(); mesh.triangleCount=mesh.vertexCount/3;
}
struct Chunk {int capacity=0;Mesh opaque{}; std::vector<Face> transparent; uint64_t version=~uint64_t(0);};
std::array<Chunk,ChunkCount*ChunkCount> Chunks;
bool Exposed(Block b,Block neighbor) {
    if(neighbor==Air||CollisionHeight(neighbor)<1.f) return true;
    if(!Transparent(b)) return Transparent(neighbor);
    if(!Transparent(neighbor)||b==neighbor) return false;
    return b>neighbor;
}
void RebuildChunk(int cx,int cz) {
    Chunk& c=Chunks[cx*ChunkCount+cz]; if(c.version==ChunkVersions[cx*ChunkCount+cz]) return;
    static Geometry geometry;geometry.Clear();c.transparent.clear();
    for(int x=cx*ChunkSize;x<(cx+1)*ChunkSize;++x)
        for(int z=cz*ChunkSize;z<(cz+1)*ChunkSize;++z) for(int y=0;y<Height;++y) {
            Block b=At(x,y,z); if(b==Air) continue;
            if(b==Bed||b==BedHead){AddBed(geometry,x,y,z,b);continue;}
            if(HasMapShape(x,y,z)){if(MapStateAt(x,y,z).shape==MapShape::Barrier)continue;auto boxes=MapShapeBoxes(x,y,z);auto state=MapStateAt(x,y,z);for(int box=0;box<boxes.count;++box)for(int f=0;f<6;++f){Face face{x,y,z,f,b,box};if(Transparent(state.material)||state.shape==MapShape::Portal||state.shape==MapShape::Beacon)c.transparent.push_back(face);else geometry.Add(face);}continue;}
            for(int f=0;f<6;++f) {
                Block neighbor=At(x+Normals[f][0],y+Normals[f][1],z+Normals[f][2]);
                if((Plant(b)||b==Ladder)&&f!=4&&f!=5) continue;
                bool exposed=HasMapShape(x+Normals[f][0],y+Normals[f][1],z+Normals[f][2])||Plant(b)||b==Ladder|| (Liquid(b)?BuildFluidFace(x,y,z,f).visible:Exposed(b,neighbor));
                if(!exposed) continue;
                Face face{x,y,z,f,b};
                if(Transparent(b)) c.transparent.push_back(face); else geometry.Add(face);
            }
        }
    int count=geometry.Count();
    if(count>c.capacity){
        if(c.opaque.vaoId)UnloadMesh(c.opaque);
        c.capacity=1024;while(c.capacity<count)c.capacity*=2;
        AllocateMesh(c.opaque,c.capacity,true);
    }
    if(count)UploadGeometry(c.opaque,geometry);
    else {c.opaque.vertexCount=0;c.opaque.triangleCount=0;}
    c.version=ChunkVersions[cx*ChunkCount+cz];
}
const char* VertexShader=R"GLSL(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
out vec2 uv;
out vec4 tint;
out vec3 worldPosition;
out vec3 normal;
void main() {
    uv=vertexTexCoord; tint=vertexColor; normal=vertexNormal;
    worldPosition=(matModel*vec4(vertexPosition,1.0)).xyz;
    gl_Position=mvp*vec4(vertexPosition,1.0);
}
)GLSL";
const char* FragmentShader=R"GLSL(#version 330
in vec2 uv;
in vec4 tint;
in vec3 worldPosition;
in vec3 normal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 eyePosition;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float regionSize;
uniform float daylight;
uniform float fullBright;
uniform vec3 sunDirection;
out vec4 finalColor;
void main() {
    vec4 color=texture(texture0,uv)*colDiffuse;
    if(color.a<0.08) discard;
    float facing=0.35+0.65*max(dot(normalize(normal),normalize(sunDirection)),0.0);
    vec3 sunlight=mix(vec3(0.025,0.035,0.075),vec3(1.0,0.98,0.92)*facing,daylight)*pow(tint.g,2.2);
    vec3 lamplight=vec3(1.0,0.86,0.64)*pow(tint.b,1.45);
    color.rgb*=mix(max(vec3(0.025,0.028,0.035),sunlight+lamplight*(1.0-0.35*daylight)),vec3(1.0),fullBright)*tint.r;
    if(color.a<0.01) discard;
    float d=length(worldPosition-eyePosition);
    float fog=1.0-exp(-pow(d*fogDensity,2.0));
    float edge=min(min(worldPosition.x,worldPosition.z),min(regionSize-worldPosition.x,regionSize-worldPosition.z));
    if(fogDensity>0.0) fog=max(fog,1.0-smoothstep(0.0,20.0,edge));
    finalColor=vec4(mix(color.rgb,fogColor,clamp(fog,0.0,1.0)),color.a);
}
)GLSL";
struct View {
    Vector3 position,forward,right,up;
    float tanY,tanX;
    explicit View(const Camera3D& c) {
        position=c.position; forward=Vector3Normalize(Vector3Subtract(c.target,c.position));
        right=Vector3Normalize(Vector3CrossProduct(forward,c.up)); up=Vector3CrossProduct(right,forward);
        tanY=std::tan(c.fovy*DEG2RAD*.5f); tanX=tanY*float(GetScreenWidth())/GetScreenHeight();
    }
    bool Sphere(Vector3 center,float radius) const {
        Vector3 delta=Vector3Subtract(center,position);
        float z=Vector3DotProduct(delta,forward);
        if(z+radius<.01f||z-radius>(bw::Ready()?256.f:RenderDistance())) return false;
        if(std::abs(Vector3DotProduct(delta,right))>z*tanX+radius*std::sqrt(1+tanX*tanX)) return false;
        return std::abs(Vector3DotProduct(delta,up))<=z*tanY+radius*std::sqrt(1+tanY*tanY);
    }
};
std::vector<int> VisibleChunks(const Camera3D& camera){
    View view(camera);std::vector<int> visible;visible.reserve(ChunkCount*ChunkCount);
    const float distance=bw::Ready()?256.f:RenderDistance(),radius=std::sqrt(float(ChunkSize*ChunkSize*2+Height*Height))*.5f;
    for(int x=0;x<ChunkCount;++x)for(int z=0;z<ChunkCount;++z){
        float nx=std::clamp(camera.position.x,float(x*ChunkSize),float((x+1)*ChunkSize)),nz=std::clamp(camera.position.z,float(z*ChunkSize),float((z+1)*ChunkSize));
        float dx=nx-camera.position.x,dz=nz-camera.position.z;
        if(dx*dx+dz*dz<=distance*distance&&view.Sphere({(x+.5f)*ChunkSize,Height*.5f,(z+.5f)*ChunkSize},radius))visible.push_back(x*ChunkCount+z);
    }
    return visible;
}
struct AlphaRenderer {
    Mesh mesh{}; int capacity=0;
    Geometry geometry;
    struct Entry {Face face; float depth;};
    std::vector<Entry> faces;
    void Draw(const View& view,Material material,const std::vector<int>& visible) {
        faces.clear(); geometry.Clear();
        for(int index:visible) for(const Face& face:Chunks[index].transparent) {
            Vector3 center=Center(face);
            if(view.Sphere(center,.72f)) faces.push_back({face,Vector3DotProduct(Vector3Subtract(center,view.position),view.forward)});
        }
        std::sort(faces.begin(),faces.end(),[](const Entry& a,const Entry& b){return a.depth>b.depth;});
        for(const Entry& entry:faces) geometry.Add(entry.face);
        if(!geometry.Count()) return;
        if(geometry.Count()>capacity) {
            if(mesh.vaoId) UnloadMesh(mesh);
            capacity=((geometry.Count()+4095)/4096)*4096;
            AllocateMesh(mesh,capacity,true);
        }
        UploadGeometry(mesh,geometry);

        rlDrawRenderBatchActive(); rlDisableDepthMask(); rlDisableBackfaceCulling();
        BeginBlendMode(BLEND_ALPHA);
        DrawMesh(mesh,material,MatrixIdentity());
        EndBlendMode();
        rlEnableBackfaceCulling(); rlEnableDepthMask();
    }
    void Release() {if(mesh.vaoId) UnloadMesh(mesh); mesh={};}
};

}
namespace {
Mesh HeldMesh(Block item,const Image& atlas,int finish=0) {
    Geometry geometry;
    constexpr int order[6]={0,1,2,0,2,3};
    constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
    const float shading[6]={.82f,.68f,1.f,.52f,.93f,.74f};
    auto add=[&](int face,int pixelX,int pixelY,bool sprite) {
        int tile=BlockTile(item,face);
        Color color=Shade(WHITE,shading[face]);
        if(finish&&sprite){Color original=GetImageColor(atlas,tile*16+pixelX,pixelY);bool handle=Sword(item)?pixelY>=9:(std::abs(pixelX+pixelY-16)<=1&&pixelX>=3&&pixelX<=10&&pixelY>=6);Color paint=handle?original:cosmetics::ColorFor(finish);float texture=.5f+.5f*std::max({original.r,original.g,original.b})/255.f;color=Shade(paint,shading[face]*(handle?1.f:texture));}
        for(int index:order) {
            Vector3 vertex;
            if(sprite) {
                float pixelEdge=pixelX+Corners[face][index][0];
                // A raised central ridge and thinner edges give swords a beveled section.
                float thickness=Sword(item)?.05f+.07f*(1.f-std::min(1.f,std::abs(pixelEdge-8.f)/2.f)):.045f;
                vertex={(pixelEdge-8)/16.f,
                        (7-pixelY+Corners[face][index][1])/16.f,
                        (Corners[face][index][2]-.5f)*thickness};
            }
            else vertex={Corners[face][index][0]-.5f,Corners[face][index][1]-.5f,Corners[face][index][2]-.5f};
            geometry.positions.insert(geometry.positions.end(),{vertex.x,vertex.y,vertex.z});
            geometry.normals.insert(geometry.normals.end(),{float(Normals[face][0]),float(Normals[face][1]),float(Normals[face][2])});
            geometry.uvs.push_back((tile*TileSize+(sprite?pixelX+.5f:.5f+uv[index][0]*15))/float(TileSize*TileCount));
            geometry.uvs.push_back((sprite?pixelY+.5f:.5f+uv[index][1]*15)/16.f);
            geometry.colors.insert(geometry.colors.end(),{color.r,color.g,color.b,255});
        }
    };
    if(item==GoldenApple){
        // A closed voxel volume, including the stalk and leaf, rather than a thin sprite.
        auto voxel=[](int x,int y,int z){
            if(y>=5&&y<=7&&x==0&&z==0)return 2;
            if(y==7&&x>=1&&x<=3&&z>=0&&z<=1)return 3;
            float xx=x/5.5f,yy=(y+.5f)/5.f,zz=z/4.5f;
            return xx*xx+yy*yy+zz*zz<1.f&&!(y>=4&&std::abs(x)<=1)?1:0;
        };
        for(int y=-5;y<=7;++y)for(int x=-6;x<=6;++x)for(int z=-5;z<=5;++z){
            int kind=voxel(x,y,z);if(!kind)continue;
            float grain=.88f+float((x*x+3*y*y+7*z*z)%7)*.018f;
            Color base=kind==2?Color{109,66,22,255}:kind==3?Color{123,166,30,255}:Color{255,190,24,255};
            if(kind==1&&x<=-2&&y>=1&&z<=-2)base={255,235,114,255};
            for(int face=0;face<6;++face){
                if(voxel(x+Normals[face][0],y+Normals[face][1],z+Normals[face][2]))continue;
                Color color=Shade(base,shading[face]*grain);
                for(int index:order){
                    geometry.positions.insert(geometry.positions.end(),{(x+Corners[face][index][0]-.5f)*.073f,(y+Corners[face][index][1]-.5f)*.073f,(z+Corners[face][index][2]-.5f)*.073f});
                    geometry.normals.insert(geometry.normals.end(),{float(Normals[face][0]),float(Normals[face][1]),float(Normals[face][2])});
                    geometry.uvs.insert(geometry.uvs.end(),{.5f,.5f});geometry.colors.insert(geometry.colors.end(),{color.r,color.g,color.b,255});
                }
            }
        }
    }else if(Placeable(item)&&!Plant(item)) {
        for(int f=0;f<6;++f) add(f,0,0,false);
    } else {
        int tile=BlockTile(item);
        auto visible=[&](int x,int y) {return x>=0&&x<16&&y>=0&&y<16&&GetImageColor(atlas,tile*16+x,y).a>40;};
        for(int y=0;y<16;++y) for(int x=0;x<16;++x) if(visible(x,y)) {
            add(4,x,y,true);add(5,x,y,true);
            if(!visible(x+1,y)) add(0,x,y,true);
            if(!visible(x-1,y)) add(1,x,y,true);
            if(!visible(x,y-1)) add(2,x,y,true);
            if(!visible(x,y+1)) add(3,x,y,true);
        }
    }
    Mesh mesh{};
    if(geometry.Count()) {AllocateMesh(mesh,geometry.Count(),false);UploadGeometry(mesh,geometry);}
    return mesh;
}
struct RemoteItemCache {
    unsigned int atlas=0;Image pixels{};Material material{};std::map<int,Mesh> meshes;
    void Clear(){for(auto& entry:meshes)if(entry.second.vaoId)UnloadMesh(entry.second);meshes.clear();if(pixels.data)UnloadImage(pixels);pixels={};if(material.maps){material.maps[MATERIAL_MAP_DIFFUSE].texture={};UnloadMaterial(material);}material={};atlas=0;}
};
RemoteItemCache remoteItems;

}
void DrawDroppedItem3D(Texture2D atlas,Block item,Vector3 position,float age){
    rlPushMatrix();rlTranslatef(position.x,position.y+.16f+.025f*std::sin(age*3),position.z);rlRotatef(age*55,0,1,0);rlScalef(.48f,.48f,.48f);
    auto transform=rlGetMatrixTransform();rlPopMatrix();DrawHeldItem3D(atlas,item,transform,WHITE,0,-1);
}
void DrawHeldItem3D(Texture2D atlas,Block item,Matrix transform,Color tint,int finish,float bowDraw){
    if(item==EnderPearl||item==Fireball){rlPushMatrix();rlMultMatrixf(MatrixToFloat(transform));DrawProjectileItemModel(item,tint);rlPopMatrix();return;}
    if(item==Bow){rlPushMatrix();rlMultMatrixf(MatrixToFloat(transform));DrawBowModel(bowDraw,tint,atlas,BlockTile(Planks));rlPopMatrix();return;}
    if(cosmetics::Category(item)<0)finish=0;
    finish=std::clamp(finish,0,progression::SkinCount);
    if(!atlas.id||item==Air||!ValidBlock(item))return;
    if(remoteItems.atlas!=atlas.id){remoteItems.Clear();remoteItems.atlas=atlas.id;remoteItems.pixels=LoadImageFromTexture(atlas);remoteItems.material=LoadMaterialDefault();remoteItems.material.maps[MATERIAL_MAP_DIFFUSE].texture=atlas;}
    if(!remoteItems.pixels.data)return;
    int key=int(item)+finish*BlockCount;auto found=remoteItems.meshes.find(key);if(found==remoteItems.meshes.end())found=remoteItems.meshes.emplace(key,HeldMesh(item,remoteItems.pixels,finish)).first;
    remoteItems.material.maps[MATERIAL_MAP_DIFFUSE].texture=(finish||item==GoldenApple)?Texture2D{rlGetTextureIdDefault(),1,1,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8}:atlas;
    remoteItems.material.maps[MATERIAL_MAP_DIFFUSE].color=Emission(item)?WHITE:tint;
    rlDrawRenderBatchActive();if(found->second.vertexCount)DrawMesh(found->second,remoteItems.material,transform);
}
static Texture2D MakeMiningCracks(int stage){
    Image image=GenImageColor(32,32,BLANK);
    static constexpr int paths[8][7][2]={
        {{16,16},{13,13},{14,10},{10,8},{11,4},{7,2},{8,0}},
        {{16,16},{19,13},{18,10},{22,9},{21,5},{25,3},{24,0}},
        {{16,16},{20,16},{22,19},{26,17},{28,21},{31,20},{31,24}},
        {{16,16},{18,20},{17,23},{21,25},{20,28},{23,30},{23,31}},
        {{16,16},{13,19},{14,23},{10,24},{11,28},{7,30},{7,31}},
        {{16,16},{12,16},{10,13},{7,15},{5,12},{2,14},{0,12}},
        {{16,16},{17,12},{15,9},{17,6},{15,4},{17,1},{16,0}},
        {{16,16},{12,20},{8,19},{6,23},{3,22},{1,26},{0,25}}};
    for(int ray=0;ray<8;++ray){int count=std::clamp(stage+1-ray/2,0,6);
        for(int i=0;i<count;++i){int x=paths[ray][i][0],y=paths[ray][i][1],nx=paths[ray][i+1][0],ny=paths[ray][i+1][1];
            ImageDrawLine(&image,x,y,nx,ny,{15,12,10,225});
            if(stage>=6)ImageDrawLine(&image,x+1,y,nx+1,ny,{15,12,10,190});
            if(i>=2&&stage>=i+ray/2+2)ImageDrawLine(&image,nx,ny,nx+(ray%2?3:-3),ny+(ray<4?3:-3),{15,12,10,210});
        }
    }
    Texture2D texture=LoadTextureFromImage(image);UnloadImage(image);SetTextureFilter(texture,TEXTURE_FILTER_POINT);return texture;
}
static void DrawMiningCracks(Texture2D texture,int x,int y,int z){
    auto boxes=CollisionBoxesAt(x,y,z);
    if(!boxes.count){boxes.boxes[0]={{float(x),float(y),float(z)},{x+1.f,y+CollisionHeight(At(x,y,z)),z+1.f}};boxes.count=1;}
    rlDrawRenderBatchActive();rlDisableDepthMask();rlSetTexture(texture.id);rlBegin(RL_QUADS);rlColor4ub(255,255,255,255);
    constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
    for(int box=0;box<boxes.count;++box){auto bounds=boxes.boxes[box];
        if(At(x,y,z)==Bed||At(x,y,z)==BedHead)bounds.max.y=y+.556f;
        for(int face=0;face<6;++face)for(int i=0;i<4;++i){
            Vector3 p{Lerp(bounds.min.x,bounds.max.x,Corners[face][i][0]),Lerp(bounds.min.y,bounds.max.y,Corners[face][i][1]),Lerp(bounds.min.z,bounds.max.z,Corners[face][i][2])};
            p.x+=Normals[face][0]*.002f;p.y+=Normals[face][1]*.002f;p.z+=Normals[face][2]*.002f;
            rlTexCoord2f(uv[i][0],uv[i][1]);rlVertex3f(p.x,p.y,p.z);
        }
    }
    rlEnd();rlSetTexture(0);rlDrawRenderBatchActive();rlEnableDepthMask();
}
struct Renderer::Impl {
    std::array<Texture2D,10> cracks{};
    Texture2D atlas{};
    Shader shader{};
    Material material{};
    AlphaRenderer alpha;
    Image atlasPixels{};
    Material heldMaterial{};
    Mesh heldMesh{};
    RenderTexture2D handLayer{};
    Block held=Air;
    int heldFinish=0;
    float handClock=0,swing=1,equip=0,walking=0,running=0;
    int eye=-1,fog=-1,density=-1,day=-1,sun=-1,fullBright=-1;
    bool ready=false,sceneReady=false,preparing=false;
    float loadProgress=0;int loadStage=0;
    struct CachedMap {Texture2D texture{};uint64_t revision=0;};
    std::map<int,CachedMap> mapTextures;
    Texture2D MapTexture(int id) {
        const auto* data=net::FindMap(id);if(!data)return {};
        if(!mapTextures.count(id)&&mapTextures.size()>=64){rlDrawRenderBatchActive();UnloadTexture(mapTextures.begin()->second.texture);mapTextures.erase(mapTextures.begin());}
        auto& entry=mapTextures[id];
        if(entry.revision!=data->revision||!entry.texture.id){
            std::array<Color,128*128> pixels;for(size_t i=0;i<pixels.size();++i)pixels[i]=net::MapColor(data->pixels[i]);
            if(!entry.texture.id){Image image{pixels.data(),128,128,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};entry.texture=LoadTextureFromImage(image);SetTextureFilter(entry.texture,TEXTURE_FILTER_POINT);SetTextureWrap(entry.texture,TEXTURE_WRAP_CLAMP);}
            else UpdateTexture(entry.texture,pixels.data());
            entry.revision=data->revision;
        }
        return entry.texture;
    }
};
Renderer::Renderer():impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() {
    for(auto texture:impl->cracks)if(texture.id)UnloadTexture(texture);
    impl->alpha.Release();
    for(auto& entry:impl->mapTextures)if(entry.second.texture.id)UnloadTexture(entry.second.texture);
    remoteItems.Clear();
    if(impl->handLayer.id)UnloadRenderTexture(impl->handLayer);
    if(impl->heldMesh.vaoId) UnloadMesh(impl->heldMesh);
    if(impl->heldMaterial.maps) {
        impl->heldMaterial.maps[MATERIAL_MAP_DIFFUSE].texture={};
        UnloadMaterial(impl->heldMaterial);
    }
    if(impl->atlasPixels.data) UnloadImage(impl->atlasPixels);
    for(Chunk& c:Chunks) {if(c.opaque.vaoId) UnloadMesh(c.opaque); c=Chunk{};}
    if(impl->material.maps) {
        impl->material.maps[MATERIAL_MAP_DIFFUSE].texture={};
        impl->material.shader={}; UnloadMaterial(impl->material);
    }
    if(impl->shader.id && impl->shader.id!=rlGetShaderIdDefault()) UnloadShader(impl->shader);
    if(impl->atlas.id) UnloadTexture(impl->atlas);
}
bool Renderer::Initialize() {
    impl->atlas=MakeAtlas(); impl->shader=LoadShaderFromMemory(VertexShader,FragmentShader);
    if(!impl->atlas.id||!impl->shader.id||impl->shader.id==rlGetShaderIdDefault()) return false;
    for(int i=0;i<10;++i)impl->cracks[i]=MakeMiningCracks(i);
    impl->atlasPixels=LoadImageFromTexture(impl->atlas);
    impl->heldMaterial=LoadMaterialDefault();
    impl->heldMaterial.maps[MATERIAL_MAP_DIFFUSE].texture=impl->atlas;
    impl->material=LoadMaterialDefault(); impl->material.shader=impl->shader;
    impl->material.maps[MATERIAL_MAP_DIFFUSE].texture=impl->atlas;
    impl->eye=GetShaderLocation(impl->shader,"eyePosition");
    impl->fog=GetShaderLocation(impl->shader,"fogColor");
    impl->density=GetShaderLocation(impl->shader,"fogDensity");
    impl->day=GetShaderLocation(impl->shader,"daylight");
    impl->sun=GetShaderLocation(impl->shader,"sunDirection");
    impl->fullBright=GetShaderLocation(impl->shader,"fullBright");
    float regionSize=float(Width);
    SetShaderValue(impl->shader,GetShaderLocation(impl->shader,"regionSize"),&regionSize,SHADER_UNIFORM_FLOAT);
    impl->ready=true; return true;
}
Texture2D Renderer::Atlas() const {return impl->atlas;}
void Renderer::BeginWorldLoad(){impl->preparing=true;impl->sceneReady=false;impl->loadProgress=0;impl->loadStage=0;}
void Renderer::CancelWorldLoad(){impl->preparing=false;impl->loadProgress=0;}
float Renderer::WorldLoadProgress() const{return impl->loadProgress;}
const char* Renderer::WorldLoadStage() const{return impl->loadStage==0?"Iluminando mapa...":impl->loadStage==1?"Preparando cenario...":"Mapa pronto";}
bool Renderer::PrepareWorld(const Camera3D& camera,float budgetMilliseconds){
    if(!impl->ready)return false;
    if(!impl->preparing)return impl->sceneReady;
    double started=GetTime();float budget=std::clamp(budgetMilliseconds,.5f,16.f);
    if(LightingPending()){
        impl->loadStage=0;UpdateLighting(budget*.5f);impl->loadProgress=.4f*LightingProgress();
        if(LightingPending())return false;
    }
    if(impl->loadStage==0)bw::ConnectionTrace("loading: lighting complete; preparing mesh uploads");
    impl->loadStage=1;
    auto visible=VisibleChunks(camera);
    auto distance=[&](int i){float dx=(i/ChunkCount+.5f)*ChunkSize-camera.position.x,dz=(i%ChunkCount+.5f)*ChunkSize-camera.position.z;return dx*dx+dz*dz;};
    std::sort(visible.begin(),visible.end(),[&](int a,int b){return distance(a)<distance(b);});
    bool rebuilt=false;
    for(int i:visible)if(Chunks[i].version!=ChunkVersions[i]){
        if(rebuilt||(GetTime()-started)*1000>=budget)break;
        RebuildChunk(i/ChunkCount,i%ChunkCount);rebuilt=true;
    }
    size_t prepared=0;for(int i:visible)if(Chunks[i].version==ChunkVersions[i])++prepared;
    impl->loadProgress=.4f+.6f*prepared/std::max(1.f,float(visible.size()));
    if(prepared<visible.size())return false;
    impl->preparing=false;impl->sceneReady=true;impl->loadStage=2;impl->loadProgress=1;return true;
}
void Renderer::Draw(const Camera3D& camera,const Hit& hit,float miningProgress) {
    if(!impl->ready) return;
    UpdateLighting(impl->sceneReady&&!LightingRequiresFullUpdate()?2.f:0.f);
    View view(camera);auto visible=VisibleChunks(camera);
    auto pending=visible;
    auto priority=[&](int index){int cx=index/ChunkCount,cz=index%ChunkCount;if(hit.found&&hit.x/ChunkSize==cx&&hit.z/ChunkSize==cz)return -1.f;float dx=(cx+.5f)*ChunkSize-camera.position.x,dz=(cz+.5f)*ChunkSize-camera.position.z;return dx*dx+dz*dz;};
    std::sort(pending.begin(),pending.end(),[&](int a,int b){return priority(a)<priority(b);});
    double rebuildStart=GetTime();int rebuilt=0;
    for(int index:pending){if(Chunks[index].version==ChunkVersions[index])continue;
        if(impl->sceneReady&&rebuilt>0&&(rebuilt>=2||GetTime()-rebuildStart>.003))break;
        RebuildChunk(index/ChunkCount,index%ChunkCount);++rebuilt;
    }
    impl->sceneReady=true;
    bool underwater=WaterContains(camera.position);
    Color sky=underwater?Color{40,88,126,255}:SkyColor();
    float fog[3]={sky.r/255.f,sky.g/255.f,sky.b/255.f};
    float eye[3]={camera.position.x,camera.position.y,camera.position.z};
    float density=bw::Ready()&&!underwater?0.f:FogDensity(underwater);
    SetShaderValue(impl->shader,impl->eye,eye,SHADER_UNIFORM_VEC3);
    SetShaderValue(impl->shader,impl->fog,fog,SHADER_UNIFORM_VEC3);
    SetShaderValue(impl->shader,impl->density,&density,SHADER_UNIFORM_FLOAT);
    float fullBright=Settings.fullBright?1.f:0.f;SetShaderValue(impl->shader,impl->fullBright,&fullBright,SHADER_UNIFORM_FLOAT);
    float daylight=Daylight();
    SetShaderValue(impl->shader,impl->day,&daylight,SHADER_UNIFORM_FLOAT);
    float angle=WorldTime/DayDuration*2*PI;
    float direction[3]={std::cos(angle),std::sin(angle),-.35f};
    SetShaderValue(impl->shader,impl->sun,direction,SHADER_UNIFORM_VEC3);
    ClearBackground(sky);
    if(Settings.beautifulSky&&!underwater){
        auto mix=[](Color a,Color b,float t){return Color{static_cast<unsigned char>(Lerp(a.r,b.r,t)),static_cast<unsigned char>(Lerp(a.g,b.g,t)),static_cast<unsigned char>(Lerp(a.b,b.b,t)),255};};
        Color zenith=mix({7,11,30,255},{48,128,209,255},daylight),horizon=mix({27,32,65,255},{176,217,240,255},daylight);
        float dusk=std::max(0.f,1.f-std::abs(std::sin(angle))*4.f);horizon=mix(horizon,{236,146,115,255},dusk*.65f);
        DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),zenith,horizon);
    }
    BeginMode3D(camera);
    if(Settings.beautifulSky&&!underwater){
        rlDisableDepthMask();
        float night=1-daylight;
        if(night>.1f)for(int i=0;i<128;++i){float a=i*2.399963f,h=.12f+std::fmod(i*.618034f,.84f),r=std::sqrt(1-h*h);Vector3 p=Vector3Add(camera.position,{std::cos(a)*r*125,h*125,std::sin(a)*r*125});float twinkle=.75f+.25f*std::sin(float(GetTime())*.9f+i);DrawSphereEx(p,.14f+(i%3)*.045f,3,4,Fade(i%3?Color{218,231,255,255}:Color{255,239,209,255},night*twinkle));}
        if(daylight>.05f)for(int i=0;i<12;++i){float x=std::fmod(i*47.f+float(GetTime())*.6f,180.f)-90,z=std::fmod(i*71.f,180.f)-90;Vector3 p=Vector3Add(camera.position,{x,32.f+(i%3)*5.f,z});Color cloud=Fade(Color{238,242,249,255},.65f*daylight);DrawCube(p,14,1.3f,8,cloud);DrawCube(Vector3Add(p,{4,1,1}),8,1.4f,6,cloud);}
        rlEnableDepthMask();
    }
    Vector3 sun=Vector3Add(camera.position,{std::cos(angle)*85,std::sin(angle)*85,-30});
    Vector3 moon=Vector3Add(camera.position,{-std::cos(angle)*85,-std::sin(angle)*85,30});
    if(!underwater) {
        DrawSphere(sun,3,{255,240,173,255});DrawSphere(moon,2,{194,208,234,255});
    }
    for(int i:visible) if(Chunks[i].opaque.vertexCount) DrawMesh(Chunks[i].opaque,impl->material,MatrixIdentity());
    impl->alpha.Draw(view,impl->material,visible);
    DrawItems(impl->atlas,BlockTile(Tnt));
    for(const auto& item:DroppedItems())DrawDroppedItem3D(impl->atlas,item.stack.block,item.position,item.age);
    if(net::Joined()) {
        net::DrawPlayers();
        for(const auto& frame:net::MapFrames()) {
            Texture2D texture=impl->MapTexture(frame.mapId);if(!texture.id)continue;
            constexpr Vector3 normals[]={{0,0,1},{-1,0,0},{0,0,-1},{1,0,0}};
            Vector3 n=normals[frame.direction&3],right{n.z,0,-n.x},up{0,1,0};
            float angle=frame.rotation*PI/4;Vector3 r=Vector3Add(Vector3Scale(right,std::cos(angle)),Vector3Scale(up,std::sin(angle)));
            Vector3 u=Vector3Add(Vector3Scale(up,std::cos(angle)),Vector3Scale(right,-std::sin(angle)));
            auto quad=[&](Texture2D tex,Color tint,float offset) {
                Vector3 center=Vector3Add(frame.position,Vector3Scale(n,offset));
                const float corner[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};const float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
                rlSetTexture(tex.id?tex.id:rlGetTextureIdDefault());rlBegin(RL_QUADS);rlColor4ub(tint.r,tint.g,tint.b,tint.a);
                for(int i=0;i<4;++i){Vector3 p=Vector3Add(center,Vector3Add(Vector3Scale(r,corner[i][0]*.49f),Vector3Scale(u,corner[i][1]*.49f)));rlTexCoord2f(uv[i][0],uv[i][1]);rlVertex3f(p.x,p.y,p.z);}rlEnd();rlSetTexture(0);
            };
            rlDrawRenderBatchActive();rlDisableBackfaceCulling();quad({},Color{219,207,157,255},.005f);quad(texture,WHITE,.009f);rlDrawRenderBatchActive();rlEnableBackfaceCulling();
        }
    }
    if(net::Joined())net::DrawHolograms(camera);
    if(hit.found&&miningProgress>0&&At(hit.x,hit.y,hit.z)!=Air)DrawMiningCracks(impl->cracks[std::clamp(int(miningProgress*10),0,9)],hit.x,hit.y,hit.z);
    if(hit.found) DrawCubeWires({hit.x+.5f,hit.y+.5f,hit.z+.5f},1.006f,1.006f,1.006f,{25,29,24,255});
    EndMode3D();
    DrawItemLabels(camera);
}
void Renderer::DrawFirstPerson(const Camera3D& camera,Block item,bool moving,bool usingItem,float dt,bool sprinting) {
    if(!impl->ready) return;
    if(net::Active()&&net::HeldMapId()>=0) {
        int mapId=net::HeldMapId();Texture2D texture=impl->MapTexture(mapId);
        int scale=std::clamp((GetScreenHeight()-160)/128,1,3),size=128*scale;
        int x=(GetScreenWidth()-size)/2,y=GetScreenHeight()-104-size;
        DrawRectangle(x-9,y-9,size+18,size+18,{111,79,45,255});DrawRectangle(x-5,y-5,size+10,size+10,{219,207,157,255});
        if(texture.id)DrawTexturePro(texture,{0,0,128,128},{float(x),float(y),float(size),float(size)},{0,0},0,WHITE);
        else DrawText("Recebendo mapa...",x+14,y+size/2,18,DARKBROWN);
        return;
    }
    if(!ValidBlock(item)) item=Air;
    dt=std::clamp(dt,0.f,.05f);
    int finish=bw::ItemSkin(item);
    if(item!=impl->held||finish!=impl->heldFinish) {
        if(impl->heldMesh.vaoId) UnloadMesh(impl->heldMesh);
        impl->heldMesh={};impl->held=item;impl->heldFinish=finish;impl->equip=1;
        if(item!=Air&&item!=Bow) impl->heldMesh=HeldMesh(item,impl->atlasPixels,finish);
    }
    impl->equip=std::max(0.f,impl->equip-dt*5);
    impl->walking=Lerp(impl->walking,moving?1.f:0.f,std::min(1.f,dt*10));
    impl->running=Lerp(impl->running,moving&&sprinting?1.f:0.f,1-std::exp(-10.f*dt));
    impl->handClock+=dt*(2+impl->walking*8+impl->running*4);
    const float bowDraw=item==Bow?(bw::Ready()?bw::BowDrawTime():game::BowDrawTime()):-1.f;
    const float eating=item==GoldenApple?(bw::Ready()?bw::EatingTime():game::EatingTime()):0.f;
    if(item==Bow||eating>0)usingItem=false;
    if(usingItem&&impl->swing>=1) impl->swing=0;
    impl->swing=std::min(1.f,impl->swing+dt*3.6f);
    float swing=item==Bow||eating>0?0.f:std::sin(impl->swing*PI);
    int x=int(std::floor(camera.position.x)),y=int(std::floor(camera.position.y)),z=int(std::floor(camera.position.z));
    float sky=SkyLightAt(x,y,z)/24.f*(.06f+.94f*Daylight());
    float torch=(.72f*DirectBlockLightAt(x,y,z)+.28f*BlockLightAt(x,y,z))/24.f;
    float brightness=Settings.fullBright?1.f:.16f+.84f*std::clamp(std::max(sky,torch),0.f,1.f);
    Color tint=Shade(WHITE,brightness);
    if(!Settings.fullBright&&torch>sky) {tint.g=static_cast<unsigned char>(tint.g*.93f);tint.b=static_cast<unsigned char>(tint.b*.8f);}
    Camera3D handCamera{{0,0,0},{0,0,-1},{0,1,0},70,CAMERA_PERSPECTIVE};
    int width=GetScreenWidth(),height=GetScreenHeight();
    if(width<1||height<1)return;
    if(!impl->handLayer.id||impl->handLayer.texture.width!=width||impl->handLayer.texture.height!=height){
        if(impl->handLayer.id)UnloadRenderTexture(impl->handLayer);
        impl->handLayer=LoadRenderTexture(width,height);
    }
    if(!impl->handLayer.id)return;
    rlDrawRenderBatchActive();BeginTextureMode(impl->handLayer);ClearBackground(BLANK);BeginMode3D(handCamera);
    rlEnableDepthTest();rlEnableDepthMask();
    rlPushMatrix();
    float bowShake=bowDraw>=1.f?.0045f:bowDraw>=0&&bowDraw<=.1f?.0015f:0.f;
    // Anchor to the viewport edge instead of a fixed world-space X coordinate.
    float rightEdge=float(width)/height*std::tan(handCamera.fovy*.5f*DEG2RAD);
    float heldX=item==Bow?rightEdge*.9f-.27f:rightEdge*1.13f-.24f;
    if(Sword(item))heldX-=.08f*(1.f-swing);
    rlTranslatef(heldX+std::sin(float(GetTime())*45)*bowShake+std::sin(impl->handClock)*(.022f+.016f*impl->running)*impl->walking-.055f*swing,
                 (item==Bow?-.59f:-.77f)+std::cos(float(GetTime())*39)*bowShake+std::abs(std::sin(impl->handClock))*(.025f+.018f*impl->running)*impl->walking-.035f*swing-.25f*impl->equip,
                 (item==Bow?-1.25f:-1.f)-.16f*swing);
    if(eating>0){
        float lift=std::min(1.f,eating/.22f),bite=std::abs(std::sin(eating*PI*5.f));
        rlTranslatef(-heldX*.64f*lift,(.30f+.045f*bite)*lift,(.18f+.025f*bite)*lift);
        rlRotatef(-18*lift+7*bite,0,0,1);rlRotatef(-18*lift,1,0,0);
    }
    // Swing swords left around the wrist while pitching the blade forward.
    rlRotatef(item==Bow?-12.f:Sword(item)?-8.f*(1.f-swing)+90.f*swing:5-7*swing,0,0,1);
    rlRotatef(item==Bow?-5.f:Sword(item)?-5-34*swing:-16-34*swing,1,0,0);
    if(item==Air){
        auto lit=[&](Color c){return Color{static_cast<unsigned char>(c.r*tint.r/255),static_cast<unsigned char>(c.g*tint.g/255),static_cast<unsigned char>(c.b*tint.b/255),255};};
        DrawCube({0,-.15f,0},.23f,.64f,.24f,lit({32,126,144,255}));
        DrawCube({0,.22f,0},.23f,.12f,.24f,lit({180,125,87,255}));
        DrawCube({0,.36f,0},.24f,.23f,.25f,lit({199,146,105,255}));
        DrawCube({.116f,.36f,.035f},.015f,.16f,.07f,lit({175,118,80,255}));
    }
    if(item==Bow){rlPushMatrix();rlTranslatef(0,.36f,-.17f);rlRotatef(-40-8*std::clamp(bowDraw/.9f,0.f,1.f),0,1,0);rlScalef(.7f,.7f,.7f);DrawBowModel(bowDraw,tint,impl->atlas,BlockTile(Planks));rlPopMatrix();}
    Matrix itemTransform=MatrixIdentity();
    const bool drawItem=item!=Air&&impl->heldMesh.vertexCount;
    if(drawItem) {
        rlPushMatrix();
        bool sprite=!Placeable(item)||Plant(item);
        if(sprite) {rlTranslatef(Tool(item)||item==Stick?.065f:.08f,Sword(item)?.42f:Tool(item)||item==Stick?.54f:.49f,-.13f);rlRotatef(Sword(item)?-60.f+30.f*swing:-12.f,0,1,0);}
        else {rlTranslatef(-.025f,.28f,-.10f);rlRotatef(38,1,0,0);rlRotatef(-35,0,1,0);}
        float scale=sprite?(Sword(item)?.94f:Tool(item)||item==Stick?.66f:.53f):.50f;
        rlScalef(scale,scale,scale);
        impl->heldMaterial.maps[MATERIAL_MAP_DIFFUSE].texture=(finish||item==GoldenApple)?Texture2D{rlGetTextureIdDefault(),1,1,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8}:impl->atlas;
        impl->heldMaterial.maps[MATERIAL_MAP_DIFFUSE].color=Emission(item)?WHITE:tint;
        itemTransform=rlGetMatrixTransform();
        rlPopMatrix();
    }
    rlPopMatrix();rlDrawRenderBatchActive();
    if(drawItem){if(item==EnderPearl||item==Fireball){rlPushMatrix();rlMultMatrixf(MatrixToFloat(itemTransform));DrawProjectileItemModel(item,tint);rlPopMatrix();}else DrawMesh(impl->heldMesh,impl->heldMaterial,itemTransform);}
    if(eating>.22f){
        for(int chip=0;chip<5;++chip){
            float age=std::fmod(eating*5.f+chip*.19f,1.f);
            Vector3 p{.16f+(chip-2)*.038f+age*(chip%2?.10f:-.10f),-.08f-age*.24f,-.68f-age*.06f};
            DrawCube(p,.014f,.012f,.012f,Shade(chip%2?Color{255,220,75,255}:Color{222,150,13,255},brightness));
        }
    }
    EndMode3D();EndTextureMode();
    DrawTexturePro(impl->handLayer.texture,{0,0,float(width),-float(height)},{0,0,float(width),float(height)},{0,0},0,WHITE);
}

}
