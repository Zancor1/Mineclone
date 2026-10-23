#include "renderer.h"
#include "lighting.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
namespace game {
int BlockTile(Block b,int f) {
    switch(b) {
        case Grass:return f==2?0:(f==3?1:6);
        case Stone:return 2;
        case Wood:return f==2||f==3?4:3;
        case Leaves:return 5;
        case Sand:return 7;
        case Water:return 8;
        case Glass:return 9;
        case Cactus:return f==2||f==3?11:10;
        case Bricks:return 12;
        case CoalOre:return 13;
        case WhiteWool:return 14;
        case Planks:return 15;
        default:return b>=Cobblestone?16+(int(b)-int(Cobblestone)):1;
    }
}
namespace {
constexpr float DrawDistance=116.f;
const Color Palette[]={{0,0,0,0},{100,153,56,255},{133,93,62,255},{134,134,131,255},
    {112,82,47,255},{64,117,44,255},{220,204,146,255},{58,121,196,145},{173,214,228,70},
    {51,129,58,255},{164,78,58,255},{134,134,131,255},{229,228,220,255},{173,135,79,255},
    {110,111,108,255},{130,131,128,255},{144,146,141,255},{45,85,57,255},
    {185,105,56,255},{67,46,29,255},{255,213,113,255},{91,72,47,255},
    {176,43,39,255},{64,132,53,255},{52,78,166,255},{227,191,46,255},
    {130,122,111,255},{155,165,177,255},{239,246,248,255},{101,190,80,160},
    {237,92,22,220},{52,54,52,255},{79,142,47,255},{125,88,44,255},
    {49,120,62,255},{172,85,187,255},{115,162,221,255},{211,53,46,255},
    {239,127,165,255},{190,34,49,255},{242,208,44,255},
    {234,126,40,255},{183,74,180,255},{101,175,215,255},{128,190,45,255},
    {222,137,164,255},{72,76,79,255},{163,169,169,255},{32,143,154,255},
    {124,60,174,255},{115,78,48,255},{30,30,35,255}};
static_assert(sizeof(Palette)/sizeof(*Palette)==BlockCount,"Every block needs an atlas color");
Color Shade(Color c,float light) {
    return {static_cast<unsigned char>(std::clamp(c.r*light,0.f,255.f)),
        static_cast<unsigned char>(std::clamp(c.g*light,0.f,255.f)),
        static_cast<unsigned char>(std::clamp(c.b*light,0.f,255.f)),c.a};
}
Texture2D MakeAtlas() {
    Image image=GenImageColor(TileSize*TileCount,TileSize,BLANK);
    for(int tile=0;tile<TileCount;++tile) for(int y=0;y<TileSize;++y) for(int x=0;x<TileSize;++x) {
        const Block types[16]={Grass,Dirt,Stone,Wood,Wood,Leaves,Dirt,Sand,Water,Glass,Cactus,Cactus,Bricks,CoalOre,WhiteWool,Planks};
        Block type=tile<16?types[tile]:static_cast<Block>(int(Cobblestone)+tile-16);
        Color c=Palette[type];
        float variation=.85f+float(Hash(x,y,tile)%31)*.01f;
        if(tile==6&&y<3+int(Hash(x,0,60)%3)) c=Palette[Grass];
        if(tile==2&&((x+int(Hash(0,y,2)%4))%11==0)) variation*=.78f;
        if(tile==3&&(x%5==0||(x%5==1&&y%8<5))) variation*=.64f;
        if(tile==4) variation=std::max(std::abs(x-7),std::abs(y-7))%3==0?.7f:1.2f;
        if(tile==5&&Hash(x,y,5)%7<2) variation*=.65f;
        if(tile==8) { variation=.97f+.09f*std::sin(x*.8f+y*.6f); c.a=145; }
        if(tile==9) {
            bool edge=x==0||x==15||y==0||y==15;
            bool glint=(x-y==3||x-y==4)&&x>4&&x<12;
            c=edge?Color{143,192,205,200}:(glint?Color{226,247,255,145}:Color{180,222,235,38});
            variation=1;
        }
        if(tile==10) {
            variation=x%4==0?.64f:1.1f;
            if(x%4==2&&y%5==1) c={204,205,126,255};
        }
        if(tile==11) variation=std::max(std::abs(x-7),std::abs(y-7))>5?.7f:1.25f;
        if(tile==12) {
            bool mortar=y%5==0 || (x+(y/5%2)*8)%8==0;
            if(mortar) {c={174,165,148,255};variation=1;}
        }
        if(tile==13 && Hash(x/2,y/2,133)%5==0) {c={38,37,35,255};variation=.8f;}
        if(Wool(type)) {
            variation=.92f+float(Hash(x,y,144)%9)*.01f;
            if((x+y)%4==0) variation*=.96f;
        }
        if(tile==15) {
            bool seam=y%4==0 || (x+(y/4%2)*7)%16==0;
            if(seam) variation*=.62f;
            else if(y%4==2) variation*=.9f;
        }
        if(type==LampOn||type==LampOff) {
            bool frame=x<2||y<2||x>13||y>13||x==y||x+y==15;
            if(frame) c={76,59,37,255};
            else if(type==LampOn) c={255,228,142,255};
        }
        if(type==StoneBricks||type==CarvedStone||type==Cobblestone) {
            int size=type==Cobblestone?5:8;
            if(y%size==0||(x+(y/size%2)*4)%size==0) variation*=.55f;
        }
        if(type==AcaciaPlanks||type==DarkOakPlanks) if(y%4==0) variation*=.6f;
        if(type==Bedrock) variation=.4f+float(Hash(x/2,y/2,200)%70)*.01f;
        if(Plant(type)) {
            bool stem=(x==7||x==8)&&y>5;
            bool leaf=(y==10&&x>=3&&x<=12)||(y==12&&x>=5&&x<=10);
            bool crown=(x-8)*(x-8)+(y-4)*(y-4)<14;
            if(type==TallGrass||type==DeadBush||type==Kelp) {
                if(!(stem||leaf||(x==y/2+2&&y>4)||(x==14-y/2&&y>4))) c=BLANK;
            } else if(stem||leaf) c={54,128,39,255};
            else if(!crown) c=BLANK;
        }
        ImageDrawPixel(&image,tile*TileSize+x,y,Shade(c,variation));
    }
    Texture2D texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(texture,TEXTURE_FILTER_POINT); SetTextureWrap(texture,TEXTURE_WRAP_CLAMP);
    return texture;
}
const int Normals[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
// Counterclockwise winding viewed from outside.
const float Corners[6][4][3]={
    {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
    {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
    {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}
};
}

WaterFace BuildFluidFace(int x, int y, int z, int direction) {
    WaterFace face;
    if (direction < 0 || direction >= 6 || !Liquid(At(x,y,z))) return face;
    const Block fluid=At(x,y,z);
    const float height = FluidHeight(x, y, z);
    if (height <= 0.0f) return face;
    const int nx = x + Normals[direction][0];
    const int ny = y + Normals[direction][1];
    const int nz = z + Normals[direction][2];
    const Block neighbor = At(nx, ny, nz);
    float bottom = 0.0f;

    if (direction == 2) {
        if (neighbor == fluid || (Solid(neighbor) && height >= 1.0f)) return face;
    } else if (direction == 3) {
        if (Solid(neighbor) || (neighbor == fluid && FluidHeight(nx, ny, nz) >= 1.0f)) return face;
    } else if (neighbor == fluid) {
        bottom = FluidHeight(nx, ny, nz);
        if (bottom >= height) return face;
    } else if (neighbor != Air) {
        if(!Liquid(neighbor)||fluid<neighbor) return face;
    }

    face.normal = {float(Normals[direction][0]), float(Normals[direction][1]), float(Normals[direction][2])};
    for (int i = 0; i < 4; ++i) {
        const float localY = Corners[direction][i][1] > 0 ? height : bottom;
        face.vertices[i] = {x + Corners[direction][i][0], y + localY, z + Corners[direction][i][2]};
    }
    face.visible = true;
    return face;
}

WaterFace BuildWaterFace(int x,int y,int z,int direction) {
    return At(x,y,z)==Water?BuildFluidFace(x,y,z,direction):WaterFace{};
}

namespace {
struct Face { int x,y,z,direction; Block block; };
Vector3 Center(const Face& f) {
    if(Liquid(f.block)) {
        const WaterFace face=BuildFluidFace(f.x,f.y,f.z,f.direction);
        Vector3 center{};
        for(Vector3 vertex:face.vertices) center=Vector3Add(center,vertex);
        return Vector3Scale(center,.25f);
    }
    return {f.x+.5f+.5f*Normals[f.direction][0],
            f.y+.5f+.5f*Normals[f.direction][1],
            f.z+.5f+.5f*Normals[f.direction][2]};
}
struct Geometry {
    std::vector<float> positions,uvs,normals;
    std::vector<unsigned char> colors;
    void Clear() { positions.clear(); uvs.clear(); normals.clear(); colors.clear(); }
    int Count() const {return int(positions.size()/3);}
    void Add(const Face& f) {
        int tile=BlockTile(f.block,f.direction);
        WaterFace water;
        if(Liquid(f.block)) {
            water=BuildFluidFace(f.x,f.y,f.z,f.direction);
            if(!water.visible) return;
        }
        Vector3 n{float(Normals[f.direction][0]),float(Normals[f.direction][1]),float(Normals[f.direction][2])};
        const int lx=f.x+Normals[f.direction][0],ly=f.y+Normals[f.direction][1],lz=f.z+Normals[f.direction][2];
        int normalAxis=Normals[f.direction][0]?0:(Normals[f.direction][1]?1:2);
        int tangentA=(normalAxis+1)%3,tangentB=(normalAxis+2)%3;
        Color vertexColors[4];
        for(int i=0;i<4;++i) {
            int sa=Corners[f.direction][i][tangentA]>.5f?1:-1;
            int sb=Corners[f.direction][i][tangentB]>.5f?1:-1;
            VertexLight light=SampleCornerLighting(lx,ly,lz,normalAxis,sa,sb);
            if(Emission(f.block)) {light.block=Emission(f.block)/24.f;light.ambient=1;}
            if(Liquid(f.block)||Plant(f.block)) light.ambient=1;
            auto channel=[](float value) {return static_cast<unsigned char>(std::clamp(value,0.f,1.f)*255+.5f);};
            vertexColors[i]={channel(light.ambient),channel(light.sky),channel(light.block),255};
        }
        constexpr int normalOrder[6]={0,1,2,0,2,3},flippedOrder[6]={0,1,3,1,2,3};
        auto brightness=[&](int i) {return vertexColors[i].r*(vertexColors[i].g+vertexColors[i].b+1);};
        const int* order=brightness(0)+brightness(2)>brightness(1)+brightness(3)?flippedOrder:normalOrder;
        constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
        for(int v=0;v<6;++v) {
            const int index=order[v];
            float px=f.x+Corners[f.direction][index][0];
            float pz=f.z+Corners[f.direction][index][2];
            float py=Corners[f.direction][index][1];
            Vector3 vertex=Liquid(f.block)?water.vertices[index]:Vector3{px,f.y+py,pz};
            if(Plant(f.block)) {
                float t=Corners[f.direction][index][0];
                // Two crossed planes, submitted on faces +Z and -Z.
                if(f.direction==4) vertex={f.x+t,f.y+py,f.z+t};
                else vertex={f.x+t,f.y+py,f.z+1-t};
            }
            positions.insert(positions.end(),{vertex.x,vertex.y,vertex.z});
            normals.insert(normals.end(),{n.x,n.y,n.z});
            uvs.push_back((tile*TileSize+.5f+uv[index][0]*(TileSize-1))/(TileSize*TileCount));
            uvs.push_back((.5f+uv[index][1]*(TileSize-1))/TileSize);
            Color color=vertexColors[index];
            colors.insert(colors.end(),{color.r,color.g,color.b,255});
        }
    }
};
void AllocateMesh(Mesh& mesh,int capacity,bool dynamic) {
    mesh={}; mesh.vertexCount=capacity; mesh.triangleCount=capacity/3;
    mesh.vertices=static_cast<float*>(MemAlloc(capacity*3*sizeof(float)));
    mesh.normals=static_cast<float*>(MemAlloc(capacity*3*sizeof(float)));
    mesh.texcoords=static_cast<float*>(MemAlloc(capacity*2*sizeof(float)));
    mesh.colors=static_cast<unsigned char*>(MemAlloc(capacity*4));
    std::memset(mesh.vertices,0,capacity*3*sizeof(float));
    std::memset(mesh.normals,0,capacity*3*sizeof(float));
    std::memset(mesh.texcoords,0,capacity*2*sizeof(float));
    std::memset(mesh.colors,255,capacity*4);
    UploadMesh(&mesh,dynamic);
}
void UploadGeometry(Mesh& mesh,const Geometry& g) {

    UpdateMeshBuffer(mesh,0,g.positions.data(),int(g.positions.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,1,g.uvs.data(),int(g.uvs.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,2,g.normals.data(),int(g.normals.size()*sizeof(float)),0);
    UpdateMeshBuffer(mesh,3,g.colors.data(),int(g.colors.size()),0);
    mesh.vertexCount=g.Count(); mesh.triangleCount=mesh.vertexCount/3;
}
struct Chunk {Mesh opaque{}; std::vector<Face> transparent; uint64_t version=~uint64_t(0);};
std::array<Chunk,ChunkCount*ChunkCount> Chunks;
bool Exposed(Block b,Block neighbor) {
    if(neighbor==Air) return true;
    if(!Transparent(b)) return Transparent(neighbor);
    if(!Transparent(neighbor)||b==neighbor) return false;
    return b>neighbor;
}
void RebuildChunk(int cx,int cz) {
    Chunk& c=Chunks[cx*ChunkCount+cz]; if(c.version==ChunkVersions[cx*ChunkCount+cz]) return;
    Geometry geometry; c.transparent.clear();
    for(int x=cx*ChunkSize;x<(cx+1)*ChunkSize;++x)
        for(int z=cz*ChunkSize;z<(cz+1)*ChunkSize;++z) for(int y=0;y<Height;++y) {
            Block b=At(x,y,z); if(b==Air) continue;
            for(int f=0;f<6;++f) {
                Block neighbor=At(x+Normals[f][0],y+Normals[f][1],z+Normals[f][2]);
                if(Plant(b)&&f!=4&&f!=5) continue;
                bool exposed=Plant(b)|| (Liquid(b)?BuildFluidFace(x,y,z,f).visible:Exposed(b,neighbor));
                if(!exposed) continue;
                Face face{x,y,z,f,b};
                if(Transparent(b)) c.transparent.push_back(face); else geometry.Add(face);
            }
        }
    if(c.opaque.vaoId) UnloadMesh(c.opaque);
    c.opaque={};
    if(geometry.Count()) {AllocateMesh(c.opaque,geometry.Count(),false); UploadGeometry(c.opaque,geometry);}
    c.version=ChunkVersions[cx*ChunkCount+cz];
}
const char* VertexShader=R"GLSL(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
out vec2 uv;
out vec4 tint;
out vec3 worldPosition;
out vec3 normal;
void main() {
    uv=vertexTexCoord; tint=vertexColor; normal=vertexNormal;
    worldPosition=(matModel*vec4(vertexPosition,1.0)).xyz;
    gl_Position=mvp*vec4(vertexPosition,1.0);
}
)GLSL";
const char* FragmentShader=R"GLSL(#version 330
in vec2 uv;
in vec4 tint;
in vec3 worldPosition;
in vec3 normal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 eyePosition;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float regionSize;
uniform float daylight;
uniform vec3 sunDirection;
out vec4 finalColor;
void main() {
    vec4 color=texture(texture0,uv)*colDiffuse;
    if(color.a<0.08) discard;
    float facing=0.35+0.65*max(dot(normalize(normal),normalize(sunDirection)),0.0);
    vec3 sunlight=mix(vec3(0.025,0.035,0.075),vec3(1.0,0.98,0.92)*facing,daylight)*pow(tint.g,2.2);
    vec3 lamplight=vec3(1.0,0.86,0.64)*pow(tint.b,1.45);
    color.rgb*=max(vec3(0.025,0.028,0.035),sunlight+lamplight*(1.0-0.35*daylight))*tint.r;
    if(color.a<0.01) discard;
    float d=length(worldPosition-eyePosition);
    float fog=1.0-exp(-pow(d*fogDensity,2.0));
    float edge=min(min(worldPosition.x,worldPosition.z),min(regionSize-worldPosition.x,regionSize-worldPosition.z));
    fog=max(fog,1.0-smoothstep(0.0,20.0,edge));
    finalColor=vec4(mix(color.rgb,fogColor,clamp(fog,0.0,1.0)),color.a);
}
)GLSL";
struct View {
    Vector3 position,forward,right,up;
    float tanY,tanX;
    explicit View(const Camera3D& c) {
        position=c.position; forward=Vector3Normalize(Vector3Subtract(c.target,c.position));
        right=Vector3Normalize(Vector3CrossProduct(forward,c.up)); up=Vector3CrossProduct(right,forward);
        tanY=std::tan(c.fovy*DEG2RAD*.5f); tanX=tanY*float(GetScreenWidth())/GetScreenHeight();
    }
    bool Sphere(Vector3 center,float radius) const {
        Vector3 delta=Vector3Subtract(center,position);
        float z=Vector3DotProduct(delta,forward);
        if(z+radius<.01f||z-radius>DrawDistance) return false;
        if(std::abs(Vector3DotProduct(delta,right))>z*tanX+radius*std::sqrt(1+tanX*tanX)) return false;
        return std::abs(Vector3DotProduct(delta,up))<=z*tanY+radius*std::sqrt(1+tanY*tanY);
    }
};
struct AlphaRenderer {
    Mesh mesh{}; int capacity=0;
    Geometry geometry;
    struct Entry {Face face; float depth;};
    std::vector<Entry> faces;
    void Draw(const View& view,Material material,const std::vector<int>& visible) {
        faces.clear(); geometry.Clear();
        for(int index:visible) for(const Face& face:Chunks[index].transparent) {
            Vector3 center=Center(face);
            if(view.Sphere(center,.72f)) faces.push_back({face,Vector3DotProduct(Vector3Subtract(center,view.position),view.forward)});
        }
        std::sort(faces.begin(),faces.end(),[](const Entry& a,const Entry& b){return a.depth>b.depth;});
        for(const Entry& entry:faces) geometry.Add(entry.face);
        if(!geometry.Count()) return;
        if(geometry.Count()>capacity) {
            if(mesh.vaoId) UnloadMesh(mesh);
            capacity=((geometry.Count()+4095)/4096)*4096;
            AllocateMesh(mesh,capacity,true);
        }
        UploadGeometry(mesh,geometry);

        rlDrawRenderBatchActive(); rlDisableDepthMask(); rlDisableBackfaceCulling();
        BeginBlendMode(BLEND_ALPHA);
        DrawMesh(mesh,material,MatrixIdentity());
        EndBlendMode();
        rlEnableBackfaceCulling(); rlEnableDepthMask();
    }
    void Release() {if(mesh.vaoId) UnloadMesh(mesh); mesh={};}
};

}
struct Renderer::Impl {
    Texture2D atlas{};
    Shader shader{};
    Material material{};
    AlphaRenderer alpha;
    int eye=-1,fog=-1,density=-1,day=-1,sun=-1;
    bool ready=false;
};
Renderer::Renderer():impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() {
    impl->alpha.Release();
    for(Chunk& c:Chunks) {if(c.opaque.vaoId) UnloadMesh(c.opaque); c=Chunk{};}
    if(impl->material.maps) {
        impl->material.maps[MATERIAL_MAP_DIFFUSE].texture={};
        impl->material.shader={}; UnloadMaterial(impl->material);
    }
    if(impl->shader.id && impl->shader.id!=rlGetShaderIdDefault()) UnloadShader(impl->shader);
    if(impl->atlas.id) UnloadTexture(impl->atlas);
}
bool Renderer::Initialize() {
    impl->atlas=MakeAtlas(); impl->shader=LoadShaderFromMemory(VertexShader,FragmentShader);
    if(!impl->atlas.id||!impl->shader.id||impl->shader.id==rlGetShaderIdDefault()) return false;
    impl->material=LoadMaterialDefault(); impl->material.shader=impl->shader;
    impl->material.maps[MATERIAL_MAP_DIFFUSE].texture=impl->atlas;
    impl->eye=GetShaderLocation(impl->shader,"eyePosition");
    impl->fog=GetShaderLocation(impl->shader,"fogColor");
    impl->density=GetShaderLocation(impl->shader,"fogDensity");
    impl->day=GetShaderLocation(impl->shader,"daylight");
    impl->sun=GetShaderLocation(impl->shader,"sunDirection");
    float regionSize=float(Width);
    SetShaderValue(impl->shader,GetShaderLocation(impl->shader,"regionSize"),&regionSize,SHADER_UNIFORM_FLOAT);
    impl->ready=true; return true;
}
Texture2D Renderer::Atlas() const {return impl->atlas;}
void Renderer::Draw(const Camera3D& camera,const Hit& hit) {
    if(!impl->ready) return;
    UpdateLighting();
    View view(camera); std::vector<int> visible; visible.reserve(ChunkCount*ChunkCount);
    const float radius=std::sqrt(float(ChunkSize*ChunkSize*2+Height*Height))*.5f;
    for(int x=0;x<ChunkCount;++x) for(int z=0;z<ChunkCount;++z) {
        RebuildChunk(x,z);
        if(view.Sphere({(x+.5f)*ChunkSize,Height*.5f,(z+.5f)*ChunkSize},radius)) visible.push_back(x*ChunkCount+z);
    }
    bool underwater=WaterContains(camera.position);
    Color sky=underwater?Color{40,88,126,255}:SkyColor();
    float fog[3]={sky.r/255.f,sky.g/255.f,sky.b/255.f};
    float eye[3]={camera.position.x,camera.position.y,camera.position.z};
    float density=underwater?.12f:.027f;
    SetShaderValue(impl->shader,impl->eye,eye,SHADER_UNIFORM_VEC3);
    SetShaderValue(impl->shader,impl->fog,fog,SHADER_UNIFORM_VEC3);
    SetShaderValue(impl->shader,impl->density,&density,SHADER_UNIFORM_FLOAT);
    float daylight=Daylight();
    SetShaderValue(impl->shader,impl->day,&daylight,SHADER_UNIFORM_FLOAT);
    float angle=WorldTime/DayDuration*2*PI;
    float direction[3]={std::cos(angle),std::sin(angle),-.35f};
    SetShaderValue(impl->shader,impl->sun,direction,SHADER_UNIFORM_VEC3);
    ClearBackground(sky); BeginMode3D(camera);
    Vector3 sun=Vector3Add(camera.position,{std::cos(angle)*85,std::sin(angle)*85,-30});
    Vector3 moon=Vector3Add(camera.position,{-std::cos(angle)*85,-std::sin(angle)*85,30});
    if(!underwater) {
        DrawSphere(sun,3,{255,240,173,255});DrawSphere(moon,2,{194,208,234,255});
    }
    for(int i:visible) if(Chunks[i].opaque.vertexCount) DrawMesh(Chunks[i].opaque,impl->material,MatrixIdentity());
    impl->alpha.Draw(view,impl->material,visible);
    if(hit.found) DrawCubeWires({hit.x+.5f,hit.y+.5f,hit.z+.5f},1.006f,1.006f,1.006f,{25,29,24,255});
    EndMode3D();
}
}
