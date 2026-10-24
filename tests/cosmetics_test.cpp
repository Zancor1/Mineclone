#include "bedwars.h"
#include "cosmetics.h"
#include "renderer.h"
#include <raymath.h>
#include <rlgl.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
using namespace game;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main(){
    Check(cosmetics::Category(IronSword)==0&&cosmetics::Category(WoodenPickaxe)==1&&cosmetics::Category(IronAxe)==2,"three supported finish categories");
    Check(cosmetics::Category(Stick)==-1&&cosmetics::Category(WhiteWool)==-1&&cosmetics::Category(Bow)==-1,"other items cannot receive finishes");
    progression::Directory=std::filesystem::temp_directory_path()/("mineclone-cosmetics-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    progression::Profile profile;profile.Award(1000);profile.Claim();for(auto& mask:profile.unlocked)mask=(1u<<(progression::SkinCount+1))-1;
    std::string error;Check(progression::Save("Preview",profile,error),"prepare graphical profile");Check(bw::Host("Preview",0),"start cosmetic lobby");
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"Tool cosmetics regression");
    {
        Renderer renderer;Check(renderer.Initialize(),"initialize renderer");auto texture=LoadRenderTexture(160,160);Camera3D camera{{0,0,2},{0,0,0},{0,1,0},50,CAMERA_PERSPECTIVE};
        auto capture=[&](Block item,int finish){BeginTextureMode(texture);ClearBackground(BLACK);BeginMode3D(camera);DrawHeldItem3D(renderer.Atlas(),item,MatrixIdentity(),WHITE,finish);EndMode3D();EndTextureMode();return LoadImageFromTexture(texture.texture);};
        for(Block item:{WoodenSword,IronSword,WoodenPickaxe,IronPickaxe,WoodenAxe,IronAxe}){
            Image normal=capture(item,0),painted=capture(item,1);int changed=0,shape=0;
            for(int y=0;y<160;++y)for(int x=0;x<160;++x){auto a=GetImageColor(normal,x,y),b=GetImageColor(painted,x,y);bool present=a.r||a.g||a.b,other=b.r||b.g||b.b;if(present!=other)++shape;if(present&&(a.r!=b.r||a.g!=b.g||a.b!=b.b))++changed;}
            Check(changed>30&&shape==0,"wood and iron tools receive colored finishes without geometry changes");UnloadImage(normal);UnloadImage(painted);
        }
        Image stick=capture(Stick,0),unchanged=capture(Stick,5);int differences=0;for(int y=0;y<160;++y)for(int x=0;x<160;++x){auto a=GetImageColor(stick,x,y),b=GetImageColor(unchanged,x,y);differences+=(a.r!=b.r||a.g!=b.g||a.b!=b.b);}Check(differences==0,"unsupported held items remain unchanged");UnloadImage(stick);UnloadImage(unchanged);UnloadRenderTexture(texture);
        auto screenshot=[](const char* path){rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,path);UnloadImage(image);};
        cosmetics::Toggle();BeginDrawing();ClearBackground(BLACK);bw::DrawInventory(renderer.Atlas(),false);screenshot("build/tool-skins-inventory.png");EndDrawing();
        bw::OpenCrate();Check(bw::Progress().crates==19,"opening consumes exactly one free crate");BeginDrawing();bw::DrawInventory(renderer.Atlas(),false);screenshot("build/tool-skins-roulette.png");EndDrawing();
        WaitTime(4.4);BeginDrawing();bw::DrawInventory(renderer.Atlas(),false);screenshot("build/tool-skins-reward.png");EndDrawing();
        bw::EquipSkin(0,1);bw::EquipSkin(1,10);bw::EquipSkin(2,5);Check(bw::ItemSkin(IronSword)==1&&bw::ItemSkin(IronPickaxe)==10&&bw::ItemSkin(IronAxe)==5&&bw::ItemSkin(Stick)==0,"equipped finishes apply independently");
        Camera3D firstPerson{{128,70,128},{128,70,127},{0,1,0},70,CAMERA_PERSPECTIVE};for(int frame=0;frame<12;++frame){BeginDrawing();ClearBackground({130,180,210,255});renderer.DrawFirstPerson(firstPerson,IronSword,false,false,.05f);if(frame==11){rlDrawRenderBatchActive();Image hand=LoadImageFromScreen();int painted=0;for(int y=0;y<hand.height;++y)for(int x=0;x<hand.width;++x){auto c=GetImageColor(hand,x,y);if(c.r>30&&c.r>c.g*2&&c.r>c.b*2)++painted;}Check(painted>100,"equipped red sword finish is visible in first person");ExportImage(hand,"build/tool-skins-hand.png");UnloadImage(hand);}EndDrawing();}
    }
    bw::Stop();CloseWindow();std::filesystem::remove_all(progression::Directory);std::puts("PASS: tool finish geometry, default appearance, skin inventory, free roulette and first-person rendering");
}
