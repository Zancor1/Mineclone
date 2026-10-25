#include "texture_atlas.h"
#include "touch_controls.h"
#include "cosmetics.h"
#include "bedwars.h"
#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace game::cosmetics {
namespace {
constexpr Color colors[]={WHITE,{210,67,67,255},{67,110,210,255},{67,180,90,255},{230,204,65,255},{64,204,204,255},{230,230,235,255},{223,116,178,255},{105,109,119,255},{237,145,49,255},{150,84,208,255},{71,55,43,255},{40,43,50,255}};
const char* names[]={"Padrao","Vermelha","Azul","Verde","Amarela","Ciano","Branca","Rosa","Cinza","Laranja","Roxa","Marrom","Preta"};
const char* categories[]={"Espadas","Picaretas","Machados"};
constexpr Block icons[]={IronSword,IronPickaxe,IronAxe};
bool opened=false,waiting=false;int category=0,result=-1;double start=0;
double Now(){return IsWindowReady()?GetTime():0;}
Rectangle Panel(){float w=std::min(720.f,GetScreenWidth()-24.f),h=std::min(530.f,GetScreenHeight()-30.f);return {(GetScreenWidth()-w)/2,(GetScreenHeight()-h)/2,w,h};}
Rectangle Tab(int i){auto p=Panel();return {p.x+16+i*(p.width-32)/3,p.y+45,(p.width-38)/3,32};}
Rectangle Cell(int i){auto p=Panel();float w=(p.width-42)/4,h=(p.height-150)/4;return {p.x+16+(i%4)*(w+3),p.y+90+(i/4)*(h+3),w,h};}
Rectangle Back(){auto p=Panel();return {p.x+16,p.y+p.height-42,p.width-32,30};}
bool Spinning(){return result>0&&Now()-start<4.3;}
void Button(Rectangle r,const std::string& text,Color color=WHITE){bool hover=CheckCollisionPointRec(PointerPosition(),r);DrawRectangleRec(r,hover?Color{83,88,103,255}:Color{48,52,65,255});DrawRectangleLinesEx(r,1,GRAY);int size=std::max(12,std::min(18,int(r.height*.35f)));DrawText(text.c_str(),int(r.x+(r.width-MeasureText(text.c_str(),size))/2),int(r.y+(r.height-size)/2),size,color);}
void Icon(Texture2D atlas,Rectangle rect,int type,int color){float size=std::min(rect.width,rect.height)*.65f;DrawTexturePro(atlas,AtlasTile(atlas,BlockTile(icons[type])),{rect.x+(rect.width-size)/2,rect.y+4,size,size},{0,0},0,ColorFor(color));}
}
int Category(Block item){if(Sword(item))return 0;if(Pickaxe(item))return 1;if(ValidBlock(item)&&Blocks[item].toolKind==3)return 2;return -1;}
Color ColorFor(int skin){return colors[std::clamp(skin,0,progression::SkinCount)];}
const char* ColorName(int skin){return names[std::clamp(skin,0,progression::SkinCount)];}
const char* CategoryName(int type){return categories[std::clamp(type,0,2)];}
void Reset(){opened=waiting=false;category=0;result=-1;start=0;}
void Toggle(){opened=!opened;}
bool Opened(){return opened||waiting||result>0;}
void AwaitReward(){waiting=true;start=Now();}
void CancelWaiting(){waiting=false;}
void Reward(int reward){if(reward<1||reward>36)return;waiting=false;opened=true;result=reward;start=Now();}
bool Input(){
    if(waiting){if(Now()-start>8)waiting=false;return true;}
    if(result>0){if(!Spinning()&&PointerPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(PointerPosition(),Back()))result=-1;return true;}
    if(!opened)return false;
    if(PointerPressed(MOUSE_BUTTON_LEFT)){
        auto mouse=PointerPosition();if(CheckCollisionPointRec(mouse,Back())){opened=false;return true;}
        for(int i=0;i<3;++i)if(CheckCollisionPointRec(mouse,Tab(i))){category=i;return true;}
        for(int i=0;i<=progression::SkinCount;++i)if(CheckCollisionPointRec(mouse,Cell(i))){bw::EquipSkin(category,i);return true;}
    }
    return true;
}
void Draw(Texture2D atlas){
    auto p=Panel();DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.8f));DrawRectangleRec(p,{24,27,36,255});DrawRectangleLinesEx(p,2,{107,115,139,255});
    DrawText(result>0?"ABRINDO CAIXA":"INVENTARIO DE SKINS",int(p.x+16),int(p.y+14),22,GOLD);
    if(waiting){DrawText("Aguardando sorteio do host...",int(p.x+24),int(p.y+100),20,WHITE);return;}
    if(result>0){
        float t=std::clamp(float((Now()-start)/4.3),0.f,1.f),ease=1-std::pow(1-t,4.f);
        int target=28;float card=std::min(114.f,p.width/4.f),center=p.x+p.width/2,travel=(target+.5f)*card*ease;
        BeginScissorMode(int(p.x+12),int(p.y+100),int(p.width-24),160);
        for(int i=0;i<34;++i){float x=center+i*card-travel;int reward=i==target?result:progression::RewardForRoll(uint32_t(i*71+result*17));Rectangle r{x,p.y+105,card-5,145};DrawRectangleRec(r,{45,49,63,255});Icon(atlas,{r.x,r.y,r.width,100},progression::RewardCategory(reward),progression::RewardColor(reward));DrawRectangle(int(r.x),int(r.y+138),int(r.width),5,ColorFor(progression::RewardColor(reward)));}
        EndScissorMode();DrawLineEx({center,p.y+93},{center,p.y+265},3,GOLD);
        if(t>=1){int c=progression::RewardCategory(result),color=progression::RewardColor(result);std::string name=std::string(CategoryName(c))+" - "+ColorName(color);DrawText(name.c_str(),int(center-MeasureText(name.c_str(),22)/2),int(p.y+295),22,ColorFor(color));DrawText("Skin salva! Equipe no inventario. Repetidas sao possiveis.",int(p.x+16),int(p.y+335),16,WHITE);Button(Back(),"Continuar");}
        else DrawText("Caixa gratuita - sem chaves ou compras",int(p.x+20),int(p.y+300),17,GRAY);
        DrawText("Comum 72% | Rara 21% | Epica 6% | Lendaria 1%",int(p.x+16),int(p.y+375),15,GRAY);
        return;
    }
    auto profile=bw::Progress();for(int i=0;i<3;++i)Button(Tab(i),CategoryName(i),i==category?GOLD:WHITE);
    for(int i=0;i<=progression::SkinCount;++i){auto r=Cell(i);bool owned=(profile.unlocked[category]&(1u<<i))!=0,equipped=profile.skins[category]==i;DrawRectangleRec(r,owned?Color{46,51,65,255}:Color{31,33,41,255});Icon(atlas,{r.x,r.y,r.width,r.height-20},category,i);if(!owned)DrawRectangleRec(r,Fade(BLACK,.65f));DrawRectangleLinesEx(r,equipped?2:1,equipped?GOLD:GRAY);DrawText(owned?ColorName(i):"Bloqueada",int(r.x+7),int(r.y+r.height-21),15,owned?WHITE:GRAY);}
    Button(Back(),"Voltar");
}
}
