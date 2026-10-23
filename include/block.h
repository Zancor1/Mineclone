#pragma once
namespace game {
// Stable IDs are also used by the versioned world.dat format.
enum Block : unsigned char {
    Air, Grass, Dirt, Stone, Wood, Leaves, Sand, Water, Glass, Cactus,
    Bricks, CoalOre, WhiteWool, Planks, Cobblestone, StoneBricks, CarvedStone, SpruceLeaves, AcaciaPlanks,
    DarkOakPlanks, LampOn, LampOff, RedWool, GreenWool, BlueWool, YellowWool,
    Gravel, Clay, Snow, Slime, Lava, Bedrock, TallGrass, DeadBush, Kelp,
    Allium, Orchid, RedTulip, PinkTulip, Rose, Dandelion,
    OrangeWool, MagentaWool, LightBlueWool, LimeWool, PinkWool, GrayWool,
    LightGrayWool, CyanWool, PurpleWool, BrownWool, BlackWool, BlockCount
};
struct BlockProperties {
    const char* name;
    bool solid;
    bool transparent;
};
inline constexpr BlockProperties Blocks[BlockCount]={
    {"Ar",false,true}, {"Grama",true,false}, {"Terra",true,false},
    {"Pedra",true,false}, {"Tronco",true,false}, {"Folhas",true,true},
    {"Areia",true,false}, {"Agua",false,true}, {"Vidro",true,true},
    {"Cacto",true,false}, {"Tijolos",true,false}, {"Carvao",true,false},
    {"La branca",true,false}, {"Tabuas",true,false},
    {"Pedregulho",true,false},{"Tijolo pedra",true,false},{"Pedra talhada",true,false},
    {"Folha pinho",true,true},{"Tabua acacia",true,false},{"Tabua escura",true,false},
    {"Lanterna",true,false},{"Lanterna off",true,false},{"La vermelha",true,false},
    {"La verde",true,false},{"La azul",true,false},{"La amarela",true,false},
    {"Cascalho",true,false},{"Argila",true,false},{"Neve",true,false},{"Slime",true,true},
    {"Lava",false,true},{"Rocha matriz",true,false},{"Capim",false,true},
    {"Arbusto seco",false,true},{"Alga",false,true},{"Allium",false,true},
    {"Orquidea",false,true},{"Tulipa rubra",false,true},{"Tulipa rosa",false,true},
    {"Rosa",false,true},{"Dente leao",false,true},
    {"La laranja",true,false},{"La magenta",true,false},{"La azul cl.",true,false},
    {"La lima",true,false},{"La rosa",true,false},{"La cinza",true,false},
    {"La cinza cl.",true,false},{"La ciano",true,false},{"La roxa",true,false},
    {"La marrom",true,false},{"La preta",true,false}
};
inline constexpr auto Names=[] {
    struct NamesArray { const char* values[BlockCount]{};
        constexpr const char* operator[](int i) const { return values[i]; } } names;
    for(int i=0;i<BlockCount;++i) names.values[i]=Blocks[i].name;
    return names;
}();
constexpr bool Liquid(Block b) {return b==Water||b==Lava;}
constexpr bool Plant(Block b) {return b>=TallGrass&&b<=Dandelion;}
constexpr bool Wool(Block b) {return b==WhiteWool||(b>=RedWool&&b<=YellowWool)||(b>=OrangeWool&&b<=BlackWool);}
constexpr bool LightPassable(Block b) {return b==Air||b==Water||b==Glass||b==Slime||Plant(b);}
constexpr unsigned char Emission(Block b) {return b==LampOn?24:(b==Lava?18:0);}
constexpr bool ValidBlock(Block b) {return b<BlockCount;}
constexpr bool Solid(Block b) {return ValidBlock(b)&&Blocks[b].solid;}
constexpr bool Transparent(Block b) {return ValidBlock(b)&&Blocks[b].transparent;}
static_assert(!Plant(OrangeWool)&&Wool(BlackWool)&&Solid(BlackWool)&&!Transparent(BlackWool),"Wool renders as opaque full cubes");
static_assert(!Solid(Water),"Water must never collide with the player");
static_assert(Solid(Glass)&&Transparent(Glass),"Glass is transparent but solid");
}
