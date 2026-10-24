#pragma once
#include <cstdint>
#include <array>
#include <filesystem>
#include <string>

namespace game::progression {
constexpr int SkinCount=12;
std::filesystem::path DefaultDirectory();
inline std::filesystem::path Directory=DefaultDirectory();
std::filesystem::path SkinDirectory(const std::filesystem::path& directory=Directory);
struct Profile {
    int xp=0,crates=0,pending=0;
    std::array<int,3> skins{};
    std::array<uint32_t,3> unlocked{{1,1,1}};
    bool operator==(const Profile& p) const {return xp==p.xp&&crates==p.crates&&pending==p.pending&&skins==p.skins&&unlocked==p.unlocked;}
    bool operator!=(const Profile& p) const {return !(*this==p);}
    int Level() const {return 1+xp/50;}
    int Remaining() const {return 50-xp%50;}
    void Award(int amount);
    int Claim();
    int Open(uint32_t random);
    bool Equip(int category,int skin);
    void NextSkin(int category=0);
};
int RewardForRoll(uint32_t random);
int RewardCategory(int reward);
int RewardColor(int reward);
std::string Key(const std::string& nickname);
bool Load(const std::string& nickname,Profile& profile,std::string& error,const std::filesystem::path& directory=Directory);
bool Save(const std::string& nickname,const Profile& profile,std::string& error,const std::filesystem::path& directory=Directory);
bool Valid(const Profile& profile);
Profile Reconcile(const Profile& local,const Profile& cached);
}
