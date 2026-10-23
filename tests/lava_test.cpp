#include "renderer.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
namespace {
void Check(bool ok,const char* message) {
    if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
}
int main() {
    using namespace game;
    Change(10,10,10,Water);Change(20,10,20,Lava);
    UpdateWorld(.15f);
    Check(WaterLevelAt(10,9,10)==7&&At(20,9,20)==Air,"water falls at 150 ms while lava waits");
    UpdateWorld(.15f);
    Check(WaterLevelAt(10,8,10)==7&&LavaLevelAt(20,9,20)==7&&At(20,8,20)==Air,
          "lava advances once for every two water ticks");
    Check(!IsLavaSource(20,9,20)&&IsLavaSource(20,10,20),"falling lava is not a source");
    Change(30,7,30,Stone);Change(31,7,30,Stone);Change(31,8,30,Water);Change(30,8,30,Lava);
    UpdateWorld(.15f);UpdateWorld(.15f);
    Check(At(30,8,30)==Lava&&At(31,8,30)==Water,"fluids do not overwrite each other");
    Change(50,7,50,Stone);Change(51,7,50,Stone);
    Change(50,8,49,Stone);Change(50,8,51,Stone);Change(49,8,50,Stone);
    Change(51,8,49,Stone);Change(51,8,51,Stone);Change(52,8,50,Stone);
    Change(50,8,50,Lava);UpdateLava(.15f);UpdateLava(.15f);
    Check(LavaLevelAt(51,8,50)==6,"lava uses decreasing horizontal levels");
    WaterFace top=BuildFluidFace(51,8,50,2);
    Check(top.visible&&std::abs(top.vertices[0].y-(8+6.f/7.f))<1e-5f,
          "lava mesh follows its level exactly like water");
    PlayerState pose;pose.valid=true;pose.position={8,20,8};
    auto path=std::filesystem::temp_directory_path()/
        ("mineclone-lava-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dat");
    Check(SaveWorld(path.string(),pose),"save active lava");
    Change(50,8,50,Air);Change(51,8,50,Air);
    Check(LoadWorld(path.string())&&IsLavaSource(50,8,50)&&!IsLavaSource(51,8,50)&&LavaLevelAt(51,8,50)==6,
          "lava levels and source flags survive reload");
    // Reconstruct an older save containing the original static lava block.
    std::ifstream input(path,std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(input)),{});input.close();
    data.resize(data.size()-12);data[6]=5;
    constexpr size_t cells=size_t(Width)*Height*Width;
    for(size_t i=0;i<cells;++i) if(data[24+i]==Lava) {data[24+cells+i]=0;data[24+2*cells+i]=0;}
    uint32_t checksum=2166136261u;
    for(size_t i=24;i<data.size();++i) checksum=(checksum^data[i])*16777619u;
    for(int i=0;i<4;++i) data[20+i]=static_cast<unsigned char>(checksum>>(8*i));
    {std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<char*>(data.data()),data.size());}
    Check(LoadWorld(path.string())&&IsLavaSource(50,8,50)&&LavaLevelAt(51,8,50)==7,
          "legacy static lava migrates to active sources");
    std::filesystem::remove(path);
    for(Block fluid:{Water,Lava}) {
        Change(70,7,70,Stone);Change(70,8,70,fluid);
        Hit floor=Trace({70.5f,11.5f,70.5f},{0,-1,0});
        Check(floor.found&&floor.y==7&&floor.y+floor.ny==8,
              "normal building ray passes through liquids to floor and replaces liquid cell");
        Hit liquid=Trace({70.5f,11.5f,70.5f},{0,-1,0},true);
        Check(liquid.found&&liquid.y==8,"Alt ray can still select either liquid directly");
        Change(70,8,70,Stone);
        Check(FluidLevelAt(70,8,70)==0&&!IsFluidSource(70,8,70),
              "replacing liquid with a block clears fluid metadata");
    }
    std::puts("PASS: half-speed lava, mixed fluids, level meshes, persistence and legacy migration");
}
