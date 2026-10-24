#pragma once
#include "map_blocks.h"
#include <raymath.h>
#include <cmath>
namespace game {
inline bool ArrowSolidPoint(Vector3 p){
    auto boxes=CollisionBoxesAt(int(std::floor(p.x)),int(std::floor(p.y)),int(std::floor(p.z)));
    for(int i=0;i<boxes.count;++i){auto b=boxes.boxes[i];if(p.x>=b.min.x&&p.x<b.max.x&&p.y>=b.min.y&&p.y<b.max.y&&p.z>=b.min.z&&p.z<b.max.z)return true;}
    return false;
}
inline Vector3 EmbeddedArrowTip(Vector3 from,Vector3 inside){
    Vector3 a=from,b=inside;for(int i=0;i<10;++i){auto mid=Vector3Scale(Vector3Add(a,b),.5f);if(ArrowSolidPoint(mid))b=mid;else a=mid;}
    return Vector3Add(b,Vector3Scale(Vector3Normalize(Vector3Subtract(inside,from)),.055f));
}
}
