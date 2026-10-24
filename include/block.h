#pragma once
namespace game {
// Stable IDs are also used by the versioned world.dat format.
enum Block : unsigned short {
    Air, Grass, Dirt, Stone, Wood, Leaves, Sand, Water, Glass, Cactus,
    Bricks, CoalOre, WhiteWool, Planks, Cobblestone, StoneBricks, CarvedStone, SpruceLeaves, AcaciaPlanks,
    DarkOakPlanks, LampOn, LampOff, RedWool, GreenWool, BlueWool, YellowWool,
    Gravel, Clay, Snow, Slime, Lava, Bedrock, TallGrass, DeadBush, Kelp,
    Allium, Orchid, RedTulip, PinkTulip, Rose, Dandelion,
    OrangeWool, MagentaWool, LightBlueWool, LimeWool, PinkWool, GrayWool,
    LightGrayWool, CyanWool, PurpleWool, BrownWool, BlackWool, Chest, CraftingTable, IronOre, Furnace, Stick, IronIngot, Coal, WoodenSword, StoneSword, IronSword, WoodenPickaxe, StonePickaxe, IronPickaxe,
#include "content18_ids.inc"
#include "minigame_ids.inc"
    SpeedPotion, JumpPotion, InvisibilityPotion,
    MapSlab, MapStairs, MapFence, IronBars, MapDecoration, MapCarpet,
    BlockCount
};
static_assert(BlockCount<=256,"Block IDs must fit the byte-based world/save format");
struct BlockProperties {
    const char* name;
    bool solid;
    bool transparent;
    bool placeable=true;
    int stack=64,durability=0,toolKind=0,tier=0,material=0,light=0;
    unsigned int color=0;
    int alpha=255;
    float hardness=.5f;
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
    {"La marrom",true,false},{"La preta",true,false},{ "Bau",true,false},{"Mesa craft",true,false},{"Minerio ferro",true,false},{"Fornalha",true,false},
    {"Graveto",false,true},{"Lingote ferro",false,true},{"Carvao item",false,true},
    {"Espada madeira",false,true},{"Espada pedra",false,true},{"Espada ferro",false,true},{"Picareta madeira",false,true},{"Picareta pedra",false,true},{"Picareta ferro",false,true},
#include "content18_properties.inc"
#include "minigame_properties.inc"
    {"Pocao velocidade",false,true,false,1,0,0,0,0,0,0x59bfe8,255,.1f},
    {"Pocao salto",false,true,false,1,0,0,0,0,0,0x64d65a,255,.1f},
    {"Pocao invisibilidade",false,true,false,1,0,0,0,0,0,0xce91dc,255,.1f},
    {"Laje de pedra",true,true,true,64,0,0,0,4,0,0x999b9d,255,1.5f},
    {"Escada de pedra",true,true,true,64,0,0,0,4,0,0x999b9d,255,1.5f},
    {"Cerca de madeira",true,true,true,64,0,0,0,3,0,0x987346,255,1.5f},
    {"Grades de ferro",true,true,true,64,0,0,0,4,0,0xa9adb1,255,2.f},
    {"Tocha",false,true,true,64,0,0,0,3,0,0xf5b547,255,.1f},
    {"Carpete",true,true,true,64,0,0,0,5,0,0xeeeeee,255,.1f},

};
inline constexpr auto Names=[] {
    struct NamesArray { const char* values[BlockCount]{};
        constexpr const char* operator[](int i) const { return values[i]; } } names;
    for(int i=0;i<BlockCount;++i) names.values[i]=Blocks[i].name;
    return names;
}();
constexpr bool Extended(Block b) {return b>IronPickaxe&&b<BlockCount;}
constexpr bool Sword(Block b) {return (b>=WoodenSword&&b<=IronSword)||(Extended(b)&&Blocks[b].toolKind==1);}
constexpr bool Pickaxe(Block b) {return (b>=WoodenPickaxe&&b<=IronPickaxe)||(Extended(b)&&Blocks[b].toolKind==2);}
constexpr bool Armor(Block b) {return b>=LeatherHelmet&&b<=DiamondBoots;}
constexpr int ArmorSlot(Block b) {return Armor(b)?(int(b)-int(LeatherHelmet))%4:-1;}
constexpr int ArmorPoints(Block b) {
    constexpr int points[5][4]={{1,3,2,1},{2,5,4,1},{2,6,5,2},{2,5,3,1},{3,8,6,3}};
    return Armor(b)?points[(int(b)-int(LeatherHelmet))/4][ArmorSlot(b)]:0;
}
constexpr float CollisionHeight(Block b) {if(b==MapSlab)return .5f;if(b==MapCarpet)return .0625f;return b==Bed||b==BedHead?.5625f:1.f;}
constexpr bool Tool(Block b) {return Sword(b)||Pickaxe(b)||(Extended(b)&&Blocks[b].toolKind!=0);}
constexpr int ToolDurability(Block b) {return Extended(b)?Blocks[b].durability:(b==WoodenPickaxe||b==WoodenSword?59:(b==StonePickaxe||b==StoneSword?131:(Tool(b)?250:0)));}
constexpr int StackCapacity(Block b) {return Extended(b)?Blocks[b].stack:(Tool(b)?1:64);}
constexpr bool Placeable(Block b) {return b<BlockCount&&(b<Stick||(Extended(b)&&Blocks[b].placeable));}
constexpr bool Liquid(Block b) {return b==Water||b==Lava;}
constexpr bool Plant(Block b) {return b>=TallGrass&&b<=Dandelion;}
constexpr bool Wool(Block b) {return b==WhiteWool||(b>=RedWool&&b<=YellowWool)||(b>=OrangeWool&&b<=BlackWool);}
constexpr bool LightPassable(Block b) {return b==Air||b==Water||b==Glass||b==Slime||Plant(b)||(Extended(b)&&Blocks[b].transparent);}
constexpr unsigned char Emission(Block b) {return b==LampOn?24:(b==Lava?18:(Extended(b)?Blocks[b].light:0));}
constexpr bool ValidBlock(Block b) {return b<BlockCount;}
constexpr bool Solid(Block b) {return ValidBlock(b)&&Blocks[b].solid;}
constexpr bool Transparent(Block b) {return ValidBlock(b)&&Blocks[b].transparent;}
static_assert(!Plant(OrangeWool)&&Wool(BlackWool)&&Solid(BlackWool)&&!Transparent(BlackWool),"Wool renders as opaque full cubes");
static_assert(!Solid(Water),"Water must never collide with the player");
static_assert(Solid(Glass)&&Transparent(Glass),"Glass is transparent but solid");
}
