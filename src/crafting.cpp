#include "crafting.h"
#include "world.h"
#include "survival.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>
namespace game {
std::array<ItemStack,4> PersonalCrafting{};
std::array<ItemStack,9> WorkbenchCrafting{};
namespace {
struct Position {int32_t x,y,z;bool operator<(const Position& p) const {return std::tie(x,y,z)<std::tie(p.x,p.y,p.z);}};
std::map<Position,FurnaceState> furnaces;
bool Plank(Block b) {return b==Planks||b==AcaciaPlanks||b==DarkOakPlanks||(b>=SpruceWoodPlanks&&b<=JungleWoodPlanks);}
bool Log(Block b) {return b==Wood||(b>=SpruceWood&&b<=DarkOakWood)||b==AcaciaWood0||b==DarkOakWood1;}
Block PlanksFor(Block b) {
    if(b==SpruceWood)return SpruceWoodPlanks;
    if(b==BirchWood)return BirchWoodPlanks;
    if(b==JungleWood)return JungleWoodPlanks;
    if(b==AcaciaWood||b==AcaciaWood0)return AcaciaPlanks;
    if(b==DarkOakWood||b==DarkOakWood1)return DarkOakPlanks;
    return Planks;
}
bool Ingredient(Block block,char code) {
    return code=='P'?Plank(block):code=='L'?Log(block):code=='C'?block==Cobblestone:
        code=='T'?block==String:code=='F'?block==Flint:code=='H'?block==Feather:code=='A'?block==Apple:code=='E'?block==Leather:code=='U'?block==Gunpowder:code=='N'?(block==Sand||block==RedSand):code=='W'?Wool(block):code=='S'?block==Stick:code=='I'?block==IronIngot:code=='D'?block==Diamond:code=='G'?block==GoldIngot:block==Air;
}
ItemStack Recipe(const ItemStack* grid,int width) {
    int left=width,right=-1,top=width,bottom=-1;
    for(int y=0;y<width;++y) for(int x=0;x<width;++x) if(grid[y*width+x].count) {
        left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
    }
    if(right<0) return {};
    auto matches=[&](const char* shape,int w,int h) {
        if(right-left+1!=w||bottom-top+1!=h) return false;
        for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
            const auto& slot=grid[(top+y)*width+left+x];
            if(!Ingredient(slot.count?slot.block:Air,shape[y*w+x])) return false;
        }
        return true;
    };
    if(matches("L",1,1)) return {PlanksFor(grid[top*width+left].block),4};
    if(matches("PP",1,2)) return {Stick,4};
    if(matches("PPPP",2,2)) return {CraftingTable,1};
    if(matches("CCCC CCCC",3,3)) return {Furnace,1};
    if(matches("I I I ",3,2))return {Bucket,1};
    if(matches("GGGGAGGGG",3,3))return {GoldenApple,1};
    if(matches(" STS T ST",3,3))return {Bow,1};
    if(matches("FSH",1,3))return {Arrow,4};
    if(matches(" IF ",2,2)||matches("I  F",2,2))return {FlintAndSteel,1};
    if(matches("UNUNUNUNU",3,3))return {Tnt,1};
    if(matches("S SSSSS S",3,3))return {Ladder,3};
    if(matches("WWWPPP",3,2))return {Bed,1};
    const char armorMaterials[]={'E','I','G','D'};
    const Block armorBase[]={LeatherHelmet,IronHelmet,GoldenHelmet,DiamondHelmet};
    for(int m=0;m<4;++m) {
        const char* patterns[]={"MMMM M","M MMMMMMM","MMMM MM M","M MM M"};
        for(int piece=0;piece<4;++piece) {
            std::string pattern=patterns[piece];for(char& c:pattern)if(c=='M')c=armorMaterials[m];
            if(matches(pattern.c_str(),3,piece==0||piece==3?2:3))return {static_cast<Block>(int(armorBase[m])+piece),1};
        }
    }
    const Block tools[5][5]={{WoodenSword,WoodenPickaxe,WoodenAxe,WoodenShovel,WoodenHoe},
        {StoneSword,StonePickaxe,StoneAxe,StoneShovel,StoneHoe},{IronSword,IronPickaxe,IronAxe,IronShovel,IronHoe},
        {DiamondSword,DiamondPickaxe,DiamondAxe,DiamondShovel,DiamondHoe},{GoldenSword,GoldenPickaxe,GoldenAxe,GoldenShovel,GoldenHoe}};
    const char materials[]={'P','C','I','D','G'};
    for(int m=0;m<5;++m) {
        auto shape=[&](const char* pattern,int w,int h) {
            std::string text=pattern;for(char& c:text)if(c=='M') c=materials[m];return matches(text.c_str(),w,h);
        };
        if(shape("MMS",1,3)) return {tools[m][0],1};
        if(shape("MMM S  S ",3,3)) return {tools[m][1],1};
        if(shape("MMMS S",2,3)||shape("MMSMS ",2,3)) return {tools[m][2],1};
        if(shape("MSS",1,3)) return {tools[m][3],1};
        if(shape("MM S S",2,3)||shape("MMS S ",2,3)) return {tools[m][4],1};
    }
    const Block dyes[]={InkSac,RoseRed,CactusGreen,CocoaBeans,LapisLazuli,PurpleDye,CyanDye,LightGrayDye,
        GrayDye,PinkDye,LimeDye,DandelionYellow,LightBlueDye,MagentaDye,OrangeDye,BoneMeal};
    const Block wools[]={BlackWool,RedWool,GreenWool,BrownWool,BlueWool,PurpleWool,CyanWool,LightGrayWool,
        GrayWool,PinkWool,LimeWool,YellowWool,LightBlueWool,MagentaWool,OrangeWool,WhiteWool};
    const Block glass[]={BlackStainedGlass,RedStainedGlass,GreenStainedGlass,BrownStainedGlass,BlueStainedGlass,
        PurpleStainedGlass,CyanStainedGlass,LightGrayStainedGlass,GrayStainedGlass,PinkStainedGlass,LimeStainedGlass,
        YellowStainedGlass,LightBlueStainedGlass,MagentaStainedGlass,OrangeStainedGlass,WhiteStainedGlass};
    const Block clay[]={BlackStainedClay,RedStainedClay,GreenStainedClay,BrownStainedClay,BlueStainedClay,
        PurpleStainedClay,CyanStainedClay,LightGrayStainedClay,GrayStainedClay,PinkStainedClay,LimeStainedClay,
        YellowStainedClay,LightBlueStainedClay,MagentaStainedClay,OrangeStainedClay,WhiteStainedClay};
    for(int dye=0;dye<16;++dye) {
        int total=0;bool wool=false,color=false,valid=true;
        for(int i=0;i<width*width;++i)if(grid[i].count) {
            ++total;if(Wool(grid[i].block)) {if(wool)valid=false;wool=true;}
            else if(grid[i].block==dyes[dye])color=true;else valid=false;
        }
        if(valid&&total==2&&wool&&color) return {wools[dye],1};
        if(width==3&&grid[4].count&&grid[4].block==dyes[dye]) for(Block base:{Glass,HardenedClay}) {
            bool ring=true;for(int i=0;i<9;++i)if(i!=4)ring&=grid[i].count&&grid[i].block==base;
            if(ring)return {base==Glass?glass[dye]:clay[dye],8};
        }
    }
    struct Square {Block ingredient,result;int count;};
    const Square squares[]={{Sand,Sandstone,1},{RedSand,RedSandstone,1},{Granite,PolishedGranite,4},
        {Diorite,PolishedDiorite,4},{Andesite,PolishedAndesite,4},{Quartz,BlockOfQuartz,1},
        {ClayBall,Clay,1},{Brick,Bricks,1},{GlowstoneDust,Glowstone,1},{NetherBrickItem,NetherBrick,1}};
    for(auto square:squares) if(right-left==1&&bottom-top==1) {
        bool full=true;for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x)
            full&=grid[y*width+x].count&&grid[y*width+x].block==square.ingredient;
        if(full)return {square.result,static_cast<unsigned char>(square.count)};
    }
    struct Compression {Block item,block;};
    const Compression conversions[]={{IronIngot,BlockOfIron},{GoldIngot,BlockOfGold},{Diamond,BlockOfDiamond},
        {Emerald,BlockOfEmerald},{Coal,BlockOfCoal},{LapisLazuli,LapisLazuliBlock}};
    for(auto conversion:conversions) {
        bool full=right-left==2&&bottom-top==2;
        for(int i=0;i<width*width&&full;++i) full=grid[i].count&&grid[i].block==conversion.item;
        if(full) return {conversion.block,1};
        if(right==left&&bottom==top&&grid[top*width+left].block==conversion.block) return {conversion.item,9};
    }
    return {};
}
Block SmeltResult(Block input) {
    switch(input) {
        case IronOre:return IronIngot;case GoldOre:return GoldIngot;
        case Sand:case RedSand:return Glass;case Cobblestone:return Stone;
        case Clay:return HardenedClay;case ClayBall:return Brick;case Netherrack:return NetherBrickItem;
        default:return Air;
    }
}
void Consume(ItemStack& slot,int amount=1) {slot.count-=amount;if(!slot.count) slot={};}
void PutWord(std::vector<unsigned char>& out,uint32_t value) {for(int i=0;i<4;++i) out.push_back(static_cast<unsigned char>(value>>(8*i)));}
}
bool FurnaceAccepts(Block input) {return SmeltResult(input)!=Air;}
float FuelDuration(Block fuel) {return fuel==BlockOfCoal?800.f:(fuel==Coal?80.f:((Plank(fuel)||Log(fuel))?15.f:(fuel==Stick?5.f:0.f)));}
ItemStack CraftResult(const ItemStack* grid,int width) {return width==2||width==3?Recipe(grid,width):ItemStack{};}
bool TakeCraft(ItemStack* grid,int width) {
    ItemStack result=CraftResult(grid,width);
    if(!result.count||(CursorStack.count&&CursorStack.block!=result.block)||
       CursorStack.count+result.count>StackCapacity(result.block)) return false;
    CursorStack.block=result.block;CursorStack.count+=result.count;
    for(int i=0;i<width*width;++i) if(grid[i].count) Consume(grid[i]);
    return true;
}
void ClearCrafting() {PersonalCrafting={};WorkbenchCrafting={};furnaces.clear();}
FurnaceState* FurnaceAt(int x,int y,int z) {
    if(At(x,y,z)!=Furnace) return nullptr;
    Position p{RegionOriginX+x,y,RegionOriginZ+z};
    if(furnaces.size()>=65536&&furnaces.find(p)==furnaces.end()) return nullptr;
    return &furnaces[p];
}
bool RemoveFurnace(int x,int y,int z) {
    auto it=furnaces.find({RegionOriginX+x,y,RegionOriginZ+z});if(it==furnaces.end()) return true;
    auto bag=Backpack;auto bar=Hotbar;
    for(ItemStack slot:it->second.slots) {
        TransferStack(slot,bag.data(),bag.size());TransferStack(slot,bar.data(),bar.size());
        if(slot.count) return false;
    }
    Backpack=bag;Hotbar=bar;furnaces.erase(it);return true;
}
void UpdateFurnaces(float dt) {
    if(!std::isfinite(dt)||dt<=0) return;
    dt=std::min(dt,.25f);
    for(auto& entry:furnaces) {
        int x=entry.first.x-RegionOriginX,z=entry.first.z-RegionOriginZ;
        if(!Inside(x,entry.first.y,z)||At(x,entry.first.y,z)!=Furnace) continue;
        auto& f=entry.second;auto& input=f.slots[0];auto& fuel=f.slots[1];auto& output=f.slots[2];
        float active=std::min(f.fuel,dt);f.fuel=std::max(0.f,f.fuel-dt);
        Block result=SmeltResult(input.block);
        bool canSmelt=result!=Air&&input.count&&(!output.count||output.block==result)&&output.count<64;
        if(!canSmelt) {f.progress=0;continue;}
        if(active<dt&&fuel.count) {
            float duration=FuelDuration(fuel.block);
            if(duration) {
                Consume(fuel);float extra=std::min(duration,dt-active);
                f.fuel+=duration-extra;active+=extra;
            }
        }
        if(active>0) {
            f.progress+=active;
            if(f.progress>=10.f) {f.progress-=10.f;Consume(input);output.block=result;++output.count;}
        }
    }
}
std::vector<unsigned char> SerializeCrafting() {
    std::vector<unsigned char> out;
    auto stack=[&](ItemStack s) {out.push_back(s.block);out.push_back(s.count);out.push_back(static_cast<unsigned char>(s.damage));out.push_back(static_cast<unsigned char>(s.damage>>8));};
    for(auto s:PersonalCrafting) stack(s);
    for(auto s:WorkbenchCrafting) stack(s);
    PutWord(out,uint32_t(furnaces.size()));
    for(const auto& entry:furnaces) {
        PutWord(out,uint32_t(entry.first.x));PutWord(out,uint32_t(entry.first.y));PutWord(out,uint32_t(entry.first.z));
        for(auto slot:entry.second.slots) stack(slot);
        uint32_t bits;std::memcpy(&bits,&entry.second.fuel,4);PutWord(out,bits);
        std::memcpy(&bits,&entry.second.progress,4);PutWord(out,bits);
    }
    auto survival=SerializeSurvival();out.insert(out.end(),survival.begin(),survival.end());
    return out;
}
bool DeserializeCrafting(const std::vector<unsigned char>& data,bool apply,bool wide) {
    size_t prefix=wide?56:30,record=wide?32:26;
    if(data.size()<prefix+(wide?13:0)) return false;
    size_t offset=0;
    auto stack=[&](ItemStack& s) {s.block=static_cast<Block>(data[offset++]);s.count=data[offset++];if(wide) {s.damage=data[offset]|(unsigned(data[offset+1])<<8);offset+=2;}return ValidBlock(s.block)&&(ToolDurability(s.block)>0?s.damage<ToolDurability(s.block):s.damage==0)&&s.count<=StackCapacity(s.block)&&((s.block==Air)==(s.count==0));};
    auto word=[&]() {uint32_t value=0;for(int i=0;i<4;++i) value|=uint32_t(data[offset++])<<(8*i);return value;};
    decltype(PersonalCrafting) personal;decltype(WorkbenchCrafting) workbench;
    for(auto& s:personal) if(!stack(s)) return false;
    for(auto& s:workbench) if(!stack(s)) return false;
    uint32_t count=word();size_t extra=data.size()-(prefix+size_t(count)*record);
    if(count>65536||(wide?(extra!=13&&extra!=41):extra!=0)) return false;
    std::map<Position,FurnaceState> loaded;
    for(uint32_t i=0;i<count;++i) {
        Position p{int32_t(word()),int32_t(word()),int32_t(word())};FurnaceState f;
        if(p.y<0||p.y>=Height||std::abs(int64_t(p.x))>100000128||std::abs(int64_t(p.z))>100000128) return false;
        for(auto& s:f.slots) if(!stack(s)) return false;
        uint32_t bits=word();std::memcpy(&f.fuel,&bits,4);bits=word();std::memcpy(&f.progress,&bits,4);
        if(!std::isfinite(f.fuel)||f.fuel<0||f.fuel>800||!std::isfinite(f.progress)||f.progress<0||f.progress>=10||!loaded.emplace(p,f).second) return false;
    }
    std::vector<unsigned char> survival;
    if(wide) {survival.assign(data.end()-extra,data.end());if(!DeserializeSurvival(survival,false)) return false;}
    if(apply) {if(wide) DeserializeSurvival(survival,true);else {Mode=GameMode::Creative;ResetVitals();}
        PersonalCrafting=personal;WorkbenchCrafting=workbench;furnaces=std::move(loaded);}
    return true;
}
}
