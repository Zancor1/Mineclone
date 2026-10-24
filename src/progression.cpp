#include "progression.h"
#include "file_replace.h"
#include <algorithm>
#include <fstream>
#include <limits>
#include <cstdlib>
#include <vector>

namespace game::progression {
std::filesystem::path DefaultDirectory(){
#ifdef _WIN32
    DWORD length=GetEnvironmentVariableW(L"APPDATA",nullptr,0);if(length){std::vector<wchar_t> value(length);if(GetEnvironmentVariableW(L"APPDATA",value.data(),length))return std::filesystem::path(value.data())/L"MineClone"/L"profiles";}
#else
    if(const char* data=std::getenv("XDG_DATA_HOME"))if(*data)return std::filesystem::path(data)/"MineClone"/"profiles";
    if(const char* home=std::getenv("HOME"))if(*home)return std::filesystem::path(home)/".local"/"share"/"MineClone"/"profiles";
#endif
    return "profiles";
}
std::filesystem::path SkinDirectory(const std::filesystem::path& directory){return directory.filename()=="profiles"?directory.parent_path()/"skins":directory/"skins";}
std::filesystem::path ExecutableDirectory(){
#ifdef _WIN32
    std::vector<wchar_t> path(32768);DWORD size=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));if(size&&size<path.size())return std::filesystem::path(std::wstring(path.data(),size)).parent_path();
#else
    std::error_code ec;auto path=std::filesystem::read_symlink("/proc/self/exe",ec);if(!ec)return path.parent_path();
#endif
    return {};
}
std::string Key(const std::string& nickname){
    if(nickname.size()<3||nickname.size()>16)return {};
    std::string key;
    for(unsigned char c:nickname){
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return {};
        key+=char(c>='A'&&c<='Z'?c+('a'-'A'):c);
    }
    return key;
}
bool Valid(const Profile& p){
    if(p.xp<0||p.crates<0||p.pending<0||int64_t(p.crates)+p.pending>p.xp/50)return false;
    for(int i=0;i<3;++i)if(p.skins[i]<0||p.skins[i]>SkinCount||!(p.unlocked[i]&1)||p.unlocked[i]>=(1u<<(SkinCount+1))||!(p.unlocked[i]&(1u<<p.skins[i])))return false;
    return true;
}
Profile Reconcile(const Profile& local,const Profile& cached){
    Profile result=cached.xp>local.xp?cached:local;
    for(int i=0;i<3;++i){result.unlocked[i]=local.unlocked[i]|cached.unlocked[i];result.skins[i]=cached.xp>local.xp?cached.skins[i]:local.skins[i];}
    return result;
}
void Profile::Award(int amount){
    if(amount<=0)return;
    int before=Level();xp+=std::min(amount,std::numeric_limits<int>::max()-xp);pending+=Level()-before;
}
int Profile::Claim(){int received=pending;crates+=pending;pending=0;return received;}
int RewardCategory(int reward){return (reward-1)/SkinCount;}
int RewardColor(int reward){return 1+(reward-1)%SkinCount;}
int RewardForRoll(uint32_t random){
    int category=int(random%3),roll=int((random/3)%100),color=1;
    constexpr int weights[SkinCount]={12,12,12,12,12,12,7,7,7,3,3,1};
    for(int weight:weights){if(roll<weight)break;roll-=weight;++color;}
    return 1+category*SkinCount+(color-1);
}
int Profile::Open(uint32_t random){
    if(!crates)return -1;
    --crates;int reward=RewardForRoll(random);unlocked[RewardCategory(reward)]|=1u<<RewardColor(reward);return reward;
}
bool Profile::Equip(int category,int color){
    if(category<0||category>=3||color<0||color>SkinCount||!(unlocked[category]&(1u<<color)))return false;
    skins[category]=color;return true;
}
void Profile::NextSkin(int category){if(category<0||category>=3)return;for(int i=1;i<=SkinCount+1;++i){int next=(skins[category]+i)%(SkinCount+1);if(Equip(category,next))return;}}
bool Load(const std::string& nickname,Profile& profile,std::string& error,const std::filesystem::path& directory){
    error.clear();auto key=Key(nickname);if(key.empty()){error="Nick invalido";return false;}
    auto path=directory/(key+".profile");std::error_code ec;
    bool exists=std::filesystem::exists(path,ec);if(ec){error=ec.message();return false;}
    if(!exists){
        if(directory==DefaultDirectory())for(auto legacy:{std::filesystem::path("profiles"),ExecutableDirectory()/"profiles"}){
            if(legacy==directory)continue;
            if(std::filesystem::exists(legacy/(key+".profile"),ec)){if(!Load(nickname,profile,error,legacy))return false;return Save(nickname,profile,error,directory);}
            if(ec){error=ec.message();return false;}
        }
        profile={};return true;
    }
    std::ifstream in(path);std::string magic,extra;int version=0;Profile loaded;
    bool ok=bool(in>>magic>>version>>loaded.xp>>loaded.crates)&&magic=="MINECLONE_PROFILE";
    if(ok&&version==1){uint32_t mask=0;int color=-1;ok=bool(in>>mask>>color);loaded.unlocked.fill(mask);loaded.skins.fill(color);}
    else if(ok&&version==2){ok=bool(in>>loaded.pending);for(int i=0;i<3&&ok;++i)ok=bool(in>>loaded.unlocked[i]>>loaded.skins[i]);}
    else ok=false;
    if(!ok||!Valid(loaded)||(in>>extra)){error="Perfil invalido: "+key;return false;}
    profile=loaded;return true;
}
bool Save(const std::string& nickname,const Profile& profile,std::string& error,const std::filesystem::path& directory){
    error.clear();auto key=Key(nickname);if(key.empty()||!Valid(profile)){error="Perfil invalido";return false;}
    std::error_code ec;std::filesystem::create_directories(directory,ec);if(ec){error=ec.message();return false;}
    auto path=directory/(key+".profile"),temporary=directory/(key+".profile.tmp");
    {std::ofstream out(temporary,std::ios::trunc);out<<"MINECLONE_PROFILE 2\n"<<profile.xp<<' '<<profile.crates<<' '<<profile.pending;for(int i=0;i<3;++i)out<<' '<<profile.unlocked[i]<<' '<<profile.skins[i];out<<'\n';out.close();if(!out){error="Falha ao salvar perfil";return false;}}
    ReplaceSaveFile(temporary,path,ec);if(ec){error=ec.message();return false;}
    // The profile is authoritative; this local skin manifest is a readable mirror.
    auto skins=SkinDirectory(directory)/key;std::filesystem::create_directories(skins,ec);
    if(!ec){auto manifest=skins/"tools.skin",temp=skins/"tools.skin.tmp";std::ofstream out(temp);out<<"MINECLONE_TOOL_SKINS 1\n";for(int i=0;i<3;++i)out<<i<<' '<<profile.unlocked[i]<<' '<<profile.skins[i]<<'\n';out.close();if(out)ReplaceSaveFile(temp,manifest,ec);}
    return true;
}
}
