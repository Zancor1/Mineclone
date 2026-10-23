#pragma once
#include "world.h"
namespace game {
struct VertexLight {float sky=0,block=0,ambient=1;};
VertexLight SampleCornerLighting(int x,int y,int z,int normalAxis,int sideA,int sideB);
void InvalidateLighting();
void UpdateLighting();
unsigned char DirectBlockLightAt(int x,int y,int z);
unsigned char SkyLightAt(int x,int y,int z);
unsigned char BlockLightAt(int x,int y,int z);
}
