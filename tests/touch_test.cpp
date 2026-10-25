#include "touch_controls.h"
#include <cstdio>
#include <cstdlib>
namespace {void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}}
int main(){using namespace game;
    constexpr int w=1280,h=720;TouchLayout layout(w,h);TouchControls input;
    auto step=[&](std::vector<ScreenTouch> touches,float dt=.016f,bool playing=true,bool back=false,bool canPlace=false){input.Update(touches,dt,w,h,playing,back);input.ResolveWorldTap(canPlace);};
    auto center=[](Rectangle r){return Vector2{r.x+r.width/2,r.y+r.height/2};};
    step({});Check(!input.enabled,"mouse-free frames do not activate touch controls");
    step({{11,{640,200}}});Check(input.enabled&&!input.Pressed(BindingAction::Attack)&&!input.Down(BindingAction::Use),"a real touch activates controls without firing immediately");
    step({{11,{640,200}}},.1f);step({});Check(input.Pressed(BindingAction::Attack)&&input.Down(BindingAction::Attack),"quick world tap punches exactly once on release");
    step({});Check(!input.Pressed(BindingAction::Attack)&&input.Released(BindingAction::Attack),"tap does not leave the attack held");
    step({{12,{640,200}}});for(int i=0;i<29;++i)step({{12,{640,200}}},.01f);
    Check(!input.Down(BindingAction::Attack)&&!input.Down(BindingAction::Use),"holding less than 0.3 seconds does not mine or place");step({{12,{640,200}}},.01f);
    Check(input.Down(BindingAction::Attack)&&input.Pressed(BindingAction::Attack)&&!input.Down(BindingAction::Use),"holding for 0.3 seconds starts mining without placing");
    step({{12,{640,200}}},.05f);Check(input.Down(BindingAction::Attack)&&!input.Pressed(BindingAction::Attack),"held mining has only one press edge");step({});Check(input.Released(BindingAction::Attack)&&!input.Pressed(BindingAction::Attack)&&!input.Pressed(BindingAction::Use),"release ends long mining without another hit or placement");
    step({{20,{640,200}}});step({{20,{680,210}}});Check(input.look.x>0&&input.look.y>0,"dragging controls camera look");
    for(int i=0;i<10;++i){step({{20,{680,210}}},.05f);}step({});Check(!input.Pressed(BindingAction::Attack)&&!input.Down(BindingAction::Use),"a look gesture never turns into a hit or a placement");
    Vector2 forwardRight{layout.pad.x+layout.cell*2.5f,layout.pad.y+layout.cell*.5f},jump=center(layout.buttons[0]);
    step({{31,forwardRight},{32,jump},{33,{700,200}}});step({{33,{725,200}},{32,jump},{31,forwardRight}});
    Check(input.Down(BindingAction::Forward)&&input.Down(BindingAction::Right)&&input.Down(BindingAction::Jump)&&input.look.x>0,"walking diagonally, jumping and looking work simultaneously with stable finger IDs");
    step({{33,{740,200}},{31,forwardRight}});Check(!input.Down(BindingAction::Jump)&&input.Down(BindingAction::Forward),"lifting one finger does not release another control");step({});
    step({{40,center(layout.Slot(5))}});Check(input.Pressed(BindingAction::Hotbar6),"touching hotbar slot selects it");step({});Check(!input.Pressed(BindingAction::Attack),"hotbar touches do not punch");
    step({{41,center(layout.buttons[2])}});Check(input.Down(BindingAction::Attack),"dedicated mining button supports continuous breaking");step({});
    step({{42,center(layout.buttons[1])}});Check(input.Down(BindingAction::Use)&&input.Pressed(BindingAction::Use),"dedicated use button starts bow, food or block use immediately");step({});
    step({{43,{640,200}}});step({{43,{640,200}}},.016f,false);Check(!input.menuPressed&&!input.Down(BindingAction::Use),"opening a menu cancels the world gesture without clicking into it");step({},.016f,false);
    step({{44,{500,300}},{45,{600,300}}},.016f,false);Check(input.menuPressed&&input.menuDown&&input.pointer.x==500,"menu pointer follows the primary touch");
    step({{45,{600,300}}},.016f,false);Check(input.menuReleased,"lifting the primary finger releases the menu click");step({{45,{600,300}}},.016f,false);Check(!input.menuPressed,"secondary held touch does not become an unintended menu click");step({},.016f,false);
    step({{46,{w-60.f,25}}},.016f,false,true);Check(input.Pressed(BindingAction::Pause),"touch back button closes inventory screens");step({},.016f,false);
    step({{47,{640,200}}},.016f,false);step({{47,{640,200}}},.016f,true);step({});Check(!input.Pressed(BindingAction::Attack),"a finger held across a menu transition cannot attack");
    for(auto size:{Vector2{640,360},Vector2{1280,720},Vector2{1920,1080}}){TouchLayout l(int(size.x),int(size.y));for(auto r:l.buttons)Check(r.x>=0&&r.y>=0&&r.x+r.width<=size.x&&r.y+r.height<=size.y,"touch buttons fit landscape screens");Check(!CheckCollisionRecs(l.pad,l.hotbar),"D-pad does not overlap hotbar");}
    step({{58,{640,200}}});step({},.016f,true,false,true);Check(input.Pressed(BindingAction::Use)&&!input.Pressed(BindingAction::Attack),"quick tap places a block when destination is valid");
    step({});Check(input.Released(BindingAction::Use)&&!input.Down(BindingAction::Use),"quick placement is a single click, not continuous use");
    Settings.touchLayout[2]={.6f,.4f,1.5f};TouchLayout custom(w,h);
    Check(std::abs(custom.buttons[0].width-layout.buttons[0].width*1.5f)<.01f,"custom button scales visually");
    step({});step({{60,center(custom.buttons[0])}});Check(input.Down(BindingAction::Jump),"moved button hit area follows saved layout");step({});
    Settings.touchLayout={};Settings.touchDpad=false;TouchLayout joystick(w,h);auto c=center(joystick.pad);
    step({{61,c}});Check(!input.Down(BindingAction::Forward)&&!input.Down(BindingAction::Sneak),"joystick center is neutral");
    step({{61,{c.x+joystick.pad.width*.4f,c.y-joystick.pad.height*.4f}}});Check(input.Down(BindingAction::Forward)&&input.Down(BindingAction::Right),"joystick supports diagonal motion");step({});
    Settings.touchDpad=true;
    std::puts("PASS: real-touch activation, 0.3-second use, tap attack, multi-touch, look, hotbar, menus and responsive controls");
}
