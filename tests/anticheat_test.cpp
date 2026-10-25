#include "anticheat.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace {void Check(bool value,const char* text){if(!value){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}}
int main(){using namespace game::ac;
    for(int n=1;n<=16;++n)Check(Confirmations(n)==(n<=3?1:n/3+1),"population thresholds");
    Budget motion;motion.Reset(0);for(int tick=1;tick<=200;++tick)Check(motion.Spend(tick*.05f,.42f,9),"legitimate movement and speed potion tolerance");
    motion.Reset(0);int accepted=0;for(int i=0;i<100;++i)accepted+=motion.Spend(0,1,9);Check(accepted==3,"packet spam cannot replenish movement allowance");
    Check(!motion.Spend(.01f,100,9),"teleport attempt rejected");Check(motion.Spend(.5f,2,9),"latency burst can recover without permanent penalty");
    Check(!EvidenceValid(Reason::Speed,std::numeric_limits<float>::infinity(),1)&&!EvidenceValid(Reason::Speed,1,2)&&!EvidenceValid(Reason(99),99,1),"malformed evidence rejected");
    Evidence e;e.serial=10;e.target=1;e.epoch=2;e.created=3;e.measured=100;e.limit=3;
    Check(!e.Confirm(1,6,3),"accused cannot confirm itself");Check(!e.Confirm(0,6,3),"six players require more than host confirmation");
    Check(!e.Confirm(2,6,3)&&!e.Confirm(2,6,3)&&!e.Confirm(2,6,3),"duplicate reports do not reach quorum");
    Check(e.Confirm(3,6,3),"third distinct observer reaches quorum");
    Evidence expired=e;expired.voters={};Check(!expired.Confirm(0,2,12),"expired episode cannot kick");
    Guard fresh;fresh.evidence=e;fresh.ResetMotion(20);Check(!fresh.evidence.Valid(20)&&fresh.hover==0,"respawn and teleport clear pending movement episodes");
    Guard server,observer;server.ResetMotion(0);observer.ResetMotion(0);
    for(int i=0;i<6;++i){float now=3+i*.05f;auto a=CheckMotion(server,now,.05f,100,0,0,9,14,false,true,false,false,false);auto b=CheckMotion(observer,now,.05f,100,0,0,9,14,false,true,false,false,false);Check(a.rejected==b.rejected&&a.reason==b.reason,"independent movement budgets agree on impossible steps");ObserveVerdict(observer,b,now);}
    Check(observer.suspectReason==int(Reason::Speed),"observer reaches its own detection after repeated samples");
    Guard legit;legit.ResetMotion(0);for(int i=1;i<=200;++i){auto v=CheckMotion(legit,i*.05f,.05f,.28f,0,0,9,14,false,true,false,false,false);ObserveVerdict(legit,v,i*.05f);Check(!v.rejected,"normal sprint is accepted by independent verifier");}Check(!legit.suspectReason,"normal sprint never produces an accusation");
    std::puts("PASS: anti-cheat thresholds, independent confirmations, expiry, movement budgets and malformed evidence");
}
