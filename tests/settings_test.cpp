#include "settings.h"
#include "world.h"
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
    for(int minute=0;minute<1440;++minute) {
        SetClockMinute(minute);
        Check(ClockMinute()==minute,"all 1440 slider minutes round trip");
    }
    SetClockMinute(720);Check(Daylight()>.99f,"12:00 corresponds to midday");
    SetClockMinute(0);Check(Daylight()<.01f,"00:00 corresponds to midnight");
    SetClockMinute(720);Settings.freezeTime=true;
    Change(8,8,8,Water);
    const float stopped=WorldTime;
    UpdateWorld(.15f);
    Check(WorldTime==stopped,"clock remains fixed when locked");
    Check(At(8,7,8)==Water,"locking time does not stop water simulation");
    for(int i=0;i<50;++i) UpdateWorld(.05f);
    Check(WorldTime==stopped,"clock stays locked across repeated updates");
    Settings.freezeTime=false;UpdateWorld(.25f);
    Check(WorldTime>stopped,"unlock resumes time");
    PlayerState pose;pose.position={32,30,32};pose.valid=true;
    Player player;Check(player.RestoreState(pose),"valid player setup");
    PlayerInput input;input.look={20,10};
    Settings.mouseSensitivity=.5f;player.Update(input,0);
    float slowYaw=player.CaptureState().yaw,slowPitch=player.CaptureState().pitch;
    player.RestoreState(pose);Settings.mouseSensitivity=2.f;player.Update(input,0);
    Check(std::abs(player.CaptureState().yaw-slowYaw*4)<1e-6f&&
          std::abs(player.CaptureState().pitch-slowPitch*4)<1e-6f,
          "sensitivity proportionally scales both camera axes");
    auto path=std::filesystem::temp_directory_path()/
        ("mineclone-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".cfg");
    Settings.freezeTime=true;Settings.mouseSensitivity=1.75f;Settings.newWorldMinute=1439;
    Check(SaveSettings(path.string()),"save options");
    Settings=GameSettings{};
    Check(LoadSettings(path.string())&&Settings.freezeTime&&Settings.mouseSensitivity==1.75f&&
          Settings.newWorldMinute==1439,"options survive reload");
    {std::ofstream out(path);out<<"MINECLONE_SETTINGS 1\n1 999 2500\n";}
    Check(!LoadSettings(path.string())&&Settings.mouseSensitivity==1.75f,"invalid config preserves current settings");
    std::filesystem::remove(path);
    std::puts("PASS: clock slider, freeze, continuing fluid ticks, sensitivity and persistent options");
}
