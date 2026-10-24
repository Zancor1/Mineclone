#pragma once
#include "map_blocks.h"
#include <array>
#include <cstddef>
#include <vector>
namespace game::bw {
constexpr int FirstExternalArena=5,ExternalArenaCount=16;
struct ExternalTeam {
    Vector3 spawn;
    Vector3 beds[2],vendors[2];
    Vector3 forge,chest;
};
struct ExternalArena {
    const char* name;
    unsigned cells;
    const unsigned char* compressed;
    size_t bytes;
    int lobbyY;
    ExternalTeam teams[8];
    std::vector<Vector3> diamonds,emeralds;
};
const ExternalArena& ExternalArenaAt(int map);
const std::vector<unsigned char>& ExternalCells(int map);
}
