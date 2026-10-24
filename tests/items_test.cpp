#include "items.h"
#include "world.h"
#include "survival.h"
#include "crafting.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
namespace {
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
}
int main(){
    using namespace game;
    Mode=GameMode::Survival;ClearChests();ClearItems();Player player;PlayerState pose;pose.valid=true;pose.position={50.5f,72,50.5f};player.RestoreState(pose);
    Check(MinigameItemCount>50&&BlockCount<=256,"minigame selection and stable byte IDs");
    ItemStack worn{IronHelmet,1,3};CursorStack={IronHelmet,1,9};ClickStack(worn,false);
    Check(worn.damage==9&&CursorStack.damage==3,"same armor type swaps without losing wear");CursorStack={};
    ItemStack armor{DiamondChestplate,1};Check(EquipArmor(armor)&&!armor.count&&EquippedArmor[1].block==DiamondChestplate,"armor moves from hand into equipment");
    ItemStack helmet{DiamondHelmet,1},legs{DiamondLeggings,1},boots{DiamondBoots,1};EquipArmor(helmet);EquipArmor(legs);EquipArmor(boots);
    Check(DefensePoints()==20,"diamond armor gives twenty defense points");Hurt(10,true);
    Check(Health>17.9f&&EquippedArmor[1].damage==2,"armor reduces damage and loses durability");
    EquippedArmor={};ResetVitals();Change(50,70,50,Stone);Change(50,70,51,Stone);
    ItemStack bucket{WaterBucket,1};UseInput click{true,true,false,.05f};
    Check(UseHeldItem(player,bucket,{0,-1,0},click)&&bucket.block==Bucket&&IsWaterSource(50,71,50),"water bucket places source and returns empty bucket");
    Check(UseHeldItem(player,bucket,{0,-1,0},click)&&bucket.block==WaterBucket&&At(50,71,50)==Air,"empty bucket collects source");
    Change(50,71,50,Water);for(auto& s:Backpack)s={Stone,64};for(auto& s:Hotbar)s={Stone,64};ItemStack fullBucket{Bucket,2};
    UseHeldItem(player,fullBucket,{0,-1,0},click);
    Check(fullBucket.block==Bucket&&fullBucket.count==2&&IsWaterSource(50,71,50),"full inventory cannot delete source or duplicate filled buckets");
    Backpack={};Hotbar={};Change(50,71,50,Air);
    ItemStack pearl{EnderPearl,2};UseHeldItem(player,pearl,{0,-1,0},click);Check(pearl.count==1&&ActiveProjectiles()==1,"pearl is thrown and consumed");
    for(int i=0;i<20;++i)UpdateItems(player,.05f);
    Check(ActiveProjectiles()==0&&player.CaptureState().position.y<72&&Health==15,"pearl teleports safely on impact and deals survival damage");
    ClearItems();player.RestoreState(pose);Hotbar[0]={Arrow,4};ItemStack bow{Bow,1};
    for(int i=0;i<20;++i)UseHeldItem(player,bow,{0,0,-1},{i==0,true,false,.05f});
    UseHeldItem(player,bow,{0,0,-1},{false,false,true,.05f});
    Check(ActiveProjectiles()==1&&Hotbar[0].count==3&&bow.damage==1,"charged bow shoots arrow and consumes ammo and durability");
    ClearItems();Hotbar={};bow={Bow,1};for(int i=0;i<20;++i)UseHeldItem(player,bow,{0,0,-1},{false,true,false,.05f});UseHeldItem(player,bow,{0,0,-1},{false,false,true,.05f});
    Check(ActiveProjectiles()==0&&bow.damage==0,"survival bow cannot shoot without arrows");
    ClearItems();Change(50,73,45,Stone);ItemStack fireball{Fireball,1};UseHeldItem(player,fireball,{0,0,-1},click);
    for(int i=0;i<20;++i)UpdateItems(player,.05f);
    Check(!fireball.count&&At(50,73,45)==Air&&ActiveProjectiles()==0,"fireball explodes on impact");
    ResetVitals();Health=10;ItemStack apple{GoldenApple,2};
    for(int i=0;i<33;++i)UseHeldItem(player,apple,{0,0,-1},{i==0,true,false,.05f});
    Check(apple.count==1&&Absorption==4&&RegenerationTime==5,"golden apple grants timed effects after eating");
    UpdateHazards(pose.position,.25f);Check(Health>10,"golden apple regenerates health");
    ClearItems();Change(61,71,60,Stone);Change(62,71,60,Obsidian);PrimeTnt({60.5f,72,60.5f},.15f);
    for(int i=0;i<4;++i)UpdateItems(player,.05f);
    Check(ActiveProjectiles()==0&&At(61,71,60)==Air&&At(62,71,60)==Obsidian,"TNT explodes after fuse without destroying obsidian");
    ClearItems();ItemStack bed{Bed,1};UseHeldItem(player,bed,{0,-1,0},click);
    Check(!bed.count&&At(50,71,50)==Bed&&At(50,71,51)==BedHead,"bed occupies two supported blocks");
    player.Respawn();Check(player.CaptureState().position.y>71.56f&&player.CaptureState().position.y<71.57f,"active bed provides regional respawn");
    Change(50,71,51,Air);Check(At(50,71,50)==Air,"breaking either bed half removes pair");
    ItemStack equip{IronHelmet,1,3};EquipArmor(equip);
    auto file=std::filesystem::temp_directory_path()/("mineclone-items-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dat");
    Check(SaveWorld(file.string(),player.CaptureState()),"save equipment and active food effects");EquippedArmor={};ResetVitals();
    Check(LoadWorld(file.string())&&EquippedArmor[0].block==IronHelmet&&EquippedArmor[0].damage==3&&Absorption==4,"equipment and food effects survive reload");
    std::ifstream in(file,std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),{});in.close();bytes.resize(bytes.size()-28);bytes[6]=11;
    uint32_t checksum=2166136261u;for(size_t i=24;i<bytes.size();++i)checksum=(checksum^bytes[i])*16777619u;
    for(int i=0;i<4;++i)bytes[20+i]=static_cast<unsigned char>(checksum>>(8*i));
    {std::ofstream out(file,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());}
    Check(LoadWorld(file.string())&&!EquippedArmor[0].count&&Absorption==0,"version eleven saves migrate with empty equipment");std::filesystem::remove(file);
    WorkbenchCrafting={};CursorStack={};WorkbenchCrafting[0]={IronIngot,1};WorkbenchCrafting[2]={IronIngot,1};WorkbenchCrafting[4]={IronIngot,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==Bucket,"bucket recipe");
    WorkbenchCrafting={};CursorStack={};for(auto& s:WorkbenchCrafting)s={GoldIngot,1};WorkbenchCrafting[4]={Apple,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==GoldenApple,"golden apple recipe");
    WorkbenchCrafting={};CursorStack={};WorkbenchCrafting[1]={Stick,1};WorkbenchCrafting[2]={String,1};WorkbenchCrafting[3]={Stick,1};WorkbenchCrafting[5]={String,1};WorkbenchCrafting[7]={Stick,1};WorkbenchCrafting[8]={String,1};
    Check(TakeCraft(WorkbenchCrafting.data(),3)&&CursorStack.block==Bow,"bow recipe");
    std::puts("PASS: minigame items, equipment, pearls, bow, buckets, food, TNT, beds, recipes and save migration");
}
