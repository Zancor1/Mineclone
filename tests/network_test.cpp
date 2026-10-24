#include "network.h"
#include "network_progress.h"
#include "dns_srv.h"
#include "world.h"
#include "survival.h"
#include "renderer.h"
#include "ui.h"
#include <rlgl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <zlib.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
using Bytes=std::vector<unsigned char>;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
struct W{Bytes b;void u(int x){b.push_back(static_cast<unsigned char>(x));}void v(int x){unsigned a=x;do{unsigned c=a&127;a>>=7;u(c|(a?128:0));}while(a);}void s(int x){u(unsigned(x)>>8);u(x);}void i(unsigned x){s(x>>16);s(x);}void l(uint64_t x){i(x>>32);i(x);}void str(const std::string& s){v(s.size());b.insert(b.end(),s.begin(),s.end());}void f(float x){uint32_t n;std::memcpy(&n,&x,4);i(n);}void d(double x){uint64_t n;std::memcpy(&n,&x,8);l(n);}};
void Send(int fd,const W& w,bool compressed=false){W p;if(compressed){uLongf n=compressBound(w.b.size());Bytes zipped(n);Check(compress(zipped.data(),&n,w.b.data(),w.b.size())==Z_OK,"compress fixture");p.v(w.b.size());p.b.insert(p.b.end(),zipped.begin(),zipped.begin()+n);}else p.b=w.b;W frame;frame.v(p.b.size());frame.b.insert(frame.b.end(),p.b.begin(),p.b.end());size_t sent=0;while(sent<frame.b.size()){ssize_t n=send(fd,frame.b.data()+sent,frame.b.size()-sent,MSG_NOSIGNAL);if(n<=0)return;sent+=n;}}
Bytes Read(int fd,bool compressed=false){unsigned length=0;int shift=0;unsigned char c;do{if(recv(fd,&c,1,MSG_WAITALL)!=1)return {};length|=(c&127)<<shift;shift+=7;if(shift>35||length>4*1024*1024)return {};}while(c&128);Bytes b(length);if(recv(fd,b.data(),b.size(),MSG_WAITALL)!=ssize_t(b.size()))return {};if(compressed){size_t p=0;unsigned raw=0;shift=0;do{if(p>=b.size())return {};c=b[p++];raw|=(c&127)<<shift;shift+=7;}while(c&128);if(raw){Bytes out(raw);uLongf n=raw;if(uncompress(out.data(),&n,b.data()+p,b.size()-p)!=Z_OK)return {};return out;}b.erase(b.begin(),b.begin()+p);}return b;}
int Listen(unsigned short& port){int fd=socket(AF_INET,SOCK_STREAM,0);Check(fd>=0,"local socket");sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=0;Check(bind(fd,reinterpret_cast<sockaddr*>(&a),sizeof(a))==0&&listen(fd,1)==0,"local listener");socklen_t n=sizeof(a);getsockname(fd,reinterpret_cast<sockaddr*>(&a),&n);port=ntohs(a.sin_port);return fd;}
}
int main(int argc,char**){
    using namespace game;
    W record;record.s(10);record.s(5);record.s(25570);record.str("game");record.str("example");record.str("com");record.u(0);
    W dns;dns.s(123);dns.s(0x8180);dns.s(0);dns.s(1);dns.s(0);dns.s(0);dns.u(0);dns.s(33);dns.s(1);dns.i(60);dns.s(record.b.size());dns.b.insert(dns.b.end(),record.b.begin(),record.b.end());
    auto srv=net::ParseSrvRecords(dns.b.data(),dns.b.size());Check(srv.size()==1&&srv[0].host=="game.example.com"&&srv[0].port==25570&&srv[0].priority==10&&srv[0].weight==5,"DNS SRV target, port, priority and weight");
    Check(net::ParseSrvRecords(dns.b.data(),10).empty(),"truncated DNS reply is rejected");
    auto epoch=net::ConnectionProgress::Time{};net::ConnectionProgress progress(epoch);
    Check(!progress.Timeout(epoch+std::chrono::seconds(34))&&progress.Timeout(epoch+std::chrono::seconds(35)),"absolute login deadline");
    progress.Accepted(epoch+std::chrono::seconds(10));
    Check(!progress.Timeout(epoch+std::chrono::seconds(54))&&progress.Timeout(epoch+std::chrono::seconds(55)),"world deadline cannot be prolonged by keepalives");
    progress.Ready();Check(!progress.Timeout(epoch+std::chrono::hours(1)),"completed connection has no startup deadline");
    unsigned short port;int listener=Listen(port);std::atomic<bool> attack{false},chat{false},keepAlive{false},digPosition{false},forgeDone{false},forgeBrand{false},stop{false};
    std::thread server([&]{int fd=accept(listener,nullptr,nullptr);timeval timeout{15,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        auto handshake=Read(fd);Check(handshake.size()>3&&handshake[0]==0&&handshake[1]==47,"real protocol 47 handshake");Check(std::string(handshake.begin(),handshake.end()).find(std::string("\0FML\0",5))!=std::string::npos,"FML marker in handshake hostname");Check(Read(fd)[0]==0,"login start");
        W compress;compress.v(3);compress.v(128);Send(fd,compress);
        W success;success.v(2);success.str("00000000-0000-0000-0000-000000000001");success.str("TestPlayer");Send(fd,success,true);
        auto fml=[&](W payload){W packet;packet.v(0x3f);packet.str("FML|HS");packet.b.insert(packet.b.end(),payload.b.begin(),payload.b.end());Send(fd,packet,true);};
        W hello;hello.u(0);hello.u(2);hello.i(0);fml(hello);
        W mods;mods.u(2);mods.v(1);mods.str("Forge");mods.str("11.15.1.2318");fml(mods);
        W registry;registry.u(3);registry.u(0);registry.str("minecraft:blocks");registry.v(2);registry.str("minecraft:air");registry.v(0);registry.str("minecraft:stone");registry.v(1);registry.v(0);registry.v(0);fml(registry);
        W ack;ack.u(255);ack.u(2);fml(ack);ack.b.back()=3;fml(ack);
        W join;join.v(1);join.i(42);join.u(0);join.u(0);join.u(1);join.u(20);join.str("default");join.u(0);Send(fd,join,true);
        W pose;pose.v(8);pose.d(8.5);pose.d(65);pose.d(8.5);pose.f(0);pose.f(0);pose.u(0);Send(fd,pose,true);
        W chunk;chunk.v(0x21);chunk.i(0);chunk.i(0);chunk.u(1);chunk.s(1<<4);Bytes raw(8192+4096+256,0);
        for(int z=0;z<16;++z)for(int x=0;x<16;++x)raw[(z*16+x)*2]=16;
        chunk.v(raw.size());chunk.b.insert(chunk.b.end(),raw.begin(),raw.end());Send(fd,chunk,true);
        W entity;entity.v(0x0c);entity.v(77);for(int i=0;i<15;++i)entity.u(0);entity.u(2);entity.i(8.5*32);entity.i(65*32);entity.i(10.5*32);entity.u(0);entity.u(0);entity.s(0);entity.u(127);Send(fd,entity,true);
        auto block=[&](int x,int y,int z,int state){W w;w.v(0x23);w.l((uint64_t(x)&0x3ffffff)<<38|(uint64_t(y)&4095)<<26|(uint64_t(z)&0x3ffffff));w.v(state);Send(fd,w,true);};
        block(1,64,1,0);block(1,64,1,16);block(3,64,3,0);block(2,64,2,3*16);block(8,65,8,171*16);block(4,65,7,44*16);
        W health;health.v(6);health.f(7);health.v(20);health.f(5);Send(fd,health,true);
        W velocity;velocity.v(0x12);velocity.v(42);velocity.s(1600);velocity.s(2400);velocity.s(0);Send(fd,velocity,true);
        W window;window.v(0x2d);window.u(1);window.str("minecraft:chest");window.str("{\"text\":\"Menu do servidor\"}");window.u(9);Send(fd,window,true);
        W inventory;inventory.v(0x30);inventory.u(1);inventory.s(45);for(int i=0;i<45;++i){if(i==0||i==36){inventory.s(i==0?264:358);inventory.u(1);inventory.s(i==0?0:7);inventory.u(0);}else inventory.s(-1);}Send(fd,inventory,true);
        auto uuid=[](W& w,int id){for(int i=0;i<15;++i)w.u(0);w.u(id);};
        W list;list.v(0x38);list.v(0);list.v(3);
        const char* names[]={"TestPlayer","Other","Plugin generated tab entry"};
        for(int i=0;i<3;++i){uuid(list,i+1);list.str(names[i]);list.v(0);list.v(i==2?-1:0);list.v(i==2?2147483647:100+i);list.u(i==1);if(i==1)list.str("{\"text\":\"Other display\"}");}Send(fd,list,true);
        W ping;ping.v(0x38);ping.v(2);ping.v(1);uuid(ping,1);ping.v(42);Send(fd,ping,true);
        W extremePing;extremePing.v(0x38);extremePing.v(2);extremePing.v(1);uuid(extremePing,2);extremePing.v(2147483647);Send(fd,extremePing,true);
        W spectator;spectator.v(0x38);spectator.v(1);spectator.v(1);uuid(spectator,2);spectator.v(3);Send(fd,spectator,true);
        W display;display.v(0x38);display.v(3);display.v(1);uuid(display,2);display.u(1);display.str("{\"text\":\"[VIP] Other\"}");Send(fd,display,true);
        W remove;remove.v(0x38);remove.v(4);remove.v(1);uuid(remove,3);Send(fd,remove,true);
        W team;team.v(0x3e);team.str("test");team.u(0);team.str("Testing");team.str("[TEST] ");team.str("");team.u(0);team.str("always");team.u(0);team.v(1);team.str("TestPlayer");Send(fd,team,true);
        W header;header.v(0x47);header.str("{\"text\":\"Servidor de teste\\nJava 1.8\"}");header.str("{\"text\":\"Bem-vindo\"}");Send(fd,header,true);
        W message;message.v(2);message.str("{\"translate\":\"chat.type.text\",\"with\":[{\"text\":\"TestPlayer\"},\"hello!\"]}");message.u(0);Send(fd,message,true);
        W map;map.v(0x34);map.v(7);map.u(0);map.v(0);map.u(128);map.u(128);map.u(0);map.u(0);map.v(16384);for(int i=0;i<16384;++i)map.u(((i/128)/16+i%128/16)%2?34:50);Send(fd,map,true);
        W patch;patch.v(0x34);patch.v(7);patch.u(0);patch.v(0);patch.u(2);patch.u(1);patch.u(10);patch.u(12);patch.v(2);patch.u(18);patch.u(50);Send(fd,patch,true);
        W hologram;hologram.v(0x0f);hologram.v(99);hologram.u(30);hologram.i(8*32);hologram.i(65*32);hologram.i(10*32);hologram.u(0);hologram.u(0);hologram.u(0);hologram.s(0);hologram.s(0);hologram.s(0);hologram.u(0);hologram.u(32);hologram.u((4<<5)|2);hologram.str("\xc2\xa7" "aTeste holograma");hologram.u(3);hologram.u(1);hologram.u(127);Send(fd,hologram,true);
        W frame;frame.v(0x0e);frame.v(88);frame.u(71);frame.i(8.5*32);frame.i(65.5*32);frame.i(11*32);frame.u(0);frame.u(0);frame.i(2);frame.s(0);frame.s(0);frame.s(0);Send(fd,frame,true);
        W metadata;metadata.v(0x1c);metadata.v(88);metadata.u((5<<5)|8);metadata.s(358);metadata.u(1);metadata.s(7);metadata.u(0);metadata.u(9);metadata.u(2);metadata.u(127);Send(fd,metadata,true);
        W keep;keep.v(0);keep.v(12345);Send(fd,keep,true);
        while(!stop){auto b=Read(fd,true);if(b.empty())break;if(b.size()==20&&b[0]==0x17&&b[1]==8&&std::memcmp(b.data()+2,"MC|Brand",8)==0&&b[10]==9&&std::memcmp(b.data()+11,"fml,forge",9)==0)forgeBrand=true;if(b.size()==10&&b[0]==0x17&&b[1]==6&&std::memcmp(b.data()+2,"FML|HS",6)==0&&b[8]==255&&b[9]==5)forgeDone=true;if(b[0]==7&&b.size()>=10){uint64_t packed=0;for(int i=2;i<10;++i)packed=(packed<<8)|b[i];digPosition=packed==((uint64_t(1)<<38)|(uint64_t(64)<<26)|1); }if(b[0]==2)attack=true;if(b[0]==1){chat=true;if(std::string(b.begin(),b.end()).find("arena_test")!=std::string::npos){
            W respawn;respawn.v(7);respawn.i(0);respawn.u(1);respawn.u(0);respawn.str("default");Send(fd,respawn,true);
            W arena;arena.v(0x26);arena.u(1);arena.v(2);for(int i=0;i<2;++i){arena.i(-1000+i);arena.i(1000);arena.s(1<<8);}
            for(int i=0;i<2;++i){Bytes data(8192+4096+256,0);for(int cell=0;cell<256;++cell){int state=(i?20:1)*16;data[cell*2]=state&255;data[cell*2+1]=state>>8;}arena.b.insert(arena.b.end(),data.begin(),data.end());}Send(fd,arena,true);
            W attributes;attributes.v(0x20);attributes.v(42);attributes.i(1);attributes.str("generic.movementSpeed");attributes.d(.1);attributes.v(1);for(int i=0;i<16;++i)attributes.u(0);attributes.d(.3);attributes.u(2);Send(fd,attributes,true);
            W target;target.v(8);target.d(-15991.5);target.d(129);target.d(16008.5);target.f(0);target.f(0);target.u(0);Send(fd,target,true);
        }}if(b[0]==0)keepAlive=true;}
        shutdown(fd,SHUT_RDWR);close(fd);
    });
    Player player;net::Connect("127.0.0.1:"+std::to_string(port),"TestPlayer");
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(std::chrono::steady_clock::now()<deadline){net::Update(player,.02f);if(net::Joined()&&net::WindowOpen()&&Health==7&&net::FindMap(7)&&net::MapFrames().size()==1)break;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
    Check(net::Joined(),"compressed login and remote chunk join");Check(At(128,32,128)==Stone,"remote section decoded with vertical offset");
    Check(At(129,32,129)==Stone&&At(131,32,131)==Air&&At(130,32,130)==Dirt,"Java 1.8 X/Y/Z block corrections and remote air removal");
    Check(net::CollisionAt(136,33,136)==1.f/16,"server carpet keeps fractional collision height");
    Check(net::Holograms()[0].text=="[VIP] Other","player UUID resolves name above head");
    Check(Health==7&&player.CaptureState().velocityY==6,"server controls damage and knockback velocity");
    Check(net::WindowOpen()&&net::WindowSlots().size()==45&&net::LocalItem(net::WindowSlots()[0])==Diamond,"server inventory uses real packets");
    auto players=net::PlayerList();Check(players.size()==2&&players[0].name=="TestPlayer"&&players[0].ping==42&&players[0].displayName=="[TEST] TestPlayer"&&players[1].ping==2147483647&&players[1].gameMode==3&&players[1].displayName=="[VIP] Other","tab add/update/remove, ping, spectator and team formatting");
    Check(net::Nickname()=="TestPlayer"&&net::TabHeader()=="Servidor de teste\nJava 1.8"&&net::TabFooter()=="Bem-vindo","tab banners and actual login nickname");
    Check(!net::Messages().empty()&&net::Messages().back()=="<TestPlayer> hello!","server chat preserves sender nickname");
    auto map=net::FindMap(7);Check(map&&map->pixels[12*128+10]==18&&map->pixels[12*128+11]==50&&map->pixels[0]==50&&net::HeldMapId()==7,"map patch updates preserve surrounding pixels and held map ID");
    Check(net::MapColor(18).r==255&&net::MapColor(18).g==0&&net::MapColor(34).r==255&&net::MapColor(0).a==0,"1.8 map palette and transparent pixels");
    Check(net::Holograms().size()==2&&net::Holograms()[1].text=="\xc2\xa7" "aTeste holograma","armor stand hologram metadata");
    Check(net::MapFrames()[0].mapId==7&&net::MapFrames()[0].direction==2&&net::MapFrames()[0].rotation==2,"item-frame map metadata");
    net::Dig({true,129,32,129,0,1,0},0);
    auto startPose=player.CaptureState();auto walkPose=startPose;walkPose.position.y=33.064f;walkPose.velocityY=0;player.RestoreState(walkPose);
    PlayerInput walk;walk.forward=1;for(int i=0;i<24;++i){net::Update(player,1.f/120);auto corrected=player.CaptureState();player.RestoreState(corrected);player.Update(walk,1.f/120);}
    Check(std::abs(player.CaptureState().position.z-walkPose.position.z)>.2f,"multiplayer movement crosses carpet at 120 FPS");player.RestoreState(startPose);
    auto measureSpeed=[&](bool sprint,bool sneak){auto pose=startPose;pose.position={133.5f,33.001f,133.5f};pose.yaw=3.14159265f;pose.velocityY=0;player.RestoreState(pose);PlayerInput move;move.forward=1;move.sprint=sprint;move.swimDown=sneak;float speed=0;for(int i=0;i<25;++i){move.sprintPressed=sprint&&i==0;auto before=player.CaptureState().position;player.Update(move,.05f);auto after=player.CaptureState().position;speed=std::abs(after.z-before.z)*20;}return speed;};
    Check(std::abs(measureSpeed(false,false)-4.317f)<.01f,"Java 1.8 steady walking speed");
    Check(std::abs(measureSpeed(true,false)-5.612f)<.01f,"Ctrl sprint speed");
    Check(std::abs(measureSpeed(true,true)-1.295f)<.01f&&!player.IsSprinting(),"Shift overrides sprint and uses sneak speed");
    auto exactGround=startPose;exactGround.position={133.5f,33.f,133.5f};exactGround.velocityY=0;player.RestoreState(exactGround);
    for(int i=0;i<40;++i)player.Update({},.05f);
    Check(player.CaptureState().position.y==33.f&&player.OnGround(),"multiplayer floor contact never adds a synthetic Y offset");
    auto stepPose=startPose;stepPose.position={132.5f,33.001f,134.5f};stepPose.velocityY=0;stepPose.yaw=3.14159265f;player.RestoreState(stepPose);PlayerInput stepping;stepping.forward=1;float highest=33;
    for(int i=0;i<15;++i){player.Update(stepping,.05f);highest=std::max(highest,player.CaptureState().position.y);}
    Check(highest>33.49f&&player.CaptureState().position.z>135.5f,"automatic half slab stepping keeps player moving");
    auto edgePose=startPose;edgePose.position={133.5f,33.001f,143.7f};edgePose.yaw=3.14159265f;edgePose.velocityY=0;player.RestoreState(edgePose);PlayerInput edge;edge.forward=1;edge.swimDown=true;
    for(int i=0;i<40;++i)player.Update(edge,.05f);
    Check(player.CaptureState().position.z<144.3f&&player.CaptureState().position.y>32.99f,"sneaking stops at an unsupported edge");
    Check(player.Camera().position.y<player.CaptureState().position.y+1.6f,"sneaking lowers eye height");
    player.RestoreState(startPose);
    if(argc>1){
        SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(720,480,"Network rendering test");
        {Renderer renderer;Check(renderer.Initialize(),"map renderer initializes");UI ui;auto camera=player.Camera();
            BeginDrawing();renderer.Draw(camera,{});renderer.DrawFirstPerson(camera,FilledMap,false,false,.05f);rlDrawRenderBatchActive();
            Image capture=LoadImageFromScreen();ExportImage(capture,"build/server-map-preview.png");
            Color pixel=GetImageColor(capture,232+8*2,120+8*2);Check(pixel.b>240&&pixel.r<100,"received blue map pixels reach framebuffer");UnloadImage(capture);EndDrawing();
            BeginDrawing();renderer.Draw(camera,{});ui.DrawPlayerList();rlDrawRenderBatchActive();capture=LoadImageFromScreen();ExportImage(capture,"build/tab-preview.png");UnloadImage(capture);EndDrawing();
            net::ShowInventory();Check(net::PlayerInventoryOpen(),"player inventory uses its own slot layout");
            BeginDrawing();ClearBackground(BLACK);ui.Draw(renderer.Atlas(),GameState::ServerInventory);rlDrawRenderBatchActive();capture=LoadImageFromScreen();ExportImage(capture,"build/inventory-layout.png");UnloadImage(capture);EndDrawing();
        }CloseWindow();
    }
    net::Attack(player);net::Chat("hello");net::ClickSlot(0,false,false);
    for(int i=0;i<100&&(!attack||!chat||!keepAlive||!digPosition||!forgeDone||!forgeBrand);++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Check(attack&&chat&&keepAlive&&digPosition&&forgeDone&&forgeBrand,"attack, chat and keepalive sent to actual TCP server");
    net::Chat("arena_test");auto arenaDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(std::chrono::steady_clock::now()<arenaDeadline){net::Update(player,.01f);if(net::Joined()&&RegionOriginX< -10000)break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
    Check(net::Joined()&&At(136,32,136)==Stone&&At(152,32,136)==Glass,"respawn, bulk chunks before teleport, distant arena and vertical origin");
    Check(std::abs(net::MovementSpeed()-.13f)<.00001f,"server movement attribute modifiers are applied");
    auto arenaStart=player.CaptureState().position;PlayerInput arenaWalk;arenaWalk.forward=1;for(int i=0;i<15;++i)player.Update(arenaWalk,.05f);
    Check(player.CaptureState().position.z>arenaStart.z+.5f&&player.OnGround(),"walk on server arena after transfer");
    if(argc>1){
        SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(720,480,"Arena transfer rendering test");
        {Renderer renderer;Check(renderer.Initialize(),"arena renderer initializes");auto camera=player.Camera();camera.target={camera.position.x,camera.position.y-3,camera.position.z+2};
            BeginDrawing();renderer.Draw(camera,{});rlDrawRenderBatchActive();Image frame=LoadImageFromScreen();ExportImage(frame,"build/arena-transfer-preview.png");Color floor=GetImageColor(frame,360,350);Check(std::abs(int(floor.r)-int(floor.b))<35,"received arena floor is rendered after teleport");UnloadImage(frame);EndDrawing();}
        CloseWindow();
    }
    net::Disconnect();stop=true;server.join();close(listener);
    Check(!net::Active(),"disconnect stops worker cleanly");
    // An online-mode challenge must fail explicitly rather than pretend authentication worked.
    listener=Listen(port);std::thread auth([&]{int fd=accept(listener,nullptr,nullptr);Read(fd);Read(fd);W challenge;challenge.v(1);Send(fd,challenge);std::this_thread::sleep_for(std::chrono::milliseconds(40));close(fd);});
    net::Connect("127.0.0.1:"+std::to_string(port),"TestPlayer");for(int i=0;i<100&&net::Active();++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Check(!net::Active()&&net::Status().find("RSA/AES")!=std::string::npos,"encryption request reports missing crypto without assuming Microsoft authentication");net::Disconnect();auth.join();close(listener);
    Check(net::PlayerList().empty()&&net::FindMap(7)==nullptr,"new session clears tab and map caches");
    listener=Listen(port);std::thread modded([&]{int fd=accept(listener,nullptr,nullptr);Read(fd);Read(fd);W success;success.v(2);success.str("00000000-0000-0000-0000-000000000001");success.str("TestPlayer");Send(fd,success);
        auto fml=[&](W payload){W packet;packet.v(0x3f);packet.str("FML|HS");packet.b.insert(packet.b.end(),payload.b.begin(),payload.b.end());Send(fd,packet);};
        W hello;hello.u(0);hello.u(2);hello.i(0);fml(hello);W mods;mods.u(2);mods.v(0);fml(mods);W registry;registry.u(3);registry.u(0);registry.str("minecraft:blocks");registry.v(1);registry.str("example:machine");registry.v(2000);registry.v(0);fml(registry);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));close(fd);});
    net::Connect("127.0.0.1:"+std::to_string(port),"TestPlayer");for(int i=0;i<100&&net::Active();++i){net::Update(player,.01f);std::this_thread::sleep_for(std::chrono::milliseconds(5));}
    Check(!net::Active()&&net::Status().find("example:machine")!=std::string::npos,"Forge rejects unsupported mod registries explicitly");net::Disconnect();modded.join();close(listener);
    std::puts("PASS: tab, chat nicknames, map patches, frames, protocol 47 TCP login, compression, chunks, players, server damage/knockback, inventory, combat and disconnect");
}
