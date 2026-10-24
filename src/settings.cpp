#include "file_replace.h"
#include "settings.h"
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
namespace game {
float RenderDistance() {return float(std::clamp(Settings.renderChunks,MinRenderChunks,MaxRenderChunks)*16);}
float FogDensity(bool underwater) {
    if(underwater) return .12f;
    return Settings.fogEnabled?1.5f/std::clamp(Settings.fogDistance,8.f,192.f):0.f;
}
bool LoadSettings(const std::string& path) {
    std::ifstream in(path);
    std::string magic;int version=0,frozen=0;
    GameSettings loaded;
    if(!(in>>magic>>version>>frozen>>loaded.mouseSensitivity>>loaded.newWorldMinute)||
       magic!="MINECLONE_SETTINGS"||(version<1||version>5)||(frozen!=0&&frozen!=1)||
       !std::isfinite(loaded.mouseSensitivity)||loaded.mouseSensitivity<.1f||loaded.mouseSensitivity>3.f||
       loaded.newWorldMinute<0||loaded.newWorldMinute>1439) return false;
    if(version>=2) {
        int fog;
        if(!(in>>fog>>loaded.renderChunks>>loaded.fogDistance)||(fog!=0&&fog!=1)||
           loaded.renderChunks<MinRenderChunks||loaded.renderChunks>MaxRenderChunks||
           !std::isfinite(loaded.fogDistance)||loaded.fogDistance<8||loaded.fogDistance>192) return false;
        loaded.fogEnabled=fog!=0;
    }
    if(version>=3){
        if(!(in>>std::quoted(loaded.playerName)>>std::quoted(loaded.serverAddress))||loaded.playerName.empty()||loaded.playerName.size()>16||loaded.playerName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos||loaded.serverAddress.empty()||loaded.serverAddress.size()>253)return false;
    }
    if(version>=4){int count=0;if(!(in>>loaded.fov>>count)||!std::isfinite(loaded.fov)||loaded.fov<40||loaded.fov>110||(count!=BindingCount&&count!=BindingCount-1))return false;
        for(int i=0;i<count;++i){int key;if(!(in>>key)||!((key>=32&&key<=348)||(key>=-7&&key<=0)))return false;for(int j=0;j<i;++j)if(key&&loaded.bindings[j]==key)return false;loaded.bindings[i]=key;}if(count<BindingCount){for(int i=0;i<count;++i)if(loaded.bindings[i]==KEY_Q)loaded.bindings[int(BindingAction::Drop)]=0;}}
    if(version>=5){int toggle=0;if(!(in>>toggle)||(toggle!=0&&toggle!=1))return false;loaded.toggleSprint=toggle!=0;}
    in>>std::ws;if(in.peek()!=std::char_traits<char>::eof()) return false;
    loaded.freezeTime=frozen!=0;Settings=loaded;return true;
}
bool SaveSettings(const std::string& path) {
    const std::string temporary=path+".tmp";
    std::ofstream out(temporary,std::ios::trunc);
    if(!out) return false;
    out<<"MINECLONE_SETTINGS 5\n"<<int(Settings.freezeTime)<<' '<<std::setprecision(9)
       <<Settings.mouseSensitivity<<' '<<Settings.newWorldMinute<<'\n'
       <<int(Settings.fogEnabled)<<' '<<Settings.renderChunks<<' '<<Settings.fogDistance<<'\n'
       <<std::quoted(Settings.playerName)<<' '<<std::quoted(Settings.serverAddress)<<'\n';
    out<<Settings.fov<<' '<<BindingCount<<'\n';for(int key:Settings.bindings)out<<key<<' ';out<<'\n';
    out<<int(Settings.toggleSprint)<<'\n';
    out.flush();bool ok=bool(out);out.close();ok=ok&&!out.fail();
    std::error_code error;
    if(ok) {ReplaceSaveFile(temporary,path,error);ok=!error;}
    if(!ok) std::filesystem::remove(temporary,error);
    return ok;
}
}

namespace game {
const char* BindingLabel(BindingAction action){static const char* labels[]={"Andar para frente","Andar para tras","Andar para esquerda","Andar para direita","Pular / subir voando","Agachar / descer","Correr","Inventario","Pausar","Atacar / quebrar","Usar / colocar","Chat","Comando no chat","Lista de jogadores","Selecionar liquido","Alternar lanterna","Avancar horario","Iniciar BedWars (host)","Reiniciar BedWars (host)","Hotbar 1","Hotbar 2","Hotbar 3","Hotbar 4","Hotbar 5","Hotbar 6","Hotbar 7","Hotbar 8","Hotbar 9","Dropar item (Ctrl: pilha)"};return labels[std::clamp(int(action),0,BindingCount-1)];}
std::string BindingKeyName(int key){if(!key)return "Nenhuma";if(key<0){const char* names[]={"Mouse esquerdo","Mouse direito","Mouse meio","Mouse lateral 1","Mouse lateral 2","Mouse extra 1","Mouse extra 2"};return names[-key-1];}if((key>=KEY_A&&key<=KEY_Z)||(key>=KEY_ZERO&&key<=KEY_NINE))return std::string(1,char(key));if(key>=KEY_F1&&key<=KEY_F12)return "F"+std::to_string(key-KEY_F1+1);switch(key){case KEY_SPACE:return "Espaco";case KEY_LEFT_SHIFT:return "Shift esquerdo";case KEY_RIGHT_SHIFT:return "Shift direito";case KEY_LEFT_CONTROL:return "Ctrl esquerdo";case KEY_RIGHT_CONTROL:return "Ctrl direito";case KEY_LEFT_ALT:return "Alt esquerdo";case KEY_RIGHT_ALT:return "Alt direito";case KEY_ESCAPE:return "Esc";case KEY_TAB:return "Tab";case KEY_ENTER:return "Enter";case KEY_SLASH:return "/";case KEY_UP:return "Seta cima";case KEY_DOWN:return "Seta baixo";case KEY_LEFT:return "Seta esquerda";case KEY_RIGHT:return "Seta direita";default:return "Tecla "+std::to_string(key);}}
bool SetBinding(BindingAction action,int key){int index=int(action);if(index<0||index>=BindingCount||!((key>=32&&key<=348)||(key>=-7&&key<=0)))return false;for(int i=0;i<BindingCount;++i)if(i!=index&&key&&Settings.bindings[i]==key)return false;Settings.bindings[index]=key;return true;}
bool ActionDown(BindingAction action){int key=Settings.bindings[int(action)];return key<0?IsMouseButtonDown(-key-1):key>0&&IsKeyDown(key);}
bool ActionPressed(BindingAction action){int key=Settings.bindings[int(action)];return key<0?IsMouseButtonPressed(-key-1):key>0&&IsKeyPressed(key);}
bool ActionReleased(BindingAction action){int key=Settings.bindings[int(action)];return key<0?IsMouseButtonReleased(-key-1):key>0&&IsKeyReleased(key);}
}
