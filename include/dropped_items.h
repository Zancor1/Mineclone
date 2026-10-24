#pragma once
#include "chest.h"
#include "world.h"
#include "arrow_collision.h"
#include <algorithm>
#include <cmath>
namespace game {
struct DroppedItem {ItemStack stack{};Vector3 position{},velocity{};float age=0,life=300;};
inline bool DropBlocked(Vector3 p){
    for(float x:{-.11f,.11f})for(float y:{-.11f,.11f})for(float z:{-.11f,.11f})
        if(ArrowSolidPoint(Vector3Add(p,{x,y,z})))return true;
    return false;
}
inline DroppedItem MakeDrop(ItemStack stack,Vector3 eye,Vector3 direction){
    return {stack,Vector3Add(eye,{0,-.2f,0}),Vector3Add(Vector3Scale(Vector3Normalize(direction),4.f),{0,1.5f,0})};
}
inline void StepDrop(DroppedItem& item,float dt){
    item.age+=dt;item.life-=dt;item.velocity.y-=16*dt;
    int steps=std::max(1,int(std::ceil(Vector3Length(item.velocity)*dt/.08f)));
    for(int step=0;step<steps;++step)for(int axis=0;axis<3;++axis){
        auto next=item.position;
        if(axis==0)next.x+=item.velocity.x*dt/steps;
        if(axis==1)next.y+=item.velocity.y*dt/steps;
        if(axis==2)next.z+=item.velocity.z*dt/steps;
        if(DropBlocked(next)){
            if(axis==0)item.velocity.x=0;
            if(axis==1){item.velocity.y=0;float friction=std::exp(-8.f*dt/steps);item.velocity.x*=friction;item.velocity.z*=friction;}
            if(axis==2)item.velocity.z=0;
        }else item.position=next;
    }
    if(item.position.y< -25||item.position.x<0||item.position.z<0||item.position.x>=Width||item.position.z>=Width)item.life=0;
}
inline bool CanPickUpDrop(const DroppedItem& item,Vector3 feet){
    auto torso=Vector3Add(feet,{0,.8f,0});
    if(item.age<.8f||Vector3Distance(torso,item.position)>1.35f)return false;
    for(int step=1;step<8;++step)if(ArrowSolidPoint(Vector3Lerp(torso,item.position,step/8.f)))return false;
    return true;
}
}
