#pragma once
#include <raylib.h>
namespace game {
constexpr float Radius=.29f,BodyHeight=1.8f,EyeHeight=1.62f,Skin=.001f;
struct PlayerInput {Vector2 look{}; float forward=0,side=0; bool jump=false,swimUp=false,swimDown=false,sprint=false;};
struct PlayerState {
    Vector3 position{};
    float yaw=0,pitch=0,velocityY=0;
    bool valid=false;
    bool flying=false;
};
class Player {
public:
    void Respawn();
    void Translate(Vector3 displacement);
    PlayerState CaptureState() const;
    bool RestoreState(const PlayerState& state);
    void Update(const PlayerInput& input,float dt);
    Camera3D Camera() const;
    bool Overlaps(int x,int y,int z) const;
private:
    Vector3 position{};
    float yaw=0,pitch=0,velocityY=0;
    bool flying=false,jumpWasDown=false;
    float jumpTapAge=1.f;
};
}
