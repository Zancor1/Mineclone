#include "external_arenas.h"
#include "projectile_physics.h"
#include "arrow_model.h"
#include "arrow_collision.h"
#include "bow_model.h"
#include "progression.h"
#include "cosmetics.h"
#include <random>
#include <chrono>
#include <fstream>
#include "bedwars.h"
#include "inventory_layout.h"
#include "settings.h"
#include "map_blocks.h"
#include "lan_transport.h"
#include "lighting.h"
#include "survival.h"
#include "items.h"
#include "renderer.h"
#include "crafting.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>
#include <set>
#include <stdexcept>
namespace game::bw {
namespace {
using Bytes=std::vector<unsigned char>;
constexpr int Version=LanProtocolVersion,Teams=8,MaxPlayers=16;
std::filesystem::path connectionLog;
double joinStarted=0;
std::string connectionFailure;
double ConnectionNow(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void StartConnectionTrace(const std::string& operation){
    joinStarted=ConnectionNow();connectionFailure.clear();
    try{connectionLog=std::filesystem::u8path(GetApplicationDirectory())/"bedwars-connection.log";std::ofstream out(connectionLog,std::ios::trunc);out<<"MineClone BedWars protocol "<<Version<<" build "<<__DATE__<<" "<<__TIME__;
#ifdef _WIN32
        out<<" Windows";
#else
        out<<" Linux";
#endif
        out<<"\n"<<operation<<"\n";
    }catch(...){connectionLog.clear();}
}
Vector3 bases[8]{};
int islandGap=64,mapId=0,teamCapacity=1;
void SetLayout(int gap){islandGap=gap;auto positions=ArenaBases(gap,mapId);std::copy(positions.begin(),positions.end(),bases);}

constexpr Color colors[8]={RED,BLUE,GREEN,YELLOW,{55,215,215,255},RAYWHITE,PINK,GRAY};
constexpr Block wool[8]={RedWool,BlueWool,GreenWool,YellowWool,CyanWool,WhiteWool,PinkWool,GrayWool};
const char* teamNames[]={"Vermelho","Azul","Verde","Amarelo","Ciano","Branco","Rosa","Cinza"};
enum Message {Hello=1,Welcome,Snapshot,Edit,Position,DigRequest,PlaceRequest,AttackRequest,BuyRequest,UseRequest,SwapRequest,StartRequest,RestartRequest,Force,Error,ChatMessage,ChooseTeam,SprintPause,HoldUseRequest,SwingAnimation,CraftRequest,InventoryClickRequest,CosmeticRequest,CosmeticResult,BowRequest,HitEffect,PenguinUpdate};
struct Buffer {
    Bytes bytes;size_t offset=0;
    void Byte(int n){bytes.push_back(static_cast<unsigned char>(n));}
    void Int(int n){for(int i=0;i<4;++i)Byte(uint32_t(n)>>(i*8));}
    void Float(float f){int n;std::memcpy(&n,&f,4);Int(n);}
    void Text(const std::string& s){Byte(int(std::min(size_t(80),s.size())));bytes.insert(bytes.end(),s.begin(),s.begin()+std::min(size_t(80),s.size()));}
    void Vec(Vector3 p){Float(p.x);Float(p.y);Float(p.z);}
    int Byte(){if(offset>=bytes.size())throw std::runtime_error("Pacote BedWars incompleto");return bytes[offset++];}
    int Int(){uint32_t n=0;for(int i=0;i<4;++i)n|=uint32_t(Byte())<<(i*8);return int32_t(n);}
    float Float(){int n=Int();float f;std::memcpy(&f,&n,4);if(!std::isfinite(f))throw std::runtime_error("Numero BedWars invalido");return f;}
    std::string Text(){int n=Byte();if(n>80||offset+n>bytes.size())throw std::runtime_error("Texto BedWars invalido");std::string s(bytes.begin()+offset,bytes.begin()+offset+n);offset+=n;return s;}
    Vector3 Vec(){float x=Float(),y=Float(),z=Float();return {x,y,z};}
};
Buffer Packet(int kind){Buffer b;b.Byte(kind);return b;}
struct Fighter {
    progression::Profile profile;
    bool connected=false,bed=true,eliminated=false,sneaking=false;int team=0,peer=-1,lastAttacker=-1,selected=0,revision=1,teleport=0,kills=0,finals=0,beds=0,armor=0,pick=0,axe=0,sharp=0,protection=0,forge=0;
    std::string name;Vector3 p{},display{};float knockSpeed=0,knockUntil=0,yaw=0,pitch=0,hp=20,respawn=0,hurtClock=0,regen=0,absorption=0,lastPosition=0,lastAction=-10,lastChat=-10,lastHurt=-100,absorbTime=0,digStart=-10,speedTime=0,jumpTime=0,invisTime=0,healClock=0,regenClock=0,eating=0,eatHeartbeat=-10,fallDistance=0,swingAge=.35f;int eatSlot=-1;
    float hitFlash=0,meleeImmuneUntil=0;
    float bowStart=-1,bowHeartbeat=-10,lastBowShot=-10;int bowSlot=-1;
    Hit digging{};std::array<ItemStack,36> inventory{};std::array<ItemStack,4> crafting{};ItemStack cursor{};
};
Color Outfit(const Fighter& f){return colors[f.team];}
struct Generator {Vector3 p;Block item;int stock=0;float clock=0,interval=1;int cap=32,team=-1;};
struct Shot {Vector3 p,v;int owner;Block kind;float life;float tick=0;bool embedded=false;Vector3 support{};float damage=4;bool critical=false;};
struct Offer {Block item,currency;int count,cost;const char* title;int special=0;};
constexpr Offer offers[]={
 {WhiteWool,IronIngot,16,4,"16 Las"},{EndStone,IronIngot,12,24,"12 Pedras do fim"},{Planks,GoldIngot,16,4,"16 Tabuas"},{Glass,IronIngot,4,12,"4 Vidros"},
 {Obsidian,Emerald,4,4,"4 Obsidianas"},{StoneSword,IronIngot,1,10,"Espada de pedra"},{IronSword,GoldIngot,1,7,"Espada de ferro"},{DiamondSword,Emerald,1,4,"Espada de diamante"},
 {WoodenPickaxe,IronIngot,1,10,"Picareta madeira"},{StonePickaxe,IronIngot,1,20,"Picareta pedra"},{IronPickaxe,GoldIngot,1,6,"Picareta ferro"},
 {GoldenApple,GoldIngot,1,3,"Maca dourada"},{Tnt,GoldIngot,1,4,"TNT"},{Fireball,IronIngot,1,40,"Bola de fogo"},{EnderPearl,Emerald,1,4,"Perola"},
 {Bow,GoldIngot,1,12,"Arco"},{Arrow,GoldIngot,8,2,"8 Flechas"},{Ladder,IronIngot,16,4,"16 Escadas"},
 {Air,GoldIngot,1,12,"Armadura de ferro",1},{Air,Emerald,1,6,"Armadura diamante",2},{Air,Diamond,1,4,"Afiacao da equipe",3},{Air,Diamond,1,2,"Protecao da equipe",4},{Air,Diamond,1,4,"Gerador melhorado",5},
 {Stick,GoldIngot,1,5,"Graveto - Knockback II"},
 {Air,IronIngot,0,0,"",7},
 {WoodenAxe,IronIngot,1,10,"Machado madeira"},{StoneAxe,IronIngot,1,20,"Machado pedra"},{IronAxe,GoldIngot,1,6,"Machado ferro"},
 {Air,IronIngot,1,40,"Armadura de malha",6},
 {SpeedPotion,Emerald,1,1,"Velocidade II - 45s"},{JumpPotion,Emerald,1,1,"Salto II - 45s"},{InvisibilityPotion,Emerald,1,2,"Invisibilidade - 30s"}};
std::string serverTitle;
LanTransport wire;LanDiscovery discovery;unsigned short roomPort=25566;std::array<Fighter,MaxPlayers> fighters;std::array<ChestInventory,Teams> chests;std::vector<Generator> generators;std::vector<Shot> shots;
std::map<int,int> peerTeam;std::set<int> placed;bool active=false,ready=false,host=false;int me=0,phase=0,winner=-1,participants=0,epoch=0,seenEpoch=-1,seenTeleport=-1,seenRevision=-1,shopKind=0,shopCategory=0;float clock=0,phaseClock=0,snapshotClock=0,positionClock=0;std::string status;std::vector<std::string> chatMessages;
Sound hitSound{};
bool hitSoundChecked=false;
void LoadHitSound(){
    if(hitSoundChecked||!IsAudioDeviceReady())return;
    hitSoundChecked=true;
    for(const auto& base:{std::filesystem::u8path(GetApplicationDirectory()),std::filesystem::current_path()}){
        for(const char* name:{"Hit.mp3","hit.mp3"}){
            auto path=base/"sounds"/name;
            if(std::filesystem::exists(path)){hitSound=LoadSound(path.string().c_str());return;}
        }
    }
}
void ShowHit(int victim,int attacker){
    if(victim<0||victim>=MaxPlayers||attacker<0||attacker>=MaxPlayers)return;
    fighters[victim].hitFlash=.2f;
    float distance=Vector3Distance(fighters[me].p,fighters[victim].p);
    if(hitSound.frameCount&&(me==victim||me==attacker||distance<24.f)){
        SetSoundVolume(hitSound,(me==victim||me==attacker)?.85f:.65f*(1.f-distance/24.f));
        PlaySound(hitSound);
    }
}
void BroadcastHit(int victim,int attacker){
    ShowHit(victim,attacker);
    auto packet=Packet(HitEffect);packet.Byte(victim);packet.Byte(attacker);wire.Broadcast(packet.bytes);
}
struct EscapingPenguin {bool active=false;Vector3 p{},display{},direction{};float age=0,velocityY=0;Sound scream{};};
std::array<EscapingPenguin,Teams> penguins;
Sound screamSound{};bool screamChecked=false;
void StopPenguin(EscapingPenguin& p){if(p.scream.frameCount){StopSound(p.scream);UnloadSoundAlias(p.scream);}p={};}
void ClearPenguins(){for(auto& p:penguins)StopPenguin(p);}
void PenguinAudio(float dt){
    (void)dt;
    if(!IsAudioDeviceReady())return;
    if(!screamChecked){screamChecked=true;for(const auto& base:{std::filesystem::u8path(GetApplicationDirectory()),std::filesystem::current_path()}){auto path=base/"sounds"/"ahh.mp3";if(std::filesystem::exists(path)){screamSound=LoadSound(path.string().c_str());break;}}}
    for(auto& p:penguins)if(p.active){
        if(!p.scream.frameCount&&screamSound.frameCount)p.scream=LoadSoundAlias(screamSound);
        if(!p.scream.frameCount)continue;
        float distance=Vector3Distance(fighters[me].p,p.display);
        if(distance>=24.f){StopSound(p.scream);continue;}
        float volume=1.f-distance/24.f;SetSoundVolume(p.scream,.9f*volume*volume);
        if(!IsSoundPlaying(p.scream))PlaySound(p.scream);
    }
}
void SpawnPenguin(int team,const std::array<Vector3,2>& beds){
    auto& p=penguins[team];StopPenguin(p);p.active=true;
    p.p=p.display=Vector3Add(beds[0],{.5f,.01f,.5f});
    p.direction=Vector3Normalize(Vector3Subtract(beds[0],beds[1]));p.direction.y=0;
    if(Vector3Length(p.direction)<.5f)p.direction={0,0,-1};
}
void SimulatePenguins(float dt){
    for(auto& p:penguins)if(p.active){
        float remaining=dt;
        while(remaining>0&&p.active){float step=std::min(.02f,remaining);remaining-=step;p.age+=step;
            if(p.age<.25f)continue;
            Vector3 next=Vector3Add(p.p,Vector3Scale(p.direction,5.5f*step));
            // Step onto voxel ledges instead of getting stuck against base decorations.
            auto ledges=CollisionBoxesAt(int(std::floor(next.x)),int(std::floor(p.p.y+.02f)),int(std::floor(next.z)));
            for(int i=0;i<ledges.count;++i){auto b=ledges.boxes[i];if(next.x>=b.min.x&&next.x<=b.max.x&&next.z>=b.min.z&&next.z<=b.max.z&&b.max.y>next.y+.01f&&b.max.y<=p.p.y+1.05f){next.y=b.max.y;p.velocityY=0;}}
            float previousY=next.y;p.velocityY=std::max(-24.f,p.velocityY-20.f*step);next.y+=p.velocityY*step;
            for(int y=int(std::floor(next.y));y<=int(std::floor(previousY));++y){auto support=CollisionBoxesAt(int(std::floor(next.x)),y,int(std::floor(next.z)));for(int i=0;i<support.count;++i){auto b=support.boxes[i];if(next.x>=b.min.x&&next.x<=b.max.x&&next.z>=b.min.z&&next.z<=b.max.z&&b.max.y<=previousY+.001f&&b.max.y>=next.y){next.y=b.max.y;p.velocityY=0;}}}
            p.p=next;
            if(p.p.y< -8.f)StopPenguin(p);
        }
    }
}
void SendPenguins(){auto b=Packet(PenguinUpdate);int count=0;for(const auto& p:penguins)if(p.active)++count;b.Byte(count);for(int team=0;team<Teams;++team){auto& p=penguins[team];if(!p.active)continue;b.Byte(team);b.Vec(p.p);b.Vec(p.direction);b.Float(p.age);}wire.Broadcast(b.bytes);}
void ReadPenguins(Buffer& b){
    int count=b.Byte();if(count>Teams)throw std::runtime_error("Pinguins invalidos");std::array<bool,Teams> present{};
    for(int i=0;i<count;++i){int team=b.Byte();if(team>=Teams||present[team])throw std::runtime_error("Time de pinguim invalido");present[team]=true;auto position=b.Vec(),direction=b.Vec();float age=b.Float();auto& p=penguins[team];if(!p.active){p.active=true;p.display=position;}p.p=position;p.direction=direction;p.age=age;}
    for(int team=0;team<Teams;++team)if(!present[team])StopPenguin(penguins[team]);
}
void DrawPenguins(){
    for(const auto& p:penguins)if(p.active){
        float beat=std::sin(p.age*19.f),light=std::clamp(Daylight(),.35f,1.f);
        auto color=[&](Color c){return Color{static_cast<unsigned char>(c.r*light),static_cast<unsigned char>(c.g*light),static_cast<unsigned char>(c.b*light),255};};
        auto cube=[&](Vector3 pos,Vector3 size,Color c){DrawCubeV(pos,size,color(c));};
        rlPushMatrix();rlTranslatef(p.display.x,p.display.y,p.display.z);rlRotatef(std::atan2(-p.direction.x,-p.direction.z)*RAD2DEG,0,1,0);
        rlRotatef(beat*3.f,0,0,1);
        cube({0,.46f,0},{.46f,.55f,.32f},{119,124,130,255});
        cube({0,.47f,-.17f},{.32f,.43f,.035f},{233,234,226,255});
        cube({0,.87f,0},{.40f,.34f,.36f},{36,38,43,255});
        cube({0,.87f,-.19f},{.31f,.25f,.025f},{239,239,230,255});
        for(float side:{-.09f,.09f})cube({side,.91f,-.21f},{.05f,.06f,.025f},BLACK);
        cube({0,.82f,-.24f},{.085f,.065f,.12f},ORANGE);
        for(float side:{-1.f,1.f}){
            rlPushMatrix();rlTranslatef(side*.25f,.67f,0);rlRotatef(side*(150.f+beat*8.f),0,0,1);cube({0,-.18f,0},{.10f,.41f,.15f},{100,106,114,255});rlPopMatrix();
            rlPushMatrix();rlTranslatef(side*.13f,.17f,0);rlRotatef(side*beat*35.f,1,0,0);cube({0,-.1f,-.07f},{.15f,.10f,.27f},ORANGE);rlPopMatrix();
        }
        rlPopMatrix();
    }
}
int PlayerCount(){return int(std::count_if(fighters.begin(),fighters.end(),[](const Fighter& f){return f.connected;}));}
int TeamLimit(){return MapTeams(mapId);}
int TeamOf(int id){return fighters[id].team;}
int TeamCount(int team){return int(std::count_if(fighters.begin(),fighters.end(),[&](const Fighter& f){return f.connected&&f.team==team;}));}
bool TeamBed(int team){for(const auto& f:fighters)if(f.connected&&f.team==team&&f.bed)return true;return false;}
void SetTeamBed(int team,bool bed){for(auto& f:fighters)if(f.team==team)f.bed=bed;}
int TeamForge(int team){for(const auto& f:fighters)if(f.connected&&f.team==team)return f.forge;return 0;}
bool Balanced(const std::array<int,Teams>& counts){
    int low=MaxPlayers,high=0,occupied=0,total=0;
    for(int t=0;t<TeamLimit();++t){int n=counts[t];total+=n;if(n>teamCapacity)return false;if(n){low=std::min(low,n);high=std::max(high,n);++occupied;}}
    return (total<2||occupied>=2)&&high-low<=1;
}
bool CanSelect(int id,int team){
    if(phase!=0||team<0||team>=TeamLimit()||!fighters[id].connected)return false;
    std::array<int,Teams> counts{};for(const auto& f:fighters)if(f.connected)++counts[f.team];
    --counts[TeamOf(id)];++counts[team];return Balanced(counts);
}
void BalanceTeams(){
    std::array<int,Teams> counts{};for(const auto& f:fighters)if(f.connected)++counts[f.team];
    while(!Balanced(counts)){
        int from=int(std::max_element(counts.begin(),counts.end())-counts.begin());
        int to=int(std::min_element(counts.begin(),counts.begin()+TeamLimit())-counts.begin());
        for(auto& f:fighters)if(f.connected&&f.team==from){f.team=to;--counts[from];++counts[to];break;}
    }
}
int AvailableTeam(){int team=0;for(int t=1;t<TeamLimit();++t)if(TeamCount(t)<TeamCount(team))team=t;return team;}
bool ClearBody(Vector3 p){BoundingBox body{{p.x-.29f,p.y+.001f,p.z-.29f},{p.x+.29f,p.y+1.8f,p.z+.29f}};for(int x=int(std::floor(body.min.x));x<=int(std::floor(body.max.x));++x)for(int y=int(std::floor(p.y));y<=int(std::floor(body.max.y));++y)for(int z=int(std::floor(body.min.z));z<=int(std::floor(body.max.z));++z){auto boxes=CollisionBoxesAt(x,y,z);for(int i=0;i<boxes.count;++i)if(CheckCollisionBoxes(body,boxes.boxes[i]))return false;}return true;}
Vector3 MateSpawn(Vector3 p){constexpr int offsets[][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{2,0},{0,2}};for(auto& o:offsets){Vector3 q{p.x+o[0],p.y,p.z+o[1]};if(!ClearBody(q))continue;auto below=CollisionBoxesAt(int(q.x),int(q.y)-1,int(q.z));for(int i=0;i<below.count;++i)if(std::abs(below.boxes[i].max.y-q.y)<.02f)return q;}return p;}
Vector3 TeamSpawn(int id){auto p=mapId>=FirstExternalArena?ExternalArenaAt(mapId).teams[TeamOf(id)].spawn:mapId==4?AirshowSpawn(TeamOf(id)):bases[TeamOf(id)];int seat=0;for(int i=0;i<id;++i)if(fighters[i].connected&&TeamOf(i)==TeamOf(id))++seat;if(mapId>=3&&seat){p=MateSpawn(p);seat=0;}return {p.x+seat*1.2f,p.y+.001f,p.z-((mapId>=3)?0:1.5f)};}
Vector3 Vendor(int team,int kind){if(mapId>=FirstExternalArena)return ExternalArenaAt(mapId).teams[team].vendors[kind];if(mapId==1)return RuinsVendor(team,kind,islandGap);if(mapId==4)return AirshowVendor(team,kind);if(mapId==3)return {team==0?74.5f:182.5f,26,float((team==0?(kind?34:22):(kind?22:34))+128)+.5f};return {bases[team].x+(kind?4.f:-4.f),26,bases[team].z+(kind?2.f:-2.f)};}
Vector3 TeamChest(int team){if(mapId>=FirstExternalArena)return ExternalArenaAt(mapId).teams[team].chest;if(mapId==4)return AirshowChest(team);if(mapId==3)return {float((team?51:-51)+128),26,float((team?32:24)+128)};return {bases[team].x+4,26,bases[team].z-2};}
bool ChestReach(Vector3 p,int team){if(Vector3Distance(p,TeamChest(team))<5)return true;return mapId==3&&Vector3Distance(p,{float((team?51:-51)+128),26,float((team?24:32)+128)})<5;}
void RemoveBed(int team,bool escape=false);
bool ShopReach(Vector3 p,int kind){for(int team=0;team<TeamLimit();++team)if(Vector3Distance(p,Vendor(team,kind))<5)return true;return false;}
int Cell(int x,int y,int z){return (x*Height+y)*Width+z;}
Vector3 LobbyPosition(int id){return {125.5f+(id%4)*2,(MapLobbyY(mapId)+1.001f),128.5f+(id/4)*2};}
bool Fighting(const Fighter& f){return f.connected&&!f.eliminated&&f.respawn<=0;}
bool Live(int id){return phase==1&&Fighting(fighters[id]);}
void Stack(Buffer& b,const ItemStack& s){b.Byte(s.block);b.Byte(s.count);b.Int(s.damage);}
ItemStack Stack(Buffer& b){int block=b.Byte(),count=b.Byte(),damage=b.Int();if(block>=BlockCount||count>64||damage<0||damage>65535)throw std::runtime_error("Item BedWars invalido");return {Block(block),static_cast<unsigned char>(count),static_cast<unsigned short>(damage)};}
int Count(const Fighter& f,Block b){int count=0;for(auto s:f.inventory)if(s.block==b)count+=s.count;return count;}
bool Give(Fighter& f,Block item,int count,int damage=0){ItemStack stack{item,static_cast<unsigned char>(count),static_cast<unsigned short>(damage)};auto inventory=f.inventory;TransferStack(stack,inventory.data(),inventory.size());if(stack.count)return false;f.inventory=inventory;++f.revision;return true;}
void Consume(Fighter& f,Block b,int count){for(auto& s:f.inventory)if(s.block==b&&s.count){int take=std::min(count,int(s.count));s.count-=take;count-=take;if(!s.count)s={};if(!count)break;}++f.revision;}
void Notice(const std::string& text){status=text;}
void AddChat(const std::string& text){chatMessages.push_back(text);if(chatMessages.size()>8)chatMessages.erase(chatMessages.begin());}
void Tell(int id,const std::string& text){if(id==0){cosmetics::CancelWaiting();Notice(text);}else {auto b=Packet(Error);b.Text(text);wire.Send(fighters[id].peer,b.bytes);}}
void Teleport(int id,Vector3 p){fighters[id].p=p;fighters[id].display=p;fighters[id].meleeImmuneUntil=0;++fighters[id].teleport;}
int ArmorRank(int armor){return armor==2?3:armor==1?2:armor==3?1:0;}
Block PickItem(int level){return level==1?WoodenPickaxe:level==2?StonePickaxe:IronPickaxe;}
Block AxeItem(int level){return level==1?WoodenAxe:level==2?StoneAxe:IronAxe;}
int ToolLevel(Block item){if(item==WoodenPickaxe||item==WoodenAxe)return 1;if(item==StonePickaxe||item==StoneAxe)return 2;if(item==IronPickaxe||item==IronAxe)return 3;return 0;}
bool IsAxe(Block item){return item==WoodenAxe||item==StoneAxe||item==IronAxe;}
int OfferCategory(const Offer& o){if((o.special>=1&&o.special<=2)||o.special==6)return 2;if(Sword(o.item)||o.item==Stick)return 1;if(ToolLevel(o.item))return 3;if(o.item>=SpeedPotion&&o.item<=InvisibilityPotion)return 4;if(o.item!=Tnt&&o.item!=Ladder&&Placeable(o.item))return 0;return 5;}
bool ShowOffer(const Offer& o,const Fighter& f){if(o.special==7)return false;bool upgrade=o.special>=3&&o.special<=5;if(upgrade!=bool(shopKind))return false;if(shopKind)return true;if(OfferCategory(o)!=shopCategory)return false;int level=ToolLevel(o.item);return !level||level==std::min(3,(IsAxe(o.item)?f.axe:f.pick)+1);}
void Loadout(int id){auto& f=fighters[id];f.inventory={};f.crafting={};f.cursor={};Give(f,WoodenSword,1);if(f.pick)Give(f,PickItem(f.pick),1);if(f.axe)Give(f,AxeItem(f.axe),1);f.hp=20;f.absorption=f.regen=f.speedTime=f.jumpTime=f.invisTime=0;f.healClock=f.regenClock=f.eating=f.fallDistance=0;f.eatSlot=-1;f.bowStart=-1;f.bowSlot=-1;f.sneaking=false;f.digging={};++f.revision;}
void Write(int x,int y,int z,Block b){if(Inside(x,y,z)){if(Liquid(b))SetRemoteBlock(x,y,z,b);else World[x][y][z]=b;}}
Vector3 OriginalForge(Vector3 expected){
    std::set<int> visited;Vector3 best=expected;float bestScore=-1e9f;
    auto slab=[&](int x,int y,int z){return Inside(x,y+2,z)&&std::abs(x-expected.x)<14&&std::abs(z-expected.z)<14&&MapStateAt(x,y,z).shape==MapShape::Slab&&At(x,y+1,z)==Air&&At(x,y+2,z)==Air;};
    for(int y=int(expected.y)-2;y<=int(expected.y)+1;++y)for(int x=int(expected.x)-13;x<=int(expected.x)+13;++x)for(int z=int(expected.z)-13;z<=int(expected.z)+13;++z){
        if(!slab(x,y,z)||visited.count(Cell(x,y,z)))continue;
        std::vector<Vector3> group{{float(x),float(y),float(z)}};visited.insert(Cell(x,y,z));
        for(size_t i=0;i<group.size();++i)for(auto d:std::array<Vector3,4>{{{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}}}){int nx=int(group[i].x+d.x),nz=int(group[i].z+d.z);if(slab(nx,y,nz)&&visited.insert(Cell(nx,y,nz)).second)group.push_back({float(nx),float(y),float(nz)});}
        Vector3 center{};for(auto c:group)center=Vector3Add(center,Vector3Add(c,{.5f,0,.5f}));center=Vector3Scale(center,1.f/group.size());
        if(!slab(int(center.x),y,int(center.z)))continue;
        center.y=y+((MapStateAt(int(center.x),y,int(center.z)).data&8)?1.f:.5f);
        float dx=center.x-expected.x,dz=center.z-expected.z,dy=center.y-expected.y;
        float score=std::min(9.f,float(group.size()))*8.f-.5f*(dx*dx+dz*dz)-24*dy*dy;
        if(score>bestScore){bestScore=score;best=center;}
    }
    auto boxes=CollisionBoxesAt(int(best.x),int(std::ceil(best.y))-1,int(best.z));
    if(bestScore<-1e8f&&boxes.count){float top=boxes.boxes[0].max.y;for(int i=1;i<boxes.count;++i)top=std::max(top,boxes.boxes[i].max.y);best.y=top;}
    return best;
}
void FinishGenerators(){
    for(auto& g:generators){
        if(mapId>=3){
            if(g.item==IronIngot||g.item==GoldIngot)g.p=OriginalForge(g.p);
            else {auto boxes=CollisionBoxesAt(int(g.p.x),int(std::ceil(g.p.y))-1,int(g.p.z));if(boxes.count){float top=boxes.boxes[0].max.y;for(int i=1;i<boxes.count;++i)top=std::max(top,boxes.boxes[i].max.y);g.p.y=top;}}
            continue;
        }
        int x=int(g.p.x),z=int(g.p.z),y=int(std::ceil(g.p.y))-1;
        g.p={x+.5f,y+.5f,z+.5f};
        Block center=g.item==Diamond?BlockOfDiamond:g.item==Emerald?BlockOfEmerald:StoneBricks;
        for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz){
            if(!Inside(x+dx,y,z+dz))continue;
            Block old=At(x+dx,y,z+dz);if(old==Chest||old==Bed||old==BedHead)continue;
            Write(x+dx,y,z+dz,MapSlab);SetMapBlockState(x+dx,y,z+dz,{MapSlab,dx==0&&dz==0?center:StoneBricks,MapShape::Slab,0});
        }
    }
    InvalidateLighting();
}
void GenerateArena(){
    ClearRemoteWorld();ClearItems();PersonalCrafting={};WorkbenchCrafting={};EquippedArmor={};RegionOriginX=RegionOriginZ=0;SetClockMinute(720);placed.clear();shots.clear();generators.clear();chests={};
    BuildArenaTerrain(mapId,islandGap,Write);
    if(mapId==3){generators.push_back({{128.5f,25,156.5f},Emerald,0,0,35,4});for(int team=0;team<2;++team){Vector3 p{team?186.5f:70.5f,25.5f,156.5f};generators.push_back({p,IronIngot,0,0,2.5f,48,team});generators.push_back({p,GoldIngot,0,0,8,16,team});}generators.push_back({{128.5f,27,183.5f},Diamond,0,0,25,8});FinishGenerators();return;}
    if(mapId>=FirstExternalArena){const auto& arena=ExternalArenaAt(mapId);for(auto p:arena.emeralds)generators.push_back({p,Emerald,0,0,35,4});for(int team=0;team<8;++team){auto p=arena.teams[team].forge;generators.push_back({p,IronIngot,0,0,2.5f,48,team});generators.push_back({p,GoldIngot,0,0,8,16,team});}for(auto p:arena.diamonds)generators.push_back({p,Diamond,0,0,25,8});FinishGenerators();return;}
    if(mapId==4){for(auto p:AirshowResources(true))generators.push_back({p,Emerald,0,0,35,4});for(int t=0;t<8;++t){auto p=AirshowForge(t);generators.push_back({p,IronIngot,0,0,2.5f,48,t});generators.push_back({p,GoldIngot,0,0,8,16,t});}for(auto p:AirshowResources(false))generators.push_back({p,Diamond,0,0,25,8});FinishGenerators();return;}
    generators.push_back({{128.5f,26,128.5f},Emerald,0,0,35,4});
    for(int team=0;team<TeamLimit();++team){auto base=bases[team];int x=int(base.x),z=int(base.z);generators.push_back({{x+.5f,26,z-2.5f},IronIngot,0,0,2.5f,48,team});generators.push_back({{x+.5f,26,z-2.5f},GoldIngot,0,0,8,16,team});}
    for(auto position:DiamondIslands(islandGap,mapId))generators.push_back({position,Diamond,0,0,25,8});
    FinishGenerators();
}
void ClaimProfile(int id);
void ResetMatch(){ClearPenguins();
    seenTeleport=seenRevision=-1;++epoch;GenerateArena();phase=0;phaseClock=clock=0;winner=-1;participants=0;
    for(int i=0;i<MaxPlayers;++i){auto& f=fighters[i];auto previous=f;f=Fighter{};f.profile=previous.profile;f.name=previous.name;f.peer=previous.peer;f.team=previous.team;f.connected=previous.connected;f.pitch=-.55f;if(f.connected){Give(f,WhiteWool,1);if(host)ClaimProfile(i);}Teleport(i,LobbyPosition(i));}
    BalanceTeams();Notice("Lobby: clique com a la para escolher o time. Host: P inicia.");
}
void EditCell(int x,int y,int z,Block block){Change(x,y,z,block);auto b=Packet(Edit);b.Int(x);b.Int(y);b.Int(z);b.Byte(block);wire.Broadcast(b.bytes);}
void RemoveBed(int team,bool escape){auto beds=MapBeds(mapId,team,bases[team]);bool existed=At(int(beds[0].x),int(beds[0].y),int(beds[0].z))==Bed;SetTeamBed(team,false);for(auto p:beds)EditCell(int(p.x),int(p.y),int(p.z),Air);if(escape&&existed){SpawnPenguin(team,beds);SendPenguins();}}
Vector3 localForce{},localMeleeForce{},localExplosionForce{};bool localSprintPause=false;
void Knock(int id,Vector3 v,bool melee=false,bool explosion=false){fighters[id].knockSpeed=std::hypot(v.x,v.z);fighters[id].knockUntil=clock+1.5f;
    if(melee){
        // Recovery covers both the hop and the horizontal impulse decaying below 0.75 blocks/s.
        float recovery=std::max(2.f*std::max(0.f,v.y)/28.f,std::log(std::max(1.f,fighters[id].knockSpeed/.75f))/4.5f);
        fighters[id].meleeImmuneUntil=clock+recovery;
    }if(id==0){if(explosion)localExplosionForce=Vector3Add(localExplosionForce,v);else if(melee)localMeleeForce=v;else localForce=Vector3Add(localForce,v);}else {auto b=Packet(Force);b.Vec(v);b.Byte(explosion?2:melee?1:0);wire.Send(fighters[id].peer,b.bytes);}}
progression::Profile joiningProfile,lastLocalProfile;bool localProfileSaved=false;
std::filesystem::path ProfileDirectory(int id){return id==0?progression::Directory:progression::Directory/"host-cache";}
void ProfilePacket(Buffer& b,const progression::Profile& p){b.Int(p.xp);b.Int(p.crates);b.Int(p.pending);for(int i=0;i<3;++i){b.Int(int(p.unlocked[i]));b.Byte(p.skins[i]);}}
progression::Profile ReadProfile(Buffer& b){progression::Profile p;p.xp=b.Int();p.crates=b.Int();p.pending=b.Int();for(int i=0;i<3;++i){p.unlocked[i]=uint32_t(b.Int());p.skins[i]=b.Byte();}if(!progression::Valid(p))throw std::runtime_error("Perfil local invalido");return p;}
bool LoadGuestProfile(const std::string& name,progression::Profile& profile,std::string& error){
    std::error_code ec;auto cache=progression::Directory/"host-cache";
    if(std::filesystem::exists(cache/(progression::Key(name)+".profile"),ec))return progression::Load(name,profile,error,cache);
    return progression::Load(name,profile,error);
}
void PersistLocalProfile(){
    if(host||!ready)return;
    const auto& f=fighters[me];if(localProfileSaved&&lastLocalProfile==f.profile)return;
    std::string error;if(progression::Save(f.name,f.profile,error)){lastLocalProfile=f.profile;localProfileSaved=true;}else {status="Falha ao salvar perfil local: "+error;ConnectionTrace(status);}
}
void SaveProfile(int id){std::string error;if(!progression::Save(fighters[id].name,fighters[id].profile,error,ProfileDirectory(id)))Tell(id,"Perfil: falha ao salvar. Verifique permissao/espaco em disco.");}
void ClaimProfile(int id){auto next=fighters[id].profile;int count=next.Claim();if(!count)return;std::string error;if(!progression::Save(fighters[id].name,next,error,ProfileDirectory(id))){Tell(id,"Falha ao entregar caixa; recompensa continua pendente.");return;}fighters[id].profile=next;Tell(id,std::to_string(count)+" caixa(s) recebida(s)! Abra no inventario do lobby.");}
void AwardXP(int id,int amount){auto& f=fighters[id];int before=f.profile.Level();f.profile.Award(amount);SaveProfile(id);if(f.profile.Level()>before)Tell(id,"Nivel "+std::to_string(f.profile.Level())+"! Sua caixa sera entregue ao voltar ao lobby.");}
void Kill(int id,int killer){auto& f=fighters[id];if(f.respawn>0||f.eliminated)return;if(killer==id)killer=-1;if(killer<0&&f.lastAttacker!=id&&clock-f.lastHurt<10)killer=f.lastAttacker;if(killer>=0&&killer<MaxPlayers){++fighters[killer].kills;AwardXP(killer,5);if(!f.bed)++fighters[killer].finals;for(Block item:{IronIngot,GoldIngot,Diamond,Emerald}){int remaining=Count(f,item);while(remaining>0){int count=std::min(64,remaining);if(!Give(fighters[killer],item,count))break;remaining-=count;}}}
    f.hp=0;f.inventory={};f.crafting={};f.cursor={};++f.revision;if(f.bed){f.respawn=5;if(f.pick>1)--f.pick;if(f.axe>1)--f.axe;}else f.eliminated=true;Teleport(id,LobbyPosition(id));}
bool Damage(int id,float damage,int attacker,Vector3 force={},bool melee=false){auto& f=fighters[id];if(!Live(id)||(attacker>=0&&attacker!=id&&TeamOf(attacker)==TeamOf(id))||(attacker>=0&&attacker!=id&&clock<f.meleeImmuneUntil)||(!melee&&f.hurtClock>0))return false;f.hurtClock=.5f;f.invisTime=0;f.lastAttacker=attacker;f.lastHurt=clock;f.healClock=0;float armor=f.armor==2?.68f:f.armor==1?.8f:f.armor==3?.86f:.92f;damage*=armor*(1.f-std::min(4,f.protection)*.04f);float absorbed=std::min(f.absorption,damage);f.absorption-=absorbed;f.hp-=damage-absorbed;if(attacker>=0&&attacker!=id)BroadcastHit(id,attacker);if(Vector3Length(force)>.001f)Knock(id,force,melee);if(f.hp<=0)Kill(id,attacker);return true;}
bool CanReach(int id,int x,int y,int z){return Live(id)&&Vector3Distance(Vector3Add(fighters[id].p,{0,fighters[id].sneaking?1.54f:1.62f,0}),{std::clamp(fighters[id].p.x,float(x),x+1.f),std::clamp(fighters[id].p.y+1.62f,float(y),y+1.f),std::clamp(fighters[id].p.z,float(z),z+1.f)})<=4.f;}
int BedTeam(int x,int y,int z){for(int t=0;t<TeamLimit();++t)for(auto p:MapBeds(mapId,t,bases[t]))if(x==int(p.x)&&y==int(p.y)&&z==int(p.z))return t;return -1;}
bool Breakable(int id,int x,int y,int z){if(!CanReach(id,x,y,z)||At(x,y,z)==Air||Liquid(At(x,y,z)))return false;int bed=BedTeam(x,y,z);return (bed>=0&&bed!=TeamOf(id)&&TeamBed(bed))||placed.count(Cell(x,y,z));}
void BeginRound(){
    if(phase!=0)return;
    BalanceTeams();participants=PlayerCount();phase=1;phaseClock=0;
    for(int team=0;team<TeamLimit();++team)if(!TeamCount(team)){RemoveBed(team);}
    for(int i=0;i<MaxPlayers;++i){auto& f=fighters[i];f.bed=f.connected;f.eliminated=!f.connected;if(!f.connected)continue;Loadout(i);auto base=bases[f.team];f.yaw=std::atan2(128.5f-base.x,base.z-128.5f);f.pitch=-.05f;Teleport(i,TeamSpawn(i));}
    Notice("Proteja sua cama e destrua as camas adversarias!");
}
void StartCountdown(){if(phase!=0||phaseClock>0)return;if(PlayerCount()<2){Notice("Aguardando pelo menos 2 jogadores para iniciar.");return;}phaseClock=5;Notice("Comecando jogo!");}
bool SolidPoint(Vector3 point){auto boxes=CollisionBoxesAt(int(std::floor(point.x)),int(std::floor(point.y)),int(std::floor(point.z)));for(int i=0;i<boxes.count;++i){auto b=boxes.boxes[i];if(point.x>=b.min.x&&point.x<b.max.x&&point.y>=b.min.y&&point.y<b.max.y&&point.z>=b.min.z&&point.z<b.max.z)return true;}return false;}
void Explosion(Vector3 p,int owner,Block kind){
    int cx=int(std::floor(p.x)),cy=int(std::floor(p.y)),cz=int(std::floor(p.z));
    for(int x=cx-4;x<=cx+4;++x)for(int y=cy-4;y<=cy+4;++y)for(int z=cz-4;z<=cz+4;++z){
        if(!Inside(x,y,z)||!placed.count(Cell(x,y,z)))continue;
        Block block=At(x,y,z);if(block==Obsidian||block==Glass)continue;
        int radius=block==EndStone?2:block==Planks||block==Wood?3:4;
        if(kind==Fireball){if(block==EndStone)continue;radius=3;}
        int distance=std::max({std::abs(x-cx),std::abs(y-cy),std::abs(z-cz)});
        if(distance<=radius){placed.erase(Cell(x,y,z));EditCell(x,y,z,Air);}
    }
    for(int i=0;i<MaxPlayers;++i){
        float distance=Vector3Distance(fighters[i].p,p);
        if(!Live(i)||distance>=5||(i!=owner&&TeamOf(i)==TeamOf(owner)))continue;
        Vector3 delta=Vector3Subtract(fighters[i].p,p),d=Vector3Normalize({delta.x,0,delta.z});
        if(kind==Fireball){
            if(Vector3Length(d)<.01f)d={-std::sin(fighters[i].yaw),0,std::cos(fighters[i].yaw)};
            float strength=std::clamp(1.f-distance/5.f,.25f,1.f);
            Knock(i,{d.x*32*strength,10+2*strength,d.z*32*strength},false,true);
            if(i!=owner)Damage(i,(5-distance)*2,owner);
        }else Damage(i,(5-distance)*3,owner,{d.x*6,5,d.z*6});
    }
}

void SendState();
void StartSwing(int id){auto& f=fighters[id];if(f.swingAge<.18f)return;f.swingAge=0;auto b=Packet(SwingAnimation);b.Byte(id);wire.Broadcast(b.bytes);}
void ArmPose(const Fighter& f,float side,float swing){
    rlTranslatef(0,f.sneaking?.82f:.67f,f.sneaking?.16f:0);if(f.sneaking)rlRotatef(-28,1,0,0);
    rlTranslatef(side,.65f,0);if(f.sneaking)rlRotatef(12,1,0,0);
    if(side>0){rlRotatef(85*swing,1,0,0);rlRotatef(-18*swing,0,0,1);}
}
void Process(int id,Buffer& b){int kind=b.Byte();auto& f=fighters[id];
    if(kind==CosmeticRequest){
        int action=b.Byte();auto next=f.profile;int reward=-1;
        if(action==0){
            if(phase!=0&&phase!=2){Tell(id,"Abra as caixas ao voltar ao lobby.");return;}
            if(!next.crates){Tell(id,"Nenhuma caixa disponivel. Suba de nivel para ganhar uma.");return;}
            static std::mt19937 rng(std::random_device{}());std::uniform_int_distribution<uint32_t> roll(0,299);reward=next.Open(roll(rng));
        }else if(action==1){int type=b.Byte();if(type>=3)return;next.NextSkin(type);}
        else if(action==2){int type=b.Byte(),color=b.Byte();if(!next.Equip(type,color)){Tell(id,"Esta skin ainda nao foi desbloqueada.");return;}}
        else return;
        std::string error;if(!progression::Save(f.name,next,error,ProfileDirectory(id))){Tell(id,"Nao foi possivel salvar o perfil; tente novamente.");return;}
        f.profile=next;SendState();if(reward>=0){if(id==0)cosmetics::Reward(reward);else {auto result=Packet(CosmeticResult);result.Byte(reward);wire.Send(f.peer,result.bytes);}}return;
    }
    if(kind==ChatMessage){std::string text=b.Text();text.erase(std::remove_if(text.begin(),text.end(),[](unsigned char c){return c<32||c>126;}),text.end());if(text.empty()||clock-f.lastChat<.75f)return;f.lastChat=clock;std::string line="<"+f.name+"> "+text.substr(0,60);AddChat(line);auto message=Packet(ChatMessage);message.Text(line);wire.Broadcast(message.bytes);return;}
    if(kind==ChooseTeam){int team=b.Byte();if(CanSelect(id,team)){f.team=team;Tell(id,std::string("Time: ")+teamNames[team]);SendState();}else Tell(id,"Time cheio ou escolha desequilibra a partida");return;}
    if(kind==StartRequest){if(id==0)StartCountdown();return;}if(kind==RestartRequest){if(id==0)ResetMatch();return;}
    if(kind==Position){Vector3 p=b.Vec();float yaw=b.Float(),pitch=b.Float();int selected=b.Byte();int crouch=b.Byte();if(crouch>1||selected>8||std::abs(p.x)>Width+16||std::abs(p.z)>Width+16||p.y>Height+20||p.y< -100)return;
        float elapsed=std::clamp(clock-f.lastPosition,.05f,.5f);float distance=std::hypot(p.x-f.p.x,p.z-f.p.z);
        if(distance<=(14+(clock<f.knockUntil?f.knockSpeed:0))*elapsed+1.5f&&!f.eliminated&&f.respawn<=0){if(phase!=0||Vector3Distance(p,LobbyPosition(id))<14){
            if(phase==1){if(p.y<f.p.y)f.fallDistance+=f.p.y-p.y;else if(p.y>f.p.y+.001f)f.fallDistance=0;bool grounded=false;int y=int(std::floor(p.y-.02f));for(int x=int(std::floor(p.x-.29f));x<=int(std::floor(p.x+.29f));++x)for(int z=int(std::floor(p.z-.29f));z<=int(std::floor(p.z+.29f));++z){auto boxes=CollisionBoxesAt(x,y,z);for(int i=0;i<boxes.count;++i)if(std::abs(p.y-boxes.boxes[i].max.y)<.04f)grounded=true;}if(grounded){if(f.fallDistance>3)Damage(id,std::floor(f.fallDistance-3),-1);f.fallDistance=0;}}
            if(f.respawn<=0&&!f.eliminated)f.p=p;
        }}f.yaw=yaw;f.pitch=std::clamp(pitch,-1.55f,1.55f);f.selected=selected;f.sneaking=crouch!=0;f.lastPosition=clock;return;}
    if(kind==SwapRequest){int a=b.Byte(),target=b.Byte();bool chest=b.Byte()!=0;bool exact=b.offset<b.bytes.size()&&b.Byte()!=0;if(a>=63||target>=63||(!chest&&(a>=40||target>=40)))return;if(chest&&(phase!=1||!ChestReach(f.p,TeamOf(id))))return;
        auto get=[&](int slot)->ItemStack&{return slot<36?f.inventory[slot]:(chest?chests[TeamOf(id)][slot-36]:f.crafting[slot-36]);};auto& from=get(a);auto& to=get(target);if(chest&&((a>=36||target>=36)&&(ToolLevel(from.block)||ToolLevel(to.block))))return;if(a!=target){if(!exact&&from.block==to.block&&from.damage==to.damage&&from.count&&to.count){int n=std::min(int(from.count),StackCapacity(to.block)-int(to.count));to.count+=n;from.count-=n;if(!from.count)from={};}else std::swap(from,to);++f.revision;SendState();}return;}
    if(kind==InventoryClickRequest){int slot=b.Byte();bool right=b.Byte()!=0,chest=b.Byte()!=0;bool quick=b.offset<b.bytes.size()&&b.Byte()!=0;if(slot>=(chest?63:40)||(chest&&(phase!=1||!ChestReach(f.p,TeamOf(id)))))return;
        auto& stack=slot<36?f.inventory[slot]:chest?chests[TeamOf(id)][slot-36]:f.crafting[slot-36];if(chest&&slot>=36&&ToolLevel(f.cursor.block))return;
        if(quick){
            if(!stack.count)return;
            int moved=0;
            if(chest){
                if(slot<36){if(ToolLevel(stack.block))return;moved=TransferStack(stack,chests[TeamOf(id)].data(),27);}
                else moved=TransferStack(stack,f.inventory.data(),36);
            }else if(slot<9)moved=TransferStack(stack,f.inventory.data()+9,27);
            else if(slot<36)moved=TransferStack(stack,f.inventory.data(),9);
            else moved=TransferStack(stack,f.inventory.data(),36);
            if(moved){++f.revision;SendState();}return;
        }
        auto saved=CursorStack;CursorStack=f.cursor;ClickStack(stack,right);f.cursor=CursorStack;CursorStack=saved;++f.revision;SendState();return;}
    if(kind==CraftRequest){int target=b.Byte();auto result=CraftResult(f.crafting.data(),2);if(!result.count)return;auto inventory=f.inventory;
        if(target==254){if(f.cursor.count&&(f.cursor.block!=result.block||f.cursor.damage!=result.damage))return;if(f.cursor.count+result.count>StackCapacity(result.block))return;f.cursor.block=result.block;f.cursor.damage=result.damage;f.cursor.count+=result.count;}
        else if(target<9){auto& slot=inventory[target];if(slot.count&&(slot.block!=result.block||slot.damage!=result.damage))return;if(slot.count+result.count>StackCapacity(result.block))return;slot.block=result.block;slot.damage=result.damage;slot.count+=result.count;}
        else if(target==255){TransferStack(result,inventory.data(),inventory.size());if(result.count)return;}else return;
        f.inventory=inventory;for(auto& stack:f.crafting)if(stack.count&&!--stack.count)stack={};++f.revision;SendState();return;}
    if(!Live(id))return;
    if(kind==BowRequest){
        int slot=b.Byte(),action=b.Byte();if(slot>8||action>3)return;
        if(action==2){f.bowStart=-1;f.bowSlot=-1;return;}
        if(f.inventory[slot].block!=Bow||!f.inventory[slot].count||!Count(f,Arrow)){f.bowStart=-1;f.bowSlot=-1;return;}
        if(action==0){f.bowStart=clock;f.bowSlot=slot;f.bowHeartbeat=clock;f.selected=slot;return;}
        if(f.bowSlot!=slot||f.bowStart<0)return;
        if(action==3){f.bowHeartbeat=clock;return;}
        float held=std::round(std::max(0.f,clock-f.bowStart)*1000.f)/1000.f;f.bowStart=-1;f.bowSlot=-1;
        if(clock-f.lastBowShot<BowShotDelay)return;
        if(shots.size()>=192){auto old=std::find_if(shots.begin(),shots.end(),[](const Shot& s){return s.embedded;});if(old==shots.end())return;shots.erase(old);}
        Vector3 eye=Vector3Add(f.p,{0,1.52f,0}),direction{std::sin(f.yaw)*std::cos(f.pitch),std::sin(f.pitch),-std::cos(f.yaw)*std::cos(f.pitch)};
        static std::mt19937 rng(std::random_device{}());float damage=BowDamage(held,rng());
        Consume(f,Arrow,1);Shot arrow{Vector3Add(eye,Vector3Scale(direction,.7f)),Vector3Scale(direction,ArrowSpeed*BowPower(held)),id,Arrow,10};arrow.damage=damage;arrow.critical=damage>4;shots.push_back(arrow);f.lastBowShot=clock;return;
    }
    if(kind==HoldUseRequest){int slot=b.Byte(),held=b.Byte();if(slot>8||held>1)return;if(held&&f.inventory[slot].block==GoldenApple&&f.inventory[slot].count){if(f.eatSlot!=slot){f.eating=0;f.eatSlot=slot;}f.eatHeartbeat=clock;}else{f.eating=0;f.eatSlot=-1;}return;}
    if(kind==BuyRequest){int index=b.Byte();if(index>=int(std::size(offers))||offers[index].special==7)return;const auto& offer=offers[index];bool upgrade=offer.special>=3&&offer.special<=5;if(!ShopReach(f.p,upgrade))return;
        int level=ToolLevel(offer.item);int current=IsAxe(offer.item)?f.axe:f.pick;if(level&&level!=current+1){Tell(id,"Compre a proxima melhoria da ferramenta");return;}
        if(Count(f,offer.currency)<offer.cost){Tell(id,"Recursos insuficientes");return;}
        if(offer.special){int armor=offer.special==1?1:offer.special==2?2:3;
            if((!upgrade&&ArmorRank(f.armor)>=ArmorRank(armor))||(offer.special==3&&f.sharp)||(offer.special==4&&f.protection>=4)||(offer.special==5&&f.forge>=2)){Tell(id,"Melhoria ja comprada ou no nivel maximo");return;}
            if(!upgrade)f.armor=armor;
            if(offer.special==3)f.sharp=1;
            if(offer.special==4)++f.protection;
            if(offer.special==5)++f.forge;
        }else {Block item=offer.item==WhiteWool?wool[TeamOf(id)]:offer.item;auto previous=f.inventory;
            if(level)for(auto& stack:f.inventory)if(ToolLevel(stack.block)&&IsAxe(stack.block)==IsAxe(item))stack={};
            bool received=false;
            if(Sword(item)){
                int target=Sword(f.inventory[f.selected].block)?f.selected:-1;
                if(target<0)for(int slot=0;slot<36;++slot)if(Sword(f.inventory[slot].block)){target=slot;break;}
                if(target>=0){f.inventory[target]={item,1};++f.revision;received=true;}
            }
            if(!received&&!Give(f,item,offer.count,item==Stick?1:0)){f.inventory=previous;Tell(id,"Inventario cheio");return;}
            if(level){if(IsAxe(item))f.axe=level;else f.pick=level;}
        }
        if(upgrade)for(auto& mate:fighters)if(mate.connected&&mate.team==f.team){mate.sharp=f.sharp;mate.protection=f.protection;mate.forge=f.forge;}
        Consume(f,offer.currency,offer.cost);Tell(id,"Compra realizada");return;
    }
    if(kind==DigRequest){int stage=b.Byte(),x=b.Int(),y=b.Int(),z=b.Int(),slot=b.Byte();if(slot>8)return;if(stage==1){f.digging={};return;}if(!Inside(x,y,z)||!Breakable(id,x,y,z))return;
        if(stage==0){f.digging={true,x,y,z};f.digStart=clock;return;}
        if(stage!=2||!f.digging.found||x!=f.digging.x||y!=f.digging.y||z!=f.digging.z||clock-f.digStart+.08f<MiningTime(At(x,y,z),f.inventory[slot].block))return;
        int bed=BedTeam(x,y,z);if(bed>=0){RemoveBed(bed,true);++f.beds;AwardXP(id,10);status=std::string("Cama destruida: ")+teamNames[bed];}
        else {Block block=At(x,y,z);placed.erase(Cell(x,y,z));EditCell(x,y,z,Air);Give(f,block,1);}f.digging={};return;}
    if(kind==PlaceRequest){int x=b.Int(),y=b.Int(),z=b.Int(),slot=b.Byte();if(slot>8||!Inside(x,y,z)||y<(mapId>=3?1:20)||y>(mapId>=3?MapLobbyY(mapId)-5:55)||!CanReach(id,x,y,z)||At(x,y,z)!=Air)return;auto& stack=f.inventory[slot];if(!stack.count||!Placeable(stack.block)||Liquid(stack.block)||stack.block==Bed||stack.block==BedHead||stack.block==Tnt)return;
        for(const auto& generator:generators)if(Vector3Distance({x+.5f,y+.5f,z+.5f},generator.p)<2)return;
        for(int i=0;i<MaxPlayers;++i){auto p=fighters[i].p;if(Fighting(fighters[i])&&p.x+.3f>x&&p.x-.3f<x+1&&p.y+1.8f>y+.001f&&p.y<y+1-.001f&&p.z+.3f>z&&p.z-.3f<z+1)return;}
        Block block=stack.block;EditCell(x,y,z,block);placed.insert(Cell(x,y,z));if(!--stack.count)stack={};++f.revision;return;}
    if(kind==AttackRequest){int slot=b.Byte();if(slot>8)return;StartSwing(id);Vector3 eye=Vector3Add(f.p,{0,f.sneaking?1.54f:1.62f,0}),direction{std::sin(f.yaw)*std::cos(f.pitch),std::sin(f.pitch),-std::cos(f.yaw)*std::cos(f.pitch)};float closest=std::numeric_limits<float>::infinity();int victim=-1;auto block=Trace(eye,direction);if(block.found){auto h=GetRayCollisionBox({eye,direction},{{float(block.x),float(block.y),float(block.z)},{block.x+1.f,block.y+1.f,block.z+1.f}});if(h.hit)closest=std::min(closest,h.distance);}
        for(int i=0;i<MaxPlayers;++i)if(i!=id&&TeamOf(i)!=TeamOf(id)&&Live(i)){auto p=fighters[i].p;auto hit=GetRayCollisionBox({eye,direction},{{p.x-.3f,p.y,p.z-.3f},{p.x+.3f,p.y+1.8f,p.z+.3f}});if(hit.hit&&hit.distance<=3.3f&&hit.distance<closest){victim=i;closest=hit.distance;}}
        if(victim>=0){Block sword=f.inventory[slot].block;float damage=sword==DiamondSword?5:sword==IronSword?4:sword==StoneSword?3:sword==WoodenSword||sword==GoldenSword?2:1;Vector3 delta=Vector3Subtract(fighters[victim].p,f.p);Vector3 d=Vector3Normalize({delta.x,0,delta.z});float horizontal=sword==Stick&&f.inventory[slot].damage==1?22.f:8.5f;if(Damage(victim,damage+f.sharp,id,{d.x*horizontal,6.f,d.z*horizontal},true)){if(id==0)localSprintPause=true;else wire.Send(f.peer,Packet(SprintPause).bytes);}SendState();}return;}
    if(kind==UseRequest){int slot=b.Byte();if(slot>8)return;auto& stack=f.inventory[slot];if(clock-f.lastAction<(stack.block==Bow?.15f:.3f))return;if(!stack.count)return;Block item=stack.block;if((item==Bow||item==Fireball||item==EnderPearl||item==Tnt)&&shots.size()>=192){auto old=std::find_if(shots.begin(),shots.end(),[](const Shot& shot){return shot.embedded;});if(old!=shots.end())shots.erase(old);else return;}Vector3 eye=Vector3Add(f.p,{0,1.4f,0}),direction{std::sin(f.yaw)*std::cos(f.pitch),std::sin(f.pitch),-std::cos(f.yaw)*std::cos(f.pitch)};
        if(item>=SpeedPotion&&item<=InvisibilityPotion){Consume(f,item,1);if(item==SpeedPotion)f.speedTime=45;if(item==JumpPotion)f.jumpTime=45;if(item==InvisibilityPotion)f.invisTime=30;}
        else if(item==GoldenApple){return;}
        else if(item==Fireball||item==EnderPearl||item==Tnt){Consume(f,item,1);shots.push_back({Vector3Add(eye,Vector3Scale(direction,.7f)),Vector3Scale(direction,item==EnderPearl?32.f:item==Fireball?FireballSpeed:2.f),id,item,item==Tnt?4.f:8.f});}
        else if(item==Bow)return;
        f.lastAction=clock;
    }
}
void SendState(){auto b=Packet(Snapshot);b.Int(epoch);b.Byte(phase);b.Float(phaseClock);b.Int(winner);b.Float(clock);b.Byte(islandGap);b.Byte(mapId);b.Byte(teamCapacity);
    for(auto& f:fighters){b.Byte(f.connected);b.Byte(f.team);b.Text(f.name);b.Int(f.profile.xp);b.Int(f.profile.crates);b.Int(f.profile.pending);for(int i=0;i<3;++i){b.Int(int(f.profile.unlocked[i]));b.Byte(f.profile.skins[i]);}b.Vec(f.p);b.Float(f.yaw);b.Float(f.pitch);b.Float(f.hp);b.Float(f.respawn);b.Byte(f.bed);b.Byte(f.eliminated);b.Int(f.teleport);b.Int(f.revision);b.Int(f.kills);b.Int(f.finals);b.Int(f.beds);b.Byte(f.armor);b.Byte(f.sharp);b.Byte(f.protection);b.Byte(f.forge);b.Byte(f.selected);b.Byte(f.sneaking);b.Byte(f.pick);b.Byte(f.axe);b.Float(f.speedTime);b.Float(f.jumpTime);b.Float(f.invisTime);b.Float(f.absorption);b.Float(f.regen);b.Float(f.eating);b.Float(f.bowStart);for(auto& s:f.inventory)Stack(b,s);for(auto& s:f.crafting)Stack(b,s);Stack(b,f.cursor);}
    for(auto& chest:chests)for(auto& s:chest)Stack(b,s);
    b.Byte(int(generators.size()));for(auto& g:generators){b.Int(g.stock);b.Float(g.clock);}
    b.Byte(int(shots.size()));for(auto& s:shots){b.Vec(s.p);b.Byte(s.kind);b.Vec(s.v);b.Byte(s.embedded);b.Byte(s.critical);}
    wire.Broadcast(b.bytes);SendPenguins();
}
void Simulate(float dt){clock+=dt;
    if(phase==0){if(phaseClock>0&&PlayerCount()<2){phaseClock=0;Notice("Contagem cancelada: aguardando 2 jogadores.");}if(phaseClock>0){phaseClock=std::max(.0001f,phaseClock-dt);if(phaseClock<.001f)BeginRound();}if(phase!=0)return;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected&&(fighters[i].p.y<(MapLobbyY(mapId)-1)||Vector3Distance(fighters[i].p,LobbyPosition(i))>14))Teleport(i,LobbyPosition(i));return;}
    phaseClock+=dt;if(phase==2){if(phaseClock>10)ResetMatch();return;}
    for(int i=0;i<MaxPlayers;++i){auto& f=fighters[i];if(f.bowStart>=0&&(f.bowSlot!=f.selected||f.bowSlot<0||f.inventory[f.bowSlot].block!=Bow||clock-f.bowHeartbeat>.4f||!Live(i))){f.bowStart=-1;f.bowSlot=-1;}f.hurtClock=std::max(0.f,f.hurtClock-dt);f.speedTime=std::max(0.f,f.speedTime-dt);f.jumpTime=std::max(0.f,f.jumpTime-dt);f.invisTime=std::max(0.f,f.invisTime-dt);f.absorbTime=std::max(0.f,f.absorbTime-dt);if(f.absorbTime==0)f.absorption=0;if(Live(i)){
            if(f.eatSlot>=0){auto& food=f.inventory[f.eatSlot];if(clock-f.eatHeartbeat>.3f||f.selected!=f.eatSlot||food.block!=GoldenApple||!food.count){f.eating=0;f.eatSlot=-1;}else{f.eating+=dt;if(f.eating+1e-5f>=1.61f){Consume(f,GoldenApple,1);f.absorption=4;f.absorbTime=120;f.regen=5;f.regenClock=f.healClock=f.eating=0;f.eatSlot=-1;}}}
            if(f.regen>0){float elapsed=std::min(dt,f.regen);f.regen=std::max(0.f,f.regen-dt);f.regenClock+=elapsed;while(f.regenClock>=1.f-1e-5f){f.regenClock=std::max(0.f,f.regenClock-1);f.hp=std::min(20.f,f.hp+1);}f.healClock=0;}
            else if(f.hp<20){f.healClock+=dt;while(f.healClock>=4){f.healClock-=4;f.hp=std::min(20.f,f.hp+2);}}else f.healClock=0;
        }if(f.respawn>0){f.respawn=std::max(0.f,f.respawn-dt);if(f.respawn==0){Loadout(i);Teleport(i,TeamSpawn(i));}}else if(Live(i)&&f.p.y< -6)Kill(i,-1);}
    for(size_t i=0;i<generators.size();++i){auto& g=generators[i];float interval=(g.item==Diamond||g.item==Emerald)?ResourceInterval(g.item,phaseClock):g.interval;if(g.team>=0)interval/=1+TeamForge(g.team);g.clock+=dt;while(g.clock>=interval){g.clock-=interval;g.stock=std::min(g.cap,g.stock+1);}for(int id=0;id<MaxPlayers;++id)if(g.stock&&Live(id)&&Vector3Distance(fighters[id].p,g.p)<2){if(Give(fighters[id],g.item,1))--g.stock;}}
    for(auto it=shots.begin();it!=shots.end();){
        auto& s=*it;s.life-=dt;
        if(s.embedded){if(s.life<=0||!ArrowSolidPoint(s.support))it=shots.erase(it);else ++it;continue;}
        bool collision=false;int victim=-1;
        int ticks=1;float stepTime=dt;
        if(s.kind==Arrow){s.tick+=dt;ticks=int((s.tick+1e-6f)/ProjectileTick);s.tick=std::max(0.f,s.tick-ticks*ProjectileTick);stepTime=ProjectileTick;}
        else if(s.kind!=Fireball)s.v.y-=dt*14;
        for(int tick=0;tick<ticks&&!collision&&victim<0;++tick){
            int steps=std::max(1,int(std::ceil(Vector3Length(s.v)*stepTime/.2f)));
            for(int step=0;step<steps;++step){
                Vector3 next=Vector3Add(s.p,Vector3Scale(s.v,stepTime/steps));
                collision=SolidPoint(next);
                if(s.kind==Arrow&&collision){s.support=next;s.p=EmbeddedArrowTip(s.p,next);s.embedded=true;s.life=60;break;}
                if(s.kind==Tnt&&collision){s.v={};collision=false;break;}
                if(s.kind!=Tnt)for(int i=0;i<MaxPlayers;++i)if(i!=s.owner&&TeamOf(i)!=TeamOf(s.owner)&&Live(i)&&Vector3Distance(Vector3Add(fighters[i].p,{0,.9f,0}),next)<.8f)victim=i;
                if(collision||victim>=0)break;
                s.p=next;
            }
            if(s.kind==Arrow&&!s.embedded)s.v=ArrowTickVelocity(s.v);
        }
        if(s.embedded){++it;continue;}
        bool expired=s.life<=.00001f;
        if((s.kind==Tnt?expired:collision||victim>=0||expired)||s.p.y< -10){
            if(s.kind==Tnt){if(expired)Explosion(s.p,s.owner,s.kind);}
            else if(s.kind==Fireball)Explosion(s.p,s.owner,s.kind);
            else if(s.kind==Arrow&&victim>=0)Damage(victim,s.damage,s.owner,Vector3Scale(Vector3Normalize(s.v),3));
            else if(s.kind==EnderPearl&&collision&&Live(s.owner)){Vector3 destination=s.p;destination.y=std::ceil(destination.y);if(!Solid(At(int(destination.x),int(destination.y),int(destination.z)))&&!Solid(At(int(destination.x),int(destination.y+1),int(destination.z)))){Teleport(s.owner,destination);Damage(s.owner,4,-1);}}
            it=shots.erase(it);
        }else ++it;
    }

