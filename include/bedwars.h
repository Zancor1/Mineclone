#pragma once
#include "projectile_physics.h"
#include <vector>
#include "world.h"
#include "progression.h"
#include "bedwars_maps.h"
#include "lan_transport.h"
#include <string>
namespace game::bw {
std::vector<ArrowState> ArrowStates();
bool Host(const std::string& name,unsigned short port=25566,int gap=64,int map=0,int capacity=1,const std::string& serverName="");
struct GeneratorInfo {Vector3 position;Block item;int stock,level;float interval,remaining;};
int ResourceLevel(Block item,float elapsed);
float ResourceInterval(Block item,float elapsed);
std::vector<GeneratorInfo> Generators();
struct TeamInfo {int id,players,capacity;const char* name;Color color;bool available;};
int TeamCapacity();
int PlayerTeam(int id);
int LocalTeam();
std::vector<TeamInfo> TeamChoices();
void SelectTeam(int team);
int IslandGap();
int SelectedMap();
void ApplyEffects(PlayerInput& input);
Vector3 BasePosition(int team);
bool Join(const std::string& address,const std::string& name);
void Stop();
void UpdateDiscovery();
void SearchRooms();
bool SearchingRooms();
const std::vector<LanRoom>& Rooms();
const std::string& DiscoveryStatus();
bool Active();
bool Ready();
bool Hosting();
bool Lobby();
bool CanControl();
void Chat(const std::string& text);
const std::vector<std::string>& Messages();
std::string Nickname();
progression::Profile Progress(int player=-1);
int ItemSkin(Block item,int player=-1);
void OpenCrate();
void NextSkin(int category=0);
void EquipSkin(int category,int skin);
struct PlayerEntry {std::string name;int team;bool eliminated;};
std::vector<PlayerEntry> PlayerList();
const std::string& Status();
void ConnectionTrace(const std::string& message);
void Update(Player& player,float dt,bool playing,int selected);
void StartRound();
void Restart();
bool CanMine(const Hit& hit);
void Dig(const Hit& hit,int stage,int selected);
void Place(int x,int y,int z,int selected,const Player* player=nullptr);
void Attack(int selected,const Player* player=nullptr);
void Use(int selected);
void HoldUse(int selected,bool held);
void Buy(int offer);
void SwapSlots(int from,int to,bool chest,bool exact=false);
bool NearShop(const Player& player);
bool AimedShop(const Player& player);
bool NearChest(const Player& player);
void DrawWorld(const Camera3D& camera,Texture2D atlas={});
void DrawHud();
void DrawShop(Texture2D atlas);
void ShopInput();
void DrawInventory(Texture2D atlas,bool chest);
void InventoryInput(bool chest);
void InventoryClick(int slot,bool right,bool chest=false,bool quick=false);
void Craft(int target=254);
}
