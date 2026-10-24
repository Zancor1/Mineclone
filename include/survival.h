#pragma once
#include "chest.h"
#include <raylib.h>
#include <vector>
namespace game {
enum class GameMode {Creative,Survival};
extern GameMode Mode,NewWorldMode;
extern float Health,Breath,FallDistance;
extern float Absorption,AbsorptionTime,RegenerationTime;
extern std::array<ItemStack,4> EquippedArmor;
int DefensePoints();
bool EquipArmor(ItemStack& stack);
void EatGoldenApple();
void ResetVitals();
void Hurt(float amount,bool armor=false);
void UpdateHazards(Vector3 feet,float dt);
int PickaxeTier(Block tool);
bool CanHarvest(Block block,Block tool);
float MiningTime(Block block,Block tool);
bool MineBlock(int x,int y,int z,ItemStack& tool);
bool PlaceSurvivalBlock(int x,int y,int z,ItemStack& stack);
std::vector<unsigned char> SerializeSurvival();
bool DeserializeSurvival(const std::vector<unsigned char>& data,bool apply);
}
