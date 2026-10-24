#include "renderer.h"
#include "projectile_models.h"
#include "bedwars.h"
#include "ui.h"
#include "chest.h"
#include "crafting.h"
#include "items.h"
#include "survival.h"
#include "settings.h"
#include <rlgl.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
namespace {int allocations=0,releases=0;void CountMeshLogs(int,const char* format,va_list args){char line[1024];std::vsnprintf(line,sizeof(line),format,args);if(std::strstr(line,"Mesh uploaded successfully"))++allocations;if(std::strstr(line,"Unloaded vertex array"))++releases;}}
int main() {
    using namespace game;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(720,480,"MineClone renderer test");
    if(!IsWindowReady()) return 1;
    int result=0;
    {
        Renderer renderer;
        if(!renderer.Initialize()) result=1;
        else {
            for(int x=3;x<=9;++x) for(int z=3;z<=9;++z) Change(x,4,z,Stone);
            Change(5,5,5,LampOn);Change(6,5,5,Stone);
            Change(7,7,7,Lava);Change(4,5,5,Water);
            for(int tick=0;tick<4;++tick) UpdateWorld(.15f);
            const Block woolColors[]={WhiteWool,OrangeWool,MagentaWool,LightBlueWool,
                YellowWool,LimeWool,PinkWool,GrayWool,LightGrayWool,CyanWool,
                PurpleWool,BlueWool,BrownWool,GreenWool,RedWool,BlackWool};
            Image atlas=LoadImageFromTexture(renderer.Atlas());
            for(int i=0;i<16;++i) {
                Block wool=woolColors[i];
                if(!Wool(wool)||Plant(wool)||!Solid(wool)||Transparent(wool)||
                   GetImageColor(atlas,BlockTile(wool)*TileSize+8,8).a!=255) result=1;
                Change(3+i%4,5,9+i/4,wool);
            }
            UnloadImage(atlas);
            Change(10,5,9,CraftingTable);Change(11,5,9,IronOre);Change(12,5,9,Furnace);
            Change(9,5,9,Chest);(*ChestAt(9,5,9))[0]={RedWool,64};
            Camera3D camera{{9,9,12},{6,5,6},{0,1,0},70,CAMERA_PERSPECTIVE};
            for(int i=0;i<4;++i) {
                WorldTime=DayDuration*i*.25f;
                BeginDrawing();renderer.Draw(camera,{});EndDrawing();
            }
            WorldTime=DayDuration*.25f;
            // A bed against full-height terrain must retain the surrounding walls and floor.
            for(int x=13;x<=16;++x)for(int z=6;z<=10;++z)Change(x,4,z,Grass);
            Change(14,5,7,Bed);Change(14,5,8,BedHead);
            Change(15,5,7,Grass);Change(15,5,8,Grass);
            Camera3D bedCamera{{12,8,5},{14.5f,5.4f,8},{0,1,0},55,CAMERA_PERSPECTIVE};
            BeginDrawing();renderer.Draw(bedCamera,{});rlDrawRenderBatchActive();
            Image bedCapture=LoadImageFromScreen();ExportImage(bedCapture,"build/bed-penguin.png");UnloadImage(bedCapture);EndDrawing();

            Camera3D crackCamera{{8,7,8},{6.5f,5.5f,5.5f},{0,1,0},55,CAMERA_PERSPECTIVE};
            Hit crackTarget{true,6,5,5};
            for(int stage=0;stage<3;++stage){
                float progress=stage==0?.1f:stage==1?.5f:.9f;
                BeginDrawing();renderer.Draw(crackCamera,crackTarget,progress);rlDrawRenderBatchActive();
                Image image=LoadImageFromScreen();ExportImage(image,TextFormat("build/mining-cracks-%d.png",stage));UnloadImage(image);EndDrawing();
            }
            const Block heldItems[]={Air,Grass,IronSword,IronPickaxe,EnderPearl,Fireball};
            const char* handFiles[]={"build/hand-empty.png","build/hand-block.png","build/hand-sword.png","build/hand-pickaxe.png","build/hand-pearl.png","build/hand-fireball.png"};
            for(int i=0;i<6;++i) for(int frame=0;frame<8;++frame) {
                BeginDrawing();renderer.Draw(camera,{});renderer.DrawFirstPerson(camera,heldItems[i],false,false,.05f);
                if(frame==7) {rlDrawRenderBatchActive();Image capture=LoadImageFromScreen();ExportImage(capture,handFiles[i]);UnloadImage(capture);}
                EndDrawing();
            }
            for(int frame=0;frame<3;++frame) {
                BeginDrawing();renderer.Draw(camera,{});
                renderer.DrawFirstPerson(camera,IronSword,false,frame==0,.05f);
                if(frame==2) {rlDrawRenderBatchActive();Image capture=LoadImageFromScreen();ExportImage(capture,"build/hand-sword-attack.png");UnloadImage(capture);}
                EndDrawing();
            }
            Player bowPlayer;PlayerState bowPose;bowPose.valid=true;bowPose.position=camera.position;bowPlayer.RestoreState(bowPose);Mode=GameMode::Creative;ItemStack displayBow{Bow,1};
            for(int stage=0;stage<4;++stage){CancelItemUse();float seconds=stage==0?-1.f:stage==1?.05f:stage==2?.5f:1.1f;if(stage){UseHeldItem(bowPlayer,displayBow,{0,0,-1},{true,true,false,0});for(int frame=0;frame<int(seconds*100);++frame)UseHeldItem(bowPlayer,displayBow,{0,0,-1},{false,true,false,.01f});}for(int frame=0;frame<8;++frame){BeginDrawing();renderer.Draw(camera,{});renderer.DrawFirstPerson(camera,Bow,false,false,.05f);if(frame==7){rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,TextFormat("build/hand-bow-%d.png",stage));UnloadImage(image);}EndDrawing();}}CancelItemUse();
            ItemStack displayApple{GoldenApple,4};
            for(int stage=0;stage<3;++stage){
                CancelItemUse();
                for(int frame=0;frame<(stage==0?0:stage==1?35:105);++frame)UseHeldItem(bowPlayer,displayApple,{0,0,-1},{false,true,false,.01f});
                for(int frame=0;frame<8;++frame){BeginDrawing();renderer.Draw(camera,{});renderer.DrawFirstPerson(camera,GoldenApple,false,false,.05f);
                    if(frame==7){rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,TextFormat("build/hand-apple-%d.png",stage));UnloadImage(image);}EndDrawing();}
            }
            CancelItemUse();
            BeginDrawing();renderer.Draw(camera,{});BeginMode3D(camera);
            DrawDroppedItem3D(renderer.Atlas(),IronSword,{6,6.3f,7},.5f);DrawDroppedItem3D(renderer.Atlas(),GoldenApple,{7,6.3f,7},.5f);DrawDroppedItem3D(renderer.Atlas(),RedWool,{8,6.3f,7},.5f);
            EndMode3D();rlDrawRenderBatchActive();Image dropped=LoadImageFromScreen();ExportImage(dropped,"build/dropped-items.png");UnloadImage(dropped);EndDrawing();
            BeginDrawing();renderer.Draw(camera,{});BeginMode3D(camera);DrawThrownItem(EnderPearl,{6,6.5f,8},{0,0,-32});DrawThrownItem(Fireball,{7.5f,6.5f,8},{0,0,-30});EndMode3D();rlDrawRenderBatchActive();Image projectiles=LoadImageFromScreen();ExportImage(projectiles,"build/thrown-items.png");UnloadImage(projectiles);EndDrawing();
            for(int x=18;x<=22;++x)for(int y=5;y<=11;++y)Change(x,y,10,Stone);
            Camera3D brightCamera{{20.5f,8,14},{20.5f,8,10},{0,1,0},70,CAMERA_PERSPECTIVE};WorldTime=DayDuration*.75f;
            double totals[2]{};
            for(int bright=0;bright<2;++bright){Settings.fullBright=bright;
                BeginDrawing();renderer.Draw(brightCamera,{});rlDrawRenderBatchActive();Image image=LoadImageFromScreen();
                for(int x=330;x<390;++x)for(int y=210;y<270;++y){auto c=GetImageColor(image,x,y);totals[bright]+=c.r+c.g+c.b;}
                ExportImage(image,bright?"build/full-bright-on.png":"build/full-bright-off.png");UnloadImage(image);EndDrawing();
            }
            if(totals[1]<totals[0]*1.5){std::fprintf(stderr,"FAIL: Full Bright does not illuminate nighttime terrain\n");result=1;}Settings.fullBright=false;
            Camera3D skyCamera{{9,9,12},{9,40,-40},{0,1,0},70,CAMERA_PERSPECTIVE};
            for(int night=0;night<2;++night){WorldTime=night?DayDuration*.75f:DayDuration*.25f;BeginDrawing();renderer.Draw(skyCamera,{});rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,night?"build/sky-night.png":"build/sky-day.png");UnloadImage(image);EndDrawing();}
            WorldTime=DayDuration*.25f;
            const bool fogBefore=Settings.fogEnabled;
            Settings.fogEnabled=false;Settings.renderChunks=6;
            BeginDrawing();renderer.Draw(camera,{});EndDrawing();
            Settings.fogEnabled=fogBefore;
            for(int frame=0;frame<16;++frame){BeginDrawing();renderer.Draw(camera,{});EndDrawing();}
            allocations=releases=0;SetTraceLogCallback(CountMeshLogs);
            for(int edit=0;edit<20;++edit){Change(6,4,6,edit%2?Dirt:Stone);BeginDrawing();renderer.Draw(camera,{});EndDrawing();}
            SetTraceLogCallback(nullptr);
            if(allocations||releases){std::fprintf(stderr,"FAIL: same-size chunk edits reallocated GPU buffers (%d/%d)\n",allocations,releases);result=1;}
            UI ui;
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::Keyboard);rlDrawRenderBatchActive();Image keyboard=LoadImageFromScreen();ExportImage(keyboard,"build/keyboard-options.png");UnloadImage(keyboard);EndDrawing();
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::Options);
            rlDrawRenderBatchActive();Image options=LoadImageFromScreen();ExportImage(options,"build/options-preview.png");UnloadImage(options);EndDrawing();
            for(int tab=0;tab<3;++tab){
                // Mouse automation IDs from raylib's AutomationEventType.
                PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});BeginDrawing();EndDrawing();
                PlayAutomationEvent({0,7,{int(GetScreenWidth()/2.f-320+20+tab*202+98),int(GetScreenHeight()/2.f-224+65),0,0}});
                PlayAutomationEvent({0,6,{MOUSE_BUTTON_LEFT,0,0,0}});ui.Update(GameState::Options);
                BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::Options);rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,TextFormat("build/options-tab-%d.png",tab));UnloadImage(image);EndDrawing();
            }
            PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});BeginDrawing();EndDrawing();
            ui.Update(GameState::Minigames);
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::Minigames);
            rlDrawRenderBatchActive();Image multiplayer=LoadImageFromScreen();ExportImage(multiplayer,"build/minigames-preview.png");UnloadImage(multiplayer);EndDrawing();
            LanDiscovery sample;LanRoom room;room.name="LocalHost";room.players=2;room.map="NC";room.playerNames={"LocalHost","Friend"};sample.Advertise(room);bw::SearchRooms();
            for(int i=0;i<8;++i){sample.Poll();bw::UpdateDiscovery();}
            BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsMenu);rlDrawRenderBatchActive();Image browser=LoadImageFromScreen();ExportImage(browser,"build/server-browser.png");UnloadImage(browser);EndDrawing();sample.StopHosting();
            for(int basic=0;basic<2;++basic){ui.standardLayout=basic!=0;ui.Update(GameState::BedwarsMaps);BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsMaps);rlDrawRenderBatchActive();Image maps=LoadImageFromScreen();ExportImage(maps,basic?"build/basic-maps.png":"build/advanced-maps.png");UnloadImage(maps);EndDrawing();}
            BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsMode);rlDrawRenderBatchActive();Image create=LoadImageFromScreen();ExportImage(create,"build/create-room-layout.png");UnloadImage(create);EndDrawing();
            ui.OpenChest(9,5,9);
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::ChestScreen);
            rlDrawRenderBatchActive();
            Image preview=LoadImageFromScreen();ExportImage(preview,"build/chest-preview.png");UnloadImage(preview);
            EndDrawing();
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::Inventory);
            rlDrawRenderBatchActive();preview=LoadImageFromScreen();
            ExportImage(preview,"build/inventory-preview.png");UnloadImage(preview);EndDrawing();
            WorkbenchCrafting[1]={IronIngot,1};WorkbenchCrafting[4]={IronIngot,1};WorkbenchCrafting[7]={Stick,1};
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::CraftScreen);
            rlDrawRenderBatchActive();preview=LoadImageFromScreen();ExportImage(preview,"build/crafting-preview.png");UnloadImage(preview);EndDrawing();
            ui.OpenChest(12,5,9);FurnaceAt(12,5,9)->slots[0]={IronOre,2};
            BeginDrawing();ClearBackground(DARKGRAY);ui.Draw(renderer.Atlas(),GameState::FurnaceScreen);EndDrawing();
        }
    }
    CloseWindow();
    if(!result) std::puts("PASS: GLSL shaders and voxel rendering at four solar phases");
    return result;
}
