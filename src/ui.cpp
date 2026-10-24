#include "ui.h"
#include "inventory_layout.h"
#include "bedwars.h"
#include "renderer.h"
#include "settings.h"
#include "chest.h"
#include "crafting.h"
#include "survival.h"
#include "items.h"
#include "network.h"
#include <cmath>
#include <algorithm>
#include <utility>
#include <cctype>
#include <cstdlib>
namespace game {
namespace {
struct RemoteSlot {int index;Rectangle rect;};
std::vector<RemoteSlot> RemoteSlots(){
    std::vector<RemoteSlot> list;bool own=net::PlayerInventoryOpen();int count=int(net::WindowSlots().size());
    int top=own?0:std::max(0,count-36),topRows=(top+8)/9;
    float size=std::min(40.f,float(GetScreenHeight()-100)/(own?10:topRows+6));
    float x=GetScreenWidth()/2.f-size*4.5f,y=GetScreenHeight()/2.f-(own?8:topRows+4)*size*.5f;
    auto add=[&](int index,float col,float row){if(index<count)list.push_back({index,{x+col*size,y+row*size,size-2,size-2}});};
    if(own){
        for(int i=0;i<4;++i)add(5+i,0,float(i));
        for(int i=0;i<4;++i)add(1+i,5+i%2,float(i/2));
        add(0,8,.5f);
        for(int i=0;i<27;++i)add(9+i,float(i%9),float(4+i/9));
        for(int i=0;i<9;++i)add(36+i,float(i),7.3f);
    }else{
        for(int i=0;i<top;++i)add(i,float(i%9),float(i/9));
        for(int i=0;i<27;++i)add(top+i,float(i%9),topRows+.4f+i/9);
        for(int i=0;i<9;++i)add(top+27+i,float(i),topRows+3.7f);
    }
    return list;
}
RenderTexture2D CreateSkywarsPreview(){
    RenderTexture2D preview=LoadRenderTexture(360,210);
    Camera3D camera{{43,36,43},{0,0,0},{0,1,0},43,CAMERA_PERSPECTIVE};
    BeginTextureMode(preview);ClearBackground({125,177,213,255});BeginMode3D(camera);
    auto island=[](float x,float z,int radius){
        for(int y=-4;y<=0;++y){int r=std::max(1,radius+y/2);
            for(int dx=-r;dx<=r;++dx)for(int dz=-r;dz<=r;++dz){
                if(dx*dx+dz*dz>r*r+1)continue;
                DrawCube({x+dx,float(y),z+dz},1,1,1,y==0?Color{91,145,61,255}:y==-1?Color{123,88,60,255}:Color{112,117,123,255});
            }
        }
    };
    island(0,0,6);
    for(int i=0;i<8;++i){float angle=i*2*PI/8,x=17*std::cos(angle),z=17*std::sin(angle);island(x,z,3);
        DrawCube({x,1.6f,z},.8f,2.2f,.8f,{103,76,46,255});
        DrawCube({x,3.3f,z},3.5f,2,3.5f,{55,109,53,255});
        DrawCube({x+1.5f,.9f,z+1.5f},.8f,.8f,.8f,{162,106,42,255});
    }
    for(int i=0;i<4;++i){float x=i%2?3.f:-3.f,z=i/2?3.f:-3.f;DrawCube({x,2,z},1,3,1,{166,170,168,255});}
    DrawCube({0,3.8f,0},8,.7f,8,{151,158,162,255});
    DrawCube({0,1,0},1.4f,1,1.4f,{191,135,43,255});
    EndMode3D();EndTextureMode();return preview;
}
struct ServerBrowserLayout {
    Rectangle left,list,right,fields[5],refresh,create,join,back,onlyOpen;
    ServerBrowserLayout(){
        float w=float(GetScreenWidth()),h=float(GetScreenHeight()),leftWidth=std::clamp(w*.18f,150.f,230.f),rightWidth=std::clamp(w*.22f,165.f,290.f);
        left={16,74,leftWidth,h-202};right={w-rightWidth-16,74,rightWidth,h-202};list={left.x+left.width+12,74,w-leftWidth-rightWidth-56,h-202};
        for(int i=0;i<3;++i)fields[i]={left.x+12,left.y+44+i*64,left.width-24,28};
        fields[3]={left.x+12,left.y+238,left.width-24,28};onlyOpen={left.x+12,left.y+204,left.width-24,22};
        fields[4]={100,h-112,w-116,30};
        float buttonWidth=(w-68)/4;refresh={16,h-65,buttonWidth,38};create={28+buttonWidth,h-65,buttonWidth,38};join={40+2*buttonWidth,h-65,buttonWidth,38};back={52+3*buttonWidth,h-65,buttonWidth,38};
    }
};
std::string LowerText(std::string value){for(char& c:value)c=char(std::tolower(static_cast<unsigned char>(c)));return value;}
std::vector<const LanRoom*> FilterRooms(const std::string& search,const std::string& map,const std::string& ping,bool onlyOpen){
    std::vector<const LanRoom*> result;auto nameFilter=LowerText(search),mapFilter=LowerText(map);int limit=ping.empty()?0:std::atoi(ping.c_str());
    for(const auto& room:bw::Rooms())if(LowerText(room.name).find(nameFilter)!=std::string::npos&&LowerText(room.map).find(mapFilter)!=std::string::npos&&(!onlyOpen||room.joinable)&&(!limit||(room.ping>=0&&room.ping<=limit)))result.push_back(&room);
    std::stable_sort(result.begin(),result.end(),[](const LanRoom* a,const LanRoom* b){return a->ping<b->ping;});return result;
}
bool JoinableAddress(const std::string& address){if(address.empty())return false;for(const auto& room:bw::Rooms())if(room.address==address)return room.joinable&&room.protocol==LanProtocolVersion;return true;}
void BrowserText(const std::string& text,Rectangle rect,int size,Color color){BeginScissorMode(int(rect.x),int(rect.y),int(rect.width),int(rect.height));DrawText(text.c_str(),int(rect.x+6),int(rect.y+5),size,color);EndScissorMode();}
Rectangle TeamButton(int team){float x=GetScreenWidth()/2.f-300,y=GetScreenHeight()/2.f-105;return {x+(team%2)*306,y+(team/2)*56,294,46};}
Rectangle TeamBack(){return {GetScreenWidth()/2.f-150,GetScreenHeight()/2.f+130,300,40};}
Rectangle MapCard(int index,int count){int columns=std::min(count,3),rows=(count+2)/3;float width=std::min(count==2?760.f:1100.f,float(GetScreenWidth()-40)),card=(width-12*(columns-1))/columns,height=std::min(300.f,(GetScreenHeight()-176.f-12*(rows-1))/rows);return {(GetScreenWidth()-width)/2+(index%columns)*(card+12),82+(index/columns)*(height+12),card,height};}
Rectangle MapPageButton(bool next){return {GetScreenWidth()/2.f+(next?80:-230),float(GetScreenHeight()-59),150,32};}

Rectangle MinigameCard(int i){
    float width=std::min(860.f,float(GetScreenWidth()-48)),gap=24,card=(width-gap)/2;
    return {(GetScreenWidth()-width)/2+i*(card+gap),GetScreenHeight()/2.f-135,card,270};
}
Rectangle MinigamesBack(){return {GetScreenWidth()/2.f-120,GetScreenHeight()/2.f+160,240,40};}
Rectangle MainButton(int i){return {GetScreenWidth()/2.f-150,GetScreenHeight()/2.f-65+i*46.f,300,40};}
Rectangle Panel() {return {GetScreenWidth()/2.f-330,GetScreenHeight()/2.f-215,660,430};}
Rectangle Slot(int i,float y,float left) {return {left+i*68,y,62,70};}
void Icon(Texture2D atlas,Rectangle slot,Block b) {
    if(b==Air) return;
    int tile=BlockTile(b);
    DrawRectangle(int(slot.x+14),int(slot.y+9),34,34,{68,72,75,255});
    DrawTexturePro(atlas,{float(tile*TileSize),0,TileSize,TileSize},{slot.x+14,slot.y+9,34,34},{0,0},0,WHITE);
    const char* label=Names[b];
    if(b>=WoodenSword&&b<=IronSword) label=b==WoodenSword?"Esp. madeira":(b==StoneSword?"Esp. pedra":"Esp. ferro");
    if(b>=WoodenPickaxe&&b<=IronPickaxe) label=b==WoodenPickaxe?"Pic. madeira":(b==StonePickaxe?"Pic. pedra":"Pic. ferro");
    int font=12;while(font>9&&MeasureText(label,font)>slot.width-4) --font;
    std::string caption=label;
    if(MeasureText(caption.c_str(),font)>slot.width-4) {
        while(!caption.empty()&&MeasureText((caption+"...").c_str(),font)>slot.width-4) caption.pop_back();
        caption+="...";
    }
    DrawText(caption.c_str(),int(slot.x+(slot.width-MeasureText(caption.c_str(),font))/2),int(slot.y+50),font,RAYWHITE);
}
bool Hover(Rectangle r) {return CheckCollisionPointRec(GetMousePosition(),r);}
int VisibleWorldRows() {return std::max(1,(GetScreenHeight()-230)/52);}
Rectangle WorldRow(int i) {return {GetScreenWidth()/2.f-310,105.f+i*52,620,46};}
Rectangle DeleteButton(int row) {
    Rectangle r=WorldRow(row);return {r.x+r.width-150,r.y+5,142,36};
}
Rectangle WorldButton(bool back) {
    return {GetScreenWidth()/2.f+(back?10.f:-310.f),float(GetScreenHeight()-92),300,44};
}
Rectangle PauseButton(int i) {
    return {GetScreenWidth()/2.f-210,GetScreenHeight()/2.f-60+i*64.f,420,48};
}
Rectangle OptionsPanel() {return {GetScreenWidth()/2.f-320,GetScreenHeight()/2.f-224,640,448};}
Rectangle OptionControl(int i){auto p=OptionsPanel();if(i==0)return {p.x+20,p.y+48,292,36};if(i==4)return {p.x+328,p.y+48,292,36};if(i==1)return {p.x+20,p.y+122,292,24};if(i==2)return {p.x+328,p.y+122,292,24};if(i==7)return {p.x+20,p.y+205,292,24};if(i==6)return {p.x+328,p.y+205,292,24};if(i==5)return {p.x+20,p.y+288,600,24};if(i==9)return {p.x+20,p.y+321,600,32};if(i==8)return {p.x+20,p.y+365,292,40};return {p.x+328,p.y+365,292,40};}
Rectangle BindingRow(int i){return {GetScreenWidth()/2.f-300,90.f+i*38,600,34};}
void Slider(Rectangle r,float value) {
    DrawRectangleRec(r,{29,29,29,255});DrawRectangleLinesEx(r,2,{105,105,105,255});
    DrawRectangle(int(r.x+3),int(r.y+r.height/2-2),int((r.width-6)*value),4,{148,163,186,255});
    Rectangle knob{r.x+3+value*(r.width-20),r.y+2,14,r.height-4};
    DrawRectangleRec(knob,Hover(r)?RAYWHITE:LIGHTGRAY);DrawRectangleLinesEx(knob,1,GRAY);
}
Rectangle ChestPanel() {return {GetScreenWidth()/2.f-214,GetScreenHeight()/2.f-214,428,428};}
Rectangle ChestSlot(int i,int group) {
    Rectangle p=ChestPanel();float top=group==0?38.f:(group==1?204.f:344.f);
    return {p.x+16+(i%9)*44,p.y+top+(i/9)*44,40,40};
}
Rectangle InventoryPanel() {return ClassicInventoryPanel();}
Rectangle PlayerSlot(int i,bool bar){return ClassicPlayerSlot(bar?i:i+9);}
Rectangle CraftSlot(int i,int width){if(width==2)return ClassicCraftSlot(i);Rectangle p=ChestPanel();return {p.x+50+(i%width)*44,p.y+38+(i/width)*44,40,40};}
Rectangle CraftOutput(int width){if(width==2)return ClassicCraftOutput();auto p=ChestPanel();return {p.x+310,p.y+82,40,40};}
Rectangle FurnaceSlot(int i) {Rectangle p=ChestPanel();return {p.x+(i==2?310.f:130.f),p.y+(i==0?38.f:(i==1?126.f:82.f)),40,40};}
void TakeFurnaceOutput(ItemStack& slot,bool right) {
    if(!slot.count||(CursorStack.count&&CursorStack.block!=slot.block)) return;
    int amount=std::min(right?1:int(slot.count),StackCapacity(slot.block)-int(CursorStack.count));
    if(amount<=0) return;
    CursorStack.block=slot.block;CursorStack.count+=amount;slot.count-=amount;
    if(!slot.count) slot={};
}
void StackIcon(Texture2D atlas,Rectangle r,ItemStack stack) {
    if(!stack.count) return;
    DrawTexturePro(atlas,{float(BlockTile(stack.block)*TileSize),0,TileSize,TileSize},
        {r.x+5,r.y+4,r.width-10,r.height-10},{0,0},0,WHITE);
    if(ToolDurability(stack.block)>0&&stack.damage) {
        float left=1.f-float(stack.damage)/ToolDurability(stack.block);
        DrawRectangle(int(r.x+4),int(r.y+r.height-5),int(r.width-8),3,BLACK);
        DrawRectangle(int(r.x+4),int(r.y+r.height-5),int((r.width-8)*left),2,Color{static_cast<unsigned char>(255*(1-left)),static_cast<unsigned char>(220*left),30,255});
    }
    const char* count=TextFormat("%d",stack.count);
    DrawText(count,int(r.x+r.width-MeasureText(count,14)-2),int(r.y+r.height-15),14,WHITE);
}
void ClassicButton(Rectangle r,const char* text,bool enabled) {
    bool hovered=enabled&&Hover(r);
    DrawRectangleRec(r,enabled?(hovered?Color{122,132,161,255}:Color{101,101,101,255}):Color{66,66,66,255});
    DrawRectangleLinesEx(r,2,{20,20,20,255});
    Color highlight=enabled?Color{182,182,182,255}:Color{95,95,95,255};
    DrawLineEx({r.x+2,r.y+2},{r.x+r.width-3,r.y+2},2,highlight);
    DrawLineEx({r.x+2,r.y+2},{r.x+2,r.y+r.height-3},2,highlight);
    DrawLineEx({r.x+3,r.y+r.height-3},{r.x+r.width-3,r.y+r.height-3},2,{45,45,45,255});
    int x=int(r.x+(r.width-MeasureText(text,20))/2),y=int(r.y+(r.height-20)/2);
    DrawText(text,x+2,y+2,20,Fade(BLACK,.65f));
    DrawText(text,x,y,20,enabled?(hovered?Color{255,245,169,255}:RAYWHITE):GRAY);
}
}
void UI::SetWorlds(std::vector<WorldSave> saves) {
    worlds=std::move(saves);worldScroll=0;selectedWorldPath.clear();
}
void UI::Notify(const std::string& text,float seconds) {message=text;messageUntil=GetTime()+seconds;}
void UI::OpenChest(int x,int y,int z) {chestX=x;chestY=y;chestZ=z;}
void UI::ResetInteraction() {capturingBinding=-1;bindingMessage.clear();draggingSlider=0;catalogOpen=false;message.clear();}
UI::~UI(){for(auto preview:mapPreviews)if(preview.id)UnloadRenderTexture(preview);if(skywarsPreview.id)UnloadRenderTexture(skywarsPreview);}
UiAction UI::Update(GameState state) {
    if(state==GameState::WorldLoading)return UiAction::None;
    if(state==GameState::Keyboard){
        int visible=std::max(1,(GetScreenHeight()-200)/38);
        if(capturingBinding>=0){int key=GetKeyPressed();if(key==KEY_ESCAPE){capturingBinding=-1;bindingMessage.clear();return UiAction::None;}if(key==KEY_BACKSPACE||key==KEY_DELETE)key=0;else if(!key){for(int button=0;button<7;++button)if(IsMouseButtonPressed(button)){key=-button-1;break;}if(!key)return UiAction::None;}
            if(SetBinding(BindingAction(capturingBinding),key)){capturingBinding=-1;bindingMessage="Bind alterada";}else bindingMessage="Tecla ja usada. Escolha outra ou ESC para cancelar.";return UiAction::None;
        }
        bindingScroll=std::clamp(bindingScroll-int(GetMouseWheelMove()),0,std::max(0,BindingCount-visible));
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            float x=GetScreenWidth()/2.f-300,y=float(GetScreenHeight()-70);
            if(Hover({x,y,290,40})){Settings.bindings=DefaultBindings;bindingMessage="Binds restauradas ao padrao";return UiAction::None;}
            if(Hover({x+310,y,290,40}))return UiAction::CloseKeyboard;
            for(int i=0;i<visible&&bindingScroll+i<BindingCount;++i)if(Hover(BindingRow(i))){capturingBinding=bindingScroll+i;bindingMessage="Pressione uma tecla ou botao do mouse. Backspace limpa; ESC cancela.";return UiAction::None;}
        }
        return UiAction::None;
    }

