#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace game::ac {
// Population includes the accused, as agreed: 1..3 -> 1, 4..5 -> 2, 6..8 -> 3.
inline int Confirmations(int players){return players<=3?1:players/3+1;}
enum class Reason {Speed=1,Vertical,Flight,Reach,Mining,Flood,Collision};
inline bool EvidenceValid(Reason reason,float measured,float limit){
    if(!std::isfinite(measured)||!std::isfinite(limit)||measured<0||limit<=0)return false;
    switch(reason){
        case Reason::Speed:case Reason::Vertical:return limit>=.5f&&measured>limit+.05f;
        case Reason::Flight:return limit>=3.f&&measured>limit;
        case Reason::Reach:return limit>=3.3f&&measured>limit+.2f;
        case Reason::Collision:return limit==1&&measured>1;
        case Reason::Mining:return limit>=1.2f&&measured>limit;
        case Reason::Flood:return limit>=200&&measured>limit;
    }return false;
}
struct Budget {
    float credit=3,time=-1;
    void Reset(float now){credit=3;time=now;}
    bool Spend(float now,float distance,float speed){
        if(!std::isfinite(now)||!std::isfinite(distance)||!std::isfinite(speed)||distance<0||speed<0)return false;
        if(time<0)Reset(now);
        credit=std::min(std::max(3.f,speed*.5f),credit+std::clamp(now-time,0.f,.5f)*speed);time=now;
        if(distance>credit+.001f)return false;
        credit=std::max(0.f,credit-distance);return true;
    }
};
struct Evidence {
    int serial=0,target=-1,epoch=0,generation=0;Reason reason=Reason::Speed;
    float created=0,measured=0,limit=1;
    std::array<bool,16> voters{};
    bool Valid(float now)const{return serial>0&&now>=created&&now-created<=8&&EvidenceValid(reason,measured,limit);}
    bool Confirm(int reporter,int population,float now){
        if(!Valid(now)||reporter<0||reporter>=16||reporter==target||voters[reporter])return false;
        voters[reporter]=true;return int(std::count(voters.begin(),voters.end(),true))>=Confirmations(population);
    }
};
struct Guard {
    Budget horizontal,upward;float lastIssue=-100,lastSent=-100,hover=0,grace=0,window=0;
    int strikes=0,packets=0,issueReason=0,motionGeneration=0,observedGeneration=-1,observedEpoch=-1,suspectReason=0;float suspectUntil=0,motionStart=0;
    Evidence evidence;
    void ResetMotion(float now){motionStart=now;++motionGeneration;suspectReason=0;suspectUntil=0;horizontal.Reset(now);upward.Reset(now);hover=0;grace=now+2;strikes=0;evidence={};}
};
struct MotionVerdict {bool rejected=false;Reason reason=Reason::Speed;float measured=0,limit=1;};
inline MotionVerdict CheckMotion(Guard& g,float now,float elapsed,float distance,float rise,float verticalDelta,float speed,float ascent,bool knock,bool supported,bool collision,bool jump,bool below){
    auto xy=g.horizontal,up=g.upward;
    float allowed=std::min(std::max(3.f,speed*.5f),xy.credit+std::clamp(now-xy.time,0.f,.5f)*speed);
    if(!xy.Spend(now,distance,speed))return {true,Reason::Speed,distance,std::max(.5f,allowed)};
    float upward=std::min(std::max(3.f,ascent*.5f),up.credit+std::clamp(now-up.time,0.f,.5f)*ascent);
    if(!knock&&!up.Spend(now,rise,ascent))return {true,Reason::Vertical,rise,std::max(.5f,upward)};
    if(supported||knock||now<g.grace||below||std::abs(verticalDelta)>.08f)g.hover=0;else g.hover+=std::clamp(elapsed,0.f,.25f);
    float hoverLimit=jump?8.f:3.5f;if(g.hover>hoverLimit)return {true,Reason::Flight,g.hover,hoverLimit};
    if(collision)return {true,Reason::Collision,2,1};
    g.horizontal=xy;if(!knock)g.upward=up;else g.upward.Reset(now);
    return {};
}
inline void ObserveVerdict(Guard& g,const MotionVerdict& v,float now){
    if(!v.rejected||!EvidenceValid(v.reason,v.measured,v.limit))return;
    if((v.reason==Reason::Speed||v.reason==Reason::Vertical||v.reason==Reason::Collision)&&now<g.grace)return;
    if(now-g.lastIssue>5||g.issueReason!=int(v.reason))g.strikes=0;
    g.lastIssue=now;g.issueReason=int(v.reason);
    if(++g.strikes>=5){g.suspectReason=int(v.reason);g.suspectUntil=now+8;}
}
// Windows monitor. Other platforms keep server-side validation and confirmations.
bool StartMonitor();
void StopMonitor();
bool Detected();
void ShowWarning();
}
