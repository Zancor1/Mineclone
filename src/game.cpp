#include "game.h"
#include "action_repeat.h"
#include "map_blocks.h"
#include "bedwars.h"
#include "world.h"
#include "player.h"
#include "renderer.h"
#include "ui.h"
#include "settings.h"
#include "chest.h"
#include "crafting.h"
#include "survival.h"
#include "items.h"
#include "network.h"
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
    UI ui;ui.playerName=Settings.playerName;ui.serverAddress=Settings.serverAddress;
    GameState state=GameState::MainMenu;
    GameState optionsParent=GameState::MainMenu;
    bool hasWorld=false,quit=false,online=false,worldPrimed=false;
    int lastSelected=0,singleUseSlot=-1;PlacementRepeat placementRepeat;
    std::string activeSave;
    Hit miningTarget;Block miningTool=Air;
    EnableCursor();
    while(!WindowShouldClose()&&!quit) {
        bw::UpdateDiscovery();
        bool transition=false;const bool typingAtFrameStart=ui.chatOpen;
        auto enter=[&](GameState next) {
            const GameState previousState=state;
            ui.miningProgress=0;miningTarget={};CancelItemUse();
            state=next;transition=true;ui.ResetInteraction();if(state!=GameState::Playing){ui.chatOpen=false;ui.chatText.clear();}
            if(state==GameState::Playing&&IsWindowFocused()) DisableCursor();
            else EnableCursor();
            if(hasWorld&&!activeSave.empty()&&(state==GameState::Paused||state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)||(previousState==GameState::ChestScreen||previousState==GameState::CraftScreen||previousState==GameState::FurnaceScreen))) {
                if(!SaveWorld(activeSave,player.CaptureState())) ui.Notify("Falha ao salvar a posicao. Tente salvar novamente.");
            }
        };
        auto closeOptions=[&]() {
            if(SaveSettings()) enter(optionsParent);
            else ui.Notify("Falha ao gravar settings.cfg. Tente novamente.");
        };
        const bool menuKey=IsKeyPressed(KEY_ESCAPE)||((state==GameState::Playing||state==GameState::Paused)&&ActionPressed(BindingAction::Pause));
        if(!ui.CapturingBinding()&&menuKey&&ui.chatOpen){ui.chatOpen=false;ui.chatText.clear();DisableCursor();}
        else if(!ui.CapturingBinding()&&menuKey) {
            if(state==GameState::WorldLoading){bw::ConnectionTrace("loading cancelled by player");renderer.CancelWorldLoad();bw::Stop();hasWorld=worldPrimed=false;enter(GameState::BedwarsMenu);}
            else if(state==GameState::BedwarsTeams||state==GameState::BedwarsShop||state==GameState::BedwarsInventory||state==GameState::BedwarsChest){enter(GameState::Playing);}
            else if(state==GameState::BedwarsMaps)enter(ui.standardLayout?GameState::BedwarsRush:GameState::BedwarsMode);
            else if(state==GameState::BedwarsDiscovery)enter(GameState::Minigames);
            else if(state==GameState::BedwarsRush)enter(GameState::BedwarsMode);
            else if(state==GameState::BedwarsMode)enter(GameState::BedwarsMenu);
            else if(state==GameState::BedwarsMenu){bw::Stop();enter(GameState::Minigames);}
            else if(state==GameState::Minigames)enter(GameState::MainMenu);
            else if(state==GameState::ServerInventory){net::CloseWindow();enter(GameState::Playing);}
            else if(state==GameState::Multiplayer){net::Disconnect();online=false;hasWorld=false;enter(GameState::MainMenu);}
            else if(state==GameState::Keyboard)enter(GameState::Options);
            else if(state==GameState::Options) closeOptions();
            else if(state==GameState::Playing) enter(GameState::Paused);
            else if(state==GameState::Paused||state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)) enter(GameState::Playing);
            else if(state==GameState::WorldSelection) enter(GameState::MainMenu);
        } else if(ActionPressed(BindingAction::Inventory)&&!ui.chatOpen) {
            if(bw::Ready()&&state==GameState::Playing)enter(GameState::BedwarsInventory);
            else if(bw::Ready()&&(state==GameState::BedwarsInventory||state==GameState::BedwarsChest))enter(GameState::Playing);
            else if(online&&state==GameState::Playing){net::ShowInventory();enter(GameState::ServerInventory);}
            else if(online&&state==GameState::ServerInventory){net::CloseWindow();enter(GameState::Playing);}
            else if(state==GameState::Playing) enter(GameState::Inventory);
            else if(state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)) enter(GameState::Playing);
        }
        if(!IsWindowFocused()&&(state==GameState::Playing||state==GameState::BedwarsTeams||state==GameState::BedwarsShop||state==GameState::BedwarsInventory||state==GameState::BedwarsChest||state==GameState::ServerInventory||state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen))) enter(GameState::Paused);

        UiAction action=UiAction::None;
        if(!transition&&IsWindowFocused()) action=ui.Update(state);
        switch(action) {
            case UiAction::DropCursor:DropItem(player,CursorStack,true);break;
            case UiAction::DropCursorSingle:DropItem(player,CursorStack,false);break;
            case UiAction::SearchBedwars:bw::SearchRooms();enter(GameState::BedwarsDiscovery);break;
            case UiAction::OpenMinigames:enter(GameState::Minigames);break;
            case UiAction::OpenBedwars:if(!bw::Active())bw::SearchRooms();enter(GameState::BedwarsMenu);break;
            case UiAction::ChooseMap:enter(GameState::BedwarsMaps);break;
            case UiAction::ChooseMode:enter(GameState::BedwarsMode);break;
            case UiAction::ChooseRush:enter(GameState::BedwarsRush);break;
            case UiAction::OpenTester:
            case UiAction::HostBedwars:
            case UiAction::JoinBedwars:{
                if(bw::Active())break;
                if(hasWorld&&!activeSave.empty()&&!SaveWorld(activeSave,player.CaptureState())){ui.Notify("Falha ao salvar o mundo local");break;}
                net::Disconnect();online=false;activeSave.clear();hasWorld=false;
                bool ok=action==UiAction::OpenTester?bw::StartTester(ui.playerName):action==UiAction::HostBedwars?bw::Host(ui.playerName,25566,ui.rushGap,ui.mapChoice,ui.teamCapacity,ui.roomName):bw::Join(ui.bedwarsAddress,ui.playerName);
                if(!ok){enter(GameState::BedwarsMenu);ui.Notify(bw::Status());}
                else {ui.selected=0;Settings.playerName=ui.playerName;if(!SaveSettings())ui.Notify("Nao foi possivel salvar o nick");enter(GameState::BedwarsMenu);}
                break;
            }

            case UiAction::OpenMultiplayer:enter(GameState::Multiplayer);break;
            case UiAction::ConnectServer:
                if(hasWorld&&!activeSave.empty()&&!SaveWorld(activeSave,player.CaptureState())) {ui.Notify("Falha ao salvar o mundo local.");break;}
                hasWorld=false;activeSave.clear();ClearItems();online=true;net::Connect(ui.serverAddress,ui.playerName);
                if(net::Active()){Settings.playerName=ui.playerName;Settings.serverAddress=ui.serverAddress;if(!SaveSettings())ui.Notify("Nao foi possivel salvar o nick e o endereco.");}break;
            case UiAction::DisconnectServer:net::Disconnect();online=false;hasWorld=false;break;
            case UiAction::Respawn:
                if(online&&state==GameState::Dead){net::Respawn();break;}
                if(state==GameState::Dead) {
                    Backpack={};Hotbar={};CursorStack={};EquippedArmor={};ClearItems();PersonalCrafting={};WorkbenchCrafting={};
                    ResetVitals();player.Respawn();enter(GameState::Playing);SaveWorld(activeSave,player.CaptureState());
                }
                break;
            case UiAction::CloseChest:
                if((state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)) enter(GameState::Playing);
                break;
            case UiAction::OpenKeyboard:enter(GameState::Keyboard);break;
            case UiAction::CloseKeyboard:enter(GameState::Options);break;
            case UiAction::OpenOptions:
                if(state==GameState::MainMenu||state==GameState::Paused) {
                    optionsParent=state;enter(GameState::Options);
                }
                break;
            case UiAction::CloseOptions:
                if(state==GameState::Options) closeOptions();
                break;
            case UiAction::Resume:
                if((state==GameState::Paused||state==GameState::BedwarsTeams)&&hasWorld) enter(GameState::Playing);
                break;
            case UiAction::SaveAndReturn:
                if(bw::Active()){bw::Stop();hasWorld=false;enter(GameState::MainMenu);break;}
                if(online){net::Disconnect();online=false;hasWorld=false;enter(GameState::MainMenu);break;}
                if((state==GameState::Paused||state==GameState::Dead)&&hasWorld) {
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
                if(bw::Active()){bw::Stop();hasWorld=false;}
                if(online){net::Disconnect();online=false;hasWorld=false;}
                enter(GameState::MainMenu);
                break;
            case UiAction::LoadSelected: {
                if(state!=GameState::WorldSelection) break;
                const std::string path=ui.SelectedWorldPath();
                PlayerState savedPlayer;
                if(LoadWorld(path,&savedPlayer)) {
                    ClearItems();activeSave=path;hasWorld=true;
                    if(!player.RestoreState(savedPlayer)) player.Respawn();
                    enter(Health<=0&&Mode==GameMode::Survival?GameState::Dead:GameState::Playing);
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
                ClearItems();Mode=NewWorldMode;
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
        if(online) {
            if(hasWorld&&state!=GameState::Playing)player.Update({},std::min(GetFrameTime(),.05f));
            net::Update(player,std::min(GetFrameTime(),.05f));
            if(net::Joined()&&state==GameState::Multiplayer){hasWorld=true;lastSelected=ui.selected=net::SelectedSlot();enter(GameState::Playing);}
            if(!net::Active()){net::Disconnect();online=false;hasWorld=false;enter(GameState::Multiplayer);}
            else if(hasWorld&&net::WindowOpen()&&state==GameState::Playing)enter(GameState::ServerInventory);
            else if(state==GameState::ServerInventory&&!net::WindowOpen())enter(GameState::Playing);
            if(online&&state==GameState::Dead&&Health>0)enter(GameState::Playing);
            if(online&&ui.selected!=lastSelected)net::SelectSlot(ui.selected);
            if(online)lastSelected=ui.selected=net::SelectedSlot();
        }
        if(bw::Active()){
            if(hasWorld&&state!=GameState::Playing&&state!=GameState::BedwarsMenu)player.Update({},std::min(GetFrameTime(),.05f));
            if(state!=GameState::Playing||!IsWindowFocused()||ui.chatOpen)bw::CancelBow();
            bw::HoldUse(ui.selected,state==GameState::Playing&&IsWindowFocused()&&!ui.chatOpen&&ActionDown(BindingAction::Use));
            bw::Update(player,std::min(GetFrameTime(),.05f),state==GameState::Playing,ui.selected);
            if(!bw::Active()){const auto reason=bw::Status();hasWorld=worldPrimed=false;renderer.CancelWorldLoad();enter(GameState::BedwarsMenu);ui.Notify(reason,15.f);}
            else if(bw::Ready()&&state==GameState::BedwarsMenu){hasWorld=worldPrimed=false;renderer.BeginWorldLoad();enter(GameState::WorldLoading);bw::ConnectionTrace("loading: beginning lighting; cursor remains free");}
            else if(bw::Ready()&&state==GameState::BedwarsTeams&&!bw::Lobby())enter(GameState::Playing);
            else if(bw::Ready()&&((state==GameState::BedwarsShop&&!bw::NearShop(player))||(state==GameState::BedwarsChest&&!bw::NearChest(player))))enter(GameState::Playing);
        }
        if(state==GameState::WorldLoading&&bw::Ready()){
            worldPrimed=renderer.PrepareWorld(player.Camera(),12.f);hasWorld=worldPrimed;
            ui.loadingProgress=renderer.WorldLoadProgress();ui.loadingText=renderer.WorldLoadStage();
        }
        if(bw::Ready()&&bw::Hosting()&&state==GameState::Playing&&!typingAtFrameStart&&!ui.chatOpen){if(ActionPressed(BindingAction::StartRound))bw::StartRound();if(ActionPressed(BindingAction::RestartRound))bw::Restart();}
        bool active=state==GameState::Playing&&IsWindowFocused()&&!transition;
        if(!active)placementRepeat.Reset();
        if(active) {
            PlayerInput input;
            input.look=ui.chatOpen?Vector2{}:GetMouseDelta();
            input.forward=ui.chatOpen?0:float(ActionDown(BindingAction::Forward))-float(ActionDown(BindingAction::Backward));
            input.side=ui.chatOpen?0:float(ActionDown(BindingAction::Right))-float(ActionDown(BindingAction::Left));
            input.jump=!ui.chatOpen&&ActionDown(BindingAction::Jump);input.swimUp=input.jump;
            input.swimDown=!ui.chatOpen&&ActionDown(BindingAction::Sneak);
            input.sprint=!ui.chatOpen&&ActionDown(BindingAction::Sprint);
            input.sprintPressed=!ui.chatOpen&&ActionPressed(BindingAction::Sprint);
            if(bw::Ready()&&!bw::CanControl()){input.forward=input.side=0;input.jump=input.swimUp=input.swimDown=input.sprint=input.sprintPressed=false;}

            float dt=std::min(GetFrameTime(),.05f);
            if(!online&&Mode==GameMode::Creative&&ActionPressed(BindingAction::SkipTime)&&!Settings.freezeTime) WorldTime=std::fmod(WorldTime+DayDuration*.25f,DayDuration);
            if(bw::Ready())bw::ApplyEffects(input);else if(!online)ApplyItemEffects(input);
            Block used=ui.SelectedBlock();input.usingItem=!ui.chatOpen&&ActionDown(BindingAction::Use)&&(Sword(used)||TimedUseItem(used)||(used==Bow&&(bw::Ready()?bw::BowDrawTime():game::BowDrawTime())>=0));
            player.Update(input,dt);if(!online&&!bw::Active()){UpdateWorld(dt);UpdateItems(player,dt);}
            if(!bw::Active()&&Mode==GameMode::Survival&&Health<=0) {active=false;enter(GameState::Dead);if(!online)SaveWorld(activeSave,player.CaptureState());}
            PlayerState streamed=player.CaptureState();
            const Vector3 previousPosition=streamed.position;bool shifted=false;
            bool streamedOk=(online||bw::Active()||state==GameState::Dead)?true:StreamWorld(streamed,activeSave,shifted);
            if(shifted) {Vector3 delta=Vector3Subtract(streamed.position,previousPosition);player.Translate(delta);TranslateItems(delta);}
            if(!streamedOk) {
                active=false;
                enter(GameState::Paused);
                ui.Notify("Falha ao carregar/salvar regioes. Verifique o disco antes de continuar.");
            }
        }
        if(!online&&state==GameState::FurnaceScreen&&!transition) UpdateFurnaces(std::min(GetFrameTime(),.05f));
        Hit hit;
        Camera3D camera{};
        if(hasWorld&&(state==GameState::WorldLoading||state==GameState::Playing||state==GameState::BedwarsTeams||state==GameState::BedwarsShop||state==GameState::BedwarsInventory||state==GameState::BedwarsChest||state==GameState::ServerInventory||state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)||state==GameState::Paused||state==GameState::Dead||
                     (state==GameState::Options&&optionsParent==GameState::Paused))) {
            camera=player.Camera();
            if(active&&!ui.chatOpen) {
                Vector3 direction=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
                bool targetLiquids=ActionDown(BindingAction::TargetLiquid);
                if(ActionPressed(BindingAction::Drop)){
                    bool all=IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
                    if(bw::Ready())bw::Drop(ui.selected,all,&player);
                    else if(online)net::Drop(all);
                    else DropItem(player,Hotbar[ui.selected],all);
                }
                const bool usePressed=ActionPressed(BindingAction::Use);
                const bool placeNow=placementRepeat.Update(usePressed,!ui.chatOpen&&ActionDown(BindingAction::Use),std::min(GetFrameTime(),.05f));
                if(bw::Ready()){
                    if(bw::Lobby()){
                        if(ui.SelectedBlock()==WhiteWool&&(ActionPressed(BindingAction::Attack)||ActionPressed(BindingAction::Use)))enter(GameState::BedwarsTeams);
                    }else{
                    hit=Trace(camera.position,direction);
                    if(hit.found){auto rayHit=GetRayCollisionBox({camera.position,direction},{{float(hit.x),float(hit.y),float(hit.z)},{hit.x+1.f,hit.y+1.f,hit.z+1.f}});if(rayHit.hit&&rayHit.distance>4.f)hit={};}
                    if(ActionPressed(BindingAction::Attack))bw::Attack(ui.selected,&player);
                    if(ActionDown(BindingAction::Attack)&&bw::CanMine(hit)){
                        if(!miningTarget.found||miningTarget.x!=hit.x||miningTarget.y!=hit.y||miningTarget.z!=hit.z||miningTool!=ui.SelectedBlock()){miningTarget=hit;miningTool=ui.SelectedBlock();ui.miningProgress=0;bw::Dig(hit,0,ui.selected);}
                        ui.miningProgress+=std::min(GetFrameTime(),.05f)/MiningTimeAt(hit.x,hit.y,hit.z,ui.SelectedBlock(),player.OnGround(),WaterContains(camera.position));
                        if(ui.miningProgress>=1){bw::Dig(hit,2,ui.selected);ui.miningProgress=0;miningTarget={};}
                    }else if(miningTarget.found){bw::Dig(miningTarget,1,ui.selected);miningTarget={};ui.miningProgress=0;}
                    bw::BowInput(ui.selected,ActionDown(BindingAction::Use),usePressed,ActionReleased(BindingAction::Use),std::min(GetFrameTime(),.05f),&player);
                    if(placeNow&&ui.SelectedBlock()!=Bow&&!Sword(ui.SelectedBlock())&&!TimedUseItem(ui.SelectedBlock())){
                        if(usePressed&&bw::NearChest(player)&&hit.found&&(At(hit.x,hit.y,hit.z)==Chest||(MapStateAt(hit.x,hit.y,hit.z).shape==MapShape::Cube&&MapStateAt(hit.x,hit.y,hit.z).block==MapDecoration&&MapStateAt(hit.x,hit.y,hit.z).material==Obsidian)))enter(GameState::BedwarsChest);
                        else if(usePressed&&bw::AimedShop(player))enter(GameState::BedwarsShop);
                        else if(hit.found&&Placeable(ui.SelectedBlock()))bw::Place(hit.x+hit.nx,hit.y+hit.ny,hit.z+hit.nz,ui.selected,&player);
                        else if(usePressed)bw::Use(ui.selected);
                    }
                    }
                }else if(online) {
                    hit=Trace(camera.position,direction);
                    if(ActionPressed(BindingAction::Attack))net::Attack(player);
                    if(ActionPressed(BindingAction::Use))net::Interact(player);
                    if(ActionDown(BindingAction::Attack)&&hit.found) {
                        if(!miningTarget.found||miningTarget.x!=hit.x||miningTarget.y!=hit.y||miningTarget.z!=hit.z){if(miningTarget.found)net::Dig(miningTarget,1);miningTarget=hit;ui.miningProgress=0;net::Dig(hit,0);}
                        ui.miningProgress+=std::min(GetFrameTime(),.05f)/MiningTimeAt(hit.x,hit.y,hit.z,ui.SelectedBlock(),player.OnGround(),WaterContains(camera.position));
                        if(Mode==GameMode::Creative||ui.miningProgress>=1){net::Dig(hit,2);ui.miningProgress=0;miningTarget={};}
                    }else if(miningTarget.found){net::Dig(miningTarget,1);miningTarget={};ui.miningProgress=0;}
                }else {
                if(singleUseSlot!=ui.selected){CancelItemUse();singleUseSlot=ui.selected;}
                bool usedItem=UseHeldItem(player,Hotbar[ui.selected],direction,{ActionPressed(BindingAction::Use),ActionDown(BindingAction::Use),ActionReleased(BindingAction::Use),std::min(GetFrameTime(),.05f)});
                hit=Trace(camera.position,direction,targetLiquids);
                if(hit.found) {
                    if(ActionPressed(BindingAction::ToggleLamp)&&(At(hit.x,hit.y,hit.z)==LampOn||At(hit.x,hit.y,hit.z)==LampOff))
                        Change(hit.x,hit.y,hit.z,At(hit.x,hit.y,hit.z)==LampOn?LampOff:LampOn);
                    if(Mode==GameMode::Survival&&ActionDown(BindingAction::Attack)) {
                        Block selected=ui.SelectedBlock();
                        if(!miningTarget.found||miningTarget.x!=hit.x||miningTarget.y!=hit.y||miningTarget.z!=hit.z||miningTool!=selected) {
                            miningTarget=hit;miningTool=selected;ui.miningProgress=0;
                        }
                        ui.miningProgress+=std::min(GetFrameTime(),.05f)/MiningTimeAt(hit.x,hit.y,hit.z,selected,player.OnGround(),WaterContains(camera.position));
                        if(ui.miningProgress>=1) {
                            if(!MineBlock(hit.x,hit.y,hit.z,Hotbar[ui.selected])) ui.Notify("Nao foi possivel recolher o bloco. Verifique o inventario.");
                            ui.miningProgress=0;
                        }
                    } else if(Mode==GameMode::Creative&&ActionPressed(BindingAction::Attack)) {
                        Block original=At(hit.x,hit.y,hit.z);
                        Change(hit.x,hit.y,hit.z,Air);
                        if(At(hit.x,hit.y,hit.z)==Air&&(original==IronOre||original==CoalOre||original==Stone||original==Wood)) {
                            ItemStack drop{original==CoalOre?Coal:(original==Stone?Cobblestone:original),1};
                            TransferStack(drop,Backpack.data(),Backpack.size());TransferStack(drop,Hotbar.data(),Hotbar.size());
                            if(drop.count) ui.Notify("Inventario cheio: material nao recolhido.");
                        }
                        if(At(hit.x,hit.y,hit.z)==Chest) ui.Notify("Inventario cheio. Esvazie o bau antes de quebra-lo.");
                    }
                    else if(!usedItem&&ActionPressed(BindingAction::Use)&&(At(hit.x,hit.y,hit.z)==Bed||At(hit.x,hit.y,hit.z)==BedHead)) {
                        if(Daylight()<.3f)SetClockMinute(720);
                        ui.Notify("Camas nesta regiao permitem renascer perto delas.");
                    }
                    else if(!usedItem&&ActionPressed(BindingAction::Use)&&
                            (At(hit.x,hit.y,hit.z)==CraftingTable||At(hit.x,hit.y,hit.z)==Furnace)&&!ActionDown(BindingAction::Sneak)) {
                        ui.OpenChest(hit.x,hit.y,hit.z);
                        enter(At(hit.x,hit.y,hit.z)==CraftingTable?GameState::CraftScreen:GameState::FurnaceScreen);
                    }
                    else if(!usedItem&&ActionPressed(BindingAction::Use)&&At(hit.x,hit.y,hit.z)==Chest&&!ActionDown(BindingAction::Sneak)) {
                        if(Solid(At(hit.x,hit.y+1,hit.z))) ui.Notify("O bloco sobre o bau impede a abertura.");
                        else {ui.OpenChest(hit.x,hit.y,hit.z);enter(GameState::ChestScreen);}
                    } else if(!usedItem&&placeNow) {
                        const bool replaceLiquid=Liquid(At(hit.x,hit.y,hit.z));
                        int x=hit.x+(replaceLiquid?0:hit.nx);
                        int y=hit.y+(replaceLiquid?0:hit.ny);
                        int z=hit.z+(replaceLiquid?0:hit.nz);
                        Block block=ui.SelectedBlock(),previous=At(x,y,z);
                        if(block!=Air&&Placeable(block)&&Inside(x,y,z)&&(previous==Air||Liquid(previous))&&(!Solid(block)||!player.Overlaps(x,y,z))) PlaceSurvivalBlock(x,y,z,Hotbar[ui.selected]);
                    }
                    hit=Trace(camera.position,direction,targetLiquids);
                }
                }
            }
        }
        if(!active||!hit.found||!ActionDown(BindingAction::Attack)) {ui.miningProgress=0;miningTarget={};}
        if(worldPrimed&&state==GameState::WorldLoading)bw::ConnectionTrace("loading: drawing first world frame");
        BeginDrawing();
        if(hasWorld&&(state==GameState::WorldLoading||state==GameState::Playing||state==GameState::BedwarsTeams||state==GameState::BedwarsShop||state==GameState::BedwarsInventory||state==GameState::BedwarsChest||state==GameState::ServerInventory||state==GameState::Inventory||(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen)||state==GameState::Paused||state==GameState::Dead||
                     (state==GameState::Options&&optionsParent==GameState::Paused))) renderer.Draw(camera,hit,ui.miningProgress);
        else ClearBackground({43,41,37,255});
        if(hasWorld&&bw::Ready())bw::DrawWorld(camera,renderer.Atlas());
        if(hasWorld&&state==GameState::Playing) {
            bool moving=active&&(ActionDown(BindingAction::Forward)||ActionDown(BindingAction::Left)||ActionDown(BindingAction::Backward)||ActionDown(BindingAction::Right));
            bool usingItem=active&&(ActionDown(BindingAction::Attack)||(ActionPressed(BindingAction::Use)&&!Sword(ui.SelectedBlock())&&ui.SelectedBlock()!=Bow&&!TimedUseItem(ui.SelectedBlock())));
            renderer.DrawFirstPerson(camera,ui.SelectedBlock(),moving,usingItem,std::min(GetFrameTime(),.05f),player.IsSprinting());
        }
        ui.Draw(renderer.Atlas(),state);
        if(bw::Ready()&&state==GameState::Playing)bw::DrawHud();
        EndDrawing();
        if(worldPrimed&&state==GameState::WorldLoading){bw::ConnectionTrace("loading: first frame complete; entering game and capturing cursor");worldPrimed=false;enter(GameState::Playing);}
    }
    bw::Stop();net::Disconnect();
    if(!SaveSettings()) TraceLog(LOG_WARNING,"Falha ao salvar settings.cfg.");
    if(hasWorld&&!activeSave.empty()&&!SaveWorld(activeSave,player.CaptureState())) {
        TraceLog(LOG_ERROR,"Falha ao salvar o mundo e a posicao ao fechar.");
        EnableCursor();return 1;
    }
    EnableCursor();
    return 0;
}
}
