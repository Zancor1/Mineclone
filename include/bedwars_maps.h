#pragma once
#include "block.h"
#include <raylib.h>
#include <array>
#include <functional>
#include <vector>
namespace game::bw {
constexpr int SkywarsMap=21;
constexpr int MapCount=22;
const char* MapName(int map);
const char* MapDescription(int map);
std::array<Vector3,8> ArenaBases(int gap,int map=0);
int MapTeams(int map);
int MapLobbyY(int map);
Vector3 RuinsVendor(int team,int kind,int gap);
Vector3 AirshowSpawn(int team);
Vector3 AirshowVendor(int team,int kind);
Vector3 AirshowForge(int team);
Vector3 AirshowChest(int team);
std::vector<Vector3> AirshowResources(bool emerald);
std::array<Vector3,8> DiamondIslands(int gap,int map=0);
std::array<Vector3,2> MapBeds(int map,int team,const Vector3& base);
void BuildArenaTerrain(int map,int gap,const std::function<void(int,int,int,Block)>& write,bool lobby=true);
RenderTexture2D CreateMapPreview(int map,int gap);
}
