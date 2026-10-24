#pragma once
#include <raylib.h>
#include <array>
#include <string>
#include "world.h"
#include "chest.h"
#include "game.h"
#include "bedwars_maps.h"
namespace game {
enum class UiAction { None, OpenKeyboard, CloseKeyboard, SearchBedwars, OpenBedwars, ChooseMode, ChooseRush, ChooseMap, HostBedwars, JoinBedwars, OpenMultiplayer, ConnectServer, DisconnectServer, Respawn, CloseChest, OpenOptions, CloseOptions, NewWorld, Resume, SaveAndReturn, OpenWorlds, LoadSelected, DeleteSelected, BackToMenu, QuitApplication };
class UI {
public:
    int selected=0;
    int rushGap=16,mapChoice=0,teamCapacity=1;
    ~UI();
    float miningProgress=0;
    UiAction Update(GameState state);
    void Draw(Texture2D atlas,GameState state) const;
    void DrawPlayerList() const;
    void OpenChest(int x,int y,int z);
    Block SelectedBlock() const {return Hotbar[selected].count?Hotbar[selected].block:Air;}
    void ResetInteraction();
    bool CapturingBinding() const {return capturingBinding>=0;}
    std::string bedwarsAddress="127.0.0.1:25566";
    std::string serverAddress="jogar.mush.com.br",playerName="Player",chatText;
    bool chatOpen=false;
    void SetWorlds(std::vector<WorldSave> saves);
    std::string SelectedWorldPath() const {return selectedWorldPath;}
    void Notify(const std::string& text);
private:
    std::vector<WorldSave> worlds;
    int worldScroll=0;
    int lanScroll=0;
    int bindingScroll=0,capturingBinding=-1;
    std::string bindingMessage;
    std::array<RenderTexture2D,bw::MapCount> mapPreviews{};
    int previewGap=0;
    int inventoryPage=0;
    int draggingSlider=0,serverField=0;
    std::string selectedWorldPath;
    int chestX=0,chestY=0,chestZ=0;
    double messageUntil=0;
    bool catalogOpen=false;
    std::string message;
};
}
