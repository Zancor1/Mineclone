#include "catalog18.h"
#include "crafting.h"
#include "survival.h"
#include "world.h"
#include <cstdio>
#include <cstdlib>
namespace {
void Check(bool condition,const char* text) {if(!condition) {std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
}
int main() {
    using namespace game;
    Check(BlockCount==256&&Catalog18Size==631,"stable expanded registry and version reference");
    int supported=0,missing=0;
    for(auto e:Catalog18) {Check(e.available?ValidBlock(e.block):e.block==Air,"reference mapping is valid");e.available?++supported:++missing;}
    Check(supported>150&&missing>100,"unsupported mechanics stay explicit");
    for(int id=65;id<BlockCount;++id) {
        Block b=static_cast<Block>(id);
        if(Placeable(b)) {Change(60,70,60,b);Check(At(60,70,60)==b&&(Solid(b)||b==Ladder||b==MapDecoration),"extended construction block can be placed");}
        else Check(!Solid(b),"inventory items cannot create invisible collision cubes");
    }
    Check(Transparent(RedStainedGlass)&&Solid(RedStainedGlass),"colored glass is solid and translucent");
    Check(Emission(Glowstone)==24&&Emission(SeaLantern)==24,"new light blocks emit light");
    Check(PickaxeTier(DiamondPickaxe)==4&&PickaxeTier(GoldenPickaxe)==1,"diamond and gold harvest tiers");
    Check(CanHarvest(Obsidian,DiamondPickaxe)&&!CanHarvest(Obsidian,IronPickaxe),"obsidian needs diamond");
    Check(MiningTime(Stone,GoldenPickaxe)<MiningTime(Stone,DiamondPickaxe),"gold mines quickly despite low harvest tier");
    Check(ToolDurability(DiamondPickaxe)==1561&&ToolDurability(GoldenPickaxe)==32,"classic tool durability");
    WorkbenchCrafting={};CursorStack={};
    for(int i=0;i<3;++i) {WorkbenchCrafting[i]={Diamond,1};}
    WorkbenchCrafting[4]={Stick,1};WorkbenchCrafting[7]={Stick,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==DiamondPickaxe,"diamond pick recipe");
    WorkbenchCrafting={};CursorStack={};for(auto& stack:WorkbenchCrafting)stack={GoldIngot,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==BlockOfGold,"nine gold ingots compress");
    WorkbenchCrafting={};CursorStack={};WorkbenchCrafting[0]={BlockOfGold,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.count==9&&CursorStack.block==GoldIngot,"gold compression reverses");
    Change(60,70,60,Furnace);auto* f=FurnaceAt(60,70,60);f->slots[0]={GoldOre,1};f->slots[1]={Coal,1};
    for(int i=0;i<40;++i)UpdateFurnaces(.25f);
    Check(f->slots[2].block==GoldIngot&&f->slots[2].count==1,"gold ore smelts into gold ingot");
    PersonalCrafting={};CursorStack={};PersonalCrafting[0]={WhiteWool,1};PersonalCrafting[3]={LapisLazuli,1};
    Check(TakeCraft(PersonalCrafting.data(),2)&&CursorStack.block==BlueWool,"shapeless wool dye recipe");
    WorkbenchCrafting={};CursorStack={};
    for(auto& slot:WorkbenchCrafting) {slot={Glass,1};}
    WorkbenchCrafting[4]={RoseRed,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==RedStainedGlass&&CursorStack.count==8,"colored glass recipe");
    PersonalCrafting={};CursorStack={};PersonalCrafting[3]={SpruceWood,1};
    Check(TakeCraft(PersonalCrafting.data(),2)&&CursorStack.block==SpruceWoodPlanks&&CursorStack.count==4,"log keeps its wood species when crafted");
    Check(FuelDuration(BlockOfCoal)==800&&FuelDuration(BirchWood)==15&&FurnaceAccepts(Clay),"additional fuel and kiln inputs");
    std::puts("PASS: 1.8 catalogue, construction blocks, transparency, light, tools, recipes and gold smelting");
}
