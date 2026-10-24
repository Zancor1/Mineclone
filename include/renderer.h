#pragma once
#include <raylib.h>
#include <memory>
#include "world.h"
namespace game {
constexpr int TileSize=16,TileCount=16+(BlockCount-14)+2;
int BlockTile(Block block,int direction=2);
void DrawHeldItem3D(Texture2D atlas,Block item,Matrix transform,Color tint=WHITE);
struct WaterFace {
    std::array<Vector3,4> vertices{};
    Vector3 normal{};
    bool visible=false;
};
WaterFace BuildFluidFace(int x,int y,int z,int direction);
WaterFace BuildWaterFace(int x,int y,int z,int direction);
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool Initialize();
    void Draw(const Camera3D& camera,const Hit& hit);
    void DrawFirstPerson(const Camera3D& camera,Block item,bool moving,bool usingItem,float dt);
    Texture2D Atlas() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
