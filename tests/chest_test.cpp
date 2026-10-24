#include "chest.h"
#include "world.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
namespace {
void Check(bool ok,const char* text) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
}
int main() {
    using namespace game;
    GenerateWorld(3197);Change(8,77,8,Chest);
    auto* slots=ChestAt(8,77,8);Check(slots!=nullptr,"placed chest opens storage");
    Hotbar[0]={Grass,64};CursorStack={};
    ClickStack(Hotbar[0],false);ClickStack((*slots)[0],false);
    Check(!Hotbar[0].count&&Hotbar[0].block==Air&&(*slots)[0].block==Grass&&(*slots)[0].count==64&&!CursorStack.count,
          "depositing a hotbar stack removes it from the source slot without copying");
    TransferStack((*slots)[0],Hotbar.data(),Hotbar.size());
    Check(!(*slots)[0].count&&Hotbar[0].count==64,"reverse quick transfer moves the original stack back");
    for(auto& slot:*slots) slot={Stone,64};
    (*slots)[0].count=60;Backpack[1]={Stone,10};
    Check(TransferStack(Backpack[1],slots->data(),slots->size())==4&&Backpack[1].count==6&&(*slots)[0].count==64,
          "partially full chest moves only available capacity and retains remainder");
    Backpack[1]={};*slots={};
    (*slots)[0]={Stone,60};CursorStack={Stone,10};ClickStack((*slots)[0],false);
    Check((*slots)[0].count==64&&CursorStack.count==6,"merging respects stack limit without losing remainder");
    ClickStack((*slots)[1],true);
    Check((*slots)[1].count==1&&CursorStack.count==5,"right click deposits one item");
    CursorStack={};(*slots)[2]={Bricks,9};ClickStack((*slots)[2],true);
    Check(CursorStack.count==5&&(*slots)[2].count==4,"right click takes half rounded up");
    (*slots)[3]={BlackWool,3};ClickStack((*slots)[3],false);
    Check(CursorStack.block==BlackWool&&CursorStack.count==3&&(*slots)[3].block==Bricks&&(*slots)[3].count==5,
          "different stacks swap without changing counts");
    CursorStack={};*slots={};(*slots)[0]={RedWool,64};(*slots)[1]={Water,12};(*slots)[26]={Chest,3};
    Backpack[0]={Dirt,10};Hotbar[1]={BlueWool,13};CursorStack={Lava,2};
    auto folder=std::filesystem::temp_directory_path()/
        ("mineclone-chests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);std::string save=(folder/"world.dat").string();
    PlayerState pose;pose.valid=true;pose.flying=true;pose.position={Width*.5f+26.5f,76,Width*.5f+.5f};
    Check(SaveWorld(save,pose),"save chest, backpack and cursor");
    GenerateWorld(9381);
    Check(LoadWorld(save,&pose),"load chest save");slots=ChestAt(8,77,8);
    Check(slots&&(*slots)[0].block==RedWool&&(*slots)[0].count==64&&(*slots)[26].count==3&&
          Backpack[0].block==Dirt&&Backpack[0].count==10&&Hotbar[1].block==BlueWool&&Hotbar[1].count==13&&CursorStack.block==Lava&&CursorStack.count==2,
          "world reload restores exact chest, backpack and cursor stacks");
    bool shifted=false;
    Check(StreamWorld(pose,save,shifted)&&shifted,"stream away from chest");
    pose.position.x=Width*.5f-24.5f;
    Check(StreamWorld(pose,save,shifted)&&shifted,"return to chest region");
    slots=ChestAt(8,77,8);
    Check(slots&&(*slots)[1].block==Water&&(*slots)[1].count==12,"offloaded chest contents survive region changes");
    for(auto& slot:Backpack) slot={Stone,64};
    for(auto& slot:Hotbar) slot={Stone,64};
    Change(8,77,8,Air);
    Check(At(8,77,8)==Chest&&(*ChestAt(8,77,8))[0].count==64,
          "breaking a full chest with full backpack cannot delete contents");
    Backpack={};Change(8,77,8,Air);
    int recovered=0;for(auto slot:Backpack) recovered+=slot.count;
    Check(At(8,77,8)==Air&&recovered==79,"breaking chest recovers its complete contents");
    Change(8,77,8,Chest);slots=ChestAt(8,77,8);
    for(auto slot:*slots) Check(slot.count==0,"replaced chest never resurrects old items");
    auto bytes=SerializeChests();auto damaged=bytes;damaged[5]=255;
    Check(!DeserializeChests(damaged,true)&&SerializeChests()==bytes,"invalid container data preserves current inventory");
    (*slots)[0]={Planks,11};CursorStack={};ClickStack((*slots)[0],false,true);
    Check((*slots)[0].count==0,"Shift click transfers chest stack to backpack");
    auto oldContainers=SerializeChests();
    oldContainers.erase(oldContainers.begin()+4+27*4,oldContainers.begin()+4+36*4);
    Check(DeserializeChests(oldContainers,true,true)&&Hotbar[0].block==Grass&&Hotbar[0].count==64,
          "version-eight container saves keep chest contents and initialize a real hotbar");
    ClearChests();Check(!CursorStack.count&&!Backpack[0].count,"new world clears player container state");
    std::filesystem::remove_all(folder);
    std::puts("PASS: chest stacks, transfers, save/load, region persistence and safe breaking");
}
