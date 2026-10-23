#pragma once
#include <raylib.h>
#include "block.h"
#include "player.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace game {
constexpr int Width=128, Height=80, ChunkSize=16, ChunkCount=Width/ChunkSize;
constexpr int SeaLevel=23;
enum class Biome { Plains, Forest, Desert, Mountains };
extern unsigned char World[Width][Height][Width];
constexpr unsigned char MaxWaterLevel=7;
unsigned char WaterLevelAt(int x,int y,int z);
bool IsWaterSource(int x,int y,int z);
float WaterHeight(int x,int y,int z);
bool WaterContains(Vector3 point);
extern std::array<uint64_t,ChunkCount*ChunkCount> ChunkVersions;
extern uint32_t Seed;
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
void Change(int x,int y,int z,Block block);
bool SaveWorld(const std::string& path,const PlayerState& player);
bool LoadWorld(const std::string& path,PlayerState* player=nullptr);
struct WorldSave {std::string name,path;};
std::vector<WorldSave> ListWorldSaves();
bool DeleteWorldSave(const std::string& path);
std::string CreateWorldSavePath();
struct Hit { bool found=false; int x=0,y=0,z=0,nx=0,ny=0,nz=0; };
Hit Trace(Vector3 origin,Vector3 direction,bool includeWater=false);
}
