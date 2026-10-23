#include "world.h"
#include "settings.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
namespace {
void Check(bool ok,const char* message) {
    if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
}
int main() {
    using namespace game;
    // Fly, hover, collide and return to gravity using two separate Space presses.
    Player player;PlayerState pose;pose.position={64.5f,75.f,64.5f};pose.valid=true;
    Check(player.RestoreState(pose),"valid initial flight pose");
    PlayerInput input;input.jump=true;input.swimUp=true;player.Update(input,.05f);
    input.jump=false;input.swimUp=false;player.Update(input,.05f);
    input.jump=true;input.swimUp=true;player.Update(input,.05f);
    Check(player.CaptureState().flying,"double Space enables flight");
    float airborne=player.CaptureState().position.y;
    input.jump=false;input.swimUp=false;player.Update(input,.1f);
    Check(player.CaptureState().position.y==airborne,"flight hovers without gravity");
    input.swimDown=true;player.Update(input,.1f);
    Check(player.CaptureState().position.y<airborne,"Shift descends while flying");
    input.swimDown=false;input.jump=true;input.swimUp=true;player.Update(input,.05f);
    input.jump=false;input.swimUp=false;player.Update(input,.05f);
    input.jump=true;player.Update(input,.05f);
    Check(!player.CaptureState().flying,"second double Space disables flight");

    pose.flying=true;player.RestoreState(pose);
    Change(65,75,64,Stone);input={};input.side=1;
    player.Update(input,.1f);
    Check(player.CaptureState().position.x<=65-Radius-Skin+.0001f&&player.CaptureState().flying,
          "creative flight collides with walls instead of clipping through blocks");
    const float beforeRebase=player.CaptureState().position.x;
    player.Translate({-32,0,0});
    Check(std::abs(player.CaptureState().position.x-(beforeRebase-32))<.0001f&&player.CaptureState().flying,
          "coordinate rebase preserves creative flight");
    auto folder=std::filesystem::temp_directory_path()/
        ("mineclone-stream-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    std::string save=(folder/"world.dat").string();
    GenerateWorld(47183);WorldTime=412.5f;
    Change(8,77,8,BlackWool);Change(7,77,8,Lava);Change(9,77,8,Water);
    Change(72,77,64,PinkWool);
    pose.position={90.5f,76.f,64.5f};pose.flying=true;
    Check(SaveWorld(save,pose),"save initial edited world");
    bool shifted=false;
    Check(StreamWorld(pose,save,shifted)&&shifted,"crossing margin streams next chunks");
    Check(RegionOriginX==32&&RegionOriginZ==0&&pose.position.x==58.5f,"world and player rebase together");
    Check(At(40,77,64)==PinkWool,"overlapping constructions survive recenter");
    Check(WorldTime==412.5f&&pose.flying,"streaming preserves clock and flight");
    PlayerState loaded;
    Check(LoadWorld(save,&loaded)&&RegionOriginX==32&&loaded.position.x==58.5f&&loaded.flying,
          "origin, player and flight survive reload");
    pose=loaded;pose.position.x=39.5f;
    Check(StreamWorld(pose,save,shifted)&&shifted&&RegionOriginX==0,"return streams archived chunks");
    Check(At(8,77,8)==BlackWool&&IsLavaSource(7,77,8)&&IsWaterSource(9,77,8),
          "return restores offloaded blocks and fluid sources");
    pose.position={39.5f,76.f,39.5f};
    Check(StreamWorld(pose,save,shifted)&&RegionOriginX==-32&&RegionOriginZ==-32,
          "streaming supports negative origins and diagonal travel");
    Check(At(40,77,40)==BlackWool,"diagonal overlap maps old coordinates correctly");
    Check(LoadWorld(save,&loaded)&&RegionOriginX==-32&&RegionOriginZ==-32,
          "negative origins survive save round trip");
    // Revisit the same coordinate after unload: generation and edits remain stable.
    pose=loaded;pose.position={90.5f,76.f,90.5f};
    Check(StreamWorld(pose,save,shifted)&&RegionOriginX==0&&RegionOriginZ==0,
          "return from negative region restores original origin");
    Check(At(8,77,8)==BlackWool,"repeated visits preserve edits");
    // A broken archive is reported before replacing the active in-memory world.
    auto archive=std::filesystem::path(save+".chunks")/"-2_0.chunk";
    Check(std::filesystem::exists(archive),"negative region archive exists");
    {std::ofstream corrupt(archive,std::ios::trunc);corrupt<<"broken";}
    pose.position.x=39.5f;shifted=false;
    Check(!StreamWorld(pose,save,shifted)&&!shifted&&RegionOriginX==0&&At(8,77,8)==BlackWool,
          "corrupt archive cannot replace active world");
    std::filesystem::remove_all(folder);
    std::puts("PASS: creative flight, region streaming, negative coordinates, construction persistence and corrupt archives");
}
