#include "external_arenas.h"
#include <stdexcept>
#include <zlib.h>
namespace game::bw {
namespace {
#include "external_maps.inc"
static_assert(std::size(ExternalArenas)==ExternalArenaCount);
std::array<std::vector<unsigned char>,ExternalArenaCount> decoded;
}
const ExternalArena& ExternalArenaAt(int map){
    if(map<FirstExternalArena||map>=FirstExternalArena+ExternalArenaCount)throw std::out_of_range("Mapa externo invalido");
    return ExternalArenas[map-FirstExternalArena];
}
const std::vector<unsigned char>& ExternalCells(int map){
    const auto& arena=ExternalArenaAt(map);auto& cells=decoded[map-FirstExternalArena];
    if(cells.empty()){
        std::vector<unsigned char> data(size_t(arena.cells)*7);uLongf bytes=data.size();
        if(uncompress(data.data(),&bytes,arena.compressed,arena.bytes)!=Z_OK||bytes!=data.size())throw std::runtime_error("Dados do mapa externo corrompidos");
        cells=std::move(data);
    }
    return cells;
}
}
