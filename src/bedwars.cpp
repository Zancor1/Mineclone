#include "bedwars.h"
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
constexpr int Version=8,Teams=4,MaxPlayers=8;
Vector3 bases[4]{};
int islandGap=64,mapId=0,teamCapacity=1;
void SetLayout(int gap){islandGap=gap;auto positions=ArenaBases(gap,mapId);std::copy(positions.begin(),positions.end(),bases);}

constexpr Color colors[4]={RED,BLUE,GREEN,YELLOW};
constexpr Block wool[4]={RedWool,BlueWool,GreenWool,YellowWool};
const char* teamNames[]={"Vermelho","Azul","Verde","Amarelo"};
enum Message {Hello=1,Welcome,Snapshot,Edit,Position,DigRequest,PlaceRequest,AttackRequest,BuyRequest,UseRequest,SwapRequest,StartRequest,RestartRequest,Force,Error,ChatMessage,ChooseTeam,SprintPause,HoldUseRequest,SwingAnimation};
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
    bool connected=false,bed=true,eliminated=false,sneaking=false;int team=0,peer=-1,lastAttacker=-1,selected=0,revision=1,teleport=0,kills=0,finals=0,beds=0,armor=0,pick=0,axe=0,sharp=0,protection=0,forge=0;
    std::string name;Vector3 p{},display{};float yaw=0,pitch=0,hp=20,respawn=0,hurtClock=0,regen=0,absorption=0,lastPosition=0,lastAction=-10,lastChat=-10,lastHurt=-100,absorbTime=0,digStart=-10,speedTime=0,jumpTime=0,invisTime=0,healClock=0,regenClock=0,eating=0,eatHeartbeat=-10,fallDistance=0,swingAge=.35f;int eatSlot=-1;
    Hit digging{};std::array<ItemStack,36> inventory{};
};
struct Generator {Vector3 p;Block item;int stock=0;float clock=0,interval=1;int cap=32,team=-1;};
struct Shot {Vector3 p,v;int owner;Block kind;float life;};
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
LanTransport wire;LanDiscovery discovery;unsigned short roomPort=25566;std::array<Fighter,MaxPlayers> fighters;std::array<ChestInventory,Teams> chests;std::vector<Generator> generators;std::vector<Shot> shots;
std::map<int,int> peerTeam;std::set<int> placed;bool active=false,ready=false,host=false;int me=0,phase=0,winner=-1,participants=0,epoch=0,seenEpoch=-1,seenTeleport=-1,seenRevision=-1,inventorySelection=-1,shopKind=0,shopCategory=0;float clock=0,phaseClock=0,snapshotClock=0,positionClock=0;std::string status;std::vector<std::string> chatMessages;
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
Vector3 TeamSpawn(int id){auto p=bases[TeamOf(id)];int seat=0;for(int i=0;i<id;++i)if(fighters[i].connected&&TeamOf(i)==TeamOf(id))++seat;return {p.x+seat*1.2f,26.001f,p.z-(mapId==3?0:1.5f)};}
Vector3 Vendor(int team,int kind){if(mapId==3)return {team==0?74.5f:182.5f,26,float((team==0?(kind?34:22):(kind?22:34))+128)+.5f};return {bases[team].x+(kind?4.f:-4.f),26,bases[team].z+(kind?2.f:-2.f)};}
Vector3 TeamChest(int team){if(mapId==3)return {float((team?51:-51)+128),26,float((team?32:24)+128)};return {bases[team].x+4,26,bases[team].z-2};}
bool ChestReach(Vector3 p,int team){if(Vector3Distance(p,TeamChest(team))<5)return true;return mapId==3&&Vector3Distance(p,{float((team?51:-51)+128),26,float((team?24:32)+128)})<5;}
void RemoveBed(int team);
bool ShopReach(Vector3 p,int kind){for(int team=0;team<TeamLimit();++team)if(Vector3Distance(p,Vendor(team,kind))<5)return true;return false;}
int Cell(int x,int y,int z){return (x*Height+y)*Width+z;}
Vector3 LobbyPosition(int id){return {125.5f+(id%4)*2,66.001f,128.5f+(id/4)*2};}
bool Fighting(const Fighter& f){return f.connected&&!f.eliminated&&f.respawn<=0;}
bool Live(int id){return phase==1&&Fighting(fighters[id]);}
void Stack(Buffer& b,const ItemStack& s){b.Byte(s.block);b.Byte(s.count);b.Int(s.damage);}
ItemStack Stack(Buffer& b){int block=b.Byte(),count=b.Byte(),damage=b.Int();if(block>=BlockCount||count>64||damage<0||damage>65535)throw std::runtime_error("Item BedWars invalido");return {Block(block),static_cast<unsigned char>(count),static_cast<unsigned short>(damage)};}
int Count(const Fighter& f,Block b){int count=0;for(auto s:f.inventory)if(s.block==b)count+=s.count;return count;}
bool Give(Fighter& f,Block item,int count){ItemStack stack{item,static_cast<unsigned char>(count)};auto inventory=f.inventory;TransferStack(stack,inventory.data(),inventory.size());if(stack.count)return false;f.inventory=inventory;++f.revision;return true;}
void Consume(Fighter& f,Block b,int count){for(auto& s:f.inventory)if(s.block==b&&s.count){int take=std::min(count,int(s.count));s.count-=take;count-=take;if(!s.count)s={};if(!count)break;}++f.revision;}
void Notice(const std::string& text){status=text;}
void AddChat(const std::string& text){chatMessages.push_back(text);if(chatMessages.size()>8)chatMessages.erase(chatMessages.begin());}
void Tell(int id,const std::string& text){if(id==0)Notice(text);else {auto b=Packet(Error);b.Text(text);wire.Send(fighters[id].peer,b.bytes);}}
void Teleport(int id,Vector3 p){fighters[id].p=p;fighters[id].display=p;++fighters[id].teleport;}
int ArmorRank(int armor){return armor==2?3:armor==1?2:armor==3?1:0;}
Block PickItem(int level){return level==1?WoodenPickaxe:level==2?StonePickaxe:IronPickaxe;}
Block AxeItem(int level){return level==1?WoodenAxe:level==2?StoneAxe:IronAxe;}
int ToolLevel(Block item){if(item==WoodenPickaxe||item==WoodenAxe)return 1;if(item==StonePickaxe||item==StoneAxe)return 2;if(item==IronPickaxe||item==IronAxe)return 3;return 0;}
bool IsAxe(Block item){return item==WoodenAxe||item==StoneAxe||item==IronAxe;}
int OfferCategory(const Offer& o){if((o.special>=1&&o.special<=2)||o.special==6)return 2;if(Sword(o.item)||o.item==Stick)return 1;if(ToolLevel(o.item))return 3;if(o.item>=SpeedPotion&&o.item<=InvisibilityPotion)return 4;if(o.item!=Tnt&&o.item!=Ladder&&Placeable(o.item))return 0;return 5;}
bool ShowOffer(const Offer& o,const Fighter& f){if(o.special==7)return false;bool upgrade=o.special>=3&&o.special<=5;if(upgrade!=bool(shopKind))return false;if(shopKind)return true;if(OfferCategory(o)!=shopCategory)return false;int level=ToolLevel(o.item);return !level||level==std::min(3,(IsAxe(o.item)?f.axe:f.pick)+1);}
void Loadout(int id){auto& f=fighters[id];f.inventory={};Give(f,WoodenSword,1);if(f.pick)Give(f,PickItem(f.pick),1);if(f.axe)Give(f,AxeItem(f.axe),1);f.hp=20;f.absorption=f.regen=f.speedTime=f.jumpTime=f.invisTime=0;f.healClock=f.regenClock=f.eating=f.fallDistance=0;f.eatSlot=-1;f.sneaking=false;f.digging={};++f.revision;}
void Write(int x,int y,int z,Block b){if(Inside(x,y,z))World[x][y][z]=b;}
void GenerateArena(){
    ClearRemoteWorld();ClearItems();PersonalCrafting={};WorkbenchCrafting={};EquippedArmor={};RegionOriginX=RegionOriginZ=0;SetClockMinute(720);placed.clear();shots.clear();generators.clear();chests={};
    BuildArenaTerrain(mapId,islandGap,Write);
    if(mapId==3){generators.push_back({{128.5f,25,156.5f},Emerald,0,0,35,4});for(int team=0;team<2;++team){Vector3 p{team?186.5f:70.5f,25.5f,156.5f};generators.push_back({p,IronIngot,0,0,2.5f,48,team});generators.push_back({p,GoldIngot,0,0,8,16,team});}generators.push_back({{128.5f,27,183.5f},Diamond,0,0,25,8});InvalidateLighting();return;}
    generators.push_back({{128.5f,26,128.5f},Emerald,0,0,35,4});
    for(int team=0;team<TeamLimit();++team){auto base=bases[team];int x=int(base.x),z=int(base.z);generators.push_back({{x+.5f,26,z-2.5f},IronIngot,0,0,2.5f,48,team});generators.push_back({{x+.5f,26,z-2.5f},GoldIngot,0,0,8,16,team});}
    for(int i=0;i<4;++i){int x=i<2?int(bases[i].x):128,z=i<2?128:int(bases[i].z)-7;if(i==3)z=int(bases[0].z)+7;generators.push_back({{x+.5f,26,z+.5f},Diamond,0,0,25,8});}
    InvalidateLighting();
}
void ResetMatch(){
    seenTeleport=seenRevision=-1;++epoch;GenerateArena();phase=0;phaseClock=clock=0;winner=-1;participants=0;
    for(int i=0;i<MaxPlayers;++i){auto& f=fighters[i];auto previous=f;f=Fighter{};f.name=previous.name;f.peer=previous.peer;f.team=previous.team;f.connected=previous.connected;f.pitch=-.55f;if(f.connected)Give(f,WhiteWool,1);Teleport(i,LobbyPosition(i));}
    BalanceTeams();Notice("Lobby: clique com a la para escolher o time. Host: P inicia.");
}
void EditCell(int x,int y,int z,Block block){Change(x,y,z,block);auto b=Packet(Edit);b.Int(x);b.Int(y);b.Int(z);b.Byte(block);wire.Broadcast(b.bytes);}
void RemoveBed(int team){SetTeamBed(team,false);for(auto p:MapBeds(mapId,team,bases[team]))EditCell(int(p.x),int(p.y),int(p.z),Air);}
Vector3 localForce{},localMeleeForce{};bool localSprintPause=false;
void Knock(int id,Vector3 v,bool melee=false){if(id==0){if(melee)localMeleeForce=v;else localForce=Vector3Add(localForce,v);}else {auto b=Packet(Force);b.Vec(v);b.Byte(melee);wire.Send(fighters[id].peer,b.bytes);}}
void Kill(int id,int killer){auto& f=fighters[id];if(f.respawn>0||f.eliminated)return;if(killer==id)killer=-1;if(killer<0&&f.lastAttacker!=id&&clock-f.lastHurt<10)killer=f.lastAttacker;if(killer>=0&&killer<MaxPlayers){++fighters[killer].kills;if(!f.bed)++fighters[killer].finals;for(Block item:{IronIngot,GoldIngot,Diamond,Emerald}){int remaining=Count(f,item);while(remaining>0){int count=std::min(64,remaining);if(!Give(fighters[killer],item,count))break;remaining-=count;}}}
    f.hp=0;f.inventory={};++f.revision;if(f.bed){f.respawn=5;if(f.pick>1)--f.pick;if(f.axe>1)--f.axe;}else f.eliminated=true;Teleport(id,LobbyPosition(id));}
