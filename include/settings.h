#pragma once
#include <string>
#include <array>
#include <raylib.h>
namespace game {
enum class BindingAction {Forward,Backward,Left,Right,Jump,Sneak,Sprint,Inventory,Pause,Attack,Use,Chat,Command,PlayerList,TargetLiquid,ToggleLamp,SkipTime,StartRound,RestartRound,Hotbar1,Hotbar2,Hotbar3,Hotbar4,Hotbar5,Hotbar6,Hotbar7,Hotbar8,Hotbar9,Count};
constexpr int BindingCount=int(BindingAction::Count);
inline constexpr std::array<int,BindingCount> DefaultBindings={KEY_W,KEY_S,KEY_A,KEY_D,KEY_SPACE,KEY_LEFT_SHIFT,KEY_LEFT_CONTROL,KEY_E,KEY_ESCAPE,-1,-2,KEY_T,KEY_SLASH,KEY_TAB,KEY_LEFT_ALT,KEY_F,KEY_F6,KEY_P,KEY_F8,KEY_ONE,KEY_TWO,KEY_THREE,KEY_FOUR,KEY_FIVE,KEY_SIX,KEY_SEVEN,KEY_EIGHT,KEY_NINE};
const char* BindingLabel(BindingAction action);
std::string BindingKeyName(int key);
bool SetBinding(BindingAction action,int key);
bool ActionDown(BindingAction action);
bool ActionPressed(BindingAction action);
bool ActionReleased(BindingAction action);
struct GameSettings {
    std::string playerName="Player",serverAddress="jogar.mush.com.br";
    bool freezeTime=false;
    bool toggleSprint=true;
    bool fogEnabled=true;
    int renderChunks=3;
    float fogDistance=40.f;
    float mouseSensitivity=1.f;
    float fov=70.f;
    std::array<int,BindingCount> bindings=DefaultBindings;
    int newWorldMinute=720;
};
inline GameSettings Settings;
constexpr int MinRenderChunks=1,MaxRenderChunks=6;
float RenderDistance();
float FogDensity(bool underwater);
bool LoadSettings(const std::string& path="settings.cfg");
bool SaveSettings(const std::string& path="settings.cfg");
}
