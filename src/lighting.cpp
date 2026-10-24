#include "map_blocks.h"
#include "lighting.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include <limits>
#include <set>
#include <chrono>
#include <cstring>
namespace game {
namespace {
constexpr int Count=Width*Height*Width;
std::vector<unsigned char> sky(Count),lamp(Count),direct(Count);
bool dirty=true,fullDirty=true;
int editMinX=Width,editMaxX=-1,editMinZ=Width,editMaxZ=-1;
std::set<int> emitters;
int Index(int x,int y,int z) {return (x*Height+y)*Width+z;}
struct Node {int x,y,z;};
constexpr int directions[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
struct LightingWork {
    bool active=false,full=false,columnOpen=true,casting=false;
    int minX=0,maxX=0,minZ=0,maxZ=0,spanZ=0;
    int editMinX=0,editMaxX=0,editMinZ=0,editMaxZ=0;
    int stage=0,castX=0,castY=0,castZ=0,castMaxX=0,castMaxY=0,castMaxZ=0,castMinY=0,castMinZ=0,emission=0;
    size_t cursor=0,source=0,head=0,cells=0;
    std::vector<unsigned char> sky,lamp,direct,passable;
    std::vector<Node> skyQueue,lampQueue,sources;
    std::set<int> emitters;
    bool Contains(int x,int z) const {return x>=minX&&x<=maxX&&z>=minZ&&z<=maxZ;}
    int Offset(int x,int y,int z) const {return ((x-minX)*Height+y)*spanZ+z-minZ;}
    Node Cell(size_t i) const {return {minX+int(i/(Height*spanZ)),int(i/spanZ)%Height,minZ+int(i%spanZ)};}
} work;
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
            if(!LightPassableAt(p[0],p[1],p[2])) return false;
        }
        for(int a=0;a<3;++a) if(next[a]<=t+1e-6f) {cell[a]+=step[a];next[a]+=stride[a];}
        if(!LightPassableAt(cell[0],cell[1],cell[2])) return false;
    }
    return true;
}
void BeginWork(){
    work.full=fullDirty;
    work.editMinX=editMinX;work.editMaxX=editMaxX;work.editMinZ=editMinZ;work.editMaxZ=editMaxZ;
    work.minX=fullDirty?0:std::max(0,editMinX-24);work.maxX=fullDirty?Width-1:std::min(Width-1,editMaxX+24);
    work.minZ=fullDirty?0:std::max(0,editMinZ-24);work.maxZ=fullDirty?Width-1:std::min(Width-1,editMaxZ+24);
    work.spanZ=work.maxZ-work.minZ+1;work.cells=size_t(work.maxX-work.minX+1)*Height*work.spanZ;
    work.sky.resize(work.cells);work.lamp.resize(work.cells);work.direct.resize(work.cells);work.passable.resize(work.cells);
    work.skyQueue.clear();work.lampQueue.clear();work.sources.clear();work.emitters=fullDirty?std::set<int>{}:emitters;
    for(auto it=work.emitters.begin();it!=work.emitters.end();){int id=*it;if(work.Contains(id/(Width*Height),id%Width))it=work.emitters.erase(it);else ++it;}
    work.skyQueue.reserve(std::min(work.cells,size_t(1000000)));work.lampQueue.reserve(std::min(work.cells,size_t(1000000)));
    work.active=true;work.stage=0;work.cursor=work.source=work.head=0;work.casting=false;
    dirty=fullDirty=false;editMinX=editMinZ=Width;editMaxX=editMaxZ=-1;
}
void PublishWork(){
    std::array<bool,Width*Width> columns{};
    std::array<bool,ChunkCount*ChunkCount> changed{};
    for(int x=work.minX;x<=work.maxX;++x)for(int y=0;y<Height;++y){
        int global=Index(x,y,work.minZ),local=work.Offset(x,y,work.minZ);
        for(int z=0;z<work.spanZ;++z)if(sky[global+z]!=work.sky[local+z]||lamp[global+z]!=work.lamp[local+z]||direct[global+z]!=work.direct[local+z])columns[x*Width+work.minZ+z]=true;
        std::memcpy(sky.data()+global,work.sky.data()+local,work.spanZ);
        std::memcpy(lamp.data()+global,work.lamp.data()+local,work.spanZ);
        std::memcpy(direct.data()+global,work.direct.data()+local,work.spanZ);
    }
    for(int x=work.minX;x<=work.maxX;++x)for(int z=work.minZ;z<=work.maxZ;++z)if(columns[x*Width+z]){
        for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz){int nx=x+dx,nz=z+dz;if(nx>=0&&nx<Width&&nz>=0&&nz<Width)changed[(nx/ChunkSize)*ChunkCount+nz/ChunkSize]=true;}
    }
    emitters.swap(work.emitters);
    for(size_t i=0;i<changed.size();++i)if(changed[i])++ChunkVersions[i];
    work.active=false;
}
void WorkStep(){
    if(work.stage==0){
        if(work.cursor==work.cells){work.cursor=0;++work.stage;return;}
        size_t column=work.cursor/Height;Node n{work.minX+int(column/work.spanZ),Height-1-int(work.cursor%Height),work.minZ+int(column%work.spanZ)};++work.cursor;
        if(n.y==Height-1)work.columnOpen=true;
        int i=work.Offset(n.x,n.y,n.z),global=Index(n.x,n.y,n.z),emission=BlockEmissionAt(n.x,n.y,n.z);
        bool passable=LightPassableAt(n.x,n.y,n.z);work.passable[i]=passable;if(!passable)work.columnOpen=false;
        work.sky[i]=work.columnOpen?24:0;work.lamp[i]=static_cast<unsigned char>(emission);work.direct[i]=0;
        if(emission){work.lampQueue.push_back(n);work.emitters.insert(global);}
        return;
    }
    if(work.stage==1){
        if(work.cursor==work.cells){work.cursor=0;++work.stage;for(int id:work.emitters)work.sources.push_back({id/(Width*Height),(id/Width)%Height,id%Width});return;}
        auto n=work.Cell(work.cursor++);int i=work.Offset(n.x,n.y,n.z);if(work.sky[i]==24||!work.passable[i])return;
        for(const auto& d:directions){int x=n.x+d[0],y=n.y+d[1],z=n.z+d[2];if(!Inside(x,y,z))continue;int light=work.Contains(x,z)?work.sky[work.Offset(x,y,z)]:sky[Index(x,y,z)];if(light==24){work.sky[i]=23;work.skyQueue.push_back(n);break;}}
        return;
    }
    if(work.stage==2){
        if(work.source==work.sources.size()){++work.stage;return;}
        auto source=work.sources[work.source];
        if(!work.casting){
            work.emission=BlockEmissionAt(source.x,source.y,source.z);int r=work.emission-1;
            if(work.Contains(source.x,source.z))work.direct[work.Offset(source.x,source.y,source.z)]=static_cast<unsigned char>(work.emission);
            if(source.x+r<work.minX||source.x-r>work.maxX||source.z+r<work.minZ||source.z-r>work.maxZ){++work.source;return;}
            work.castX=std::max(work.minX,source.x-r);work.castMaxX=std::min(work.maxX,source.x+r);
            work.castY=work.castMinY=std::max(0,source.y-r);work.castMaxY=std::min(Height-1,source.y+r);
            work.castZ=work.castMinZ=std::max(work.minZ,source.z-r);work.castMaxZ=std::min(work.maxZ,source.z+r);work.casting=true;
        }
        int x=work.castX,y=work.castY,z=work.castZ,i=work.Offset(x,y,z);
        int strength=work.emission-std::abs(x-source.x)-std::abs(y-source.y)-std::abs(z-source.z);
        if(strength>0&&work.passable[i]&&strength>work.direct[i]&&Visible(source,{x,y,z}))work.direct[i]=static_cast<unsigned char>(strength);
        if(++work.castZ>work.castMaxZ){work.castZ=work.castMinZ;if(++work.castY>work.castMaxY){work.castY=work.castMinY;if(++work.castX>work.castMaxX){work.casting=false;++work.source;}}}
        return;
    }
    if(work.stage==3){
        if(work.cursor==work.cells){work.head=0;++work.stage;return;}
        auto n=work.Cell(work.cursor++);if(n.x!=work.minX&&n.x!=work.maxX&&n.z!=work.minZ&&n.z!=work.maxZ)return;
        int i=work.Offset(n.x,n.y,n.z);if(!work.passable[i])return;
        for(const auto& d:directions){int x=n.x+d[0],y=n.y+d[1],z=n.z+d[2];if(!Inside(x,y,z)||work.Contains(x,z))continue;int outside=Index(x,y,z);
            if(sky[outside]>work.sky[i]+1){work.sky[i]=sky[outside]-1;work.skyQueue.push_back(n);}
            if(lamp[outside]>work.lamp[i]+1){work.lamp[i]=lamp[outside]-1;work.lampQueue.push_back(n);}
        }
        return;
    }
    if(work.stage==4||work.stage==5){
        auto& queue=work.stage==4?work.skyQueue:work.lampQueue;auto& light=work.stage==4?work.sky:work.lamp;
        if(work.head==queue.size()){work.head=0;++work.stage;return;}
        Node n=queue[work.head++];int next=int(light[work.Offset(n.x,n.y,n.z)])-1;if(next<=0)return;
        for(const auto& d:directions){int x=n.x+d[0],y=n.y+d[1],z=n.z+d[2];if(!Inside(x,y,z)||!work.Contains(x,z))continue;int i=work.Offset(x,y,z);if(work.passable[i]&&light[i]<next){light[i]=static_cast<unsigned char>(next);queue.push_back({x,y,z});}}
        return;
    }
    PublishWork();
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

        open[i]=LightPassableAt(cells[i][0],cells[i][1],cells[i][2])||BlockEmissionAt(cells[i][0],cells[i][1],cells[i][2])>0;
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
float LightingProgress(){
    if(!LightingPending())return 1.f;
    if(!work.active||dirty)return 0.f;
    float cells=std::max(1.f,float(work.cells));
    switch(work.stage){case 0:return .35f*work.cursor/cells;case 1:return .35f+.15f*work.cursor/cells;
    case 2:return .5f+.15f*work.source/std::max(1.f,float(work.sources.size()));case 3:return .65f+.15f*work.cursor/cells;
    case 4:return .8f+.1f*work.head/std::max(1.f,float(work.skyQueue.size()));case 5:return .9f+.09f*work.head/std::max(1.f,float(work.lampQueue.size()));default:return .99f;}
}
bool LightingPending(){return dirty||work.active;}
bool LightingRequiresFullUpdate(){return fullDirty||(work.active&&work.full);}
void UpdateLighting(float budgetMilliseconds) {
    using Clock=std::chrono::steady_clock;
    auto start=Clock::now();
    if(dirty&&work.active){
        fullDirty=fullDirty||work.full;
        if(!fullDirty){editMinX=std::min(editMinX,work.editMinX);editMaxX=std::max(editMaxX,work.editMaxX);editMinZ=std::min(editMinZ,work.editMinZ);editMaxZ=std::max(editMaxZ,work.editMaxZ);}
        work.active=false;
    }
    if(!work.active&&dirty)BeginWork();
    unsigned steps=0;
    while(work.active){
        if(budgetMilliseconds>0&&(steps++%64)==0&&std::chrono::duration<float,std::milli>(Clock::now()-start).count()>=budgetMilliseconds)return;
        WorkStep();
    }
}
}