void Damage(int id,float damage,int attacker,Vector3 force={},bool melee=false){auto& f=fighters[id];if(!Live(id)||(attacker>=0&&attacker!=id&&TeamOf(attacker)==TeamOf(id))||(!melee&&f.hurtClock>0))return;f.hurtClock=.5f;f.invisTime=0;f.lastAttacker=attacker;f.lastHurt=clock;f.healClock=0;float armor=f.armor==2?.68f:f.armor==1?.8f:f.armor==3?.86f:.92f;damage*=armor*(1.f-std::min(4,f.protection)*.04f);float absorbed=std::min(f.absorption,damage);f.absorption-=absorbed;f.hp-=damage-absorbed;Knock(id,force,melee);if(f.hp<=0)Kill(id,attacker);}
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
void Explosion(Vector3 p,int owner,Block kind){for(int x=int(p.x)-3;x<=int(p.x)+3;++x)for(int y=int(p.y)-3;y<=int(p.y)+3;++y)for(int z=int(p.z)-3;z<=int(p.z)+3;++z)if(Inside(x,y,z)&&placed.count(Cell(x,y,z))&&At(x,y,z)!=Obsidian&&At(x,y,z)!=Glass&&(kind!=Fireball||At(x,y,z)!=EndStone)&&Vector3Distance(p,{x+.5f,y+.5f,z+.5f})<3){placed.erase(Cell(x,y,z));EditCell(x,y,z,Air);}for(int i=0;i<MaxPlayers;++i){float distance=Vector3Distance(fighters[i].p,p);if(Live(i)&&distance<5&&(i==owner||TeamOf(i)!=TeamOf(owner))){Vector3 d=Vector3Normalize(Vector3Subtract(fighters[i].p,p));if(kind==Fireball){float strength=std::clamp(1.f-distance/5.f,.25f,1.f);Knock(i,{d.x*(10+5*strength),6+3*strength,d.z*(10+5*strength)});if(i!=owner)Damage(i,(5-distance)*2,owner);}else Damage(i,(5-distance)*3,owner,{d.x*6,5,d.z*6});}}}
void SendState();
void StartSwing(int id){auto& f=fighters[id];if(f.swingAge<.18f)return;f.swingAge=0;auto b=Packet(SwingAnimation);b.Byte(id);wire.Broadcast(b.bytes);}
void ArmPose(const Fighter& f,float side,float swing){
    rlTranslatef(0,f.sneaking?.82f:.67f,f.sneaking?.16f:0);if(f.sneaking)rlRotatef(-28,1,0,0);
    rlTranslatef(side,.65f,0);if(f.sneaking)rlRotatef(12,1,0,0);
    if(side>0){rlRotatef(85*swing,1,0,0);rlRotatef(-18*swing,0,0,1);}
}
void Process(int id,Buffer& b){int kind=b.Byte();auto& f=fighters[id];
    if(kind==ChatMessage){std::string text=b.Text();text.erase(std::remove_if(text.begin(),text.end(),[](unsigned char c){return c<32||c>126;}),text.end());if(text.empty()||clock-f.lastChat<.75f)return;f.lastChat=clock;std::string line="<"+f.name+"> "+text.substr(0,60);AddChat(line);auto message=Packet(ChatMessage);message.Text(line);wire.Broadcast(message.bytes);return;}
    if(kind==ChooseTeam){int team=b.Byte();if(CanSelect(id,team)){f.team=team;Tell(id,std::string("Time: ")+teamNames[team]);SendState();}else Tell(id,"Time cheio ou escolha desequilibra a partida");return;}
    if(kind==StartRequest){if(id==0)StartCountdown();return;}if(kind==RestartRequest){if(id==0)ResetMatch();return;}
    if(kind==Position){Vector3 p=b.Vec();float yaw=b.Float(),pitch=b.Float();int selected=b.Byte();int crouch=b.Byte();if(crouch>1||selected>8||std::abs(p.x)>Width+16||std::abs(p.z)>Width+16||p.y>Height+20||p.y< -100)return;
        float elapsed=std::clamp(clock-f.lastPosition,.05f,.5f);float distance=std::hypot(p.x-f.p.x,p.z-f.p.z);
        if(distance<=14*elapsed+1.5f&&!f.eliminated&&f.respawn<=0){if(phase!=0||Vector3Distance(p,LobbyPosition(id))<14){
            if(phase==1){if(p.y<f.p.y)f.fallDistance+=f.p.y-p.y;else if(p.y>f.p.y+.001f)f.fallDistance=0;bool grounded=false;int y=int(std::floor(p.y-.02f));for(int x=int(std::floor(p.x-.29f));x<=int(std::floor(p.x+.29f));++x)for(int z=int(std::floor(p.z-.29f));z<=int(std::floor(p.z+.29f));++z){auto boxes=CollisionBoxesAt(x,y,z);for(int i=0;i<boxes.count;++i)if(std::abs(p.y-boxes.boxes[i].max.y)<.04f)grounded=true;}if(grounded){if(f.fallDistance>3)Damage(id,std::floor(f.fallDistance-3),-1);f.fallDistance=0;}}
            if(f.respawn<=0&&!f.eliminated)f.p=p;
        }}f.yaw=yaw;f.pitch=std::clamp(pitch,-1.55f,1.55f);f.selected=selected;f.sneaking=crouch!=0;f.lastPosition=clock;return;}
    if(kind==SwapRequest){int a=b.Byte(),target=b.Byte();bool chest=b.Byte()!=0;if(a>=63||target>=63||(!chest&&(a>=36||target>=36)))return;if(chest&&(phase!=1||!ChestReach(f.p,TeamOf(id))))return;
        auto get=[&](int slot)->ItemStack&{return slot<36?f.inventory[slot]:chests[TeamOf(id)][slot-36];};auto& from=get(a);auto& to=get(target);if(chest&&((a>=36||target>=36)&&(ToolLevel(from.block)||ToolLevel(to.block))))return;if(a!=target){if(from.block==to.block&&from.count&&to.count){int n=std::min(int(from.count),StackCapacity(to.block)-int(to.count));to.count+=n;from.count-=n;if(!from.count)from={};}else std::swap(from,to);++f.revision;}return;}
    if(!Live(id))return;
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
            if(!received&&!Give(f,item,offer.count)){f.inventory=previous;Tell(id,"Inventario cheio");return;}
            if(item==Stick)for(auto& stack:f.inventory)if(stack.block==Stick)stack.damage=1;
            if(level){if(IsAxe(item))f.axe=level;else f.pick=level;}
        }
        if(upgrade)for(auto& mate:fighters)if(mate.connected&&mate.team==f.team){mate.sharp=f.sharp;mate.protection=f.protection;mate.forge=f.forge;}
        Consume(f,offer.currency,offer.cost);Tell(id,"Compra realizada");return;
    }
    if(kind==DigRequest){int stage=b.Byte(),x=b.Int(),y=b.Int(),z=b.Int(),slot=b.Byte();if(slot>8)return;if(stage==1){f.digging={};return;}if(!Inside(x,y,z)||!Breakable(id,x,y,z))return;
        if(stage==0){f.digging={true,x,y,z};f.digStart=clock;return;}
        if(stage!=2||!f.digging.found||x!=f.digging.x||y!=f.digging.y||z!=f.digging.z||clock-f.digStart+.08f<MiningTime(At(x,y,z),f.inventory[slot].block))return;
        int bed=BedTeam(x,y,z);if(bed>=0){RemoveBed(bed);++f.beds;status=std::string("Cama destruida: ")+teamNames[bed];}
        else {Block block=At(x,y,z);placed.erase(Cell(x,y,z));EditCell(x,y,z,Air);Give(f,block,1);}f.digging={};return;}
    if(kind==PlaceRequest){int x=b.Int(),y=b.Int(),z=b.Int(),slot=b.Byte();if(slot>8||!Inside(x,y,z)||y<20||y>55||!CanReach(id,x,y,z)||At(x,y,z)!=Air)return;auto& stack=f.inventory[slot];if(!stack.count||!Placeable(stack.block)||Liquid(stack.block)||stack.block==Bed||stack.block==BedHead||stack.block==Tnt)return;
        for(const auto& generator:generators)if(Vector3Distance({x+.5f,y+.5f,z+.5f},generator.p)<2)return;
        for(int i=0;i<MaxPlayers;++i){auto p=fighters[i].p;if(Fighting(fighters[i])&&p.x+.3f>x&&p.x-.3f<x+1&&p.y+1.8f>y&&p.y<y+1&&p.z+.3f>z&&p.z-.3f<z+1)return;}
        Block block=stack.block;EditCell(x,y,z,block);placed.insert(Cell(x,y,z));if(!--stack.count)stack={};++f.revision;return;}
    if(kind==AttackRequest){int slot=b.Byte();if(slot>8)return;StartSwing(id);Vector3 eye=Vector3Add(f.p,{0,f.sneaking?1.54f:1.62f,0}),direction{std::sin(f.yaw)*std::cos(f.pitch),std::sin(f.pitch),-std::cos(f.yaw)*std::cos(f.pitch)};float closest=std::numeric_limits<float>::infinity();int victim=-1;auto block=Trace(eye,direction);if(block.found){auto h=GetRayCollisionBox({eye,direction},{{float(block.x),float(block.y),float(block.z)},{block.x+1.f,block.y+1.f,block.z+1.f}});if(h.hit)closest=std::min(closest,h.distance);}
        for(int i=0;i<MaxPlayers;++i)if(i!=id&&TeamOf(i)!=TeamOf(id)&&Live(i)){auto p=fighters[i].p;auto hit=GetRayCollisionBox({eye,direction},{{p.x-.3f,p.y,p.z-.3f},{p.x+.3f,p.y+1.8f,p.z+.3f}});if(hit.hit&&hit.distance<=3.3f&&hit.distance<closest){victim=i;closest=hit.distance;}}
        if(victim>=0){Block sword=f.inventory[slot].block;float damage=sword==DiamondSword?7:sword==IronSword?6:sword==StoneSword?5:sword==WoodenSword||sword==GoldenSword?4:1;Vector3 delta=Vector3Subtract(fighters[victim].p,f.p);Vector3 d=Vector3Normalize({delta.x,0,delta.z});float horizontal=sword==Stick&&f.inventory[slot].damage==1?13.6f:6.8f;Damage(victim,damage+f.sharp,id,{d.x*horizontal,6.f,d.z*horizontal},true);if(id==0)localSprintPause=true;else wire.Send(f.peer,Packet(SprintPause).bytes);SendState();}return;}
    if(kind==UseRequest){int slot=b.Byte();if(slot>8||clock-f.lastAction<.3f)return;auto& stack=f.inventory[slot];if(!stack.count)return;Block item=stack.block;Vector3 eye=Vector3Add(f.p,{0,1.4f,0}),direction{std::sin(f.yaw)*std::cos(f.pitch),std::sin(f.pitch),-std::cos(f.yaw)*std::cos(f.pitch)};
        if(item>=SpeedPotion&&item<=InvisibilityPotion){Consume(f,item,1);if(item==SpeedPotion)f.speedTime=45;if(item==JumpPotion)f.jumpTime=45;if(item==InvisibilityPotion)f.invisTime=30;}
        else if(item==GoldenApple){return;}
        else if(item==Fireball||item==EnderPearl||item==Tnt){Consume(f,item,1);shots.push_back({Vector3Add(eye,Vector3Scale(direction,.7f)),Vector3Scale(direction,item==EnderPearl?18.f:item==Fireball?12.f:2.f),id,item,item==Tnt?4.f:8.f});}
        else if(item==Bow&&Count(f,Arrow)){Consume(f,Arrow,1);shots.push_back({Vector3Add(eye,Vector3Scale(direction,.7f)),Vector3Scale(direction,28),id,Arrow,5});}
        f.lastAction=clock;
    }
}
void SendState(){auto b=Packet(Snapshot);b.Int(epoch);b.Byte(phase);b.Float(phaseClock);b.Int(winner);b.Float(clock);b.Byte(islandGap);b.Byte(mapId);b.Byte(teamCapacity);
    for(auto& f:fighters){b.Byte(f.connected);b.Byte(f.team);b.Text(f.name);b.Vec(f.p);b.Float(f.yaw);b.Float(f.pitch);b.Float(f.hp);b.Float(f.respawn);b.Byte(f.bed);b.Byte(f.eliminated);b.Int(f.teleport);b.Int(f.revision);b.Int(f.kills);b.Int(f.finals);b.Int(f.beds);b.Byte(f.armor);b.Byte(f.sharp);b.Byte(f.protection);b.Byte(f.forge);b.Byte(f.selected);b.Byte(f.sneaking);b.Byte(f.pick);b.Byte(f.axe);b.Float(f.speedTime);b.Float(f.jumpTime);b.Float(f.invisTime);b.Float(f.absorption);b.Float(f.regen);b.Float(f.eating);for(auto& s:f.inventory)Stack(b,s);}
    for(auto& chest:chests)for(auto& s:chest)Stack(b,s);
    b.Byte(int(generators.size()));for(auto& g:generators){b.Int(g.stock);b.Float(g.clock);}
    b.Byte(int(shots.size()));for(auto& s:shots){b.Vec(s.p);b.Byte(s.kind);}
    wire.Broadcast(b.bytes);
}
void Simulate(float dt){clock+=dt;
    if(phase==0){if(phaseClock>0&&PlayerCount()<2){phaseClock=0;Notice("Contagem cancelada: aguardando 2 jogadores.");}if(phaseClock>0){phaseClock=std::max(.0001f,phaseClock-dt);if(phaseClock<.001f)BeginRound();}if(phase!=0)return;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected&&(fighters[i].p.y<64||Vector3Distance(fighters[i].p,LobbyPosition(i))>14))Teleport(i,LobbyPosition(i));return;}
    phaseClock+=dt;if(phase==2){if(phaseClock>10)ResetMatch();return;}
    for(int i=0;i<MaxPlayers;++i){auto& f=fighters[i];f.hurtClock=std::max(0.f,f.hurtClock-dt);f.speedTime=std::max(0.f,f.speedTime-dt);f.jumpTime=std::max(0.f,f.jumpTime-dt);f.invisTime=std::max(0.f,f.invisTime-dt);f.absorbTime=std::max(0.f,f.absorbTime-dt);if(f.absorbTime==0)f.absorption=0;if(Live(i)){
            if(f.eatSlot>=0){auto& food=f.inventory[f.eatSlot];if(clock-f.eatHeartbeat>.3f||f.selected!=f.eatSlot||food.block!=GoldenApple||!food.count){f.eating=0;f.eatSlot=-1;}else{f.eating+=dt;if(f.eating+1e-5f>=1.61f){Consume(f,GoldenApple,1);f.absorption=4;f.absorbTime=120;f.regen=5;f.regenClock=f.healClock=f.eating=0;f.eatSlot=-1;}}}
            if(f.regen>0){float elapsed=std::min(dt,f.regen);f.regen=std::max(0.f,f.regen-dt);f.regenClock+=elapsed;while(f.regenClock>=1.f-1e-5f){f.regenClock=std::max(0.f,f.regenClock-1);f.hp=std::min(20.f,f.hp+1);}f.healClock=0;}
            else if(f.hp<20){f.healClock+=dt;while(f.healClock>=4){f.healClock-=4;f.hp=std::min(20.f,f.hp+2);}}else f.healClock=0;
        }if(f.respawn>0){f.respawn=std::max(0.f,f.respawn-dt);if(f.respawn==0){Loadout(i);Teleport(i,TeamSpawn(i));}}else if(Live(i)&&f.p.y< -6)Kill(i,-1);}
    for(size_t i=0;i<generators.size();++i){auto& g=generators[i];float interval=(g.item==Diamond||g.item==Emerald)?ResourceInterval(g.item,phaseClock):g.interval;if(g.team>=0)interval/=1+TeamForge(g.team);g.clock+=dt;while(g.clock>=interval){g.clock-=interval;g.stock=std::min(g.cap,g.stock+1);}for(int id=0;id<MaxPlayers;++id)if(g.stock&&Live(id)&&Vector3Distance(fighters[id].p,g.p)<2){if(Give(fighters[id],g.item,1))--g.stock;}}
    for(auto it=shots.begin();it!=shots.end();){auto& s=*it;s.life-=dt;if(s.kind!=Fireball)s.v.y-=dt*14;Vector3 next=Vector3Add(s.p,Vector3Scale(s.v,dt));bool collision=Solid(At(int(std::floor(next.x)),int(std::floor(next.y)),int(std::floor(next.z))));if(s.kind==Tnt&&collision){s.v={};next=s.p;collision=false;}
        int victim=-1;for(int i=0;i<MaxPlayers;++i)if(i!=s.owner&&TeamOf(i)!=TeamOf(s.owner)&&Live(i)&&Vector3Distance(Vector3Add(fighters[i].p,{0,.9f,0}),next)<.8f)victim=i;
        if(collision||victim>=0||s.life<=0||next.y< -10){if(s.kind==Tnt||s.kind==Fireball)Explosion(s.p,s.owner,s.kind);else if(s.kind==Arrow&&victim>=0)Damage(victim,4,s.owner,Vector3Scale(Vector3Normalize(s.v),3));else if(s.kind==EnderPearl&&collision&&Live(s.owner)){Vector3 destination=s.p;destination.y=std::ceil(destination.y);if(!Solid(At(int(destination.x),int(destination.y),int(destination.z)))&&!Solid(At(int(destination.x),int(destination.y+1),int(destination.z)))){Teleport(s.owner,destination);Damage(s.owner,4,-1);}}it=shots.erase(it);}else{s.p=next;++it;}}
    if(phaseClock>=600)for(int team=0;team<TeamLimit();++team)if(TeamBed(team)){SetTeamBed(team,false);RemoveBed(team);}
    if(phaseClock>=900)for(int i=0;i<MaxPlayers;++i)Damage(i,dt*2,-1);
    std::set<int> alive;for(const auto& f:fighters)if(f.connected&&!f.eliminated)alive.insert(f.team);
    if(participants>=2&&alive.size()<=1){winner=alive.empty()?-1:*alive.begin();phase=2;phaseClock=0;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected)Teleport(i,LobbyPosition(i));}
}
void Receive(int peer,Buffer& b){if(host){int kind=b.bytes.empty()?0:b.bytes[0];if(kind==Hello){b.Byte();if(b.Int()!=Version){wire.Drop(peer);return;}std::string name=b.Text();if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos||phase!=0){auto e=Packet(Error);e.Text("Aguarde o host voltar ao lobby; use nick de 3 a 16 letras/numeros.");wire.Send(peer,e.bytes);wire.Drop(peer);return;}if(peerTeam.count(peer))return;int id=-1;for(int i=1;i<MaxPlayers;++i)if(!fighters[i].connected){id=i;break;}if(id<0||PlayerCount()>=TeamLimit()*teamCapacity){wire.Drop(peer);return;}peerTeam[peer]=id;auto& f=fighters[id];f=Fighter{};f.name=name;f.peer=peer;f.team=AvailableTeam();f.connected=true;Give(f,WhiteWool,1);Teleport(id,LobbyPosition(id));auto w=Packet(Welcome);w.Int(Version);w.Byte(id);wire.Send(peer,w.bytes);SendState();}else if(peerTeam.count(peer))Process(peerTeam[peer],b);return;}
    int kind=b.Byte();if(kind==Welcome){if(b.Int()!=Version)throw std::runtime_error("Versao BedWars diferente do host");me=b.Byte();if(me>=MaxPlayers)throw std::runtime_error("Equipe invalida");status="Conectado ao BedWars LAN";return;}
    if(kind==Snapshot){int nextEpoch=b.Int();epoch=nextEpoch;phase=b.Byte();phaseClock=b.Float();winner=b.Int();clock=b.Float();int gap=b.Byte();int nextMap=b.Byte();teamCapacity=b.Byte();if(teamCapacity!=1&&teamCapacity!=2)throw std::runtime_error("Modo invalido");if(nextMap>=MapCount)throw std::runtime_error("Mapa invalido");if(gap!=16&&gap!=32&&gap!=64)throw std::runtime_error("Distancia Rush invalida");if(nextEpoch!=seenEpoch){mapId=nextMap;SetLayout(gap);GenerateArena();seenEpoch=nextEpoch;seenTeleport=seenRevision=-1;inventorySelection=-1;}for(auto& f:fighters){f.connected=b.Byte()!=0;f.team=b.Byte();if(f.team>=TeamLimit())throw std::runtime_error("Time invalido");f.name=b.Text();f.p=b.Vec();f.yaw=b.Float();f.pitch=b.Float();f.hp=b.Float();f.respawn=b.Float();f.bed=b.Byte()!=0;f.eliminated=b.Byte()!=0;f.teleport=b.Int();f.revision=b.Int();f.kills=b.Int();f.finals=b.Int();f.beds=b.Int();f.armor=b.Byte();f.sharp=b.Byte();f.protection=b.Byte();f.forge=b.Byte();f.selected=b.Byte();if(f.selected>8)throw std::runtime_error("Slot BedWars invalido");f.sneaking=b.Byte()!=0;f.pick=b.Byte();f.axe=b.Byte();f.speedTime=b.Float();f.jumpTime=b.Float();f.invisTime=b.Float();f.absorption=b.Float();f.regen=b.Float();f.eating=b.Float();for(auto& s:f.inventory)s=Stack(b);}for(auto& chest:chests)for(auto& s:chest)s=Stack(b);int count=b.Byte();if(count!=int(generators.size()))throw std::runtime_error("Geradores inconsistentes");for(auto& g:generators){g.stock=b.Int();g.clock=b.Float();}int n=b.Byte();shots.clear();for(int i=0;i<n;++i){auto p=b.Vec();int block=b.Byte();if(block>=BlockCount)throw std::runtime_error("Projetil invalido");shots.push_back({p,{},0,Block(block),0});}ready=true;return;}
    if(kind==Edit){int x=b.Int(),y=b.Int(),z=b.Int(),block=b.Byte();if(!Inside(x,y,z)||block>=BlockCount)throw std::runtime_error("Edicao invalida");Change(x,y,z,Block(block));if(block==Air)placed.erase(Cell(x,y,z));else placed.insert(Cell(x,y,z));return;}
    if(kind==SwingAnimation){int id=b.Byte();if(id>=MaxPlayers||!fighters[id].connected)return;fighters[id].swingAge=0;return;}
    if(kind==SprintPause){localSprintPause=true;return;}
    if(kind==ChatMessage){AddChat(b.Text());return;}
    if(kind==Force){auto force=b.Vec();bool melee=b.offset<b.bytes.size()&&b.Byte()!=0;if(melee)localMeleeForce=force;else localForce=Vector3Add(localForce,force);return;}if(kind==Error){status=b.Text();return;}
}
void Request(Buffer b){if(!active||!ready)return;if(host)Process(0,b);else wire.Send(0,b.bytes);}

