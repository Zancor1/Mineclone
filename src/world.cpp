#include "world.h"
#include "lighting.h"
#include "settings.h"
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
unsigned char WaterLevels[Width][Height][Width]{};
unsigned char WaterSources[Width][Height][Width]{};
}
unsigned char WaterLevelAt(int x,int y,int z) {
    return Inside(x,y,z)&&At(x,y,z)==Water?WaterLevels[x][y][z]:0;
}
bool IsWaterSource(int x, int y, int z) {
    return At(x, y, z) == Water && WaterSources[x][y][z] != 0;
}
float WaterHeight(int x, int y, int z) {
    if (At(x, y, z) != Water) return 0.0f;
    if (At(x, y + 1, z) == Water) return 1.0f;
    return std::clamp(WaterLevelAt(x, y, z) / 7.0f, 0.0f, 1.0f);
}
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

void ResetWaterFlow() {
    waterClock = 0.0f;
}

void WriteFluidCell(int x, int y, int z, Block block,
                    unsigned char level, bool source) {
    const auto nextLevel = block == Water ? level : 0;
    const auto nextSource = block == Water && source ? 1 : 0;
    if (At(x, y, z) == block && WaterLevels[x][y][z] == nextLevel &&
        WaterSources[x][y][z] == nextSource) {
        return;
    }

    if(LightPassable(At(x,y,z))!=LightPassable(block)||Emission(At(x,y,z))!=Emission(block)) InvalidateLighting();
    World[x][y][z] = block;
    WaterLevels[x][y][z] = static_cast<unsigned char>(nextLevel);
    WaterSources[x][y][z] = static_cast<unsigned char>(nextSource);

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
unsigned DrainDirections(int x,int y,int z,unsigned char level) {
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
            if(cell!=Air&&cell!=Water) continue;
            if(below==Air||below==Water) {
                if(n.distance<best) {best=n.distance;result=0;}
                result|=1u<<first;break;
            }
            if(!Solid(below)||n.distance==radius) continue;
            for(const auto& d:Directions) queue.push_back({n.x+d[0],n.z+d[1],n.distance+1});
        }
    }
    return result;
}
struct WaterSupport {
    std::vector<unsigned char> levels=std::vector<unsigned char>(FluidCellCount,NoSupport);
};
WaterSupport SourceConnections() {
    WaterSupport support;
    struct Node {int x,y,z;unsigned char level;};
    std::deque<Node> queue;
    auto visit=[&](int x,int y,int z,unsigned char level) {
        if(At(x,y,z)!=Water||level==0) return;
        size_t id=FluidIndex(x,y,z);
        if(IsWaterSource(x,y,z)) {level=MaxWaterLevel;}
        if(support.levels[id]!=NoSupport) {
            if(support.levels[id]>level) return;
            if(support.levels[id]==level) return;
        }
        support.levels[id]=level;
        queue.push_back({x,y,z,level});
    };
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        if(IsWaterSource(x,y,z)) visit(x,y,z,MaxWaterLevel);
    while(!queue.empty()) {
        Node n=queue.front();queue.pop_front();
        size_t id=FluidIndex(n.x,n.y,n.z);
        if(support.levels[id]!=n.level||n.y==0) continue;
        Block below=At(n.x,n.y-1,n.z);
        if(below==Water) visit(n.x,n.y-1,n.z,MaxWaterLevel);
        if(!Solid(below)||n.level<=1) continue;
        unsigned drains=DrainDirections(n.x,n.y,n.z,n.level);
        for(int i=0;i<4;++i) if(drains==0||(drains&(1u<<i)))
            visit(n.x+Directions[i][0],n.y,n.z+Directions[i][1],n.level-1);
    }
    return support;
}

constexpr unsigned char HorizontalLevel(unsigned char level) {
    return level > 1 ? static_cast<unsigned char>(level - 1) : 0;
}
static_assert(HorizontalLevel(7) == 6 && HorizontalLevel(1) == 0 && HorizontalLevel(0) == 0);

