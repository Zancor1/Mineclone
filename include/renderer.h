#pragma once
#include <raylib.h>
#include <memory>
#include "world.h"
namespace game {
constexpr int TileSize=16,TileCount=16+(BlockCount-14);
int BlockTile(Block block,int direction=2);
struct WaterFace {
    std::array<Vector3,4> vertices{};
    Vector3 normal{};
    bool visible=false;
};
WaterFace BuildWaterFace(int x,int y,int z,int direction);
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool Initialize();
    void Draw(const Camera3D& camera,const Hit& hit);
    Texture2D Atlas() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
