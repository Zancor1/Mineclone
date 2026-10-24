#include "main.h"
#include "game.h"
#include <raylib.h>

int game::Run() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280,720,"MineClone");
    SetWindowMinSize(720,480);
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);
    const int result=RunGame();
    CloseWindow();
    return result;
}

int main() {return game::Run();}
