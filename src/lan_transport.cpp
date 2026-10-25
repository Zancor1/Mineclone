#include "lan_transport.h"
#include <chrono>
#include <algorithm>
namespace game {
namespace {
double Now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void Configure(netio::Socket fd){netio::SetNonblocking(fd,true);int yes=1;netio::SetOption(fd,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes));}
constexpr size_t MaxPacket=256*1024,MaxQueue=1024*1024;
}
LanTransport::~LanTransport(){Stop();}
void LanTransport::Stop(){for(auto& p:peers)netio::Close(p.fd);peers.clear();if(listener!=netio::Invalid)netio::Close(listener);listener=netio::Invalid;nextId=1;}
void LanTransport::Swap(LanTransport& other){peers.swap(other.peers);std::swap(listener,other.listener);std::swap(nextId,other.nextId);error.swap(other.error);}
unsigned short LanTransport::Port()const{sockaddr_in a{};netio::SocketLength n=sizeof(a);return listener!=netio::Invalid&&getsockname(listener,reinterpret_cast<sockaddr*>(&a),&n)==0?ntohs(a.sin_port):0;}
std::string LanTransport::PeerIP(int peer)const{for(const auto& p:peers)if(p.id==peer){sockaddr_in a{};netio::SocketLength n=sizeof(a);char ip[INET_ADDRSTRLEN]{};if(getpeername(p.fd,reinterpret_cast<sockaddr*>(&a),&n)==0&&inet_ntop(AF_INET,&a.sin_addr,ip,sizeof(ip)))return ip;}return {};}
bool LanTransport::Host(unsigned short port){Stop();error.clear();if(!netio::Initialize()){error="Falha ao inicializar rede";return false;}listener=socket(AF_INET,SOCK_STREAM,0);if(listener==netio::Invalid){error="Falha ao criar socket LAN";return false;}int yes=1;netio::SetOption(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=INADDR_ANY;a.sin_port=htons(port);if(bind(listener,reinterpret_cast<sockaddr*>(&a),sizeof(a))<0||listen(listener,8)<0){error="Porta LAN ocupada ou indisponivel";Stop();return false;}Configure(listener);return true;}
bool LanTransport::Join(const std::string& address){Stop();error.clear();if(!netio::Initialize()){error="Falha ao inicializar rede";return false;}std::string host=address;int port=25566;try{auto c=host.find(':');if(c!=std::string::npos){auto s=host.substr(c+1);if(s.empty()||s.find_first_not_of("0123456789")!=std::string::npos)throw 0;port=std::stoi(s);host.resize(c);}if(port<1||port>65535)throw 0;}catch(...){error="Use IP:porta, por exemplo 192.168.1.20:25566";return false;}if(host=="localhost")host="127.0.0.1";sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(port);if(inet_pton(AF_INET,host.c_str(),&a.sin_addr)!=1){error="Informe o IP IPv4 do computador que hospeda";return false;}netio::Socket fd=socket(AF_INET,SOCK_STREAM,0);if(fd==netio::Invalid){error="Falha ao criar socket";return false;}Configure(fd);int result=connect(fd,reinterpret_cast<sockaddr*>(&a),sizeof(a));if(result<0&&!netio::Connecting(netio::LastError())){netio::Close(fd);error="Nao foi possivel conectar ao host";return false;}peers.push_back({0,fd,true,Now(),{},{},0});return true;}
void LanTransport::Drop(int peer){for(auto& p:peers)if(p.id==peer)p.closing=true;}
void LanTransport::Send(int peer,const std::vector<unsigned char>& data){if(data.empty()||data.size()>MaxPacket)return;for(auto& p:peers)if(p.id==peer){if(p.output.size()-p.sent+data.size()+4>MaxQueue){netio::Shutdown(p.fd);return;}if(p.sent){p.output.erase(p.output.begin(),p.output.begin()+p.sent);p.sent=0;}uint32_t n=uint32_t(data.size());for(int i=0;i<4;++i)p.output.push_back(static_cast<unsigned char>(n>>(24-i*8)));p.output.insert(p.output.end(),data.begin(),data.end());
        if(!p.connecting&&!p.closing){int n=netio::Send(p.fd,p.output.data(),std::min(size_t(65536),p.output.size()));if(n>0)p.sent=size_t(n);else if(n==0||!netio::Temporary(netio::LastError()))netio::Shutdown(p.fd);}
    }}
void LanTransport::Broadcast(const std::vector<unsigned char>& data){for(auto& p:peers)Send(p.id,data);}
std::vector<LanTransport::Event> LanTransport::Poll(){
    std::vector<Event> events;
    if(listener!=netio::Invalid)for(int count=0;count<8;++count){netio::Socket fd=accept(listener,nullptr,nullptr);if(fd==netio::Invalid)break;if(peers.size()>=15){netio::Close(fd);continue;}Configure(fd);int id=nextId++;peers.push_back({id,fd,false,Now(),{},{},0});events.push_back({Event::Connected,id,{}});}
    for(auto it=peers.begin();it!=peers.end();){auto& p=*it;bool dead=false,readClosed=false;
        if(p.connecting){sockaddr_in address{};netio::SocketLength size=sizeof(address);if(getpeername(p.fd,reinterpret_cast<sockaddr*>(&address),&size)==0){p.connecting=false;events.push_back({Event::Connected,p.id,{}});}else {int value=netio::SocketError(p.fd);if(value||Now()-p.started>8){dead=true;error="Host LAN sem resposta. Confira IP, porta e firewall.";}else {++it;continue;}}}
        for(int i=0;!dead&&!p.closing&&i<8;++i){unsigned char bytes[8192];int n=netio::Receive(p.fd,bytes,sizeof(bytes));if(n>0){p.input.insert(p.input.end(),bytes,bytes+n);if(p.input.size()>MaxQueue)dead=true;}else {if(n==0)readClosed=true;else if(!netio::Temporary(netio::LastError()))dead=true;break;}}
        size_t consumed=0;int messages=0;
        while(!dead&&p.input.size()-consumed>=4&&messages++<128){uint32_t n=0;for(int i=0;i<4;++i)n=(n<<8)|p.input[consumed+i];if(n==0||n>MaxPacket){dead=true;break;}if(p.input.size()-consumed<4+n)break;events.push_back({Event::Data,p.id,{p.input.begin()+consumed+4,p.input.begin()+consumed+4+n}});consumed+=4+n;}
        if(consumed)p.input.erase(p.input.begin(),p.input.begin()+consumed);
        if(!dead&&!readClosed&&p.sent<p.output.size()){int n=netio::Send(p.fd,p.output.data()+p.sent,std::min(size_t(65536),p.output.size()-p.sent));if(n>0)p.sent+=size_t(n);else if(n<0&&!netio::Temporary(netio::LastError()))dead=true;}
        if(p.sent==p.output.size()){p.output.clear();p.sent=0;}
        if(p.closing&&p.output.empty())dead=true;
        if(readClosed&&(p.input.empty()||messages<128))dead=true;
        if(dead){events.push_back({Event::Disconnected,p.id,{}});netio::Close(p.fd);it=peers.erase(it);if(listener==netio::Invalid&&error.empty())error="O host encerrou a partida LAN";}else ++it;
    }
    return events;
}
}

