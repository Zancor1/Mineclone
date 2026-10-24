#include "survival.h"
#include "world.h"
#include "crafting.h"
#include "player.h"
#include "map_blocks.h"
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
namespace {
void Check(bool ok,const char* text) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int Count(game::Block b) {int n=0;for(auto s:game::Backpack) if(s.block==b)n+=s.count;for(auto s:game::Hotbar)if(s.block==b)n+=s.count;return n;}
}
int main() {
    using namespace game;
    Mode=GameMode::Survival;ClearChests();
    Check(Count(Grass)==0&&Health==20,"survival starts empty with full health");
    ItemStack empty;
    Change(60,70,60,Stone);
    Check(MineBlock(60,70,60,empty)&&Count(Cobblestone)==0,"stone broken by hand produces no cobblestone");
    Hotbar[0]={WoodenPickaxe,1};Change(60,70,60,Stone);
    Check(MineBlock(60,70,60,Hotbar[0])&&Count(Cobblestone)==1&&Hotbar[0].damage==1,"wood pick harvests stone and wears");
    Check(!CanHarvest(IronOre,WoodenPickaxe)&&CanHarvest(IronOre,StonePickaxe),"iron requires stone pick or better");
    Check(MiningTime(Stone,IronPickaxe)<MiningTime(Stone,StonePickaxe)&&MiningTime(Stone,StonePickaxe)<MiningTime(Stone,WoodenPickaxe),"better picks mine faster");
    const Block picks[]={Air,WoodenPickaxe,StonePickaxe,IronPickaxe,DiamondPickaxe,GoldenPickaxe};
    const float stoneTimes[]={7.5f,1.15f,.6f,.4f,.3f,.2f};
    for(int i=0;i<6;++i)Check(std::abs(MiningTime(Stone,picks[i])-stoneTimes[i])<.001f,"stone timings match Minecraft 1.8 tick-rounded table");
    Check(std::abs(MiningTime(Cobblestone,IronPickaxe)-.5f)<.001f,"cobblestone retains hardness two");
    Check(std::abs(MiningTime(EndStone,IronPickaxe)-.75f)<.001f,"end stone uses its original hardness");
    Check(std::abs(MiningTime(Obsidian,DiamondPickaxe)-9.4f)<.001f&&std::abs(MiningTime(Obsidian,IronPickaxe)-41.7f)<.001f,"obsidian distinguishes speed from harvest eligibility");
    Check(!CanHarvest(IronOre,GoldenPickaxe)&&CanHarvest(IronOre,StonePickaxe),"gold has wood harvest level despite fast mining");
    Check(std::abs(MiningTime(Planks,WoodenAxe)-1.5f)<.001f&&std::abs(MiningTime(Planks,IronAxe)-.5f)<.001f,"axes use the wood material efficiency");
    Check(std::abs(MiningTime(Sand,IronShovel)-.15f)<.001f&&std::abs(MiningTime(Sand,IronPickaxe)-.75f)<.001f,"shovels accelerate sand while pickaxes do not");
    Check(std::abs(MiningTime(WhiteWool,IronSword)-1.2f)<.001f&&std::abs(MiningTime(Glass,Air)-.45f)<.001f,"wool and glass keep original hardness without wrong-tool acceleration");
    Check(std::abs(MiningTime(Stone,IronPickaxe,false)-1.9f)<.001f&&std::abs(MiningTime(Stone,IronPickaxe,true,true)-1.9f)<.001f,"airborne and underwater mining apply fivefold slowdown before tick rounding");
    for(Block tool:{Air,WoodenAxe,DiamondPickaxe})Check(std::abs(MiningTime(Bed,tool)-.15f)<.001f&&std::abs(MiningTime(BedHead,tool)-.15f)<.001f,"both bed halves break in 0.15 seconds with any tool");
    Change(70,70,70,MapStairs);SetMapBlockState(70,70,70,{MapStairs,Planks,MapShape::Stairs,0});
    Check(std::abs(MiningTimeAt(70,70,70,IronAxe)-.5f)<.001f,"imported wood stairs use wood mining efficiency");
    Change(70,70,70,MapSlab);SetMapBlockState(70,70,70,{MapSlab,StoneBricks,MapShape::Slab,0});
    Check(std::abs(MiningTimeAt(70,70,70,IronPickaxe)-.5f)<.001f,"imported slabs retain slab hardness");
    Change(70,70,70,MapDecoration);SetMapBlockState(70,70,70,{MapDecoration,Stone,MapShape::Barrier,0});
    Check(std::isinf(MiningTimeAt(70,70,70,DiamondPickaxe)),"barriers remain unbreakable");
    Change(70,70,70,Air);
    Hotbar[0]={StonePickaxe,1};Change(60,70,60,IronOre);
    Check(MineBlock(60,70,60,Hotbar[0])&&Count(IronOre)==1,"stone pick collects iron ore");
    ItemStack block{Planks,2};
    Check(PlaceSurvivalBlock(61,70,60,block)&&block.count==1,"building consumes a block");
    Check(PlaceSurvivalBlock(62,70,60,block)&&!block.count&&block.block==Air,"last block clears slot");
    Hotbar[0]={WoodenPickaxe,1,58};Change(60,70,60,Stone);
    Check(MineBlock(60,70,60,Hotbar[0])&&!Hotbar[0].count,"exhausted tool breaks");
    ItemStack damaged{IronPickaxe,1,23};CursorStack={};
    ClickStack(damaged,false);Check(CursorStack.damage==23&&!damaged.count,"pickup preserves durability");
    ClickStack(damaged,true);Check(damaged.damage==23&&!CursorStack.count,"right deposit preserves durability");
    ResetVitals();
    for(int x=69;x<=71;++x) for(int z=69;z<=71;++z) Change(x,60,z,Stone);
    Player player;PlayerState falling;falling.valid=true;falling.position={70.5f,69,70.5f};falling.flying=true;
    Check(player.RestoreState(falling)&&!player.CaptureState().flying,"survival forbids restoring flight");
    PlayerInput input;
    for(int i=0;i<120;++i)player.Update(input,.02f);
    Check(Health<20&&player.CaptureState().position.y<62,"landing after a long fall deals damage");
    input.jump=true;player.Update(input,.02f);input.jump=false;player.Update(input,.02f);input.jump=true;player.Update(input,.02f);
    Check(!player.CaptureState().flying,"double space cannot enable survival flight");
    ResetVitals();Hurt(4);Check(Health==16,"survival takes damage");
    Change(65,71,65,Water);Change(65,72,65,Water);
    for(int i=0;i<92;++i)UpdateHazards({65.5f,70.9f,65.5f},.25f);
    Check(Health<16&&Breath==0,"submersion depletes air then damages health");
    ResetVitals();Change(66,70,66,Lava);UpdateHazards({66.5f,70,66.5f},.1f);
    Check(Health==16,"lava damages survival player");
    ResetVitals();Health=13;Breath=8;Hotbar[0]={IronPickaxe,1,23};
    auto path=std::filesystem::temp_directory_path()/("mineclone-survival-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dat");
    PlayerState pose;pose.valid=true;pose.position={64,75,64};
    Check(SaveWorld(path.string(),pose),"save survival world");
    Mode=GameMode::Creative;ClearChests();Check(LoadWorld(path.string()),"load survival world");
    Check(Mode==GameMode::Survival&&Health==13&&Breath==8&&Hotbar[0].damage==23,"save preserves mode, vitals and tool wear");
    auto corrupt=SerializeSurvival();corrupt[0]=2;Check(!DeserializeSurvival(corrupt,false),"reject invalid mode");
    Mode=GameMode::Creative;ResetVitals();Hurt(20);Check(Health==20,"creative ignores damage");
    block={Planks,1};Check(PlaceSurvivalBlock(63,70,60,block)&&block.count==1,"creative preserves placed items");
    std::vector<unsigned char> legacy(30,0);Check(DeserializeCrafting(legacy,true,false)&&Mode==GameMode::Creative,"old crafting format remains readable");
    std::filesystem::remove(path);
    std::puts("PASS: survival harvesting, mining tiers, consumption, durability, hazards and persistence");
}
