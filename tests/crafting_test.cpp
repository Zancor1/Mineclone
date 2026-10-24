#include "crafting.h"
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
    PersonalCrafting[3]={Wood,2};
    Check(CraftResult(PersonalCrafting.data(),2).block==Planks&&TakeCraft(PersonalCrafting.data(),2)&&
          CursorStack.count==4&&PersonalCrafting[3].count==1,"log gives four planks and consumes one log");
    PersonalCrafting={};CursorStack={};PersonalCrafting[1]={Planks,3};PersonalCrafting[3]={AcaciaPlanks,2};
    Check(TakeCraft(PersonalCrafting.data(),2)&&CursorStack.block==Stick&&CursorStack.count==4&&
          PersonalCrafting[1].count==2&&PersonalCrafting[3].count==1,"vertical planks make four sticks with offset recipe");
    PersonalCrafting={};CursorStack={};for(auto& slot:PersonalCrafting) slot={Planks,1};
    Check(TakeCraft(PersonalCrafting.data(),2)&&CursorStack.block==CraftingTable&&CursorStack.count==1,
          "four planks make one crafting table");
    for(Block material:{Planks,Cobblestone,IronIngot}) for(int column=0;column<3;++column) {
        WorkbenchCrafting={};CursorStack={};
        WorkbenchCrafting[column]={material,2};WorkbenchCrafting[column+3]={material,2};WorkbenchCrafting[column+6]={Stick,2};
        Block sword=material==Planks?WoodenSword:(material==Cobblestone?StoneSword:IronSword);
        Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==sword&&CursorStack.count==1,
              "two matching materials above stick craft correct sword in every column");
        Check(!TakeCraft(WorkbenchCrafting.data(),3)&&WorkbenchCrafting[column].count==1,
              "full sword cursor prevents ingredient consumption");
        Check(!Placeable(sword)&&StackCapacity(sword)==1,"swords are nonplaceable single items");
    }
    for(Block material:{Planks,Cobblestone,IronIngot}) {
        WorkbenchCrafting={};CursorStack={};
        for(int i=0;i<3;++i) WorkbenchCrafting[i]={material,1};
        WorkbenchCrafting[4]={Stick,1};WorkbenchCrafting[7]={Stick,1};
        Block pick=material==Planks?WoodenPickaxe:(material==Cobblestone?StonePickaxe:IronPickaxe);
        Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==pick&&CursorStack.count==1,
              "three materials above two sticks craft the matching pickaxe");
        Check(!Placeable(pick)&&StackCapacity(pick)==1,"pickaxes are nonplaceable single items");
    }
    WorkbenchCrafting={};CursorStack={};WorkbenchCrafting[0]={Stone,1};WorkbenchCrafting[3]={Stone,1};WorkbenchCrafting[6]={Stick,1};
    Check(!CraftResult(WorkbenchCrafting.data(),3).count,"stone sword requires cobblestone rather than smooth stone");
    for(auto& slot:WorkbenchCrafting) slot={Cobblestone,1};
    WorkbenchCrafting[4]={};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==Furnace,"eight cobblestones craft furnace");
    CursorStack={};Change(50,70,50,Furnace);
    auto* f=FurnaceAt(50,70,50);f->slots[0]={IronOre,2};
    for(int i=0;i<20;++i) UpdateFurnaces(.25f);
    Check(f->slots[0].count==2&&!f->slots[2].count,"ore cannot smelt without fuel");
    f->slots[1]={Coal,1};
    for(int i=0;i<20;++i) UpdateFurnaces(.25f);
    Check(f->progress==5&&f->fuel==75&&!f->slots[1].count&&!f->slots[2].count,"coal starts a ten-second smelting cycle");
    WorkbenchCrafting[0]={IronIngot,3};PersonalCrafting[0]={Wood,7};
    auto file=std::filesystem::temp_directory_path()/
        ("mineclone-crafting-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dat");
    PlayerState pose;pose.valid=true;pose.position={64,75,64};
    Check(SaveWorld(file.string(),pose),"save crafting and half-smelted iron");
    ClearChests();Check(LoadWorld(file.string()),"reload crafting state");f=FurnaceAt(50,70,50);
    Check(f&&f->progress==5&&f->slots[0].count==2&&PersonalCrafting[0].count==7&&WorkbenchCrafting[0].count==3,
          "save preserves input slots and smelting progress");
    for(int i=0;i<20;++i) UpdateFurnaces(.25f);
    Check(f->slots[2].block==IronIngot&&f->slots[2].count==1&&f->slots[0].count==1,
          "iron ore produces one ingot after ten seconds");
    Change(60,70,60,IronSword);Check(At(60,70,60)==Air,"world rejects inventory-only tools");
    Change(60,70,60,IronOre);Check(At(60,70,60)==IronOre,"iron ore is a placeable block");
    std::filesystem::remove(file);
    std::puts("PASS: classic shaped recipes, sword limits, ingredient consumption, iron smelting and crafting saves");
}
