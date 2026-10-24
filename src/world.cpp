#include "file_replace.h"
#include "world.h"
#include "map_blocks.h"
#include "lighting.h"
#include "settings.h"
#include "chest.h"
#include "crafting.h"
#include "survival.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <chrono>
#include <random>
#include <fstream>
#include <filesystem>
#include <vector>
#include <deque>
namespace game {
uint32_t Seed=0;
int32_t RegionOriginX=0,RegionOriginZ=0;
float WorldTime=DayDuration*.25f;
int ClockMinute() {
    float minute=std::fmod(WorldTime/DayDuration*1440.f+360.f,1440.f);
    return std::clamp(int(std::lround(minute))%1440,0,1439);
}
void SetClockMinute(int minute) {
    minute=std::clamp(minute,0,1439);
    WorldTime=float((minute+1080)%1440)/1440.f*DayDuration;
}
float Daylight() {return std::clamp(.5f+.75f*std::sin(WorldTime/DayDuration*2*PI),0.f,1.f);}
Color SkyColor() {
    float t=Daylight();
    return {static_cast<unsigned char>(12+132*t),static_cast<unsigned char>(17+173*t),
            static_cast<unsigned char>(35+183*t),255};
}
std::array<uint64_t,ChunkCount*ChunkCount> ChunkVersions{};
unsigned char World[Width][Height][Width]{};
namespace {
unsigned char FluidLevels[Width][Height][Width]{};
unsigned char FluidSources[Width][Height][Width]{};
}
unsigned char FluidLevelAt(int x,int y,int z) {
    return Inside(x,y,z)&&Liquid(At(x,y,z))?FluidLevels[x][y][z]:0;
}
bool IsFluidSource(int x,int y,int z) {
    return Liquid(At(x,y,z))&&FluidSources[x][y][z]!=0;
}
unsigned char WaterLevelAt(int x,int y,int z) {return At(x,y,z)==Water?FluidLevelAt(x,y,z):0;}
unsigned char LavaLevelAt(int x,int y,int z) {return At(x,y,z)==Lava?FluidLevelAt(x,y,z):0;}
bool IsWaterSource(int x,int y,int z) {return At(x,y,z)==Water&&IsFluidSource(x,y,z);}
bool IsLavaSource(int x,int y,int z) {return At(x,y,z)==Lava&&IsFluidSource(x,y,z);}
float FluidHeight(int x,int y,int z) {
    Block fluid=At(x,y,z);
    if(!Liquid(fluid)) return 0;
    if(At(x,y+1,z)==fluid) return 1;
    return std::clamp(FluidLevelAt(x,y,z)/7.f,0.f,1.f);
}
float WaterHeight(int x,int y,int z) {return At(x,y,z)==Water?FluidHeight(x,y,z):0;}
bool WaterContains(Vector3 p) {
    int x=int(std::floor(p.x)),y=int(std::floor(p.y)),z=int(std::floor(p.z));
    return At(x,y,z)==Water&&p.y<y+WaterHeight(x,y,z);
}
Biome Biomes[Width][Width]{};
int Surface[Width][Width]{};
bool Inside(int x,int y,int z) { return x>=0&&x<Width&&y>=0&&y<Height&&z>=0&&z<Width; }
Block At(int x,int y,int z) { return Inside(x,y,z)?static_cast<Block>(World[x][y][z]):Air; }
uint32_t Hash(int x,int z,int salt) {
    uint32_t v=uint32_t(x)*374761393u+uint32_t(z)*668265263u+uint32_t(salt)*2246822519u+Seed;
    v=(v^(v>>13))*1274126177u; return v^(v>>16);
}
namespace {
constexpr float WaterTick = 0.15f;
float waterClock = 0.0f;
float lavaClock = 0.0f;

void ResetWaterFlow() {
    waterClock = lavaClock = 0.0f;
}

void WriteFluidCell(int x, int y, int z, Block block,
                    unsigned char level, bool source) {
    if(!Inside(x,y,z)) return;
    const auto nextLevel = Liquid(block) ? level : 0;
    const auto nextSource = Liquid(block) && source ? 1 : 0;
    if (At(x, y, z) == block && FluidLevels[x][y][z] == nextLevel &&
        FluidSources[x][y][z] == nextSource) {
        return;
    }

    if(LightPassable(At(x,y,z))!=LightPassable(block)||Emission(At(x,y,z))!=Emission(block)) InvalidateLighting(x,y,z);
    World[x][y][z] = block;
    FluidLevels[x][y][z] = static_cast<unsigned char>(nextLevel);
    FluidSources[x][y][z] = static_cast<unsigned char>(nextSource);

    constexpr int offsets[9][2] = {{0,0},{1,0},{-1,0},{0,1},{0,-1},
                                   {1,1},{1,-1},{-1,1},{-1,-1}};
    for (const auto& offset : offsets) {
        const int nx = x + offset[0];
        const int nz = z + offset[1];
        if (nx >= 0 && nx < Width && nz >= 0 && nz < Width) {
            ++ChunkVersions[(nx / ChunkSize) * ChunkCount + nz / ChunkSize];
        }
    }
}

constexpr size_t FluidCellCount = size_t(Width) * Height * Width;
constexpr unsigned char NoSupport = 255;
constexpr int Directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

size_t FluidIndex(int x, int y, int z) {
    return size_t((x * Height + y) * Width + z);
}

// Compare complete traversable paths; preserve every equally short first step.
unsigned DrainDirections(int x,int y,int z,unsigned char level,Block fluid) {
    if(y<=0||level<=1) return 0;
    constexpr int radius=6,side=radius*2+1;
    struct Search {int x,z,distance;};
    int best=radius+1;unsigned result=0;
    for(int first=0;first<4;++first) {
        bool visited[side][side]{};
        visited[radius][radius]=true;
        std::deque<Search> queue;
        queue.push_back({x+Directions[first][0],z+Directions[first][1],1});
        while(!queue.empty()) {
            Search n=queue.front();queue.pop_front();
            if(n.distance>=level||n.distance>best||n.distance>radius||!Inside(n.x,y,n.z)) continue;
            int vx=n.x-x+radius,vz=n.z-z+radius;
            if(visited[vx][vz]) continue;
            visited[vx][vz]=true;
            Block cell=At(n.x,y,n.z),below=At(n.x,y-1,n.z);
            if(cell!=Air&&cell!=fluid) continue;
            if(below==Air||below==fluid) {
                if(n.distance<best) {best=n.distance;result=0;}
                result|=1u<<first;break;
            }
            if(!Solid(below)||n.distance==radius) continue;
            for(const auto& d:Directions) queue.push_back({n.x+d[0],n.z+d[1],n.distance+1});
        }
    }
    return result;
}
struct FluidSupport {
    std::vector<unsigned char> levels=std::vector<unsigned char>(FluidCellCount,NoSupport);
};
FluidSupport SourceConnections(Block fluid) {
    FluidSupport support;
    struct Node {int x,y,z;unsigned char level;};
    std::deque<Node> queue;
    auto visit=[&](int x,int y,int z,unsigned char level) {
        if(At(x,y,z)!=fluid||level==0) return;
        size_t id=FluidIndex(x,y,z);
        if(At(x,y,z)==fluid&&IsFluidSource(x,y,z)) {level=MaxWaterLevel;}
        if(support.levels[id]!=NoSupport) {
            if(support.levels[id]>level) return;
            if(support.levels[id]==level) return;
        }
        support.levels[id]=level;
        queue.push_back({x,y,z,level});
    };
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        if(At(x,y,z)==fluid&&IsFluidSource(x,y,z)) visit(x,y,z,MaxWaterLevel);
    while(!queue.empty()) {
        Node n=queue.front();queue.pop_front();
        size_t id=FluidIndex(n.x,n.y,n.z);
        if(support.levels[id]!=n.level||n.y==0) continue;
        Block below=At(n.x,n.y-1,n.z);
        if(below==fluid) visit(n.x,n.y-1,n.z,MaxWaterLevel);
        if(!Solid(below)||n.level<=1) continue;
        unsigned drains=DrainDirections(n.x,n.y,n.z,n.level,fluid);
        for(int i=0;i<4;++i) if(drains==0||(drains&(1u<<i)))
            visit(n.x+Directions[i][0],n.y,n.z+Directions[i][1],n.level-1);
    }
    return support;
}

constexpr unsigned char HorizontalLevel(unsigned char level) {
    return level > 1 ? static_cast<unsigned char>(level - 1) : 0;
}
static_assert(HorizontalLevel(7) == 6 && HorizontalLevel(1) == 0 && HorizontalLevel(0) == 0);

std::array<bool, 4> LowestNeighbors(int x, int y, int z, unsigned char level,Block fluid) {
    std::array<bool, 4> chosen{};
    int bestLevel = MaxWaterLevel + 1;
    for (size_t i = 0; i < chosen.size(); ++i) {
        const int nx = x + Directions[i][0];
        const int nz = z + Directions[i][1];
        if (!Inside(nx, y, nz)) continue;
        const Block neighbor = At(nx, y, nz);
        if (neighbor != Air && neighbor != fluid) continue;
        const int neighborLevel = neighbor == Air ? 0 : FluidLevelAt(nx, y, nz);
        if (neighborLevel >= HorizontalLevel(level)) continue;
        if (neighborLevel < bestLevel) {
            chosen.fill(false);
            bestLevel = neighborLevel;
        }
        if (neighborLevel == bestLevel) chosen[i] = true;
    }
    return chosen;
}

void StepFluid(Block fluid) {
    // Compute from the current grid, then commit all results together.
    const auto support = SourceConnections(fluid);
    std::vector<unsigned char> next(FluidCellCount, 0);
    std::vector<unsigned char> incoming(FluidCellCount, 0);
    auto propose = [&](int x, int y, int z, unsigned char level) {
        if (!Inside(x, y, z) || level == 0 || level > MaxWaterLevel) return;
        const Block block = At(x, y, z);
        if (block == Air || (block == fluid && FluidLevelAt(x, y, z) < level)) {
            auto& value = incoming[FluidIndex(x, y, z)];
            value = std::max(value, level);
        }
    };

    for (int x = 0; x < Width; ++x) {
        for (int y = 0; y < Height; ++y) {
            for (int z = 0; z < Width; ++z) {
                if (At(x, y, z) != fluid) continue;
                const size_t id = FluidIndex(x, y, z);
                const unsigned char current = FluidLevelAt(x, y, z);
                const unsigned char target = support.levels[id] == NoSupport ? 0 : support.levels[id];
                next[id] = current > target ? static_cast<unsigned char>(current - 1) : current;
                if (FluidSources[x][y][z]) next[id] = MaxWaterLevel;
                // Disconnected remnants may drain, but cannot feed new water.
                if (target == 0 || current == 0 || y == 0) continue;
                const unsigned char level = std::min(current, target);
                const Block below = At(x, y - 1, z);
                if (below == Air || (below == fluid && FluidLevelAt(x, y - 1, z) < MaxWaterLevel)) {
                    propose(x, y - 1, z, MaxWaterLevel);
                    continue;
                }
                const unsigned char lateralLevel = HorizontalLevel(level);
                if (!Solid(below) || lateralLevel == 0) continue;
                const unsigned drains=DrainDirections(x,y,z,level,fluid);
                if(drains) {
                    for(int i=0;i<4;++i) if(drains&(1u<<i))
                        propose(x+Directions[i][0],y,z+Directions[i][1],lateralLevel);
                    continue;
                }
                const auto chosen = LowestNeighbors(x, y, z, level,fluid);
                for (size_t i = 0; i < chosen.size(); ++i)
                    if (chosen[i]) propose(x + Directions[i][0], y, z + Directions[i][1],
                                           lateralLevel);
            }
        }
    }

    for (int x = 0; x < Width; ++x) {
        for (int y = 0; y < Height; ++y) {
            for (int z = 0; z < Width; ++z) {
                const Block block = At(x, y, z);
                if (block != Air && block != fluid) continue;
                const size_t id = FluidIndex(x, y, z);
                const unsigned char level = std::max(next[id], incoming[id]);
                if (level > 0) WriteFluidCell(x, y, z, fluid, level, IsFluidSource(x, y, z));
                else if (block == fluid) WriteFluidCell(x, y, z, Air, 0, false);
            }
        }
    }
}
}

void UpdateWater(float dt) {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }
    waterClock += std::min(dt, 0.25f);
    while (waterClock >= WaterTick) {
        waterClock -= WaterTick;
        StepFluid(Water);
    }
}

