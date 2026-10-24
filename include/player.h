#pragma once
#include <raylib.h>
namespace game {
constexpr float Radius=.29f,BodyHeight=1.8f,EyeHeight=1.62f,Skin=.001f;
struct PlayerInput {Vector2 look{}; float forward=0,side=0; bool jump=false,swimUp=false,swimDown=false,sprint=false,sprintPressed=false;float speedMultiplier=1,jumpMultiplier=1;};
struct PlayerState {
    Vector3 position{};
    float yaw=0,pitch=0,velocityY=0;
    bool valid=false;
    bool flying=false;
};
class Player {
public:
    void Respawn();
    void Knockback(Vector3 impulse);
    void MeleeKnockback(Vector3 impulse);
    void ExplosionKnockback(Vector3 impulse);
    void SuspendSprint(float seconds);
    void SetRemoteVelocity(Vector3 velocity);
    bool OnGround() const;
    bool IsSneaking() const {return sneaking;}
    bool IsSprinting() const {return sprinting;}
    void Translate(Vector3 displacement);
    PlayerState CaptureState() const;
    bool RestoreState(const PlayerState& state);
    void Update(const PlayerInput& input,float dt,bool affectVitals=true);
    Camera3D Camera() const;
    bool Overlaps(int x,int y,int z) const;
private:
    Vector3 position{},impulse{},previousPosition{},pendingExplosion{};
    float explosionRamp=0,explosionDrag=0;
    float yaw=0,pitch=0,velocityY=0;
    bool flying=false,jumpWasDown=false,sneaking=false,sprinting=false,sprintLatched=false,meleeMotion=false;
    float motionX=0,motionZ=0,sprintLock=0;
    float jumpTapAge=1.f,networkAccumulator=0;
};
}
