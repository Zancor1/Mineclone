#include "ui.h"
#include "renderer.h"
#include "settings.h"
#include <cmath>
#include <algorithm>
#include <utility>
namespace game {
namespace {
Rectangle Panel() {return {GetScreenWidth()/2.f-330,GetScreenHeight()/2.f-215,660,430};}
Rectangle Slot(int i,float y,float left) {return {left+i*68,y,62,70};}
void Icon(Texture2D atlas,Rectangle slot,Block b) {
    if(b==Air) return;
    int tile=BlockTile(b);
    DrawRectangle(int(slot.x+14),int(slot.y+9),34,34,{68,72,75,255});
    DrawTexturePro(atlas,{float(tile*TileSize),0,TileSize,TileSize},{slot.x+14,slot.y+9,34,34},{0,0},0,WHITE);
    DrawText(Names[b],int(slot.x+(slot.width-MeasureText(Names[b],12))/2),int(slot.y+50),12,RAYWHITE);
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
Rectangle OptionsPanel() {return {GetScreenWidth()/2.f-300,GetScreenHeight()/2.f-195,600,390};}
Rectangle OptionControl(int i) {
    Rectangle p=OptionsPanel();
    if(i==0) return {p.x+40,p.y+65,520,44};
    if(i==1) return {p.x+40,p.y+154,520,26};
    if(i==2) return {p.x+40,p.y+252,520,26};
    return {p.x+140,p.y+325,320,44};
}
void Slider(Rectangle r,float value) {
    DrawRectangleRec(r,{29,29,29,255});DrawRectangleLinesEx(r,2,{105,105,105,255});
    DrawRectangle(int(r.x+3),int(r.y+r.height/2-2),int((r.width-6)*value),4,{148,163,186,255});
    Rectangle knob{r.x+3+value*(r.width-20),r.y+2,14,r.height-4};
    DrawRectangleRec(knob,Hover(r)?RAYWHITE:LIGHTGRAY);DrawRectangleLinesEx(knob,1,GRAY);
}
void ClassicButton(Rectangle r,const char* text,bool enabled) {
    bool hovered=enabled&&Hover(r);
    DrawRectangleRec(r,enabled?(hovered?Color{122,132,161,255}:Color{101,101,101,255}):Color{66,66,66,255});
    DrawRectangleLinesEx(r,2,{20,20,20,255});
    Color highlight=enabled?Color{182,182,182,255}:Color{95,95,95,255};
    DrawLineEx({r.x+2,r.y+2},{r.x+r.width-3,r.y+2},2,highlight);
    DrawLineEx({r.x+2,r.y+2},{r.x+2,r.y+r.height-3},2,highlight);
    DrawLineEx({r.x+3,r.y+r.height-3},{r.x+r.width-3,r.y+r.height-3},2,{45,45,45,255});
    int x=int(r.x+(r.width-MeasureText(text,20))/2),y=int(r.y+14);
    DrawText(text,x+2,y+2,20,Fade(BLACK,.65f));
    DrawText(text,x,y,20,enabled?(hovered?Color{255,245,169,255}:RAYWHITE):GRAY);
}
}
void UI::SetWorlds(std::vector<WorldSave> saves) {
    worlds=std::move(saves);worldScroll=0;selectedWorldPath.clear();
}
void UI::Notify(const std::string& text) {message=text;}
void UI::ResetInteraction() {draggingSlider=0;held=Air;message.clear();}
UiAction UI::Update(GameState state) {
    if(state==GameState::Options) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(OptionControl(0))) Settings.freezeTime=!Settings.freezeTime;
            else if(Hover(OptionControl(1))) draggingSlider=1;
            else if(Hover(OptionControl(2))) draggingSlider=2;
            else if(Hover(OptionControl(3))) return UiAction::CloseOptions;
        }
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) draggingSlider=0;
        if(draggingSlider) {
            Rectangle r=OptionControl(draggingSlider);
            float value=std::clamp((GetMousePosition().x-r.x-10)/(r.width-20),0.f,1.f);
            if(draggingSlider==1) {
                Settings.newWorldMinute=int(std::lround(value*1439));
                SetClockMinute(Settings.newWorldMinute);
            } else Settings.mouseSensitivity=.1f+2.9f*value;
        }
        return UiAction::None;
    }
    if(state==GameState::MainMenu) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(Hover(PauseButton(0))) return UiAction::OpenWorlds;
            if(Hover(PauseButton(1))) return UiAction::OpenOptions;
            if(Hover(PauseButton(2))) return UiAction::QuitApplication;
        }
        return UiAction::None;
    }
    if(state==GameState::WorldSelection) {
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
    if(inventory) {
        int pages=(int(BlockCount)-2)/21+1;
        if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_PAGE_DOWN)) inventoryPage=(inventoryPage+1)%pages;
        if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_PAGE_UP)) inventoryPage=(inventoryPage+pages-1)%pages;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle p=Panel();
            if(Hover({p.x+24,p.y+264,105,22})) inventoryPage=(inventoryPage+pages-1)%pages;
            if(Hover({p.x+520,p.y+264,115,22})) inventoryPage=(inventoryPage+1)%pages;
        }
    }
    for(int i=0;i<9;++i) if(IsKeyPressed(KEY_ONE+i)) {
        selected=i;
        if(inventory&&held!=Air) hotbar[i]=held;
    }
    float wheel=GetMouseWheelMove();
    if(wheel!=0) selected=(selected+(wheel>0?8:1))%9;
    if(!inventory||!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return UiAction::None;
    Rectangle p=Panel();
    for(int i=1+inventoryPage*21;i<BlockCount&&i<=inventoryPage*21+21;++i) {
        Rectangle r{p.x+30+((i-1)%7)*86.f,p.y+52+(((i-1)%21)/7)*72.f,78,66};
        if(Hover(r)) {held=static_cast<Block>(i);return UiAction::None;}
    }
    for(int i=0;i<9;++i) if(Hover(Slot(i,p.y+318,p.x+24))) {
        selected=i;
        if(held!=Air) hotbar[i]=held; else held=hotbar[i];
        return UiAction::None;
    }
    return UiAction::None;
}
void UI::Draw(Texture2D atlas,GameState state) const {
    if(state==GameState::Options) {
        int w=GetScreenWidth(),h=GetScreenHeight();Rectangle p=OptionsPanel();
        DrawRectangle(0,0,w,h,Fade(BLACK,.55f));
        DrawRectangleRec(p,{48,46,42,255});DrawRectangleLinesEx(p,3,{155,151,138,255});
        DrawText("OPCOES",int(p.x+(p.width-MeasureText("OPCOES",26))/2),int(p.y+19),26,RAYWHITE);
        ClassicButton(OptionControl(0),Settings.freezeTime?"Travar horario: LIGADO":"Travar horario: DESLIGADO",true);
        int minute=ClockMinute();
        DrawText(TextFormat("Horario: %02d:%02d",minute/60,minute%60),int(p.x+40),int(p.y+123),20,RAYWHITE);
        Slider(OptionControl(1),minute/1439.f);
        DrawText("00:00",int(p.x+40),int(p.y+185),15,LIGHTGRAY);
        DrawText("12:00",int(p.x+280),int(p.y+185),15,LIGHTGRAY);
        DrawText("23:59",int(p.x+515),int(p.y+185),15,LIGHTGRAY);
        DrawText(TextFormat("Sensibilidade do mouse: %.0f%%",Settings.mouseSensitivity*100),int(p.x+40),int(p.y+221),20,RAYWHITE);
        Slider(OptionControl(2),(Settings.mouseSensitivity-.1f)/2.9f);
        DrawText("10%",int(p.x+40),int(p.y+284),15,LIGHTGRAY);
        DrawText("300%",int(p.x+515),int(p.y+284),15,LIGHTGRAY);
        ClassicButton(OptionControl(3),"Concluido",true);
        if(!message.empty()) DrawText(message.c_str(),int(p.x+12),int(p.y+370),14,GOLD);
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
        ClassicButton(PauseButton(0),"Jogar",true);
        ClassicButton(PauseButton(1),"Opcoes...",true);
        ClassicButton(PauseButton(2),"Sair",true);
        if(!message.empty()) DrawText(message.c_str(),(w-MeasureText(message.c_str(),16))/2,h/2+160,16,RAYWHITE);
        return;
    }
    if(state==GameState::WorldSelection) {
        int w=GetScreenWidth(),h=GetScreenHeight();
        ClearBackground({43,41,37,255});
        const char* title="Selecionar Mundo";
        DrawText(title,(w-MeasureText(title,30))/2,34,30,RAYWHITE);
        DrawText(TextFormat("%d mundos | Use a roda para rolar",int(worlds.size())),w/2-190,76,17,LIGHTGRAY);
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
            Icon(atlas,r,hotbar[i]); DrawText(TextFormat("%d",i+1),int(r.x+5),int(r.y+4),14,WHITE);
        }
    }
    DrawFPS(w-100,16);
    if(inventory) {
        DrawRectangle(0,0,w,h,Fade(BLACK,.6f));Rectangle p=Panel();
        DrawRectangleRec(p,{45,43,39,255});DrawRectangleLinesEx(p,3,{155,151,138,255});
        DrawText("INVENTARIO CRIATIVO",int(p.x+24),int(p.y+16),22,RAYWHITE);
        for(int i=1+inventoryPage*21;i<BlockCount&&i<=inventoryPage*21+21;++i) {
            Rectangle r{p.x+30+((i-1)%7)*86.f,p.y+52+(((i-1)%21)/7)*72.f,78,66};
            DrawRectangleRec(r,Hover(r)?Color{95,91,78,255}:Color{60,58,52,255});
            DrawRectangleLinesEx(r,held==i?2.f:1.f,held==i?GOLD:GRAY);Icon(atlas,r,static_cast<Block>(i));
        }
        DrawText("< Anterior",int(p.x+24),int(p.y+264),17,RAYWHITE);
        DrawText(TextFormat("Pagina %d / %d",inventoryPage+1,(int(BlockCount)-2)/21+1),int(p.x+270),int(p.y+264),17,LIGHTGRAY);
        DrawText("Proxima >",int(p.x+520),int(p.y+264),17,RAYWHITE);
        DrawText(held==Air?"Escolha um bloco, depois clique em uma posicao da hotbar.":
            TextFormat("%s selecionado: clique na hotbar ou aperte 1-9.",Names[held]),int(p.x+24),int(p.y+287),16,RAYWHITE);
        for(int i=0;i<9;++i) {
            Rectangle r=Slot(i,p.y+318,p.x+24); DrawRectangleRec(r,{61,59,53,255});
            DrawRectangleLinesEx(r,selected==i?3.f:1.f,selected==i?GOLD:GRAY);Icon(atlas,r,hotbar[i]);
            DrawText(TextFormat("%d",i+1),int(r.x+4),int(r.y+4),14,WHITE);
        }
        DrawText("E / ESC: fechar inventario",int(p.x+24),int(p.y+402),15,LIGHTGRAY);
    } else if(paused) {
        DrawRectangle(0,0,w,h,Fade(BLACK,.55f));
        const char* title="Menu do Jogo";
        DrawText(title,(w-MeasureText(title,32))/2,h/2-130,32,RAYWHITE);
        ClassicButton(PauseButton(0),"Voltar ao Jogo",true);
        ClassicButton(PauseButton(1),"Opcoes...",true);
        ClassicButton(PauseButton(2),"Salvar e Voltar ao Menu",true);
        if(!message.empty()) DrawText(message.c_str(),(w-MeasureText(message.c_str(),16))/2,h/2+160,16,RAYWHITE);
    }
}
}
