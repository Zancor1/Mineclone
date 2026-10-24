#pragma once
#include "block.h"
#include <array>
#include <cstddef>
#include <vector>
namespace game {
struct ItemStack {Block block=Air;unsigned char count=0;unsigned short damage=0;};
constexpr int StackLimit=64,ChestSlotCount=27;
using ChestInventory=std::array<ItemStack,ChestSlotCount>;
extern ChestInventory Backpack;
extern std::array<ItemStack,9> Hotbar;
int TransferStack(ItemStack& source,ItemStack* destination,size_t count);
extern ItemStack CursorStack;
ChestInventory* ChestAt(int x,int y,int z);
bool RemoveChest(int x,int y,int z);
void ClearChests();
void ClickStack(ItemStack& slot,bool right,bool shift=false);
std::vector<unsigned char> SerializeChests();
bool DeserializeChests(const std::vector<unsigned char>& data,bool apply,bool legacy=false,bool crafting=true,bool wide=true);
}
