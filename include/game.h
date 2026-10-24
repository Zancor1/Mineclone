#pragma once
namespace game {
enum class GameState { MainMenu, Minigames, Keyboard, BedwarsMode, BedwarsTeams, BedwarsMaps, BedwarsDiscovery, BedwarsRush, BedwarsMenu, BedwarsShop, BedwarsInventory, BedwarsChest, Multiplayer, ServerInventory, WorldSelection, Playing, Inventory, Paused, Options, ChestScreen, CraftScreen, FurnaceScreen, Dead };
int RunGame();
}
