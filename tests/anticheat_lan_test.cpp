#include "anticheat.h"
#include "bedwars.h"
#include <chrono>
#include <cstring>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <csignal>
namespace {void Check(bool v,const char* s){if(!v){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
struct Packet {std::vector<unsigned char> data;Packet(){data.reserve(128);}void Byte(int v){data.push_back(v);}void Int(int v){for(int i=0;i<4;++i)Byte(uint32_t(v)>>(i*8));}void Float(float v){int n;std::memcpy(&n,&v,4);Int(n);}void Text(std::string s){Byte(int(s.size()));data.insert(data.end(),s.begin(),s.end());}};
int ReadInt(const std::vector<unsigned char>& p,int at){uint32_t n=0;for(int i=0;i<4;++i)n|=uint32_t(p[at+i])<<(i*8);return int(n);}}
int main(){using namespace game;
    struct Sandbox {std::filesystem::path p;~Sandbox(){std::error_code e;std::filesystem::remove_all(p,e);}} sandbox{std::filesystem::temp_directory_path()/("mineclone-anticheat-"+std::to_string(getpid()))};progression::Directory=sandbox.p/"profiles";
    int probe=socket(AF_INET,SOCK_STREAM,0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);Check(bind(probe,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,"bind probe");socklen_t size=sizeof(address);getsockname(probe,reinterpret_cast<sockaddr*>(&address),&size);int port=ntohs(address.sin_port);close(probe);
    Check(bw::Host("Host",port),"host");std::array<LanTransport,5> guests;Player player;int serial=0,eventEpoch=0;bool kickMessage=false;
    for(auto& peer:guests)Check(peer.Join("127.0.0.1:"+std::to_string(port)),"connect observers");
    auto pump=[&](int frames){for(int tick=0;tick<frames;++tick){for(int i=0;i<5;++i)for(auto event:guests[i].Poll()){
        if(event.kind==LanTransport::Event::Connected){Packet hello;hello.Byte(1);hello.Int(LanProtocolVersion);hello.Text("Peer"+std::to_string(i+1));guests[i].Send(0,hello.data);}
        if(event.kind==LanTransport::Event::Data&&!event.data.empty()){
            if(event.data[0]==32&&event.data.size()==19){serial=ReadInt(event.data,1);eventEpoch=ReadInt(event.data,6);}
            if(i==0&&event.data[0]==15)kickMessage=true;
        }
    }bw::Update(player,.05f,true,0);std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
    pump(80);Check(bw::PlayerList().size()==6,"six players connected");bw::StartRound();pump(140);Check(!bw::Lobby(),"round active");
    Packet fake;fake.Byte(33);fake.Int(999999);fake.Byte(1);fake.Int(1);guests[1].Send(0,fake.data);pump(3);Check(bw::PlayerList().size()==6,"fabricated accusations cannot kick");
    for(int i=0;i<6;++i){Packet move;move.Byte(5);move.Float(10);move.Float(90);move.Float(10);move.Float(0);move.Float(0);move.Byte(0);move.Byte(0);guests[0].Send(0,move.data);pump(2);}
    pump(10);Check(serial>0,"host publishes independently checkable movement episode");Check(bw::PlayerList().size()==6,"host alone cannot kick with six players");
    auto confirm=[&](int observer,int episode){Packet p;p.Byte(33);p.Int(episode);p.Byte(1);p.Int(eventEpoch);guests[observer].Send(0,p.data);pump(4);};
    confirm(0,serial);confirm(1,serial+100);Check(bw::PlayerList().size()==6,"self-vote and invented episode ignored");
    confirm(1,serial);confirm(1,serial);Check(bw::PlayerList().size()==6,"duplicate observer cannot supply third confirmation");
    confirm(2,serial);pump(10);Check(bw::PlayerList().size()==5&&kickMessage,"three distinct observers kick with explanatory message");
    // Consensus is a kick, not a nickname ban. Stale confirmations must not
    // transfer to a new connection reusing the same fighter slot.
    int oldSerial=serial,oldEpoch=eventEpoch;guests[0].Stop();bw::Restart();pump(5);
    Check(guests[0].Join("127.0.0.1:"+std::to_string(port)),"kicked nickname may reconnect in lobby");pump(40);Check(bw::PlayerList().size()==6,"consensus creates no nickname ban");
    Packet replay;replay.Byte(33);replay.Int(oldSerial);replay.Byte(1);replay.Int(oldEpoch);guests[1].Send(0,replay.data);guests[2].Send(0,replay.data);pump(5);Check(bw::PlayerList().size()==6,"old confirmations cannot kick a reused slot");
    bw::Stop();for(auto& peer:guests)peer.Stop();
    int start[2],readyPipe[2];Check(pipe(start)==0&&pipe(readyPipe)==0,"observer synchronization pipes");pid_t child=fork();Check(child>=0,"spawn independent real client");
    if(child==0){close(start[1]);close(readyPipe[0]);char signal=0;bool launched=read(start[0],&signal,1)==1;close(start[0]);progression::Directory=sandbox.p/"observer";
        bool joined=launched&&bw::Join("127.0.0.1:"+std::to_string(port),"Observer"),notified=false;Player remote;
        for(int i=0;i<5000&&joined&&bw::Active();++i){bw::Update(remote,.02f,true,0);if(bw::Ready()&&!notified){char r='R';write(readyPipe[1],&r,1);notified=true;}std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        bw::Stop();close(readyPipe[1]);_exit(notified?0:1);
    }
    close(start[0]);close(readyPipe[1]);fcntl(readyPipe[0],F_SETFL,O_NONBLOCK);
    Check(bw::Host("Host",port),"restart for automatic observer test");for(int i=0;i<2;++i)Check(guests[i].Join("127.0.0.1:"+std::to_string(port)),"connect unmodified transport peers");pump(50);char signal='S';write(start[1],&signal,1);close(start[1]);bool observerReady=false;
    for(int i=0;i<1000&&!observerReady;++i){pump(2);char r;if(read(readyPipe[0],&r,1)==1)observerReady=true;}
    Check(observerReady&&bw::PlayerList().size()==4,"real observer enters with four players");bw::StartRound();pump(150);
    for(int i=0;i<6;++i){Packet move;move.Byte(5);move.Float(10);move.Float(90);move.Float(10);move.Float(0);move.Float(0);move.Byte(0);move.Byte(0);guests[0].Send(0,move.data);pump(2);}
    pump(50);Check(bw::PlayerList().size()==3,"real client independently detects movement and supplies second confirmation automatically");
    bw::Stop();for(auto& peer:guests)peer.Stop();int childStatus=0;bool exited=false;for(int i=0;i<500&&!exited;++i){exited=waitpid(child,&childStatus,WNOHANG)==child;if(!exited)std::this_thread::sleep_for(std::chrono::milliseconds(5));}if(!exited){kill(child,SIGTERM);waitpid(child,&childStatus,0);}close(readyPipe[0]);Check(exited&&WIFEXITED(childStatus)&&WEXITSTATUS(childStatus)==0,"real observer shuts down cleanly");
    std::puts("PASS: LAN anti-cheat rejects forged/replayed votes and kicks only at quorum");
}
