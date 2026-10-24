#include "player.h"
#include "map_blocks.h"
#include "world.h"
#include "settings.h"
#include "survival.h"
#include "network.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
namespace game {
namespace {
float PlayerRadius(){return net::Active()?.3f:Radius;}
float CellHeight(int x,int y,int z){return net::Active()?net::CollisionAt(x,y,z):CollisionHeight(At(x,y,z));}
bool CellSolid(int x,int y,int z){return net::Active()?net::CollisionAt(x,y,z)>0:CollisionBoxesAt(x,y,z).count>0;}
bool Intersects(Vector3 p,BoundingBox box){return p.x+PlayerRadius()>box.min.x&&p.x-PlayerRadius()<box.max.x&&p.y+BodyHeight>box.min.y&&p.y<box.max.y&&p.z+PlayerRadius()>box.min.z&&p.z-PlayerRadius()<box.max.z;}
bool Overlap(Vector3 p,int x,int y,int z) {
    if(net::Active()||Liquid(At(x,y,z)))return Intersects(p,{{float(x),float(y),float(z)},{x+1.f,y+CellHeight(x,y,z),z+1.f}});
    auto boxes=CollisionBoxesAt(x,y,z);for(int i=0;i<boxes.count;++i)if(Intersects(p,boxes.boxes[i]))return true;return false;
}
bool Supported(Vector3 p) {
    p.y-=.025f;
    for(int x=int(std::floor(p.x-PlayerRadius()));x<=int(std::floor(p.x+PlayerRadius()));++x)
        for(int z=int(std::floor(p.z-PlayerRadius()));z<=int(std::floor(p.z+PlayerRadius()));++z) {
            int y=int(std::floor(p.y)); if(CellSolid(x,y,z)&&Overlap(p,x,y,z)) return true;
        }
    return false;
}
bool MoveAxis(Vector3& p,int axis,float movement) {
    if(movement==0) return false;
    auto component=[](Vector3& v,int a)->float& {if(a==0)return v.x; if(a==1)return v.y; return v.z;};
    Vector3 target=p; component(target,axis)+=movement;
    float resolved=component(target,axis); bool hit=false;
    for(int x=int(std::floor(target.x-PlayerRadius()));x<=int(std::floor(target.x+PlayerRadius()));++x)
        for(int y=int(std::floor(target.y));y<=int(std::floor(target.y+BodyHeight));++y)
            for(int z=int(std::floor(target.z-PlayerRadius()));z<=int(std::floor(target.z+PlayerRadius()));++z) {
                if(!CellSolid(x,y,z)||!Overlap(target,x,y,z)) continue;
                BlockBoxes boxes;
                if(net::Active()){boxes.count=1;boxes.boxes[0]={{float(x),float(y),float(z)},{x+1.f,y+CellHeight(x,y,z),z+1.f}};}else boxes=CollisionBoxesAt(x,y,z);
                for(int i=0;i<boxes.count;++i){auto box=boxes.boxes[i];if(!Intersects(target,box))continue;
                    float lower=axis==0?box.min.x:axis==1?box.min.y:box.min.z,upper=axis==0?box.max.x:axis==1?box.max.y:box.max.z;
                    float low=axis==1?0.f:PlayerRadius(),high=axis==1?BodyHeight:PlayerRadius(),skin=net::Active()?0.f:Skin;
                    resolved=movement>0?std::min(resolved,lower-high-skin):std::max(resolved,upper+low+skin);hit=true;
                }
            }
    component(p,axis)=resolved; return hit;
}
bool InWater(Vector3 p) {
    for(int x=int(std::floor(p.x-PlayerRadius()));x<=int(std::floor(p.x+PlayerRadius()));++x)
        for(int y=int(std::floor(p.y));y<=int(std::floor(p.y+BodyHeight));++y)
            for(int z=int(std::floor(p.z-PlayerRadius()));z<=int(std::floor(p.z+PlayerRadius()));++z)
                if(Liquid(At(x,y,z))&&Overlap(p,x,y,z)&&p.y<y+FluidHeight(x,y,z)) return true;
    return false;
}
Vector3 Spawn() {
    for(int x=0;x<Width;++x)for(int z=0;z<Width;++z)for(int y=0;y<Height-3;++y)
        if(At(x,y,z)==Bed&&At(x,y,z+1)==BedHead&&At(x,y+1,z)==Air&&At(x,y+2,z)==Air)
            return {x+.5f,y+CollisionHeight(Bed)+Skin,z+.5f};
    for(int r=0;r<Width/2;++r) for(int dx=-r;dx<=r;++dx) for(int dz=-r;dz<=r;++dz) {
        if(std::max(std::abs(dx),std::abs(dz))!=r) continue;
        int x=Width/2+dx,z=Width/2+dz,y=Height-1;
        while(y>=0&&At(x,y,z)==Air) --y;
        if(y>=0&&y<Height-3&&CellSolid(x,y,z)) return {x+.5f,y+1+Skin,z+.5f};
    }
    return {Width*.5f,Height+2.f,Width*.5f};
}

}
PlayerState Player::CaptureState() const {return {position,yaw,pitch,velocityY,true,flying};}
bool Player::RestoreState(const PlayerState& state) {
    if(!state.valid||!std::isfinite(state.position.x)||!std::isfinite(state.position.y)||
       !std::isfinite(state.position.z)||!std::isfinite(state.yaw)||!std::isfinite(state.pitch)||
       !std::isfinite(state.velocityY)||std::abs(state.pitch)>1.55f) return false;
    sprintLock=0;sprintLatched=false;meleeMotion=false;motionX=motionZ=0;impulse={};if(!net::Active())networkAccumulator=0;previousPosition=state.position;position=state.position;yaw=state.yaw;pitch=state.pitch;velocityY=state.velocityY;
    flying=state.flying&&Mode==GameMode::Creative;jumpWasDown=false;jumpTapAge=1.f;
    return true;
}
void Player::Translate(Vector3 displacement) {position=Vector3Add(position,displacement);previousPosition=Vector3Add(previousPosition,displacement);}
bool Player::OnGround() const {return velocityY<=0&&Supported(position);}
void Player::SetRemoteVelocity(Vector3 v) {meleeMotion=false;impulse=v;velocityY=v.y;}
void Player::SuspendSprint(float seconds){sprintLock=std::max(sprintLock,seconds);sprinting=false;}
void Player::MeleeKnockback(Vector3 push){
    const bool alreadyAirborne=meleeMotion&&!OnGround();
    meleeMotion=true;
    motionX*=.5f;motionZ*=.5f;impulse.x=impulse.x*.35f+push.x;impulse.z=impulse.z*.35f+push.z;
    float cap=std::max(8.5f,std::hypot(push.x,push.z)*1.25f);float horizontal=std::hypot(impulse.x,impulse.z);if(horizontal>cap){impulse.x*=cap/horizontal;impulse.z*=cap/horizontal;}
    if(!alreadyAirborne)velocityY=std::min(push.y,velocityY*.5f+push.y);
    FallDistance=0;
}
void Player::Knockback(Vector3 push) {meleeMotion=false;impulse=Vector3Add(impulse,push);velocityY=std::max(velocityY,push.y);FallDistance=0;}
void Player::Respawn() {sprintLock=0;sprintLatched=false;meleeMotion=false;motionX=motionZ=0;sneaking=sprinting=false;impulse={};position=Spawn(); velocityY=0; yaw=0; pitch=0;flying=false;jumpWasDown=false;jumpTapAge=1.f;previousPosition=position;networkAccumulator=0;}
bool Player::Overlaps(int x,int y,int z) const {return Overlap(position,x,y,z);}
Camera3D Player::Camera() const {
    Vector3 view=net::Active()?Vector3Lerp(previousPosition,position,std::clamp(networkAccumulator/.05f,0.f,1.f)):position;
    Camera3D c{}; c.position={view.x,view.y+EyeHeight-(sneaking?.08f:0.f),view.z};
    Vector3 d{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
    c.target=Vector3Add(c.position,d); c.up={0,1,0}; c.fovy=Settings.fov; c.projection=CAMERA_PERSPECTIVE; return c;
}
void Player::Update(const PlayerInput& input,float dt) {
    if(!Settings.toggleSprint)sprintLatched=false;
    else if(input.sprintPressed)sprintLatched=!sprintLatched;
    const bool sprintRequested=(Settings.toggleSprint?sprintLatched:input.sprint)&&sprintLock<=0;
    sprintLock=std::max(0.f,sprintLock-std::max(0.f,dt));

    if(Mode==GameMode::Survival) flying=false;
    jumpTapAge+=std::max(0.f,dt);
    if(Mode==GameMode::Creative&&input.jump&&!jumpWasDown) {
        if(jumpTapAge<=.28f) {flying=!flying;velocityY=0;jumpTapAge=1.f;}
        else jumpTapAge=0;
    }
    jumpWasDown=input.jump;
    yaw=std::remainder(yaw+input.look.x*.0025f*Settings.mouseSensitivity,2.f*PI);
    pitch=std::clamp(pitch-input.look.y*.0025f*Settings.mouseSensitivity,-1.55f,1.55f);
    if(net::Active()){networkAccumulator+=std::clamp(dt,0.f,.05f);if(networkAccumulator<.05f)return;networkAccumulator-=.05f;dt=.05f;previousPosition=position;}
    bool swimming=InWater(position);
    if(swimming||flying)meleeMotion=false;
    sneaking=input.swimDown&&!flying&&!swimming;
    sprinting=sprintRequested&&input.forward>=.8f&&!sneaking;
    float inputScale=net::Active()?.98f:1.f;
    float f=input.forward*inputScale,s=input.side*inputScale;
    if(sneaking){f*=.3f;s*=.3f;}
    float length=std::sqrt(f*f+s*s);if(length>1){f/=length;s/=length;}
    const bool grounded=velocityY<=0&&Supported(position);
    float friction=grounded?.546f:.91f;
    float speed=flying?(sprintRequested?22.f:11.f):(swimming?2.7f:(sprinting?5.2f:4.317f));
    if(!net::Active())speed*=std::clamp(input.speedMultiplier,1.f,1.5f);
    float vx,vz;
    if(net::Active()&&!flying&&!swimming){
        float acceleration=grounded?(net::MovementSpeed()*(sprinting?1.3f:1.f)*.16277136f/(friction*friction*friction)):(sprinting?.026f:.02f);
        motionX+=(std::sin(yaw)*f+std::cos(yaw)*s)*acceleration*20.f;
        motionZ+=(-std::cos(yaw)*f+std::sin(yaw)*s)*acceleration*20.f;
        vx=motionX;vz=motionZ;
    }else{
        vx=(std::sin(yaw)*f+std::cos(yaw)*s)*speed;
        vz=(-std::cos(yaw)*f+std::sin(yaw)*s)*speed;
        motionX=motionZ=0;
    }
    if(flying) {
        velocityY=(float(input.swimUp||input.jump)-float(input.swimDown))*(sprintRequested?16.f:8.f);
    } else if(swimming) {
        velocityY=std::max(velocityY-5.f*dt,-3.f);
        if(input.swimUp) velocityY=3.4f;
        if(input.swimDown) velocityY=-3.4f;
    } else {
        const bool grounded=velocityY<=0&&Supported(position);
        if(grounded&&input.jump){velocityY=(net::Active()?8.4f:8.2f)*std::clamp(input.jumpMultiplier,1.f,1.5f);
            if(net::Active()&&sprinting){motionX+=std::sin(yaw)*4.f;motionZ-=std::cos(yaw)*4.f;vx=motionX;vz=motionZ;}}

        if(!net::Active())velocityY=std::max(velocityY-(meleeMotion?28.f:23.f)*dt,-32.f);
    }
    Block atBody=At(int(std::floor(position.x)),int(std::floor(position.y+.5f)),int(std::floor(position.z)));
    if(atBody==Ladder&&!flying) {velocityY=input.swimDown?-2.f:((input.forward>0||input.jump)?3.f:0.f);FallDistance=0;}
    vx+=impulse.x;vz+=impulse.z;impulse=Vector3Scale(impulse,std::exp(-dt*(meleeMotion?4.5f:6.f)));
    if(sneaking&&grounded){
        auto floorBelow=[](Vector3 p){p.y-=1.f;for(int x=int(std::floor(p.x-PlayerRadius()));x<=int(std::floor(p.x+PlayerRadius()));++x)for(int y=int(std::floor(p.y));y<=int(std::floor(p.y+BodyHeight));++y)for(int z=int(std::floor(p.z-PlayerRadius()));z<=int(std::floor(p.z+PlayerRadius()));++z)if(CellSolid(x,y,z)&&Overlap(p,x,y,z))return true;return false;};
        float dx=vx*dt,dz=vz*dt;
        auto reduce=[](float v){return std::abs(v)<.05f?0.f:v+(v>0?-.05f:.05f);};
        while(dx!=0&&!floorBelow(Vector3Add(position,{dx,0,0})))dx=reduce(dx);
        while(dz!=0&&!floorBelow(Vector3Add(position,{0,0,dz})))dz=reduce(dz);
        while(dx!=0&&dz!=0&&!floorBelow(Vector3Add(position,{dx,0,dz}))){dx=reduce(dx);dz=reduce(dz);}
        if(dt>0){vx=dx/dt;vz=dz/dt;}
    }
    int steps=std::max(1,int(std::ceil(std::max({std::abs(vx),std::abs(vz),std::abs(velocityY)})*dt/.09f)));
    float tick=dt/steps;
    for(int i=0;i<steps;++i) {
        Vector3 start=position;
        bool hitX=MoveAxis(position,0,vx*tick),hitZ=MoveAxis(position,2,vz*tick);
        if((hitX||hitZ)&&!flying&&!swimming&&velocityY<=0&&Supported(start)){
            Vector3 raised=start;
            MoveAxis(raised,1,.6f);
            MoveAxis(raised,0,vx*tick);MoveAxis(raised,2,vz*tick);
            MoveAxis(raised,1,start.y-raised.y);
            float normalDistance=std::hypot(position.x-start.x,position.z-start.z),stepDistance=std::hypot(raised.x-start.x,raised.z-start.z);
            if(stepDistance>normalDistance+.00001f&&raised.y<=start.y+.601f&&Supported(raised)){position=raised;hitX=hitZ=false;}
        }
        if(hitX)motionX=0;
        if(hitZ)motionZ=0;
        float beforeY=position.y;bool descending=velocityY<0;
        if(MoveAxis(position,1,velocityY*tick)) {
            if(descending&&!swimming&&!flying) {meleeMotion=false;if(!net::Active()&&FallDistance>3) Hurt(std::floor(FallDistance-3));FallDistance=0;}
            velocityY=0;
        } else if(descending&&!swimming&&!flying) FallDistance+=std::max(0.f,beforeY-position.y);
        else if(swimming||flying||velocityY>0) FallDistance=0;
    }
    if(net::Active()&&!flying&&!swimming){motionX*=friction;motionZ*=friction;}
    if(net::Active()&&!flying&&!swimming)velocityY=(velocityY-1.6f)*.98f;
    if(position.y<-25&&Mode==GameMode::Creative&&!net::Active()) Respawn();
    if(net::Joined()){net::MovementFlags(sprinting,input.swimDown);net::SendMovement(*this);}
    if(!net::Active())UpdateHazards(position,dt);
}
}