#ifdef _WIN32
#include <iptypes.h>
extern "C" ULONG WINAPI GetAdaptersAddresses(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);
#else
#include <ifaddrs.h>
#include <net/if.h>
#endif
#include <cstring>
#include <random>
namespace game {
namespace {
constexpr unsigned short DiscoveryPort=25567;
constexpr uint32_t MulticastAddress=0xefff4d43; // 239.255.77.67
static_assert(LanProtocolVersion>=0&&LanProtocolVersion<100);
constexpr char ReplyMagic[]={'M','C','L','A','N',char('0'+LanProtocolVersion/10),char('0'+LanProtocolVersion%10),'!',0};
int DiscoveryVersion(const unsigned char* data,char suffix){
    if(std::memcmp(data,"MCLAN",5)||data[7]!=suffix||data[5]<'0'||data[5]>'9'||data[6]<'0'||data[6]>'9')return -1;
    return (data[5]-'0')*10+data[6]-'0';
}
bool BindDiscovery(netio::Socket fd,unsigned short port){sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=INADDR_ANY;address.sin_port=htons(port);return bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0&&netio::SetNonblocking(fd,true);}
}
LanDiscovery::~LanDiscovery(){StopHosting();CancelSearch();}
void LanDiscovery::FindInterfaces(){
    interfaces.clear();
    auto add=[&](uint32_t address,uint32_t mask){if(!address)return;for(auto i:interfaces)if(i.address==address)return;interfaces.push_back({address,address|~mask});};
#ifdef _WIN32
    ULONG size=16384;std::vector<unsigned char> buffer(size);
    auto flags=GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER;
    ULONG result=GetAdaptersAddresses(AF_INET,flags,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&size);
    if(result==ERROR_BUFFER_OVERFLOW){buffer.resize(size);result=GetAdaptersAddresses(AF_INET,flags,nullptr,reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&size);}
    if(result==NO_ERROR)for(auto* adapter=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());adapter;adapter=adapter->Next){
        if(adapter->OperStatus!=IfOperStatusUp)continue;
        for(auto* u=adapter->FirstUnicastAddress;u;u=u->Next)if(u->Address.lpSockaddr&&u->Address.lpSockaddr->sa_family==AF_INET){unsigned prefix=u->OnLinkPrefixLength;uint32_t mask=prefix==0?0:prefix>=32?0xffffffffu:0xffffffffu<<(32-prefix);add(reinterpret_cast<sockaddr_in*>(u->Address.lpSockaddr)->sin_addr.s_addr,htonl(mask));}
    }
#else
    ifaddrs* list=nullptr;
    if(getifaddrs(&list)==0){for(auto* i=list;i;i=i->ifa_next)if(i->ifa_addr&&i->ifa_netmask&&i->ifa_addr->sa_family==AF_INET&&(i->ifa_flags&IFF_UP)){auto* a=reinterpret_cast<sockaddr_in*>(i->ifa_addr);auto* m=reinterpret_cast<sockaddr_in*>(i->ifa_netmask);add(a->sin_addr.s_addr,m->sin_addr.s_addr);}freeifaddrs(list);}
#endif
    if(interfaces.empty())add(htonl(INADDR_LOOPBACK),htonl(0xff000000));
}
bool LanDiscovery::Advertise(const LanRoom& room){
    advertised=room;
    if(server!=netio::Invalid)return true;
    if(!netio::Initialize())return false;
    server=socket(AF_INET,SOCK_DGRAM,0);if(server==netio::Invalid)return false;
    int yes=1;netio::SetOption(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    if(!BindDiscovery(server,DiscoveryPort)){StopHosting();return false;}
    std::random_device random;instance=(uint64_t(random())<<32)|random();if(!instance)instance=1;
    FindInterfaces();
    for(auto i:interfaces){ip_mreq membership{};membership.imr_multiaddr.s_addr=htonl(MulticastAddress);membership.imr_interface.s_addr=i.address;netio::SetOption(server,IPPROTO_IP,IP_ADD_MEMBERSHIP,&membership,sizeof(membership));}
    return true;
}
void LanDiscovery::StopHosting(){if(server!=netio::Invalid)netio::Close(server);server=netio::Invalid;instance=0;}
void LanDiscovery::CancelSearch(){if(scanner!=netio::Invalid)netio::Close(scanner);scanner=netio::Invalid;deadline=0;}
bool LanDiscovery::Search(){
    CancelSearch();rooms.clear();
    if(!netio::Initialize()){status="Falha ao iniciar a rede";return false;}
    scanner=socket(AF_INET,SOCK_DGRAM,0);
    if(scanner==netio::Invalid){status="Falha ao iniciar a busca";return false;}
    int yes=1;netio::SetOption(scanner,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes));
    if(!BindDiscovery(scanner,0)){CancelSearch();status="Falha ao abrir a busca UDP";return false;}
    FindInterfaces();nonce=std::random_device{}();deadline=Now()+5;nextProbe=0;status="Procurando salas nas interfaces de rede...";Probe();return true;
}
bool LanDiscovery::Searching() const{return scanner!=netio::Invalid&&Now()<deadline;}
void LanDiscovery::Probe(){
    ++nonce;probeSentAt=Now();
    unsigned char query[12];std::memcpy(query,"MCLAN00?",8);for(int i=0;i<4;++i)query[8+i]=static_cast<unsigned char>(nonce>>(i*8));
    auto send=[&](uint32_t ip){sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=ip;address.sin_port=htons(DiscoveryPort);sendto(scanner,reinterpret_cast<const char*>(query),sizeof(query),0,reinterpret_cast<sockaddr*>(&address),sizeof(address));};
    // Probe older discovery identifiers as well as the version-independent query.
    for(int version=0;version<=LanProtocolVersion;++version){
        query[5]=static_cast<unsigned char>('0'+version/10);query[6]=static_cast<unsigned char>('0'+version%10);
        send(htonl(INADDR_LOOPBACK));send(INADDR_BROADCAST);
        for(auto i:interfaces){if(i.broadcast!=i.address)send(i.broadcast);netio::SetOption(scanner,IPPROTO_IP,IP_MULTICAST_IF,&i.address,sizeof(i.address));send(htonl(MulticastAddress));}
    }
    nextProbe=Now()+1;
}
void LanDiscovery::Poll(){
    if(server!=netio::Invalid)for(int count=0;count<32;++count){unsigned char data[512];sockaddr_in source{};netio::SocketLength length=sizeof(source);int n=recvfrom(server,reinterpret_cast<char*>(data),sizeof(data),0,reinterpret_cast<sockaddr*>(&source),&length);if(n<0)break;if(n!=12||DiscoveryVersion(data,'?')<0)continue;
        unsigned char reply[512]{};std::memcpy(reply,ReplyMagic,8);std::memcpy(reply+8,data+8,4);reply[12]=advertised.port>>8;reply[13]=advertised.port&255;reply[14]=static_cast<unsigned char>(advertised.players);reply[15]=static_cast<unsigned char>(advertised.gap);reply[16]=advertised.joinable;size_t size=std::min(size_t(32),advertised.name.size());reply[17]=static_cast<unsigned char>(size);for(int i=0;i<8;++i)reply[18+i]=static_cast<unsigned char>(instance>>(i*8));reply[26]=static_cast<unsigned char>(advertised.capacity);std::memcpy(reply+27,advertised.name.data(),size);size_t used=27+size;
        auto text=[&](const std::string& value,size_t limit){size_t count=std::min(value.size(),limit);reply[used++]=static_cast<unsigned char>(count);std::memcpy(reply+used,value.data(),count);used+=count;};
        text(advertised.map,64);size_t players=std::min(size_t(16),advertised.playerNames.size());reply[used++]=static_cast<unsigned char>(players);for(size_t i=0;i<players;++i)text(advertised.playerNames[i],16);
        sendto(server,reinterpret_cast<const char*>(reply),int(used),0,reinterpret_cast<sockaddr*>(&source),length);
    }
    if(scanner==netio::Invalid)return;
    if(Now()>=deadline){CancelSearch();status=rooms.empty()?"Nenhuma sala encontrada. Voce ainda pode informar o IP manualmente.":"Busca concluida. Selecione uma sala para ver os detalhes.";return;}
    if(Now()>=nextProbe)Probe();
    for(int count=0;count<64;++count){unsigned char data[512];sockaddr_in source{};netio::SocketLength length=sizeof(source);int n=recvfrom(scanner,reinterpret_cast<char*>(data),sizeof(data),0,reinterpret_cast<sockaddr*>(&source),&length);if(n<0)break;if(n<27||DiscoveryVersion(data,'!')<1)continue;uint32_t token=0;for(int i=0;i<4;++i)token|=uint32_t(data[8+i])<<(i*8);if(token!=nonce||data[17]>32||n<27+data[17]||data[14]<1||data[14]>data[26]||(data[26]!=2&&data[26]!=4&&data[26]!=8&&data[26]!=16)||(data[15]!=16&&data[15]!=32&&data[15]!=64)||data[16]>1)continue;
        LanRoom room;room.protocol=DiscoveryVersion(data,'!');room.port=static_cast<unsigned short>((unsigned(data[12])<<8)|data[13]);if(!room.port)continue;char ip[INET_ADDRSTRLEN];if(!inet_ntop(AF_INET,&source.sin_addr,ip,sizeof(ip)))continue;room.address=std::string(ip)+":"+std::to_string(room.port);for(int i=0;i<8;++i)room.identity|=uint64_t(data[18+i])<<(i*8);if(!room.identity)continue;room.name.assign(reinterpret_cast<char*>(data+27),data[17]);if(room.name.empty()||std::any_of(room.name.begin(),room.name.end(),[](unsigned char c){return c<32||c>126;}))continue;room.players=data[14];room.gap=data[15];room.capacity=data[26];room.joinable=data[16]&&room.players<room.capacity;
        room.ping=std::max(0,int((Now()-probeSentAt)*1000));
        size_t offset=27+data[17];bool valid=true;
        auto text=[&](size_t limit){std::string value;if(offset>=size_t(n)){valid=false;return value;}size_t length=data[offset++];if(length>limit||offset+length>size_t(n)){valid=false;return value;}value.assign(reinterpret_cast<char*>(data+offset),length);offset+=length;for(unsigned char c:value)if(c<32||c>126)valid=false;return value;};
        if(offset<size_t(n)){room.map=text(64);if(!valid||offset>=size_t(n))continue;int count=data[offset++];if(count>16||count>room.players)continue;for(int i=0;i<count;++i){auto name=text(16);if(name.empty()||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos)valid=false;room.playerNames.push_back(name);}if(!valid||offset!=size_t(n))continue;}
        auto found=std::find_if(rooms.begin(),rooms.end(),[&](const LanRoom& r){return r.identity==room.identity&&r.port==room.port;});if(found!=rooms.end()){room.address=found->address;*found=room;}else if(rooms.size()<64)rooms.push_back(room);
    }
}
}
