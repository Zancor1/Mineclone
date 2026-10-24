#pragma once
#include "chest.h"
namespace game {
extern std::array<ItemStack,4> PersonalCrafting;
extern std::array<ItemStack,9> WorkbenchCrafting;
ItemStack CraftResult(const ItemStack* grid,int width);
bool TakeCraft(ItemStack* grid,int width);
void ClearCrafting();
struct FurnaceState {std::array<ItemStack,3> slots{};float fuel=0,progress=0;};
FurnaceState* FurnaceAt(int x,int y,int z);
bool RemoveFurnace(int x,int y,int z);
bool FurnaceAccepts(Block input);
float FuelDuration(Block fuel);
void UpdateFurnaces(float dt);
std::vector<unsigned char> SerializeCrafting();
bool DeserializeCrafting(const std::vector<unsigned char>& data,bool apply,bool wide=true);
}