void UpdateLava(float dt) {
    if(!std::isfinite(dt)||dt<=0) return;
    lavaClock+=std::min(dt,.25f);
    constexpr float LavaTick=2*WaterTick;
    while(lavaClock>=LavaTick) {lavaClock-=LavaTick;StepFluid(Lava);}
}

void UpdateWorld(float dt) {
    if(!Settings.freezeTime&&std::isfinite(dt)) WorldTime=std::fmod(WorldTime+std::max(0.f,dt),DayDuration);
    UpdateWater(dt);
    UpdateLava(dt);
    UpdateFurnaces(dt);
}

float FadeCurve(float t) { return t*t*t*(t*(t*6-15)+10); }
float Perlin(float x,float z) {
    int ix=int(std::floor(x)),iz=int(std::floor(z));
    float u=x-ix,v=z-iz;
    auto gradient=[](int a,int b,float dx,float dz) {
        constexpr float diagonal=.70710678f;
        const Vector2 gradients[8]={{1,0},{-1,0},{0,1},{0,-1},
            {diagonal,diagonal},{-diagonal,diagonal},{diagonal,-diagonal},{-diagonal,-diagonal}};
        Vector2 g=gradients[Hash(a,b)%8]; return g.x*dx+g.y*dz;
    };
    return Lerp(Lerp(gradient(ix,iz,u,v),gradient(ix+1,iz,u-1,v),FadeCurve(u)),
                Lerp(gradient(ix,iz+1,u,v-1),gradient(ix+1,iz+1,u-1,v-1),FadeCurve(u)),FadeCurve(v));
}
float Fbm(float x,float z,int octaves=5) {
    float total=0,amplitude=.5f,weight=0;
    for(int i=0;i<octaves;++i) {
        total+=Perlin(x,z)*amplitude; weight+=amplitude;
        x=x*2.03f+17.2f; z=z*2.03f+9.1f; amplitude*=.5f;
    }
    return total/weight;
}
namespace {
struct TerrainColumn {int height;Biome biome;};
TerrainColumn ColumnAt(int x,int z) {
    float moisture=Fbm(x*.013f+130,z*.013f-74,3);
    float warmth=Fbm(x*.012f-218,z*.012f+83,3);
    float mountain=std::clamp((Fbm(x*.016f+19,z*.016f+31,3)+.07f)*3.f,0.f,1.f);
    float ridge=1.f-std::abs(Perlin(x*.038f+11,z*.038f+35));
    float warpX=x+12*Fbm(x*.018f,z*.018f),warpZ=z+12*Fbm(x*.018f+30,z*.018f+30);
    int height=std::clamp(int(25+14*Fbm(warpX*.025f,warpZ*.025f)+mountain*mountain*26*ridge),7,Height-13);
    Biome biome=mountain>.58f?Biome::Mountains:(warmth>.035f&&moisture<.055f?Biome::Desert:
        (moisture>.035f?Biome::Forest:Biome::Plains));
    return {height,biome};
}
int FloorDiv(int value,int divisor) {int q=value/divisor;return q-(value%divisor<0);}
void GenerateRegion() {
    std::memset(World,0,sizeof(World));
    std::memset(FluidLevels,0,sizeof(FluidLevels));
    std::memset(FluidSources,0,sizeof(FluidSources));
    for(int x=0;x<Width;++x) for(int z=0;z<Width;++z) {
        int wx=RegionOriginX+x,wz=RegionOriginZ+z;
        auto column=ColumnAt(wx,wz);int h=column.height;Biome biome=column.biome;
        Biomes[x][z]=biome;Surface[x][z]=h;
        for(int y=0;y<=h;++y) {
            Block block=Stone;
            if(y>h-4) block=(biome==Biome::Desert||h<=SeaLevel+1)?Sand:Dirt;
            if(y==h&&block==Dirt) block=Grass;
            if(biome==Biome::Mountains&&h>43&&y>h-2) block=Stone;
            if(block==Stone&&y<h-4&&Hash(wx,wz,y)%41<3) block=CoalOre;
            else if(block==Stone&&y<h-4&&Hash(wx,wz,y+174)%67<3) block=IronOre;
            if(block==Stone&&y<16&&Hash(wx,wz,y+280)%191<2) block=DiamondOre;
            else if(block==Stone&&y<24&&Hash(wx,wz,y+290)%131<3) block=GoldOre;
            else if(block==Stone&&y<24&&Hash(wx,wz,y+300)%149<3) block=RedstoneOre;
            else if(block==Stone&&y<24&&Hash(wx,wz,y+310)%151<2) block=LapisLazuliOre;
            else if(block==Stone&&y<24&&Hash(wx,wz,y+320)%503==0) block=EmeraldOre;
            World[x][y][z]=block;
        }
        for(int y=h+1;y<=SeaLevel;++y) {
            World[x][y][z]=Water;FluidLevels[x][y][z]=7;FluidSources[x][y][z]=1;
        }
    }
    // Generate tree roots beyond the window so crowns agree across chunk boundaries.
    for(int gx=FloorDiv(RegionOriginX-7,7)*7;gx<RegionOriginX+Width+7;gx+=7)
        for(int gz=FloorDiv(RegionOriginZ-7,7)*7;gz<RegionOriginZ+Width+7;gz+=7) {
            int wx=gx+int(Hash(gx,gz,10)%4),wz=gz+int(Hash(gx,gz,11)%4);
            int x=wx-RegionOriginX,z=wz-RegionOriginZ;
            auto column=ColumnAt(wx,wz);int y=column.height;
            if(y<=SeaLevel+1) continue;
            if(column.biome==Biome::Desert) {
                if(Hash(wx,wz,12)%3==0) for(int i=1;i<=2+int(Hash(wx,wz,13)%3);++i)
                    if(Inside(x,y+i,z)) World[x][y+i][z]=Cactus;
            } else {
                unsigned chance=column.biome==Biome::Forest?85u:24u;
                if(Hash(wx,wz,14)%100>=chance||(column.biome==Biome::Mountains&&y>43)) continue;
                int trunk=4+int(Hash(wx,wz,15)%3);
                for(int dy=1;dy<=trunk;++dy) if(Inside(x,y+dy,z)) World[x][y+dy][z]=Wood;
                for(int dy=-1;dy<=3;++dy) for(int dx=-3;dx<=3;++dx) for(int dz=-3;dz<=3;++dz) {
                    float r=dy<0?2.3f:std::sqrt(std::max(0.f,10.f-float(dy*dy)));
                    if(dx*dx+dz*dz>r*r) continue;
                    int yy=y+trunk-1+dy;
                    if(Inside(x+dx,yy,z+dz)&&At(x+dx,yy,z+dz)==Air) World[x+dx][yy][z+dz]=Leaves;
                }
            }
        }
    ResetWaterFlow();InvalidateLighting();
    for(auto& version:ChunkVersions) ++version;
}
}
void GenerateWorld(uint32_t seed) {
    ClearMapBlocks();
    ClearChests();
    Seed=seed;RegionOriginX=RegionOriginZ=0;
    SetClockMinute(Settings.newWorldMinute);
    GenerateRegion();
}

