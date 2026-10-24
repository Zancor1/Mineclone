#pragma once
#include <raylib.h>
#include <algorithm>
namespace game {
struct ArrowState {Vector3 tip,direction;bool embedded;};
constexpr float ProjectileTick=.05f,ArrowSpeed=60.f,FireballSpeed=24.f;
inline Vector3 ArrowTickVelocity(Vector3 v){return {v.x*.99f,v.y*.99f-1.f,v.z*.99f};}
inline float BowPower(float seconds){float t=std::clamp(seconds,0.f,1.f);return (t*t+2*t)/3.f;}
}
