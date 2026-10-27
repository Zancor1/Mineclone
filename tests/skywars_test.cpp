#include "bedwars.h"
#include "skywars.h"
#include "survival.h"
#include <raymath.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <set>
#include <sstream>
#include <map>
namespace {
void Check(bool b,const char* text){if(!b){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
struct Packet {std::vector<unsigned char> data;void B(int n){data.push_back(n);}void I(int n){for(int i=0;i<4;++i)B(uint32_t(n)>>(8*i));}void F(float f){int n;std::memcpy(&n,&f,4);I(n);}void S(const char* s){B(std::strlen(s));while(*s)B(*s++);}};
}
int main(){using namespace game;
 struct Sandbox{std::filesystem::path p;~Sandbox(){std::error_code e;std::filesystem::remove_all(p,e);}} sandbox{std::filesystem::temp_directory_path()/("mineclone-skywars-"+std::to_string(getpid()))};progression::Directory=sandbox.p/"profiles";
 for(unsigned seed=0;seed<32;++seed)for(int refill=0;refill<4;++refill){std::set<std::string> contents;
 for(int i=0;i<sky::ChestCount;++i){auto a=sky::Loot(seed,i,refill),b=sky::Loot(seed,i,refill);int occupied=0;bool blocks=false,throwable=false,strong=false;std::map<int,int> counts;
 for(int j=0;j<27;++j){Check(a[j].block==b[j].block&&a[j].count==b[j].count,"loot is deterministic for synchronization");if(a[j].count){++occupied;Check(a[j].count<=StackCapacity(a[j].block),"loot respects stack limits");blocks|=a[j].block==Stone||a[j].block==Planks||a[j].block==Cobblestone;throwable|=a[j].block==Egg||a[j].block==Snowball;strong|=a[j].block==DiamondSword;counts[int(a[j].block)]+=a[j].count;}}
 Check(occupied>=5&&blocks&&throwable,"every chest has usable blocks and projectiles");Check(strong==(i>=24),"diamond weapons reserved for stronger central loot");std::ostringstream signature;for(auto item:counts)signature<<item.first<<':'<<item.second<<',';Check(contents.insert(signature.str()).second,"all chest contents differ regardless of slot order");}}
 int fd=socket(AF_INET,SOCK_STREAM,0);sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);Check(bind(fd,(sockaddr*)&addr,sizeof(addr))==0,"reserve port");socklen_t len=sizeof(addr);getsockname(fd,(sockaddr*)&addr,&len);int port=ntohs(addr.sin_port);close(fd);
 Check(bw::Host("Host",port,32,bw::SkywarsMap),"host SkyWars");Check(bw::Generators().empty(),"SkyWars has no BedWars generators");
 for(auto p:sky::Chests())Check(At(int(p.x),int(p.y),int(p.z))==Chest,"all 28 loot chests exist");
 for(auto p:sky::Bases())Check(At(int(p.x)-3,32,int(p.z)-3)!=CraftingTable&&At(int(p.x)-4,32,int(p.z)-3)!=Furnace,"no crafting table or furnace in SkyWars islands");
 for(auto p:sky::Bases())Check(Solid(At(int(p.x),31,int(p.z)))&&At(int(p.x),32,int(p.z))==Air,"eight safe island spawns");
 LanTransport guest;Check(guest.Join("127.0.0.1:"+std::to_string(port)),"guest connects");Player player;bool round=false,victory=false,knocked=false;std::string roomIdentity;std::array<ChestInventory,sky::ChestCount> remoteChests{};
 auto pump=[&](int frames){for(int i=0;i<frames;++i){for(auto event:guest.Poll()){if(event.kind==LanTransport::Event::Connected){Packet p;p.B(1);p.I(LanProtocolVersion);p.S("Guest");guest.Send(0,p.data);}if(event.kind==LanTransport::Event::Data&&!event.data.empty()&&event.data[0]==14)knocked=true;if(event.kind==LanTransport::Event::Data&&event.data.size()>2&&event.data[0]==35){roomIdentity.assign(event.data.begin()+2,event.data.begin()+2+event.data[1]);}
 if(event.kind==LanTransport::Event::Data&&event.data.size()>14&&event.data[0]==3){
 size_t offset=21;auto skip=[&](size_t n){Check(n<=event.data.size()-offset,"snapshot field fits");offset+=n;};
 for(int f=0;f<16;++f){skip(2);int name=event.data.at(offset);skip(1+name);skip(27+28+22+8+28+45*6);}
 for(auto& chest:remoteChests)for(auto& stack:chest){Check(offset+6<=event.data.size(),"shared chest data fits snapshot");stack={Block(event.data[offset]),event.data[offset+1]};skip(6);}
round|=event.data[5]==1;victory|=event.data[5]==2;}}bw::Update(player,.05f,true,0);std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
 pump(30);Check(bw::PlayerList().size()==2,"two participants in lobby");bw::StartRound();pump(106);Check(round&&!bw::Lobby(),"host starts SkyWars");
 for(auto stack:Hotbar)Check(!stack.count,"SkyWars starts with an empty hotbar");
 for(auto stack:Backpack)Check(!stack.count,"SkyWars starts with an empty backpack");
 for(auto stack:EquippedArmor)Check(!stack.count,"SkyWars starts without armor");
 auto chest=sky::Chests()[0];PlayerState pose;pose.position={chest.x,32,chest.z+2};pose.pitch=-.55f;pose.valid=true;auto origin=player.CaptureState().position;auto destination=pose.position;for(int i=1;i<=6;++i){pose.position=Vector3Lerp(origin,destination,i/6.f);player.RestoreState(pose);pump(1);}
 Check(bw::NearChest(player),"aimed SkyWars chest can be opened");
 Check(!roomIdentity.empty(),"room identity received");auto loot=sky::Loot(sky::LootSeed(roomIdentity,1),0);int throwSlot=-1;for(int i=0;i<27;++i)if(loot[i].block==Snowball||loot[i].block==Egg)throwSlot=i;
 Check(throwSlot>=0,"throwable in chest loot");bw::InventoryClick(36+throwSlot,false,true,true);pump(4);
 int invSlot=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==Egg||Hotbar[i].block==Snowball)invSlot=i;
 Check(invSlot>=0&&Hotbar[invSlot].count==loot[throwSlot].count,"shift-click chest loot reaches inventory");
 bw::InventoryClick(36+throwSlot,false,true,true);pump(4);Check(Hotbar[invSlot].count==loot[throwSlot].count,"looted chest slot cannot duplicate items");
 Check(!remoteChests[0][throwSlot].count,"item removed by host disappears in guest snapshot");
 bw::SwapSlots(invSlot,40,true,true);pump(4);Check(remoteChests[0][4].count>0,"deposit is visible to guest");
 bw::InventoryClick(40,false,true,true);pump(4);Check(!remoteChests[0][4].count,"chest slot 4 is not mistaken for armor and removal reaches guest");
 invSlot=-1;for(int i=0;i<9;++i)if(Hotbar[i].count&&(Hotbar[i].block==Egg||Hotbar[i].block==Snowball))invSlot=i;Check(invSlot>=0,"throwables preserved across shared chest transfers");
 auto targetPosition=sky::Bases()[bw::PlayerTeam(1)];auto delta=Vector3Subtract(targetPosition,pose.position);float horizontal=std::hypot(delta.x,delta.z),speed=30,gravity=14,dy=-.5f;
 pose.yaw=std::atan2(delta.x,-delta.z);pose.pitch=std::atan((speed*speed-std::sqrt(speed*speed*speed*speed-gravity*(gravity*horizontal*horizontal+2*dy*speed*speed)))/(gravity*horizontal));player.RestoreState(pose);pump(1);
 int before=Hotbar[invSlot].count;bw::Use(invSlot);pump(1);Check(Hotbar[invSlot].count==before-1,"throwing consumes one egg or snowball");pump(38);Check(knocked,"egg or snowball applies knockback to another player on its island");
 // A controlled fall uses the guest's existing horizontal position.
 auto p=sky::Bases()[bw::PlayerTeam(1)];Packet fall;fall.B(5);fall.F(p.x);fall.F(-7);fall.F(p.z);fall.F(0);fall.F(0);fall.B(0);fall.B(0);guest.Send(0,fall.data);pump(10);
 auto players=bw::PlayerList();Check(players.size()==2&&players[1].eliminated,"fall into void eliminates without respawn");Check(victory,"last surviving team wins");Check(bw::Progress().xp==35,"winner receives 25 XP plus 10 XP for projectile-assisted kill");
 bw::Stop();guest.Stop();std::puts("PASS: SkyWars loot, eight safe islands, shared mode startup, void elimination and victory XP");
}
