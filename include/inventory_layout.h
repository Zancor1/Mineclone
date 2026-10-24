#pragma once
#include "chest.h"
#include "settings.h"
#include <algorithm>
namespace game {
inline float InventoryScale(){return float(std::clamp(std::min((GetScreenWidth()-24)/176,(GetScreenHeight()-32)/166),1,3));}
inline Rectangle ClassicInventoryPanel(){float s=InventoryScale();return {(GetScreenWidth()-176*s)/2,(GetScreenHeight()-166*s)/2,176*s,166*s};}
inline Rectangle InventoryArea(float x,float y,float w,float h){auto p=ClassicInventoryPanel();float s=InventoryScale();return {p.x+x*s,p.y+y*s,w*s,h*s};}
inline Rectangle ClassicPlayerSlot(int index){return index<9?InventoryArea(8+index*18,142,16,16):InventoryArea(8+(index-9)%9*18,84+(index-9)/9*18,16,16);}
inline Rectangle ClassicArmorSlot(int index){return InventoryArea(8,8+index*18,16,16);}
inline Rectangle ClassicCraftSlot(int index){return InventoryArea(98+(index%2)*18,18+(index/2)*18,16,16);}
inline Rectangle ClassicCraftOutput(){return InventoryArea(154,28,16,16);}
inline Rectangle CreativeItemsButton(){return InventoryArea(97,61,72,15);}
inline int PressedHotbarBinding(){for(int i=0;i<9;++i)if(ActionPressed(BindingAction(int(BindingAction::Hotbar1)+i)))return i;return -1;}
inline void DrawInventorySlot(Rectangle r,bool hover=false){float s=InventoryScale();DrawRectangleRec({r.x-s,r.y-s,r.width+2*s,r.height+2*s},{55,55,55,255});DrawRectangleRec({r.x,r.y,r.width+s,r.height+s},{242,242,242,255});DrawRectangleRec(r,hover?Color{178,178,178,255}:Color{139,139,139,255});}
inline void DrawClassicInventoryPanel(){auto p=ClassicInventoryPanel();float s=InventoryScale();DrawRectangleRec(p,{198,198,198,255});DrawRectangleLinesEx(p,s,{55,55,55,255});DrawLineEx({p.x+s,p.y+s},{p.x+p.width-s,p.y+s},s,WHITE);DrawLineEx({p.x+s,p.y+s},{p.x+s,p.y+p.height-s},s,WHITE);}
inline void DrawInventoryPlayer(Color shirt={25,163,180,255}){
    DrawRectangleRec(InventoryArea(26,8,52,70),BLACK);
    auto box=[](float x,float y,float w,float h,Color c){DrawRectangleRec(InventoryArea(x,y,w,h),c);};
    box(40,15,16,16,{190,139,103,255});box(40,15,16,5,{64,43,30,255});box(40,20,3,5,{64,43,30,255});box(53,20,3,5,{64,43,30,255});
    box(43,23,3,2,WHITE);box(50,23,3,2,WHITE);box(44,23,2,2,{59,55,105,255});box(50,23,2,2,{59,55,105,255});box(45,28,7,2,{91,55,39,255});
    box(38,31,20,23,shirt);box(31,32,7,11,shirt);box(58,32,7,11,shirt);box(31,43,7,16,{190,139,103,255});box(58,43,7,16,{190,139,103,255});
    box(38,54,9,19,{58,54,139,255});box(49,54,9,19,{47,44,116,255});box(38,70,9,4,{66,67,72,255});box(49,70,9,4,{56,57,63,255});
}
}
