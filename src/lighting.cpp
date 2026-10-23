#include "lighting.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include <limits>
namespace game {
namespace {
constexpr int Count=Width*Height*Width;
std::vector<unsigned char> sky(Count),lamp(Count),direct(Count);
bool dirty=true;
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
    int emission=Emission(At(source.x,source.y,source.z));
    direct[Index(source.x,source.y,source.z)]=static_cast<unsigned char>(emission);
    int r=emission-1;
    for(int x=std::max(0,source.x-r);x<=std::min(Width-1,source.x+r);++x)
        for(int y=std::max(0,source.y-r);y<=std::min(Height-1,source.y+r);++y)
            for(int z=std::max(0,source.z-r);z<=std::min(Width-1,source.z+r);++z) {
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
            if(!Inside(x,y,z)||!LightPassable(At(x,y,z))) continue;
            auto& value=light[Index(x,y,z)];
            if(value<next) {value=static_cast<unsigned char>(next);queue.push_back({x,y,z});}
        }
    }
}
}
void InvalidateLighting() {dirty=true;}
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
        open[i]=LightPassable(block)||Emission(block)>0;
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
    auto oldSky=sky,oldLamp=lamp,oldDirect=direct;
    std::fill(direct.begin(),direct.end(),0);
    std::fill(sky.begin(),sky.end(),0);std::fill(lamp.begin(),lamp.end(),0);
    std::vector<Node> skyQueue,lampQueue;
    // Direct skylight is unattenuated; only boundary cells seed lateral propagation.
    for(int x=0;x<Width;++x) for(int z=0;z<Width;++z) {
        bool open=true;
        for(int y=Height-1;y>=0;--y) {
            Block b=At(x,y,z);int id=Index(x,y,z);
            if(!LightPassable(b)) open=false;
            if(open) sky[id]=24;
            if(Emission(b)) {lamp[id]=Emission(b);lampQueue.push_back({x,y,z});}
        }
    }
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z) {
        if(sky[Index(x,y,z)]!=24) continue;
        for(const auto& d:directions) {
            int nx=x+d[0],ny=y+d[1],nz=z+d[2];
            if(Inside(nx,ny,nz)&&LightPassable(At(nx,ny,nz))&&!sky[Index(nx,ny,nz)]) {
                skyQueue.push_back({x,y,z});break;
            }
        }
    }
    for(Node source:lampQueue) CastLamp(source);
    Flood(sky,skyQueue);Flood(lamp,lampQueue);
    std::array<bool,ChunkCount*ChunkCount> changed{};
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z) {
        int i=Index(x,y,z);
        if(sky[i]==oldSky[i]&&lamp[i]==oldLamp[i]&&direct[i]==oldDirect[i]) continue;
        for(int dx=-1;dx<=1;++dx) for(int dz=-1;dz<=1;++dz) {
            int nx=x+dx,nz=z+dz;
            if(nx>=0&&nx<Width&&nz>=0&&nz<Width) changed[(nx/ChunkSize)*ChunkCount+nz/ChunkSize]=true;
        }
    }
    for(size_t i=0;i<changed.size();++i) if(changed[i]) ++ChunkVersions[i];
}
}
