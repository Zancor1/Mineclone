#include "network.h"
#include "network_progress.h"
#include "dns_srv.h"
#include "catalog18.h"
#include "chest.h"
#include "survival.h"
#include "chat_text.h"
#include <raymath.h>
#include <rlgl.h>
#include <zlib.h>
#include "platform_socket.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <deque>
#include <map>
#include <array>
#include <memory>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <fstream>
namespace game::net {
namespace {
using Bytes=std::vector<unsigned char>;
constexpr size_t Limit=4*1024*1024;
struct Reader {
    const Bytes& b;size_t p=0;
    void need(size_t n)const {if(n>b.size()-std::min(p,b.size()))throw std::runtime_error("Pacote incompleto");}
    unsigned char byte(){need(1);return b[p++];}
    uint16_t word(){uint16_t a=byte();return uint16_t((a<<8)|byte());}
    uint32_t integer(){uint32_t a=word();return (a<<16)|word();}
    uint64_t longword(){uint64_t a=integer();return(a<<32)|integer();}
    int var(){uint32_t v=0;for(int i=0;i<5;++i){unsigned c=byte();if(i==4&&(c&240))throw std::runtime_error("VarInt invalido");v|=(c&127)<<(7*i);if(!(c&128))return int32_t(v);}throw std::runtime_error("VarInt invalido");}
    std::string string(){int n=var();if(n<0||n>32767)throw std::runtime_error("Texto invalido");need(n);std::string s(b.begin()+p,b.begin()+p+n);p+=n;return s;}
    float real(){uint32_t v=integer();float f;std::memcpy(&f,&v,4);return f;}
    double decimal(){uint64_t v=longword();double d;std::memcpy(&d,&v,8);return d;}
    void skip(size_t n){need(n);p+=n;}
};
struct Writer {
    Bytes b;
    void byte(int v){b.push_back(static_cast<unsigned char>(v));}
    void word(int v){byte(unsigned(v)>>8);byte(v);}
    void integer(uint32_t v){word(v>>16);word(v);}
    void longword(uint64_t v){integer(v>>32);integer(v);}
    void var(int32_t i){uint32_t v=uint32_t(i);do{unsigned c=v&127;v>>=7;byte(c|(v?128:0));}while(v);}
    void string(const std::string& s){var(int(s.size()));b.insert(b.end(),s.begin(),s.end());}
    void real(float f){uint32_t v;std::memcpy(&v,&f,4);integer(v);}
    void decimal(double d){uint64_t v;std::memcpy(&v,&d,8);longword(v);}
};
std::atomic<bool> running{false},playing{false},worldReady{false};
std::atomic<netio::Socket> socketFd{netio::Invalid};
std::thread worker;
std::mutex mutex;
std::deque<Bytes> incoming,outgoing;
size_t queued=0;
std::string status="Desconectado";
std::map<std::pair<int,int>,std::vector<uint16_t>> chunks;
Vector3 serverCenter{};
std::ofstream diagnostics;
unsigned correctionCount=0,chunkCountReceived=0,movementCount=0;
struct Entity {Vector3 position{};float yaw=0;int type=0;bool invisible=false,nameVisible=false,small=false;std::string name,uuid;};
std::map<int,Entity> entities;
std::vector<std::string> messages;
std::map<std::string,PlayerEntry> playerList;
std::string tabHeader,tabFooter,sessionNick;
struct Team {std::string prefix,suffix;std::vector<std::string> names;};
std::map<std::string,Team> teams;
std::map<int,MapData> maps;
std::map<int,MapFrame> mapFrames;
uint64_t mapRevision=0;

std::vector<Slot> playerSlots(45),windowSlots;
Slot cursor;
std::string title="Inventario";
int windowId=0,ownId=0,originY=0,dimension=0,selected=0;short transaction=0;
float movementAttribute=.1f;
bool joined=false,windowOpen=false,sprinting=false,sneaking=false,allowFlight=false,lastFlying=false;
void SetStatus(const std::string& text){std::lock_guard<std::mutex> lock(mutex);status=text.substr(0,240);}
void Send(Writer w){std::lock_guard<std::mutex> lock(mutex);if(outgoing.size()<512)outgoing.push_back(std::move(w.b));}
Writer Packet(int id){Writer w;w.var(id);return w;}
std::string Text(const std::string& json){return PlainMinecraftText(json);}
std::string Uuid(Reader& r){r.need(16);static const char* digits="0123456789abcdef";std::string id;for(int i=0;i<16;++i){unsigned c=r.byte();id+=digits[c>>4];id+=digits[c&15];}return id;}
void Message(const std::string& text){messages.push_back(text.substr(0,4096));if(messages.size()>8)messages.erase(messages.begin());}
bool WriteAll(netio::Socket fd,const Bytes& data){size_t p=0;while(p<data.size()&&running){int n=netio::Send(fd,data.data()+p,data.size()-p);if(n<=0)return false;p+=size_t(n);}return p==data.size();}
void Frame(netio::Socket fd,const Bytes& body,int compression){
    Writer payload;
    if(compression>=0){if(int(body.size())>=compression){uLongf n=compressBound(body.size());Bytes zipped(n);if(compress2(zipped.data(),&n,body.data(),body.size(),Z_DEFAULT_COMPRESSION)!=Z_OK)throw std::runtime_error("Falha de compressao");payload.var(int(body.size()));payload.b.insert(payload.b.end(),zipped.begin(),zipped.begin()+n);}else{payload.var(0);payload.b.insert(payload.b.end(),body.begin(),body.end());}}
    else payload.b=body;
    Writer frame;frame.var(int(payload.b.size()));frame.b.insert(frame.b.end(),payload.b.begin(),payload.b.end());
    if(!WriteAll(fd,frame.b))throw std::runtime_error("Conexao encerrada ao enviar");
}
netio::Socket OpenSocket(const std::string& host,int port,bool useSrv){
    if(!netio::Initialize())throw std::runtime_error("Falha ao inicializar rede Windows");
    addrinfo hints{};hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;addrinfo* result=nullptr;
    struct Resolution {std::atomic<bool> done{false};addrinfo* result=nullptr;int error=0;~Resolution(){if(result)freeaddrinfo(result);}};
    auto resolution=std::make_shared<Resolution>();
    std::thread([resolution,host,port,hints,useSrv]{auto srv=useSrv?LookupMinecraftSrv(host):SrvRecord{};std::string target=srv.host.empty()?host:srv.host;int targetPort=srv.host.empty()?port:srv.port;resolution->error=getaddrinfo(target.c_str(),std::to_string(targetPort).c_str(),&hints,&resolution->result);resolution->done.store(true,std::memory_order_release);}).detach();
    for(int t=0;t<200&&running&&!resolution->done.load(std::memory_order_acquire);++t)std::this_thread::sleep_for(std::chrono::milliseconds(30));
    if(!running||!resolution->done.load(std::memory_order_acquire)||resolution->error)throw std::runtime_error("Endereco nao encontrado ou DNS sem resposta");
    result=resolution->result;
    netio::Socket fd=netio::Invalid;
    for(auto* a=result;a&&running;a=a->ai_next){fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);if(fd==netio::Invalid)continue;
        socketFd=fd;if(!netio::SetNonblocking(fd,true)){socketFd=netio::Invalid;netio::Close(fd);fd=netio::Invalid;continue;}
        int err=connect(fd,a->ai_addr,a->ai_addrlen);netio::PollFd event{fd,POLLOUT,0};
        bool ready=err==0;
        for(int t=0;!ready&&t<30&&running;++t){if(netio::Poll(&event,1,100)>0){int value=netio::SocketError(fd);ready=value==0;break;}}
        if(ready){netio::SetNonblocking(fd,false);netio::SendTimeout(fd);break;}
        socketFd=netio::Invalid;netio::Close(fd);fd=netio::Invalid;
    }
    if(fd==netio::Invalid)throw std::runtime_error("Nao foi possivel conectar");
    return fd;
}
void Worker(std::string host,int port,std::string name,bool useSrv){
    netio::Socket fd=netio::Invalid;
    try{
        SetStatus("Conectando ao servidor...");fd=OpenSocket(host,port,useSrv);
        Writer handshake=Packet(0);handshake.var(47);handshake.string(host+std::string("\0FML\0",5));handshake.word(port);handshake.var(2);Frame(fd,handshake.b,-1);
        Writer login=Packet(0);login.string(name);Frame(fd,login.b,-1);SetStatus("Aguardando login Java 1.8...");
        Bytes buffer;int compression=-1;bool play=false;
        auto last=std::chrono::steady_clock::now();ConnectionProgress progress;
        while(running){
            if(worldReady.load())progress.Ready();
            if(const char* timeout=progress.Timeout())throw std::runtime_error(timeout);
            std::deque<Bytes> sendQueue;{std::lock_guard<std::mutex> lock(mutex);sendQueue.swap(outgoing);}
            for(auto& b:sendQueue)Frame(fd,b,compression);
            netio::PollFd event{fd,POLLIN,0};int ready=netio::Poll(&event,1,20);if(ready<0)throw std::runtime_error("Falha de leitura");
            if(!ready){if(std::chrono::steady_clock::now()-last>std::chrono::seconds(35))throw std::runtime_error("Tempo de conexao esgotado");continue;}
            unsigned char data[16384];int n=netio::Receive(fd,data,sizeof(data));if(n<=0)throw std::runtime_error("Servidor encerrou a conexao");last=std::chrono::steady_clock::now();buffer.insert(buffer.end(),data,data+n);
            if(buffer.size()>Limit*2)throw std::runtime_error("Buffer de rede excedido");
            for(;;){
                uint32_t length=0;size_t prefix=0;bool complete=false;
                for(;prefix<buffer.size()&&prefix<5;++prefix){unsigned c=buffer[prefix];if(prefix==4&&(c&240))throw std::runtime_error("Comprimento invalido");length|=(c&127)<<(prefix*7);if(!(c&128)){++prefix;complete=true;break;}}
                if(!complete){if(prefix==5)throw std::runtime_error("VarInt de frame invalido");break;}
                if(length>Limit)throw std::runtime_error("Pacote muito grande");
                if(buffer.size()<prefix+length)break;
                Bytes body(buffer.begin()+prefix,buffer.begin()+prefix+length);buffer.erase(buffer.begin(),buffer.begin()+prefix+length);
                if(compression>=0){Reader r{body};int raw=r.var();if(raw<0||raw>int(Limit))throw std::runtime_error("Compressao invalida");if(raw){Bytes decoded(raw);uLongf size=raw;if(uncompress(decoded.data(),&size,body.data()+r.p,body.size()-r.p)!=Z_OK||size!=unsigned(raw))throw std::runtime_error("Pacote comprimido invalido");body=std::move(decoded);}else body.erase(body.begin(),body.begin()+r.p);}
                Reader r{body};int id=r.var();
                if(!play){
                    if(id==0)throw std::runtime_error(Text(r.string()));
                    if(id==1)throw std::runtime_error("Servidor pediu criptografia de login (RSA/AES), ainda nao suportada. Conta offline no launcher nao elimina esse pedido.");
                    if(id==3){compression=r.var();if(compression<0||compression>int(Limit))throw std::runtime_error("Limite de compressao invalido");continue;}
                    if(id==2){r.string();{std::lock_guard<std::mutex> lock(mutex);sessionNick=r.string();}play=true;playing=true;progress.Accepted();SetStatus("Login aceito; recebendo mundo...");}
                    continue;
                }
                if(id==0){Frame(fd,body,compression);continue;}
                if(id==0x46){compression=r.var();if(compression<0||compression>int(Limit))throw std::runtime_error("Compressao invalida");continue;}
                if(id==0x40)throw std::runtime_error(Text(r.string()));
                {std::lock_guard<std::mutex> lock(mutex);if(queued+body.size()>Limit*8)throw std::runtime_error("Fila de pacotes excedida");queued+=body.size();incoming.push_back(std::move(body));}
            }
        }
    }catch(const std::exception& e){if(running)SetStatus(e.what());}
    running=false;playing=false;
    if(fd!=netio::Invalid){socketFd=netio::Invalid;netio::Close(fd);}
}
void Nbt(Reader& r,int type,int depth=0){
    if(depth>32)throw std::runtime_error("NBT profundo demais");
    auto length=[&](){int32_t n=int32_t(r.integer());if(n<0||n>1000000)throw std::runtime_error("NBT invalido");return size_t(n);};
    switch(type){case 0:return;case 1:r.skip(1);return;case 2:r.skip(2);return;case 3:case 5:r.skip(4);return;case 4:case 6:r.skip(8);return;
        case 7:r.skip(length());return;case 8:r.skip(r.word());return;case 9:{int t=r.byte();size_t n=length();for(size_t i=0;i<n;++i)Nbt(r,t,depth+1);return;}
        case 10:for(;;){int t=r.byte();if(!t)return;r.skip(r.word());Nbt(r,t,depth+1);}case 11:r.skip(length()*4);return;default:throw std::runtime_error("Tipo NBT invalido");}
}
Slot ReadSlot(Reader& r){size_t start=r.p;Slot s;s.id=short(r.word());if(s.id>=0){s.count=r.byte();s.damage=short(r.word());int tag=r.byte();if(tag){r.skip(r.word());Nbt(r,tag);}if(s.count>64)throw std::runtime_error("Pilha invalida");}s.wire.assign(r.b.begin()+start,r.b.begin()+r.p);return s;}
ItemStack LocalStack(const Slot& s){Block b=LocalItem(s);if(s.id<0||!s.count)return {};return {b,static_cast<unsigned char>(std::min(int(s.count),StackCapacity(b))),static_cast<unsigned short>(ToolDurability(b)>0?std::clamp(int(s.damage),0,ToolDurability(b)-1):0)};}
Block WorldBlock(uint16_t state){
    static const auto mapping=[] {
        std::array<Block,65536> map;map.fill(Stone);
        for(const auto& e:Catalog18)if(e.legacyId<256&&e.metadata==0&&e.available&&Placeable(e.block))for(int m=0;m<16;++m)map[e.legacyId*16+m]=e.block;
        for(const auto& e:Catalog18)if(e.legacyId<256&&e.metadata>=0&&e.metadata<16&&e.available&&Placeable(e.block))map[e.legacyId*16+e.metadata]=e.block;
        for(int m=0;m<16;++m){map[m]=Air;map[8*16+m]=map[9*16+m]=Water;map[10*16+m]=map[11*16+m]=Lava;map[26*16+m]=m&8?BedHead:Bed;for(int id:{50,75,76})map[id*16+m]=TallGrass;for(int id:{63,68,90,119})map[id*16+m]=Air;}
        return map;
    }();
    return mapping[state];
}
Vector3 Local(Vector3 p){return {p.x-RegionOriginX,p.y-originY,p.z-RegionOriginZ};}
Vector3 Global(Vector3 p){return {p.x+RegionOriginX,p.y+originY,p.z+RegionOriginZ};}
void ApplyChunk(int cx,int cz,const std::vector<uint16_t>& states){
    int x=cx*16-RegionOriginX,z=cz*16-RegionOriginZ;if(x<0||z<0||x+16>Width||z+16>Width)return;
    std::vector<Block> blocks(16*Height*16);
    for(int lx=0;lx<16;++lx)for(int y=0;y<Height;++y)for(int lz=0;lz<16;++lz){int sy=y+originY;blocks[(lx*Height+y)*16+lz]=sy>=0&&sy<256?WorldBlock(states[(sy*16+lz)*16+lx]):Air;}
    ApplyRemoteChunk(x/16,z/16,blocks);
}
void UpdateCell(int x,int y,int z,uint16_t state){
    if(y<0||y>=256)return;
    int cx=int(std::floor(x/16.f)),cz=int(std::floor(z/16.f));auto it=chunks.find({cx,cz});
    if(it!=chunks.end())it->second[(y*16+(z&15))*16+(x&15)]=state;
    SetRemoteBlock(x-RegionOriginX,y-originY,z-RegionOriginZ,WorldBlock(state));
}
void Metadata(Reader& r,int entity) {
    auto e=entities.find(entity);auto frame=mapFrames.find(entity);
    for(int count=0;count<64;++count){int h=r.byte();if(h==127)return;int type=h>>5,index=h&31;
        switch(type){
        case 0:{int value=r.byte();if(e!=entities.end()){if(index==0)e->second.invisible=value&32;if(index==3)e->second.nameVisible=value!=0;if(index==10)e->second.small=value&1;}if(frame!=mapFrames.end()&&index==9)frame->second.rotation=value&7;break;}
        case 1:r.word();break;case 2:case 3:r.integer();break;
        case 4:{auto name=r.string();if(e!=entities.end()&&index==2)e->second.name=name;break;}
        case 5:{Slot slot=ReadSlot(r);if(frame!=mapFrames.end()&&index==8)frame->second.mapId=slot.id==358?uint16_t(slot.damage):-1;break;}
        case 6:case 7:r.skip(12);break;default:throw std::runtime_error("Metadados invalidos");
        }
    }
    throw std::runtime_error("Metadados sem terminador");
}
void ClearRemote(){movementAttribute=.1f;worldReady=false;ClearRemoteWorld();chunks.clear();entities.clear();mapFrames.clear();maps.clear();joined=false;windowOpen=false;windowSlots.clear();playerSlots.assign(45,{});cursor={};}
void Recenter(Player& player,Vector3 global){
    RegionOriginX=int(std::floor(global.x/16))*16-Width/2;RegionOriginZ=int(std::floor(global.z/16))*16-Width/2;
    originY=std::clamp(int(std::floor(global.y/16))*16-32,0,256-Height);ClearRemoteWorld(false);
    for(auto& entry:chunks)ApplyChunk(entry.first.first,entry.first.second,entry.second);
    auto pose=player.CaptureState();pose.position=Local(global);pose.valid=true;player.RestoreState(pose);
}
void Chunk(int cx,int cz,int mask,bool full,Reader& r,bool sky){
    if(std::abs(int64_t(cx))>6250000||std::abs(int64_t(cz))>6250000)throw std::runtime_error("Coordenada de chunk invalida");
    if(chunks.size()>=1024&&chunks.find({cx,cz})==chunks.end()){
        auto distance=[](const auto& item){double dx=item.first.first*16.0+8-serverCenter.x,dz=item.first.second*16.0+8-serverCenter.z;return dx*dx+dz*dz;};
        auto farthest=std::max_element(chunks.begin(),chunks.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});chunks.erase(farthest);
    }
    auto& states=chunks[{cx,cz}];if(states.empty()||full)states.assign(65536,0);
    int sections=0;
    for(int section=0;section<16;++section)if(mask&(1<<section)){++sections;for(int i=0;i<4096;++i){uint16_t lo=r.byte(),hi=r.byte();states[section*4096+i]=lo|(hi<<8);}}
    r.skip(size_t(sections)*2048*(sky?2:1));if(full)r.skip(256);ApplyChunk(cx,cz,states);
    if(diagnostics&&(++chunkCountReceived<=20||chunkCountReceived%100==0))diagnostics<<"chunk "<<cx<<' '<<cz<<" mask="<<mask<<" full="<<full<<" cache="<<chunks.size()<<std::endl;
}
void SyncInventory(){
    if(windowOpen&&windowId>0&&windowSlots.size()>=36)for(int i=0;i<36;++i)playerSlots[9+i]=windowSlots[windowSlots.size()-36+i];
    for(int i=0;i<27;++i)Backpack[i]=LocalStack(playerSlots[9+i]);
    for(int i=0;i<9;++i)Hotbar[i]=LocalStack(playerSlots[36+i]);
    for(int i=0;i<4;++i)EquippedArmor[i]=LocalStack(playerSlots[5+i]);
    CursorStack=LocalStack(cursor);if(windowId==0)windowSlots=playerSlots;
}
void Position(const Player& player){auto pose=player.CaptureState();Vector3 p=Global(pose.position);Writer w=Packet(6);w.decimal(p.x);w.decimal(p.y);w.decimal(p.z);w.real((pose.yaw-PI)*RAD2DEG);w.real(-pose.pitch*RAD2DEG);w.byte(player.OnGround());Send(std::move(w));}
enum class ForgePhase {Hello,ModList,Registry,ServerAck,CompleteAck,Done};
ForgePhase forgePhase=ForgePhase::Hello;
void ForgePayload(Writer body){auto w=Packet(0x17);w.string("FML|HS");w.b.insert(w.b.end(),body.b.begin(),body.b.end());Send(std::move(w));}
void ForgeAck(int phase){Writer w;w.byte(255);w.byte(phase);ForgePayload(std::move(w));}
void ForgeHandshake(Reader& r){
    int type=r.byte();
    if(type==254){forgePhase=ForgePhase::Hello;return;}
    if(type==0&&forgePhase==ForgePhase::Hello){
        int protocol=r.byte();if(protocol<1||protocol>2)throw std::runtime_error("Versao FML nao suportada (apenas Forge 1.8.x)");
        if(protocol>1){int dim=int32_t(r.integer());if(dim!=0&&dim!=-1&&dim!=1)throw std::runtime_error("Forge exige dimensao de mod nao implementada");}
        auto registration=Packet(0x17);registration.string("REGISTER");std::string channels("FML|HS\0FML\0FML|MP",17);registration.b.insert(registration.b.end(),channels.begin(),channels.end());Send(std::move(registration));
        Writer hello;hello.byte(1);hello.byte(2);ForgePayload(std::move(hello));
        Writer mods;mods.byte(2);mods.var(0);ForgePayload(std::move(mods));forgePhase=ForgePhase::ModList;return;
    }
    if(type==2&&forgePhase==ForgePhase::ModList){
        int count=r.var();if(count<0||count>4096)throw std::runtime_error("Lista FML invalida");
        for(int i=0;i<count;++i){r.string();r.string();}
        ForgeAck(2);forgePhase=ForgePhase::Registry;return;
    }
    if(type==3&&forgePhase==ForgePhase::Registry){
        bool more=r.byte()!=0;std::string registry=r.string();int count=r.var();if(count<0||count>65536)throw std::runtime_error("Registro FML invalido");
        struct VanillaId{const char* registry;const char* name;int id;};
        static constexpr VanillaId vanilla[]={
#include "vanilla_registry.inc"
        };
        for(int i=0;i<count;++i){std::string name=r.string();int id=r.var();bool found=false;
            for(const auto& entry:vanilla)if(registry==entry.registry&&name==entry.name&&id==entry.id){found=true;break;}
            if(!found)throw std::runtime_error("Forge exige registro nao suportado: "+name+". Use os mods correspondentes no cliente Minecraft Java.");
        }
        int substitutions=r.var();if(substitutions!=0)throw std::runtime_error("Forge exige substituicoes de mods nao implementadas");
        if(r.p<r.b.size()){int dummies=r.var();if(dummies!=0)throw std::runtime_error("Forge exige registros de mods ausentes");}
        if(!more){ForgeAck(3);forgePhase=ForgePhase::ServerAck;}return;
    }
    if(type==255){int phase=r.byte();if(forgePhase==ForgePhase::ServerAck&&phase==2){ForgeAck(4);forgePhase=ForgePhase::CompleteAck;return;}if(forgePhase==ForgePhase::CompleteAck&&phase==3){ForgeAck(5);forgePhase=ForgePhase::Done;return;}}
    throw std::runtime_error("Sequencia de handshake Forge nao suportada");
}
void Handle(Bytes& bytes,Player& player){
    Reader r{bytes};int id=r.var();
    switch(id){
        case 1:{ownId=int(r.integer());int mode=r.byte()&7;dimension=int8_t(r.byte());Mode=mode==1?GameMode::Creative:GameMode::Survival;
            Writer brand=Packet(0x17);brand.string("MC|Brand");brand.string("fml,forge");Send(std::move(brand));break;}
        case 2:{std::string text=Text(r.string());int position=r.byte();if(position!=2)Message(text);break;}
        case 3:{r.longword();int64_t ticks=int64_t(r.longword());WorldTime=float(std::abs(ticks%24000))/20.f;break;}
        case 6:{float health=r.real();r.var();r.real();if(!std::isfinite(health)||health<0||health>2048)throw std::runtime_error("Vida invalida");Health=std::min(health,20.f);break;}
        case 7:{if(diagnostics)diagnostics<<"respawn: clearing remote world"<<std::endl;chunkCountReceived=0;dimension=int32_t(r.integer());r.byte();Mode=r.byte()==1?GameMode::Creative:GameMode::Survival;ClearRemote();break;}
        case 8:{double x=r.decimal(),y=r.decimal(),z=r.decimal();float yaw=r.real(),pitch=r.real();int flags=r.byte();if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(yaw)||!std::isfinite(pitch)||std::abs(x)>100000000||std::abs(z)>100000000||std::abs(y)>1000000)throw std::runtime_error("Posicao invalida");auto pose=player.CaptureState();Vector3 before=Global(pose.position);Vector3 target{float(x)+(flags&1?before.x:0),float(y)+(flags&2?before.y:0),float(z)+(flags&4?before.z:0)};
            serverCenter=target;++correctionCount;
            if(diagnostics&&(correctionCount<=30||correctionCount%100==0))diagnostics<<"position "<<target.x<<' '<<target.y<<' '<<target.z<<" previous="<<before.x<<','<<before.y<<','<<before.z<<" chunks="<<chunks.size()<<std::endl;
            bool center=!joined||target.x-RegionOriginX<48||target.x-RegionOriginX>Width-48||target.z-RegionOriginZ<48||target.z-RegionOriginZ>Width-48||target.y-originY<8||target.y-originY>Height-16;
            if(center)Recenter(player,target);
            pose.position=Local(target);pose.yaw=(flags&8?pose.yaw:PI)+yaw*DEG2RAD;pose.pitch=std::clamp((flags&16?pose.pitch:0)-pitch*DEG2RAD,-1.55f,1.55f);pose.velocityY=0;pose.valid=true;player.RestoreState(pose);joined=true;SetStatus("Conectado (Java 1.8 experimental)");Position(player);
            Writer settings=Packet(0x15);settings.string("pt_BR");settings.byte(4);settings.var(0);settings.byte(1);settings.byte(127);Send(std::move(settings));break;}
        case 9:selected=r.byte();if(selected>8)throw std::runtime_error("Hotbar invalida");break;
        case 0x0c:{int entity=r.var();Entity e;e.uuid=Uuid(r);e.position={int32_t(r.integer())/32.f,int32_t(r.integer())/32.f,int32_t(r.integer())/32.f};e.yaw=r.byte()*2*PI/256;entities[entity]=e;if(entities.size()>512)entities.erase(entities.begin());break;}
        case 0x0e:{int entity=r.var(),type=r.byte();Vector3 p{int32_t(r.integer())/32.f,int32_t(r.integer())/32.f,int32_t(r.integer())/32.f};r.byte();r.byte();int direction=int32_t(r.integer());if(type==71&&mapFrames.size()<512)mapFrames[entity]={p,-1,direction&3,0};break;}
        case 0x0f:{int entity=r.var();Entity e;e.type=r.byte();e.position={int32_t(r.integer())/32.f,int32_t(r.integer())/32.f,int32_t(r.integer())/32.f};e.yaw=r.byte()*2*PI/256;r.skip(8);if(entities.size()<2048)entities[entity]=e;Metadata(r,entity);break;}
        case 0x1c:{int entity=r.var();Metadata(r,entity);break;}
        case 0x12:{int entity=r.var();Vector3 velocity{int16_t(r.word())/400.f,int16_t(r.word())/400.f,int16_t(r.word())/400.f};if(entity==ownId)player.SetRemoteVelocity(velocity);break;}
        case 0x13:{int count=r.var();if(count<0||count>2048)throw std::runtime_error("Entidades invalidas");while(count--){int entity=r.var();entities.erase(entity);mapFrames.erase(entity);}break;}
        case 0x15:case 0x17:{int entity=r.var();Vector3 delta{int8_t(r.byte())/32.f,int8_t(r.byte())/32.f,int8_t(r.byte())/32.f};if(auto it=entities.find(entity);it!=entities.end())it->second.position=Vector3Add(it->second.position,delta);break;}
        case 0x18:{int entity=r.var();Vector3 p{int32_t(r.integer())/32.f,int32_t(r.integer())/32.f,int32_t(r.integer())/32.f};if(auto it=entities.find(entity);it!=entities.end())it->second.position=p;break;}
        case 0x20:{int entity=r.var(),count=int32_t(r.integer());if(count<0||count>1024)throw std::runtime_error("Atributos invalidos");
            for(int i=0;i<count;++i){std::string key=r.string();double base=r.decimal(),add=0,multiplyBase=0,multiply=1;int modifiers=r.var();if(!std::isfinite(base)||modifiers<0||modifiers>1024)throw std::runtime_error("Atributo invalido");
                for(int j=0;j<modifiers;++j){std::string uuid=Uuid(r);double amount=r.decimal();int operation=r.byte();if(!std::isfinite(amount)||operation>2)throw std::runtime_error("Modificador invalido");
                    if(uuid=="662a6b8dda3e4c1c881396ea6097278d")continue;
                    if(operation==0)add+=amount;else if(operation==1)multiplyBase+=amount;else multiply*=1+amount;
                }
                if(entity==ownId&&key=="generic.movementSpeed"){double speed=(base+add)*(1+multiplyBase)*multiply;if(!std::isfinite(speed))throw std::runtime_error("Velocidade invalida");movementAttribute=float(std::clamp(speed,0.0,10.0));if(diagnostics)diagnostics<<"movement attribute="<<movementAttribute<<std::endl;}
            }break;}
        case 0x21:{int cx=int32_t(r.integer()),cz=int32_t(r.integer());if(std::abs(int64_t(cx))>6250000||std::abs(int64_t(cz))>6250000)throw std::runtime_error("Coordenada de chunk invalida");bool full=r.byte()!=0;int mask=r.word();int length=r.var();if(length<0||length>int(Limit))throw std::runtime_error("Chunk invalido");r.need(length);if(!mask&&full){chunks.erase({cx,cz});std::vector<uint16_t> empty(65536);ApplyChunk(cx,cz,empty);}else Chunk(cx,cz,mask,full,r,dimension==0);break;}
        case 0x26:{bool sky=r.byte()!=0;int count=r.var();if(count<0||count>1024)throw std::runtime_error("Chunks invalidos");struct Meta{int x,z,mask;};std::vector<Meta> list;for(int i=0;i<count;++i)list.push_back({int32_t(r.integer()),int32_t(r.integer()),int(r.word())});for(auto m:list)Chunk(m.x,m.z,m.mask,true,r,sky);break;}
        case 0x22:{int cx=int32_t(r.integer()),cz=int32_t(r.integer()),count=r.var();if(count<0||count>65536)throw std::runtime_error("Atualizacoes invalidas");for(int i=0;i<count;++i){int packed=r.byte(),y=r.byte(),state=r.var(),x=(packed>>4)&15,z=packed&15;auto it=chunks.find({cx,cz});if(it!=chunks.end())it->second[(y*16+z)*16+x]=uint16_t(state);SetRemoteBlock(cx*16+x-RegionOriginX,y-originY,cz*16+z-RegionOriginZ,WorldBlock(uint16_t(state)));}break;}
        case 0x23:{uint64_t pos=r.longword();int x=int32_t(pos>>38);if(x&0x2000000)x|=~0x3ffffff;int z=int(pos&0x3ffffff);if(z&0x2000000)z|=~0x3ffffff;int y=int((pos>>26)&4095);if(y&2048)y|=~4095;int state=r.var();int cx=int(std::floor(x/16.f)),cz=int(std::floor(z/16.f));if(y>=0&&y<256){auto it=chunks.find({cx,cz});if(it!=chunks.end())it->second[(y*16+(z&15))*16+(x&15)]=uint16_t(state);SetRemoteBlock(x-RegionOriginX,y-originY,z-RegionOriginZ,WorldBlock(uint16_t(state)));}break;}
        case 0x27:{float x=r.real(),y=r.real(),z=r.real();r.real();int count=int32_t(r.integer());if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||std::abs(x)>100000000||std::abs(y)>1000000||std::abs(z)>100000000||count<0||count>65536)throw std::runtime_error("Explosao invalida");for(int i=0;i<count;++i){int dx=int8_t(r.byte()),dy=int8_t(r.byte()),dz=int8_t(r.byte());UpdateCell(int(std::floor(x))+dx,int(std::floor(y))+dy,int(std::floor(z))+dz,0);}Vector3 push{r.real()*20,r.real()*20,r.real()*20};if(!std::isfinite(push.x)||!std::isfinite(push.y)||!std::isfinite(push.z)||Vector3Length(push)>1000)throw std::runtime_error("Impulso invalido");player.Knockback(push);break;}
        case 0x2b:{int reason=r.byte();float value=r.real();if(reason==3)Mode=value==1?GameMode::Creative:GameMode::Survival;break;}
        case 0x2d:{windowId=r.byte();r.string();title=Text(r.string());int count=r.byte();if(count>90)throw std::runtime_error("Janela invalida");windowSlots.assign(count+36,{});windowOpen=true;break;}
        case 0x2e:windowOpen=false;windowId=0;break;
        case 0x2f:{int window=int8_t(r.byte()),slot=int16_t(r.word());Slot item=ReadSlot(r);if(window==-1&&slot==-1)cursor=item;else if(window==0&&slot>=0&&slot<45)playerSlots[slot]=item;else if(window==windowId&&slot>=0&&slot<int(windowSlots.size()))windowSlots[slot]=item;SyncInventory();break;}
        case 0x30:{int window=r.byte(),count=int16_t(r.word());if(count<0||count>256)throw std::runtime_error("Inventario invalido");std::vector<Slot> slots;for(int i=0;i<count;++i)slots.push_back(ReadSlot(r));if(window==0&&count==45)playerSlots=slots;else if(window==windowId)windowSlots=std::move(slots);SyncInventory();break;}
        case 0x32:{int window=r.byte();int action=r.word();bool accepted=r.byte()!=0;if(!accepted){auto w=Packet(0x0f);w.byte(window);w.word(action);w.byte(1);Send(std::move(w));}break;}
        case 0x34:{int mapId=r.var(),scale=r.byte(),count=r.var();if(mapId<0||scale>4||count<0||count>256)throw std::runtime_error("Mapa invalido");
            std::vector<MapIcon> icons;for(int i=0;i<count;++i){int packed=r.byte();icons.push_back({packed>>4,packed&15,int8_t(r.byte()),int8_t(r.byte())});}
            int columns=r.byte(),rows=0,x=0,y=0;Bytes pixels;
            if(columns){rows=r.byte();x=r.byte();y=r.byte();int length=r.var();if(columns>128||rows<1||rows>128||x+columns>128||y+rows>128||length!=columns*rows)throw std::runtime_error("Regiao de mapa invalida");r.need(length);pixels.assign(r.b.begin()+r.p,r.b.begin()+r.p+length);r.skip(length);}
            if(maps.size()>=64&&maps.find(mapId)==maps.end())maps.erase(maps.begin());
            auto& map=maps[mapId];map.id=mapId;map.scale=scale;map.icons=std::move(icons);
            for(int row=0;row<rows;++row)for(int col=0;col<columns;++col)map.pixels[(y+row)*128+x+col]=pixels[row*columns+col];
            map.revision=++mapRevision;break;}
        case 0x38:{int action=r.var(),count=r.var();if(action<0||action>4||count<0||count>2048)throw std::runtime_error("Lista de jogadores invalida");
            for(int i=0;i<count;++i){std::string uuid=Uuid(r);if(action==4){playerList.erase(uuid);continue;}PlayerEntry entry;auto it=playerList.find(uuid);if(it!=playerList.end())entry=it->second;entry.uuid=uuid;
                if(action==0){entry.name=r.string();int properties=r.var();if(properties<0||properties>64)throw std::runtime_error("Propriedades invalidas");for(int j=0;j<properties;++j){r.string();r.string();if(r.byte())r.string();}entry.gameMode=r.var();entry.ping=r.var();entry.displayName=r.byte()?Text(r.string()):"";}
                else if(action==1)entry.gameMode=r.var();else if(action==2)entry.ping=r.var();else if(action==3)entry.displayName=r.byte()?Text(r.string()):"";
                if(entry.name.size()>256)entry.name.resize(256);
                if(entry.gameMode<0||entry.gameMode>3)entry.gameMode=0;
                if(entry.ping<0)entry.ping=-1;
                if(it!=playerList.end()||action==0){if(playerList.size()>=2048&&it==playerList.end())throw std::runtime_error("Lista de jogadores muito grande");playerList[uuid]=std::move(entry);}}
            break;}
        case 0x3e:{std::string name=r.string();int mode=r.byte();if(mode==1){teams.erase(name);break;}if(mode<0||mode>4)throw std::runtime_error("Equipe invalida");Team team;if(auto it=teams.find(name);it!=teams.end())team=it->second;
            if(mode==0||mode==2){r.string();team.prefix=StripMinecraftFormatting(r.string());team.suffix=StripMinecraftFormatting(r.string());r.byte();r.string();r.byte();}
            if(mode==0)team.names.clear();
            if(mode==0||mode==3||mode==4){int count=r.var();if(count<0||count>2048)throw std::runtime_error("Equipe muito grande");for(int i=0;i<count;++i){auto member=r.string();if(mode==4)team.names.erase(std::remove(team.names.begin(),team.names.end(),member),team.names.end());else if(team.names.size()<2048&&std::find(team.names.begin(),team.names.end(),member)==team.names.end())team.names.push_back(member);}}
            if(teams.size()<1024||teams.count(name))teams[name]=std::move(team);
            break;}
        case 0x47:tabHeader=Text(r.string());tabFooter=Text(r.string());break;
        case 0x39:{int flags=r.byte();allowFlight=flags&4;lastFlying=flags&2;r.real();r.real();auto pose=player.CaptureState();pose.flying=flags&2;player.RestoreState(pose);break;}
        case 0x3f:{std::string channel=r.string();if(channel=="FML|HS")ForgeHandshake(r);break;}
        case 0x48:{r.string();r.string();for(int result:{3,1}){auto w=Packet(0x19);w.var(result);Send(std::move(w));}break;}
        default:break;
    }
}
}
bool Active(){return running;}
bool Joined(){return joined&&running&&playing&&!chunks.empty();}
std::string Status(){std::lock_guard<std::mutex> lock(mutex);return status;}
void Disconnect(){running=false;netio::Socket fd=socketFd;if(fd!=netio::Invalid)netio::Shutdown(fd);if(worker.joinable())worker.join();playing=false;joined=false;windowOpen=false;}
void Connect(const std::string& address,const std::string& name){
    Disconnect();std::string host=address;int port=25565;bool useSrv=true;
    try{
        auto first=host.find_first_not_of(" \t\r\n"),last=host.find_last_not_of(" \t\r\n");host=first==std::string::npos?"":host.substr(first,last-first+1);
        std::string number;
        if(!host.empty()&&host.front()=='['){auto end=host.find(']');if(end==std::string::npos)throw std::runtime_error("IPv6 invalido");if(end+1<host.size()){if(host[end+1]!=':')throw std::runtime_error("Endereco invalido");number=host.substr(end+2);if(number.empty())throw std::runtime_error("Porta invalida");}host=host.substr(1,end-1);}
        else {auto colon=host.rfind(':');if(colon!=std::string::npos&&host.find(':')==colon){number=host.substr(colon+1);host.resize(colon);if(number.empty())throw std::runtime_error("Porta invalida");}}
        if(!number.empty()){useSrv=false;if(number.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Porta invalida");port=std::stoi(number);}
        if(host.empty()||host.size()>253||port<1||port>65535||name.size()<3||name.size()>16||name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos)throw std::runtime_error("Endereco ou nome invalido");}
    catch(...){SetStatus("Use servidor:porta e nome de 3 a 16 letras/numeros");return;}
    {std::lock_guard<std::mutex> lock(mutex);incoming.clear();outgoing.clear();queued=0;}
    diagnostics.close();diagnostics.open("multiplayer.log",std::ios::trunc);correctionCount=chunkCountReceived=movementCount=0;serverCenter={};
    if(diagnostics)diagnostics<<"MineClone protocol 47: chunk and position diagnostics"<<std::endl;
    forgePhase=ForgePhase::Hello;ClearRemote();selected=0;ownId=0;windowId=0;dimension=0;transaction=0;sprinting=sneaking=allowFlight=lastFlying=false;ResetVitals();Health=20;messages.clear();playerList.clear();teams.clear();tabHeader.clear();tabFooter.clear();{std::lock_guard<std::mutex> lock(mutex);sessionNick=name;}RegionOriginX=RegionOriginZ=0;originY=0;running=true;worker=std::thread(Worker,host,port,name,useSrv);
}
Block LocalItem(const Slot& slot){if(slot.id<0)return Air;if(slot.id==358)return FilledMap;int meta=slot.damage;for(auto e:Catalog18)if(e.legacyId==slot.id&&e.metadata==meta&&e.available)return e.block;for(auto e:Catalog18)if(e.legacyId==slot.id&&e.metadata==0&&e.available)return e.block;return Stick;}
void Update(Player& player,float dt){
    std::deque<Bytes> batch;{std::lock_guard<std::mutex> lock(mutex);batch.swap(incoming);queued=0;}
    try{for(auto& b:batch)Handle(b,player);}catch(const std::exception& e){if(diagnostics)diagnostics<<"protocol error: "<<e.what()<<std::endl;SetStatus(std::string("Protocolo: ")+e.what());Disconnect();return;}
    if(!Joined())return;
    worldReady=true;
    auto pose=player.CaptureState();Vector3 global=Global(pose.position);int wantedY=std::clamp(int(std::floor(global.y/16))*16-32,0,256-Height);
    if(pose.position.x<48||pose.position.x>Width-48||pose.position.z<48||pose.position.z>Width-48||((pose.position.y<8||pose.position.y>Height-16)&&wantedY!=originY))Recenter(player,global);
    auto flight=player.CaptureState();
    if(flight.flying&&!allowFlight){flight.flying=false;player.RestoreState(flight);}
    if(flight.flying!=lastFlying){lastFlying=flight.flying;auto w=Packet(0x13);w.byte((lastFlying?2:0)|(allowFlight?4:0));w.real(.05f);w.real(.1f);Send(std::move(w));}
    (void)dt;
}
std::vector<Hologram> Holograms(){std::vector<Hologram> list;for(const auto& pair:entities){const auto& e=pair.second;std::string name=e.name;if(e.type==0&&!e.uuid.empty()){auto it=playerList.find(e.uuid);if(it!=playerList.end()){name=it->second.displayName.empty()?it->second.name:it->second.displayName;}}if(!name.empty()&&((e.type==0&&!e.invisible)||(e.type!=0&&e.nameVisible))){Vector3 p=Local(e.position);p.y+=(e.type==30?(e.small?.9875f:1.975f):1.8f)+.25f;list.push_back({p,name});}}return list;}
void DrawHolograms(const Camera3D& camera){
    static constexpr unsigned rgb[]={0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xffaa00,0xaaaaaa,0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff};
    Font font=GetFontDefault();Vector3 right=Vector3Normalize(Vector3CrossProduct(Vector3Subtract(camera.target,camera.position),camera.up));
    for(const auto& label:Holograms()){
        if(Vector3Distance(camera.position,label.position)>64)continue;
        std::string plain=StripMinecraftFormatting(label.text);float width=MeasureTextEx(font,plain.c_str(),10,1).x*.025f,offset=-width*.5f;Color color=WHITE;
        for(size_t i=0;i<label.text.size();){int bytes=0,code=GetCodepointNext(label.text.c_str()+i,&bytes);i+=std::max(1,bytes);
            if(code==167&&i<label.text.size()){char c=label.text[i++];const char* digits="0123456789abcdef";const char* found=std::strchr(digits,c);if(found){unsigned v=rgb[found-digits];color={static_cast<unsigned char>(v>>16),static_cast<unsigned char>(v>>8),static_cast<unsigned char>(v),255};}else if(c=='r')color=WHITE;continue;}
            int glyph=GetGlyphIndex(font,code);Rectangle source=font.recs[glyph];float advance=float(font.glyphs[glyph].advanceX?font.glyphs[glyph].advanceX:source.width)+1;
            Vector3 p=Vector3Add(label.position,Vector3Scale(right,offset));
            if(code==0x2588||code==0x2593||code==0x2592){Texture2D white{rlGetTextureIdDefault(),1,1,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};DrawBillboardPro(camera,white,{0,0,1,1},p,camera.up,{.2f,.25f},{0,0},0,color);advance=8;}
            else if(code!=' ')DrawBillboardPro(camera,font.texture,source,p,camera.up,{source.width*.025f,source.height*.025f},{0,0},0,color);
            offset+=advance*.025f;
        }
    }
}
float MovementSpeed(){return movementAttribute;}
float CollisionAt(int x,int y,int z){
    int gx=x+RegionOriginX,gz=z+RegionOriginZ,gy=y+originY;
    if(gy<0||gy>=256)return 0;
    auto it=chunks.find({int(std::floor(gx/16.f)),int(std::floor(gz/16.f))});if(it==chunks.end())return 0;
    int state=it->second[(gy*16+(gz&15))*16+(gx&15)],id=state>>4,meta=state&15;
    switch(id){
    case 0:case 6:case 8:case 9:case 10:case 11:case 27:case 28:case 31:case 32:case 37:case 38:case 39:case 40:case 50:case 51:case 55:case 59:case 63:case 65:case 66:case 68:case 69:case 75:case 76:case 77:case 83:case 90:case 104:case 105:case 106:case 115:case 119:case 127:case 131:case 132:case 141:case 142:case 143:case 157:case 175:return 0;
    case 171:return 1.f/16;case 78:return (meta+1)/8.f;case 44:case 126:case 182:return meta&8?1.f:.5f;
    case 60:return .9375f;case 88:return .875f;case 26:return .5625f;case 54:case 130:case 146:return .875f;
    default:return Solid(WorldBlock(uint16_t(state)))?1.f:0.f;
    }
}
void SendMovement(const Player& player){if(!Joined())return;Position(player);
    if(diagnostics&&(++movementCount<=10||movementCount%20==0)){auto p=Global(player.CaptureState().position);diagnostics<<"movement "<<p.x<<' '<<p.y<<' '<<p.z<<" ground="<<player.OnGround()<<std::endl;}
}
bool PlayerInventoryOpen(){return windowId==0;}
void DrawPlayers(){for(auto& entry:entities){if(entry.second.type!=0||entry.second.invisible)continue;Vector3 p=Local(entry.second.position);if(!Inside(int(p.x),int(p.y),int(p.z)))continue;DrawCube(Vector3Add(p,{0,1.5f,0}),.5f,.5f,.5f,{186,138,99,255});DrawCube(Vector3Add(p,{0,.95f,0}),.6f,.7f,.3f,SKYBLUE);DrawCube(Vector3Add(p,{-.16f,.35f,0}),.24f,.7f,.26f,BLUE);DrawCube(Vector3Add(p,{.16f,.35f,0}),.24f,.7f,.26f,BLUE);}}
void Attack(const Player& player){auto c=player.Camera();Vector3 d=Vector3Normalize(Vector3Subtract(c.target,c.position));int target=-1;float nearest=3.f;Hit block=Trace(c.position,d);if(block.found){auto solid=GetRayCollisionBox({c.position,d},{{float(block.x),float(block.y),float(block.z)},{block.x+1.f,block.y+CollisionHeight(At(block.x,block.y,block.z)),block.z+1.f}});if(solid.hit)nearest=std::min(nearest,solid.distance);}for(auto e:entities){Vector3 p=Local(e.second.position);auto hit=GetRayCollisionBox({c.position,d},{{p.x-.3f,p.y,p.z-.3f},{p.x+.3f,p.y+1.8f,p.z+.3f}});if(hit.hit&&hit.distance<nearest){target=e.first;nearest=hit.distance;}}auto swing=Packet(0x0a);Send(std::move(swing));if(target>=0){auto w=Packet(2);w.var(target);w.var(1);Send(std::move(w));}}
void Drop(bool all){if(!Joined())return;auto w=Packet(7);w.byte(all?3:4);w.longword(0);w.byte(0);Send(std::move(w));}
void Dig(const Hit& hit,int status){if(!hit.found)return;auto w=Packet(7);w.byte(status);int x=hit.x+RegionOriginX,y=hit.y+originY,z=hit.z+RegionOriginZ;w.longword((uint64_t(x)&0x3ffffff)<<38|(uint64_t(y)&4095)<<26|(uint64_t(z)&0x3ffffff));w.byte(hit.ny>0?1:hit.ny<0?0:hit.nz<0?2:hit.nz>0?3:hit.nx<0?4:5);Send(std::move(w));}
void Interact(const Player& player){auto c=player.Camera();Hit hit=Trace(c.position,Vector3Normalize(Vector3Subtract(c.target,c.position)),true);auto w=Packet(8);int face=255;if(hit.found){int x=hit.x+RegionOriginX,y=hit.y+originY,z=hit.z+RegionOriginZ;w.longword((uint64_t(x)&0x3ffffff)<<38|(uint64_t(y)&4095)<<26|(uint64_t(z)&0x3ffffff));face=hit.ny>0?1:hit.ny<0?0:hit.nz<0?2:hit.nz>0?3:hit.nx<0?4:5;}else w.longword(~uint64_t(0));w.byte(face);const auto& item=playerSlots[36+selected];if(item.wire.empty())w.word(-1);else w.b.insert(w.b.end(),item.wire.begin(),item.wire.end());w.byte(8);w.byte(8);w.byte(8);Send(std::move(w));}
void MovementFlags(bool sprint,bool sneak){
    if(!Joined())return;
    for(int kind=0;kind<2;++kind){bool wanted=kind?sneak:sprint;bool& current=kind?sneaking:sprinting;if(wanted==current)continue;current=wanted;auto w=Packet(0x0b);w.var(ownId);w.var(kind?(wanted?0:1):(wanted?3:4));w.var(0);Send(std::move(w));}
}
int SelectedSlot(){return selected;}
void SelectSlot(int index){if(index<0||index>8)return;selected=index;auto w=Packet(9);w.word(index);Send(std::move(w));}
void Chat(const std::string& text){if(!Joined()||text.empty())return;auto w=Packet(1);w.string(text.substr(0,100));Send(std::move(w));}
std::string Nickname(){std::lock_guard<std::mutex> lock(mutex);return sessionNick;}
std::vector<PlayerEntry> PlayerList(){std::vector<PlayerEntry> list;for(const auto& item:playerList){auto player=item.second;if(player.displayName.empty()){player.displayName=player.name;for(const auto& team:teams)if(std::find(team.second.names.begin(),team.second.names.end(),player.name)!=team.second.names.end()){player.displayName=team.second.prefix+player.name+team.second.suffix;break;}}list.push_back(std::move(player));}std::sort(list.begin(),list.end(),[](const PlayerEntry& a,const PlayerEntry& b){if((a.gameMode==3)!=(b.gameMode==3))return a.gameMode!=3;return a.name<b.name;});return list;}
const std::string& TabHeader(){return tabHeader;}
const std::string& TabFooter(){return tabFooter;}
const MapData* FindMap(int id){auto it=maps.find(id);return it==maps.end()?nullptr:&it->second;}
int HeldMapId(){const Slot& item=playerSlots[36+selected];return item.id==358&&item.count?uint16_t(item.damage):-1;}
std::vector<MapFrame> MapFrames(){std::vector<MapFrame> frames;for(auto item:mapFrames){auto frame=item.second;frame.position=Local(frame.position);if(frame.mapId>=0)frames.push_back(frame);}return frames;}
Color MapColor(unsigned char index){
    static constexpr unsigned base[]={0,0x7fb238,0xf7e9a3,0xc7c7c7,0xff0000,0xa0a0ff,0xa7a7a7,0x007c00,0xffffff,0xa4a8b8,0x976d4d,0x707070,0x4040ff,0x8f7748,0xfffcf5,0xd87f33,0xb24cd8,0x6699d8,0xe5e533,0x7fcc19,0xf27fa5,0x4c4c4c,0x999999,0x4c7f99,0x7f3fb2,0x334cb2,0x664c33,0x667f33,0x993333,0x191919,0xfaee4d,0x5cdbd5,0x4a80ff,0x00d93a,0x815631,0x700200};
    static constexpr int shade[]={180,220,255,135};unsigned color=index/4;if(color==0||color>=sizeof(base)/sizeof(*base))return BLANK;
    unsigned rgb=base[color];int light=shade[index&3];return {static_cast<unsigned char>((rgb>>16)*light/255),static_cast<unsigned char>(((rgb>>8)&255)*light/255),static_cast<unsigned char>((rgb&255)*light/255),255};
}
const std::vector<std::string>& Messages(){return messages;}
void ShowInventory(){windowId=0;windowSlots=playerSlots;title="Inventario do servidor";windowOpen=true;}
bool WindowOpen(){return windowOpen;}
const std::vector<Slot>& WindowSlots(){return windowSlots;}
const Slot& Cursor(){return cursor;}
const std::string& WindowTitle(){return title;}
void SwapHotbar(int slot,int hotbar){if(slot<0||slot>=int(windowSlots.size())||hotbar<0||hotbar>8)return;auto w=Packet(0x0e);w.byte(windowId);w.word(slot);w.byte(hotbar);w.word(++transaction);w.byte(2);w.word(-1);Send(std::move(w));}
void ClickSlot(int slot,bool right,bool shift){if((slot<0&&slot!=-999)||slot>=int(windowSlots.size()))return;auto w=Packet(0x0e);w.byte(windowId);w.word(slot);w.byte(right?1:0);w.word(++transaction);w.byte(shift?1:0);if(slot==-999)w.word(-1);else{const auto& raw=windowSlots[slot].wire;if(raw.empty())w.word(-1);else w.b.insert(w.b.end(),raw.begin(),raw.end());}Send(std::move(w));}
void CloseWindow(){auto w=Packet(0x0d);w.byte(windowId);Send(std::move(w));windowOpen=false;windowId=0;}
void Respawn(){auto w=Packet(0x16);w.var(0);Send(std::move(w));}
}
