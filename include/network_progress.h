#pragma once
#include <chrono>
namespace game::net {
class ConnectionProgress {
public:
    using Clock=std::chrono::steady_clock;
    using Time=Clock::time_point;
    explicit ConnectionProgress(Time now=Clock::now()):started(now){}
    void Accepted(Time now=Clock::now()){login=false;started=now;}
    void Ready(){complete=true;}
    const char* Timeout(Time now=Clock::now()) const {
        if(complete)return nullptr;
        if(now-started<std::chrono::seconds(login?35:45))return nullptr;
        return login?"Login sem resposta (35s). Confira endereco, porta e suporte a Java 1.8.":"Login aceito, mas o mundo nao chegou em 45s (posicao/chunks).";
    }
private:
    Time started;
    bool login=true,complete=false;
};
}
