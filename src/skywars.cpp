#include "skywars.h"
#include <algorithm>
#include <cmath>
#include <random>
namespace game::sky {
std::array<Vector3,8> Bases(){std::array<Vector3,8> out{};for(int i=0;i<8;++i){float a=i*3.14159265f/4;out[i]={float(int(128+std::sin(a)*43))+.5f,32,float(int(128+std::cos(a)*43))+.5f};}return out;}
std::array<Vector3,ChestCount> Chests(){std::array<Vector3,ChestCount> out{};auto bases=Bases();for(int i=0;i<8;++i){auto p=bases[i];out[i*3]={p.x-4,32,p.z};out[i*3+1]={p.x+4,32,p.z};out[i*3+2]={p.x,28,p.z+3};}out[24]={122.5f,34,122.5f};out[25]={134.5f,34,122.5f};out[26]={122.5f,34,134.5f};out[27]={134.5f,34,134.5f};return out;}
void Build(const std::function<void(int,int,int,Block)>& put,bool lobby){
 auto island=[&](int cx,int cz,int radius,int y){for(int dy=-9;dy<=0;++dy){int r=std::max(1,radius+dy/2);for(int x=-r;x<=r;++x)for(int z=-r;z<=r;++z)if(x*x+z*z<=r*r)put(cx+x,y+dy,cz+z,dy==0?Grass:dy>-3?Dirt:Stone);}};
 auto bases=Bases();for(int i=0;i<8;++i){int x=int(bases[i].x),z=int(bases[i].z);island(x,z,8,31);
 // Open stairway into the mining room and third chest.
 for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=4;++dz)for(int y=28;y<=30;++y)put(x+dx,y,z+dz,Air);
 for(int n=0;n<4;++n){int zz=z-3-n;for(int dx=-1;dx<=1;++dx){put(x+dx,30-n,zz,Stone);for(int y=31-n;y<=33;++y)put(x+dx,y,zz,Air);}}
 put(x-2,28,z+1,IronOre);put(x+2,28,z+1,CoalOre);put(x,27,z,DiamondOre);
 for(int y=32;y<=35;++y){put(x+2,y,z-2,Wood);}
 for(int dx=-2;dx<=2;++dx)for(int dz=-2;dz<=2;++dz){put(x+2+dx,36,z-2+dz,Leaves);if(std::abs(dx)+std::abs(dz)<3)put(x+2+dx,37,z-2+dz,Leaves);}
 }
 island(128,128,14,33);for(int x:{119,137})for(int z:{119,137})for(int y=34;y<=38;++y)put(x,y,z,StoneBricks);
 for(auto p:Chests())put(int(p.x),int(p.y),int(p.z),Chest);
 if(lobby)for(int x=119;x<=137;++x)for(int z=119;z<=137;++z){put(x,65,z,Glass);if(x==119||x==137||z==119||z==137){put(x,66,z,Glass);put(x,67,z,Glass);}}
}
unsigned LootSeed(std::string_view room,int round){
    unsigned hash=2166136261u;
    for(unsigned char c:room){hash^=c;hash*=16777619u;}
    return hash^(unsigned(round)*0x9e3779b9u);
}
ChestInventory Loot(unsigned seed,int chest,int refill){
    std::mt19937 rng(seed^unsigned(chest*7919+refill*104729));
    ChestInventory out{};int n=0;bool center=chest>=24;
    auto pick=[&](std::initializer_list<Block> items){return *(items.begin()+rng()%items.size());};
    auto add=[&](Block item,int count=1){out[n++]={item,static_cast<unsigned char>(count)};};
    // Disjoint block counts keep each chest's contents distinct, even before
    // considering the different weapons, armor and utility rolls.
    int blocks=center?48+(chest-24)*4+int(rng()%4):16+(chest*5+int(seed%24)+refill*3)%24;
    add(center?Stone:pick({Stone,Cobblestone,Planks}),blocks);
    add(pick({Snowball,Egg}),center?16:8+int(rng()%9));
    if(center){
        add(DiamondSword);
        constexpr Block diamond[]={DiamondHelmet,DiamondChestplate,DiamondLeggings,DiamondBoots};
        add(diamond[(chest+refill)%4]);add(diamond[(chest+refill+1)%4]);
        add(pick({DiamondPickaxe,DiamondAxe}));
        add(EnderPearl,1+int(rng()%3));add(GoldenApple,2+int(rng()%3));
        add(pick({SpeedPotion,JumpPotion,InvisibilityPotion}));
        if(rng()%2){add(Bow);add(Arrow,24+int(rng()%9));}
        add(pick({Tnt,Obsidian}),4+int(rng()%5));
    }else{
        constexpr Block armor[]={LeatherHelmet,LeatherChestplate,LeatherLeggings,LeatherBoots,
            ChainmailHelmet,ChainmailChestplate,ChainmailLeggings,ChainmailBoots,
            IronHelmet,IronChestplate,IronLeggings,IronBoots};
        add(armor[rng()%12]);
        int role=(chest+refill)%3;
        if(role==0){
            add(rng()%4==0?IronSword:StoneSword);
            add(armor[rng()%12]);add(pick({WoodenPickaxe,StonePickaxe}));
            if(rng()%3==0)add(GoldenApple);
        }else if(role==1){
            add(WaterBucket);add(pick({StoneAxe,IronAxe,Ladder}),1);
            if(rng()%2){add(Bow);add(Arrow,8+int(rng()%9));}
            else{add(GoldenApple);add(pick({LeatherHelmet,ChainmailBoots}));}
        }else{
            add(IronPickaxe);add(pick({WaterBucket,LavaBucket}));add(Tnt,1+int(rng()%2));
            if(rng()%2)add(GoldenApple);else add(pick({WoodenSword,StoneSword}));
        }
    }
    std::shuffle(out.begin(),out.end(),rng);return out;
}
}
