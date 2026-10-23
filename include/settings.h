#pragma once
#include <string>
namespace game {
struct GameSettings {
    bool freezeTime=false;
    float mouseSensitivity=1.f;
    int newWorldMinute=720;
};
inline GameSettings Settings;
bool LoadSettings(const std::string& path="settings.cfg");
bool SaveSettings(const std::string& path="settings.cfg");
}
