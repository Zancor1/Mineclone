#include "settings.h"
#include "action_repeat.h"
#include "projectile_physics.h"
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
    Check(BowStage(0)==1&&BowStage(.1f)==1&&BowStage(.1001f)==2&&BowStage(.8f)==2&&BowStage(.899f)==2&&BowStage(.9f)==3,"three draw levels follow requested timing boundaries");Check(BowDamage(.05f,0)==1&&BowDamage(.5f,0)==2&&BowDamage(.9f,0)==4&&BowDamage(.999f,0)==4,"short medium and full charge damage");int crits=0;for(unsigned roll=0;roll<100;++roll){float damage=BowDamage(1.f,roll);if(damage==6)++crits;else Check(damage==4,"noncritical fully drawn shot remains two hearts");}Check(crits==10,"critical unlocks at one second with ten-percent chance");Check(BowPower(.05f)<BowPower(.5f)&&BowPower(.5f)<BowPower(.9f),"draw levels increase range and speed");
    Vector3 arrow{ArrowSpeed*.70710678f,ArrowSpeed*.70710678f,0},flight{};for(int i=0;i<200;++i){flight.x+=arrow.x*ProjectileTick;flight.y+=arrow.y*ProjectileTick;arrow=ArrowTickVelocity(arrow);if(flight.y<0)break;}Check(flight.x>110&&flight.x<130,"fully powered arrow has classic long-range trajectory");Check(BowPower(1)==1&&BowPower(.5f)<1,"bow charge controls projectile speed");auto firstArrow=ArrowTickVelocity({60,0,0});Check(std::abs(firstArrow.x-59.4f)<.001f&&firstArrow.y==-1,"arrow uses classic drag and gravity per twenty-Hz tick");
    PlacementRepeat repeat;Check(repeat.Update(true,true,.001f),"initial placement click is immediate");for(int i=0;i<12;++i)Check(repeat.Update(true,true,.01f),"individual placement clicks bypass held repeat delay");Check(!repeat.Update(false,true,.1f),"held placement waits for repeat interval");Check(repeat.Update(false,true,.11f),"held placement repeats after 200 ms");Check(!repeat.Update(false,false,.01f)&&repeat.Update(true,true,.001f),"releasing and clicking places immediately again");
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
    player.RestoreState(sprintPose);player.MeleeKnockback({11.05f,6.5f,0});for(int i=0;i<4;++i)player.Update({},.05f);auto regular=player.CaptureState();player.RestoreState(sprintPose);player.MeleeKnockback({28.6f,6.5f,0});for(int i=0;i<4;++i)player.Update({},.05f);auto enhanced=player.CaptureState();Check(std::abs((enhanced.position.x-sprintPose.position.x)-(28.6f/11.05f)*(regular.position.x-sprintPose.position.x))<.02f&&std::abs(enhanced.position.y-regular.position.y)<.001f,"knockback stick preserves horizontal impulse ratio without extra height");player.RestoreState(sprintPose);
    player.RestoreState(sprintPose);player.MeleeKnockback({28.6f,6.5f,0});for(int i=0;i<240;++i)player.Update({},1.f/120);float stickDistance=player.CaptureState().position.x-sprintPose.position.x;Check(stickDistance>6.1f&&stickDistance<6.9f,"stronger stick travels approximately six and a half blocks in open space");
    player.RestoreState(sprintPose);player.ExplosionKnockback({32,12,0});player.Update({},1.f/120);float initialBurst=player.CaptureState().position.x-sprintPose.position.x;Check(initialBurst>0&&initialBurst<.03f,"fireball accelerates gradually instead of teleporting horizontally");for(int i=1;i<240;++i)player.Update({},1.f/120);Check(player.CaptureState().position.x-sprintPose.position.x>14&&player.CaptureState().position.x-sprintPose.position.x<21,"smooth fireball preserves long range displacement");player.RestoreState(sprintPose);
    Settings.toggleSprint=false;run.sprintPressed=false;run.sprint=true;player.Update(run,0);Check(player.IsSprinting(),"held sprint works with toggle off");run.sprint=false;player.Update(run,0);Check(!player.IsSprinting(),"releasing sprint stops running when toggle off");player.RestoreState(sprintPose);
    player.RestoreState(sprintPose);player.MeleeKnockback({8.5f,5.4f,0});for(int i=0;i<4;++i)player.Update({},.05f);auto knocked=player.CaptureState();Check(knocked.velocityY<0.f&&knocked.position.x-sprintPose.position.x>.8f,"melee knockback descends quickly and travels farther horizontally");player.RestoreState(sprintPose);player.Knockback({8.5f,5.4f,0});for(int i=0;i<4;++i)player.Update({},.05f);Check(player.CaptureState().velocityY>0,"faster knockback gravity does not affect other impulses");player.RestoreState(sprintPose);
    player.RestoreState(sprintPose);for(int i=0;i<5;++i){player.MeleeKnockback({8.5f,5.4f,0});player.Update({},.05f);}Check(player.CaptureState().velocityY< -1.f,"repeated hits do not restart airtime and keep the player floating");player.RestoreState(sprintPose);
    for(int x=76;x<=84;++x)for(int z=70;z<=85;++z)Change(x,10,z,Stone);
    Player smooth;PlayerState start;start.valid=true;start.position={80.5f,11.001f,80.5f};smooth.RestoreState(start);PlayerInput walking;walking.forward=1;
    smooth.Update(walking,.05f);float first=80.5f-smooth.CaptureState().position.z;Check(first>0&&first<4.317f*.05f,"movement accelerates instead of snapping to full speed");
    for(int i=0;i<19;++i)smooth.Update(walking,.05f);
    auto slowFrames=smooth.CaptureState();float beforeStop=slowFrames.position.z;smooth.Update({},.05f);Check(smooth.CaptureState().position.z<beforeStop,"movement decelerates instead of stopping instantly");
    smooth.RestoreState(start);for(int i=0;i<100;++i)smooth.Update(walking,.01f);Check(std::abs(smooth.CaptureState().position.z-slowFrames.position.z)<.002f,"acceleration is consistent across frame rates");
    Player blast;PlayerState blastStart;blastStart.valid=true;blastStart.position={140,50,140};
    blast.RestoreState(blastStart);blast.Knockback({120,12,0});for(int i=0;i<40;++i)blast.Update({},.05f);
    Check(std::abs(blast.CaptureState().position.x-160.f)<.02f,"maximum fireball impulse travels twenty unobstructed blocks");
    blast.RestoreState(blastStart);blast.Knockback({120,12,0});for(int i=0;i<200;++i)blast.Update({},.01f);
    Check(std::abs(blast.CaptureState().position.x-160.f)<.02f,"fireball range stays consistent across frame rates");
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
    for(bool customQ:{false,true}){
        std::ofstream out(path);out<<"MINECLONE_SETTINGS 5\n0 1 720\n1 3 40\n\"Tester\" \"127.0.0.1\"\n70 "<<BindingCount-1<<'\n';
        for(int i=0;i<BindingCount-1;++i){out<<(i==0&&customQ?KEY_Q:DefaultBindings[i])<<' ';}out<<"\n1\n";out.close();
        Check(LoadSettings(path.string()),"previous binding count migrates without discarding settings");
        Check(Settings.bindings[int(BindingAction::Drop)]==(customQ?0:KEY_Q),"migration adds Q unless an existing custom binding already uses it");
    }
    std::filesystem::remove(path);
    std::puts("PASS: clock slider, freeze, continuing fluid ticks, sensitivity and persistent options");
}
