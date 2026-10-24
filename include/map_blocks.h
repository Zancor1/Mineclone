#pragma once
#include "block.h"
#include <raylib.h>
#include <array>
namespace game {
enum class MapShape : unsigned char {Cube,Slab,Stairs,Fence,Bars,Carpet,Torch,Pot,Plate,Hopper,Sign,Portal,Web,Fire,Barrier,Wall,Pane,Trapdoor,Door,Gate,Rail,Vine,Plant,Anvil,Skull,Cocoa,Hook,PortalFrame,Dispenser,Jukebox,RedstoneBlock,Log,Beacon,Button};
struct MapBlockState {Block block=Air,material=Stone;MapShape shape=MapShape::Cube;unsigned char data=0;};
struct BlockBoxes {std::array<BoundingBox,12> boxes{};int count=0;};
void ClearMapBlocks();
void SetMapBlockState(int x,int y,int z,MapBlockState state);
MapBlockState MapStateAt(int x,int y,int z);
BlockBoxes CollisionBoxesAt(int x,int y,int z);
BlockBoxes MapShapeBoxes(int x,int y,int z);
unsigned char BlockEmissionAt(int x,int y,int z);
bool LightPassableAt(int x,int y,int z);
bool HasMapShape(int x,int y,int z);
}
