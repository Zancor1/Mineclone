#pragma once
#include <raylib.h>
#include <algorithm>
#include <cmath>
namespace game {
struct PrimedTntState {Vector3 position;float remaining;};
constexpr float TntFuse=5.f;
inline float TntPulseScale(float remaining){float elapsed=std::clamp(TntFuse-remaining,0.f,TntFuse);return .98f+.09f*std::sin(2*PI*(1.5f*elapsed+.7f*elapsed*elapsed));}
struct ArrowState {Vector3 tip,direction;bool embedded;float damage=0;bool critical=false;};
constexpr float ProjectileTick=.05f,ArrowSpeed=60.f,FireballSpeed=24.f;
inline Vector3 ArrowTickVelocity(Vector3 v){return {v.x*.99f,v.y*.99f-1.f,v.z*.99f};}
constexpr float BowShotDelay=.3f;
inline int BowStage(float seconds){return seconds>=.9f?3:seconds>.1f?2:1;}
inline float BowPower(float seconds){return seconds>=.9f?1.f:seconds<=.1f?.14f:.3f+.5f*std::clamp((seconds-.1f)/.8f,0.f,1.f);}
inline float BowDamage(float seconds,unsigned roll){return BowStage(seconds)==1?1.f:BowStage(seconds)==2?2.f:seconds>=1.f&&roll%100<10?6.f:4.f;}
}