Rectangle InventoryRect(int i,bool chest){float size=std::min(42.f,(GetScreenHeight()-120.f)/(chest?8:5));float x=GetScreenWidth()/2.f-size*4.5f,y=GetScreenHeight()/2.f-size*(chest?3.7f:2.f);int column,row;if(i<27){column=i%9;row=i/9;}else if(i<36){column=i-27;row=3;}else{column=(i-36)%9;row=(i-36)/9;}if(i<36&&chest)row+=4;return {x+column*size,y+row*size,size-3,size-3};}
int StorageIndex(int displayed){return displayed<27?displayed+9:displayed-27;}
}
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
Vector3 BasePosition(int team){return bases[std::clamp(team,0,3)];}
bool Host(const std::string& name,unsigned short port,int gap,int map,int capacity){Stop();if(capacity!=1&&capacity!=2){status="Modo invalido";return false;}teamCapacity=capacity;if(map<0||map>=MapCount){status="Mapa invalido";return false;}mapId=map;if(gap!=16&&gap!=32&&gap!=64){status="Distancia Rush invalida";return false;}SetLayout(gap);if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos){status="Nick: use 3 a 16 letras, numeros ou _";return false;}if(!wire.Host(port)){status=wire.Error();return false;}roomPort=port;host=active=ready=true;me=0;fighters={};fighters[0].connected=true;fighters[0].name=name.empty()?"Host":name;fighters[0].peer=0;seenEpoch=seenRevision=seenTeleport=-1;ResetMatch();UpdateDiscovery();return true;}
bool Join(const std::string& address,const std::string& name){Stop();if(name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos){status="Nick: use 3 a 16 letras, numeros ou _";return false;}if(!wire.Join(address)){status=wire.Error();return false;}active=true;host=ready=false;me=0;fighters={};fighters[0].name=name;status="Conectando ao BedWars LAN...";seenEpoch=seenRevision=seenTeleport=-1;return true;}
void Stop(){discovery.StopHosting();discovery.CancelSearch();wire.Stop();active=ready=host=false;shots.clear();peerTeam.clear();chatMessages.clear();localForce=localMeleeForce={};localSprintPause=false;inventorySelection=-1;}
void UpdateDiscovery(){if(host&&active&&ready)discovery.Advertise({{},fighters[0].name,roomPort,PlayerCount(),islandGap,phase==0,0,TeamLimit()*teamCapacity});discovery.Poll();}
void SearchRooms(){discovery.Search();}
bool SearchingRooms(){return discovery.Searching();}
const std::vector<LanRoom>& Rooms(){return discovery.Rooms();}
const std::string& DiscoveryStatus(){return discovery.Status();}
bool Active(){return active;}bool Ready(){return active&&ready;}bool Hosting(){return host;}bool Lobby(){return phase==0;}bool CanControl(){return Ready()&&fighters[me].respawn<=0;}
const std::vector<std::string>& Messages(){return chatMessages;}std::string Nickname(){return fighters[me].name;}
void Chat(const std::string& text){auto b=Packet(ChatMessage);b.Text(text.substr(0,60));Request(std::move(b));}const std::string& Status(){return status;}
void StartRound(){Request(Packet(StartRequest));}void Restart(){Request(Packet(RestartRequest));}
void Update(Player& player,float dt,bool playing,int selected){if(!active)return;dt=std::clamp(dt,0.f,.1f);for(auto& event:wire.Poll()){try{if(event.kind==LanTransport::Event::Connected&&!host){auto b=Packet(Hello);b.Int(Version);b.Text(fighters[0].name);wire.Send(0,b.bytes);}else if(event.kind==LanTransport::Event::Data){Buffer b{event.data};Receive(event.peer,b);}else if(event.kind==LanTransport::Event::Disconnected){if(host){auto it=peerTeam.find(event.peer);if(it!=peerTeam.end()){auto& f=fighters[it->second];f.connected=false;f.eliminated=true;f.bed=false;peerTeam.erase(it);if(phase==0)BalanceTeams();}}else{status=wire.Error();Stop();return;}}}catch(const std::exception& error){status=error.what();if(host)wire.Drop(event.peer);else {Stop();return;}}}
    if(!wire.Alive()){status=wire.Error();Stop();return;}if(!ready)return;
    auto& own=fighters[me];if(own.teleport!=seenTeleport){PlayerState pose;pose.position=own.p;pose.yaw=own.yaw;pose.pitch=own.pitch;pose.valid=true;player.RestoreState(pose);ResetVitals();seenTeleport=own.teleport;}
    if(own.revision!=seenRevision){for(int i=0;i<9;++i)Hotbar[i]=own.inventory[i];for(int i=0;i<27;++i)Backpack[i]=own.inventory[9+i];CursorStack={};seenRevision=own.revision;}
    EquippedArmor={};EquippedArmor[0]={LeatherHelmet,1};EquippedArmor[1]={LeatherChestplate,1};EquippedArmor[2]={own.armor==2?DiamondLeggings:own.armor==1?IronLeggings:own.armor==3?ChainmailLeggings:LeatherLeggings,1};EquippedArmor[3]={own.armor==2?DiamondBoots:own.armor==1?IronBoots:own.armor==3?ChainmailBoots:LeatherBoots,1};
    Health=own.hp>0?own.hp:20;Absorption=own.absorption;Mode=own.eliminated?GameMode::Creative:GameMode::Survival;FallDistance=0;
    if(localSprintPause){player.SuspendSprint(.1f);localSprintPause=false;}
    if(Vector3Length(localMeleeForce)>.001f){player.MeleeKnockback(localMeleeForce);localMeleeForce={};}
    if(Vector3Length(localForce)>.001f){player.Knockback(localForce);localForce={};}
    if(own.respawn<=0&&!own.eliminated){positionClock+=dt;if(positionClock>=.05f){positionClock=0;auto pose=player.CaptureState();auto b=Packet(Position);b.Vec(pose.position);b.Float(pose.yaw);b.Float(pose.pitch);b.Byte(selected);b.Byte(player.IsSneaking());Request(std::move(b));}}
    if(host)Simulate(dt);
    if(host){snapshotClock+=dt;if(snapshotClock>=.1f){snapshotClock=0;SendState();}}
    for(auto& f:fighters){f.display=Vector3Lerp(f.display,f.p,std::min(1.f,dt*14));f.swingAge=std::min(.35f,f.swingAge+dt);}
    (void)playing;
}
bool CanMine(const Hit& hit){return Ready()&&hit.found&&Inside(hit.x,hit.y,hit.z)&&Breakable(me,hit.x,hit.y,hit.z);}
void Dig(const Hit& hit,int stage,int selected){if(!hit.found)return;auto b=Packet(DigRequest);b.Byte(stage);b.Int(hit.x);b.Int(hit.y);b.Int(hit.z);b.Byte(selected);Request(std::move(b));}
void Place(int x,int y,int z,int selected){auto b=Packet(PlaceRequest);b.Int(x);b.Int(y);b.Int(z);b.Byte(selected);Request(std::move(b));}
void Attack(int selected,const Player* player){if(player){auto pose=player->CaptureState();auto position=Packet(Position);position.Vec(pose.position);position.Float(pose.yaw);position.Float(pose.pitch);position.Byte(selected);position.Byte(player->IsSneaking());Request(std::move(position));}auto b=Packet(AttackRequest);b.Byte(selected);Request(std::move(b));}
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
void DrawWorld(const Camera3D& camera,Texture2D atlas){if(!Ready())return;BeginMode3D(camera);for(int i=0;i<MaxPlayers;++i){if(i!=me&&fighters[i].connected&&!fighters[i].eliminated&&fighters[i].respawn<=0&&fighters[i].invisTime<=0){auto p=fighters[i].display;auto& f=fighters[i];Color metal=f.armor==2?Color{87,224,224,255}:f.armor==1?Color{207,211,217,255}:f.armor==3?Color{125,131,139,255}:colors[f.team];
        rlPushMatrix();
        rlTranslatef(p.x,p.y,p.z);
        rlRotatef(-f.yaw*RAD2DEG,0,1,0);
        const float hipY=f.sneaking?.82f:.67f,hipZ=f.sneaking?.16f:0;
        float swing=std::sin(std::clamp(f.swingAge/.35f,0.f,1.f)*PI);
        rlPushMatrix();rlTranslatef(0,hipY,hipZ);if(f.sneaking)rlRotatef(-28,1,0,0);
        DrawCube({0,.36f,0},.62f,.72f,.36f,colors[TeamOf(i)]);
        rlPopMatrix();
        for(float side:{-.43f,.43f}){
            rlPushMatrix();ArmPose(f,side,swing);DrawCube({0,-.32f,0},.22f,.64f,.26f,colors[TeamOf(i)]);rlPopMatrix();
        }
        for(float side:{-.17f,.17f}){
            rlPushMatrix();rlTranslatef(side,hipY,hipZ);if(f.sneaking)rlRotatef(13,1,0,0);
            DrawCube({0,-.32f,0},.25f,.64f,.3f,metal);rlPopMatrix();
            DrawCube({side,.12f,f.sneaking?.02f:-.04f},.28f,.24f,.38f,metal);
        }
        rlPushMatrix();
        rlTranslatef(0,f.sneaking?1.63f:1.64f,f.sneaking?-.21f:0);
        rlRotatef(f.pitch*RAD2DEG,1,0,0);
        DrawCube({0,0,0},.52f,.52f,.52f,colors[TeamOf(i)]);
        DrawCube({0,-.02f,-.27f},.42f,.26f,.02f,BEIGE);
        for(float side:{-.11f,.11f})DrawCube({side,.02f,-.285f},.07f,.05f,.015f,{45,39,34,255});
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
            unsigned char light=static_cast<unsigned char>(brightness*255);DrawHeldItem3D(atlas,held,transform,{light,light,light,255});
        }
    }}

    for(int i=0;i<TeamLimit();++i)for(int kind=0;kind<2;++kind){
        auto p=Vendor(i,kind);Color robe=kind?Color{91,62,111,255}:Color{106,73,48,255};Color skin{177,136,103,255};
        DrawCube(Vector3Add(p,{0,.8f,0}),.64f,1.05f,.44f,robe);
        for(float side:{-.17f,.17f})DrawCube(Vector3Add(p,{side,.2f,0}),.23f,.4f,.3f,{65,48,38,255});
        DrawCube(Vector3Add(p,{0,1.59f,0}),.57f,.65f,.55f,skin);
        DrawCube(Vector3Add(p,{0,1.44f,.36f}),.16f,.3f,.23f,skin);
        DrawCube(Vector3Add(p,{0,1.73f,.282f}),.46f,.07f,.025f,{58,44,30,255});
        for(float side:{-.15f,.15f})DrawCube(Vector3Add(p,{side,1.64f,.285f}),.09f,.07f,.025f,DARKGREEN);
        DrawCube(Vector3Add(p,{0,1.02f,.3f}),.76f,.23f,.23f,robe);
        DrawCube(Vector3Add(p,{0,1.02f,.43f}),.24f,.17f,.08f,skin);
    }
    for(auto& g:generators)if(g.stock){DrawCube(Vector3Add(g.p,{0,.4f+std::sin(float(GetTime())*2)*.08f,0}),.35f,.2f,.35f,g.item==IronIngot?LIGHTGRAY:g.item==GoldIngot?GOLD:g.item==Diamond?SKYBLUE:GREEN);}
    for(auto& shot:shots){if(shot.kind==Tnt)DrawCube(shot.p,.6f,.6f,.6f,RED);else DrawSphere(shot.p,.16f,shot.kind==Fireball?ORANGE:shot.kind==EnderPearl?DARKGREEN:BEIGE);}EndMode3D();
    auto label=[&](Vector3 p,const std::string& text,Color color){Vector3 direction=Vector3Subtract(camera.target,camera.position);if(Vector3DotProduct(direction,Vector3Subtract(p,camera.position))<=0||Vector3Distance(camera.position,p)>36)return;Vector2 screen=GetWorldToScreen(p,camera);int width=MeasureText(text.c_str(),16);DrawRectangle(int(screen.x-width/2-3),int(screen.y-2),width+6,20,Fade(BLACK,.6f));DrawText(text.c_str(),int(screen.x-width/2),int(screen.y),16,color);};
    for(const auto& g:Generators())if((g.item==Diamond||g.item==Emerald)&&Vector3Distance(camera.position,g.position)<=5.f){std::string text=std::string(g.item==Diamond?"DIAMANTE":"ESMERALDA")+" | Nivel "+std::to_string(g.level)+" | "+std::to_string(int(std::ceil(g.remaining)))+"s";label(Vector3Add(g.position,{0,2.7f,0}),text,g.item==Diamond?SKYBLUE:GREEN);}
    for(int i=0;i<TeamLimit();++i)for(int kind=0;kind<2;++kind)label(Vector3Add(Vendor(i,kind),{0,2.2f,0}),kind?"MELHORIAS":"LOJA DE ITENS",kind?GREEN:GOLD);
    for(int i=0;i<MaxPlayers;++i)if(i!=me&&fighters[i].connected&&!fighters[i].sneaking&&fighters[i].invisTime<=0)label(Vector3Add(fighters[i].display,{0,2,0}),fighters[i].name,colors[TeamOf(i)]);
}
void DrawHud(){if(!Ready())return;int x=GetScreenWidth()-220,y=70;DrawRectangle(x-8,y-8,216,178,Fade(BLACK,.6f));DrawText("BEDWARS LAN",x,y,21,GOLD);DrawText(TextFormat("%02d:%02d",int(phaseClock)/60,int(phaseClock)%60),x,y+28,17,WHITE);
    for(int team=0;team<TeamLimit();++team)DrawText(TextFormat("%s %s %d/%d%s",TeamBed(team)?"[C]":"[ ]",teamNames[team],TeamCount(team),teamCapacity,team==TeamOf(me)?" <":""),x,y+52+team*24,17,TeamCount(team)?colors[team]:GRAY);
    const auto& f=fighters[me];if(f.eating>0){int x=GetScreenWidth()/2-70,y=GetScreenHeight()/2+36;DrawRectangle(x,y,140,8,DARKGRAY);DrawRectangle(x,y,int(140*std::min(1.f,f.eating/1.61f)),8,GOLD);}
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
    DrawText("Clique para comprar",int(panel.x+12),int(panel.y+36+3.7f*s),14,DARKGRAY);
    int visible=0;
    const auto& own=fighters[me];
    for(const auto& o:offers){if(!ShowOffer(o,own))continue;auto rect=ShopCell(visible%9,1.45f+visible/9);++visible;Block icon=o.item;if(icon==Air)icon=o.special==1?IronBoots:o.special==2?DiamondBoots:o.special==6?ChainmailBoots:o.special==3?GoldenSword:o.special==4?IronChestplate:IronIngot;ShopIcon(atlas,rect,icon,o.count);bool owned=(o.special==3&&own.sharp)||(o.special==4&&own.protection>=4)||(o.special==5&&own.forge>=2)||((o.special==1||o.special==2||o.special==6)&&ArmorRank(own.armor)>=ArmorRank(o.special==1?1:o.special==2?2:3));
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
    int visible=0;for(int i=0;i<int(std::size(offers));++i){if(!ShowOffer(offers[i],fighters[me]))continue;auto rect=ShopCell(visible%9,1.45f+visible/9);++visible;if(CheckCollisionPointRec(GetMousePosition(),rect)){Buy(i);return;}}
}
void DrawInventory(Texture2D atlas,bool chest){DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,.8f));auto first=InventoryRect(0,chest);DrawText(chest?"BAU DA EQUIPE":"INVENTARIO - 27 + 9",int(first.x),int(InventoryRect(chest?36:0,chest).y-28),20,WHITE);
    for(int i=0;i<(chest?63:36);++i){auto rect=InventoryRect(i,chest);int index=i<36?StorageIndex(i):i;ItemStack stack=index<36?fighters[me].inventory[index]:chests[TeamOf(me)][index-36];DrawRectangleRec(rect,index==inventorySelection?GOLD:GRAY);if(stack.count){int tile=BlockTile(stack.block);DrawTexturePro(atlas,{float(tile*TileSize),0,TileSize,TileSize},{rect.x+5,rect.y+3,rect.width-10,rect.height-10},{0,0},0,WHITE);DrawText(TextFormat("%d",stack.count),int(rect.x+rect.width-17),int(rect.y+rect.height-15),14,WHITE);if(CheckCollisionPointRec(GetMousePosition(),rect))DrawText(Names[stack.block],int(first.x),int(InventoryRect(35,chest).y+55),17,WHITE);}}
}
void SwapSlots(int from,int to,bool chest){if(from<0||to<0||from>=63||to>=63)return;auto b=Packet(SwapRequest);b.Byte(from);b.Byte(to);b.Byte(chest);Request(std::move(b));}
void InventoryInput(bool chest){if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))for(int i=0;i<(chest?63:36);++i)if(CheckCollisionPointRec(GetMousePosition(),InventoryRect(i,chest))){int slot=i<36?StorageIndex(i):i;if(inventorySelection<0)inventorySelection=slot;else{auto b=Packet(SwapRequest);b.Byte(inventorySelection);b.Byte(slot);b.Byte(chest);Request(std::move(b));inventorySelection=-1;}}}
}
