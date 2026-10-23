#include "game.h"
#include "world.h"
#include "player.h"
#include "renderer.h"
#include "ui.h"
#include "settings.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace game {
int RunGame() {
    LoadSettings();
    SetClockMinute(Settings.newWorldMinute);
    Renderer renderer;
    if(!renderer.Initialize()) {
        TraceLog(LOG_ERROR,"Falha ao carregar atlas/shader GLSL 330.");
        return 1;
    }
    Player player;
    UI ui;
    GameState state=GameState::MainMenu;
    GameState optionsParent=GameState::MainMenu;
    bool hasWorld=false,quit=false;
    std::string activeSave;
    EnableCursor();
    while(!WindowShouldClose()&&!quit) {
        bool transition=false;
        auto enter=[&](GameState next) {
            state=next;transition=true;ui.ResetInteraction();
            if(state==GameState::Playing&&IsWindowFocused()) DisableCursor();
            else EnableCursor();
            if(hasWorld&&!activeSave.empty()&&(state==GameState::Paused||state==GameState::Inventory)) {
                if(!SaveWorld(activeSave,player.CaptureState())) ui.Notify("Falha ao salvar a posicao. Tente salvar novamente.");
            }
        };
        auto closeOptions=[&]() {
            if(SaveSettings()) enter(optionsParent);
            else ui.Notify("Falha ao gravar settings.cfg. Tente novamente.");
        };
        if(IsKeyPressed(KEY_ESCAPE)) {
            if(state==GameState::Options) closeOptions();
            else if(state==GameState::Playing) enter(GameState::Paused);
            else if(state==GameState::Paused||state==GameState::Inventory) enter(GameState::Playing);
            else if(state==GameState::WorldSelection) enter(GameState::MainMenu);
        } else if(IsKeyPressed(KEY_E)) {
            if(state==GameState::Playing) enter(GameState::Inventory);
            else if(state==GameState::Inventory) enter(GameState::Playing);
        }
        if(!IsWindowFocused()&&(state==GameState::Playing||state==GameState::Inventory)) enter(GameState::Paused);

        UiAction action=UiAction::None;
        if(!transition&&IsWindowFocused()) action=ui.Update(state);
        switch(action) {
            case UiAction::OpenOptions:
                if(state==GameState::MainMenu||state==GameState::Paused) {
                    optionsParent=state;enter(GameState::Options);
                }
                break;
            case UiAction::CloseOptions:
                if(state==GameState::Options) closeOptions();
                break;
            case UiAction::Resume:
                if(state==GameState::Paused&&hasWorld) enter(GameState::Playing);
                break;
            case UiAction::SaveAndReturn:
                if(state==GameState::Paused&&hasWorld) {
                    if(SaveWorld(activeSave,player.CaptureState())) enter(GameState::MainMenu);
                    else ui.Notify("Falha ao salvar. O mundo continua aberto.");
                }
                break;
            case UiAction::QuitApplication:
                if(state==GameState::MainMenu) quit=true;
                break;
            case UiAction::OpenWorlds:
                enter(GameState::WorldSelection);ui.SetWorlds(ListWorldSaves());
                break;
            case UiAction::BackToMenu:
                enter(GameState::MainMenu);
                break;
            case UiAction::LoadSelected: {
                if(state!=GameState::WorldSelection) break;
                const std::string path=ui.SelectedWorldPath();
                PlayerState savedPlayer;
                if(LoadWorld(path,&savedPlayer)) {
                    activeSave=path;hasWorld=true;
                    if(!player.RestoreState(savedPlayer)) player.Respawn();
                    enter(GameState::Playing);
                } else ui.Notify("Save invalido ou ilegivel. Escolha outro mundo.");
                break;
            }
            case UiAction::DeleteSelected: {
                if(state!=GameState::WorldSelection) break;
                const std::string path=ui.SelectedWorldPath();
                if(DeleteWorldSave(path)) {
                    if(path==activeSave) {activeSave.clear();hasWorld=false;}
                    ui.SetWorlds(ListWorldSaves());ui.Notify("Mundo excluido permanentemente.");
                } else ui.Notify("Nao foi possivel excluir o save. Arquivo preservado.");
                break;
            }
            case UiAction::NewWorld: {
                if(state!=GameState::WorldSelection) break;
                if(hasWorld&&!activeSave.empty()&&!SaveWorld(activeSave,player.CaptureState())) {
                    ui.Notify("Falha ao salvar o mundo atual. Criacao cancelada.");break;
                }
                const std::string path=CreateWorldSavePath();
                if(path.empty()) {ui.Notify("Nao foi possivel criar a pasta do mundo.");break;}
                GenerateWorld(RandomSeed());player.Respawn();hasWorld=true;activeSave=path;
                if(SaveWorld(activeSave,player.CaptureState())) enter(GameState::Playing);
                else {
                    enter(GameState::Paused);
                    ui.Notify("Falha ao gravar o novo mundo. Tente salvar novamente.");
                }
                break;
            }
            case UiAction::None: break;
        }
        if(quit) break;
        const bool active=state==GameState::Playing&&IsWindowFocused()&&!transition;
        if(active) {
            PlayerInput input;
            input.look=GetMouseDelta();
            input.forward=float(IsKeyDown(KEY_W))-float(IsKeyDown(KEY_S));
            input.side=float(IsKeyDown(KEY_D))-float(IsKeyDown(KEY_A));
            input.jump=IsKeyDown(KEY_SPACE);input.swimUp=IsKeyDown(KEY_SPACE);
            input.swimDown=IsKeyDown(KEY_LEFT_SHIFT);
            float dt=std::min(GetFrameTime(),.05f);
            if(IsKeyPressed(KEY_F6)&&!Settings.freezeTime) WorldTime=std::fmod(WorldTime+DayDuration*.25f,DayDuration);
            player.Update(input,dt);UpdateWorld(dt);
        }
        Hit hit;
        Camera3D camera{};
        if(hasWorld&&(state==GameState::Playing||state==GameState::Inventory||state==GameState::Paused||
                     (state==GameState::Options&&optionsParent==GameState::Paused))) {
            camera=player.Camera();
            if(active) {
                Vector3 direction=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
                bool targetWater=IsKeyDown(KEY_LEFT_ALT)||IsKeyDown(KEY_RIGHT_ALT);
                hit=Trace(camera.position,direction,targetWater);
                if(hit.found) {
                    if(IsKeyPressed(KEY_F)&&(At(hit.x,hit.y,hit.z)==LampOn||At(hit.x,hit.y,hit.z)==LampOff))
                        Change(hit.x,hit.y,hit.z,At(hit.x,hit.y,hit.z)==LampOn?LampOff:LampOn);
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Change(hit.x,hit.y,hit.z,Air);
                    else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        int x=hit.x+hit.nx,y=hit.y+hit.ny,z=hit.z+hit.nz;
                        Block block=ui.SelectedBlock(),previous=At(x,y,z);
                        if(Inside(x,y,z)&&(previous==Air||previous==Water)&&(!Solid(block)||!player.Overlaps(x,y,z))) Change(x,y,z,block);
                    }
                    hit=Trace(camera.position,direction,targetWater);
                }
            }
        }
        BeginDrawing();
        if(hasWorld&&(state==GameState::Playing||state==GameState::Inventory||state==GameState::Paused||
                     (state==GameState::Options&&optionsParent==GameState::Paused))) renderer.Draw(camera,hit);
        else ClearBackground({43,41,37,255});
        ui.Draw(renderer.Atlas(),state);
        EndDrawing();
    }
    if(!SaveSettings()) TraceLog(LOG_WARNING,"Falha ao salvar settings.cfg.");
    if(hasWorld&&!activeSave.empty()&&!SaveWorld(activeSave,player.CaptureState())) {
        TraceLog(LOG_ERROR,"Falha ao salvar o mundo e a posicao ao fechar.");
        EnableCursor();return 1;
    }
    EnableCursor();
    return 0;
}
}