    int hotbar=PressedHotbarBinding();
    if(hotbar>=0){
        if(state==GameState::ServerInventory)for(auto slot:RemoteSlots())if(Hover(slot.rect)){net::SwapHotbar(slot.index,hotbar);return UiAction::None;}
        ItemStack* target=nullptr;int armor=-1;
        if(state==GameState::Inventory&&!catalogOpen){for(int i=0;i<36;++i)if(Hover(ClassicPlayerSlot(i)))target=i<9?&Hotbar[i]:&Backpack[i-9];for(int i=0;i<4;++i){if(Hover(ClassicArmorSlot(i))){target=&EquippedArmor[i];armor=i;}if(Hover(CraftSlot(i,2)))target=&PersonalCrafting[i];}
            if(Hover(CraftOutput(2))){auto cursor=CursorStack;CursorStack=Hotbar[hotbar];if(TakeCraft(PersonalCrafting.data(),2))Hotbar[hotbar]=CursorStack;CursorStack=cursor;return UiAction::None;}}
        if(state==GameState::ChestScreen){auto* chest=ChestAt(chestX,chestY,chestZ);for(int i=0;i<27;++i)if(chest&&Hover(ChestSlot(i,0)))target=&(*chest)[i];}
        if(state==GameState::ChestScreen||state==GameState::CraftScreen||state==GameState::FurnaceScreen){for(int i=0;i<27;++i)if(Hover(ChestSlot(i,1)))target=&Backpack[i];for(int i=0;i<9;++i)if(Hover(ChestSlot(i,2)))target=&Hotbar[i];}
        if(state==GameState::CraftScreen)for(int i=0;i<9;++i)if(Hover(CraftSlot(i,3)))target=&WorkbenchCrafting[i];
        if(target){if(armor<0||!Hotbar[hotbar].count||(Armor(Hotbar[hotbar].block)&&ArmorSlot(Hotbar[hotbar].block)==armor))std::swap(*target,Hotbar[hotbar]);return UiAction::None;}
    }
    if(state==GameState::BedwarsMode){
        float x=GetScreenWidth()/2.f-220,y=GetScreenHeight()/2.f-60;
        bool control=IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
        if(control&&IsKeyPressed(KEY_A))roomName.clear();
        auto append=[&](int c){if(c>=32&&c<=126&&roomName.size()<32)roomName.push_back(char(c));};
        if(control&&IsKeyPressed(KEY_V)){const char* text=GetClipboardText();if(text)for(;*text;++text)append(static_cast<unsigned char>(*text));}
        for(int c=GetCharPressed();c;c=GetCharPressed())if(!control)append(c);
        if(IsKeyPressed(KEY_BACKSPACE)&&!roomName.empty())roomName.pop_back();
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            for(int i=0;i<2;++i)if(Hover({x,y+40+i*65,440,50})){if(roomName.find_first_not_of(" ")==std::string::npos){Notify("Escolha um nome para a sala");return UiAction::None;}teamCapacity=i+1;return standardLayout?UiAction::ChooseRush:UiAction::ChooseMap;}
            if(Hover({x,y+170,440,40})){standardLayout=!standardLayout;mapPage=0;mapChoice=standardLayout?0:3;return UiAction::None;}
            if(Hover({x,y+220,440,40}))return UiAction::OpenBedwars;
        }
        return UiAction::None;
    }

    if(state==GameState::BedwarsTeams){if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){for(const auto& team:bw::TeamChoices())if(team.available&&Hover(TeamButton(team.id))){bw::SelectTeam(team.id);return UiAction::None;}if(Hover(TeamBack()))return UiAction::Resume;}return UiAction::None;}
    if(state==GameState::Minigames){
        if(!mapPreviews[0].id)mapPreviews[0]=bw::CreateMapPreview(0,16);
        if(!skywarsPreview.id)skywarsPreview=CreateSkywarsPreview();
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            if(Hover(MinigameCard(0)))return UiAction::OpenBedwars;
            if(Hover(MinigamesBack()))return UiAction::BackToMenu;
        }
        return UiAction::None;
    }
    if(state==GameState::BedwarsMaps){
        int pages=(bw::MapCount-3+5)/6;
        if(!standardLayout){int delta=IsKeyPressed(KEY_RIGHT)?1:IsKeyPressed(KEY_LEFT)?-1:0;delta-=int(GetMouseWheelMove());if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){if(Hover(MapPageButton(false)))--delta;if(Hover(MapPageButton(true)))++delta;}mapPage=std::clamp(mapPage+delta,0,pages-1);}
        int first=standardLayout?0:3+mapPage*6,last=standardLayout?3:std::min(first+6,bw::MapCount);
        for(int i=first;i<last;++i)if(!mapPreviews[i].id||(standardLayout&&previewGap!=rushGap)){if(mapPreviews[i].id)UnloadRenderTexture(mapPreviews[i]);mapPreviews[i]=bw::CreateMapPreview(i,rushGap);}
        if(standardLayout)previewGap=rushGap;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))for(int i=first;i<last;++i)if(Hover(MapCard(i-first,last-first))){mapChoice=i;return UiAction::HostBedwars;}
        return UiAction::None;
    }

    if(state==GameState::BedwarsMenu||state==GameState::BedwarsDiscovery){
        ServerBrowserLayout layout;auto rooms=FilterRooms(roomSearch,mapSearch,pingSearch,onlyOpenRooms);int visible=std::max(1,int((layout.list.height-34)/36));
        if(Hover(layout.list))lanScroll-=int(GetMouseWheelMove());
        if(Hover(layout.right))rosterScroll=std::max(0,rosterScroll-int(GetMouseWheelMove()));
        lanScroll=std::clamp(lanScroll,0,std::max(0,int(rooms.size())-visible));
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            browserField=-1;for(int i=0;i<5;++i)if(Hover(layout.fields[i]))browserField=i;
            if(Hover(layout.onlyOpen)){onlyOpenRooms=!onlyOpenRooms;lanScroll=0;}
            if(Hover(layout.refresh)&&!bw::Active()){lanScroll=0;return UiAction::SearchBedwars;}
            if(Hover(layout.create)&&!bw::Active())return UiAction::ChooseMode;
            if(Hover(layout.back))return UiAction::OpenMinigames;
            if(Hover(layout.join)&&!bw::Active()&&JoinableAddress(bedwarsAddress))return UiAction::JoinBedwars;
            for(int i=0;i<visible&&lanScroll+i<int(rooms.size());++i)if(Hover({layout.list.x,layout.list.y+34+i*36,layout.list.width,34})){const auto& room=*rooms[lanScroll+i];selectedRoom=room.identity;rosterScroll=0;bedwarsAddress=room.address;}
        }
        if(IsKeyPressed(KEY_TAB))browserField=(browserField+1)%5;
        if(browserField>=0){
            std::string* fields[]={&roomSearch,&mapSearch,&pingSearch,&playerName,&bedwarsAddress};auto& field=*fields[browserField];size_t limit=browserField==2?4:browserField==3?16:64;
            bool control=IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL);
            if(control&&IsKeyPressed(KEY_A))field.clear();
            auto append=[&](int c){if(c>=32&&c<=126&&field.size()<limit&&(browserField!=2||std::isdigit(c))&&(browserField!=3||(std::isalnum(c)||c=='_')))field.push_back(char(c));};
            if(control&&IsKeyPressed(KEY_V)){const char* pasted=GetClipboardText();if(pasted)for(;*pasted;++pasted)append(static_cast<unsigned char>(*pasted));}
            for(int c=GetCharPressed();c;c=GetCharPressed())if(!control)append(c);
            if(IsKeyPressed(KEY_BACKSPACE)&&!field.empty())field.pop_back();
            if(browserField<3)lanScroll=0;
        }
        if(IsKeyPressed(KEY_ENTER)&&!bw::Active()&&JoinableAddress(bedwarsAddress))return UiAction::JoinBedwars;
        return UiAction::None;
    }

    if(state==GameState::BedwarsRush){float x=GetScreenWidth()/2.f-220,y=GetScreenHeight()/2.f-100;if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))for(int i=0;i<3;++i)if(Hover({x,y+i*60,440,48})){rushGap=16<<i;return UiAction::ChooseMap;}return UiAction::None;}

    if(state==GameState::BedwarsShop){bw::ShopInput();return UiAction::None;}
    if(state==GameState::BedwarsInventory||state==GameState::BedwarsChest){bw::InventoryInput(state==GameState::BedwarsChest);return UiAction::None;}

    if(state==GameState::Multiplayer) {
        float x=GetScreenWidth()/2.f-280,y=GetScreenHeight()/2.f-150;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover({x,y+55,560,38}))serverField=0;
            if(Hover({x,y+130,560,38}))serverField=1;
            if(Hover({x,y+262,270,40}))return net::Active()?UiAction::DisconnectServer:UiAction::ConnectServer;
            if(Hover({x+290,y+262,270,40}))return UiAction::BackToMenu;
        }
        if(IsKeyPressed(KEY_TAB))serverField=1-serverField;
        std::string& field=serverField?playerName:serverAddress;
        for(int c=GetCharPressed();c;c=GetCharPressed())if(c>=33&&c<=126&&field.size()<(serverField?16u:253u))field.push_back(char(c));
        if(IsKeyPressed(KEY_BACKSPACE)&&!field.empty())field.pop_back();
        return UiAction::None;
    }
    if(state==GameState::ServerInventory) {
        bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(left||right)for(const auto& slot:RemoteSlots())if(Hover(slot.rect))net::ClickSlot(slot.index,right,ActionDown(BindingAction::Sneak));
        return UiAction::None;
    }
    if(state==GameState::Playing&&(net::Active()||bw::Ready())) {
        if(!chatOpen&&(ActionPressed(BindingAction::Chat)||ActionPressed(BindingAction::Command))) {chatOpen=true;chatText=ActionPressed(BindingAction::Command)?"/":"";EnableCursor();return UiAction::None;}
        if(chatOpen) {
            for(int c=GetCharPressed();c;c=GetCharPressed())if(c>=32&&c<127&&chatText.size()<100)chatText.push_back(char(c));
            if(IsKeyPressed(KEY_BACKSPACE)&&!chatText.empty())chatText.pop_back();
            if(IsKeyPressed(KEY_ENTER)){if(bw::Ready())bw::Chat(chatText);else net::Chat(chatText);chatText.clear();chatOpen=false;DisableCursor();}
            return UiAction::None;
        }
    }
    if(state==GameState::Dead) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(PauseButton(0))) return UiAction::Respawn;
            if(Hover(PauseButton(1))) return UiAction::SaveAndReturn;
        }
        return UiAction::None;
    }
    if(state==GameState::CraftScreen||state==GameState::FurnaceScreen) {
        Rectangle p=ChestPanel();bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(left&&Hover({p.x+390,p.y+8,26,23})) return UiAction::CloseChest;
        if(state==GameState::CraftScreen) {
            if(left||right) {
                for(int i=0;i<9;++i) if(Hover(CraftSlot(i,3))) {ClickStack(WorkbenchCrafting[i],right);return UiAction::None;}
                if(Hover(CraftOutput(3))) {TakeCraft(WorkbenchCrafting.data(),3);return UiAction::None;}
            }
        } else {
            auto* furnace=FurnaceAt(chestX,chestY,chestZ);if(!furnace) return UiAction::CloseChest;
            if(left||right) for(int i=0;i<3;++i) if(Hover(FurnaceSlot(i))) {
                if(i==2) TakeFurnaceOutput(furnace->slots[i],right);
                else if(!CursorStack.count||(i==0?FurnaceAccepts(CursorStack.block):FuelDuration(CursorStack.block)>0)) ClickStack(furnace->slots[i],right);
                return UiAction::None;
            }
        }
        if(left||right) for(int group=0;group<2;++group) for(int i=0;i<(group?9:27);++i)
            if(Hover(ChestSlot(i,group?2:1))) {ClickStack(group?Hotbar[i]:Backpack[i],right);return UiAction::None;}
        return UiAction::None;
    }
    if(state==GameState::ChestScreen) {
        auto* slots=ChestAt(chestX,chestY,chestZ);
        if(!slots) return UiAction::CloseChest;
        Rectangle p=ChestPanel();
        bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(left&&Hover({p.x+390,p.y+8,26,23})) return UiAction::CloseChest;
        if(left||right) {
            for(int i=0;i<27;++i) {
                if(Hover(ChestSlot(i,0))) {ClickStack((*slots)[i],right,left&&ActionDown(BindingAction::Sneak));return UiAction::None;}
                if(Hover(ChestSlot(i,1))) {
                    if(left&&ActionDown(BindingAction::Sneak)) TransferStack(Backpack[i],slots->data(),slots->size());
                    else ClickStack(Backpack[i],right);
                    return UiAction::None;
                }
            }
            for(int i=0;i<9;++i) if(Hover(ChestSlot(i,2))) {
                selected=i;
                if(left&&ActionDown(BindingAction::Sneak)) TransferStack(Hotbar[i],slots->data(),slots->size());
                else ClickStack(Hotbar[i],right);
                return UiAction::None;
            }
        }
        return UiAction::None;
    }
    if(state==GameState::Options) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(OptionControl(0))) Settings.freezeTime=!Settings.freezeTime;
            else if(Hover(OptionControl(1))) draggingSlider=1;
            else if(Hover(OptionControl(2))) draggingSlider=2;
            else if(Hover(OptionControl(4))) Settings.fogEnabled=!Settings.fogEnabled;
            else if(Hover(OptionControl(5))) draggingSlider=5;
            else if(Hover(OptionControl(6))) draggingSlider=6;
            else if(Hover(OptionControl(7))) draggingSlider=7;
            else if(Hover(OptionControl(9))) Settings.toggleSprint=!Settings.toggleSprint;
            else if(Hover(OptionControl(8))) return UiAction::OpenKeyboard;
            else if(Hover(OptionControl(3))) return UiAction::CloseOptions;
        }
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) draggingSlider=0;
        if(draggingSlider) {
            Rectangle r=OptionControl(draggingSlider);
            float value=std::clamp((GetMousePosition().x-r.x-10)/(r.width-20),0.f,1.f);
            if(draggingSlider==1) {
                Settings.newWorldMinute=int(std::lround(value*1439));
                SetClockMinute(Settings.newWorldMinute);
            } else if(draggingSlider==2) Settings.mouseSensitivity=.1f+2.9f*value;
            else if(draggingSlider==7) Settings.fov=40+70*value;
            else if(draggingSlider==5) Settings.fogDistance=8+184*value;
            else if(draggingSlider==6) Settings.renderChunks=MinRenderChunks+int(std::lround(value*(MaxRenderChunks-MinRenderChunks)));
        }
        return UiAction::None;
    }
    if(state==GameState::MainMenu) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(MainButton(0))) return UiAction::OpenWorlds;
            if(Hover(MainButton(1))) return UiAction::OpenMinigames;
            if(Hover(MainButton(2))) return UiAction::OpenOptions;
            if(Hover(MainButton(3))) return UiAction::QuitApplication;
        }
        return UiAction::None;
    }
    if(state==GameState::WorldSelection) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&Hover({GetScreenWidth()/2.f+35,70,275,28})) {
            NewWorldMode=NewWorldMode==GameMode::Creative?GameMode::Survival:GameMode::Creative;
            return UiAction::None;
        }
        float wheel=GetMouseWheelMove();
        worldScroll=std::clamp(worldScroll+(wheel>0?-1:wheel<0?1:0),0,
            std::max(0,int(worlds.size())-VisibleWorldRows()));
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(WorldButton(false))) return UiAction::NewWorld;
            if(Hover(WorldButton(true))) return UiAction::BackToMenu;
            for(int i=0;i<VisibleWorldRows()&&worldScroll+i<int(worlds.size());++i) {
                if(Hover(DeleteButton(i))) {selectedWorldPath=worlds[worldScroll+i].path;return UiAction::DeleteSelected;}
                if(Hover(WorldRow(i))) {selectedWorldPath=worlds[worldScroll+i].path;return UiAction::LoadSelected;}
            }
        }
        return UiAction::None;
    }
    const bool paused=state==GameState::Paused;
    const bool inventory=state==GameState::Inventory;
    if(paused) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(PauseButton(0))) {message.clear();return UiAction::Resume;}
            if(Hover(PauseButton(1))) return UiAction::OpenOptions;
            if(Hover(PauseButton(2))) return UiAction::SaveAndReturn;
        }
        return UiAction::None;
    }
    if(inventory&&!catalogOpen) {
        bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(Mode==GameMode::Creative&&left&&Hover(CreativeItemsButton())) {catalogOpen=true;return UiAction::None;}
        if(left||right)for(int i=0;i<4;++i)if(Hover(ClassicArmorSlot(i))) {
            if(!CursorStack.count||(Armor(CursorStack.block)&&ArmorSlot(CursorStack.block)==i))ClickStack(EquippedArmor[i],right);
            return UiAction::None;
        }
        if(left||right) {
            for(int i=0;i<4;++i) if(Hover(CraftSlot(i,2))) {
                if(Mode==GameMode::Creative&&left&&ActionDown(BindingAction::Sneak)) PersonalCrafting[i]={};else ClickStack(PersonalCrafting[i],right);
                return UiAction::None;
            }
            if(Hover(CraftOutput(2))) {TakeCraft(PersonalCrafting.data(),2);return UiAction::None;}
        }
        if(left||right) for(int group=0;group<2;++group) for(int i=0;i<(group?9:27);++i) {
            if(!Hover(PlayerSlot(i,group!=0))) continue;
            ItemStack& slot=group?Hotbar[i]:Backpack[i];
            if(right&&Armor(slot.block)) {EquipArmor(slot);return UiAction::None;}
            if(left&&ActionDown(BindingAction::Sneak)) {
                if(Mode==GameMode::Creative) slot={};
                else if(group) TransferStack(slot,Backpack.data(),Backpack.size());
                else TransferStack(slot,Hotbar.data(),Hotbar.size());
            } else ClickStack(slot,right);
            if(group) selected=i;
            return UiAction::None;
        }
    }
    for(int i=0;i<9;++i) if(ActionPressed(BindingAction(int(BindingAction::Hotbar1)+i))) {
        selected=i;
        if(inventory&&CursorStack.count) ClickStack(Hotbar[i],false);
    }
    float wheel=GetMouseWheelMove();
    if(wheel!=0) selected=(selected+(wheel>0?8:1))%9;
    if(inventory&&catalogOpen&&Mode==GameMode::Creative) {
        int pages=(int(BlockCount)-2)/21+1;
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)) inventoryPage=(inventoryPage+1)%pages;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)) inventoryPage=(inventoryPage+pages-1)%pages;
        Rectangle p=Panel();
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover({p.x+440,p.y+12,195,28})) {catalogOpen=false;return UiAction::None;}
            if(Hover({p.x+24,p.y+264,105,22})) inventoryPage=(inventoryPage+pages-1)%pages;
            if(Hover({p.x+520,p.y+264,115,22})) inventoryPage=(inventoryPage+1)%pages;
            for(int i=1+inventoryPage*21;i<BlockCount&&i<=inventoryPage*21+21;++i) {
                if(i==BedHead)continue;
                Rectangle r{p.x+30+((i-1)%7)*86.f,p.y+52+(((i-1)%21)/7)*72.f,78,66};
                if(Hover(r)) {
                    if(ActionDown(BindingAction::Sneak)) CursorStack={};
                    else if(!CursorStack.count) CursorStack={static_cast<Block>(i),static_cast<unsigned char>(StackCapacity(static_cast<Block>(i)))};
                    return UiAction::None;
                }
            }
        }
        bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(left||right) for(int i=0;i<9;++i) if(Hover(Slot(i,p.y+318,p.x+24))) {
            selected=i;
            if(left&&ActionDown(BindingAction::Sneak)) Hotbar[i]={};else ClickStack(Hotbar[i],right);
            return UiAction::None;
        }
    }
    return UiAction::None;
}
void UI::Draw(Texture2D atlas,GameState state) const {
    if(state==GameState::WorldLoading){
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),{28,30,39,255});
        int w=std::min(480,GetScreenWidth()-48),x=(GetScreenWidth()-w)/2,y=GetScreenHeight()/2;
        DrawText("ENTRANDO NA SALA",x,y-80,28,GOLD);DrawText(loadingText.c_str(),x,y-35,20,WHITE);
        DrawRectangle(x,y,w,18,{61,65,78,255});DrawRectangle(x,y,int(w*std::clamp(loadingProgress,0.f,1.f)),18,{103,181,111,255});
        DrawText(TextFormat("%d%%",int(loadingProgress*100)),x,y+30,18,LIGHTGRAY);DrawText("ESC para cancelar",x,y+65,17,GRAY);return;
    }

    if(state==GameState::Keyboard){
        ClearBackground({36,37,44,255});float x=GetScreenWidth()/2.f-300;DrawText("KEYBOARD",GetScreenWidth()/2-MeasureText("KEYBOARD",28)/2,24,28,GOLD);DrawText("Clique na acao para trocar a tecla ou o botao do mouse",int(x),65,16,WHITE);
        int visible=std::max(1,(GetScreenHeight()-200)/38);for(int i=0;i<visible&&bindingScroll+i<BindingCount;++i){int index=bindingScroll+i;auto row=BindingRow(i);DrawRectangleRec(row,capturingBinding==index?Color{101,91,49,255}:Hover(row)?Color{79,84,96,255}:Color{54,58,69,255});DrawText(BindingLabel(BindingAction(index)),int(row.x+10),int(row.y+9),16,WHITE);auto key=capturingBinding==index?std::string("Pressione..."):BindingKeyName(Settings.bindings[index]);DrawText(key.c_str(),int(row.x+row.width-10-MeasureText(key.c_str(),16)),int(row.y+9),16,GOLD);}
        DrawText(bindingMessage.empty()?"Roda: mais acoes | ESC: voltar":bindingMessage.c_str(),int(x),GetScreenHeight()-105,14,GOLD);ClassicButton({x,float(GetScreenHeight()-70),290,40},"Reset",true);ClassicButton({x+310,float(GetScreenHeight()-70),290,40},"Concluido",true);return;
    }

    if(state==GameState::Minigames){
        ClearBackground({34,35,43,255});
        const char* title="MULTIPLAYER";
        DrawText(title,(GetScreenWidth()-MeasureText(title,30))/2,int(MinigameCard(0).y-65),30,RAYWHITE);
        for(int i=0;i<2;++i){
            Rectangle r=MinigameCard(i);bool enabled=i==0,hover=enabled&&Hover(r);
            DrawRectangleRec(r,hover?Color{66,71,83,255}:Color{45,47,54,255});
            Rectangle image{r.x+8,r.y+8,r.width-16,165};
            const auto& preview=enabled?mapPreviews[0]:skywarsPreview;
            if(preview.id)DrawTexturePro(preview.texture,{0,0,360,-210},image,{0,0},0,enabled?WHITE:Color{160,160,160,255});
            if(!enabled)DrawRectangleRec(image,Fade(Color{70,70,70,255},.5f));
            const char* name=enabled?"BEDWARS":"SKYWARS";
            DrawText(name,int(r.x+(r.width-MeasureText(name,26))/2),int(r.y+188),26,enabled?GOLD:GRAY);
            const char* subtitle=enabled?"Criar sala ou jogar com amigos":"Em breve";
            DrawText(subtitle,int(r.x+(r.width-MeasureText(subtitle,16))/2),int(r.y+229),16,enabled?RAYWHITE:GRAY);
            DrawRectangleLinesEx(r,2,hover?GOLD:enabled?GRAY:Color{66,66,70,255});
        }
        ClassicButton(MinigamesBack(),"Voltar",true);return;
    }
    if(state==GameState::BedwarsMode){
        ClearBackground({34,35,43,255});float x=GetScreenWidth()/2.f-220,y=GetScreenHeight()/2.f-60;
        DrawText("CRIAR SERVIDOR",GetScreenWidth()/2-MeasureText("CRIAR SERVIDOR",28)/2,int(y-100),28,GOLD);
        DrawText("Nome do servidor",int(x),int(y-44),16,LIGHTGRAY);
        Rectangle field{x,y-20,440,36};DrawRectangleRec(field,{17,19,25,255});DrawRectangleLinesEx(field,1,GOLD);BrowserText(roomName+(int(GetTime()*2)%2?"_":""),field,18,WHITE);
        ClassicButton({x,y+40,440,50},"Solo - 1 jogador por time",true);ClassicButton({x,y+105,440,50},"Dupla - 2 jogadores por time",true);
        ClassicButton({x,y+170,440,40},standardLayout?"Layout padrao: Ligado":"Layout padrao: Desligado",true);
        ClassicButton({x,y+220,440,40},"Voltar",true);
        if(!message.empty()&&GetTime()<messageUntil)BrowserText(message,{x,y+265,440,24},14,GOLD);
        return;
    }

    if(state==GameState::BedwarsTeams){DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.7f));DrawText("ESCOLHA SEU TIME",GetScreenWidth()/2-150,GetScreenHeight()/2-155,26,GOLD);for(const auto& team:bw::TeamChoices()){auto r=TeamButton(team.id);ClassicButton(r,TextFormat("%s %d/%d%s",team.name,team.players,team.capacity,team.id==bw::LocalTeam()?" *":""),team.available);DrawRectangle(int(r.x+6),int(r.y+10),7,26,team.color);}ClassicButton(TeamBack(),"Voltar ao jogo",true);return;}
    if(state==GameState::BedwarsMaps){
        ClearBackground({34,35,43,255});const char* title=standardLayout?"MAPAS BASICOS":"MAPAS AVANCADOS";DrawText(title,(GetScreenWidth()-MeasureText(title,28))/2,28,28,GOLD);
        int pages=(bw::MapCount-3+5)/6;int first=standardLayout?0:3+mapPage*6,last=standardLayout?3:std::min(first+6,bw::MapCount);
        for(int i=first;i<last;++i){Rectangle r=MapCard(i-first,last-first);DrawRectangleRec(r,Hover(r)?Color{76,80,91,255}:Color{47,50,60,255});DrawRectangleLinesEx(r,2,Hover(r)?GOLD:GRAY);
            float imageHeight=std::min(r.height-78,(r.width-14)*210/360),imageWidth=imageHeight*360/210;if(mapPreviews[i].id)DrawTexturePro(mapPreviews[i].texture,{0,0,360,-210},{r.x+(r.width-imageWidth)/2,r.y+7,imageWidth,imageHeight},{0,0},0,WHITE);
            BrowserText(bw::MapName(i),{r.x+6,r.y+r.height-70,r.width-12,27},20,WHITE);BrowserText(bw::MapDescription(i),{r.x+6,r.y+r.height-44,r.width-12,21},12,LIGHTGRAY);
            BrowserText(i==3?"2 times | Layout original":i>=4?"8 times | Layout original":TextFormat("8 times | Rush %d",rushGap+6),{r.x+6,r.y+r.height-24,r.width-12,21},14,LIGHTGRAY);
        }if(!standardLayout){ClassicButton(MapPageButton(false),"Anterior",mapPage>0);ClassicButton(MapPageButton(true),"Proxima",mapPage+1<pages);DrawText(TextFormat("%d / %d",mapPage+1,pages),GetScreenWidth()/2-22,GetScreenHeight()-52,16,WHITE);}
        DrawText("ESC para voltar",GetScreenWidth()/2-80,GetScreenHeight()-22,14,WHITE);return;
    }

    if(state==GameState::BedwarsMenu||state==GameState::BedwarsDiscovery){
        ClearBackground({24,26,33,255});ServerBrowserLayout layout;
        DrawText("BEDWARS",22,22,28,GOLD);DrawText("SERVIDORES NA REDE LOCAL",190,30,16,LIGHTGRAY);
        for(auto rect:{layout.left,layout.list,layout.right}){DrawRectangleRec(rect,{34,37,46,255});DrawRectangleLinesEx(rect,1,{61,66,81,255});}
        DrawText("FILTROS",int(layout.left.x+12),int(layout.left.y+9),16,GOLD);
        const char* labels[]={"Buscar servidor","Mapa","Ping maximo (ms)","Seu nick"};const std::string* values[]={&roomSearch,&mapSearch,&pingSearch,&playerName,&bedwarsAddress};
        for(int i=0;i<4;++i){Rectangle r=layout.fields[i];DrawText(labels[i],int(r.x),int(r.y-17),14,LIGHTGRAY);DrawRectangleRec(r,{17,19,25,255});DrawRectangleLinesEx(r,1,browserField==i?GOLD:Color{74,79,94,255});BrowserText(*values[i]+(browserField==i&&int(GetTime()*2)%2?"_":""),r,16,WHITE);}
        DrawRectangle(int(layout.onlyOpen.x),int(layout.onlyOpen.y+3),14,14,onlyOpenRooms?GOLD:DARKGRAY);DrawText("Com vagas",int(layout.onlyOpen.x+22),int(layout.onlyOpen.y+2),14,WHITE);
        auto rooms=FilterRooms(roomSearch,mapSearch,pingSearch,onlyOpenRooms);const LanRoom* selected=nullptr;for(const auto& room:bw::Rooms())if(room.identity==selectedRoom)selected=&room;
        float x=layout.list.x,w=layout.list.width,mapWidth=std::min(115.f,w*.28f),nameWidth=w-mapWidth-100;
        DrawRectangle(int(x),int(layout.list.y),int(w),30,{49,54,65,255});BrowserText("Servidor",{x,layout.list.y,nameWidth,28},14,LIGHTGRAY);BrowserText("Mapa",{x+nameWidth,layout.list.y,mapWidth,28},14,LIGHTGRAY);BrowserText("Players",{x+w-100,layout.list.y,58,28},12,LIGHTGRAY);BrowserText("Ping",{x+w-42,layout.list.y,42,28},12,LIGHTGRAY);
        int visible=std::max(1,int((layout.list.height-34)/36)),scroll=std::clamp(lanScroll,0,std::max(0,int(rooms.size())-visible));
        for(int i=0;i<visible&&scroll+i<int(rooms.size());++i){const auto& room=*rooms[scroll+i];Rectangle row{x,layout.list.y+34+i*36,w,34};bool active=room.identity==selectedRoom;
            DrawRectangleRec(row,active?Color{65,72,92,255}:Hover(row)?Color{47,52,65,255}:Color{30,33,41,255});if(active)DrawRectangle(int(x),int(row.y),3,34,GOLD);
            BrowserText(room.name,{x+3,row.y,nameWidth-3,32},16,room.joinable?WHITE:GRAY);BrowserText(room.map.empty()?"Desconhecido":room.map,{x+nameWidth,row.y,mapWidth,32},14,LIGHTGRAY);
            BrowserText(TextFormat("%d/%d",room.players,room.capacity),{x+w-100,row.y,58,32},14,room.joinable?WHITE:GRAY);BrowserText(room.ping<0?"--":std::to_string(room.ping),{x+w-42,row.y,42,32},14,room.ping<80?LIME:room.ping<180?GOLD:ORANGE);
        }
        if(rooms.empty()){BrowserText(bw::SearchingRooms()?"Procurando servidores...":"Nenhuma sala encontrada",{x+8,layout.list.y+45,w-16,30},16,LIGHTGRAY);BrowserText("Use o endereco para conectar.",{x+8,layout.list.y+78,w-16,30},14,GRAY);}
        float rx=layout.right.x+12,ry=layout.right.y+12,rw=layout.right.width-24;
        DrawText("JOGADORES",int(rx),int(ry),16,GOLD);ry+=32;
        if(selected){BrowserText(selected->name,{rx,ry,rw,28},18,WHITE);ry+=32;BrowserText(selected->map.empty()?"Mapa indisponivel":selected->map,{rx,ry,rw,26},16,LIGHTGRAY);ry+=28;BrowserText(selected->protocol!=LanProtocolVersion?"Versao diferente: atualize os dois":selected->joinable?"Lobby - pode entrar":selected->players>=selected->capacity?"Sala cheia":"Partida em andamento",{rx,ry,rw,26},14,selected->joinable?LIME:GOLD);ry+=36;
            int visibleNames=std::max(1,int((layout.right.y+layout.right.height-ry)/32));int first=std::clamp(rosterScroll,0,std::max(0,int(selected->playerNames.size())-visibleNames));
            for(int i=first;i<int(selected->playerNames.size())&&i<first+visibleNames;++i){const auto& name=selected->playerNames[i];DrawRectangle(int(rx),int(ry),int(rw),28,{45,50,62,255});BrowserText(name,{rx+4,ry,rw-8,28},16,WHITE);ry+=32;}
            if(selected->playerNames.empty())BrowserText("Lista indisponivel",{rx,ry,rw,30},14,GRAY);
        }else {BrowserText("Selecione um servidor",{rx,ry,rw,30},14,GRAY);}
        DrawText("Endereco:",16,int(layout.fields[4].y+8),16,LIGHTGRAY);auto address=layout.fields[4];DrawRectangleRec(address,{13,15,20,255});DrawRectangleLinesEx(address,1,browserField==4?GOLD:GRAY);BrowserText(bedwarsAddress,address,16,WHITE);
        ClassicButton(layout.refresh,bw::SearchingRooms()?"Buscando...":"Atualizar",!bw::Active());ClassicButton(layout.create,"Criar sala",!bw::Active());ClassicButton(layout.join,"Conectar",!bw::Active()&&JoinableAddress(bedwarsAddress));ClassicButton(layout.back,"Voltar",true);
        bool nicknameInUse=false;
        if(selected)for(const auto& name:selected->playerNames)if(LowerText(name)==LowerText(playerName))nicknameInUse=true;
        const std::string status=!message.empty()&&GetTime()<messageUntil?message:bw::Active()?bw::Status():nicknameInUse?"Esse nick ja esta conectado. Escolha outro nick para entrar.":bw::DiscoveryStatus();BrowserText(status,{16,float(GetScreenHeight()-24),float(GetScreenWidth()-32),22},12,LIGHTGRAY);return;
    }

    if(state==GameState::BedwarsRush){ClearBackground({34,35,43,255});float x=GetScreenWidth()/2.f-220,y=GetScreenHeight()/2.f-100;DrawText("RUSH",int(x+174),int(y-55),32,GOLD);const char* labels[]={"Rapido (22 blocos)","Medio (38 blocos)","Lento (70 blocos)"};for(int i=0;i<3;++i)ClassicButton({x,y+i*60,440,48},labels[i],true);return;}

    if(state==GameState::BedwarsShop){bw::DrawShop(atlas);return;}
    if(state==GameState::BedwarsInventory||state==GameState::BedwarsChest){bw::DrawInventory(atlas,state==GameState::BedwarsChest);return;}

    if(state==GameState::Multiplayer) {
        ClearBackground({39,39,44,255});float x=GetScreenWidth()/2.f-280,y=GetScreenHeight()/2.f-150;
        DrawText("MULTIPLAYER JAVA 1.8",int(x),int(y),26,WHITE);
        DrawText("Servidor (endereco:porta)",int(x),int(y+32),17,LIGHTGRAY);
        DrawRectangleRec({x,y+55,560,38},BLACK);DrawRectangleLinesEx({x,y+55,560,38},2,serverField==0?GOLD:GRAY);DrawText(serverAddress.c_str(),int(x+9),int(y+66),18,WHITE);
        DrawText("Nome do jogador",int(x),int(y+108),17,LIGHTGRAY);
        DrawRectangleRec({x,y+130,560,38},BLACK);DrawRectangleLinesEx({x,y+130,560,38},2,serverField==1?GOLD:GRAY);DrawText(playerName.c_str(),int(x+9),int(y+141),18,WHITE);
        DrawText("Experimental: servidores 1.8 sem autenticacao Microsoft.",int(x),int(y+184),15,GOLD);
        std::string status=net::Status();
        BeginScissorMode(int(x),int(y+204),560,52);
        for(int line=0;line<3&&!status.empty();++line){
            size_t length=status.size();while(length>0&&MeasureText(status.substr(0,length).c_str(),15)>550)--length;
            if(length==0)break;
            if(length<status.size()){size_t space=status.rfind(' ',length);if(space!=std::string::npos&&space>0)length=space;}
            std::string part=status.substr(0,length);DrawText(part.c_str(),int(x),int(y+207+line*16),15,RAYWHITE);
            status.erase(0,length);while(!status.empty()&&status.front()==' ')status.erase(0,1);
        }
        EndScissorMode();
        ClassicButton({x,y+262,270,40},net::Active()?"Cancelar":"Conectar",true);ClassicButton({x+290,y+262,270,40},"Voltar",true);return;
    }
    if(state==GameState::ServerInventory) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.65f));
        const auto layout=RemoteSlots();const auto& slots=net::WindowSlots();
        if(!layout.empty()){
            float left=layout[0].rect.x,top=layout[0].rect.y,right=left,bottom=top;
            for(const auto& slot:layout){left=std::min(left,slot.rect.x);top=std::min(top,slot.rect.y);right=std::max(right,slot.rect.x+slot.rect.width);bottom=std::max(bottom,slot.rect.y+slot.rect.height);}
            DrawRectangle(int(left-10),int(top-32),int(right-left+20),int(bottom-top+65),{178,174,165,255});DrawText(net::WindowTitle().c_str(),int(left),int(top-24),18,DARKGRAY);
            for(const auto& slot:layout){DrawRectangleRec(slot.rect,Hover(slot.rect)?LIGHTGRAY:GRAY);const auto& item=slots[slot.index];if(item.count)StackIcon(atlas,slot.rect,{net::LocalItem(item),item.count});}
            DrawText("E / ESC: fechar",int(left),int(bottom+12),14,DARKGRAY);
        }
        if(net::Cursor().count){auto m=GetMousePosition();StackIcon(atlas,{m.x-18,m.y-18,40,40},{net::LocalItem(net::Cursor()),net::Cursor().count});}return;
    }
    if(state==GameState::Dead) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),{65,5,5,180});
        const char* title="Voce morreu!";DrawText(title,(GetScreenWidth()-MeasureText(title,36))/2,GetScreenHeight()/2-135,36,WHITE);
        ClassicButton(PauseButton(0),"Renascer",true);ClassicButton(PauseButton(1),"Salvar e voltar ao menu",true);return;
    }
    if(state==GameState::CraftScreen||state==GameState::FurnaceScreen) {
        Rectangle p=ChestPanel();DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.6f));
        DrawRectangleRec(p,{190,185,174,255});DrawRectangleLinesEx(p,3,DARKGRAY);
        DrawText(state==GameState::CraftScreen?"Mesa de trabalho":"Fornalha",int(p.x+16),int(p.y+12),20,DARKGRAY);
        DrawText("X",int(p.x+397),int(p.y+10),20,MAROON);Block hovered=Air;
        auto drawSlot=[&](Rectangle r,ItemStack slot) {DrawRectangleRec(r,Hover(r)?LIGHTGRAY:GRAY);DrawRectangleLinesEx(r,2,DARKGRAY);StackIcon(atlas,r,slot);if(Hover(r)) hovered=slot.block;};
        if(state==GameState::CraftScreen) {
            for(int i=0;i<9;++i) drawSlot(CraftSlot(i,3),WorkbenchCrafting[i]);
            drawSlot(CraftOutput(3),CraftResult(WorkbenchCrafting.data(),3));
            DrawText("->",int(p.x+235),int(p.y+94),24,DARKGRAY);
        } else if(auto* furnace=FurnaceAt(chestX,chestY,chestZ)) {
            for(int i=0;i<3;++i) drawSlot(FurnaceSlot(i),furnace->slots[i]);
            DrawText(furnace->fuel>0?"ACESO":"APAGADO",int(p.x+117),int(p.y+91),13,furnace->fuel>0?ORANGE:DARKGRAY);
            DrawRectangle(int(p.x+215),int(p.y+94),70,14,DARKGRAY);
            DrawRectangle(int(p.x+215),int(p.y+94),int(70*furnace->progress/10),14,ORANGE);
        }
        DrawText("Inventario",int(p.x+16),int(p.y+180),18,DARKGRAY);
        for(int group=0;group<2;++group) for(int i=0;i<(group?9:27);++i) drawSlot(ChestSlot(i,group?2:1),group?Hotbar[i]:Backpack[i]);
        DrawText("E / ESC: fechar",int(p.x+16),int(p.y+405),14,DARKGRAY);
        if(CursorStack.count) {Vector2 m=GetMousePosition();StackIcon(atlas,{m.x-18,m.y-18,40,40},CursorStack);}
        else if(hovered!=Air) {Vector2 m=GetMousePosition();DrawRectangle(int(m.x+12),int(m.y-28),MeasureText(Names[hovered],16)+16,25,{29,24,36,245});DrawText(Names[hovered],int(m.x+20),int(m.y-23),16,WHITE);}
        return;
    }
    if(state==GameState::ChestScreen) {
        Rectangle p=ChestPanel();DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.55f));
        DrawRectangleRec(p,{190,185,174,255});DrawRectangleLinesEx(p,3,{60,54,43,255});
        DrawText("Bau",int(p.x+16),int(p.y+12),20,{40,36,29,255});
        DrawText("X",int(p.x+397),int(p.y+10),20,MAROON);
        DrawText("Inventario",int(p.x+16),int(p.y+180),18,{40,36,29,255});
        auto* slots=ChestAt(chestX,chestY,chestZ);Block hovered=Air;
        for(int group=0;group<3;++group) for(int i=0;i<(group==2?9:27);++i) {
            Rectangle r=ChestSlot(i,group);DrawRectangleRec(r,Hover(r)?Color{157,155,144,255}:Color{112,109,100,255});
            DrawRectangleLinesEx(r,2,{66,63,57,255});
            ItemStack stack=group==0?(slots?(*slots)[i]:ItemStack{}):(group==1?Backpack[i]:Hotbar[i]);
            StackIcon(atlas,r,stack);if(Hover(r)) hovered=stack.block;
        }
        DrawText("Shift: transferir pilha | E / ESC: fechar",int(p.x+12),int(p.y+405),14,{45,40,31,255});
        if(CursorStack.count) {
            Vector2 mouse=GetMousePosition();StackIcon(atlas,{mouse.x-18,mouse.y-18,40,40},CursorStack);
        } else if(hovered!=Air) {
            Vector2 mouse=GetMousePosition();int width=MeasureText(Names[hovered],16)+16;
            DrawRectangle(int(mouse.x+12),int(mouse.y-28),width,25,{29,24,36,245});
            DrawText(Names[hovered],int(mouse.x+20),int(mouse.y-23),16,WHITE);
        }
        return;
    }
    if(state==GameState::Playing&&!message.empty()&&GetTime()<messageUntil) {
        int width=MeasureText(message.c_str(),18)+20;
        DrawRectangle((GetScreenWidth()-width)/2,GetScreenHeight()-128,width,27,Fade(BLACK,.7f));
        DrawText(message.c_str(),(GetScreenWidth()-width)/2+10,GetScreenHeight()-123,18,WHITE);
    }
    if(state==GameState::Inventory&&!catalogOpen) {
        DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.6f));DrawClassicInventoryPanel();DrawInventoryPlayer();float scale=InventoryScale();auto p=InventoryPanel();
        DrawText("Fabricacao",int(p.x+96*scale),int(p.y+5*scale),int(8*scale),DARKGRAY);
        Block hovered=Air;
        for(int i=0;i<4;++i){auto r=ClassicArmorSlot(i);DrawInventorySlot(r,Hover(r));StackIcon(atlas,r,EquippedArmor[i]);if(Hover(r))hovered=EquippedArmor[i].block;}
        for(int i=0;i<4;++i){auto r=CraftSlot(i,2);DrawInventorySlot(r,Hover(r));StackIcon(atlas,r,PersonalCrafting[i]);if(Hover(r))hovered=PersonalCrafting[i].block;}
        auto output=CraftOutput(2);DrawInventorySlot(output,Hover(output));auto result=CraftResult(PersonalCrafting.data(),2);StackIcon(atlas,output,result);if(Hover(output))hovered=result.block;
        DrawText(">",int(p.x+139*scale),int(p.y+29*scale),int(10*scale),GRAY);
        if(Mode==GameMode::Creative){auto button=CreativeItemsButton();DrawRectangleRec(button,Hover(button)?Color{109,137,96,255}:Color{93,112,82,255});DrawText("Todos os itens",int(button.x+3*scale),int(button.y+4*scale),int(6*scale),WHITE);}
        for(int i=0;i<36;++i){auto r=ClassicPlayerSlot(i);DrawInventorySlot(r,Hover(r));auto stack=i<9?Hotbar[i]:Backpack[i-9];StackIcon(atlas,r,stack);if(Hover(r))hovered=stack.block;}
        if(CursorStack.count){auto m=GetMousePosition();StackIcon(atlas,{m.x-8*scale,m.y-8*scale,16*scale,16*scale},CursorStack);}
        else if(hovered!=Air){auto m=GetMousePosition();int width=MeasureText(Names[hovered],16)+16;DrawRectangle(int(m.x+12),int(m.y-28),width,25,{29,24,36,245});DrawText(Names[hovered],int(m.x+20),int(m.y-23),16,WHITE);}
        return;
    }
    if(state==GameState::Options) {
        int w=GetScreenWidth(),h=GetScreenHeight();Rectangle p=OptionsPanel();
        DrawRectangle(0,0,w,h,Fade(BLACK,.55f));
        DrawRectangleRec(p,{48,46,42,255});DrawRectangleLinesEx(p,3,{155,151,138,255});
        DrawText("OPCOES",int(p.x+(p.width-MeasureText("OPCOES",26))/2),int(p.y+14),26,RAYWHITE);
        ClassicButton(OptionControl(0),Settings.freezeTime?"Horario: travado":"Horario: livre",true);ClassicButton(OptionControl(4),Settings.fogEnabled?"Neblina: ligada":"Neblina: desligada",true);
        int minute=ClockMinute();DrawText(TextFormat("Horario: %02d:%02d",minute/60,minute%60),int(p.x+20),int(p.y+97),17,WHITE);Slider(OptionControl(1),minute/1439.f);
        DrawText(TextFormat("Sensibilidade: %.0f%%",Settings.mouseSensitivity*100),int(p.x+328),int(p.y+97),17,WHITE);Slider(OptionControl(2),(Settings.mouseSensitivity-.1f)/2.9f);
        DrawText(TextFormat("FOV: %.0f",Settings.fov),int(p.x+20),int(p.y+180),17,WHITE);Slider(OptionControl(7),(Settings.fov-40)/70.f);
        DrawText(TextFormat("Visao: %d chunks (%d blocos)",Settings.renderChunks,Settings.renderChunks*16),int(p.x+328),int(p.y+180),17,WHITE);Slider(OptionControl(6),float(Settings.renderChunks-MinRenderChunks)/(MaxRenderChunks-MinRenderChunks));
        DrawText(TextFormat("Distancia da neblina: %.0f blocos",Settings.fogDistance),int(p.x+20),int(p.y+263),17,WHITE);Slider(OptionControl(5),(Settings.fogDistance-8)/184.f);
        ClassicButton(OptionControl(9),Settings.toggleSprint?"Toggle Sprint: LIGADO":"Toggle Sprint: DESLIGADO",true);
        ClassicButton(OptionControl(8),"Keyboard",true);ClassicButton(OptionControl(3),"Concluido",true);
        if(!message.empty())DrawText(message.c_str(),int(p.x+12),int(p.y+423),12,GOLD);
        return;
    }
    const bool paused=state==GameState::Paused;
    const bool inventory=state==GameState::Inventory;
    if(state==GameState::MainMenu) {
        int w=GetScreenWidth(),h=GetScreenHeight();
        ClearBackground({45,42,37,255});
        for(int y=0;y<h;y+=48) for(int x=0;x<w;x+=48) {
            int v=39+int(Hash(x/48,y/48,200)%12);
            DrawRectangle(x,y,48,48,{static_cast<unsigned char>(v+8),static_cast<unsigned char>(v+3),static_cast<unsigned char>(v),255});
        }
        const char* title="MineClone";
        DrawText(title,(w-MeasureText(title,44))/2+3,h/2-150+3,44,BLACK);
        DrawText(title,(w-MeasureText(title,44))/2,h/2-150,44,RAYWHITE);
        ClassicButton(MainButton(0),"Jogar",true);
        ClassicButton(MainButton(1),"Multiplayer",true);
        ClassicButton(MainButton(2),"Opcoes...",true);
        ClassicButton(MainButton(3),"Sair",true);
        if(!message.empty()) DrawText(message.c_str(),(w-MeasureText(message.c_str(),16))/2,h/2+160,16,RAYWHITE);
        return;
    }
    if(state==GameState::WorldSelection) {
        int w=GetScreenWidth(),h=GetScreenHeight();
        ClearBackground({43,41,37,255});
        const char* title="Selecionar Mundo";
        DrawText(title,(w-MeasureText(title,30))/2,34,30,RAYWHITE);
        DrawText(TextFormat("%d mundos",int(worlds.size())),w/2-300,78,17,LIGHTGRAY);
        Rectangle modeButton{w/2.f+35,70,275,28};DrawRectangleRec(modeButton,Hover(modeButton)?GRAY:DARKGRAY);
        DrawText(NewWorldMode==GameMode::Creative?"Novo mundo: Criativo":"Novo mundo: Sobrevivencia",int(modeButton.x+9),int(modeButton.y+6),16,WHITE);
        if(worlds.empty()) DrawText("Nenhum mundo salvo. Crie seu primeiro mundo.",w/2-235,130,20,LIGHTGRAY);
        for(int i=0;i<VisibleWorldRows()&&worldScroll+i<int(worlds.size());++i) {
            const WorldSave& save=worlds[worldScroll+i];
            Rectangle row=WorldRow(i);
            DrawRectangleRec(row,Hover(row)?Color{96,97,105,255}:Color{65,64,60,255});
            DrawRectangleLinesEx(row,2,Hover(row)?GOLD:GRAY);
            std::string name=save.name;
            while(name.size()>3&&MeasureText(name.c_str(),19)>430) name.pop_back();
            DrawText(name.c_str(),int(row.x+14),int(row.y+5),19,RAYWHITE);
            std::string path=save.path;
            while(path.size()>3&&MeasureText(path.c_str(),13)>430) path.erase(0,1);
            DrawText(path.c_str(),int(row.x+14),int(row.y+28),13,LIGHTGRAY);
            ClassicButton(DeleteButton(i),"Excluir Mundo",true);
        }
        ClassicButton(WorldButton(false),"Criar Novo Mundo",true);
        ClassicButton(WorldButton(true),"Voltar",true);
        if(!message.empty()) DrawText(message.c_str(),(w-MeasureText(message.c_str(),15))/2,h-32,15,RAYWHITE);
        return;
    }
    int w=GetScreenWidth(),h=GetScreenHeight();
    if(!inventory) {
        if(!paused) {
            DrawRectangle(w/2-8,h/2-1,17,3,Fade(BLACK,.8f)); DrawRectangle(w/2-1,h/2-8,3,17,Fade(BLACK,.8f));
            DrawLine(w/2-6,h/2,w/2+6,h/2,RAYWHITE);DrawLine(w/2,h/2-6,w/2,h/2+6,RAYWHITE);
        }
        float left=w/2.f-306;
        DrawRectangle(int(left-6),h-93,620,85,Fade(BLACK,.7f));
        for(int i=0;i<9;++i) {
            Rectangle r=Slot(i,float(h-87),left);
            DrawRectangleRec(r,{48,46,42,235});
            DrawRectangleLinesEx(r,selected==i?3.f:1.f,selected==i?GOLD:GRAY);
            Icon(atlas,r,Hotbar[i].block);
            if(ToolDurability(Hotbar[i].block)>0&&Hotbar[i].damage) {
                float remaining=1.f-float(Hotbar[i].damage)/ToolDurability(Hotbar[i].block);
                DrawRectangle(int(r.x+10),int(r.y+48),44,4,BLACK);
                DrawRectangle(int(r.x+10),int(r.y+48),int(44*remaining),3,remaining>.25f?GREEN:RED);
            }
            if(Hotbar[i].count) DrawText(TextFormat("%d",Hotbar[i].count),int(r.x+43),int(r.y+33),14,WHITE);
            DrawText(TextFormat("%d",i+1),int(r.x+5),int(r.y+4),14,WHITE);
        }
    }
    if(state==GameState::Playing&&Mode==GameMode::Survival) {
        int left=w/2-300;
        for(int i=0;i<10;++i) {
            int x=left+i*18,y=h-115;float units=Health-i*2;
            Color color=units>=2?Color{214,42,48,255}:(units>0?Color{145,31,40,255}:Color{43,24,26,255});
            DrawRectangle(x,y,5,5,color);DrawRectangle(x+6,y,5,5,color);
            DrawRectangle(x+1,y+4,9,5,color);DrawRectangle(x+3,y+9,5,3,color);
        }
        for(int i=0;i<10;++i)DrawRectangle(left+i*18,h-130,11,8,DefensePoints()>i*2?LIGHTGRAY:Color{43,43,45,255});
        for(int i=0;i<int(std::ceil(Absorption/2));++i){int x=left+185+i*18,y=h-115;Color c=Absorption-i*2>=2?GOLD:Color{173,133,30,255};DrawRectangle(x,y,5,5,c);DrawRectangle(x+6,y,5,5,c);DrawRectangle(x+1,y+4,9,5,c);DrawRectangle(x+3,y+9,5,3,c);}
        if(Breath<20) for(int i=0;i<10;++i) DrawCircle(left+i*18+5,h-144,4,Breath>i*2?SKYBLUE:DARKGRAY);
        if(miningProgress>0) {DrawRectangle(w/2-44,h/2+22,88,6,Fade(BLACK,.7f));DrawRectangle(w/2-43,h/2+23,int(86*miningProgress),4,RAYWHITE);}
    }
    if(net::Active()||bw::Ready()) {
        int y=GetScreenHeight()-170;
        const auto& messages=bw::Ready()?bw::Messages():net::Messages();
        for(auto it=messages.rbegin();it!=messages.rend();++it){DrawRectangle(8,y-2,GetScreenWidth()-16,20,Fade(BLACK,.45f));DrawText(it->c_str(),12,y,14,WHITE);y-=21;}
        if(chatOpen){DrawRectangle(6,GetScreenHeight()-30,GetScreenWidth()-12,24,Fade(BLACK,.8f));DrawText(("<"+(bw::Ready()?bw::Nickname():net::Nickname())+"> "+chatText+"_").c_str(),12,GetScreenHeight()-25,17,WHITE);}
    }
    if(state==GameState::Playing&&(net::Joined()||bw::Ready())&&ActionDown(BindingAction::PlayerList)&&!chatOpen)DrawPlayerList();
    DrawFPS(w-100,16);
    if(inventory) {
        DrawRectangle(0,0,w,h,Fade(BLACK,.6f));Rectangle p=Panel();
        DrawRectangleRec(p,{45,43,39,255});DrawRectangleLinesEx(p,3,{155,151,138,255});
        DrawText("BLOCOS CRIATIVOS",int(p.x+24),int(p.y+16),22,RAYWHITE);
        DrawRectangleRec({p.x+440,p.y+12,195,28},DARKGRAY);
        DrawText("Voltar ao inventario",int(p.x+448),int(p.y+18),16,WHITE);
        for(int i=1+inventoryPage*21;i<BlockCount&&i<=inventoryPage*21+21;++i) {
                if(i==BedHead)continue;
            Rectangle r{p.x+30+((i-1)%7)*86.f,p.y+52+(((i-1)%21)/7)*72.f,78,66};
            DrawRectangleRec(r,Hover(r)?Color{95,91,78,255}:Color{60,58,52,255});
            DrawRectangleLinesEx(r,CursorStack.block==i?2.f:1.f,CursorStack.block==i?GOLD:GRAY);Icon(atlas,r,static_cast<Block>(i));
        }
        DrawText("< Anterior",int(p.x+24),int(p.y+264),17,RAYWHITE);
        DrawText(TextFormat("Pagina %d / %d",inventoryPage+1,(int(BlockCount)-2)/21+1),int(p.x+270),int(p.y+264),17,LIGHTGRAY);
        DrawText("Proxima >",int(p.x+520),int(p.y+264),17,RAYWHITE);
        DrawText(CursorStack.block==Air?"Escolha um bloco para pegar uma pilha de 64.":
            TextFormat("%s: deposite na hotbar ou volte ao inventario.",Names[CursorStack.block]),int(p.x+24),int(p.y+287),16,RAYWHITE);
        for(int i=0;i<9;++i) {
            Rectangle r=Slot(i,p.y+318,p.x+24); DrawRectangleRec(r,{61,59,53,255});
            DrawRectangleLinesEx(r,selected==i?3.f:1.f,selected==i?GOLD:GRAY);Icon(atlas,r,Hotbar[i].block);
            DrawText(TextFormat("%d",i+1),int(r.x+4),int(r.y+4),14,WHITE);
        }
        DrawText("E / ESC: fechar inventario",int(p.x+24),int(p.y+402),15,LIGHTGRAY);
        if(CursorStack.count) {Vector2 m=GetMousePosition();StackIcon(atlas,{m.x-18,m.y-18,40,40},CursorStack);}
    } else if(paused) {
        DrawRectangle(0,0,w,h,Fade(BLACK,.55f));
        const char* title="Menu do Jogo";
        DrawText(title,(w-MeasureText(title,32))/2,h/2-130,32,RAYWHITE);
        ClassicButton(PauseButton(0),"Voltar ao Jogo",true);
        ClassicButton(PauseButton(1),"Opcoes...",true);
        ClassicButton(PauseButton(2),net::Active()?"Desconectar":bw::Active()?"Sair do BedWars":"Salvar e Voltar ao Menu",true);
        if(!message.empty()) DrawText(message.c_str(),(w-MeasureText(message.c_str(),16))/2,h/2+160,16,RAYWHITE);
    }
}
}

