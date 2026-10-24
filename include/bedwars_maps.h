#pragma once
#include "block.h"
#include <raylib.h>
#include <array>
#include <functional>
namespace game::bw {
constexpr int MapCount=4;
const char* MapName(int map);
const char* MapDescription(int map);
std::array<Vector3,4> ArenaBases(int gap,int map=0);
int MapTeams(int map);
std::array<Vector3,2> MapBeds(int map,int team,const Vector3& base);
void BuildArenaTerrain(int map,int gap,const std::function<void(int,int,int,Block)>& write,bool lobby=true);
RenderTexture2D CreateMapPreview(int map,int gap);
}
