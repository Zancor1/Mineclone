#include "settings.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
namespace game {
bool LoadSettings(const std::string& path) {
    std::ifstream in(path);
    std::string magic;int version=0,frozen=0;
    GameSettings loaded;
    if(!(in>>magic>>version>>frozen>>loaded.mouseSensitivity>>loaded.newWorldMinute)||
       magic!="MINECLONE_SETTINGS"||version!=1||(frozen!=0&&frozen!=1)||
       !std::isfinite(loaded.mouseSensitivity)||loaded.mouseSensitivity<.1f||loaded.mouseSensitivity>3.f||
       loaded.newWorldMinute<0||loaded.newWorldMinute>1439) return false;
    in>>std::ws;if(in.peek()!=std::char_traits<char>::eof()) return false;
    loaded.freezeTime=frozen!=0;Settings=loaded;return true;
}
bool SaveSettings(const std::string& path) {
    const std::string temporary=path+".tmp";
    std::ofstream out(temporary,std::ios::trunc);
    if(!out) return false;
    out<<"MINECLONE_SETTINGS 1\n"<<int(Settings.freezeTime)<<' '<<std::setprecision(9)
       <<Settings.mouseSensitivity<<' '<<Settings.newWorldMinute<<'\n';
    out.flush();bool ok=bool(out);out.close();ok=ok&&!out.fail();
    std::error_code error;
    if(ok) {std::filesystem::rename(temporary,path,error);ok=!error;}
    if(!ok) std::filesystem::remove(temporary,error);
    return ok;
}
}
