#pragma once
#include <raylib.h>
#include "block.h"
#include "player.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace game {
constexpr int Width=256, Height=80, ChunkSize=16, ChunkCount=Width/ChunkSize;
constexpr int SeaLevel=23;
enum class Biome { Plains, Forest, Desert, Mountains };
extern unsigned char World[Width][Height][Width];
constexpr unsigned char MaxWaterLevel=7;
unsigned char FluidLevelAt(int x,int y,int z);
bool IsFluidSource(int x,int y,int z);
unsigned char LavaLevelAt(int x,int y,int z);
bool IsLavaSource(int x,int y,int z);
float FluidHeight(int x,int y,int z);
unsigned char WaterLevelAt(int x,int y,int z);
bool IsWaterSource(int x,int y,int z);
float WaterHeight(int x,int y,int z);
bool WaterContains(Vector3 point);
extern std::array<uint64_t,ChunkCount*ChunkCount> ChunkVersions;
extern uint32_t Seed;
extern int32_t RegionOriginX,RegionOriginZ;
bool StreamWorld(PlayerState& player,const std::string& savePath,bool& shifted);
void ClearRemoteWorld(bool inventory=true);
void SetRemoteBlock(int x,int y,int z,Block block);
void ApplyRemoteChunk(int cx,int cz,const std::vector<Block>& blocks);
bool Inside(int x,int y,int z);
Block At(int x,int y,int z);
uint32_t Hash(int x,int z,int salt=0);
void GenerateWorld(uint32_t seed);
uint32_t RandomSeed();

constexpr float DayDuration=1200.f;
extern float WorldTime;
int ClockMinute();
void SetClockMinute(int minute);
float Daylight();
Color SkyColor();
void UpdateWorld(float dt);
void UpdateWater(float dt);
void UpdateLava(float dt);
void Change(int x,int y,int z,Block block);
bool SaveWorld(const std::string& path,const PlayerState& player);
bool LoadWorld(const std::string& path,PlayerState* player=nullptr);
struct WorldSave {std::string name,path;};
std::vector<WorldSave> ListWorldSaves();
bool DeleteWorldSave(const std::string& path);
std::string CreateWorldSavePath();
struct Hit { bool found=false; int x=0,y=0,z=0,nx=0,ny=0,nz=0; };
Hit Trace(Vector3 origin,Vector3 direction,bool includeLiquids=false);
}
