#include "anticheat.h"
#include "main.h"
#include "game.h"
#include "android_platform.h"
#include <raylib.h>

namespace {
int RunSession() {
    using namespace game;
    if(!ac::StartMonitor()){ac::ShowWarning();ac::StopMonitor();return 1;}
    struct MonitorLifetime {~MonitorLifetime(){ac::StopMonitor();}} monitor;
    SetTraceLogLevel(LOG_WARNING);
#ifdef __ANDROID__
    SetConfigFlags(0);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
#endif
    InitWindow(1280,720,"MineClone");
#ifndef __ANDROID__
    SetWindowMinSize(720,480);
#endif
    if(!PrepareAndroidStorage()){TraceLog(LOG_ERROR,"Falha ao preparar dados privados do aplicativo");CloseWindow();return 1;}
    SetExitKey(KEY_NULL);
#ifdef __ANDROID__
    SetTargetFPS(60);
#else
    SetTargetFPS(120);
#endif
    InitAudioDevice();
    const int result=RunGame();
    if(IsAudioDeviceReady())CloseAudioDevice();
    CancelAndroidText();CloseWindow();
    bool integrityFailure=ac::Detected();ac::StopMonitor();
    if(integrityFailure){ac::ShowWarning();return 1;}
    return result;
}

}
int game::Run(){return RunSession();}
int main() {return game::Run();}
