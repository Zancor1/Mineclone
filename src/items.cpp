#include "items.h"
#include "projectile_physics.h"
#include "arrow_model.h"
#include "arrow_collision.h"
#include "world.h"
#include "survival.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <random>
namespace game {
namespace {
struct Projectile {Block type;Vector3 position,velocity;float life;float tick=0;bool embedded=false;Vector3 support{};float damage=4;bool critical=false;};
std::vector<Projectile> projectiles;
float cooldown=0,charge=0,eating=0;
Block lastItem=Air;bool bowHeld=false;
bool Free(Vector3 feet) {
    if(!Inside(int(std::floor(feet.x)),int(std::floor(feet.y)),int(std::floor(feet.z))))return false;
    for(int x=int(std::floor(feet.x-Radius));x<=int(std::floor(feet.x+Radius));++x)
        for(int y=int(std::floor(feet.y));y<=int(std::floor(feet.y+BodyHeight));++y)
            for(int z=int(std::floor(feet.z-Radius));z<=int(std::floor(feet.z+Radius));++z)
                if(Solid(At(x,y,z))&&feet.y<y+CollisionHeight(At(x,y,z)))return false;
    return true;
}
void Consume(ItemStack& stack) {if(Mode==GameMode::Survival&&stack.count) {--stack.count;if(!stack.count)stack={};}}
bool Container(ItemStack& held,Block replacement) {
    if(Mode==GameMode::Creative)return true;
    if(held.count==1) {held={replacement,1};return true;}
    auto bag=Backpack;auto bar=Hotbar;ItemStack result{replacement,1};
    TransferStack(result,bag.data(),bag.size());TransferStack(result,bar.data(),bar.size());
    if(result.count)return false;
    --held.count;Backpack=bag;
    // The copied hotbar still contains the pre-consumption bucket stack.
    for(size_t i=0;i<Hotbar.size();++i)if(&held==&Hotbar[i])bar[i]=held;
    Hotbar=bar;return true;
}
bool Ammo() {
    if(Mode==GameMode::Creative)return true;
    for(auto* slots:{&Backpack})for(auto& s:*slots)if(s.block==Arrow&&s.count){Consume(s);return true;}
    for(auto& s:Hotbar)if(s.block==Arrow&&s.count){Consume(s);return true;}
    return false;
}
void Wear(ItemStack& s) {if(Mode==GameMode::Survival&&s.count&&++s.damage>=ToolDurability(s.block))s={};}
void Shoot(Block item,Vector3 eye,Vector3 direction,float speed) {projectiles.push_back({item,eye,Vector3Scale(direction,speed),12});}
void PearlHit(Player& player,Vector3 position,Vector3 direction) {
    Vector3 target=Vector3Subtract(position,Vector3Scale(direction,.4f));
    for(float y: {0.f,.5f,1.f,1.5f,2.f}) {
        Vector3 feet=Vector3Add(target,{0,y,0});
        if(Free(feet)) {auto pose=player.CaptureState();pose.position=feet;pose.velocityY=0;if(player.RestoreState(pose)) {FallDistance=0;Hurt(5);}return;}
    }
}
}
int ActiveProjectiles() {return int(projectiles.size());}
float BowDrawTime(){return bowHeld?charge:-1.f;}
void CancelItemUse() {charge=eating=0;bowHeld=false;}
void ClearItems() {projectiles.clear();cooldown=charge=eating=0;lastItem=Air;bowHeld=false;}
void TranslateItems(Vector3 displacement) {for(auto& p:projectiles){p.position=Vector3Add(p.position,displacement);p.support=Vector3Add(p.support,displacement);}}
void PrimeTnt(Vector3 position,float fuse) {if(projectiles.size()<128)projectiles.push_back({Tnt,position,{0,2,0},fuse});}
void Explode(Player& player,Vector3 center,float radius) {
    int extent=int(std::ceil(radius));
    struct Cell {int x,y,z;};std::vector<Cell> destroyed;
    for(int x=int(std::floor(center.x))-extent;x<=int(center.x)+extent;++x)
        for(int y=int(std::floor(center.y))-extent;y<=int(center.y)+extent;++y)
            for(int z=int(std::floor(center.z))-extent;z<=int(center.z)+extent;++z) {
                Block b=At(x,y,z);if(b==Air||Liquid(b)||b==Bedrock||b==Obsidian||b==Chest||b==Furnace)continue;
                Vector3 target{x+.5f,y+.5f,z+.5f};float distance=Vector3Distance(center,target);
                if(distance>radius)continue;
                bool blocked=false;Vector3 delta=Vector3Subtract(target,center);
                for(float t=.1f;t<distance-.6f;t+=.2f) {
                    Vector3 p=Vector3Add(center,Vector3Scale(delta,t/std::max(distance,.01f)));
                    Block wall=At(int(std::floor(p.x)),int(std::floor(p.y)),int(std::floor(p.z)));
                    if(wall==Obsidian||wall==Bedrock||Liquid(wall)){blocked=true;break;}
                }
                if(!blocked)destroyed.push_back({x,y,z});
            }
    for(auto c:destroyed) {if(At(c.x,c.y,c.z)==Tnt)PrimeTnt({c.x+.5f,c.y+.5f,c.z+.5f},.8f);Change(c.x,c.y,c.z,Air);}
    Vector3 torso=Vector3Add(player.CaptureState().position,{0,.9f,0});float distance=Vector3Distance(center,torso);
    if(distance<radius*2) {
        Vector3 direction=Vector3Normalize(Vector3Subtract(torso,center));float force=1-distance/(radius*2);
        Hurt(12*force,true);player.Knockback({direction.x*force*12,std::max(2.f,direction.y*force*10+4),direction.z*force*12});
    }
}
bool UseHeldItem(Player& player,ItemStack& stack,Vector3 direction,const UseInput& input) {
    Block item=stack.count?stack.block:Air;
    if(item!=lastItem) {charge=eating=0;lastItem=item;bowHeld=false;}
    const Vector3 eye=player.Camera().position;
    if(item==Bow) {
        bool ammunition=Mode==GameMode::Creative;for(const auto& s:Hotbar)ammunition|=s.block==Arrow&&s.count;for(const auto& s:Backpack)ammunition|=s.block==Arrow&&s.count;
        if(input.pressed&&ammunition){bowHeld=true;charge=0;}
        if(bowHeld&&input.down)charge=std::min(1.5f,charge+input.dt);
        if(input.released){
            if(bowHeld&&cooldown<=0&&projectiles.size()<128&&Ammo()){
                static std::mt19937 rng(std::random_device{}());float damage=BowDamage(charge,rng());
                Shoot(Arrow,eye,direction,ArrowSpeed*BowPower(charge));projectiles.back().damage=damage;projectiles.back().critical=damage>4;Wear(stack);cooldown=BowShotDelay;
            }
            bowHeld=false;charge=0;
        }
        return true;
    }
    if(item==GoldenApple) {
        eating=input.down?eating+input.dt:0;
        if(eating>=1.6f&&Health>0) {EatGoldenApple();Consume(stack);eating=0;}
        return true;
    }
    if(!input.pressed||cooldown>0)return false;
    if(Armor(item)) {EquipArmor(stack);return true;}
    if(item==EnderPearl||item==Fireball||item==Snowball||item==Egg) {
        if(projectiles.size()<128) {Shoot(item,eye,direction,item==Fireball?FireballSpeed:20.f);Consume(stack);cooldown=item==Fireball?.5f:.3f;}
        return true;
    }
    if(item==Bucket||item==WaterBucket||item==LavaBucket) {
        Hit hit=Trace(eye,direction,true);if(!hit.found)return true;
        if(item==Bucket) {
            Block liquid=At(hit.x,hit.y,hit.z);
            if(Liquid(liquid)&&IsFluidSource(hit.x,hit.y,hit.z)&&Container(stack,liquid==Water?WaterBucket:LavaBucket))Change(hit.x,hit.y,hit.z,Air);
        }else {
            bool liquid=Liquid(At(hit.x,hit.y,hit.z));int x=hit.x+(liquid?0:hit.nx),y=hit.y+(liquid?0:hit.ny),z=hit.z+(liquid?0:hit.nz);
            if(Inside(x,y,z)&&(At(x,y,z)==Air||Liquid(At(x,y,z)))&&Container(stack,Bucket))Change(x,y,z,item==WaterBucket?Water:Lava);
        }
        return true;
    }
    if(item==Tnt||item==FlintAndSteel||item==Bed) {
        Hit hit=Trace(eye,direction);if(!hit.found)return true;
        int x=hit.x+hit.nx,y=hit.y+hit.ny,z=hit.z+hit.nz;
        if(item==FlintAndSteel) {
            if(At(hit.x,hit.y,hit.z)==Tnt){Change(hit.x,hit.y,hit.z,Air);PrimeTnt({hit.x+.5f,hit.y+.5f,hit.z+.5f});Wear(stack);}return true;
        }
        if(!Inside(x,y,z)||At(x,y,z)!=Air||player.Overlaps(x,y,z))return true;
        if(item==Tnt) {PrimeTnt({x+.5f,y+.5f,z+.5f});Consume(stack);}
        else if(Inside(x,y,z+1)&&At(x,y,z+1)==Air&&Solid(At(x,y-1,z))&&Solid(At(x,y-1,z+1))&&!player.Overlaps(x,y,z+1)) {
            Change(x,y,z,Bed);Change(x,y,z+1,BedHead);Consume(stack);
        }
        return true;
    }
    return false;
}
void UpdateItems(Player& player,float dt) {
    dt=std::clamp(dt,0.f,.05f);cooldown=std::max(0.f,cooldown-dt);
    // Explosions may append primed TNT; process only the entities present at tick start.
    size_t count=projectiles.size();
    for(size_t i=0;i<count;++i) {
        Projectile p=projectiles[i];if(p.life<=0)continue;p.life-=dt;
        if(p.embedded){if(!ArrowSolidPoint(p.support))p.life=0;projectiles[i]=p;continue;}
        if(p.type==Tnt&&p.life<=0) {projectiles[i].life=0;Explode(player,p.position);continue;}
        int ticks=1;float stepTime=dt;
        if(p.type==Arrow){p.tick+=dt;ticks=int((p.tick+1e-6f)/ProjectileTick);p.tick=std::max(0.f,p.tick-ticks*ProjectileTick);stepTime=ProjectileTick;}
        else if(p.type!=Fireball)p.velocity.y-=20.f*dt;
        for(int tick=0;tick<ticks&&p.life>0&&!p.embedded;++tick){
            Vector3 delta=Vector3Scale(p.velocity,stepTime);int steps=std::max(1,int(std::ceil(Vector3Length(delta)/.08f)));

            for(int step=0;step<steps;++step) {
                Vector3 next=Vector3Add(p.position,Vector3Scale(delta,1.f/steps));
                int x=int(std::floor(next.x)),y=int(std::floor(next.y-(p.type==Tnt?.45f:0.f))),z=int(std::floor(next.z));Block b=At(x,y,z);
                if(x<0||x>=Width||z<0||z>=Width||next.y<-25||next.y>Height+256) {p.life=0;break;}
                if(p.type==Arrow&&ArrowSolidPoint(next)){p.support=next;p.position=EmbeddedArrowTip(p.position,next);p.embedded=true;p.life=60;break;}
                if(p.type!=Arrow&&Solid(b)&&next.y<y+CollisionHeight(b)) {
                    if(p.type==Tnt) {p.velocity={0,0,0};break;}
                    p.life=0;
                    if(p.type==EnderPearl)PearlHit(player,p.position,Vector3Normalize(p.velocity));
                    if(p.type==Fireball) {projectiles[i].life=0;Explode(player,p.position,2.5f);}
                    break;
                }
                p.position=next;
            }
            if(p.type==Arrow&&!p.embedded)p.velocity=ArrowTickVelocity(p.velocity);
        }
        projectiles[i]=p;
    }
    projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const Projectile& p){return p.life<=0;}),projectiles.end());
}
std::vector<ArrowState> ArrowStates(){std::vector<ArrowState> result;for(const auto& p:projectiles)if(p.type==Arrow)result.push_back({p.position,Vector3Normalize(p.velocity),p.embedded,p.damage,p.critical});return result;}
void DrawItems() {
    for(const auto& p:projectiles) {
        if(p.type==Tnt){DrawCube(p.position,.9f,.9f,.9f,int(p.life*5)%2?Color{195,56,41,255}:RAYWHITE);DrawCubeWires(p.position,.91f,.91f,.91f,DARKBROWN);}
        else if(p.type==Arrow){DrawArrowModel(p.position,p.velocity);if(p.critical&&!p.embedded)DrawArrowSparkles(p.position,p.velocity,float(GetTime()));}
        else {Color c=p.type==Fireball?ORANGE:(p.type==EnderPearl?Color{39,155,142,255}:(p.type==Egg?BEIGE:RAYWHITE));DrawSphere(p.position,p.type==Fireball?.18f:.12f,c);}
    }
}
}
