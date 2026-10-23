#include "renderer.h"
#include <cstdio>
int main() {
    using namespace game;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(160,120,"MineClone renderer test");
    if(!IsWindowReady()) return 1;
    int result=0;
    {
        Renderer renderer;
        if(!renderer.Initialize()) result=1;
        else {
            for(int x=3;x<=9;++x) for(int z=3;z<=9;++z) Change(x,4,z,Stone);
            Change(5,5,5,LampOn);Change(6,5,5,Stone);
            Camera3D camera{{9,9,12},{6,5,6},{0,1,0},70,CAMERA_PERSPECTIVE};
            for(int i=0;i<4;++i) {
                WorldTime=DayDuration*i*.25f;
                BeginDrawing();renderer.Draw(camera,{});EndDrawing();
            }
        }
    }
    CloseWindow();
    if(!result) std::puts("PASS: GLSL shaders and voxel rendering at four solar phases");
    return result;
}
