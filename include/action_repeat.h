#pragma once
#include <algorithm>
namespace game {
struct PlacementRepeat {
    float remaining=0;
    void Reset(){remaining=0;}
    bool Update(bool pressed,bool held,float dt){
        if(pressed){remaining=.2f;return true;}
        if(!held){Reset();return false;}
        remaining-=std::max(0.f,dt);
        if(remaining>0)return false;
        remaining=.2f;return true;
    }
};
}