void game::UI::DrawPlayerList() const {
    if(bw::Ready()){
        auto players=bw::PlayerList();int width=std::min(420,GetScreenWidth()-32),x=(GetScreenWidth()-width)/2,y=28;
        DrawRectangle(x,y,width,32,{22,24,29,235});DrawText(TextFormat("BEDWARS | %d jogadores",int(players.size())),x+12,y+8,18,GOLD);y+=34;
        constexpr Color colors[]={RED,BLUE,GREEN,YELLOW,{55,215,215,255},RAYWHITE,PINK,GRAY};const char* teams[]={"Vermelho","Azul","Verde","Amarelo","Ciano","Branco","Rosa","Cinza"};
        for(const auto& p:players){DrawRectangle(x,y,width,28,{40,43,50,230});DrawText(p.name.c_str(),x+12,y+6,16,p.name==bw::Nickname()?GOLD:RAYWHITE);const char* team=p.eliminated?"Espectador":teams[std::clamp(p.team,0,7)];DrawText(team,x+width-MeasureText(team,16)-12,y+6,16,p.eliminated?GRAY:colors[std::clamp(p.team,0,7)]);y+=30;}return;
    }
    int w=GetScreenWidth();
    auto lines=[](const std::string& text){return text.empty()?0:std::min(4,1+int(std::count(text.begin(),text.end(),'\n')));};
    int bannerHeight=22*(lines(net::TabHeader())+lines(net::TabFooter()));

        auto players=net::PlayerList();int visible=std::min(int(players.size()),80),rows=std::max(1,std::min(20,visible));
        int columns=std::max(1,(visible+rows-1)/rows),columnWidth=std::min(240,(w-40)/columns);
        int rowHeight=std::clamp((GetScreenHeight()-80-bannerHeight)/rows,12,22);
        int fontSize=std::min(16,rowHeight-2);
        int width=columns*columnWidth,x=(w-width)/2,y=26;
        auto banner=[&](const std::string& text) {
            size_t start=0;int count=0;
            while(start<text.size()&&count++<4){size_t end=text.find('\n',start);std::string line=text.substr(start,end==std::string::npos?std::string::npos:end-start);
                DrawRectangle(x,y,width,22,{22,24,29,220});BeginScissorMode(x,y,width,22);DrawText(line.c_str(),x+(width-MeasureText(line.c_str(),16))/2,y+3,16,WHITE);EndScissorMode();y+=22;if(end==std::string::npos)break;start=end+1;}
        };
        banner(net::TabHeader());DrawRectangle(x,y,width,24,{22,24,29,230});DrawText(TextFormat("Jogadores: %d",int(players.size())),x+8,y+4,16,WHITE);y+=26;
        std::string own=net::Nickname();
        for(int i=0;i<visible;++i){const auto& player=players[i];int px=x+(i/rows)*columnWidth,py=y+(i%rows)*rowHeight;
            DrawRectangle(px+1,py,columnWidth-2,rowHeight-1,{49,53,61,220});BeginScissorMode(px+5,py,columnWidth-39,rowHeight-1);
            DrawText(player.displayName.c_str(),px+6,py+1,fontSize,player.gameMode==3?GRAY:player.name==own?GOLD:WHITE);EndScissorMode();
            int bars=player.ping<0?0:player.ping<150?5:player.ping<300?4:player.ping<600?3:player.ping<1000?2:1;
            for(int b=0;b<5;++b)DrawRectangle(px+columnWidth-26+b*4,py+rowHeight-6-b*2,3,3+b*2,b<bars?LIME:DARKGRAY);
        }
        y+=rows*rowHeight+2;banner(net::TabFooter());
}
