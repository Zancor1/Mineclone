#include "map_blocks.h"
#include "lighting.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include <limits>
#include <set>
namespace game {
namespace {
constexpr int Count=Width*Height*Width;
std::vector<unsigned char> sky(Count),lamp(Count),direct(Count);
bool dirty=true,fullDirty=true;
int editMinX=Width,editMaxX=-1,editMinZ=Width,editMaxZ=-1;
int minX=0,maxX=Width-1,minZ=0,maxZ=Width-1;
std::set<int> emitters;
bool InRegion(int x,int z){return x>=minX&&x<=maxX&&z>=minZ&&z<=maxZ;}
int Index(int x,int y,int z) {return (x*Height+y)*Width+z;}
struct Node {int x,y,z;};
constexpr int directions[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
bool Visible(Node from,Node to) {
    int cell[3]={from.x,from.y,from.z},target[3]={to.x,to.y,to.z};
    int delta[3]={to.x-from.x,to.y-from.y,to.z-from.z},step[3];
    float next[3],stride[3];
    for(int a=0;a<3;++a) {
        step[a]=delta[a]>0?1:-1;
        stride[a]=delta[a]?1.f/std::abs(float(delta[a])):std::numeric_limits<float>::infinity();
        next[a]=stride[a]*.5f;
    }
    while(cell[0]!=target[0]||cell[1]!=target[1]||cell[2]!=target[2]) {
        float t=std::min({next[0],next[1],next[2]});
        // Conservative corner crossings prevent light leaking through touching walls.
        for(int a=0;a<3;++a) if(next[a]<=t+1e-6f) {
            int p[3]={cell[0],cell[1],cell[2]};p[a]+=step[a];
            if(!LightPassable(At(p[0],p[1],p[2]))) return false;
        }
        for(int a=0;a<3;++a) if(next[a]<=t+1e-6f) {cell[a]+=step[a];next[a]+=stride[a];}
        if(!LightPassable(At(cell[0],cell[1],cell[2]))) return false;
    }
    return true;
}
void CastLamp(Node source) {
    int emission=BlockEmissionAt(source.x,source.y,source.z);
    if(InRegion(source.x,source.z))direct[Index(source.x,source.y,source.z)]=static_cast<unsigned char>(emission);
    int r=emission-1;
    if(source.x+r<minX||source.x-r>maxX||source.z+r<minZ||source.z-r>maxZ)return;
    for(int x=std::max(minX,source.x-r);x<=std::min(maxX,source.x+r);++x)
        for(int y=std::max(0,source.y-r);y<=std::min(Height-1,source.y+r);++y)
            for(int z=std::max(minZ,source.z-r);z<=std::min(maxZ,source.z+r);++z) {
                int strength=emission-std::abs(x-source.x)-std::abs(y-source.y)-std::abs(z-source.z);
                if(strength<=0||!LightPassable(At(x,y,z))) continue;
                auto& value=direct[Index(x,y,z)];
                if(strength>value&&Visible(source,{x,y,z})) value=static_cast<unsigned char>(strength);
            }
}
void Flood(std::vector<unsigned char>& light,std::vector<Node>& queue) {
    for(size_t head=0;head<queue.size();++head) {
        const Node n=queue[head];
        const int next=int(light[Index(n.x,n.y,n.z)])-1;
        if(next<=0) continue;
        for(const auto& d:directions) {
            int x=n.x+d[0],y=n.y+d[1],z=n.z+d[2];
            if(!Inside(x,y,z)||!InRegion(x,z)||!LightPassable(At(x,y,z))) continue;
            auto& value=light[Index(x,y,z)];
            if(value<next) {value=static_cast<unsigned char>(next);queue.push_back({x,y,z});}
        }
    }
}
}
void InvalidateLighting() {dirty=fullDirty=true;}
void InvalidateLighting(int x,int,int z) {
    dirty=true;editMinX=std::min(editMinX,x);editMaxX=std::max(editMaxX,x);editMinZ=std::min(editMinZ,z);editMaxZ=std::max(editMaxZ,z);
}
unsigned char SkyLightAt(int x,int y,int z) {
    return Inside(x,y,z)?sky[Index(x,y,z)]:24;
}
unsigned char BlockLightAt(int x,int y,int z) {
    return Inside(x,y,z)?lamp[Index(x,y,z)]:0;
}
unsigned char DirectBlockLightAt(int x,int y,int z) {
    return Inside(x,y,z)?direct[Index(x,y,z)]:0;
}
VertexLight SampleCornerLighting(int x,int y,int z,int normalAxis,int sideA,int sideB) {
    VertexLight result;
    if(normalAxis<0||normalAxis>2) return result;
    const int a=(normalAxis+1)%3,b=(normalAxis+2)%3;
    int cells[4][3]={{x,y,z},{x,y,z},{x,y,z},{x,y,z}};
    cells[1][a]+=sideA;cells[2][b]+=sideB;
    cells[3][a]+=sideA;cells[3][b]+=sideB;
    bool open[4];
    for(int i=0;i<4;++i) {
        Block block=At(cells[i][0],cells[i][1],cells[i][2]);
        open[i]=LightPassable(block)||BlockEmissionAt(cells[i][0],cells[i][1],cells[i][2])>0;
    }
    if(!open[0]) return result;
    // Never gather diagonal light across two touching opaque side cells.
    if(!open[1]&&!open[2]) open[3]=false;
    int count=0;
    for(int i=0;i<4;++i) {
        if(!open[i]) continue;
        int cx=cells[i][0],cy=cells[i][1],cz=cells[i][2];
        result.sky+=SkyLightAt(cx,cy,cz)/24.f;
        result.block+=(.72f*DirectBlockLightAt(cx,cy,cz)+.28f*BlockLightAt(cx,cy,cz))/24.f;
        ++count;
    }
    if(count) {result.sky/=count;result.block/=count;}
    result.ambient=!open[1]&&!open[2]?.72f:1.f-.075f*(int(!open[1])+int(!open[2])+int(!open[3]));
    return result;
}
void UpdateLighting() {
    if(!dirty) return;
    dirty=false;
    minX=fullDirty?0:std::max(0,editMinX-24);maxX=fullDirty?Width-1:std::min(Width-1,editMaxX+24);
    minZ=fullDirty?0:std::max(0,editMinZ-24);maxZ=fullDirty?Width-1:std::min(Width-1,editMaxZ+24);
    if(fullDirty)emitters.clear();
    fullDirty=false;editMinX=editMinZ=Width;editMaxX=editMaxZ=-1;
    std::vector<unsigned char> oldSky,oldLamp,oldDirect;
    size_t cells=size_t(maxX-minX+1)*Height*(maxZ-minZ+1);oldSky.reserve(cells);oldLamp.reserve(cells);oldDirect.reserve(cells);
    for(int x=minX;x<=maxX;++x)for(int y=0;y<Height;++y)for(int z=minZ;z<=maxZ;++z){int i=Index(x,y,z);oldSky.push_back(sky[i]);oldLamp.push_back(lamp[i]);oldDirect.push_back(direct[i]);sky[i]=lamp[i]=direct[i]=0;}
    std::vector<Node> skyQueue,lampQueue;
    // Direct skylight is unattenuated; only boundary cells seed lateral propagation.
    for(int x=minX;x<=maxX;++x) for(int z=minZ;z<=maxZ;++z) {
        bool open=true;
        for(int y=Height-1;y>=0;--y) {
            Block b=At(x,y,z);int id=Index(x,y,z);
            if(!LightPassable(b)) open=false;
            if(open) sky[id]=24;
            if(BlockEmissionAt(x,y,z)) {lamp[id]=BlockEmissionAt(x,y,z);lampQueue.push_back({x,y,z});emitters.insert(id);}else emitters.erase(id);
        }
    }
    for(int x=minX;x<=maxX;++x) for(int y=0;y<Height;++y) for(int z=minZ;z<=maxZ;++z) {
        int id=Index(x,y,z);
        if(sky[id]==24||!LightPassable(At(x,y,z)))continue;
        for(const auto& d:directions){int nx=x+d[0],ny=y+d[1],nz=z+d[2];
            if(Inside(nx,ny,nz)&&sky[Index(nx,ny,nz)]==24){sky[id]=23;skyQueue.push_back({x,y,z});break;}
        }
    }
    for(int id:emitters){int z=id%Width,y=(id/Width)%Height,x=id/(Width*Height);CastLamp({x,y,z});}
    for(int x=minX;x<=maxX;++x)for(int y=0;y<Height;++y)for(int z=minZ;z<=maxZ;++z){
        if(x!=minX&&x!=maxX&&z!=minZ&&z!=maxZ)continue;
        if(!LightPassable(At(x,y,z)))continue;
        for(const auto& d:directions){int nx=x+d[0],ny=y+d[1],nz=z+d[2];if(!Inside(nx,ny,nz)||InRegion(nx,nz))continue;
            int id=Index(x,y,z),outside=Index(nx,ny,nz);
            if(sky[outside]>sky[id]+1){sky[id]=sky[outside]-1;skyQueue.push_back({x,y,z});}
            if(lamp[outside]>lamp[id]+1){lamp[id]=lamp[outside]-1;lampQueue.push_back({x,y,z});}
        }
    }
    Flood(sky,skyQueue);Flood(lamp,lampQueue);
    std::array<bool,ChunkCount*ChunkCount> changed{};
    size_t previous=0;
    for(int x=minX;x<=maxX;++x) for(int y=0;y<Height;++y) for(int z=minZ;z<=maxZ;++z) {
        int i=Index(x,y,z);size_t old=previous++;
        if(sky[i]==oldSky[old]&&lamp[i]==oldLamp[old]&&direct[i]==oldDirect[old]) continue;
        for(int dx=-1;dx<=1;++dx) for(int dz=-1;dz<=1;++dz) {
            int nx=x+dx,nz=z+dz;
            if(nx>=0&&nx<Width&&nz>=0&&nz<Width) changed[(nx/ChunkSize)*ChunkCount+nz/ChunkSize]=true;
        }
    }
    for(size_t i=0;i<changed.size();++i) if(changed[i]) ++ChunkVersions[i];
}
}
