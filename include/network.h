#pragma once
#include "player.h"
#include "world.h"
#include <string>
#include <vector>
#include <array>
namespace game::net {
struct Slot {short id=-1,damage=0;unsigned char count=0;std::vector<unsigned char> wire;};
struct PlayerEntry {std::string uuid,name,displayName;int ping=0,gameMode=0;};
struct MapIcon {int type=0,rotation=0,x=0,y=0;};
struct MapData {int id=0,scale=0;uint64_t revision=0;std::array<unsigned char,128*128> pixels{};std::vector<MapIcon> icons;};
struct MapFrame {Vector3 position{};int mapId=-1,direction=0,rotation=0;};
std::vector<PlayerEntry> PlayerList();
const std::string& TabHeader();
const std::string& TabFooter();
std::string Nickname();
const MapData* FindMap(int id);
int HeldMapId();
std::vector<MapFrame> MapFrames();
Color MapColor(unsigned char index);
bool Active();
bool Joined();
void Connect(const std::string& address,const std::string& name);
void Disconnect();
std::string Status();
void Update(Player& player,float dt);
void SendMovement(const Player& player);
bool PlayerInventoryOpen();
struct Hologram {Vector3 position{};std::string text;};
std::vector<Hologram> Holograms();
void DrawHolograms(const Camera3D& camera);
float MovementSpeed();
float CollisionAt(int x,int y,int z);
void DrawPlayers();
void Attack(const Player& player);
void Interact(const Player& player);
void Dig(const Hit& hit,int status);
void SelectSlot(int index);
void MovementFlags(bool sprint,bool sneak);
int SelectedSlot();
void Chat(const std::string& text);
const std::vector<std::string>& Messages();
void ShowInventory();
bool WindowOpen();
const std::vector<Slot>& WindowSlots();
const Slot& Cursor();
const std::string& WindowTitle();
void ClickSlot(int slot,bool right,bool shift);
void SwapHotbar(int slot,int hotbar);
void CloseWindow();
void Respawn();
Block LocalItem(const Slot& slot);
}
