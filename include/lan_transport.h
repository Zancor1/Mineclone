#pragma once
#include "platform_socket.h"
#include <cstdint>
#include <string>
#include <vector>
namespace game {
class LanTransport {
public:
    struct Event {enum Kind {Connected,Disconnected,Data} kind;int peer;std::vector<unsigned char> data;};
    LanTransport()=default;
    ~LanTransport();
    LanTransport(const LanTransport&)=delete;
    LanTransport& operator=(const LanTransport&)=delete;
    bool Host(unsigned short port);
    bool Join(const std::string& address);
    void Stop();
    std::vector<Event> Poll();
    void Send(int peer,const std::vector<unsigned char>& data);
    void Broadcast(const std::vector<unsigned char>& data);
    void Drop(int peer);
    bool Hosting() const {return listener!=netio::Invalid;}
    bool Alive() const {return listener!=netio::Invalid||!peers.empty();}
    const std::string& Error() const {return error;}
private:
    struct Peer {int id;netio::Socket fd;bool connecting=false;double started=0;std::vector<unsigned char> input,output;size_t sent=0;bool closing=false;};
    std::vector<Peer> peers;
    netio::Socket listener=netio::Invalid;
    int nextId=1;
    std::string error;
};
}

namespace game {
struct LanRoom {
    std::string address,name;
    unsigned short port=25566;
    int players=1,gap=64;
    bool joinable=true;
    uint64_t identity=0;
    int capacity=4;
    std::string map;
    std::vector<std::string> playerNames;
    int ping=-1;
};
class LanDiscovery {
public:
    ~LanDiscovery();
    bool Advertise(const LanRoom& room);
    void StopHosting();
    bool Search();
    void CancelSearch();
    void Poll();
    bool Searching() const;
    const std::vector<LanRoom>& Rooms() const {return rooms;}
    const std::string& Status() const {return status;}
private:
    struct Interface {uint32_t address,broadcast;};
    std::vector<Interface> interfaces;
    std::vector<LanRoom> rooms;
    netio::Socket server=netio::Invalid,scanner=netio::Invalid;
    LanRoom advertised;
    uint32_t nonce=0;
    uint64_t instance=0;
    double deadline=0,nextProbe=0,probeSentAt=0;
    std::string status;
    void Probe();
    void FindInterfaces();
};
}