    if(phaseClock>=600)for(int team=0;team<TeamLimit();++team)if(TeamBed(team)){SetTeamBed(team,false);RemoveBed(team);}
    if(phaseClock>=900)for(int i=0;i<MaxPlayers;++i)Damage(i,dt*2,-1);
    std::set<int> alive;for(const auto& f:fighters)if(f.connected&&!f.eliminated)alive.insert(f.team);
    if(participants>=2&&alive.size()<=1){winner=alive.empty()?-1:*alive.begin();phase=2;phaseClock=0;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected){if(winner>=0&&fighters[i].team==winner){AwardXP(i,25);Tell(i,"Vitoria! +25 XP");}Teleport(i,LobbyPosition(i));ClaimProfile(i);}}
}
void Receive(int peer,Buffer& b){if(host){int kind=b.bytes.empty()?0:b.bytes[0];if(kind==Hello){b.Byte();int version=b.Int();if(version!=Version){auto e=Packet(Error);e.Text("Versoes diferentes. Atualize o host e o convidado.");wire.Send(peer,e.bytes);ConnectionTrace("rejected protocol "+std::to_string(version));wire.Drop(peer);return;}std::string name=b.Text();if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos||phase!=0){auto e=Packet(Error);e.Text("Aguarde o host voltar ao lobby; use nick de 3 a 16 letras/numeros.");wire.Send(peer,e.bytes);wire.Drop(peer);return;}if(peerTeam.count(peer))return;
        progression::Profile profile;std::string profileError;
        bool duplicate=std::any_of(fighters.begin(),fighters.end(),[&](const Fighter& f){return f.connected&&progression::Key(f.name)==progression::Key(name);});
        if(duplicate||!LoadGuestProfile(name,profile,profileError)){auto e=Packet(Error);e.Text(duplicate?"Nick ja conectado nesta sala":"Perfil corrompido ou inacessivel no host");wire.Send(peer,e.bytes);wire.Drop(peer);return;}if(b.offset<b.bytes.size())profile=progression::Reconcile(ReadProfile(b),profile);int id=-1;for(int i=1;i<MaxPlayers;++i)if(!fighters[i].connected){id=i;break;}if(id<0||PlayerCount()>=TeamLimit()*teamCapacity){wire.Drop(peer);return;}peerTeam[peer]=id;auto& f=fighters[id];f=Fighter{};f.profile=profile;f.name=name;f.peer=peer;f.team=AvailableTeam();f.connected=true;ClaimProfile(id);Give(f,WhiteWool,1);Teleport(id,LobbyPosition(id));auto w=Packet(Welcome);w.Int(Version);w.Byte(id);wire.Send(peer,w.bytes);SendState();}else if(peerTeam.count(peer))Process(peerTeam[peer],b);return;}
    int kind=b.Byte();if(kind==Welcome){ConnectionTrace("welcome received; awaiting world snapshot");if(b.Int()!=Version)throw std::runtime_error("Versao BedWars diferente do host");me=b.Byte();if(me>=MaxPlayers)throw std::runtime_error("Equipe invalida");status="Conectado ao BedWars LAN";return;}
    if(kind==Snapshot){int nextEpoch=b.Int();epoch=nextEpoch;phase=b.Byte();phaseClock=b.Float();winner=b.Int();clock=b.Float();int gap=b.Byte();int nextMap=b.Byte();teamCapacity=b.Byte();if(teamCapacity!=1&&teamCapacity!=2)throw std::runtime_error("Modo invalido");if(nextMap>=MapCount)throw std::runtime_error("Mapa invalido");if(gap!=16&&gap!=32&&gap!=64)throw std::runtime_error("Distancia Rush invalida");if(nextEpoch!=seenEpoch){ClearPenguins();mapId=nextMap;ConnectionTrace("generating map "+std::to_string(mapId)+" epoch "+std::to_string(nextEpoch));SetLayout(gap);GenerateArena();ConnectionTrace("map block data ready");seenEpoch=nextEpoch;seenTeleport=seenRevision=-1;}for(auto& f:fighters){f.connected=b.Byte()!=0;f.team=b.Byte();if(f.team>=TeamLimit())throw std::runtime_error("Time invalido");f.name=b.Text();f.profile.xp=b.Int();f.profile.crates=b.Int();f.profile.pending=b.Int();for(int i=0;i<3;++i){f.profile.unlocked[i]=uint32_t(b.Int());f.profile.skins[i]=b.Byte();}if(!progression::Valid(f.profile))throw std::runtime_error("Perfil invalido no pacote");f.p=b.Vec();f.yaw=b.Float();f.pitch=b.Float();f.hp=b.Float();f.respawn=b.Float();f.bed=b.Byte()!=0;f.eliminated=b.Byte()!=0;f.teleport=b.Int();f.revision=b.Int();f.kills=b.Int();f.finals=b.Int();f.beds=b.Int();f.armor=b.Byte();f.sharp=b.Byte();f.protection=b.Byte();f.forge=b.Byte();f.selected=b.Byte();if(f.selected>8)throw std::runtime_error("Slot BedWars invalido");f.sneaking=b.Byte()!=0;f.pick=b.Byte();f.axe=b.Byte();f.speedTime=b.Float();f.jumpTime=b.Float();f.invisTime=b.Float();f.absorption=b.Float();f.regen=b.Float();f.eating=b.Float();f.bowStart=b.Float();for(auto& s:f.inventory)s=Stack(b);for(auto& s:f.crafting)s=Stack(b);f.cursor=Stack(b);}for(auto& chest:chests)for(auto& s:chest)s=Stack(b);int count=b.Byte();if(count!=int(generators.size()))throw std::runtime_error("Geradores inconsistentes");for(auto& g:generators){g.stock=b.Int();g.clock=b.Float();}int n=b.Byte();shots.clear();for(int i=0;i<n;++i){auto p=b.Vec();int block=b.Byte();if(block>=BlockCount)throw std::runtime_error("Projetil invalido");auto velocity=b.Vec();bool embedded=b.Byte()!=0;bool critical=b.Byte()!=0;Shot shot{p,velocity,0,Block(block),0,0,embedded};shot.critical=critical;shots.push_back(shot);}if(!ready)ConnectionTrace("first world snapshot ready");ready=true;PersistLocalProfile();return;}
    if(kind==PenguinUpdate){ReadPenguins(b);return;}
    if(kind==HitEffect){int victim=b.Byte(),attacker=b.Byte();if(victim>=MaxPlayers||attacker>=MaxPlayers)throw std::runtime_error("Hit invalido");ShowHit(victim,attacker);return;}
    if(kind==CosmeticResult){int reward=b.Byte();if(reward<1||reward>36)throw std::runtime_error("Skin invalida");cosmetics::Reward(reward);return;}
    if(kind==Edit){int x=b.Int(),y=b.Int(),z=b.Int(),block=b.Byte();if(!Inside(x,y,z)||block>=BlockCount)throw std::runtime_error("Edicao invalida");Change(x,y,z,Block(block));if(block==Air)placed.erase(Cell(x,y,z));else placed.insert(Cell(x,y,z));return;}
    if(kind==SwingAnimation){int id=b.Byte();if(id>=MaxPlayers||!fighters[id].connected)return;fighters[id].swingAge=0;return;}
    if(kind==SprintPause){localSprintPause=true;return;}
    if(kind==ChatMessage){AddChat(b.Text());return;}
    if(kind==Force){auto force=b.Vec();int mode=b.offset<b.bytes.size()?b.Byte():0;if(mode==2)localExplosionForce=Vector3Add(localExplosionForce,force);else if(mode==1)localMeleeForce=force;else localForce=Vector3Add(localForce,force);return;}if(kind==Error){cosmetics::CancelWaiting();status=b.Text();if(!ready){connectionFailure=status;ConnectionTrace("host rejected connection: "+status);}return;}
}
void Request(Buffer b){if(!active||!ready)return;if(host)Process(0,b);else wire.Send(0,b.bytes);}

