#include "survival.h"
#include "world.h"
#include "map_blocks.h"
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
struct MiningDefinition {float hardness=.5f;int material=0;unsigned tools=0;};
constexpr auto MiningDefinitions=[](){
    std::array<MiningDefinition,BlockCount> values{};
#include "mining18_properties.inc"
    return values;
}();
int ToolMaterialIndex(Block tool){
    if(tool==GoldenPickaxe||tool==GoldenAxe||tool==GoldenShovel||tool==GoldenSword||tool==GoldenHoe)return 4;
    int tier=Tool(tool)?(Extended(tool)?Blocks[tool].tier:PickaxeTier(tool)):0;
    return std::clamp(tier-1,0,3);
}
unsigned HarvestBit(Block tool){
    int offset=Pickaxe(tool)?0:(Extended(tool)&&Blocks[tool].toolKind==4)?5:Sword(tool)?10:-1;
    return offset<0?0:1u<<(offset+ToolMaterialIndex(tool));
}
float ToolSpeed(Block block,Block tool,int material){
    int kind=Pickaxe(tool)?2:Extended(tool)?Blocks[tool].toolKind:0;
    bool effective=(kind==2&&material==1)||(kind==3&&(material==2||material==6))||(kind==4&&material==3);
    if(effective){constexpr float speeds[]={2,4,6,8,12};return speeds[ToolMaterialIndex(tool)];}
    if(Sword(tool)){if(material==7)return 15;if(material==5||material==6)return 1.5f;}
    (void)block;return 1;
}
float MiningDuration(Block block,Block tool,MiningDefinition definition,bool onGround,bool underwater){
    if(block==Bed||block==BedHead)return .15f;
    if(block==Air||Liquid(block)||definition.hardness<0)return std::numeric_limits<float>::infinity();
    float speed=ToolSpeed(block,tool,definition.material);
    if(!onGround)speed/=5;
    if(underwater)speed/=5;
    bool harvest=definition.tools==0||(definition.tools&HarvestBit(tool));
    double ticks=definition.hardness*(harvest?30.0:100.0)/speed;
    return float(std::max(1.0,std::ceil(ticks-1e-5))*.05);
}
}
bool CanHarvest(Block block,Block tool) {
    if(block==Air||Liquid(block)||MiningDefinitions[block].hardness<0)return false;
    auto mask=MiningDefinitions[block].tools;return !mask||(mask&HarvestBit(tool));
}
float MiningTime(Block block,Block tool,bool onGround,bool underwater) {
    return MiningDuration(block,tool,MiningDefinitions[block],onGround,underwater);
}
float MiningTimeAt(int x,int y,int z,Block tool,bool onGround,bool underwater){
    Block block=At(x,y,z);if(!HasMapShape(x,y,z))return MiningTime(block,tool,onGround,underwater);
    auto state=MapStateAt(x,y,z);auto definition=MiningDefinitions[state.material];
    switch(state.shape){
    case MapShape::Barrier:case MapShape::Portal:definition.hardness=-1;break;
    case MapShape::Slab:definition.hardness=2;break;
    case MapShape::Carpet:definition.hardness=.1f;definition.material=4;definition.tools=0;break;
    case MapShape::Fence:case MapShape::Gate:definition.hardness=2;break;
    case MapShape::Bars:definition.hardness=5;break;
    case MapShape::Pane:definition.hardness=.3f;definition.tools=0;break;
    case MapShape::Web:definition={4,7,0x7c00u};break;
    case MapShape::Torch:case MapShape::Plant:case MapShape::Fire:definition.hardness=0;definition.tools=0;break;
    default:break;
    }
    return MiningDuration(block,tool,definition,onGround,underwater);
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
