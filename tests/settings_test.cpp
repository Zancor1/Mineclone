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
    Settings.playerName="Zancor_1";Settings.serverAddress="127.0.0.1:25566";
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
    Check(Settings.toggleSprint,"toggle sprint enabled by default");
    auto sprintPose=player.CaptureState();PlayerInput run;run.forward=1;run.sprintPressed=true;player.Update(run,0);Check(player.IsSprinting(),"one press enables sprint");run.sprintPressed=false;run.sprint=false;player.Update(run,0);Check(player.IsSprinting(),"sprint remains enabled after releasing Ctrl");player.SuspendSprint(.1f);player.Update(run,.05f);Check(!player.IsSprinting(),"confirmed hit interrupts sprint for first 50 ms");player.Update(run,.05f);Check(!player.IsSprinting(),"sprint remains interrupted for 100 ms");player.Update(run,0);Check(player.IsSprinting(),"toggle sprint resumes after hit pause");run.swimDown=true;player.Update(run,0);Check(!player.IsSprinting(),"sneaking suppresses automatic sprint");run.swimDown=false;player.Update(run,0);Check(player.IsSprinting(),"sprint resumes after releasing sneak");run.sprintPressed=true;player.Update(run,0);Check(!player.IsSprinting(),"second press disables toggle sprint");
    player.RestoreState(sprintPose);player.MeleeKnockback({6.8f,6,0});for(int i=0;i<4;++i)player.Update({},.05f);auto regular=player.CaptureState();player.RestoreState(sprintPose);player.MeleeKnockback({13.6f,6,0});for(int i=0;i<4;++i)player.Update({},.05f);auto enhanced=player.CaptureState();Check(std::abs((enhanced.position.x-sprintPose.position.x)-2*(regular.position.x-sprintPose.position.x))<.02f&&std::abs(enhanced.position.y-regular.position.y)<.001f,"knockback stick doubles horizontal displacement without extra height");player.RestoreState(sprintPose);
    Settings.toggleSprint=false;run.sprintPressed=false;run.sprint=true;player.Update(run,0);Check(player.IsSprinting(),"held sprint works with toggle off");run.sprint=false;player.Update(run,0);Check(!player.IsSprinting(),"releasing sprint stops running when toggle off");player.RestoreState(sprintPose);
    player.RestoreState(sprintPose);player.MeleeKnockback({8.5f,5.4f,0});for(int i=0;i<4;++i)player.Update({},.05f);auto knocked=player.CaptureState();Check(knocked.velocityY<0.f&&knocked.position.x-sprintPose.position.x>.8f,"melee knockback descends quickly and travels farther horizontally");player.RestoreState(sprintPose);player.Knockback({8.5f,5.4f,0});for(int i=0;i<4;++i)player.Update({},.05f);Check(player.CaptureState().velocityY>0,"faster knockback gravity does not affect other impulses");player.RestoreState(sprintPose);
    player.RestoreState(sprintPose);for(int i=0;i<5;++i){player.MeleeKnockback({8.5f,5.4f,0});player.Update({},.05f);}Check(player.CaptureState().velocityY< -1.f,"repeated hits do not restart airtime and keep the player floating");player.RestoreState(sprintPose);
    auto path=std::filesystem::temp_directory_path()/
        ("mineclone-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".cfg");
    Settings.freezeTime=true;Settings.mouseSensitivity=1.75f;Settings.newWorldMinute=1439;
    Settings.fogEnabled=false;Settings.renderChunks=6;Settings.fogDistance=137.f;
    Check(FogDensity(false)==0&&FogDensity(true)>.1f,"disabling outdoor fog preserves underwater visibility limit");
    Check(RenderDistance()==96,"render distance is measured in chunks");
    Settings.fov=96;Check(SetBinding(BindingAction::Forward,KEY_UP),"remap movement");Check(!SetBinding(BindingAction::Backward,KEY_UP),"reject duplicate bindings");Check(SetBinding(BindingAction::Use,-3)&&SetBinding(BindingAction::Attack,-2),"remap mouse actions");Check(!SetBinding(BindingAction::Use,-99),"reject invalid mouse button");
    Check(player.Camera().fovy==96,"FOV setting updates camera");
    Check(SaveSettings(path.string()),"save options");
    Settings=GameSettings{};
    Check(LoadSettings(path.string())&&Settings.freezeTime&&Settings.mouseSensitivity==1.75f&&
          Settings.newWorldMinute==1439&&!Settings.fogEnabled&&Settings.renderChunks==6&&Settings.fogDistance==137.f,"options survive reload");
    Check(!Settings.toggleSprint&&Settings.fov==96&&Settings.bindings[int(BindingAction::Forward)]==KEY_UP&&Settings.bindings[int(BindingAction::Use)]==-3&&Settings.bindings[int(BindingAction::Attack)]==-2,"FOV and keyboard/mouse bindings survive reload");
    Check(Settings.playerName=="Zancor_1"&&Settings.serverAddress=="127.0.0.1:25566","multiplayer nickname and address persist");
    {std::ofstream out(path);out<<"MINECLONE_SETTINGS 1\n1 999 2500\n";}
    Check(!LoadSettings(path.string())&&Settings.mouseSensitivity==1.75f,"invalid config preserves current settings");
    {std::ofstream out(path);out<<"MINECLONE_SETTINGS 1\n0 1 720\n";}
    Check(LoadSettings(path.string())&&Settings.fogEnabled&&Settings.renderChunks==3,"old settings migrate with fog defaults");
    Check(Settings.toggleSprint&&Settings.fov==70&&Settings.bindings==DefaultBindings,"legacy configs receive default FOV and bindings");
    Settings.bindings[int(BindingAction::Forward)]=KEY_UP;Settings.bindings=DefaultBindings;Check(Settings.bindings[int(BindingAction::Forward)]==KEY_W,"reset restores default keys");
    std::filesystem::remove(path);
    std::puts("PASS: clock slider, freeze, continuing fluid ticks, sensitivity and persistent options");
}
