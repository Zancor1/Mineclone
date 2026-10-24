#include "progression.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace game::progression;
void Check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("mineclone-progression-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
#ifdef _WIN32
    if(const char* appdata=std::getenv("APPDATA"))Check(DefaultDirectory()==std::filesystem::path(appdata)/"MineClone"/"profiles","Windows profiles use the user AppData directory");
#endif
    Profile p;std::string error;
    Check(Load("Player",p,error,dir)&&p.Level()==1&&p.Remaining()==50,"new nickname starts at level one");
    for(int i=0;i<9;++i)p.Award(5);
    Check(p.xp==45&&p.Level()==1&&!p.crates&&!p.pending,"kills give five XP");p.Award(5);
    Check(p.Level()==2&&p.pending==1&&p.crates==0&&p.Remaining()==50,"level reward stays pending during match");
    Check(p.Open(0)==-1,"pending rewards cannot be opened before lobby");
    for(int i=0;i<5;++i)p.Award(10);
    Check(p.Level()==3&&p.pending==2&&p.xp==100,"level three requires only fifty more XP");
    Check(Save("Player",p,error,dir),"save pending rewards before disconnect");Profile loaded;
    Check(Load("pLaYeR",loaded,error,dir)&&loaded.pending==2,"pending rewards survive restart and nickname case");
    Check(loaded.Claim()==2&&loaded.Claim()==0&&loaded.crates==2,"lobby delivers each reward exactly once");
    p.Award(150);Check(p.Level()==6&&p.pending==5&&!p.crates,"multi-level rewards remain pending");p.Claim();
    int reward=p.Open(0);Check(RewardCategory(reward)==0&&RewardColor(reward)==1,"crate draws a specific item category and color");
    Check(p.Equip(0,1)&&p.skins[0]==1&&p.skins[1]==0&&p.skins[2]==0,"sword finish does not affect pickaxes or axes");
    Check(!p.Equip(1,1)&&!p.Equip(0,12)&&!p.Equip(3,1)&&!p.Equip(0,-1),"locked or invalid finishes cannot be equipped");
    p.NextSkin(0);Check(p.skins[0]==0,"can return to default appearance");
    Check(Save("Player",p,error,dir)&&Load("player",loaded,error,dir)&&loaded.unlocked==p.unlocked&&loaded.skins==p.skins,"item finishes persist by nickname");
    Check(std::filesystem::exists(SkinDirectory(dir)/"player"/"tools.skin"),"local skin inventory manifest saved with profile");
    auto cached=p;cached.skins[0]=0;cached.crates+=1;auto reconciled=Reconcile(p,cached);Check(reconciled.crates==p.crates&&reconciled.skins==p.skins,"same-XP cache cannot restore a spent crate or overwrite local skin choice");auto newer=p;newer.Award(50);Check(Reconcile(p,newer).xp==newer.xp,"older local backup recovers newer host progress");
    Check(!Save("../escape",p,error,dir),"nickname cannot escape profile directory");
    {std::ofstream out(dir/"player.profile");out<<"MINECLONE_PROFILE 1\n100 1 7 2\n";}
    Check(Load("Player",loaded,error,dir)&&loaded.xp==100&&loaded.crates==1&&loaded.pending==0&&loaded.skins[2]==2&&loaded.unlocked[1]==7,"legacy character colors migrate to tools without extra crates");
    {std::ofstream out(dir/"player.profile");out<<"MINECLONE_PROFILE 2\n100 999 0 1 0 1 0 1 0\n";}
    Check(!Load("Player",loaded,error,dir),"invalid profiles are rejected instead of reset");
    Profile all;all.Award(300*50);all.Claim();int frequencies[3][SkinCount]{};
    for(unsigned roll=0;roll<300;++roll){int r=all.Open(roll);++frequencies[RewardCategory(r)][RewardColor(r)-1];}
    constexpr int weights[]={12,12,12,12,12,12,7,7,7,3,3,1};
    for(int type=0;type<3;++type){Check(all.unlocked[type]==(1u<<(SkinCount+1))-1,"all 36 item finishes are obtainable");for(int color=0;color<SkinCount;++color)Check(frequencies[type][color]==weights[color],"published loot probabilities match all 300 outcomes");}
    Check(all.Open(0)==-1,"empty crate cannot be opened repeatedly");
    std::filesystem::remove_all(dir);std::cout<<"PASS: lobby rewards, persistent pending crates, 36 tool finishes, weighted drops and profile migration\n";
}
