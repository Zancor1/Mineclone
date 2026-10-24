#include "bedwars.h"
#include "cosmetics.h"
#include "map_blocks.h"
#include "lan_transport.h"
#include "survival.h"
#include "renderer.h"
#include "ui.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <rlgl.h>
#include <raymath.h>
namespace {
void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
struct W{std::vector<unsigned char> bytes;W(){bytes.reserve(256);}void B(int v){bytes.push_back(v);}void I(int n){for(int i=0;i<4;++i)B(uint32_t(n)>>(i*8));}void F(float f){int n;std::memcpy(&n,&f,4);I(n);}void S(const char* s){B(std::strlen(s));while(*s)B(*s++);}void V(Vector3 p){F(p.x);F(p.y);F(p.z);}};
int Count(game::Block item){int total=0;for(auto s:game::Hotbar)if(s.block==item)total+=s.count;for(auto s:game::Backpack)if(s.block==item)total+=s.count;return total;}
}
int main(int argc,char**){using namespace game;
    struct ProfileSandbox {std::filesystem::path directory;~ProfileSandbox(){std::error_code e;std::filesystem::remove_all(directory,e);}} sandbox{std::filesystem::temp_directory_path()/("mineclone-bw-profiles-"+std::to_string(getpid()))};
    progression::Directory=sandbox.directory;
    progression::Profile initial;initial.Award(50);std::string profileError;
    Check(progression::Save("Host",initial,profileError),"seed host level and crate");
    progression::Profile friendProfile;friendProfile.Award(45);Check(progression::Save("Friend",friendProfile,profileError),"seed friend close to level two");
    Check(bw::ResourceInterval(Diamond,0)==25&&bw::ResourceInterval(Emerald,0)==35,"initial resource generator periods");
    Check(bw::ResourceLevel(Diamond,169.99f)==1&&bw::ResourceLevel(Diamond,170)==2&&bw::ResourceInterval(Diamond,170)==20&&bw::ResourceInterval(Emerald,170)==30,"generator tier increases every two minutes fifty seconds");
    Check(bw::ResourceInterval(Diamond,340)==15&&bw::ResourceInterval(Emerald,340)==25&&bw::ResourceInterval(Diamond,5000)==5&&bw::ResourceInterval(Emerald,5000)==5,"five-second reduction per tier and safe minimum period");
    int probe=socket(AF_INET,SOCK_STREAM,0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);Check(bind(probe,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,"ephemeral LAN port");socklen_t length=sizeof(address);getsockname(probe,reinterpret_cast<sockaddr*>(&address),&length);unsigned short port=ntohs(address.sin_port);close(probe);
    {
        Check(bw::StartTester("Host")&&bw::Testing()&&!bw::Lobby(),"Tester opens immediately without another player");
        Player training;
        auto tick=[&](int count){for(int i=0;i<count;++i)bw::Update(training,.05f,true,0);};tick(2);
        Check(bw::Generators().size()==1&&bw::Generators()[0].interval<.1f,"Tester has one very fast generator");
        const Block defense[]={Air,Planks,EndStone,Obsidian};
        for(int i=0;i<4;++i){int x=100+i*18;Check(At(x,26,144)==Bed&&At(x,26,145)==BedHead,"Tester has four independent beds");Check(At(x,27,144)==defense[i],"training bed cover matches bare, wood, end stone and obsidian");}
        Check(Count(IronSword)==1&&Count(IronPickaxe)==1&&Count(Tnt)==64,"Tester provides a usable equipment kit");
        int foodSlot=-1;for(int slot=0;slot<27;++slot)if(Backpack[slot].block==GoldenApple)foodSlot=slot+9;
        Check(foodSlot>=0,"Tester provides golden apples");bw::SwapSlots(foodSlot,4,false,true);tick(1);
        auto fallPose=training.CaptureState();fallPose.position.y+=10;training.RestoreState(fallPose);tick(1);fallPose.position.y-=10;training.RestoreState(fallPose);tick(2);
        Check(Health<16,"fall damage prepares a wounded player for the regeneration test");
        for(int frame=0;frame<193;++frame){bw::HoldUse(4,true);bw::Update(training,1.f/120,true,4);}
        Check(Count(GoldenApple)==64&&Absorption==0,"high FPS apple consumption waits for 1.61 seconds");
        for(int frame=0;frame<4;++frame){bw::HoldUse(4,true);bw::Update(training,1.f/120,true,4);}
        bw::HoldUse(4,false);tick(1);
        Check(Count(GoldenApple)==63&&Absorption==4,"apple consumes after switching slots at 120 FPS");
        float beforeRegen=Health;for(int frame=0;frame<120;++frame)bw::Update(training,1.f/120,true,4);
        Check(std::abs(Health-beforeRegen-2)<.01f,"golden apple heals one full heart in one second while wounded");
        bw::Restart();tick(2);

        int tntSlot=-1;for(int slot=0;slot<9;++slot)if(Hotbar[slot].block==Tnt)tntSlot=slot;
        Check(tntSlot>=0,"Tester TNT is available for placement");
        bw::Place(129,26,104,tntSlot,&training);tick(1);auto primed=bw::TntStates();
        Check(primed.size()==1&&Vector3Distance(primed[0].position,{129.5f,26.5f,104.5f})<.02f&&Count(Tnt)==63,"TNT appears centered on selected block and consumes one item");
        if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(720,480,"Placed TNT timer");{Renderer renderer;Check(renderer.Initialize(),"TNT timer renderer");Camera3D camera{{132,29,102},{129.5f,26.5f,104.5f},{0,1,0},55,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(camera,{});bw::DrawWorld(camera,renderer.Atlas());rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,"build/tnt-timer.png");UnloadImage(image);EndDrawing();}CloseWindow();}
        auto initialTntPosition=primed[0].position;tick(96);primed=bw::TntStates();
        Check(primed.size()==1&&primed[0].remaining>0&&Vector3Distance(primed[0].position,initialTntPosition)<.03f,"placed TNT stays on its support until the five-second fuse ends");
        float beforeTntHealth=Health;tick(4);Check(bw::TntStates().empty()&&Health==beforeTntHealth,"TNT explodes after five seconds without direct player damage");
        training.Update({},.05f);Check(training.CaptureState().velocityY>0,"TNT still knocks back its owner");
        bw::Restart();tick(2);
        auto walk=[&](Vector3 destination,float yaw){auto from=training.CaptureState();for(int step=1;step<=40;++step){auto pose=from;pose.position=Vector3Lerp(from.position,destination,step/40.f);pose.yaw=yaw;training.RestoreState(pose);tick(1);}};
        bw::Drop(4,false,&training);tick(2);Check(Count(WhiteWool)==63&&bw::DroppedItems().size()==1&&bw::DroppedItems()[0].stack.count==1,"BedWars Q removes one item from the authoritative inventory");
        bw::Drop(4,true,&training);tick(2);Check(Count(WhiteWool)==0&&bw::DroppedItems().size()==2&&bw::DroppedItems()[1].stack.count==63,"BedWars Ctrl Q drops the selected stack");
        tick(30);auto pickup=bw::DroppedItems()[0].position;pickup.y=26.001f;walk(pickup,0);tick(6);
        Check(Count(WhiteWool)==64&&bw::DroppedItems().empty(),"BedWars pickup restores the complete dropped stack");
        bw::InventoryClick(4,false);tick(2);Check(Count(WhiteWool)==0,"taking a stack into the cursor removes it from its slot");
        bw::DropCursor(false);tick(2);Check(bw::DroppedItems().size()==1&&bw::DroppedItems()[0].stack.count==1,"right click outside drops one cursor item");
        bw::DropCursor();tick(2);Check(bw::DroppedItems().size()==2&&bw::DroppedItems()[1].stack.count==63,"left click outside drops the remaining cursor stack");
        bw::DropCursor();bw::InventoryClick(4,false);tick(2);Check(bw::DroppedItems().size()==2&&Count(WhiteWool)==0,"cursor drop clears the cursor without duplicating items");
        bw::Restart();tick(2);

        walk({128.5f,26.001f,109.5f},0);tick(20);Check(Count(IronIngot)>0&&Count(GoldIngot)>0&&Count(Diamond)>0&&Count(Emerald)>0,"single training generator supplies all four resources rapidly");
        walk({121.5f,26.001f,117.f},PI);auto beforeFireball=bw::TrainingDummies()[0];
        bw::Use(8);tick(6);auto afterFireball=bw::TrainingDummies()[0];
        Check(afterFireball.health==beforeFireball.health&&Vector3Distance(afterFireball.position,beforeFireball.position)>.1f,"fireball knocks back an enemy without dealing damage");
        bw::Restart();tick(2);
        int pearlSlot=-1;for(int slot=0;slot<27;++slot)if(Backpack[slot].block==EnderPearl)pearlSlot=slot+9;
        Check(pearlSlot>=0,"Tester provides pearls for player collision tests");bw::SwapSlots(pearlSlot,8,false,true);tick(2);
        walk({121.5f,26.001f,115.f},PI);auto pearlTarget=bw::TrainingDummies()[0];auto pearlStart=training.CaptureState().position;
        bw::Use(8);tick(8);auto afterPearl=bw::TrainingDummies()[0];
        Check(std::abs(afterPearl.health-19.f)<.001f,"pearl hit deals exactly half a heart even with armor");
        Check(Vector3Distance(afterPearl.position,pearlTarget.position)>.2f,"pearl hit knocks back the passive dummy");
        Check(Vector3Distance(training.CaptureState().position,pearlStart)>2.f&&Vector3Distance(training.CaptureState().position,pearlTarget.position)<2.f,"pearl hit teleports its thrower to the target");
        bw::Restart();tick(2);
        auto dummies=bw::TrainingDummies();Check(dummies.size()==2&&dummies[0].health==20&&dummies[1].health==20&&!dummies[1].fighting,"both training dummies start healthy and inactive");
        walk({121.5f,26.001f,117.f},PI);bw::Attack(0,&training);tick(3);auto hitDummy=bw::TrainingDummies();Check(hitDummy[0].health<20&&Vector3Distance(hitDummy[0].position,dummies[0].position)>.1f,"passive dummy receives real damage and knockback");Check(!hitDummy[1].fighting,"other dummy does not aggro without being hit");
        walk({135.5f,26.001f,117.f},PI);bw::Attack(0,&training);tick(3);Check(bw::TrainingDummies()[1].fighting,"fighter dummy wakes when struck");tick(55);Check(Health<20,"fighter dummy pursues and lands melee hits");
        Check(bw::Progress().xp==50,"training combat never awards progression XP");
        bw::Restart();tick(2);Check(bw::Testing()&&bw::TrainingDummies()[0].health==20&&!bw::TrainingDummies()[1].fighting&&At(118,27,144)==Planks,"Tester restart restores dummies and defenses");
        if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"Tester arena");{Renderer renderer;Check(renderer.Initialize(),"Tester renderer");Camera3D camera{{145,50,91},{129,26,130},{0,1,0},65,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(camera,{});bw::DrawWorld(camera,renderer.Atlas());bw::DrawHud();rlDrawRenderBatchActive();Image capture=LoadImageFromScreen();ExportImage(capture,"build/tester-arena.png");UnloadImage(capture);EndDrawing();}CloseWindow();}
        bw::Stop();Check(!bw::Testing(),"leaving Tester clears training state");
    }
    {
        Player lobbyPlayer;std::array<LanTransport,15> guests;
        auto tick=[&](int frames){for(int n=0;n<frames;++n){
            for(int i=0;i<15;++i)for(auto& event:guests[i].Poll())if(event.kind==LanTransport::Event::Connected){W hello;hello.B(1);hello.I(20);auto name="Guest"+std::to_string(i);hello.S(name.c_str());guests[i].Send(0,hello.bytes);}
            bw::Update(lobbyPlayer,.05f,true,0);std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }};
        auto connect=[&](int guest){Check(guests[guest].Join("127.0.0.1:"+std::to_string(port)),"team guest connects");tick(20);};
        auto choose=[&](int guest,int team){W request;request.B(17);request.B(team);guests[guest].Send(0,request.bytes);tick(5);};
        Check(bw::Host("Host",port,16,0,1),"Solo host");tick(2);
        Check(bw::Progress().Level()==2&&bw::Progress().crates==1&&bw::Progress().pending==0,"host receives pending crate on lobby entry");bw::OpenCrate();auto unlocked=bw::Progress();int type=-1,color=-1;for(int t=0;t<3;++t)for(int c=1;c<=progression::SkinCount;++c)if(unlocked.unlocked[t]&(1u<<c)){type=t;color=c;}Check(unlocked.crates==0&&type>=0,"crate unlocks one tool finish");bw::OpenCrate();Check(bw::Progress().unlocked==unlocked.unlocked,"empty crate request cannot grant skins");bw::EquipSkin(type,color);Check(bw::Progress().skins[type]==color,"equip owned finish");bw::EquipSkin((type+1)%3,color);Check(bw::Progress().skins[(type+1)%3]==0,"cannot equip locked finish for another tool type");bw::NextSkin(type);Check(bw::Progress().skins[type]==0,"can return to default tool appearance");Check(Count(WhiteWool)==1,"host receives lobby team selector");
        connect(0);Check(bw::PlayerTeam(1)==1,"new Solo player uses separate team");bw::SelectTeam(1);Check(bw::LocalTeam()==0,"Solo cannot select occupied team");
        for(int i=1;i<8;++i)connect(i);
        Check(bw::PlayerTeam(7)==7&&bw::PlayerTeam(8)==-1,"Solo accepts eight players and rejects a ninth");
        for(auto& guest:guests)guest.Stop();
        tick(5);bw::Stop();
        Check(bw::Host("Host",port,16,0,2,"Arena dos amigos!"),"Dupla host");connect(0);bw::SelectTeam(1);
        Check(bw::TeamCapacity()==2&&bw::LocalTeam()==0,"two-player Dupla cannot put everyone on one team");
        connect(1);connect(2);bw::SelectTeam(1);choose(1,3);
        Check(bw::LocalTeam()==1&&bw::PlayerTeam(1)==1&&bw::PlayerTeam(2)==3&&bw::PlayerTeam(3)==3,"four players can select balanced pairs");
        choose(1,1);Check(bw::PlayerTeam(2)==3,"host rejects request for full team");
        guests[2].Stop();tick(10);Check(bw::PlayerTeam(2)==3,"two-versus-one remains balanced");
        guests[1].Stop();tick(10);Check(bw::PlayerTeam(0)!=bw::PlayerTeam(1),"disconnect rebalances remaining two players onto different teams");
        connect(1);connect(2);for(int i=3;i<15;++i)connect(i);
        for(int team=0;team<8;++team){auto choices=bw::TeamChoices();Check(choices[team].players==2,"Dupla supports sixteen players with two per team");}
        bw::SearchRooms();for(int i=0;i<30;++i){bw::UpdateDiscovery();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        bool found=false;for(const auto& room:bw::Rooms())if(room.port==port){Check(room.players==16&&room.capacity==16&&!room.joinable,"Dupla discovery advertises sixteen-player capacity");Check(room.name=="Arena dos amigos!"&&room.playerNames[0]=="Host","custom server title is independent of host nickname");found=true;}Check(found,"Dupla room discovered");
        bw::StartRound();tick(110);Check(!bw::Lobby(),"balanced Dupla starts");
        int own=bw::LocalTeam();auto base=bw::BasePosition(own);Check(std::abs(lobbyPlayer.CaptureState().position.x-base.x)<2,"Dupla spawns at selected team base");
        int mate=-1;for(int id=1;id<16;++id)if(bw::PlayerTeam(id)==own)mate=id;Check(mate>0,"host has a teammate");
        W aim;aim.B(5);aim.V({base.x,26.001f,base.z+1});aim.F(0);aim.F(0);aim.B(0);aim.B(0);guests[mate-1].Send(0,aim.bytes);tick(5);
        float hp=Health;W strike;strike.B(8);strike.B(0);guests[mate-1].Send(0,strike.bytes);tick(5);Check(Health==hp,"teammate cannot damage host");
        W dig;dig.B(6);dig.B(0);dig.I(int(base.x));dig.I(26);dig.I(int(base.z)+3);dig.B(0);guests[mate-1].Send(0,dig.bytes);tick(40);dig.bytes[1]=2;guests[mate-1].Send(0,dig.bytes);tick(5);Check(At(int(base.x),26,int(base.z)+3)==Bed,"teammate cannot mine own team bed");
        for(auto& guest:guests)guest.Stop();
        bw::Stop();
    }
    std::array<uint64_t,3> silhouettes{};
    for(int map=0;map<3;++map)for(int gap:{16,32,64}){
        Check(bw::Host("Host",port,gap,map),"host cardinal Rush layout");auto bases=bw::ArenaBases(gap,map);
        if(gap==32){uint64_t signature=1469598103934665603ull;for(int dx=-13;dx<=13;++dx)for(int dz=-13;dz<=13;++dz){signature^=At(int(bases[0].x)+dx,25,int(bases[0].z)+dz)!=Air;signature*=1099511628211ull;}silhouettes[map]=signature;}
        Check(bases[0].x==128.5f&&bases[0].z<128&&bases[1].x>128.5f&&bases[1].z==128.5f&&bases[2].z>128.5f&&bases[3].x<128,"bases occupy north east south west");
        for(auto base:bases){auto beds=bw::MapBeds(map,0,base);Check(At(int(base.x),25,int(base.z))!=Air&&At(int(beds[0].x),26,int(beds[0].z))==Bed,"every base has supported bed");}
        if(map==1)for(int team=0;team<8;++team)for(int kind=0;kind<2;++kind){
            auto v=bw::RuinsVendor(team,kind,gap);int x=int(v.x),z=int(v.z);
            Check(At(x,25,z)==LampOn&&At(x,29,z)==CarvedStone,"Ruins vendors have floor lighting and a niche roof");
            BoundingBox body{{v.x-.4f,v.y,v.z-.4f},{v.x+.4f,v.y+1.95f,v.z+.4f}};
            for(int y=26;y<=28;++y){auto boxes=CollisionBoxesAt(x,y,z);for(int i=0;i<boxes.count;++i)Check(!CheckCollisionBoxes(body,boxes.boxes[i]),"vendor body fits inside Ruins niche");}
            for(int dz=-2;dz<=0;++dz)for(int y=26;y<=28;++y)Check(At(x,y,z+dz)==Air,"Ruins niche has an open approach from the base");
            Vector3 eye{v.x,v.y+1.62f,v.z-3};auto npc=GetRayCollisionBox({eye,{0,0,1}},body);auto wall=Trace(eye,{0,0,1});
            Check(npc.hit&&(!wall.found||wall.z>v.z),"shop can be targeted before the niche back wall");
        }
        if(argc>1&&map==1&&gap==16){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1100,720,"Ruins shop niches");{Renderer renderer;Check(renderer.Initialize(),"Ruins vendor renderer");Camera3D view{{bases[0].x,31,bases[0].z-3},{bases[0].x,27,bases[0].z+5},{0,1,0},70,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(view,{});bw::DrawWorld(view,renderer.Atlas());rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,"build/ruins-vendor-niches.png");UnloadImage(image);EndDrawing();}CloseWindow();}
        for(const auto& generator:bw::Generators())for(int dx=-1;dx<=1;++dx)for(int dz=-1;dz<=1;++dz){int x=int(generator.position.x)+dx,y=int(std::floor(generator.position.y)),z=int(generator.position.z)+dz;auto state=MapStateAt(x,y,z);Check(state.shape==MapShape::Slab&&!(state.data&8),"all generators have nine lower slabs");auto boxes=CollisionBoxesAt(x,y,z);Check(boxes.count==1&&std::abs(boxes.boxes[0].max.y-generator.position.y)<.001f,"generator pickup height matches lower slab surface");}
        constexpr int ring[]={0,4,1,5,2,6,3,7};auto diamonds=bw::DiamondIslands(gap,map);for(int i=0;i<8;++i){auto p=diamonds[i];Check(MapStateAt(int(p.x),25,int(p.z)).material==BlockOfDiamond,"diamond generator is supported between adjacent bases");auto a=bases[ring[i]],b=bases[ring[(i+1)%8]];Check(std::abs(Vector3Distance(p,a)-Vector3Distance(p,b))<2,"diamond access balanced between neighboring bases");}
        Check(MapStateAt(128,25,128).material==BlockOfEmerald&&bw::Generators().size()==25,"central emerald and four diamonds registered");
        Check(Vector3Distance(bases[0],bases[1])-24>=gap+5,"base separation retains clear Rush void");bw::Stop();
    }
    Check(silhouettes[0]!=silhouettes[1]&&silhouettes[1]!=silhouettes[2]&&silhouettes[0]!=silhouettes[2],"three arenas have distinct island footprints");
    Check(bw::Host("Host",port,16,3),"host imported NC arena");Check(bw::TeamChoices().size()==2&&bw::SelectedMap()==3,"NC uses two original teams");
    Check(At(86,26,156)==Bed&&At(85,26,156)==BedHead&&At(170,26,156)==Bed&&At(171,26,156)==BedHead,"NC restores both original beds with X orientation");
    Check(At(75,25,156)==Sandstone&&At(181,25,156)==Sandstone,"NC original base geometry imported");
    int shaped=0,stairs=0,upper=0;for(int x=0;x<Width;++x)for(int z=0;z<Width;++z)for(int y=0;y<Height;++y)if(At(x,y,z)>=MapSlab){++shaped;auto state=MapStateAt(x,y,z);stairs+=state.shape==MapShape::Stairs;upper+=state.shape==MapShape::Slab&&(state.data&8);auto boxes=MapShapeBoxes(x,y,z);Check(boxes.count>0&&boxes.count<=12,"imported shape has bounded geometry");}
    Check(shaped>2000&&stairs>600&&upper>300,"NC includes slabs, inverted slabs, stairs and decorations");Check(bw::Generators().size()==6,"NC original generator positions integrated");
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1100,720,"NC imported map");{Renderer renderer;Check(renderer.Initialize(),"NC renderer initialized");Camera3D view{{208,90,225},{128,27,150},{0,1,0},70,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(view,{});bw::DrawWorld(view,renderer.Atlas());rlDrawRenderBatchActive();Image preview=LoadImageFromScreen();ExportImage(preview,"build/nc-arena.png");UnloadImage(preview);EndDrawing();auto thumbnail=bw::CreateMapPreview(3,16);Check(thumbnail.id!=0,"NC selection preview");UnloadRenderTexture(thumbnail);}CloseWindow();}
    Change(230,20,230,MapSlab);SetMapBlockState(230,20,230,{MapSlab,StoneBricks,MapShape::Slab,8});
    auto slab=CollisionBoxesAt(230,20,230);Check(slab.count==1&&slab.boxes[0].min.y==20.5f,"upper slab preserves collision height");
    Check(!Trace({230.5f,20.25f,229},{0,0,1}).found&&Trace({230.5f,20.75f,229},{0,0,1}).found,"raycast passes through empty half of upper slab");
    bw::Stop();
    Check(bw::Host("Host",port,16,4),"host imported Airshow");Check(bw::TeamChoices().size()==8&&bw::Generators().size()==24,"Airshow has eight teams and original resource islands");
    for(const auto& g:bw::Generators())if(g.item==IronIngot||g.item==GoldIngot){int x=int(g.position.x),y=int(std::ceil(g.position.y))-1,z=int(g.position.z);Check(MapStateAt(x,y,z).shape==MapShape::Slab,"Airshow forge uses an original slab");auto boxes=CollisionBoxesAt(x,y,z);Check(boxes.count==1&&std::abs(boxes.boxes[0].max.y-g.position.y)<.001f,"Airshow drops sit on original slab height");}
    bool greenArea=false;for(const auto& g:bw::Generators())if(g.item==IronIngot&&std::abs(g.position.x-88.5f)<.01f&&std::abs(g.position.z-42.f)<.01f&&std::abs(g.position.y-42.5f)<.01f)greenArea=true;Check(greenArea,"Airshow red forge occupies the six original slabs at the back of the shop");
    int highBlocks=0,barriers=0;for(int x=0;x<Width;++x)for(int z=0;z<Width;++z)for(int y=0;y<Height;++y){if(At(x,y,z)!=Air&&y>=80)++highBlocks;if(HasMapShape(x,y,z)&&MapStateAt(x,y,z).shape==MapShape::Barrier){++barriers;Check(CollisionBoxesAt(x,y,z).count==1,"invisible barriers retain collision");}}
    Check(highBlocks>1000&&barriers==416,"Airshow retains balloon tops and all original barriers");
    for(int t=0;t<8;++t){auto beds=bw::MapBeds(4,t,{});Check(At(int(beds[0].x),int(beds[0].y),int(beds[0].z))==Bed&&At(int(beds[1].x),int(beds[1].y),int(beds[1].z))==BedHead,"all eight original Airshow beds integrated");for(int k=0;k<2;++k){auto v=bw::AirshowVendor(t,k);Check(!CollisionBoxesAt(int(v.x),int(v.y),int(v.z)).count&&CollisionBoxesAt(int(v.x),int(v.y)-1,int(v.z)).count,"Airshow villager stands on original floor");}auto p=bw::AirshowSpawn(t);Check(!CollisionBoxesAt(int(p.x),int(p.y),int(p.z)).count&&CollisionBoxesAt(int(p.x),int(p.y)-1,int(p.z)).count,"Airshow spawn is clear and supported");}
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1100,720,"Airshow imported map");{Renderer renderer;Check(renderer.Initialize(),"Airshow renderer initialized");Camera3D view{{252,145,252},{128,48,128},{0,1,0},72,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(view,{});bw::DrawWorld(view,renderer.Atlas());rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,"build/airshow-arena.png");UnloadImage(image);EndDrawing();auto preview=bw::CreateMapPreview(4,16);Check(preview.id!=0,"Airshow map preview");UnloadRenderTexture(preview);}CloseWindow();}bw::Stop();
    Check(bw::Host("Host",port),"host BedWars LAN");Check(bw::PenguinStates().empty(),"new session has no escaping penguins");auto home=bw::BasePosition(0),enemy=bw::BasePosition(1);int hx=int(home.x),hz=int(home.z);Player player;bw::StartRound();for(int i=0;i<120;++i)bw::Update(player,.05f,true,0);Check(bw::Lobby(),"cannot start with only one player");LanTransport guest;Check(guest.Join("127.0.0.1:"+std::to_string(port)),"guest TCP connect");bool welcomed=false,edited=false,won=false,forceReceived=false,sprintPaused=false,swingReceived=false;bool dropReplicated=false;bool arrowReplicated=false;bool penguinSeen=false,penguinEnded=false;int hitEffects=0,lastHitVictim=-1,lastHitAttacker=-1;int snapshots=0,countdownMask=0,drawBowSlot=-1;bool eatingApple=false;int appleSlot=0;
    auto pump=[&](int frames){for(int i=0;i<frames;++i){for(auto& event:guest.Poll()){if(event.kind==LanTransport::Event::Connected){W hello;hello.B(1);hello.I(20);hello.S("Friend");guest.Send(0,hello.bytes);}if(event.kind==LanTransport::Event::Data){if(event.data[0]==2)welcomed=event.data.size()==6&&event.data[5]==1;if(event.data[0]==4)edited=true;if(event.data[0]==14)forceReceived=true;if(event.data[0]==18)sprintPaused=true;if(event.data[0]==20)swingReceived=true;if(event.data[0]==30&&event.data.size()>1&&event.data[1]>0)dropReplicated=true;if(event.data[0]==27){if(event.data.size()>1&&event.data[1]>0)penguinSeen=true;else if(penguinSeen)penguinEnded=true;}if(event.data[0]==26){Check(event.data.size()==3,"hit feedback packet contains only victim and attacker");++hitEffects;lastHitVictim=event.data[1];lastHitAttacker=event.data[2];}if(event.data[0]==3){if(event.data.size()>=28){auto end=event.data.data()+event.data.size();if(end[-28]==1&&end[-15]==Arrow&&end[-2]==1){Vector3 direction;std::memcpy(&direction,end-14,12);if(Vector3Length(direction)>1)arrowReplicated=true;}}++snapshots;if(event.data.size()>10&&event.data[5]==0){float remaining;std::memcpy(&remaining,event.data.data()+6,4);int second=int(std::ceil(remaining));if(second>=1&&second<=5)countdownMask|=1<<second;}if(event.data.size()>14&&event.data[5]==2&&event.data[10]==1)won=true;}}}bw::HoldUse(appleSlot,eatingApple);bw::Update(player,.05f,true,eatingApple?appleSlot:drawBowSlot>=0?drawBowSlot:0);std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
    pump(30);Check(welcomed&&snapshots>0,"LAN protocol welcome, team assignment and state snapshots");Check(bw::Lobby()&&player.CaptureState().position.y>65,"host always starts in elevated lobby");
    bw::SearchRooms();for(int i=0;i<30;++i){bw::UpdateDiscovery();std::this_thread::sleep_for(std::chrono::milliseconds(1));}bool foundHost=false;for(const auto& room:bw::Rooms())if(room.name=="Host"&&room.port==port){Check(room.players==2&&room.gap==64&&room.joinable,"advertised host state matches current lobby");foundHost=true;}Check(foundHost,"BedWars host automatically answers network search");
    Check(At(hx,26,hz+3)==Bed&&At(int(enemy.x),26,int(enemy.z)+3)==Bed&&MapStateAt(128,25,128).material==BlockOfEmerald,"four beds and center generator exist");
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"BedWars LAN preview");{Renderer renderer;Check(renderer.Initialize(),"BedWars renderer");UI ui;for(auto state:{GameState::BedwarsMode,GameState::BedwarsTeams}){BeginDrawing();ui.Draw(renderer.Atlas(),state);EndDrawing();}
        ui.Update(GameState::BedwarsMaps);BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsMaps);rlDrawRenderBatchActive();Image selection=LoadImageFromScreen();ExportImage(selection,"build/bedwars-map-selection.png");UnloadImage(selection);EndDrawing();BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsDiscovery);rlDrawRenderBatchActive();Image browser=LoadImageFromScreen();ExportImage(browser,"build/bedwars-discovery.png");UnloadImage(browser);EndDrawing();BeginDrawing();ui.Draw(renderer.Atlas(),GameState::BedwarsRush);rlDrawRenderBatchActive();Image menu=LoadImageFromScreen();ExportImage(menu,"build/bedwars-menu.png");UnloadImage(menu);EndDrawing();Camera3D camera{{128,78,161},{128,25,128},{0,1,0},75,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(camera,{});bw::DrawWorld(camera,renderer.Atlas());bw::DrawHud();rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,"build/bedwars-lobby.png");UnloadImage(image);EndDrawing();}CloseWindow();}
    Check(bw::PlayerList().size()==2,"LAN tab roster contains host and guest");
    W greeting;greeting.B(16);greeting.S("oi host");guest.Send(0,greeting.bytes);pump(4);Check(!bw::Messages().empty()&&bw::Messages().back()=="<Friend> oi host","LAN chat preserves nickname");
    W unauthorized;unauthorized.B(12);guest.Send(0,unauthorized.bytes);pump(110);Check(bw::Lobby()&&countdownMask==0,"guest cannot start the countdown");
    bw::StartRound();pump(20);guest.Stop();pump(10);Check(bw::Lobby(),"disconnect cancels countdown before round begins");pump(110);Check(bw::Lobby(),"cancelled countdown never starts a solo round");
    Check(guest.Join("127.0.0.1:"+std::to_string(port)),"guest reconnects after cancelled countdown");pump(30);countdownMask=0;
    bw::StartRound();pump(110);Check(countdownMask==62,"guest receives each countdown second from five to one");Check(!bw::Lobby()&&Vector3Distance(player.CaptureState().position,home)<5&&player.CaptureState().position.y<30,"countdown sends player to own island");
    auto initialGenerators=bw::Generators();Check(initialGenerators.size()==25,"all resource generators available");Check(initialGenerators[0].interval==35&&initialGenerators[1].interval==2.5f&&initialGenerators[2].interval==8&&initialGenerators[17].interval==25,"live generator periods match the new economy");for(const auto& generator:initialGenerators)Check(generator.remaining>=0&&generator.remaining<=generator.interval,"countdown remains within generator period");
    swingReceived=false;bw::Attack(0,&player);pump(3);Check(swingReceived,"missed attack still broadcasts arm swing");Check(hitEffects==0,"missed attacks do not trigger damage flash or hit sound");
    auto savedPose=player.CaptureState();
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(960,640,"BedWars shops");}
    for(int kind=0;kind<2;++kind){auto view=savedPose;view.position={home.x+(kind?4:-4),26.001f,home.z+(kind?-1:1)};view.yaw=kind?PI:0;view.pitch=0;player.RestoreState(view);Check(bw::AimedShop(player)&&bw::NearShop(player),kind?"upgrade villager opens own shop":"item villager opens own shop");
        if(argc>1){Renderer renderer;Check(renderer.Initialize(),"shop preview renderer");BeginDrawing();renderer.Draw(player.Camera(),{});bw::DrawWorld(player.Camera(),renderer.Atlas());rlDrawRenderBatchActive();Image npc=LoadImageFromScreen();ExportImage(npc,kind?"build/bedwars-upgrade-villager.png":"build/bedwars-item-villager.png");UnloadImage(npc);EndDrawing();BeginDrawing();bw::DrawShop(renderer.Atlas());rlDrawRenderBatchActive();Image shop=LoadImageFromScreen();ExportImage(shop,kind?"build/bedwars-upgrades.png":"build/bedwars-items.png");UnloadImage(shop);EndDrawing();}}
    if(argc>1)CloseWindow();
    player.RestoreState(savedPose);
    auto pose=player.CaptureState();pose.position={home.x,26.001f,home.z-3};pose.valid=true;player.RestoreState(pose);pump(500);Check(Count(IronIngot)>=8&&Count(GoldIngot)>=1,"base generator produces collectible resources");
    int iron=Count(IronIngot);bw::Buy(0);pump(1);Check(Count(RedWool)==16&&Count(IronIngot)>=iron-4,"shop charges currency and grants team wool");
    bw::SwapSlots(0,8,false,true);pump(2);Check(Hotbar[8].block==WoodenSword&&Hotbar[0].block!=WoodenSword,"number-key swap moves held item to hovered hotbar slot");bw::SwapSlots(8,0,false,true);pump(2);
    int woolSlot=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==RedWool)woolSlot=i;Check(woolSlot>=0,"shop fills hotbar");
    bw::InventoryClick(woolSlot,false,false,true);pump(2);Check(!Hotbar[woolSlot].count&&Backpack[0].block==RedWool&&Backpack[0].count==16&&!CursorStack.count,"shift click transfers hotbar stack to backpack without cursor");
    bw::InventoryClick(9,false,false,true);pump(2);Check(Count(RedWool)==16&&!Backpack[0].count,"shift click returns backpack stack to hotbar");
    for(int i=0;i<9;++i)if(Hotbar[i].block==RedWool)woolSlot=i;
    bw::Drop(woolSlot,false,&player);pump(3);Check(Count(RedWool)==15&&bw::DroppedItems().size()==1&&dropReplicated,"host drop is replicated to the connected guest");
    pump(30);auto pickupDrop=bw::DroppedItems()[0].position;auto returnPose=player.CaptureState();auto pickupPose=returnPose;pickupPose.position={pickupDrop.x,26.001f,pickupDrop.z};for(int step=1;step<=20;++step){auto moving=returnPose;moving.position=Vector3Lerp(returnPose.position,pickupPose.position,step/20.f);player.RestoreState(moving);pump(1);}pump(3);
    Check(Count(RedWool)==16&&bw::DroppedItems().empty(),"host can pick up a replicated drop without duplication");for(int step=1;step<=20;++step){auto moving=returnPose;moving.position=Vector3Lerp(pickupPose.position,returnPose.position,step/20.f);player.RestoreState(moving);pump(1);}pump(3);
    bw::Place(hx+2,26,hz+1,woolSlot);pump(4);Check(At(hx+2,26,hz+1)==RedWool&&Count(RedWool)==15&&edited,"placement consumes item and broadcasts world edit");
    Hit placed{true,hx+2,26,hz+1,0,1,0};bw::Dig(placed,0,0);pump(26);bw::Dig(placed,2,0);pump(2);Check(At(hx+2,26,hz+1)==Air&&Count(RedWool)==16,"mining returns placed block");Check(!bw::CanMine(placed),"air cannot be mined after block removal");
    Hit protectedBlock{true,hx,25,hz,0,1,0};bw::Dig(protectedBlock,0,0);pump(60);bw::Dig(protectedBlock,2,0);pump(2);Check(At(hx,25,hz)==RedWool,"arena terrain is protected");
    bw::InventoryClick(woolSlot,false,true,true);pump(1);Check(Count(RedWool)==0,"shift deposit removes stack from player inventory");bw::InventoryClick(36,false,true,true);pump(1);Check(Count(RedWool)==16&&!CursorStack.count,"shift withdrawal restores exactly one stack without cursor");
    for(int i=0;i<9;++i)if(Hotbar[i].block==RedWool)woolSlot=i;
    pump(8000);bw::Buy(15);pump(2);bw::Buy(16);pump(2);int arrowBow=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==Bow)arrowBow=i;Check(arrowBow>=0,"buy bow for LAN embedded arrow test");auto arrowPose=player.CaptureState();auto downward=arrowPose;downward.pitch=-1.55f;player.RestoreState(downward);pump(3);drawBowSlot=arrowBow;int bowAmmo=Count(Arrow);bw::BowInput(arrowBow,true,true,false,.05f,&player);pump(1);Check(bw::ArrowStates().empty()&&Count(Arrow)==bowAmmo,"LAN bow press does not fire or consume ammunition");bw::BowInput(arrowBow,false,false,true,.01f,&player);pump(8);drawBowSlot=-1;auto stuck=bw::ArrowStates();Check(stuck.size()==1&&stuck[0].embedded&&arrowReplicated,"LAN snapshot carries embedded arrow and its orientation");
    auto chargedShot=[&](int frames){drawBowSlot=arrowBow;for(int i=0;i<frames;++i){bw::BowInput(arrowBow,true,i==0,false,.05f,&player);pump(1);}bw::BowInput(arrowBow,false,false,true,.01f,&player);pump(8);drawBowSlot=-1;return bw::ArrowStates().back();};
    auto mediumArrow=chargedShot(10);Check(mediumArrow.damage==2&&!mediumArrow.critical,"host measures medium draw and applies one-heart damage");auto fullArrow=chargedShot(18);Check(fullArrow.damage==4&&!fullArrow.critical,"nine-tenths draw is full power without critical chance");auto criticalArrow=chargedShot(22);Check(criticalArrow.damage==4||criticalArrow.damage==6,"one-second charge enables bounded critical damage");Check(criticalArrow.critical==(criticalArrow.damage==6),"critical visual matches authoritative damage");drawBowSlot=arrowBow;int beforeCancel=Count(Arrow);auto beforeShots=bw::ArrowStates().size();bw::BowInput(arrowBow,true,true,false,.05f,&player);pump(2);bw::CancelBow();pump(2);drawBowSlot=-1;Check(Count(Arrow)==beforeCancel&&bw::ArrowStates().size()==beforeShots,"cancelled LAN draw consumes no ammunition and fires nothing");player.RestoreState(arrowPose);for(int i=0;i<9;++i){if(Hotbar[i].block==Bow)bw::SwapSlots(i,32,false,true);if(Hotbar[i].block==Arrow)bw::SwapSlots(i,33,false,true);}pump(2);
    bw::Buy(5);pump(2);Check(Hotbar[0].block==StoneSword&&Count(WoodenSword)==0&&Count(StoneSword)==1,"sword purchase replaces held starter sword");bw::SwapSlots(woolSlot,36,false,true);pump(2);Check(Count(RedWool)==0,"crafting grid removes ingredients from inventory");bw::SwapSlots(36,woolSlot,false,true);pump(2);Check(Count(RedWool)==16,"craft grid returns exact stack without duplication");bw::Buy(10);pump(1);Check(Count(IronPickaxe)==0,"cannot skip wooden and stone pickaxe tiers");
    for(int offer:{8,9,10,25,26,27}){bw::Buy(offer);pump(1);}
    Check(Count(IronPickaxe)==1&&Count(IronAxe)==1&&Count(WoodenPickaxe)==0&&Count(StonePickaxe)==0&&Count(WoodenAxe)==0&&Count(StoneAxe)==0,"independent tool upgrades replace previous tools");
    bw::Buy(28);pump(1);Check(EquippedArmor[2].block==ChainmailLeggings&&EquippedArmor[3].block==ChainmailBoots,"chain purchase equips leggings and boots");bw::Buy(18);pump(1);Check(EquippedArmor[0].block==LeatherHelmet&&EquippedArmor[1].block==LeatherChestplate&&EquippedArmor[2].block==IronLeggings&&EquippedArmor[3].block==IronBoots,"armor upgrades only replace leggings and boots");
    int pickSlot=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==IronPickaxe)pickSlot=i;Check(pickSlot>=0,"upgraded pickaxe exists");bw::SwapSlots(pickSlot,36,true);pump(1);Check(Count(IronPickaxe)==1,"persistent tools cannot be duplicated through chest storage");
    auto resourcePose=player.CaptureState();
    auto travel=[&](Vector3 target){auto start=player.CaptureState();for(int step=1;step<=70;++step){auto state=start;float t=step/70.f;state.position={start.position.x+(target.x-start.position.x)*t,26.001f,start.position.z+(target.z-start.position.z)*t};player.RestoreState(state);pump(1);}};
    travel({128.5f,26.001f,128.5f});pump(12);Check(Count(Emerald)>=4,"center generator supplies potion resources");travel(resourcePose.position);
    auto drink=[&](int offer,Block potion){bw::Buy(offer);pump(2);int slot=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==potion)slot=i;Check(slot>=0,"potion purchased into hotbar");bw::Use(slot);pump(8);Check(Count(potion)==0,"drinking consumes potion");};
    drink(29,SpeedPotion);drink(30,JumpPotion);drink(31,InvisibilityPotion);PlayerInput effects;bw::ApplyEffects(effects);Check(effects.speedMultiplier>1.3f&&effects.jumpMultiplier>1.4f,"drinking applies speed and jump effects");pump(910);effects={};bw::ApplyEffects(effects);Check(effects.speedMultiplier==1&&effects.jumpMultiplier==1,"potion effects expire");
    travel({enemy.x-4,26.001f,enemy.z+1});auto foreignView=player.CaptureState();foreignView.yaw=0;foreignView.pitch=0;player.RestoreState(foreignView);Check(bw::AimedShop(player)&&bw::NearShop(player),"enemy villager shop opens for visiting player");bw::Buy(23);pump(2);Check(Count(Stick)==1,"enemy shop sells knockback stick");bool enchanted=false;for(auto stack:Hotbar)if(stack.block==Stick&&stack.damage==1)enchanted=true;Check(enchanted,"knockback enchantment preserved in purchased stick");travel(resourcePose.position);
    for(int i=0;i<9;++i)if(Hotbar[i].block==Diamond){bw::SwapSlots(i,35,false,true);pump(2);}
    bw::Buy(13);pump(2);int fireballSlot=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==Fireball)fireballSlot=i;Check(fireballSlot>=0,"buy fireball for jump test");
    auto jumpPose=player.CaptureState();pose=jumpPose;pose.pitch=-1.55f;player.RestoreState(pose);pump(2);float jumpHealth=Health;bw::Use(fireballSlot);pump(12);Check(Health==jumpHealth,"own fireball does not cause self damage");player.Update({},.05f);Check(player.CaptureState().position.y>jumpPose.position.y+.2f,"fireball supplies strong upward knockback");player.RestoreState(jumpPose);pump(2);
    pose=player.CaptureState();pose.position.y=-8;player.RestoreState(pose);pump(4);Check(player.CaptureState().position.y>65,"void death with bed enters respawn waiting area");pump(110);Check(player.CaptureState().position.y<30&&At(hx,26,hz+3)==Bed,"bed permits respawn on own island");Check(Count(StonePickaxe)==1&&Count(StoneAxe)==1&&Count(IronPickaxe)==0&&Count(IronAxe)==0,"death downgrades both tools by one tier");Check(EquippedArmor[2].block==IronLeggings,"armor purchase persists through death");
    bw::Buy(7);pump(1);Check(Count(DiamondSword)==0,"cannot buy without emeralds");
    for(int step=0;step<=140;++step){W position;position.B(5);float t=step/140.f;position.V({enemy.x+(home.x-enemy.x)*t,26.001f,enemy.z+(home.z-1.5f-enemy.z)*t});position.F(0);position.F(0);position.B(0);position.B(0);guest.Send(0,position.bytes);pump(1);}
    for(int step=1;step<=5;++step){W p;p.B(5);p.V({home.x,26.001f,home.z-1.5f+step*.5f});p.F(0);p.F(0);p.B(0);p.B(0);guest.Send(0,p.bytes);pump(2);}
    auto freshAim=player.CaptureState();freshAim.yaw=PI;player.RestoreState(freshAim);forceReceived=false;bw::Attack(0,&player);pump(3);Check(hitEffects>0&&lastHitVictim==1&&lastHitAttacker==0,"confirmed host attack broadcasts victim and attacker for flash and sound");Check(forceReceived&&swingReceived,"attack animation is broadcast with immediate knockback; click sends current aim and immediate knockback instead of waiting for movement tick");
    auto combatPose=player.CaptureState();
    auto aimAt=[&](float z){W aim;aim.B(5);aim.V({home.x,26.001f,z});aim.F(0);aim.F(0);aim.B(0);aim.B(0);guest.Send(0,aim.bytes);pump(3);};
    aimAt(home.z+2.11f);float outsideHealth=Health;W strike;strike.B(8);strike.B(0);guest.Send(0,strike.bytes);pump(12);Check(Health==outsideHealth,"melee cannot hit beyond 3.3 blocks from target hitbox");
    aimAt(home.z+2.1f);guest.Send(0,strike.bytes);pump(3);Check(Health<outsideHealth&&sprintPaused,"confirmed melee hit pauses attacker sprint and reaches 3.3 blocks");player.Update({},.05f);auto pushed=player.CaptureState().position;Check(std::hypot(pushed.x-combatPose.position.x,pushed.z-combatPose.position.z)>.49f&&std::hypot(pushed.x-combatPose.position.x,pushed.z-combatPose.position.z)<.61f&&pushed.y>combatPose.position.y+.24f&&pushed.y<combatPose.position.y+.29f,"melee knockback is bounded and has a natural upward impulse");player.RestoreState(combatPose);float repeatHealth=Health;int feedbackBefore=hitEffects;sprintPaused=false;guest.Send(0,strike.bytes);pump(1);Check(Health==repeatHealth&&hitEffects==feedbackBefore&&!sprintPaused,"hits during knockback do not apply damage, feedback or attacker sprint pause");pump(2);aimAt(home.z+2.1f);guest.Send(0,strike.bytes);pump(2);Check(Health<repeatHealth&&hitEffects==feedbackBefore+1,"new hit is allowed before the final knockback decay finishes");pump(12);
    W aim;aim.B(5);aim.V({home.x,26.001f,home.z});aim.F(0);aim.F(0);aim.B(0);aim.B(0);guest.Send(0,aim.bytes);pump(3);float beforeHealth=Health;W attack;attack.B(8);attack.B(0);guest.Send(0,attack.bytes);pump(3);Check(Health<beforeHealth,"LAN sword attack applies authoritative damage and knockback");Check(lastHitVictim==0&&lastHitAttacker==1,"guest attack broadcasts hit feedback for host victim");
    float woundedHealth=Health;pump(70);Check(std::abs(Health-woundedHealth)<.001f,"natural healing waits four seconds after damage");pump(10);Check(std::abs(Health-std::min(20.f,woundedHealth+2))<.001f,"natural healing restores one heart per four seconds");
    travel({home.x,26.001f,home.z-3});pump(600);travel({home.x-4,26.001f,home.z});bw::Buy(11);pump(2);for(int slot=0;slot<9;++slot)if(Hotbar[slot].block==GoldenApple)appleSlot=slot;
    Check(Count(GoldenApple)==1,"purchase golden apple");eatingApple=true;pump(20);Check(Count(GoldenApple)==1&&Absorption==0,"apple is not consumed immediately");eatingApple=false;pump(2);eatingApple=true;pump(32);Check(Count(GoldenApple)==1,"apple requires full 1.61 seconds after cancellation");pump(2);eatingApple=false;pump(2);Check(Count(GoldenApple)==0&&Absorption==4,"completed apple grants two golden hearts");float regenHealth=Health;pump(20);Check(Health>=std::min(20.f,regenHealth+2)-.01f,"apple restores one heart per second");
    W dig;dig.B(6);dig.B(0);dig.I(hx);dig.I(26);dig.I(hz+3);dig.B(0);guest.Send(0,dig.bytes);pump(1);dig.bytes[1]=2;guest.Send(0,dig.bytes);pump(1);Check(At(hx,26,hz+3)==Bed,"host rejects bed break before 0.1 second");pump(1);guest.Send(0,dig.bytes);pump(2);Check(At(hx,26,hz+3)==Air&&At(hx,26,hz+4)==Air,"enemy can destroy both bed halves over LAN");auto escaping=bw::PenguinStates();Check(escaping.size()==1&&escaping[0].team==0&&penguinSeen,"destroyed bed wakes one penguin and replicates escape to guest");auto escapeStart=escaping[0].position;pump(20);escaping=bw::PenguinStates();Check(escaping.size()==1&&Vector3Distance(escapeStart,escaping[0].position)>3,"awake penguin runs forward with host-authoritative motion");
    if(argc>1){SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(720,480,"Escaping penguin");{Renderer renderer;Check(renderer.Initialize(),"escaping penguin renderer");auto p=escaping[0].position;Camera3D camera{Vector3Add(p,{2.5f,1.8f,-3}),Vector3Add(p,{0,.6f,0}),{0,1,0},50,CAMERA_PERSPECTIVE};BeginDrawing();renderer.Draw(camera,{});bw::DrawWorld(camera,renderer.Atlas());rlDrawRenderBatchActive();Image image=LoadImageFromScreen();ExportImage(image,"build/penguin-escape.png");UnloadImage(image);EndDrawing();}CloseWindow();}
    pump(300);Check(bw::PenguinStates().empty()&&penguinEnded,"penguin falls into void and removal replicates so clients stop its sound");
    Check(bw::Progress(1).xp==55&&bw::Progress(1).pending==1&&bw::Progress(1).crates==0,"bed grants ten XP and crate stays pending during match");
    travel(combatPose.position);pump(2);guest.Send(0,attack.bytes);pump(2);
    pose=player.CaptureState();pose.position.y=-8;player.RestoreState(pose);pump(8);Check(won&&Mode==GameMode::Creative,"bedless void death eliminates team and declares winner");
    Check(bw::Progress(0).xp==50,"losing player does not receive victory XP");
    Check(bw::Progress(1).xp==95&&bw::Progress(1).crates==1&&bw::Progress(1).pending==0,"final kill grants fifteen XP plus twenty-five victory XP, and lobby delivers crate");
    pump(220);Check(bw::Lobby()&&At(hx,26,hz+3)==Bed&&player.CaptureState().position.y>65,"next round resets arena and returns to lobby");
    Check(bw::Progress(1).xp==95&&bw::Progress(1).crates==1,"round reset duplicates neither victory XP nor crates");
    guest.Stop();pump(4);bw::Stop();Check(!bw::Active(),"LAN session closes cleanly");
    Check(bw::Host("Host",port,16,3,2),"restart NC Dupla host for real client test");int signal[2];Check(pipe(signal)==0,"client test pipe");pid_t child=fork();Check(child>=0,"spawn second MineClone LAN process");
    if(child==0){close(signal[0]);bw::Stop();progression::Directory=sandbox.directory/"separate-client";progression::Profile portable;portable.Award(110);portable.Claim();portable.Open(0);portable.Equip(0,1);std::string error;bool saved=progression::Save("Friend",portable,error);Player remote;bool ok=saved&&bw::Join("127.0.0.1:"+std::to_string(port),"Friend");bool notified=false,started=false;
        for(int i=0;i<1200&&ok;++i){bw::Update(remote,.02f,true,0);if(bw::Ready()&&!notified){char c='R';write(signal[1],&c,1);notified=true;ok=bw::Progress().xp==110&&bw::Progress().crates==1&&bw::Progress().skins[0]==1&&remote.CaptureState().position.y>65&&Count(WhiteWool)==1&&bw::TeamCapacity()==2&&bw::LocalTeam()==1;}
            if(bw::Ready()&&!bw::Lobby()&&remote.CaptureState().position.x>140&&bw::IslandGap()==16&&bw::SelectedMap()==3&&remote.CaptureState().position.y<30){auto generators=bw::Generators();ok=generators.size()==6&&generators[0].remaining>30&&generators[0].remaining<=35&&generators[5].interval==25&&At(86,26,156)==Bed;started=ok;break;}if(!bw::Active())ok=false;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        bw::Stop();progression::Profile persisted;ok=ok&&progression::Load("Friend",persisted,error)&&persisted.xp==110&&persisted.skins[0]==1&&std::filesystem::exists(progression::SkinDirectory()/"friend"/"tools.skin");close(signal[1]);_exit(ok&&notified&&started?0:1);
    }
    close(signal[1]);fcntl(signal[0],F_SETFL,O_NONBLOCK);bool start=false;int childStatus=0;bool exited=false;
    for(int i=0;i<1400;++i){char c;if(!start&&read(signal[0],&c,1)>0){bw::StartRound();start=true;}bw::Update(player,.02f,true,0);if(waitpid(child,&childStatus,WNOHANG)==child){exited=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    if(!exited){bw::Stop();waitpid(child,&childStatus,0);}close(signal[0]);Check(exited&&WIFEXITED(childStatus)&&WEXITSTATUS(childStatus)==0,"separate client brings local level and skin, persists them and enters its team island");bw::Stop();
    std::puts("PASS: BedWars LAN lobby, teams, generators, purchases, placement, mining, bed destruction, elimination, victory and round reset");
}