Rectangle InventoryRect(int i,bool chest){if(!chest)return i<36?ClassicPlayerSlot(i<27?i+9:i-27):ClassicCraftSlot(i-36);float size=std::min(42.f,(GetScreenHeight()-120.f)/(chest?8:5));float x=GetScreenWidth()/2.f-size*4.5f,y=GetScreenHeight()/2.f-size*(chest?3.7f:2.f);int column,row;if(i<27){column=i%9;row=i/9;}else if(i<36){column=i-27;row=3;}else{column=(i-36)%9;row=(i-36)/9;}if(i<36&&chest)row+=4;return {x+column*size,y+row*size,size-3,size-3};}
int StorageIndex(int displayed){return displayed<27?displayed+9:displayed-27;}
}
void ConnectionTrace(const std::string& message){if(connectionLog.empty())return;try{std::ofstream out(connectionLog,std::ios::app);out<<int64_t((ConnectionNow()-joinStarted)*1000)<<" ms: "<<message<<'\n';}catch(...){}}
int ResourceLevel(Block item,float elapsed){if(item!=Diamond&&item!=Emerald)return 1;int maxLevel=item==Diamond?5:7;return 1+int(std::clamp(std::max(0.f,elapsed)/170.f,0.f,float(maxLevel-1)));}
float ResourceInterval(Block item,float elapsed){if(item!=Diamond&&item!=Emerald)return item==IronIngot?2.5f:8.f;float base=item==Diamond?25.f:item==Emerald?35.f:item==IronIngot?2.5f:8.f;return std::max(5.f,base-5*(ResourceLevel(item,elapsed)-1));}
std::vector<GeneratorInfo> Generators(){std::vector<GeneratorInfo> result;for(size_t i=0;i<generators.size();++i){const auto& g=generators[i];float interval=(g.item==Diamond||g.item==Emerald)?ResourceInterval(g.item,phaseClock):g.interval;if(g.team>=0)interval/=1+TeamForge(g.team);result.push_back({g.p,g.item,g.stock,ResourceLevel(g.item,phaseClock),interval,std::max(0.f,interval-g.clock)});}return result;}
int TeamCapacity(){return teamCapacity;}
int PlayerTeam(int id){return id>=0&&id<MaxPlayers&&fighters[id].connected?TeamOf(id):-1;}
int LocalTeam(){return TeamOf(me);}
std::vector<TeamInfo> TeamChoices(){std::vector<TeamInfo> result;for(int t=0;t<TeamLimit();++t)result.push_back({t,TeamCount(t),teamCapacity,teamNames[t],colors[t],CanSelect(me,t)});return result;}
void SelectTeam(int team){if(team<0||team>=TeamLimit())return;auto b=Packet(ChooseTeam);b.Byte(team);Request(std::move(b));}
int IslandGap(){return islandGap;}
int SelectedMap(){return mapId;}
void ApplyEffects(PlayerInput& input){if(!Ready())return;input.speedMultiplier=fighters[me].speedTime>0?1.4f:1.f;input.jumpMultiplier=fighters[me].jumpTime>0?1.45f:1.f;}
Vector3 BasePosition(int team){return bases[std::clamp(team,0,7)];}
bool Host(const std::string& name,unsigned short port,int gap,int map,int capacity,const std::string& serverName){Stop();StartConnectionTrace("host port "+std::to_string(port)+" map "+std::to_string(map));
    serverTitle=serverName.empty()?name:serverName;
    if(serverTitle.size()>32||serverTitle.find_first_not_of(" ")==std::string::npos||std::any_of(serverTitle.begin(),serverTitle.end(),[](unsigned char c){return c<32||c>126;})){status="Nome da sala: use de 1 a 32 caracteres";return false;}
if(capacity!=1&&capacity!=2){status="Modo invalido";return false;}teamCapacity=capacity;if(map<0||map>=MapCount){status="Mapa invalido";return false;}mapId=map;if(gap!=16&&gap!=32&&gap!=64){status="Distancia Rush invalida";return false;}SetLayout(gap);if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos){status="Nick: use 3 a 16 letras, numeros ou _";return false;}progression::Profile profile;std::string profileError;if(!progression::Load(name,profile,profileError)){status=profileError;return false;}if(!wire.Host(port)){status=wire.Error();return false;}roomPort=port;host=active=ready=true;me=0;fighters={};fighters[0].connected=true;fighters[0].profile=profile;fighters[0].name=name.empty()?"Host":name;fighters[0].peer=0;seenEpoch=seenRevision=seenTeleport=-1;ResetMatch();UpdateDiscovery();return true;}
bool Join(const std::string& address,const std::string& name){Stop();StartConnectionTrace("join "+address);if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos){status="Nick: use 3 a 16 letras, numeros ou _";return false;}std::string profileError;if(!progression::Load(name,joiningProfile,profileError)){status=profileError;return false;}localProfileSaved=false;if(!wire.Join(address)){status=wire.Error();return false;}active=true;host=ready=false;me=0;fighters={};fighters[0].name=name;status="Conectando ao BedWars LAN...";seenEpoch=seenRevision=seenTeleport=-1;return true;}
void Stop(){ClearPenguins();if(screamSound.frameCount)UnloadSound(screamSound);screamSound={};screamChecked=false;if(hitSound.frameCount)UnloadSound(hitSound);hitSound={};hitSoundChecked=false;CancelBow();if(active&&!host)PersistLocalProfile();if(active)ConnectionTrace("session stopped: "+status);cosmetics::Reset();if(host&&active)for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected)SaveProfile(i);discovery.StopHosting();discovery.CancelSearch();wire.Stop();active=ready=host=false;shots.clear();peerTeam.clear();chatMessages.clear();localForce=localMeleeForce=localExplosionForce={};localSprintPause=false;}
void UpdateDiscovery(){if(host&&active&&ready){LanRoom room;room.name=serverTitle;room.port=roomPort;room.players=PlayerCount();room.gap=islandGap;room.joinable=phase==0;room.capacity=TeamLimit()*teamCapacity;room.map=MapName(mapId);for(const auto& f:fighters)if(f.connected)room.playerNames.push_back(f.name);discovery.Advertise(room);}discovery.Poll();}
void SearchRooms(){discovery.Search();}
bool SearchingRooms(){return discovery.Searching();}
const std::vector<LanRoom>& Rooms(){return discovery.Rooms();}
const std::string& DiscoveryStatus(){return discovery.Status();}
bool Active(){return active;}bool Ready(){return active&&ready;}bool Hosting(){return host;}bool Lobby(){return phase==0;}bool CanControl(){return Ready()&&fighters[me].respawn<=0;}
const std::vector<std::string>& Messages(){return chatMessages;}std::string Nickname(){return fighters[me].name;}
progression::Profile Progress(int player){return fighters[player<0?me:std::clamp(player,0,MaxPlayers-1)].profile;}
int ItemSkin(Block item,int player){int type=cosmetics::Category(item);return Ready()&&type>=0?Progress(player).skins[type]:0;}
void OpenCrate(){if(!Ready())return;if(phase!=0&&phase!=2){Notice("Abra as caixas ao voltar ao lobby.");return;}if(!Progress().crates){Notice("Nenhuma caixa disponivel.");return;}cosmetics::AwaitReward();auto b=Packet(CosmeticRequest);b.Byte(0);Request(std::move(b));}
void NextSkin(int category){if(Ready()&&category>=0&&category<3){auto b=Packet(CosmeticRequest);b.Byte(1);b.Byte(category);Request(std::move(b));}}
void EquipSkin(int category,int color){if(Ready()&&category>=0&&category<3&&color>=0&&color<=progression::SkinCount){auto b=Packet(CosmeticRequest);b.Byte(2);b.Byte(category);b.Byte(color);Request(std::move(b));}}
std::vector<PlayerEntry> PlayerList(){std::vector<PlayerEntry> result;for(const auto& f:fighters)if(f.connected)result.push_back({f.name,f.team,f.eliminated});return result;}
void Chat(const std::string& text){auto b=Packet(ChatMessage);b.Text(text.substr(0,60));Request(std::move(b));}const std::string& Status(){return status;}
void StartRound(){Request(Packet(StartRequest));}void Restart(){Request(Packet(RestartRequest));}
void Update(Player& player,float dt,bool playing,int selected){if(!active)return;dt=std::clamp(dt,0.f,.1f);LoadHitSound();for(auto& f:fighters)f.hitFlash=std::max(0.f,f.hitFlash-dt);for(auto& event:wire.Poll()){try{if(event.kind==LanTransport::Event::Connected&&!host){ConnectionTrace("TCP connected; sending hello");auto b=Packet(Hello);b.Int(Version);b.Text(fighters[0].name);ProfilePacket(b,joiningProfile);wire.Send(0,b.bytes);}else if(event.kind==LanTransport::Event::Data){Buffer b{event.data};Receive(event.peer,b);}else if(event.kind==LanTransport::Event::Disconnected){if(host){auto it=peerTeam.find(event.peer);if(it!=peerTeam.end()){auto& f=fighters[it->second];SaveProfile(it->second);f.connected=false;f.eliminated=true;f.bed=false;peerTeam.erase(it);if(phase==0)BalanceTeams();}}else{status=connectionFailure.empty()?wire.Error():connectionFailure;ConnectionTrace("disconnected: "+status);Stop();return;}}}catch(const std::exception& error){status=error.what();ConnectionTrace("packet error: "+status);if(host)wire.Drop(event.peer);else {Stop();return;}}}
    if(!wire.Alive()){status=wire.Error();Stop();return;}
    if(!host&&!ready&&ConnectionNow()-joinStarted>20){status="Host nao enviou o mapa. Confira as versoes e tente novamente.";ConnectionTrace("world handshake timeout");Stop();return;}
    if(!ready)return;
    auto& own=fighters[me];if(own.teleport!=seenTeleport){PlayerState pose;pose.position=own.p;pose.yaw=own.yaw;pose.pitch=own.pitch;pose.valid=true;player.RestoreState(pose);ResetVitals();seenTeleport=own.teleport;}
    if(own.revision!=seenRevision){for(int i=0;i<9;++i)Hotbar[i]=own.inventory[i];for(int i=0;i<27;++i)Backpack[i]=own.inventory[9+i];CursorStack={};seenRevision=own.revision;}
    EquippedArmor={};EquippedArmor[0]={LeatherHelmet,1};EquippedArmor[1]={LeatherChestplate,1};EquippedArmor[2]={own.armor==2?DiamondLeggings:own.armor==1?IronLeggings:own.armor==3?ChainmailLeggings:LeatherLeggings,1};EquippedArmor[3]={own.armor==2?DiamondBoots:own.armor==1?IronBoots:own.armor==3?ChainmailBoots:LeatherBoots,1};
    Health=own.hp>0?own.hp:20;Absorption=own.absorption;Mode=own.eliminated?GameMode::Creative:GameMode::Survival;FallDistance=0;
    if(localSprintPause){player.SuspendSprint(.1f);localSprintPause=false;}
    if(Vector3Length(localMeleeForce)>.001f){player.MeleeKnockback(localMeleeForce);localMeleeForce={};}
    if(Vector3Length(localExplosionForce)>.001f){player.ExplosionKnockback(localExplosionForce);localExplosionForce={};}
    if(Vector3Length(localForce)>.001f){player.Knockback(localForce);localForce={};}
    if(own.respawn<=0&&!own.eliminated){positionClock+=dt;if(positionClock>=.05f){positionClock=0;auto pose=player.CaptureState();auto b=Packet(Position);b.Vec(pose.position);b.Float(pose.yaw);b.Float(pose.pitch);b.Byte(selected);b.Byte(player.IsSneaking());Request(std::move(b));}}
    if(host){Simulate(dt);SimulatePenguins(dt);}
    for(auto& p:penguins)if(p.active)p.display=host?p.p:Vector3Lerp(p.display,p.p,std::min(1.f,dt*18.f));
    PenguinAudio(dt);
    if(host){snapshotClock+=dt;if(snapshotClock>=.1f){snapshotClock=0;SendState();}}
    for(auto& f:fighters){f.display=Vector3Lerp(f.display,f.p,std::min(1.f,dt*14));f.swingAge=std::min(.35f,f.swingAge+dt);}
    (void)playing;
}
bool CanMine(const Hit& hit){return Ready()&&hit.found&&Inside(hit.x,hit.y,hit.z)&&Breakable(me,hit.x,hit.y,hit.z);}
void Dig(const Hit& hit,int stage,int selected){if(!hit.found)return;auto b=Packet(DigRequest);b.Byte(stage);b.Int(hit.x);b.Int(hit.y);b.Int(hit.z);b.Byte(selected);Request(std::move(b));}
std::vector<PenguinState> PenguinStates(){std::vector<PenguinState> result;for(int team=0;team<Teams;++team){auto& p=penguins[team];if(p.active)result.push_back({team,p.p,p.direction,p.age});}return result;}
std::vector<ArrowState> ArrowStates(){std::vector<ArrowState> result;for(const auto& shot:shots)if(shot.kind==Arrow)result.push_back({shot.p,Vector3Normalize(shot.v),shot.embedded,shot.damage,shot.critical});return result;}
void Place(int x,int y,int z,int selected,const Player* player){if(player){auto pose=player->CaptureState();auto position=Packet(Position);position.Vec(pose.position);position.Float(pose.yaw);position.Float(pose.pitch);position.Byte(selected);position.Byte(player->IsSneaking());Request(std::move(position));}auto b=Packet(PlaceRequest);b.Int(x);b.Int(y);b.Int(z);b.Byte(selected);Request(std::move(b));}
void Attack(int selected,const Player* player){if(player){auto pose=player->CaptureState();auto position=Packet(Position);position.Vec(pose.position);position.Float(pose.yaw);position.Float(pose.pitch);position.Byte(selected);position.Byte(player->IsSneaking());Request(std::move(position));}auto b=Packet(AttackRequest);b.Byte(selected);Request(std::move(b));}
namespace {bool localBowHeld=false;float localBowAge=0,localBowHeartbeat=0;int localBowSlot=-1;}
float BowDrawTime(){return localBowHeld?localBowAge:-1.f;}
void CancelBow(){if(localBowHeld&&Ready()){auto b=Packet(BowRequest);b.Byte(localBowSlot);b.Byte(2);Request(std::move(b));}localBowHeld=false;localBowAge=localBowHeartbeat=0;localBowSlot=-1;}
void BowInput(int selected,bool held,bool pressed,bool released,float dt,const Player* player){
    bool valid=Ready()&&phase==1&&Live(me)&&selected>=0&&selected<9&&Hotbar[selected].block==Bow&&Hotbar[selected].count&&Count(fighters[me],Arrow)>0;
    if(localBowHeld&&(!valid||selected!=localBowSlot)){CancelBow();return;}
    if(!valid)return;
    if(pressed){CancelBow();localBowHeld=true;localBowSlot=selected;localBowAge=localBowHeartbeat=0;auto b=Packet(BowRequest);b.Byte(selected);b.Byte(0);Request(std::move(b));}
    if(localBowHeld&&held){localBowAge=std::min(1.5f,localBowAge+dt);localBowHeartbeat+=dt;if(localBowHeartbeat>=.1f){localBowHeartbeat=0;auto b=Packet(BowRequest);b.Byte(selected);b.Byte(3);Request(std::move(b));}}
    if(localBowHeld&&released){if(player){auto pose=player->CaptureState();auto b=Packet(Position);b.Vec(pose.position);b.Float(pose.yaw);b.Float(pose.pitch);b.Byte(selected);b.Byte(player->IsSneaking());Request(std::move(b));}auto b=Packet(BowRequest);b.Byte(selected);b.Byte(1);Request(std::move(b));localBowHeld=false;localBowSlot=-1;localBowAge=0;}
}
void HoldUse(int selected,bool held){static double sent=-1;static bool previous=false;static int slot=-1;if(!Ready())return;held=held&&selected>=0&&selected<9&&Hotbar[selected].block==GoldenApple&&Hotbar[selected].count;if(held!=previous||selected!=slot||(held&&(clock-sent>=.1||clock<sent))){auto b=Packet(HoldUseRequest);b.Byte(selected);b.Byte(held);Request(std::move(b));sent=clock;previous=held;slot=selected;}}
void Use(int selected){auto b=Packet(UseRequest);b.Byte(selected);Request(std::move(b));}
bool NearShop(const Player& player){return ready&&phase==1&&ShopReach(player.CaptureState().position,shopKind);}
bool AimedShop(const Player& player){
    if(!ready||phase!=1)return false;
    auto camera=player.Camera();Ray ray{camera.position,Vector3Normalize(Vector3Subtract(camera.target,camera.position))};
    float closest=5;int selected=-1;auto block=Trace(ray.position,ray.direction);
    if(block.found){auto hit=GetRayCollisionBox(ray,{{float(block.x),float(block.y),float(block.z)},{block.x+1.f,block.y+1.f,block.z+1.f}});if(hit.hit)closest=std::min(closest,hit.distance);}
    for(int team=0;team<TeamLimit();++team)for(int kind=0;kind<2;++kind){auto p=Vendor(team,kind);if(Vector3Distance(player.CaptureState().position,p)>=5)continue;auto hit=GetRayCollisionBox(ray,{{p.x-.4f,p.y,p.z-.4f},{p.x+.4f,p.y+1.95f,p.z+.4f}});if(hit.hit&&hit.distance<closest){closest=hit.distance;selected=kind;}}
    if(selected<0)return false;
    shopKind=selected;return true;
}
bool NearChest(const Player& player){return ready&&phase==1&&ChestReach(player.CaptureState().position,TeamOf(me));}
void DrawWorld(const Camera3D& camera,Texture2D atlas){if(!Ready())return;BeginMode3D(camera);for(int i=0;i<MaxPlayers;++i){if(i!=me&&fighters[i].connected&&!fighters[i].eliminated&&fighters[i].respawn<=0&&fighters[i].invisTime<=0){auto p=fighters[i].display;auto& f=fighters[i];
        auto tint=[&](Color c){return f.hitFlash>0?Color{255,0,0,c.a}:c;};
        Color shirt=tint(Outfit(f));Color metal=f.armor==2?Color{87,224,224,255}:f.armor==1?Color{207,211,217,255}:f.armor==3?Color{125,131,139,255}:Outfit(f);metal=tint(metal);
        rlPushMatrix();
        rlTranslatef(p.x,p.y,p.z);
        rlRotatef(-f.yaw*RAD2DEG,0,1,0);
        const float hipY=f.sneaking?.82f:.67f,hipZ=f.sneaking?.16f:0;
        float swing=std::sin(std::clamp(f.swingAge/.35f,0.f,1.f)*PI);
        rlPushMatrix();rlTranslatef(0,hipY,hipZ);if(f.sneaking)rlRotatef(-28,1,0,0);
        DrawCube({0,.36f,0},.62f,.72f,.36f,shirt);
        rlPopMatrix();
        for(float side:{-.43f,.43f}){
            rlPushMatrix();ArmPose(f,side,swing);DrawCube({0,-.32f,0},.22f,.64f,.26f,shirt);rlPopMatrix();
        }
        for(float side:{-.17f,.17f}){
            rlPushMatrix();rlTranslatef(side,hipY,hipZ);if(f.sneaking)rlRotatef(13,1,0,0);
            DrawCube({0,-.32f,0},.25f,.64f,.3f,metal);rlPopMatrix();
            DrawCube({side,.12f,f.sneaking?.02f:-.04f},.28f,.24f,.38f,metal);
        }
        rlPushMatrix();
        rlTranslatef(0,f.sneaking?1.63f:1.64f,f.sneaking?-.21f:0);
        rlRotatef(f.pitch*RAD2DEG,1,0,0);
        DrawCube({0,0,0},.52f,.52f,.52f,shirt);
        DrawCube({0,-.02f,-.27f},.42f,.26f,.02f,tint(BEIGE));
        for(float side:{-.11f,.11f})DrawCube({side,.02f,-.285f},.07f,.05f,.015f,tint({45,39,34,255}));
        rlPopMatrix();
        rlPopMatrix();
        Block held=f.inventory[f.selected].count?f.inventory[f.selected].block:Air;
        if(held!=Air&&atlas.id){
            rlPushMatrix();rlTranslatef(p.x,p.y,p.z);rlRotatef(-f.yaw*RAD2DEG,0,1,0);
            bool sprite=!Placeable(held)||Plant(held);
            ArmPose(f,.43f,swing);rlTranslatef(0,-.64f,0);
            if(sprite){rlRotatef(-25,1,0,0);rlTranslatef(0,.24f,-.08f);rlRotatef(90,0,1,0);}else rlTranslatef(0,.02f,-.17f);
            float size=sprite?(Tool(held)||held==Stick?.65f:.45f):.32f;rlScalef(size,size,size);
            Matrix transform=rlGetMatrixTransform();rlPopMatrix();
            float brightness=std::clamp(std::max(SkyLightAt(int(p.x),int(p.y+1),int(p.z))/24.f*Daylight(),BlockLightAt(int(p.x),int(p.y+1),int(p.z))/24.f),.28f,1.f);
            unsigned char light=static_cast<unsigned char>(brightness*255);DrawHeldItem3D(atlas,held,transform,{light,light,light,255},ItemSkin(held,i),f.bowStart>=0?clock-f.bowStart:-1.f);
        }
    }}

    DrawPenguins();
    for(int i=0;i<TeamLimit();++i)for(int kind=0;kind<2;++kind){
        auto p=Vendor(i,kind);Color robe=kind?Color{91,62,111,255}:Color{106,73,48,255};Color skin{177,136,103,255};
        rlPushMatrix();auto facing=Vector3Subtract(bases[i],p);float heading=std::atan2(facing.x,facing.z)*RAD2DEG;
        rlTranslatef(p.x,p.y,p.z);rlRotatef(heading,0,1,0);rlTranslatef(-p.x,-p.y,-p.z);
        DrawCube(Vector3Add(p,{0,.84f,0}),.64f,1.12f,.44f,robe);
        DrawCube(Vector3Add(p,{0,.34f,.005f}),.69f,.18f,.48f,ColorBrightness(robe,-.18f));
        DrawCube(Vector3Add(p,{0,.81f,.231f}),.4f,.69f,.025f,kind?Color{151,123,172,255}:Color{186,156,112,255});
        DrawCube(Vector3Add(p,{0,.63f,.254f}),.62f,.065f,.035f,{69,47,30,255});
        DrawCube(Vector3Add(p,{0,.63f,.28f}),.08f,.08f,.025f,GOLD);
        for(float side:{-.17f,.17f}){DrawCube(Vector3Add(p,{side,.2f,0}),.23f,.4f,.3f,{65,48,38,255});DrawCube(Vector3Add(p,{side,.075f,.06f}),.25f,.15f,.4f,{47,35,28,255});}
        DrawCube(Vector3Add(p,{0,1.59f,0}),.57f,.65f,.55f,skin);
        for(float side:{-.29f,.29f})DrawCube(Vector3Add(p,{side,1.55f,0}),.055f,.2f,.17f,ColorBrightness(skin,-.12f));
        DrawCube(Vector3Add(p,{0,1.82f,0}),.59f,.12f,.57f,kind?Color{99,63,131,255}:Color{127,88,47,255});
        if(kind)DrawCube(Vector3Add(p,{0,1.83f,.297f}),.12f,.08f,.025f,{92,225,199,255});
        DrawCube(Vector3Add(p,{0,1.44f,.36f}),.16f,.3f,.23f,skin);
        DrawCube(Vector3Add(p,{0,1.73f,.282f}),.46f,.07f,.025f,{58,44,30,255});
        for(float side:{-.15f,.15f}){DrawCube(Vector3Add(p,{side,1.64f,.285f}),.13f,.1f,.025f,RAYWHITE);DrawCube(Vector3Add(p,{side,1.64f,.302f}),.055f,.085f,.025f,DARKGREEN);}
        DrawCube(Vector3Add(p,{0,1.34f,.284f}),.18f,.035f,.025f,{94,64,46,255});
        DrawCube(Vector3Add(p,{0,1.02f,.3f}),.76f,.23f,.23f,robe);
        DrawCube(Vector3Add(p,{0,1.02f,.43f}),.24f,.17f,.08f,skin);rlPopMatrix();
    }
    for(auto& g:generators)if(g.stock){DrawCube(Vector3Add(g.p,{0,.4f+std::sin(float(GetTime())*2)*.08f,0}),.35f,.2f,.35f,g.item==IronIngot?LIGHTGRAY:g.item==GoldIngot?GOLD:g.item==Diamond?SKYBLUE:GREEN);}
    for(auto& shot:shots){if(shot.kind==Tnt)DrawCube(shot.p,.6f,.6f,.6f,RED);else if(shot.kind==Arrow){DrawArrowModel(shot.p,shot.v);if(shot.critical&&!shot.embedded)DrawArrowSparkles(shot.p,shot.v,float(GetTime()));}else DrawSphere(shot.p,.16f,shot.kind==Fireball?ORANGE:shot.kind==EnderPearl?DARKGREEN:BEIGE);}EndMode3D();
    auto label=[&](Vector3 p,const std::string& text,Color color){Vector3 direction=Vector3Subtract(camera.target,camera.position);if(Vector3DotProduct(direction,Vector3Subtract(p,camera.position))<=0||Vector3Distance(camera.position,p)>36)return;Vector2 screen=GetWorldToScreen(p,camera);int width=MeasureText(text.c_str(),16);DrawRectangle(int(screen.x-width/2-3),int(screen.y-2),width+6,20,Fade(BLACK,.6f));DrawText(text.c_str(),int(screen.x-width/2),int(screen.y),16,color);};
    for(const auto& g:Generators())if((g.item==Diamond||g.item==Emerald)&&Vector3Distance(camera.position,g.position)<=5.f){std::string text=std::string(g.item==Diamond?"DIAMANTE":"ESMERALDA")+" | Nivel "+std::to_string(g.level)+" | "+std::to_string(int(std::ceil(g.remaining)))+"s";label(Vector3Add(g.position,{0,2.7f,0}),text,g.item==Diamond?SKYBLUE:GREEN);}
    for(int i=0;i<TeamLimit();++i)for(int kind=0;kind<2;++kind)label(Vector3Add(Vendor(i,kind),{0,2.2f,0}),kind?"MELHORIAS":"LOJA DE ITENS",kind?GREEN:GOLD);
    for(int i=0;i<MaxPlayers;++i)if(i!=me&&fighters[i].connected&&!fighters[i].sneaking&&fighters[i].invisTime<=0)label(Vector3Add(fighters[i].display,{0,2,0}),"["+std::to_string(fighters[i].profile.Level())+"] "+fighters[i].name,colors[TeamOf(i)]);
}
void DrawHud(){if(!Ready())return;int x=GetScreenWidth()-220,y=70;DrawRectangle(x-8,y-8,216,74+TeamLimit()*24,Fade(BLACK,.6f));DrawText("BEDWARS LAN",x,y,21,GOLD);DrawText(TextFormat("%02d:%02d",int(phaseClock)/60,int(phaseClock)%60),x,y+28,17,WHITE);
    for(int team=0;team<TeamLimit();++team)DrawText(TextFormat("%s %s %d/%d%s",TeamBed(team)?"[C]":"[ ]",teamNames[team],TeamCount(team),teamCapacity,team==TeamOf(me)?" <":""),x,y+52+team*24,17,TeamCount(team)?colors[team]:GRAY);
    const auto& f=fighters[me];DrawText(TextFormat("Nivel %d | XP %d/%d | Caixas %d",f.profile.Level(),f.profile.xp,f.profile.xp-f.profile.xp%50+50,f.profile.crates),12,GetScreenHeight()-195,16,GOLD);if(f.eating>0){int x=GetScreenWidth()/2-70,y=GetScreenHeight()/2+36;DrawRectangle(x,y,140,8,DARKGRAY);DrawRectangle(x,y,int(140*std::min(1.f,f.eating/1.61f)),8,GOLD);}
    if(f.speedTime>0||f.jumpTime>0||f.invisTime>0)DrawText(TextFormat("Velocidade %ds | Salto %ds | Invisibilidade %ds",int(f.speedTime),int(f.jumpTime),int(f.invisTime)),12,GetScreenHeight()-170,16,SKYBLUE);
    DrawText(TextFormat("Kills %d  Finais %d  Camas %d",f.kills,f.finals,f.beds),12,GetScreenHeight()-122,16,WHITE);
    if(phase==0){const char* message=phaseClock>0?TextFormat("Iniciando em %d",int(std::ceil(phaseClock))):host?"P: iniciar (2+ jogadores) | F8: reiniciar":"Lobby: aguardando o host iniciar";DrawText(message,12,90,17,WHITE);if(host)DrawText("Amigos entram no IP do host:25566",12,114,17,WHITE);}
    if(phase==0&&phaseClock>0){int w=GetScreenWidth(),h=GetScreenHeight();DrawRectangle(w/2-220,h/2-94,440,164,Fade(BLACK,.6f));const char* title="COMECANDO JOGO";DrawText(title,(w-MeasureText(title,28))/2,h/2-72,28,GOLD);const char* number=TextFormat("%d",int(std::ceil(phaseClock)));DrawText(number,(w-MeasureText(number,72))/2,h/2-25,72,WHITE);}
    if(f.respawn>0)DrawText(TextFormat("Renascendo em %d",int(std::ceil(f.respawn))),GetScreenWidth()/2-120,GetScreenHeight()/2-45,24,GOLD);
    if(f.eliminated)DrawText("Eliminado - aguarde a proxima rodada",GetScreenWidth()/2-220,110,22,RED);
    if(phase==2){const char* text=winner==TeamOf(me)?"VITORIA!":winner>=0?TextFormat("Vencedor: %s",teamNames[winner]):"EMPATE";DrawText(text,(GetScreenWidth()-MeasureText(text,32))/2,130,32,GOLD);}
    if(!status.empty())DrawText(status.c_str(),12,GetScreenHeight()-146,16,RAYWHITE);
}
namespace {
float ShopSlot(){return std::min(54.f,(GetScreenHeight()-100.f)/9.f);}
Rectangle ShopPanel(){float s=ShopSlot(),w=9*s+24,h=9*s+84;return {(GetScreenWidth()-w)/2,(GetScreenHeight()-h)/2,w,h};}
Rectangle ShopCell(int column,float row){auto panel=ShopPanel();float s=ShopSlot();return {panel.x+12+column*s,panel.y+36+row*s,s-3,s-3};}
Rectangle UpgradeCard(int index){auto p=ShopPanel();float s=ShopSlot();float w=(p.width-36)/3;return {p.x+12+index*(w+6),p.y+42,w,3.4f*s};}
void ShopIcon(Texture2D atlas,Rectangle rect,Block item,int count=0){
    bool hover=CheckCollisionPointRec(GetMousePosition(),rect);
    DrawRectangleRec(rect,hover?Color{166,166,166,255}:Color{139,139,139,255});
    DrawRectangleLinesEx(rect,2,{62,62,62,255});
    DrawLine(int(rect.x+2),int(rect.y+rect.height-2),int(rect.x+rect.width-2),int(rect.y+rect.height-2),LIGHTGRAY);
    DrawLine(int(rect.x+rect.width-2),int(rect.y+2),int(rect.x+rect.width-2),int(rect.y+rect.height-2),LIGHTGRAY);
    if(item!=Air&&atlas.id){
        if(Placeable(item)&&Solid(item)&&item!=Ladder){
            float cx=rect.x+rect.width*.5f,cy=rect.y+rect.height*.48f,r=rect.width*.35f;
            Vector2 top{cx,cy-r},left{cx-r,cy-r*.45f},middle{cx,cy+r*.1f},right{cx+r,cy-r*.45f},bottom{cx,cy+r},bl{cx-r,cy+r*.45f},br{cx+r,cy+r*.45f};
            auto face=[&](int direction,Color tint,std::array<Vector2,4> points){
                int tile=BlockTile(item,direction);float u0=(tile*TileSize+.5f)/atlas.width,u1=(tile*TileSize+TileSize-.5f)/atlas.width,v0=.5f/atlas.height,v1=(TileSize-.5f)/atlas.height;
                rlSetTexture(atlas.id);rlBegin(RL_QUADS);rlColor4ub(tint.r,tint.g,tint.b,tint.a);
                const Vector2 uv[]={{u0,v0},{u0,v1},{u1,v1},{u1,v0}};
                for(int i=0;i<4;++i){rlTexCoord2f(uv[i].x,uv[i].y);rlVertex2f(points[i].x,points[i].y);}rlEnd();rlSetTexture(0);
            };
            face(4,{180,180,180,255},{left,bl,bottom,middle});face(0,{220,220,220,255},{middle,bottom,br,right});face(2,WHITE,{top,left,middle,right});
        }else{
            int tile=BlockTile(item);float size=std::floor((rect.width-10)/TileSize)*TileSize;if(size<TileSize)size=rect.width-8;
            Rectangle target{std::round(rect.x+(rect.width-size)/2),std::round(rect.y+(rect.height-size)/2),size,size};
            DrawTexturePro(atlas,{float(tile*TileSize),0,TileSize,TileSize},target,{0,0},0,WHITE);
        }
    }
    if(count>1){const char* text=TextFormat("%d",count);int x=int(rect.x+rect.width-4-MeasureText(text,16)),y=int(rect.y+rect.height-18);DrawText(text,x+1,y+1,16,BLACK);DrawText(text,x,y,16,WHITE);}
}
void ShopTooltip(const std::string& name,const std::string& price={},bool affordable=true){
    if(name.empty())return;
    int font=16,width=std::max(MeasureText(name.c_str(),font),MeasureText(price.c_str(),font))+20,height=price.empty()?32:56;
    auto mouse=GetMousePosition();int x=std::clamp(int(mouse.x)+14,4,std::max(4,GetScreenWidth()-width-4));int y=int(mouse.y)-height-8;if(y<4)y=std::min(GetScreenHeight()-height-4,int(mouse.y)+20);
    DrawRectangle(x,y,width,height,{20,12,29,250});DrawRectangleLinesEx({float(x),float(y),float(width),float(height)},2,{91,54,132,255});
    DrawText(name.c_str(),x+10,y+8,font,RAYWHITE);if(!price.empty())DrawText(price.c_str(),x+10,y+31,font,affordable?Color{114,230,111,255}:Color{255,112,105,255});
}

const char* CategoryName(int category){constexpr const char* names[]={"Blocos","Espadas","Armaduras","Ferramentas","Pocoes","Utilitarios"};return names[std::clamp(category,0,5)];}
}
void DrawShop(Texture2D atlas){
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.72f));auto panel=ShopPanel();float s=ShopSlot();DrawRectangleRec(panel,{198,198,198,255});DrawRectangleLinesEx(panel,3,{238,238,238,255});
    DrawText(shopKind?"Melhorias da equipe":CategoryName(shopCategory),int(panel.x+12),int(panel.y+10),20,DARKGRAY);
    constexpr Block categories[]={Grass,GoldenSword,IronBoots,IronPickaxe,SpeedPotion,Tnt};
    std::string tooltip,price;bool affordable=true;
    if(!shopKind)for(int i=0;i<6;++i){auto rect=ShopCell(i,0);ShopIcon(atlas,rect,categories[i]);if(i==shopCategory)DrawRectangleLinesEx(rect,3,GOLD);if(CheckCollisionPointRec(GetMousePosition(),rect))tooltip=CategoryName(i);}
    DrawLine(int(panel.x+12),int(panel.y+36+1.12f*s),int(panel.x+panel.width-12),int(panel.y+36+1.12f*s),GRAY);
    if(!shopKind)DrawText("Clique para comprar",int(panel.x+12),int(panel.y+36+3.7f*s),14,DARKGRAY);
    int visible=0;
    const auto& own=fighters[me];
    if(shopKind){
        const int levels[]={own.sharp,own.protection,own.forge},limits[]={1,4,2};
        const char* names[]={"Afiacao","Protecao","Forja"};
        const char* descriptions[]={"Mais dano","Menos dano","Mais recursos"};
        const Block icons[]={GoldenSword,IronChestplate,IronIngot};
        int card=0;
        for(const auto& offer:offers)if(ShowOffer(offer,own)){
            auto r=UpgradeCard(card);bool full=levels[card]>=limits[card],canBuy=Count(own,offer.currency)>=offer.cost;
            bool hover=CheckCollisionPointRec(GetMousePosition(),r);
            DrawRectangleRec(r,hover?Color{226,223,213,255}:Color{211,209,202,255});DrawRectangleLinesEx(r,2,{91,87,78,255});
            DrawRectangle(int(r.x+2),int(r.y+2),int(r.width-4),4,full?SKYBLUE:GOLD);
            float iconSize=std::min(54.f,r.width-18);ShopIcon(atlas,{r.x+(r.width-iconSize)/2,r.y+13,iconSize,iconSize},icons[card]);
            auto centered=[&](const char* text,float y,int font,Color color){DrawText(text,int(r.x+(r.width-MeasureText(text,font))/2),int(y),font,color);};
            centered(names[card],r.y+iconSize+22,16,DARKGRAY);centered(descriptions[card],r.y+iconSize+45,12,{87,82,74,255});
            for(int n=0;n<limits[card];++n){float bw=(r.width-24)/limits[card];DrawRectangle(int(r.x+12+n*bw),int(r.y+r.height-59),int(bw-3),7,n<levels[card]?Color{76,159,114,255}:Color{133,129,118,255});}
            centered(full?"MAXIMO":TextFormat("%d diamantes",offer.cost),r.y+r.height-43,14,full?Color{43,111,144,255}:canBuy?Color{42,112,54,255}:Color{156,61,52,255});
            centered(full?"Adquirido":"Clique para melhorar",r.y+r.height-23,10,DARKGRAY);
            if(hover){tooltip=offer.title;price=full?"Nivel maximo":"Preco: "+std::to_string(offer.cost)+" diamantes";affordable=canBuy||full;}++card;
        }
    }
    for(const auto& o:offers){if(shopKind||!ShowOffer(o,own))continue;auto rect=ShopCell(visible%9,1.45f+visible/9);++visible;Block icon=o.item;if(icon==Air)icon=o.special==1?IronBoots:o.special==2?DiamondBoots:o.special==6?ChainmailBoots:o.special==3?GoldenSword:o.special==4?IronChestplate:IronIngot;ShopIcon(atlas,rect,icon,o.count);bool owned=(o.special==3&&own.sharp)||(o.special==4&&own.protection>=4)||(o.special==5&&own.forge>=2)||((o.special==1||o.special==2||o.special==6)&&ArmorRank(own.armor)>=ArmorRank(o.special==1?1:o.special==2?2:3));
        Color stateColor=owned?SKYBLUE:Count(own,o.currency)>=o.cost?Color{83,163,83,255}:Color{168,80,73,255};DrawRectangle(int(rect.x+3),int(rect.y+rect.height-4),int(rect.width-6),2,stateColor);
        if(CheckCollisionPointRec(GetMousePosition(),rect)){DrawRectangleLinesEx(rect,2,WHITE);tooltip=o.title;price="Preco: "+std::to_string(o.cost)+" "+Names[o.currency];affordable=Count(own,o.currency)>=o.cost;if(owned)price="Melhoria ja adquirida";else price+=" | Clique para comprar";if(ToolLevel(o.item)==3&&(IsAxe(o.item)?own.axe:own.pick)==3)tooltip+=" (Maximo)";}}
    DrawLine(int(panel.x+12),int(panel.y+36+4.12f*s),int(panel.x+panel.width-12),int(panel.y+36+4.12f*s),GRAY);
    DrawText("Inventario",int(panel.x+12),int(panel.y+36+4.3f*s),17,DARKGRAY);
    for(int i=0;i<36;++i){int slot=i<27?i+9:i-27;auto stack=own.inventory[slot];auto rect=ShopCell(i%9,i<27?4.8f+i/9:8.f);ShopIcon(atlas,rect,stack.count?stack.block:Air,stack.count);if(stack.count&&CheckCollisionPointRec(GetMousePosition(),rect)){tooltip=Names[stack.block];price.clear();}}

    DrawText(TextFormat("F %d | O %d | D %d | E %d",Count(own,IronIngot),Count(own,GoldIngot),Count(own,Diamond),Count(own,Emerald)),int(panel.x+12),int(panel.y+panel.height-25),16,DARKGRAY);
    ShopTooltip(tooltip,price,affordable);
}
void Buy(int offer){if(offer<0||offer>=int(std::size(offers)))return;auto b=Packet(BuyRequest);b.Byte(offer);Request(std::move(b));}
void ShopInput(){
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))return;
    if(!shopKind)for(int i=0;i<6;++i)if(CheckCollisionPointRec(GetMousePosition(),ShopCell(i,0))){shopCategory=i;return;}
    int visible=0;for(int i=0;i<int(std::size(offers));++i){if(!ShowOffer(offers[i],fighters[me]))continue;auto rect=shopKind?UpgradeCard(visible):ShopCell(visible%9,1.45f+visible/9);++visible;if(CheckCollisionPointRec(GetMousePosition(),rect)){Buy(i);return;}}
}
namespace {
Rectangle CosmeticButton(int action){float width=std::min(240.f,(GetScreenWidth()-36.f)/2);return {GetScreenWidth()/2.f-width-4+action*(width+8),12,width,34};}
void CosmeticsInput(int action){if(action==0)OpenCrate();else cosmetics::Toggle();}
}
void DrawInventory(Texture2D atlas,bool chest){
    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.7f));const auto& own=fighters[me];std::string tooltip;
    if(!chest&&cosmetics::Opened()){cosmetics::Draw(atlas);return;}
    if(!chest){for(int i=0;i<2;++i){auto r=CosmeticButton(i);bool hover=CheckCollisionPointRec(GetMousePosition(),r);DrawRectangleRec(r,hover?Color{110,110,120,255}:Color{65,65,72,255});DrawRectangleLinesEx(r,2,GRAY);std::string text=i?"Inventario de skins":"Abrir caixa ("+std::to_string(own.profile.crates)+")";DrawText(text.c_str(),int(r.x+8),int(r.y+9),16,i?WHITE:GOLD);}}
    auto draw=[&](Rectangle r,ItemStack stack,bool selected=false){DrawInventorySlot(r,CheckCollisionPointRec(GetMousePosition(),r));if(selected)DrawRectangleLinesEx(r,2,GOLD);if(stack.count){float margin=r.width*.12f;DrawTexturePro(atlas,{float(BlockTile(stack.block)*TileSize),0,TileSize,TileSize},{r.x+margin,r.y+margin,r.width-2*margin,r.height-2*margin},{0,0},0,WHITE);if(stack.count>1)DrawText(TextFormat("%d",stack.count),int(r.x+r.width-19),int(r.y+r.height-17),16,WHITE);if(CheckCollisionPointRec(GetMousePosition(),r))tooltip=Names[stack.block];}};
    if(!chest){DrawClassicInventoryPanel();DrawInventoryPlayer(Outfit(own));float scale=InventoryScale();auto p=ClassicInventoryPanel();DrawText("Fabricacao",int(p.x+96*scale),int(p.y+5*scale),int(8*scale),DARKGRAY);
        for(int i=0;i<4;++i)draw(ClassicArmorSlot(i),EquippedArmor[i]);
        for(int i=0;i<4;++i)draw(ClassicCraftSlot(i),own.crafting[i],false);
        draw(ClassicCraftOutput(),CraftResult(own.crafting.data(),2));DrawText(">",int(p.x+139*scale),int(p.y+29*scale),int(10*scale),GRAY);
    }else {auto first=InventoryRect(36,true);DrawText("BAU DA EQUIPE",int(first.x),int(first.y-28),20,WHITE);}
    for(int i=0;i<(chest?63:36);++i){int index=i<36?StorageIndex(i):i;draw(InventoryRect(i,chest),index<36?own.inventory[index]:chests[own.team][index-36],false);}
    if(own.cursor.count){auto m=GetMousePosition();float size=InventoryScale()*16;draw({m.x-size/2,m.y-size/2,size,size},own.cursor);}
    else ShopTooltip(tooltip);
}
void SwapSlots(int from,int to,bool chest,bool exact){if(from<0||to<0||from>=63||to>=63)return;auto b=Packet(SwapRequest);b.Byte(from);b.Byte(to);b.Byte(chest);b.Byte(exact);Request(std::move(b));}
void InventoryClick(int slot,bool right,bool chest,bool quick){if(slot<0||slot>=(chest?63:40))return;auto b=Packet(InventoryClickRequest);b.Byte(slot);b.Byte(right);b.Byte(chest);if(quick)b.Byte(1);Request(std::move(b));}
void Craft(int target){auto b=Packet(CraftRequest);b.Byte(target);Request(std::move(b));}
void InventoryInput(bool chest){
    if(!chest&&cosmetics::Input())return;
    if(!chest&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))for(int i=0;i<2;++i)if(CheckCollisionPointRec(GetMousePosition(),CosmeticButton(i))){CosmeticsInput(i);return;}
    int key=PressedHotbarBinding();bool left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT),right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if(!chest&&CheckCollisionPointRec(GetMousePosition(),ClassicCraftOutput())){if(key>=0||left||right)Craft(key>=0?key:ActionDown(BindingAction::Sneak)?255:254);return;}
    for(int i=0;i<(chest?63:40);++i)if(CheckCollisionPointRec(GetMousePosition(),InventoryRect(i,chest))){int slot=i<36?StorageIndex(i):i;
        if(key>=0){SwapSlots(slot,key,chest,true);return;}if(left||right)InventoryClick(slot,right,chest,left&&ActionDown(BindingAction::Sneak));return;
    }
}
}
