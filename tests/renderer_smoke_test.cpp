#include "renderer.h"
#include "bedwars.h"
#include "ui.h"
#include "chest.h"
#include "crafting.h"
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
            const Block heldItems[]={Air,Grass,IronSword,IronPickaxe};
            const char* handFiles[]={"build/hand-empty.png","build/hand-block.png","build/hand-sword.png","build/hand-pickaxe.png"};
            for(int i=0;i<4;++i) for(int frame=0;frame<8;++frame) {
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