std::array<bool, 4> LowestNeighbors(int x, int y, int z, unsigned char level) {
    std::array<bool, 4> chosen{};
    int bestLevel = MaxWaterLevel + 1;
    for (size_t i = 0; i < chosen.size(); ++i) {
        const int nx = x + Directions[i][0];
        const int nz = z + Directions[i][1];
        if (!Inside(nx, y, nz)) continue;
        const Block neighbor = At(nx, y, nz);
        if (neighbor != Air && neighbor != Water) continue;
        const int neighborLevel = neighbor == Air ? 0 : WaterLevelAt(nx, y, nz);
        if (neighborLevel >= HorizontalLevel(level)) continue;
        if (neighborLevel < bestLevel) {
            chosen.fill(false);
            bestLevel = neighborLevel;
        }
        if (neighborLevel == bestLevel) chosen[i] = true;
    }
    return chosen;
}

void StepWater() {
    // Compute from the current grid, then commit all results together.
    const auto support = SourceConnections();
    std::vector<unsigned char> next(FluidCellCount, 0);
    std::vector<unsigned char> incoming(FluidCellCount, 0);
    auto propose = [&](int x, int y, int z, unsigned char level) {
        if (!Inside(x, y, z) || level == 0 || level > MaxWaterLevel) return;
        const Block block = At(x, y, z);
        if (block == Air || (block == Water && WaterLevelAt(x, y, z) < level)) {
            auto& value = incoming[FluidIndex(x, y, z)];
            value = std::max(value, level);
        }
    };

    for (int x = 0; x < Width; ++x) {
        for (int y = 0; y < Height; ++y) {
            for (int z = 0; z < Width; ++z) {
                if (At(x, y, z) != Water) continue;
                const size_t id = FluidIndex(x, y, z);
                const unsigned char current = WaterLevelAt(x, y, z);
                const unsigned char target = support.levels[id] == NoSupport ? 0 : support.levels[id];
                next[id] = current > target ? static_cast<unsigned char>(current - 1) : current;
                if (WaterSources[x][y][z]) next[id] = MaxWaterLevel;
                // Disconnected remnants may drain, but cannot feed new water.
                if (target == 0 || current == 0 || y == 0) continue;
                const unsigned char level = std::min(current, target);
                const Block below = At(x, y - 1, z);
                if (below == Air || (below == Water && WaterLevelAt(x, y - 1, z) < MaxWaterLevel)) {
                    propose(x, y - 1, z, MaxWaterLevel);
                    continue;
                }
                const unsigned char lateralLevel = HorizontalLevel(level);
                if (!Solid(below) || lateralLevel == 0) continue;
                const unsigned drains=DrainDirections(x,y,z,level);
                if(drains) {
                    for(int i=0;i<4;++i) if(drains&(1u<<i))
                        propose(x+Directions[i][0],y,z+Directions[i][1],lateralLevel);
                    continue;
                }
                const auto chosen = LowestNeighbors(x, y, z, level);
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
                if (block != Air && block != Water) continue;
                const size_t id = FluidIndex(x, y, z);
                const unsigned char level = std::max(next[id], incoming[id]);
                if (level > 0) WriteFluidCell(x, y, z, Water, level, IsWaterSource(x, y, z));
                else if (block == Water) WriteFluidCell(x, y, z, Air, 0, false);
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
        StepWater();
    }
}

void UpdateWorld(float dt) {
    if(!Settings.freezeTime&&std::isfinite(dt)) WorldTime=std::fmod(WorldTime+std::max(0.f,dt),DayDuration);
    UpdateWater(dt);
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
void GenerateWorld(uint32_t seed) {
    Seed=seed;
    SetClockMinute(Settings.newWorldMinute);
    InvalidateLighting();
    std::memset(World,0,sizeof(World));
    std::memset(WaterLevels,0,sizeof(WaterLevels));
    std::memset(WaterSources,0,sizeof(WaterSources));
    for(auto& version:ChunkVersions) ++version;
    for(int x=0;x<Width;++x) for(int z=0;z<Width;++z) {
        float moisture=Fbm(x*.013f+130,z*.013f-74,3);
        float warmth=Fbm(x*.012f-218,z*.012f+83,3);
        float mountain=std::clamp((Fbm(x*.016f+19,z*.016f+31,3)+.07f)*3.f,0.f,1.f);
        float ridge=1.f-std::abs(Perlin(x*.038f+11,z*.038f+35));
        float warpX=x+12*Fbm(x*.018f,z*.018f),warpZ=z+12*Fbm(x*.018f+30,z*.018f+30);
        int h=int(25+14*Fbm(warpX*.025f,warpZ*.025f)+mountain*mountain*26*ridge);
        h=std::clamp(h,7,Height-13);
        Biome biome=mountain>.58f?Biome::Mountains:(warmth>.035f&&moisture<.055f?Biome::Desert:
            (moisture>.035f?Biome::Forest:Biome::Plains));
        Biomes[x][z]=biome; Surface[x][z]=h;
        for(int y=0;y<=h;++y) {
            Block b=Stone;
            if(y>h-4) b=(biome==Biome::Desert||h<=SeaLevel+1)?Sand:Dirt;
            if(y==h&&b==Dirt) b=Grass;
            if(biome==Biome::Mountains&&h>43&&y>h-2) b=Stone;
            if(b==Stone && y<h-4 && Hash(x,z,y)%41<3) b=CoalOre;
            World[x][y][z]=b;
        }
        for(int y=h+1;y<=SeaLevel;++y) World[x][y][z]=Water;
    }
    for(int gx=3;gx<Width-7;gx+=7) for(int gz=3;gz<Width-7;gz+=7) {
        int x=gx+int(Hash(gx,gz,10)%4),z=gz+int(Hash(gx,gz,11)%4),y=Surface[x][z];
        if(y<=SeaLevel+1) continue;
        if(Biomes[x][z]==Biome::Desert) {
            if(Hash(x,z,12)%3==0) {
                int length=2+int(Hash(x,z,13)%3);
                for(int i=1;i<=length;++i) World[x][y+i][z]=Cactus;
            }
        } else if(At(x,y,z)==Grass) {
            unsigned chance=Biomes[x][z]==Biome::Forest?85u:24u;
            if(Hash(x,z,14)%100>=chance) continue;
            int trunk=4+int(Hash(x,z,15)%3);
            for(int dy=1;dy<=trunk;++dy) World[x][y+dy][z]=Wood;
            for(int dy=-1;dy<=3;++dy) for(int dx=-3;dx<=3;++dx) for(int dz=-3;dz<=3;++dz) {
                float r=dy<0?2.3f:std::sqrt(std::max(0.f,10.f-float(dy*dy)));
                if(dx*dx+dz*dz>r*r) continue;
                int yy=y+trunk-1+dy;
                if(Inside(x+dx,yy,z+dz)&&At(x+dx,yy,z+dz)==Air) World[x+dx][yy][z+dz]=Leaves;
            }
        }
    }
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        if(At(x,y,z)==Water) {WaterLevels[x][y][z]=MaxWaterLevel;WaterSources[x][y][z]=1;}
    ResetWaterFlow();
}

void Change(int x,int y,int z,Block b) {
    if(!Inside(x,y,z)||b>=BlockCount) return;
    if(At(x,y,z)==b && (b!=Water||WaterSources[x][y][z])) return;
    WriteFluidCell(x,y,z,b,b==Water?MaxWaterLevel:0,b==Water);
}
uint32_t RandomSeed() {
    uint64_t now=uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    uint32_t seed=uint32_t(now)^uint32_t(now>>32);
    try {std::random_device rd; seed^=rd();} catch(...) {}
    return seed==Seed?seed+1:seed;
}
namespace {
constexpr unsigned char Magic[8]={'B','L','O','C','O','S',5,0};
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
    std::vector<unsigned char> data(sizeof(World)+sizeof(WaterLevels)+sizeof(WaterSources)+PoseBytes+4);
    std::memcpy(data.data(),World,sizeof(World));
    std::memcpy(data.data()+sizeof(World),WaterLevels,sizeof(WaterLevels));
    std::memcpy(data.data()+sizeof(World)+sizeof(WaterLevels),WaterSources,sizeof(WaterSources));
    const float pose[6]={player.position.x,player.position.y,player.position.z,player.yaw,player.pitch,player.velocityY};
    const size_t poseOffset=sizeof(World)+sizeof(WaterLevels)+sizeof(WaterSources);
    for(int i=0;i<6;++i) EncodeFloat(data.data()+poseOffset+i*4,pose[i]);
    EncodeFloat(data.data()+poseOffset+PoseBytes,WorldTime);
    Write32(out,Checksum(data.data(),data.size()));
    out.write(reinterpret_cast<const char*>(data.data()),data.size());
    out.flush(); bool ok=bool(out); out.close(); ok=ok&&!out.fail();
    std::error_code error;
    if(ok) {std::filesystem::rename(temporary,path,error); ok=!error;}
    if(!ok) std::filesystem::remove(temporary,error);
    return ok;
}
bool LoadWorld(const std::string& path,PlayerState* player) {
    std::ifstream in(path,std::ios::binary);
    unsigned char magic[8]{};
    in.read(reinterpret_cast<char*>(magic),8);
    if(!in||std::memcmp(magic,Magic,6)!=0||magic[7]!=0||(magic[6]<1||magic[6]>5)) return false;
    bool legacy=magic[6]==1;
    uint32_t width=Read32(in),height=Read32(in),seed=Read32(in),checksum=Read32(in);
    if(!in||width!=Width||height!=Height) return false;
    std::vector<unsigned char> data(sizeof(World)+(legacy?0:sizeof(WaterLevels))+(magic[6]>=3?sizeof(WaterSources):0)+(magic[6]>=4?PoseBytes:0)+(magic[6]>=5?4:0));
    in.read(reinterpret_cast<char*>(data.data()),data.size());
    if(!in||in.peek()!=EOF||Checksum(data.data(),data.size())!=checksum) return false;
    for(size_t i=0;i<sizeof(World);++i) {
        if(data[i]>=BlockCount) return false;
        if(!legacy) {
            unsigned char level=data[sizeof(World)+i];
            if(level>MaxWaterLevel||(data[i]!=Water&&level!=0)) return false;
            if(magic[6]>=3) {
                unsigned char source=data[sizeof(World)+sizeof(WaterLevels)+i];
                if(source>1||(source&&(data[i]!=Water||level!=MaxWaterLevel))) return false;
            }
        }
    }
    PlayerState loadedPlayer;
    if(magic[6]>=4) {
        size_t offset=sizeof(World)+sizeof(WaterLevels)+sizeof(WaterSources);
        loadedPlayer.position={DecodeFloat(data.data()+offset),DecodeFloat(data.data()+offset+4),DecodeFloat(data.data()+offset+8)};
        loadedPlayer.yaw=DecodeFloat(data.data()+offset+12);loadedPlayer.pitch=DecodeFloat(data.data()+offset+16);
        loadedPlayer.velocityY=DecodeFloat(data.data()+offset+20);loadedPlayer.valid=true;
        if(!ValidPlayerState(loadedPlayer)) return false;
    }
    float loadedTime=DayDuration*.25f;
    if(magic[6]>=5) {
        loadedTime=DecodeFloat(data.data()+sizeof(World)+sizeof(WaterLevels)+sizeof(WaterSources)+PoseBytes);
        if(!std::isfinite(loadedTime)||loadedTime<0||loadedTime>=DayDuration) return false;
    }
    WorldTime=loadedTime;InvalidateLighting();
    std::memcpy(World,data.data(),sizeof(World)); Seed=seed;
    if(legacy) {
        auto* levels=reinterpret_cast<unsigned char*>(WaterLevels);
        for(size_t i=0;i<sizeof(World);++i) levels[i]=data[i]==Water?MaxWaterLevel:0;
    } else std::memcpy(WaterLevels,data.data()+sizeof(World),sizeof(WaterLevels));
    if(magic[6]>=3) std::memcpy(WaterSources,data.data()+sizeof(World)+sizeof(WaterLevels),sizeof(WaterSources));
    else {
        std::memset(WaterSources,0,sizeof(WaterSources));
        for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
            if(At(x,y,z)==Water&&WaterLevelAt(x,y,z)==MaxWaterLevel&&At(x,y+1,z)!=Water) WaterSources[x][y][z]=1;
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
Hit Trace(Vector3 origin,Vector3 direction,bool includeWater) {
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
        if(At(cell[0],cell[1],cell[2])!=Air && (includeWater || At(cell[0],cell[1],cell[2])!=Water)) return {true,cell[0],cell[1],cell[2],normal[0],normal[1],normal[2]};
        int axis=next[0]<next[1]?0:1; if(next[2]<next[axis]) axis=2;
        distance=next[axis]; cell[axis]+=step[axis]; next[axis]+=delta[axis];
        normal[0]=normal[1]=normal[2]=0; normal[axis]=-step[axis];
    }
    return {};
}
}
