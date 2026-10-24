#pragma once
#include "projectile_physics.h"
#include "item_use.h"
#include "dropped_items.h"
#include <vector>
#include "player.h"
#include "chest.h"
namespace game {
std::vector<ArrowState> ArrowStates();
struct UseInput {bool pressed=false,down=false,released=false;float dt=0;};
bool UseHeldItem(Player& player,ItemStack& stack,Vector3 direction,const UseInput& input);
void UpdateItems(Player& player,float dt);
void DrawItems(Texture2D atlas={},int tntTile=0);
void DrawItemLabels(const Camera3D& camera);
std::vector<PrimedTntState> TntStates();
void TranslateItems(Vector3 displacement);
void ClearItems();
void CancelItemUse();
float BowDrawTime();
float EatingTime();
bool Blocking();
void ApplyItemEffects(PlayerInput& input);
void PrimeTnt(Vector3 position,float fuse=TntFuse);
void Explode(Player& player,Vector3 position,float radius=3.5f,bool damage=true);
int ActiveProjectiles();
bool DropItem(const Player& player,ItemStack& stack,bool all);
const std::vector<DroppedItem>& DroppedItems();
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
