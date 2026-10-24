#include "survival.h"
#include "world.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace game {
GameMode Mode=GameMode::Creative,NewWorldMode=GameMode::Creative;
float Health=20,Breath=20,FallDistance=0;
float Absorption=0,AbsorptionTime=0,RegenerationTime=0;
std::array<ItemStack,4> EquippedArmor{};
namespace {float damageCooldown=0,drownClock=0,burnClock=0,burning=0,contactClock=0;}
void ResetVitals() {Health=Breath=20;FallDistance=0;Absorption=AbsorptionTime=RegenerationTime=0;damageCooldown=drownClock=burnClock=burning=contactClock=0;}
int DefensePoints() {int points=0;for(auto stack:EquippedArmor)if(stack.count)points+=ArmorPoints(stack.block);return std::min(20,points);}
bool EquipArmor(ItemStack& stack) {
    if(!stack.count||!Armor(stack.block))return false;
    std::swap(stack,EquippedArmor[ArmorSlot(stack.block)]);return true;
}
void EatGoldenApple() {Absorption=4;AbsorptionTime=120;RegenerationTime=5;}
void Hurt(float amount,bool armor) {
    if(Mode==GameMode::Survival&&amount>0&&damageCooldown<=0) {
        if(armor) {
            float reduction=1.f-DefensePoints()*.04f;
            for(auto& stack:EquippedArmor)if(stack.count) {stack.damage+=std::max(1,int(amount/4));if(stack.damage>=ToolDurability(stack.block))stack={};}
            amount*=reduction;
        }
        float absorbed=std::min(Absorption,amount);Absorption-=absorbed;amount-=absorbed;
        Health=std::max(0.f,Health-amount);damageCooldown=.5f;
    }
}
void UpdateHazards(Vector3 feet,float dt) {
    if(Mode!=GameMode::Survival||dt<=0||!std::isfinite(dt)) return;
    dt=std::min(dt,.25f);
    if(AbsorptionTime>0) {AbsorptionTime=std::max(0.f,AbsorptionTime-dt);if(AbsorptionTime==0)Absorption=0;}
    if(RegenerationTime>0) {Health=std::min(20.f,Health+dt*2.f);RegenerationTime=std::max(0.f,RegenerationTime-dt);}
    damageCooldown=std::max(0.f,damageCooldown-dt);
    Vector3 eye{feet.x,feet.y+EyeHeight,feet.z};
    if(WaterContains(eye)) {
        Breath=std::max(0.f,Breath-dt);
        if(Breath==0) {drownClock+=dt;if(drownClock>=1) {Hurt(2);drownClock=0;}}
    } else {Breath=std::min(20.f,Breath+dt*5);drownClock=0;}
    bool lava=false,cactus=false;
    for(int x=int(std::floor(feet.x-Radius-.01f));x<=int(std::floor(feet.x+Radius+.01f));++x)
        for(int y=int(std::floor(feet.y));y<=int(std::floor(feet.y+BodyHeight));++y)
            for(int z=int(std::floor(feet.z-Radius-.01f));z<=int(std::floor(feet.z+Radius+.01f));++z) {
                Block b=At(x,y,z);
                lava|=b==Lava&&feet.y<y+FluidHeight(x,y,z);
                cactus|=b==Cactus;
            }
    if(lava) {burning=4;Hurt(4,true);}else burning=std::max(0.f,burning-dt);
    if(burning>0) {burnClock+=dt;if(burnClock>=1) {Hurt(1);burnClock=0;}}else burnClock=0;
    bool suffocating=Solid(At(int(std::floor(eye.x)),int(std::floor(eye.y)),int(std::floor(eye.z))));
    if(cactus||suffocating||feet.y<-25) {contactClock+=dt;if(contactClock>=1) {Hurt(feet.y<-25?4:1,cactus);contactClock=0;}}
    else contactClock=0;
}
int PickaxeTier(Block tool) {return Extended(tool)&&Pickaxe(tool)?Blocks[tool].tier:(tool==WoodenPickaxe?1:tool==StonePickaxe?2:tool==IronPickaxe?3:0);}
namespace {
bool Rock(Block b) {return (Extended(b)&&Blocks[b].material==1)||b==Stone||b==Cobblestone||b==CoalOre||b==IronOre||b==Bricks||b==StoneBricks||b==CarvedStone||b==Furnace;}
}
bool CanHarvest(Block block,Block tool) {
    if(block==Bedrock||Liquid(block)||block==Air) return false;
    int required=block==IronOre?2:1;
    if(block==GoldOre||block==DiamondOre||block==EmeraldOre||block==RedstoneOre) required=3;
    if(block==Obsidian) required=4;
    return Rock(block)?PickaxeTier(tool)>=required:true;
}
float MiningTime(Block block,Block tool) {
    if(block==Bedrock||Liquid(block)||block==Air) return std::numeric_limits<float>::infinity();
    float hardness=Rock(block)?(block==IronOre||block==CoalOre?3.f:(block==Furnace?3.5f:1.5f)):
        (block==Wood||block==Planks||block==CraftingTable||block==Chest?2.f:
        (block==Leaves||Plant(block)? .2f:(Wool(block)?.8f:.5f)));
    if(Extended(block)) hardness=Blocks[block].hardness;
    float speed=Rock(block)&&PickaxeTier(tool)?float(PickaxeTier(tool)*2):1.f;
    if(Extended(tool)&&Tool(tool)) {
        bool effective=(Blocks[tool].toolKind==2&&Rock(block))||
            (Blocks[tool].toolKind==3&&(block==Wood||block==Planks||block==Chest||block==CraftingTable||(Extended(block)&&Blocks[block].material==2)))||
            (Blocks[tool].toolKind==4&&(block==Dirt||block==Grass||block==Sand||block==Gravel||block==Clay||block==Snow||(Extended(block)&&Blocks[block].material==3)));
        if(effective) speed=(tool==GoldenPickaxe||(Blocks[tool].tier==1&&ToolDurability(tool)==32))?12.f:float(Blocks[tool].tier*2);
    }
    return hardness*(CanHarvest(block,tool)?1.5f:5.f)/speed;
}
bool MineBlock(int x,int y,int z,ItemStack& tool) {
    Block block=At(x,y,z);
    if(block==Air||block==Bedrock||Liquid(block)) return false;
    Block used=tool.count?tool.block:Air;
    ItemStack drop;
    if(CanHarvest(block,used)&&block!=Glass&&block!=Leaves&&!Plant(block))
        drop={block==Stone?Cobblestone:(block==CoalOre?Coal:(block==Grass?Dirt:block)),1};
    if(block==BedHead)drop={Bed,1};
    if(drop.count) {
        if(block==DiamondOre) drop.block=Diamond;
        if(block==EmeraldOre) drop.block=Emerald;
        if(block==NetherQuartzOre) drop.block=Quartz;
        if(block==LapisLazuliOre) {drop.block=LapisLazuli;drop.count=4;}
        if(block==RedstoneOre) {drop.block=Redstone;drop.count=4;}
    }
    if(drop.count) {
        auto bag=Backpack;auto bar=Hotbar;ItemStack test=drop;
        TransferStack(test,bag.data(),bag.size());TransferStack(test,bar.data(),bar.size());
        if(test.count) return false;
    }
    Change(x,y,z,Air);if(At(x,y,z)!=Air) return false;
    TransferStack(drop,Backpack.data(),Backpack.size());TransferStack(drop,Hotbar.data(),Hotbar.size());
    if(Tool(used)&&tool.count) {
        tool.damage+=Sword(used)?2:1;
        if(tool.damage>=ToolDurability(used)) tool={};
    }
    return true;
}
bool PlaceSurvivalBlock(int x,int y,int z,ItemStack& stack) {
    if(!stack.count||!Placeable(stack.block)||stack.block==Air||!Inside(x,y,z)||
       (At(x,y,z)!=Air&&!Liquid(At(x,y,z)))) return false;
    Block block=stack.block;Change(x,y,z,block);if(At(x,y,z)!=block) return false;
    if(Mode==GameMode::Survival) {--stack.count;if(!stack.count) stack={};}
    return true;
}
std::vector<unsigned char> SerializeSurvival() {
    std::vector<unsigned char> data;data.push_back(Mode==GameMode::Survival?1:0);
    auto value=[&](float f) {uint32_t bits;std::memcpy(&bits,&f,4);for(int i=0;i<4;++i)data.push_back(static_cast<unsigned char>(bits>>(8*i)));};
    for(float f:{Health,Breath,FallDistance})value(f);
    for(auto s:EquippedArmor) {data.push_back(s.block);data.push_back(s.count);data.push_back(s.damage&255);data.push_back(s.damage>>8);}
    for(float f:{Absorption,AbsorptionTime,RegenerationTime})value(f);
    return data;
}
bool DeserializeSurvival(const std::vector<unsigned char>& data,bool apply) {
    if((data.size()!=13&&data.size()!=41)||data[0]>1) return false;
    auto value=[&](int offset) {uint32_t bits=0;for(int i=0;i<4;++i)bits|=uint32_t(data[offset+i])<<(8*i);float f;std::memcpy(&f,&bits,4);return f;};
    float health=value(1),breath=value(5),fall=value(9),shield=0,shieldTime=0,regen=0;
    if(!std::isfinite(health)||health<0||health>20||!std::isfinite(breath)||breath<0||breath>20||!std::isfinite(fall)||fall<0||fall>10000)return false;
    std::array<ItemStack,4> armor{};
    if(data.size()==41) {
        for(int i=0;i<4;++i) {
            int o=13+i*4;auto& s=armor[i];s={static_cast<Block>(data[o]),data[o+1],static_cast<unsigned short>(data[o+2]|(data[o+3]<<8))};
            if(s.count?(s.count!=1||!Armor(s.block)||ArmorSlot(s.block)!=i||s.damage>=ToolDurability(s.block)):(s.block!=Air||s.damage))return false;
        }
        shield=value(29);shieldTime=value(33);regen=value(37);
        if(!std::isfinite(shield)||shield<0||shield>4||!std::isfinite(shieldTime)||shieldTime<0||shieldTime>120||!std::isfinite(regen)||regen<0||regen>5)return false;
    }
    if(apply) {ResetVitals();Mode=data[0]?GameMode::Survival:GameMode::Creative;Health=health;Breath=breath;FallDistance=fall;EquippedArmor=armor;Absorption=shield;AbsorptionTime=shieldTime;RegenerationTime=regen;}
    return true;
}
}
