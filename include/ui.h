#pragma once
#include <raylib.h>
#include <array>
#include <string>
#include "world.h"
#include "game.h"
namespace game {
enum class UiAction { None, OpenOptions, CloseOptions, NewWorld, Resume, SaveAndReturn, OpenWorlds, LoadSelected, DeleteSelected, BackToMenu, QuitApplication };
class UI {
public:
    std::array<Block,9> hotbar{{Grass,Dirt,Stone,Wood,Leaves,Sand,Water,Glass,Cactus}};
    int selected=0;
    UiAction Update(GameState state);
    void Draw(Texture2D atlas,GameState state) const;
    Block SelectedBlock() const {return hotbar[selected];}
    void ResetInteraction();
    void SetWorlds(std::vector<WorldSave> saves);
    std::string SelectedWorldPath() const {return selectedWorldPath;}
    void Notify(const std::string& text);
private:
    std::vector<WorldSave> worlds;
    int worldScroll=0;
    int inventoryPage=0;
    int draggingSlider=0;
    std::string selectedWorldPath;
    Block held=Air;
    std::string message;
};
}