void Change(int x,int y,int z,Block b) {
    if(!Inside(x,y,z)||!Placeable(b)) return;
    if(At(x,y,z)==b && (!Liquid(b)||FluidSources[x][y][z])) return;
    if(At(x,y,z)==Chest&&b!=Chest&&!RemoveChest(x,y,z)) return;
    if(At(x,y,z)==Furnace&&b!=Furnace&&!RemoveFurnace(x,y,z)) return;
    Block previous=At(x,y,z);
    if((previous==Bed||previous==BedHead)&&b!=previous) {
        int otherZ=z+(previous==Bed?1:-1);
        if(At(x,y,otherZ)==(previous==Bed?BedHead:Bed))WriteFluidCell(x,y,otherZ,Air,0,false);
    }
    WriteFluidCell(x,y,z,b,Liquid(b)?MaxWaterLevel:0,Liquid(b));
}
void ClearRemoteWorld(bool inventory) {
    ClearMapBlocks();
    std::memset(World,0,sizeof(World));std::memset(FluidLevels,0,sizeof(FluidLevels));std::memset(FluidSources,0,sizeof(FluidSources));
    if(inventory){ClearChests();Hotbar={};Backpack={};CursorStack={};}
    InvalidateLighting();for(auto& version:ChunkVersions)++version;
}
void SetRemoteBlock(int x,int y,int z,Block b) {if(Inside(x,y,z)&&(b==Air||Placeable(b)))WriteFluidCell(x,y,z,b,Liquid(b)?7:0,Liquid(b));}
void ApplyRemoteChunk(int cx,int cz,const std::vector<Block>& blocks) {
    if(cx<0||cz<0||cx>=ChunkCount||cz>=ChunkCount||blocks.size()!=size_t(ChunkSize*Height*ChunkSize))return;
    for(int x=0;x<ChunkSize;++x)for(int y=0;y<Height;++y)for(int z=0;z<ChunkSize;++z){Block b=blocks[(x*Height+y)*ChunkSize+z];int wx=cx*ChunkSize+x,wz=cz*ChunkSize+z;World[wx][y][wz]=Placeable(b)?b:Air;FluidLevels[wx][y][wz]=Liquid(b)?7:0;FluidSources[wx][y][wz]=Liquid(b);}
    for(int x=std::max(0,cx-1);x<=std::min(ChunkCount-1,cx+1);++x)for(int z=std::max(0,cz-1);z<=std::min(ChunkCount-1,cz+1);++z)++ChunkVersions[x*ChunkCount+z];
    InvalidateLighting();
}
uint32_t RandomSeed() {
    uint64_t now=uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    uint32_t seed=uint32_t(now)^uint32_t(now>>32);
    try {std::random_device rd; seed^=rd();} catch(...) {}
    return seed==Seed?seed+1:seed;
}
namespace {
bool ReadChunk(const std::filesystem::path& path,int cx,int cz,std::vector<unsigned char>& data,uint32_t expectedSeed);
void ApplyChunk(int cx,int cz,const std::vector<unsigned char>& data);
constexpr unsigned char Magic[8]={'B','L','O','C','O','S',12,0};
constexpr size_t PoseBytes=6*sizeof(uint32_t);
static_assert(sizeof(float)==sizeof(uint32_t)&&std::numeric_limits<float>::is_iec559,
              "Saves require 32-bit IEEE 754 floats");
bool ValidPlayerState(const PlayerState& p) {
    return p.valid&&std::isfinite(p.position.x)&&std::isfinite(p.position.y)&&std::isfinite(p.position.z)&&
        std::isfinite(p.yaw)&&std::isfinite(p.pitch)&&std::isfinite(p.velocityY)&&std::abs(p.pitch)<=1.55f;
}
void EncodeFloat(unsigned char* out,float value) {
    uint32_t bits;std::memcpy(&bits,&value,sizeof(bits));
    for(int i=0;i<4;++i) out[i]=static_cast<unsigned char>((bits>>(i*8))&255);
}
float DecodeFloat(const unsigned char* in) {
    uint32_t bits=0;for(int i=0;i<4;++i) bits|=uint32_t(in[i])<<(i*8);
    float value;std::memcpy(&value,&bits,sizeof(value));return value;
}
uint32_t Checksum(const unsigned char* data,size_t size) {
    uint32_t hash=2166136261u;
    for(size_t i=0;i<size;++i) hash=(hash^data[i])*16777619u;
    return hash;
}
void Write32(std::ostream& out,uint32_t value) {
    for(int i=0;i<4;++i) out.put(char((value>>(i*8))&255));
}
uint32_t Read32(std::istream& in) {
    uint32_t v=0;
    for(int i=0;i<4;++i) {int c=in.get(); if(c==EOF) {in.setstate(std::ios::failbit); return 0;} v|=uint32_t(c)<<(i*8);}
    return v;
}
}
bool SaveWorld(const std::string& path,const PlayerState& player) {
    if(!ValidPlayerState(player)) return false;
    const std::string temporary=path+".tmp";
    std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    if(!out) return false;
    out.write(reinterpret_cast<const char*>(Magic),8);
    Write32(out,Width); Write32(out,Height); Write32(out,Seed);
    std::vector<unsigned char> data(sizeof(World)+sizeof(FluidLevels)+sizeof(FluidSources)+PoseBytes+4+12);
    std::memcpy(data.data(),World,sizeof(World));
    std::memcpy(data.data()+sizeof(World),FluidLevels,sizeof(FluidLevels));
    std::memcpy(data.data()+sizeof(World)+sizeof(FluidLevels),FluidSources,sizeof(FluidSources));
    const float pose[6]={player.position.x,player.position.y,player.position.z,player.yaw,player.pitch,player.velocityY};
    const size_t poseOffset=sizeof(World)+sizeof(FluidLevels)+sizeof(FluidSources);
    for(int i=0;i<6;++i) EncodeFloat(data.data()+poseOffset+i*4,pose[i]);
    EncodeFloat(data.data()+poseOffset+PoseBytes,WorldTime);
    const uint32_t region[3]={static_cast<uint32_t>(RegionOriginX),static_cast<uint32_t>(RegionOriginZ),uint32_t(player.flying)};
    for(int i=0;i<3;++i) for(int j=0;j<4;++j)
        data[poseOffset+PoseBytes+4+i*4+j]=static_cast<unsigned char>(region[i]>>(8*j));
    auto containers=SerializeChests();
    data.insert(data.end(),containers.begin(),containers.end());
    Write32(out,Checksum(data.data(),data.size()));
    out.write(reinterpret_cast<const char*>(data.data()),data.size());
    out.flush(); bool ok=bool(out); out.close(); ok=ok&&!out.fail();
    std::error_code error;
    if(ok) {ReplaceSaveFile(temporary,path,error); ok=!error;}
    if(!ok) std::filesystem::remove(temporary,error);
    return ok;
}
bool LoadWorld(const std::string& path,PlayerState* player) {
    std::ifstream in(path,std::ios::binary);
    unsigned char magic[8]{};
    in.read(reinterpret_cast<char*>(magic),8);
    if(!in||std::memcmp(magic,Magic,6)!=0||magic[7]!=0||(magic[6]<1||magic[6]>12)) return false;
    bool legacy=magic[6]==1;
    uint32_t width=Read32(in),height=Read32(in),seed=Read32(in),checksum=Read32(in);
    if(!in||(width!=Width&&width!=128)||height!=Height) return false;
    const size_t cells=size_t(width)*height*width;
    std::vector<unsigned char> data(cells+(legacy?0:cells)+(magic[6]>=3?cells:0)+(magic[6]>=4?PoseBytes:0)+(magic[6]>=5?4:0)+(magic[6]>=7?12:0));
    in.read(reinterpret_cast<char*>(data.data()),data.size());
    std::vector<unsigned char> containers;
    if(magic[6]>=8) {
        uint32_t count=Read32(in);
        if(!in||count>65536) return false;
        size_t stackBytes=magic[6]>=11?4:2;
        containers.resize(4+(magic[6]>=9?37:28)*stackBytes+size_t(count)*(12+27*stackBytes));
        for(int i=0;i<4;++i) containers[i]=static_cast<unsigned char>(count>>(8*i));
        in.read(reinterpret_cast<char*>(containers.data()+4),containers.size()-4);
        if(magic[6]>=10) {
            const size_t base=containers.size();size_t prefix=magic[6]>=11?56:30,record=magic[6]>=11?32:26;
            containers.resize(base+prefix);
            in.read(reinterpret_cast<char*>(containers.data()+base),prefix);
            if(!in) return false;
            uint32_t furnaceCount=0;for(int i=0;i<4;++i) furnaceCount|=uint32_t(containers[base+prefix-4+i])<<(8*i);
            if(furnaceCount>65536) return false;
            containers.resize(base+prefix+size_t(furnaceCount)*record+(magic[6]>=12?41:(magic[6]>=11?13:0)));
            in.read(reinterpret_cast<char*>(containers.data()+base+prefix),size_t(furnaceCount)*record+(magic[6]>=12?41:(magic[6]>=11?13:0)));
        }
        if(!in||!DeserializeChests(containers,false,magic[6]<9,magic[6]>=10,magic[6]>=11)) return false;
        data.insert(data.end(),containers.begin(),containers.end());
    }
    if(!in||in.peek()!=EOF||Checksum(data.data(),data.size())!=checksum) return false;
    for(size_t i=0;i<cells;++i) {
        if(!Placeable(static_cast<Block>(data[i]))) return false;
        if(!legacy) {
            unsigned char level=data[cells+i];
            if(level>MaxWaterLevel||((magic[6]>=6?!Liquid(static_cast<Block>(data[i])):data[i]!=Water)&&level!=0)) return false;
            if(magic[6]>=3) {
                unsigned char source=data[cells+cells+i];
                if(source>1||(source&&((magic[6]>=6?!Liquid(static_cast<Block>(data[i])):data[i]!=Water)||level!=MaxWaterLevel))) return false;
            }
        }
    }
    PlayerState loadedPlayer;
    if(magic[6]>=4) {
        size_t offset=cells+cells+cells;
        loadedPlayer.position={DecodeFloat(data.data()+offset),DecodeFloat(data.data()+offset+4),DecodeFloat(data.data()+offset+8)};
        loadedPlayer.yaw=DecodeFloat(data.data()+offset+12);loadedPlayer.pitch=DecodeFloat(data.data()+offset+16);
        loadedPlayer.velocityY=DecodeFloat(data.data()+offset+20);loadedPlayer.valid=true;
        if(!ValidPlayerState(loadedPlayer)) return false;
    }
    float loadedTime=DayDuration*.25f;
    if(magic[6]>=5) {
        loadedTime=DecodeFloat(data.data()+cells+cells+cells+PoseBytes);
        if(!std::isfinite(loadedTime)||loadedTime<0||loadedTime>=DayDuration) return false;
    }
    int32_t originX=0,originZ=0;
    if(magic[6]>=7) {
        const size_t offset=cells+cells+cells+PoseBytes+4;
        uint32_t words[3]{};
        for(int i=0;i<3;++i) for(int j=0;j<4;++j) words[i]|=uint32_t(data[offset+i*4+j])<<(8*j);
        std::memcpy(&originX,&words[0],4);std::memcpy(&originZ,&words[1],4);
        if(originX%ChunkSize||originZ%ChunkSize||std::abs(int64_t(originX))>100000000||
           std::abs(int64_t(originZ))>100000000||words[2]>1) return false;
        loadedPlayer.flying=words[2]!=0;
    }
    struct Expansion {int x,z;std::vector<unsigned char> data;};
    std::vector<Expansion> expansion;
    if(width<Width) {
        std::error_code error;
        for(int x=0;x<ChunkCount;++x) for(int z=0;z<ChunkCount;++z) {
            if(x<int(width)/ChunkSize&&z<int(width)/ChunkSize) continue;
            int cx=originX/ChunkSize+x,cz=originZ/ChunkSize+z;
            auto archive=std::filesystem::path(path+".chunks")/(std::to_string(cx)+"_"+std::to_string(cz)+".chunk");
            bool exists=std::filesystem::exists(archive,error);if(error) return false;
            if(exists) {Expansion e{x,z,{}};if(!ReadChunk(archive,cx,cz,e.data,seed)) return false;expansion.push_back(std::move(e));}
        }
    }
    if(magic[6]<11) {Mode=GameMode::Creative;EquippedArmor={};ResetVitals();}
    if(magic[6]>=8) DeserializeChests(containers,true,magic[6]<9,magic[6]>=10,magic[6]>=11);else {Mode=GameMode::Creative;ClearChests();}
    ClearMapBlocks();
    RegionOriginX=originX;RegionOriginZ=originZ;
    WorldTime=loadedTime;InvalidateLighting();
    Seed=seed;
    if(width<Width) GenerateRegion();
    auto* blocks=reinterpret_cast<unsigned char*>(World);
    auto* levels=reinterpret_cast<unsigned char*>(FluidLevels);
    auto* sources=reinterpret_cast<unsigned char*>(FluidSources);
    for(int x=0;x<int(width);++x) for(int y=0;y<Height;++y) for(int z=0;z<int(width);++z) {
        size_t from=(size_t(x)*Height+y)*width+z,to=(size_t(x)*Height+y)*Width+z;
        blocks[to]=data[from];
        levels[to]=legacy?(data[from]==Water?MaxWaterLevel:0):data[cells+from];
        sources[to]=magic[6]>=3?data[cells*2+from]:0;
    }
    for(const auto& e:expansion) ApplyChunk(e.x,e.z,e.data);
    if(magic[6]<3) {
        for(int x=0;x<int(width);++x) for(int y=0;y<Height;++y) for(int z=0;z<int(width);++z)
            if(At(x,y,z)==Water&&WaterLevelAt(x,y,z)==MaxWaterLevel&&At(x,y+1,z)!=Water) FluidSources[x][y][z]=1;
    }
    if(magic[6]<6) {
        for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
            if(At(x,y,z)==Lava) {FluidLevels[x][y][z]=MaxWaterLevel;FluidSources[x][y][z]=1;}
    }
    if(player) *player=loadedPlayer;
    ResetWaterFlow();
    for(auto& version:ChunkVersions) ++version;
    return true;
}
std::vector<WorldSave> ListWorldSaves() {
    namespace fs=std::filesystem;
    std::vector<WorldSave> result;
    auto add=[&](const fs::directory_entry& entry) {
        std::error_code error;
        if(!entry.is_regular_file(error)||entry.path().extension()!=".dat") return;
        const fs::path path=entry.path();
        std::string name=path.filename()=="world.dat"&&path.parent_path().filename()!="."?
            path.parent_path().filename().string():path.stem().string();
        result.push_back({name,path.lexically_normal().string()});
    };
    std::error_code error;
    fs::directory_iterator local(".",fs::directory_options::skip_permission_denied,error),end;
    while(!error&&local!=end) {add(*local);local.increment(error);}
    error.clear();
    if(fs::is_directory("saves",error)) {
        error.clear();
        fs::recursive_directory_iterator it("saves",fs::directory_options::skip_permission_denied,error),last;
        while(!error&&it!=last) {add(*it);it.increment(error);}
    }
    std::sort(result.begin(),result.end(),[](const WorldSave& a,const WorldSave& b) {
        return a.name==b.name?a.path<b.path:a.name<b.name;
    });
    return result;
}
bool DeleteWorldSave(const std::string& path) {
    namespace fs=std::filesystem;
    const auto saves=ListWorldSaves();
    const fs::path requested=fs::path(path).lexically_normal();
    if(std::none_of(saves.begin(),saves.end(),[&](const WorldSave& save) {
        return fs::path(save.path).lexically_normal()==requested;
    })) return false;
    std::error_code error;
    if(fs::is_symlink(fs::symlink_status(requested,error))||error) return false;
    fs::path root=fs::current_path(error);if(error) return false;
    fs::path actual=fs::canonical(requested,error);if(error) return false;
    fs::path relative=actual.lexically_relative(root);
    if(relative.empty()||*relative.begin()=="..") return false;
    if(actual.parent_path()!=root&&*relative.begin()!="saves") return false;
    fs::remove_all(actual.string()+".chunks",error);if(error) return false;
    if(!fs::remove(actual,error)||error) return false;
    if(actual.parent_path()!=root&&actual.parent_path()!=root/"saves") {
        error.clear();
        if(fs::is_empty(actual.parent_path(),error)&&!error) fs::remove(actual.parent_path(),error);
    }
    return true;
}
std::string CreateWorldSavePath() {
    namespace fs=std::filesystem;
    std::error_code error;
    fs::create_directories("saves",error);
    if(error) return {};
    const auto stamp=std::chrono::system_clock::now().time_since_epoch().count();
    for(int i=0;i<100;++i) {
        fs::path directory=fs::path("saves")/("Mundo-"+std::to_string(stamp)+"-"+std::to_string(i));
        error.clear();
        if(fs::create_directory(directory,error)) return (directory/"world.dat").string();
        if(error) return {};
    }
    return {};
}
namespace {
constexpr size_t ChunkCells=size_t(ChunkSize)*Height*ChunkSize;
constexpr size_t ChunkBytes=ChunkCells*3;
struct StoredChunk {int cx,cz;std::vector<unsigned char> data;};
std::filesystem::path ChunkPath(const std::string& save,int cx,int cz) {
    return std::filesystem::path(save+".chunks")/(std::to_string(cx)+"_"+std::to_string(cz)+".chunk");
}
std::vector<unsigned char> CaptureChunk(int cx,int cz) {
    std::vector<unsigned char> result;result.reserve(ChunkBytes);
    for(int x=cx*ChunkSize;x<(cx+1)*ChunkSize;++x) for(int y=0;y<Height;++y)
        for(int z=cz*ChunkSize;z<(cz+1)*ChunkSize;++z) {
            result.push_back(World[x][y][z]);result.push_back(FluidLevels[x][y][z]);result.push_back(FluidSources[x][y][z]);
        }
    return result;
}
bool ReadChunk(const std::filesystem::path& path,int cx,int cz,std::vector<unsigned char>& data,uint32_t expectedSeed) {
    std::ifstream input(path,std::ios::binary);char magic[4]{};input.read(magic,4);
    uint32_t seed=Read32(input),storedX=Read32(input),storedZ=Read32(input),checksum=Read32(input);
    if(!input||std::memcmp(magic,"MCC1",4)||seed!=expectedSeed||storedX!=uint32_t(cx)||storedZ!=uint32_t(cz)) return false;
    data.resize(ChunkBytes);input.read(reinterpret_cast<char*>(data.data()),data.size());
    if(!input||input.peek()!=EOF||Checksum(data.data(),data.size())!=checksum) return false;
    for(size_t i=0;i<data.size();i+=3) {
        Block block=static_cast<Block>(data[i]);unsigned char level=data[i+1],source=data[i+2];
        if(!Placeable(block)||level>7||source>1||(!Liquid(block)&&(level||source))||
           (source&&level!=7)) return false;
    }
    return true;
}
bool WriteChunk(const std::filesystem::path& path,int cx,int cz,const std::vector<unsigned char>& data) {
    std::string temporary=path.string()+".tmp";
    std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    if(!out) return false;
    out.write("MCC1",4);Write32(out,Seed);Write32(out,uint32_t(cx));Write32(out,uint32_t(cz));
    Write32(out,Checksum(data.data(),data.size()));
    out.write(reinterpret_cast<const char*>(data.data()),data.size());
    out.flush();bool ok=bool(out);out.close();ok=ok&&!out.fail();
    std::error_code error;
    if(ok) {ReplaceSaveFile(temporary,path,error);ok=!error;}
    if(!ok) std::filesystem::remove(temporary,error);
    return ok;
}
void ApplyChunk(int cx,int cz,const std::vector<unsigned char>& data) {
    size_t i=0;
    for(int x=cx*ChunkSize;x<(cx+1)*ChunkSize;++x) for(int y=0;y<Height;++y)
        for(int z=cz*ChunkSize;z<(cz+1)*ChunkSize;++z) {
            World[x][y][z]=data[i++];FluidLevels[x][y][z]=data[i++];FluidSources[x][y][z]=data[i++];
        }
}
}
bool StreamWorld(PlayerState& player,const std::string& savePath,bool& shifted) {
    shifted=false;
    if(!player.valid||!std::isfinite(player.position.x)||!std::isfinite(player.position.z)) return false;
    auto displacement=[](float coordinate) {
        return coordinate<Width*.5f-24||coordinate>Width*.5f+24?int(std::floor((coordinate-(Width*.5f-16))/32.f))*32:0;
    };
    if(std::abs(player.position.x)>1000000||std::abs(player.position.z)>1000000) return false;
    int dx=displacement(player.position.x),dz=displacement(player.position.z);
    if(dx==0&&dz==0) return true;
    const int64_t newX=int64_t(RegionOriginX)+dx,newZ=int64_t(RegionOriginZ)+dz;
    if(savePath.empty()||std::abs(newX)>100000000||std::abs(newZ)>100000000) return false;
    const int oldCX=RegionOriginX/ChunkSize,oldCZ=RegionOriginZ/ChunkSize;
    const int newCX=int(newX)/ChunkSize,newCZ=int(newZ)/ChunkSize;
    std::error_code error;
    std::filesystem::create_directories(savePath+".chunks",error);if(error) return false;
    std::vector<StoredChunk> incoming;
    for(int cx=newCX;cx<newCX+ChunkCount;++cx) for(int cz=newCZ;cz<newCZ+ChunkCount;++cz) {
        if(cx>=oldCX&&cx<oldCX+ChunkCount&&cz>=oldCZ&&cz<oldCZ+ChunkCount) continue;
        auto path=ChunkPath(savePath,cx,cz);
        bool exists=std::filesystem::exists(path,error);if(error) return false;
        if(exists) {
            StoredChunk stored{cx-newCX,cz-newCZ,{}};
            if(!ReadChunk(path,cx,cz,stored.data,Seed)) return false;
            incoming.push_back(std::move(stored));
        }
    }
    for(int cx=oldCX;cx<oldCX+ChunkCount;++cx) for(int cz=oldCZ;cz<oldCZ+ChunkCount;++cz) {
        if(cx>=newCX&&cx<newCX+ChunkCount&&cz>=newCZ&&cz<newCZ+ChunkCount) continue;
        if(!WriteChunk(ChunkPath(savePath,cx,cz),cx,cz,CaptureChunk(cx-oldCX,cz-oldCZ))) return false;
    }
    constexpr size_t cells=sizeof(World);
    std::vector<unsigned char> previous(cells*3);
    std::memcpy(previous.data(),World,cells);std::memcpy(previous.data()+cells,FluidLevels,cells);
    std::memcpy(previous.data()+cells*2,FluidSources,cells);
    RegionOriginX=int32_t(newX);RegionOriginZ=int32_t(newZ);GenerateRegion();
    for(int x=0;x<Width;++x) for(int z=0;z<Width;++z) {
        int oldX=x+dx,oldZ=z+dz;
        if(oldX<0||oldX>=Width||oldZ<0||oldZ>=Width) continue;
        for(int y=0;y<Height;++y) {
            size_t index=(size_t(oldX)*Height+y)*Width+oldZ;
            World[x][y][z]=previous[index];FluidLevels[x][y][z]=previous[cells+index];
            FluidSources[x][y][z]=previous[cells*2+index];
        }
    }
    for(const auto& chunk:incoming) ApplyChunk(chunk.cx,chunk.cz,chunk.data);
    player.position.x-=dx;player.position.z-=dz;
    shifted=true;
    return SaveWorld(savePath,player);
}

