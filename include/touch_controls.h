#pragma once
#include "settings.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace game {
struct ScreenTouch {int id;Vector2 position;};
struct TouchLayout {
    float cell;Rectangle pad,hotbar;
    std::array<Rectangle,7> buttons{};
    explicit TouchLayout(int width,int height){
        cell=std::clamp(std::min(width/16.f,height/7.5f),32.f,96.f);
        float slot=std::min({96.f,height*.105f,(width-24.f)/10.f}),barHeight=std::min(100.f,slot*1.12f);
        hotbar={(width-10*slot)/2.f,float(height)-barHeight-10,9*slot,barHeight};
        pad={14,hotbar.y-cell*3-16,cell*3,cell*3};
        float right=width-cell-18,bottom=hotbar.y-cell-18;
        buttons[0]={right,bottom,cell,cell}; // Jump
        buttons[1]={right,bottom-cell-10,cell,cell}; // Use
        buttons[2]={right-cell-10,bottom,cell,cell}; // Mine
        buttons[3]={14,pad.y-cell*.65f-10,cell*1.65f,cell*.58f}; // Sprint
        buttons[4]={width-cell-12.f,12,cell,cell*.65f}; // Pause
        buttons[5]={hotbar.x+hotbar.width+3,hotbar.y,slot*.8f,barHeight}; // Inventory
        buttons[6]={width-cell*2-22,12,cell,cell*.65f}; // Drop
        for(int i=0;i<9;++i){auto& r=Control(i);auto b=Settings.touchLayout[i];float cx=b.x<0?r.x+r.width/2:b.x*width,cy=b.y<0?r.y+r.height/2:b.y*height;
            r.width=std::min(float(width),r.width*b.scale);r.height=std::min(float(height),r.height*b.scale);
            r.x=std::clamp(cx-r.width/2,0.f,width-r.width);r.y=std::clamp(cy-r.height/2,0.f,height-r.height);}
        cell=pad.width/3;
    }
    Rectangle& Control(int i){return i==0?pad:i==1?hotbar:buttons[i-2];}
    Rectangle Slot(int i)const{float w=hotbar.width/9;return {hotbar.x+i*w,hotbar.y,w-3,hotbar.height};}
};
class TouchControls {
    struct Finger {int id=-1,role=-1;Vector2 start{},last{};float age=0;bool dragged=false,usingItem=false;};
    std::vector<Finger> fingers;
    std::array<bool,BindingCount> down{},previous{},pressed{},released{};
    int primary=-1;bool previousMenu=false,wasPlaying=false;
    float mouseGuard=0;bool worldTap=false;
public:
    bool enabled=false,mouseLook=false,playing=false,canGoBack=false,menuDown=false,menuPressed=false,menuReleased=false;
    Vector2 look{},pointer{};
    void Cancel(){worldTap=false;fingers.clear();down={};previous={};pressed={};released={};look={};primary=-1;previousMenu=menuDown=menuPressed=menuReleased=false;}
    // Resolve a released world tap after the current camera/target is known.
    void ResolveWorldTap(bool canPlace){if(!worldTap)return;worldTap=false;int action=int(canPlace?BindingAction::Use:BindingAction::Attack);down[action]=pressed[action]=previous[action]=true;}
    bool Down(BindingAction action)const{return down[int(action)];}
    bool Pressed(BindingAction action)const{return pressed[int(action)];}
    bool Released(BindingAction action)const{return released[int(action)];}
    bool SuppressMouse()const{return mouseGuard>0;}
    void Update(const std::vector<ScreenTouch>& touches,float dt,int width,int height,bool inGame,bool back=false){
        dt=std::clamp(dt,0.f,.1f);mouseGuard=std::max(0.f,mouseGuard-dt);if(!touches.empty()){enabled=true;mouseLook=false;mouseGuard=.2f;}
        worldTap=false;canGoBack=back;playing=inGame;look={};pressed={};released={};down={};menuPressed=menuReleased=false;
        if(inGame!=wasPlaying){Cancel();wasPlaying=inGame;for(auto touch:touches)fingers.push_back({touch.id,-2,touch.position,touch.position});}
        if(!inGame){
            auto found=std::find_if(touches.begin(),touches.end(),[&](auto t){return t.id==primary;});
            if(primary<0){auto fresh=std::find_if(touches.begin(),touches.end(),[&](auto t){return std::none_of(fingers.begin(),fingers.end(),[&](auto f){return f.id==t.id;});});if(fresh!=touches.end()){primary=fresh->id;found=fresh;}}
            menuDown=found!=touches.end();if(menuDown)pointer=found->position;
            menuPressed=menuDown&&!previousMenu;if(menuPressed&&back&&CheckCollisionPointRec(pointer,{float(width)-106,10,96,38}))pressed[int(BindingAction::Pause)]=true;menuReleased=!menuDown&&previousMenu;previousMenu=menuDown;if(!menuDown)primary=-1;
            for(auto touch:touches)if(std::none_of(fingers.begin(),fingers.end(),[&](auto f){return f.id==touch.id;}))fingers.push_back({touch.id,-2,touch.position,touch.position});
            fingers.erase(std::remove_if(fingers.begin(),fingers.end(),[&](auto f){return std::none_of(touches.begin(),touches.end(),[&](auto t){return t.id==f.id;});}),fingers.end());return;
        }
        TouchLayout layout(width,height);
        for(const auto& touch:touches){
            auto found=std::find_if(fingers.begin(),fingers.end(),[&](auto f){return f.id==touch.id;});
            bool fresh=found==fingers.end();
            if(fresh){int role=-1;if(CheckCollisionPointRec(touch.position,layout.pad))role=0;
                for(int i=0;i<7;++i)if(CheckCollisionPointRec(touch.position,layout.buttons[i]))role=1+i;
                if(CheckCollisionPointRec(touch.position,layout.hotbar))role=8;
                // A single finger owns looking/using; extra free fingers stay ignored.
                if(role<0&&std::any_of(fingers.begin(),fingers.end(),[](auto f){return f.role==9;}))role=-2;
                if(role==-1)role=9;
                fingers.push_back({touch.id,role,touch.position,touch.position});found=fingers.end()-1;
            }
            auto& f=*found;f.age+=fresh?0:dt;Vector2 delta{touch.position.x-f.last.x,touch.position.y-f.last.y};
            if(std::hypot(touch.position.x-f.start.x,touch.position.y-f.start.y)>std::max(9.f,height*.014f))f.dragged=true;
            f.last=touch.position;
            if(f.role==0&&!Settings.touchDpad){float dx=(touch.position.x-layout.pad.x-layout.pad.width/2)/(layout.pad.width/2),dy=(touch.position.y-layout.pad.y-layout.pad.height/2)/(layout.pad.height/2);
                if(std::hypot(dx,dy)>.22f){if(dy<-.3f)down[int(BindingAction::Forward)]=true;if(dy>.3f)down[int(BindingAction::Backward)]=true;if(dx<-.3f)down[int(BindingAction::Left)]=true;if(dx>.3f)down[int(BindingAction::Right)]=true;}}
            else if(f.role==0){if(CheckCollisionPointRec(touch.position,layout.pad)){int x=std::clamp(int((touch.position.x-layout.pad.x)/layout.cell),0,2),y=std::clamp(int((touch.position.y-layout.pad.y)/layout.cell),0,2);
                if(y==0)down[int(BindingAction::Forward)]=true;
                if(y==2)down[int(BindingAction::Backward)]=true;
                if(x==0)down[int(BindingAction::Left)]=true;
                if(x==2)down[int(BindingAction::Right)]=true;
                if(x==1&&y==1)down[int(BindingAction::Sneak)]=true;}}
            else if(f.role>=1&&f.role<=7){constexpr BindingAction actions[]={BindingAction::Jump,BindingAction::Use,BindingAction::Attack,BindingAction::Sprint,BindingAction::Pause,BindingAction::Inventory,BindingAction::Drop};down[int(actions[f.role-1])]=true;}
            else if(f.role==8){int slot=std::clamp(int((touch.position.x-layout.hotbar.x)/(layout.hotbar.width/9)),0,8);if(fresh)pressed[int(BindingAction::Hotbar1)+slot]=true;}
            else if(f.role==9){if(f.dragged){float scale=720.f/std::max(240,height);look.x+=delta.x*scale;look.y+=delta.y*scale;}
                if(!f.dragged&&f.age+1e-5f>=.3f)f.usingItem=true;
                if(f.usingItem)down[int(BindingAction::Attack)]=true;
            }
        }
        for(const auto& f:fingers)if(std::none_of(touches.begin(),touches.end(),[&](auto t){return t.id==f.id;})){
            if(f.role==9&&!f.dragged&&!f.usingItem&&f.age<.3f)worldTap=true;
        }
        fingers.erase(std::remove_if(fingers.begin(),fingers.end(),[&](auto f){return std::none_of(touches.begin(),touches.end(),[&](auto t){return t.id==f.id;});}),fingers.end());
        for(int i=0;i<BindingCount;++i){pressed[i]=pressed[i]||(down[i]&&!previous[i]);released[i]=!down[i]&&previous[i];previous[i]=down[i];}
    }
    void Draw(int width,int height)const{
        if(!enabled)return;
        if(!playing){if(canGoBack){Rectangle back{float(width)-106,10,96,38};DrawRectangleRec(back,Fade(BLACK,.7f));DrawRectangleLinesEx(back,2,GRAY);DrawText("Voltar",int(back.x+16),20,18,WHITE);}return;}
        TouchLayout l(width,height);
        auto box=[&](Rectangle r,bool held){DrawRectangleRec(r,Fade(held?WHITE:BLACK,held?.42f:.26f));DrawRectangleLinesEx(r,2,Fade(WHITE,.55f));};
        if(!Settings.touchDpad){Vector2 c{l.pad.x+l.pad.width/2,l.pad.y+l.pad.height/2};float r=l.pad.width/2;DrawCircleV(c,r,Fade(BLACK,.3f));DrawCircleLines(int(c.x),int(c.y),r,Fade(WHITE,.6f));Vector2 knob=c;for(auto f:fingers)if(f.role==0){float dx=f.last.x-c.x,dy=f.last.y-c.y,n=std::max(1.f,std::hypot(dx,dy)/(r*.65f));knob={c.x+dx/n,c.y+dy/n};}DrawCircleV(knob,r*.32f,Fade(WHITE,.45f));}
        else for(int y=0;y<3;++y)for(int x=0;x<3;++x){Rectangle r{l.pad.x+x*l.cell+2,l.pad.y+y*l.cell+2,l.cell-4,l.cell-4};bool held=(y==0&&Down(BindingAction::Forward))||(y==2&&Down(BindingAction::Backward))||(x==0&&Down(BindingAction::Left))||(x==2&&Down(BindingAction::Right))||(x==1&&y==1&&Down(BindingAction::Sneak));box(r,held);Vector2 c{r.x+r.width/2,r.y+r.height/2};float a=l.cell*.18f;
            if(x==1&&y==1)DrawCircleLines(int(c.x),int(c.y),a,Fade(WHITE,.8f));
            else{Vector2 d{float(x-1),float(y-1)};float n=std::hypot(d.x,d.y);d.x/=n;d.y/=n;DrawTriangle({c.x+d.x*a,c.y+d.y*a},{c.x-d.x*a+d.y*a,c.y-d.y*a-d.x*a},{c.x-d.x*a-d.y*a,c.y-d.y*a+d.x*a},Fade(WHITE,.8f));}}
        const char* labels[]={"Pular","Usar","Picar","Correr","II","...","Q"};constexpr BindingAction actions[]={BindingAction::Jump,BindingAction::Use,BindingAction::Attack,BindingAction::Sprint,BindingAction::Pause,BindingAction::Inventory,BindingAction::Drop};
        for(int i=0;i<7;++i){auto r=l.buttons[i];box(r,Down(actions[i]));int font=std::clamp(int(l.cell*.23f),11,18);DrawText(labels[i],int(r.x+(r.width-MeasureText(labels[i],font))/2),int(r.y+(r.height-font)/2),font,WHITE);}
    }
};
inline TouchControls Touch;
inline Vector2 PointerPosition(){return Touch.enabled&&!Touch.playing&&(Touch.menuDown||Touch.menuReleased)?Touch.pointer:GetMousePosition();}
inline bool PointerPressed(int button){return (button==MOUSE_BUTTON_LEFT&&Touch.menuPressed)||(!Touch.SuppressMouse()&&IsMouseButtonPressed(button));}
inline bool PointerDown(int button){return (button==MOUSE_BUTTON_LEFT&&Touch.menuDown)||(!Touch.SuppressMouse()&&IsMouseButtonDown(button));}
std::vector<ScreenTouch> PollScreenTouches();
void ShutdownScreenTouches();
}
