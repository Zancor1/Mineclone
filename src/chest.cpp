#include "chest.h"
#include "world.h"
#include "crafting.h"
#include "survival.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <tuple>
namespace game {
ChestInventory Backpack{};
ItemStack CursorStack{};
namespace {
std::array<ItemStack,9> DefaultHotbar() {
    return {{{Grass,64},{Dirt,64},{Stone,64},{Wood,64},{Leaves,64},{Sand,64},{Water,64},{Glass,64},{Cactus,64}}};
}
}
std::array<ItemStack,9> Hotbar=DefaultHotbar();
namespace {
struct Position {
    int32_t x,y,z;
    bool operator<(const Position& p) const {return std::tie(x,y,z)<std::tie(p.x,p.y,p.z);}
};
std::map<Position,ChestInventory> chests;
Position Global(int x,int y,int z) {return {RegionOriginX+x,y,RegionOriginZ+z};}

}
ChestInventory* ChestAt(int x,int y,int z) {
    if(At(x,y,z)!=Chest) return nullptr;
    auto key=Global(x,y,z);
    if(chests.size()>=65536&&chests.find(key)==chests.end()) return nullptr;
    return &chests[key];
}
bool RemoveChest(int x,int y,int z) {
    auto it=chests.find(Global(x,y,z));if(it==chests.end()) return true;
    ChestInventory bag=Backpack;auto bar=Hotbar;
    for(ItemStack stack:it->second) if(stack.count) {
        TransferStack(stack,bag.data(),bag.size());TransferStack(stack,bar.data(),bar.size());
        if(stack.count) return false;
    }
    Backpack=bag;Hotbar=bar;chests.erase(it);return true;
}
void ClearChests() {chests.clear();Backpack={};Hotbar=Mode==GameMode::Survival?std::array<ItemStack,9>{}:DefaultHotbar();CursorStack={};ClearCrafting();EquippedArmor={};ResetVitals();}
int TransferStack(ItemStack& source,ItemStack* destination,size_t count) {
    int before=source.count;
    for(int pass=0;pass<2&&source.count;++pass) for(size_t i=0;i<count&&source.count;++i) {
        ItemStack& target=destination[i];
        if(&target==&source) continue;
        if(pass==0?(target.block!=source.block||target.damage!=source.damage||!target.count):target.count!=0) continue;
        int moved=std::min(int(source.count),StackCapacity(source.block)-int(target.count));
        if(!moved) continue;
        if(!target.count) target.damage=source.damage;
        target.block=source.block;target.count+=moved;source.count-=moved;
    }
    if(!source.count) source={};
    return before-source.count;
}
void ClickStack(ItemStack& slot,bool right,bool shift) {
    if(shift&&slot.count) {
        TransferStack(slot,Backpack.data(),Backpack.size());
        TransferStack(slot,Hotbar.data(),Hotbar.size());
        return;
    }
    if(slot.count&&CursorStack.count&&ToolDurability(slot.block)>0&&slot.block==CursorStack.block) {
        if(!right) std::swap(slot,CursorStack);
        return;
    }
    if(!CursorStack.count) {
        if(!slot.count) return;
        int amount=right?(slot.count+1)/2:slot.count;
        CursorStack=slot;CursorStack.count=static_cast<unsigned char>(amount);slot.count-=amount;
        if(!slot.count) slot={};
    } else if(!slot.count||(slot.block==CursorStack.block&&slot.damage==CursorStack.damage)) {
        int amount=std::min(right?1:int(CursorStack.count),StackCapacity(CursorStack.block)-int(slot.count));
        if(amount) {if(!slot.count) slot.damage=CursorStack.damage;slot.block=CursorStack.block;slot.count+=amount;CursorStack.count-=amount;}
        if(!CursorStack.count) CursorStack={};
    } else if(!right) std::swap(slot,CursorStack);
}
std::vector<unsigned char> SerializeChests() {
    std::vector<unsigned char> data;
    auto word=[&](uint32_t value) {for(int i=0;i<4;++i) data.push_back(static_cast<unsigned char>(value>>(8*i)));};
    auto stack=[&](ItemStack s) {data.push_back(s.block);data.push_back(s.count);data.push_back(static_cast<unsigned char>(s.damage));data.push_back(static_cast<unsigned char>(s.damage>>8));};
    word(uint32_t(chests.size()));
    for(ItemStack s:Backpack) stack(s);
    for(ItemStack s:Hotbar) stack(s);
    stack(CursorStack);
    for(const auto& entry:chests) {
        word(uint32_t(entry.first.x));word(uint32_t(entry.first.y));word(uint32_t(entry.first.z));
        for(ItemStack s:entry.second) stack(s);
    }
    auto crafting=SerializeCrafting();data.insert(data.end(),crafting.begin(),crafting.end());
    return data;
}
bool DeserializeChests(const std::vector<unsigned char>& data,bool apply,bool legacy,bool crafting,bool wide) {
    const size_t prefix=4+(ChestSlotCount+1+(legacy?0:9))*(wide?4:2);
    const size_t record=12+ChestSlotCount*(wide?4:2);
    if(data.size()<prefix) return false;
    size_t offset=0;
    auto word=[&]() {uint32_t value=0;for(int i=0;i<4;++i) value|=uint32_t(data[offset++])<<(8*i);return value;};
    uint32_t count=word();
    size_t base=prefix+size_t(count)*record;
    if(count>65536||(crafting?data.size()<base:data.size()!=base)) return false;
    std::vector<unsigned char> extra;
    if(crafting) {extra.assign(data.begin()+base,data.end());if(!DeserializeCrafting(extra,false,wide)) return false;}
    auto stack=[&](ItemStack& s) {
        s.block=static_cast<Block>(data[offset++]);s.count=data[offset++];
        if(wide) {s.damage=data[offset]|(unsigned(data[offset+1])<<8);offset+=2;}
        return ValidBlock(s.block)&&(ToolDurability(s.block)>0?s.damage<ToolDurability(s.block):s.damage==0)&&s.count<=StackCapacity(s.block)&&((s.block==Air)==(s.count==0));
    };
    ChestInventory bag;ItemStack cursor;
    for(auto& s:bag) if(!stack(s)) return false;
    auto bar=DefaultHotbar();
    if(!legacy) for(auto& s:bar) if(!stack(s)) return false;
    if(!stack(cursor)) return false;
    std::map<Position,ChestInventory> loaded;
    for(uint32_t i=0;i<count;++i) {
        Position p{int32_t(word()),int32_t(word()),int32_t(word())};
        if(p.y<0||p.y>=Height||std::abs(int64_t(p.x))>100000128||std::abs(int64_t(p.z))>100000128) return false;
        ChestInventory slots;
        for(auto& s:slots) if(!stack(s)) return false;
        if(!loaded.emplace(p,slots).second) return false;
    }
    if(apply) {chests=std::move(loaded);Backpack=bag;Hotbar=bar;CursorStack=cursor;
        if(crafting) DeserializeCrafting(extra,true,wide);else ClearCrafting();
    }
    return true;
}
}
