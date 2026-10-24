#pragma once
#include <raylib.h>
#include <memory>
#include "world.h"
namespace game {
constexpr int TileSize=16,BaseTileCount=16+(BlockCount-14)+2,TileCount=BaseTileCount+13;
int BlockTile(Block block,int direction=2);
void DrawDroppedItem3D(Texture2D atlas,Block item,Vector3 position,float age);
void DrawHeldItem3D(Texture2D atlas,Block item,Matrix transform,Color tint=WHITE,int finish=0,float bowDraw=-1.f);
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
    void BeginWorldLoad();
    void CancelWorldLoad();
    bool PrepareWorld(const Camera3D& camera,float budgetMilliseconds=4.f);
    float WorldLoadProgress() const;
    const char* WorldLoadStage() const;
    void Draw(const Camera3D& camera,const Hit& hit,float miningProgress=0);
    void DrawFirstPerson(const Camera3D& camera,Block item,bool moving,bool usingItem,float dt);
    Texture2D Atlas() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
