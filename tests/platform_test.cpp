#include "platform_socket.h"
#include "lan_transport.h"
#include "dns_srv.h"
#include "file_replace.h"
#include <chrono>
#include <thread>
#include <fstream>
#include <cstdio>
#include <cstdlib>
namespace {
void Check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
}
int main(){
    using namespace game;
    Check(netio::Initialize(),"socket startup");
    netio::Socket probe=socket(AF_INET,SOCK_STREAM,0);Check(probe!=netio::Invalid,"socket handle");
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    Check(bind(probe,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,"ephemeral port");
    netio::SocketLength length=sizeof(address);Check(getsockname(probe,reinterpret_cast<sockaddr*>(&address),&length)==0,"socket address");
    auto port=ntohs(address.sin_port);netio::Close(probe);
    LanTransport host,guest;Check(host.Host(port),"LAN listener");Check(guest.Join("127.0.0.1:"+std::to_string(port)),"LAN connect");
    bool sent=false,echo=false;const std::vector<unsigned char> payload={1,0,255,42,0,17};
    auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!echo&&std::chrono::steady_clock::now()<until){
        for(auto& event:guest.Poll()){
            if(event.kind==LanTransport::Event::Connected){guest.Send(0,payload);sent=true;}
            if(event.kind==LanTransport::Event::Data){Check(event.data==payload,"client frame bytes");echo=true;}
        }
        for(auto& event:host.Poll())if(event.kind==LanTransport::Event::Data){Check(event.data==payload,"host frame bytes");host.Send(event.peer,event.data);}
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Check(sent&&echo,"nonblocking LAN round trip");guest.Stop();host.Stop();
    LanDiscovery advertiser,browser;
    Check(advertiser.Advertise({{},"DiscoveryHost",port,2,32,true}),"UDP discovery host");Check(browser.Search(),"start LAN discovery");
    bool discovered=false;until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
    while(!discovered&&std::chrono::steady_clock::now()<until){advertiser.Poll();browser.Poll();for(auto& room:browser.Rooms())if(room.name=="DiscoveryHost"&&room.port==port){Check(room.players==2&&room.gap==32&&room.joinable,"discovered room metadata");discovered=true;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    Check(discovered,"broadcast/multicast discovery finds local server");
    advertiser.Advertise({{},"DiscoveryHost",port,4,16,false});Check(browser.Search(),"refresh clears stale rooms");Check(browser.Rooms().empty(),"old room list cleared");discovered=false;until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
    while(!discovered&&std::chrono::steady_clock::now()<until){advertiser.Poll();browser.Poll();for(auto& room:browser.Rooms())if(room.name=="DiscoveryHost"&&room.port==port){Check(room.players==4&&room.gap==16&&!room.joinable,"full/running room cannot be joined");discovered=true;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    Check(discovered,"discovery refresh updates host metadata");advertiser.Advertise({{},"DiscoveryHost",port,7,16,true,0,8});Check(browser.Search(),"search Dupla room");discovered=false;until=std::chrono::steady_clock::now()+std::chrono::seconds(4);
    while(!discovered&&std::chrono::steady_clock::now()<until){advertiser.Poll();browser.Poll();for(const auto& room:browser.Rooms())if(room.name=="DiscoveryHost"&&room.port==port){Check(room.players==7&&room.capacity==8&&room.joinable,"Dupla advertises capacity and remaining slot");discovered=true;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    Check(discovered,"Dupla discovery metadata");browser.CancelSearch();Check(!browser.Searching(),"cancel discovery");advertiser.StopHosting();
    const std::string path="platform-save-test.dat",temporary=path+".tmp";std::error_code error;
    {std::ofstream file(path);file<<"old";}{std::ofstream file(temporary);file<<"new";}
    ReplaceSaveFile(temporary,path,error);Check(!error,"replace existing save");
    {std::ifstream file(path);std::string value;file>>value;Check(value=="new","saved content");}
    std::filesystem::remove(path,error);Check(!std::filesystem::exists(temporary),"temporary save consumed");
    const unsigned char dns[]={0,1,0x81,0x80,0,0,0,1,0,0,0,0,0,0,33,0,1,0,0,0,60,0,18,0,0,0,5,0x63,0xdd,4,'t','e','s','t',5,'l','o','c','a','l',0};
    auto records=net::ParseSrvRecords(dns,sizeof(dns));Check(records.size()==1&&records[0].host=="test.local"&&records[0].port==25565,"portable DNS SRV parser");
    Check(net::ParseSrvRecords(dns,15).empty(),"truncated DNS rejected");
    std::puts("PASS: socket startup, LAN frames, UDP discovery, save replacement and DNS SRV");
}
