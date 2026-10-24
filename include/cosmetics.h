#pragma once
#include <raylib.h>
#include "block.h"
namespace game::cosmetics {
int Category(Block item);
Color ColorFor(int skin);
const char* ColorName(int skin);
const char* CategoryName(int category);
void Reset();
void Toggle();
bool Opened();
void AwaitReward();
void CancelWaiting();
void Reward(int reward);
bool Input();
void Draw(Texture2D atlas);
}
