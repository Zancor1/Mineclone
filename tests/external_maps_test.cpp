#include "external_arenas.h"
#include "bedwars.h"
#include "renderer.h"
#include "lighting.h"
#include "settings.h"
#include <raymath.h>
#include <rlgl.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <set>
namespace {
void Check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void CheckStation(Vector3 p){
    using namespace game;
    Check(Inside(int(p.x),int(p.y),int(p.z)),"station inside active world");
    bool floor=false;auto under=CollisionBoxesAt(int(p.x),int(p.y)-1,int(p.z));
    for(int i=0;i<under.count;++i)floor|=std::abs(under.boxes[i].max.y-p.y)<.01f;
    Check(floor,"station has solid original floor");
    BoundingBox body{{p.x-.29f,p.y+.001f,p.z-.29f},{p.x+.29f,p.y+1.8f,p.z+.29f}};
    for(int y=int(p.y);y<=int(p.y+1.8f);++y){auto boxes=CollisionBoxesAt(int(p.x),y,int(p.z));for(int i=0;i<boxes.count;++i){if(CheckCollisionBoxes(body,boxes.boxes[i]))std::fprintf(stderr,"Station collision at %.1f %.1f %.1f, block %d\n",p.x,p.y,p.z,int(At(int(p.x),y,int(p.z))));Check(!CheckCollisionBoxes(body,boxes.boxes[i]),"station does not intersect imported geometry");}}
}
}
int main(int argc,char**){
    using namespace game;
    std::set<std::string> names;size_t total=0;
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"Imported BedWars maps");}
    for(int map=bw::FirstExternalArena;map<bw::MapCount;++map){
        const auto& arena=bw::ExternalArenaAt(map);const auto& cells=bw::ExternalCells(map);total+=arena.cells;
        Check(names.insert(arena.name).second,"unique imported map name");Check(cells.size()==size_t(arena.cells)*7,"complete compressed map resource");
        ClearRemoteWorld();bw::BuildArenaTerrain(map,16,[](int x,int y,int z,Block block){Check(Inside(x,y,z),"no original arena blocks clipped");if(Liquid(block))SetRemoteBlock(x,y,z,block);else World[x][y][z]=block;});InvalidateLighting();
        int sourceTop=0;for(size_t i=0;i<cells.size();i+=7){Check(At(cells[i],cells[i+1],cells[i+2])==Block(cells[i+3]),"all original blocks retained");sourceTop=std::max(sourceTop,int(cells[i+1]));if(cells[i+3]>=MapSlab){auto state=MapStateAt(cells[i],cells[i+1],cells[i+2]);Check(int(state.shape)==cells[i+5]&&state.material==Block(cells[i+4])&&state.data==cells[i+6],"shape material and metadata preserved");}}
        Check(bw::MapLobbyY(map)>sourceTop&&bw::MapLobbyY(map)+3<Height,"lobby above full original geometry");
        for(int team=0;team<8;++team){auto beds=bw::MapBeds(map,team,{});Check(At(int(beds[0].x),int(beds[0].y),int(beds[0].z))==Bed,"original team bed foot");Check(At(int(beds[1].x),int(beds[1].y),int(beds[1].z))==BedHead,"original team bed head");const auto& t=arena.teams[team];CheckStation(t.spawn);CheckStation(t.forge);CheckStation(t.vendors[0]);CheckStation(t.vendors[1]);Check(At(int(t.chest.x),int(t.chest.y),int(t.chest.z))==Chest,"functional chest matches original location");}
        Check(arena.diamonds.size()==4&&!arena.emeralds.empty(),"configured resource islands");for(auto p:arena.diamonds)CheckStation(p);for(auto p:arena.emeralds)CheckStation(p);
        Check(bw::Host("MapTester",0,16,map),"external map starts as LAN host");Check(bw::Generators().size()==16+arena.diamonds.size()+arena.emeralds.size(),"host uses original map resource stations");
        for(size_t i=0;i<cells.size();i+=7){Check(At(cells[i],cells[i+1],cells[i+2])==Block(cells[i+3]),"starting a match preserves every original imported block");if(cells[i+3]>=MapSlab){auto shape=MapStateAt(cells[i],cells[i+1],cells[i+2]);Check(shape.material==Block(cells[i+4])&&int(shape.shape)==cells[i+5]&&shape.data==cells[i+6],"generators preserve original slab states");}}
        for(const auto& station:bw::Generators()){int x=int(station.position.x),y=int(std::ceil(station.position.y))-1,z=int(station.position.z);auto boxes=CollisionBoxesAt(x,y,z);bool supported=false;for(int i=0;i<boxes.count;++i)supported|=std::abs(boxes.boxes[i].max.y-station.position.y)<.001f;Check(supported,"imported pickup stands on the original surface");if(station.item==IronIngot||station.item==GoldIngot){if(MapStateAt(x,y,z).shape!=MapShape::Slab)std::fprintf(stderr,"Forge without slabs: %s at %.1f %.1f %.1f\n",arena.name,station.position.x,station.position.y,station.position.z);Check(MapStateAt(x,y,z).shape==MapShape::Slab,"imported base forge uses existing slabs");}}
        if(argc>1){Renderer renderer;Check(renderer.Initialize(),"external map renderer");Settings.fogEnabled=false;Settings.renderChunks=MaxRenderChunks;Camera3D view{{128,185,228},{128,float(8+sourceTop)*.5f,128},{0,1,0},75,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(view,{});rlDrawRenderBatchActive();auto image=LoadImageFromScreen();Color sky=SkyColor();auto pixels=LoadImageColors(image);int drawn=0;for(int i=0;i<image.width*image.height;i+=4)if(std::abs(int(pixels[i].r)-sky.r)+std::abs(int(pixels[i].g)-sky.g)+std::abs(int(pixels[i].b)-sky.b)>20)++drawn;UnloadImageColors(pixels);Check(drawn>2000,"imported geometry appears in framebuffer");auto file=std::string("build/map-")+arena.name+".png";ExportImage(image,file.c_str());UnloadImage(image);EndDrawing();auto preview=bw::CreateMapPreview(map,16);Check(preview.id!=0,"imported map selection preview");UnloadRenderTexture(preview);}
        bw::Stop();
        std::printf("PASS: %s (%u original blocks)\n",arena.name,arena.cells);
    }
    if(argc>1)CloseWindow();
    Check(names.size()==16&&total>1700000,"all sixteen supplied maps imported");
}
