#pragma once
#include "player.h"
#include "chest.h"
namespace game {
struct UseInput {bool pressed=false,down=false,released=false;float dt=0;};
bool UseHeldItem(Player& player,ItemStack& stack,Vector3 direction,const UseInput& input);
void UpdateItems(Player& player,float dt);
void DrawItems();
void TranslateItems(Vector3 displacement);
void ClearItems();
void CancelItemUse();
void PrimeTnt(Vector3 position,float fuse=4.f);
void Explode(Player& player,Vector3 position,float radius=3.5f);
int ActiveProjectiles();
inline constexpr Block MinigameItems[]={EnderPearl,Fireball,Tnt,GoldenApple,Bow,Arrow,Snowball,Egg,
    Bucket,WaterBucket,LavaBucket,FlintAndSteel,Bed,Ladder,WhiteWool,RedWool,BlueWool,GreenWool,
    YellowWool,Planks,EndStone,Obsidian,Glass,Chest,CraftingTable,
    WoodenSword,StoneSword,IronSword,DiamondSword,WoodenPickaxe,StonePickaxe,IronPickaxe,DiamondPickaxe,
    WoodenAxe,StoneAxe,IronAxe,DiamondAxe,
    LeatherHelmet,LeatherChestplate,LeatherLeggings,LeatherBoots,ChainmailHelmet,ChainmailChestplate,ChainmailLeggings,ChainmailBoots,
    IronHelmet,IronChestplate,IronLeggings,IronBoots,GoldenHelmet,GoldenChestplate,GoldenLeggings,GoldenBoots,
    DiamondHelmet,DiamondChestplate,DiamondLeggings,DiamondBoots,IronIngot,GoldIngot,Diamond,Emerald};
inline constexpr int MinigameItemCount=sizeof(MinigameItems)/sizeof(*MinigameItems);
}
