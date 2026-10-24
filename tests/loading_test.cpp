#include "renderer.h"
#include "bedwars.h"
#include "ui.h"
#include "lighting.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace game;
void Check(bool condition,const char* message){if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(){
    progression::Directory=std::filesystem::temp_directory_path()/("mineclone-loading-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Check(bw::Host("LoadingTest",0,16,3),"host imported NC map");Player player;bw::Update(player,.016f,false,0);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"World loading regression");EnableCursor();
    {
        Renderer renderer;Check(renderer.Initialize(),"initialize renderer");UI ui;
        renderer.BeginWorldLoad();renderer.PrepareWorld(player.Camera(),.5f);renderer.CancelWorldLoad();
        Check(!renderer.PrepareWorld(player.Camera())&&!IsCursorHidden(),"cancel loading without entering world or capturing cursor");
        renderer.BeginWorldLoad();bool done=false,meshStage=false;int frames=0;double maximum=0;
        while(!done&&frames<10000){
            auto start=std::chrono::steady_clock::now();done=renderer.PrepareWorld(player.Camera());maximum=std::max(maximum,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
            ui.loadingProgress=renderer.WorldLoadProgress();ui.loadingText=renderer.WorldLoadStage();meshStage|=ui.loadingProgress>=.4f;
            Check(!IsCursorHidden(),"cursor remains free throughout world preparation");Check(ui.Update(GameState::WorldLoading)==UiAction::None,"loading screen cannot place blocks or activate menu behind it");
            BeginDrawing();ui.Draw(renderer.Atlas(),GameState::WorldLoading);EndDrawing();++frames;
        }
        Check(done&&frames>1&&meshStage&&!LightingPending()&&renderer.WorldLoadProgress()==1.f,"lighting and meshes finish across multiple responsive frames");
        BeginDrawing();renderer.Draw(player.Camera(),{});bw::DrawWorld(player.Camera(),renderer.Atlas());EndDrawing();
        Check(!IsCursorHidden(),"first world frame finishes before cursor capture");DisableCursor();Check(IsCursorHidden(),"cursor can be captured after map is visible");EnableCursor();
        std::printf("Loading: %d frames, max preparation slice %.2f ms\n",frames,maximum);
    }
    bw::Stop();CloseWindow();std::filesystem::remove_all(progression::Directory);std::puts("PASS: cancellable incremental world loading, free cursor and first-frame preparation");
}