Hit Trace(Vector3 origin,Vector3 direction,bool includeLiquids) {
    int cell[3]={int(std::floor(origin.x)),int(std::floor(origin.y)),int(std::floor(origin.z))};
    float o[3]={origin.x,origin.y,origin.z},d[3]={direction.x,direction.y,direction.z};
    float next[3],delta[3]; int step[3],normal[3]={0,0,0};
    for(int a=0;a<3;++a) {
        step[a]=d[a]>=0?1:-1;
        if(std::abs(d[a])<.0000001f) next[a]=delta[a]=std::numeric_limits<float>::infinity();
        else {delta[a]=std::abs(1.f/d[a]); next[a]=(cell[a]+(step[a]>0?1:0)-o[a])/d[a];}
    }
    float distance=0;
    while(distance<=7.f) {
        if(At(cell[0],cell[1],cell[2])!=Air && (includeLiquids || !Liquid(At(cell[0],cell[1],cell[2])))) {
            if(!HasMapShape(cell[0],cell[1],cell[2]))return {true,cell[0],cell[1],cell[2],normal[0],normal[1],normal[2]};
            auto boxes=MapShapeBoxes(cell[0],cell[1],cell[2]);RayCollision closest{};closest.distance=8;
            for(int i=0;i<boxes.count;++i){auto hit=GetRayCollisionBox({origin,direction},boxes.boxes[i]);if(hit.hit&&hit.distance<closest.distance)closest=hit;}
            if(closest.hit&&closest.distance<=7)return {true,cell[0],cell[1],cell[2],int(closest.normal.x),int(closest.normal.y),int(closest.normal.z)};
        }
        int axis=next[0]<next[1]?0:1; if(next[2]<next[axis]) axis=2;
        distance=next[axis]; cell[axis]+=step[axis]; next[axis]+=delta[axis];
        normal[0]=normal[1]=normal[2]=0; normal[axis]=-step[axis];
    }
    return {};
}
}
