#include "lighting.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <chrono>
namespace {
void Check(bool ok,const char* what) {
    if(!ok) {std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}
}
}
int main() {
    using namespace game;
    std::memset(World,0,sizeof(World));InvalidateLighting();UpdateLighting();
    Check(SkyLightAt(16,20,16)==24,"open sky has full skylight");
    Change(15,20,15,LampOn);UpdateLighting();
    Check(BlockLightAt(15,20,15)==24,"lamp emits reference level 24");
    Check(BlockLightAt(16,20,15)==23,"lamp crosses chunk boundary with attenuation");
    Check(BlockLightAt(20,20,15)==19,"linear attenuation through air");
    Check(DirectBlockLightAt(17,20,15)==22,"unoccluded lamp has direct light");
    Change(16,20,15,Stone);UpdateLighting();
    Check(BlockLightAt(17,20,15)>0,"indirect light can travel around an obstacle");
    Check(DirectBlockLightAt(17,20,15)==0,"single opaque block casts direct lamp shadow");
    Check(DirectBlockLightAt(15,20,17)>0,"unobstructed side stays illuminated");
    Change(16,20,15,Air);UpdateLighting();
    Check(DirectBlockLightAt(17,20,15)==22,"removing blocker restores direct light");
    VertexLight sharedA=SampleCornerLighting(18,20,18,1,1,1);
    VertexLight sharedB=SampleCornerLighting(19,20,18,1,1,-1);
    Check(std::abs(sharedA.block-sharedB.block)<1e-6f&&std::abs(sharedA.sky-sharedB.sky)<1e-6f,
          "adjacent coplanar faces share identical light at their common vertex");
    Check(std::abs(sharedA.block-17.f/24.f)<1e-5f,
          "vertex light averages the four neighboring attenuation levels");
    Change(18,20,19,Stone);Change(19,20,18,Stone);UpdateLighting();
    VertexLight corner=SampleCornerLighting(18,20,18,1,1,1);
    Check(corner.ambient>=.7f&&corner.ambient<1.f,"corner occlusion is soft rather than black");
    Change(18,20,19,Air);Change(19,20,18,Air);UpdateLighting();
    // Seal the emitter so no alternative path can go around the obstruction.
    Change(14,20,15,Stone);Change(16,20,15,Stone);
    Change(15,19,15,Stone);Change(15,21,15,Stone);
    Change(15,20,14,Stone);Change(15,20,16,Stone);UpdateLighting();
    Check(BlockLightAt(17,20,15)==0,"opaque enclosure blocks light");
    Change(16,20,15,Glass);UpdateLighting();
    Check(BlockLightAt(17,20,15)==22,"glass transmits light");
    Check(DirectBlockLightAt(17,20,15)==22,"glass does not cast an opaque lamp shadow");
    Change(16,20,15,Water);UpdateLighting();
    Check(BlockLightAt(17,20,15)==22,"water transmits light without solid collision");
    Check(!Solid(Water),"water remains noncollidable");
    Change(15,20,15,LampOff);UpdateLighting();
    Check(BlockLightAt(17,20,15)==0,"switching lamp off removes propagated light");
    Change(15,20,15,LampOn);UpdateLighting();Change(15,20,15,Air);UpdateLighting();
    Check(BlockLightAt(17,20,15)==0,"removing emitter clears stale light");
    WorldTime=DayDuration*.25f;Check(Daylight()>.99f,"daytime reaches full intensity");
    UpdateWorld(DayDuration*.5f);Check(Daylight()<.01f,"simulation advances from midday to midnight");
    Check(SkyColor().r<30,"night sky is dark");
    const float savedTime=WorldTime;
    PlayerState pose;pose.valid=true;pose.position={3.25f,27.5f,41.75f};pose.yaw=.6f;pose.pitch=-.4f;
    auto path=std::filesystem::temp_directory_path()/
        ("mineclone-lighting-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dat");
    Check(SaveWorld(path.string(),pose),"save world clock and player");
    WorldTime=0;PlayerState loaded;
    Check(LoadWorld(path.string(),&loaded),"load updated save format");
    std::filesystem::remove(path);
    Check(WorldTime==savedTime&&loaded.position.x==pose.position.x&&loaded.yaw==pose.yaw,"time and player round trip exactly");
    for(int b=1;b<BlockCount;++b) Check(Names[b]&&Names[b][0],"every block has inventory label");
    std::printf("PASS: lighting propagation, occlusion, removal, day/night, saves and %d inventory blocks\n",int(BlockCount)-1);
}
