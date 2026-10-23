#pragma once
namespace game {
enum class GameState { MainMenu, WorldSelection, Playing, Inventory, Paused, Options };
int RunGame();
}
