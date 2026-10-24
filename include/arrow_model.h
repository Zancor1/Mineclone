#pragma once
#include <raylib.h>
#include <raymath.h>
namespace game {
inline void DrawArrowModel(Vector3 tip,Vector3 direction){
    if(Vector3Length(direction)<.001f)direction={0,0,-1};
    direction=Vector3Normalize(direction);
    Vector3 side=Vector3Normalize(Vector3CrossProduct(direction,std::abs(direction.y)>.9f?Vector3{1,0,0}:Vector3{0,1,0}));
    Vector3 up=Vector3CrossProduct(side,direction);
    auto along=[&](float distance){return Vector3Add(tip,Vector3Scale(direction,distance));};
    DrawCylinderEx(along(-.85f),along(-.11f),.016f,.016f,4,{133,92,49,255});
    DrawCylinderEx(along(-.14f),tip,.065f,0,4,{181,190,195,255});
    for(auto axis:{side,up,Vector3Negate(side),Vector3Negate(up)}){
        auto a=along(-.82f),b=along(-.55f),c=Vector3Add(along(-.74f),Vector3Scale(axis,.105f));
        DrawTriangle3D(a,b,c,RAYWHITE);DrawTriangle3D(c,b,a,RAYWHITE);
    }
}
}
