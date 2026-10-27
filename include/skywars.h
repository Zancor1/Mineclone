#pragma once
#include "chest.h"
#include <raylib.h>
#include <array>
#include <functional>
#include <string_view>
namespace game::sky {
constexpr int ChestCount=28;
std::array<Vector3,8> Bases();
std::array<Vector3,ChestCount> Chests();
void Build(const std::function<void(int,int,int,Block)>& put,bool lobby);
unsigned LootSeed(std::string_view room,int round);
ChestInventory Loot(unsigned seed,int chest,int refill=0);
}
