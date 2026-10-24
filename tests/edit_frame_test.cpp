#include "bedwars.h"
#include "renderer.h"
#include "lighting.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <vector>
int main(){using namespace game;
    progression::Directory=std::filesystem::temp_directory_path()/("mineclone-edit-frames-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!bw::Host("FrameTest",0,16,3))return 1;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"Block edit frame regression");
    {
        Renderer renderer;if(!renderer.Initialize())return 2;
        Camera3D camera{{77,28,154},{83,26,156},{0,1,0},70,CAMERA_PERSPECTIVE};
        for(int i=0;i<25;++i){BeginDrawing();renderer.Draw(camera,{});EndDrawing();}
        std::vector<double> edits,all;int y=26;while(y<Height-1&&At(78,y,156)!=Air)++y;
        for(int frame=0;frame<240;++frame){auto start=std::chrono::steady_clock::now();bool edit=frame%30==0;if(edit)Change(78,y,156,(frame/30)%2?Air:WhiteWool);BeginDrawing();renderer.Draw(camera,{});EndDrawing();double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();all.push_back(ms);if(edit)edits.push_back(ms);}
        for(int n=0;n<1000&&LightingPending();++n){BeginDrawing();renderer.Draw(camera,{});EndDrawing();}
        if(LightingPending())return 3;
        std::sort(all.begin(),all.end());
        std::printf("Edit frames: max %.2f ms; all frames p95 %.2f ms, max %.2f ms\n",*std::max_element(edits.begin(),edits.end()),all[all.size()*95/100],all.back());
    }
    bw::Stop();CloseWindow();std::filesystem::remove_all(progression::Directory);std::puts("PASS: repeated block placement/removal renders and lighting converges on imported NC map");
}
