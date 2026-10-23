#include "player.h"
#include "world.h"
#include "settings.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
namespace game {
namespace {
bool Overlap(Vector3 p,int x,int y,int z) {
    return p.x+Radius>x&&p.x-Radius<x+1&&p.y+BodyHeight>y&&p.y<y+1&&p.z+Radius>z&&p.z-Radius<z+1;
}
bool Supported(Vector3 p) {
    p.y-=.025f;
    for(int x=int(std::floor(p.x-Radius));x<=int(std::floor(p.x+Radius));++x)
        for(int z=int(std::floor(p.z-Radius));z<=int(std::floor(p.z+Radius));++z) {
            int y=int(std::floor(p.y)); if(Solid(At(x,y,z))&&Overlap(p,x,y,z)) return true;
        }
    return false;
}
bool MoveAxis(Vector3& p,int axis,float movement) {
    if(movement==0) return false;
    auto component=[](Vector3& v,int a)->float& {if(a==0)return v.x; if(a==1)return v.y; return v.z;};
    Vector3 target=p; component(target,axis)+=movement;
    float resolved=component(target,axis); bool hit=false;
    for(int x=int(std::floor(target.x-Radius));x<=int(std::floor(target.x+Radius));++x)
        for(int y=int(std::floor(target.y));y<=int(std::floor(target.y+BodyHeight));++y)
            for(int z=int(std::floor(target.z-Radius));z<=int(std::floor(target.z+Radius));++z) {
                if(!Solid(At(x,y,z))||!Overlap(target,x,y,z)) continue;
                int cell=axis==0?x:(axis==1?y:z);
                float low=axis==1?0.f:Radius,high=axis==1?BodyHeight:Radius;
                resolved=movement>0?std::min(resolved,cell-high-Skin):std::max(resolved,cell+1+low+Skin);
                hit=true;
            }
    component(p,axis)=resolved; return hit;
}
bool InWater(Vector3 p) {
    for(int x=int(std::floor(p.x-Radius));x<=int(std::floor(p.x+Radius));++x)
        for(int y=int(std::floor(p.y));y<=int(std::floor(p.y+BodyHeight));++y)
            for(int z=int(std::floor(p.z-Radius));z<=int(std::floor(p.z+Radius));++z)
                if(At(x,y,z)==Water&&Overlap(p,x,y,z)&&p.y<y+WaterHeight(x,y,z)) return true;
    return false;
}
Vector3 Spawn() {
    for(int r=0;r<Width/2;++r) for(int dx=-r;dx<=r;++dx) for(int dz=-r;dz<=r;++dz) {
        if(std::max(std::abs(dx),std::abs(dz))!=r) continue;
        int x=Width/2+dx,z=Width/2+dz,y=Height-1;
        while(y>=0&&At(x,y,z)==Air) --y;
        if(y>=0&&y<Height-3&&Solid(At(x,y,z))) return {x+.5f,y+1+Skin,z+.5f};
    }
    return {Width*.5f,Height+2.f,Width*.5f};
}

}
PlayerState Player::CaptureState() const {return {position,yaw,pitch,velocityY,true};}
bool Player::RestoreState(const PlayerState& state) {
    if(!state.valid||!std::isfinite(state.position.x)||!std::isfinite(state.position.y)||
       !std::isfinite(state.position.z)||!std::isfinite(state.yaw)||!std::isfinite(state.pitch)||
       !std::isfinite(state.velocityY)||std::abs(state.pitch)>1.55f) return false;
    position=state.position;yaw=state.yaw;pitch=state.pitch;velocityY=state.velocityY;
    return true;
}
void Player::Respawn() {position=Spawn(); velocityY=0; yaw=0; pitch=0;}
bool Player::Overlaps(int x,int y,int z) const {return Overlap(position,x,y,z);}
Camera3D Player::Camera() const {
    Camera3D c{}; c.position={position.x,position.y+EyeHeight,position.z};
    Vector3 d{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
    c.target=Vector3Add(c.position,d); c.up={0,1,0}; c.fovy=70; c.projection=CAMERA_PERSPECTIVE; return c;
}
void Player::Update(const PlayerInput& input,float dt) {
    yaw=std::remainder(yaw+input.look.x*.0025f*Settings.mouseSensitivity,2.f*PI);
    pitch=std::clamp(pitch-input.look.y*.0025f*Settings.mouseSensitivity,-1.55f,1.55f);
    float f=input.forward,s=input.side,length=std::sqrt(f*f+s*s);
    if(length>1){f/=length;s/=length;}
    bool swimming=InWater(position); float speed=swimming?2.7f:4.8f;
    float vx=(std::sin(yaw)*f+std::cos(yaw)*s)*speed;
    float vz=(-std::cos(yaw)*f+std::sin(yaw)*s)*speed;
    if(swimming) {
        velocityY=std::max(velocityY-5.f*dt,-3.f);
        if(input.swimUp) velocityY=3.4f;
        if(input.swimDown) velocityY=-3.4f;
    } else {
        const bool grounded=velocityY<=0&&Supported(position);
        if(grounded&&input.jump) velocityY=8.2f;
        velocityY=std::max(velocityY-23.f*dt,-32.f);
    }
    int steps=std::max(1,int(std::ceil(std::max({std::abs(vx),std::abs(vz),std::abs(velocityY)})*dt/.09f)));
    float tick=dt/steps;
    for(int i=0;i<steps;++i) {
        MoveAxis(position,0,vx*tick); MoveAxis(position,2,vz*tick);
        if(MoveAxis(position,1,velocityY*tick)) velocityY=0;
    }
    if(position.y<-25) Respawn();
}
}
