#pragma once
#include <raylib.h>
#include <array>
#include <string>
#include "world.h"
#include "chest.h"
#include "game.h"
#include "bedwars_maps.h"
namespace game {
enum class UiAction { None, OpenMinigames, OpenKeyboard, CloseKeyboard, SearchBedwars, OpenBedwars, ChooseMode, ChooseRush, ChooseMap, HostBedwars, JoinBedwars, OpenMultiplayer, ConnectServer, DisconnectServer, Respawn, CloseChest, OpenOptions, CloseOptions, NewWorld, Resume, SaveAndReturn, OpenWorlds, LoadSelected, DeleteSelected, BackToMenu, QuitApplication, OpenTester, DropCursor, DropCursorSingle };
class UI {
public:
    int selected=0;
    int rushGap=16,mapChoice=0,teamCapacity=1;
    bool standardLayout=true;
    ~UI();
    float miningProgress=0,loadingProgress=0;
    std::string loadingText;
    UiAction Update(GameState state);
    void Draw(Texture2D atlas,GameState state) const;
    void DrawPlayerList() const;
    void OpenChest(int x,int y,int z);
    Block SelectedBlock() const {return Hotbar[selected].count?Hotbar[selected].block:Air;}
    void ResetInteraction();
    bool CapturingBinding() const {return capturingBinding>=0;}
    std::string roomName="BedWars LAN";
    std::string bedwarsAddress="127.0.0.1:25566";
    std::string serverAddress="jogar.mush.com.br",playerName="Player",chatText;
    bool chatOpen=false;
    void SetWorlds(std::vector<WorldSave> saves);
    std::string SelectedWorldPath() const {return selectedWorldPath;}
    void Notify(const std::string& text,float seconds=4.f);
private:
    std::vector<WorldSave> worlds;
    int worldScroll=0;
    int lanScroll=0,rosterScroll=0,browserField=-1;
    uint64_t selectedRoom=0;
    std::string roomSearch,mapSearch,pingSearch;
    bool onlyOpenRooms=false;
    int bindingScroll=0,capturingBinding=-1;
    std::string bindingMessage;
    std::array<RenderTexture2D,bw::MapCount> mapPreviews{};
    RenderTexture2D skywarsPreview{};
    int previewGap=0,mapPage=0;
    int inventoryPage=0;
    int draggingSlider=0,serverField=0;
    std::string selectedWorldPath;
    int chestX=0,chestY=0,chestZ=0;
    double messageUntil=0;
    bool catalogOpen=false;
    std::string message;
};
}
