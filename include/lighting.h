#pragma once
#include "world.h"
namespace game {
struct VertexLight {float sky=0,block=0,ambient=1;};
VertexLight SampleCornerLighting(int x,int y,int z,int normalAxis,int sideA,int sideB);
void InvalidateLighting();
void InvalidateLighting(int x,int y,int z);
// Zero completes the update; a positive budget preserves the committed light while work is pending.
void UpdateLighting(float budgetMilliseconds=0.f);
bool LightingPending();
bool LightingRequiresFullUpdate();
float LightingProgress();
unsigned char DirectBlockLightAt(int x,int y,int z);
unsigned char SkyLightAt(int x,int y,int z);
unsigned char BlockLightAt(int x,int y,int z);
}
