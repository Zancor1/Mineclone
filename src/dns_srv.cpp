#include "dns_srv.h"
#include "platform_socket.h"
#ifdef _WIN32
#include <windns.h>
#else
#include <arpa/nameser.h>
#include <resolv.h>
#endif
#include <algorithm>
#include <array>
#include <random>
namespace game::net {
namespace {
unsigned Word(const unsigned char* p){return (unsigned(p[0])<<8)|p[1];}
bool Name(const unsigned char* data,int size,int& offset,std::string& text){
    int cursor=offset;bool jumped=false;text.clear();
    for(int hops=0;hops<128;++hops){
        if(cursor<0||cursor>=size)return false;
        unsigned length=data[cursor++];
        if((length&192)==192){if(cursor>=size)return false;int next=int((length&63)<<8)|data[cursor++];if(!jumped)offset=cursor;jumped=true;cursor=next;continue;}
        if(length>63||cursor+int(length)>size)return false;
        if(!jumped)offset=cursor+int(length);
        if(!length)return true;
        if(!text.empty())text+='.';
        text.append(reinterpret_cast<const char*>(data+cursor),length);
        if(text.size()>253)return false;
        cursor+=int(length);
    }
    return false;
}
SrvRecord Choose(std::vector<SrvRecord> records){
    if(records.empty())return {};
    int priority=std::min_element(records.begin(),records.end(),[](auto& a,auto& b){return a.priority<b.priority;})->priority;
    records.erase(std::remove_if(records.begin(),records.end(),[&](auto& r){return r.priority!=priority;}),records.end());
    std::mt19937 random(std::random_device{}());int total=0;for(auto& r:records)total+=r.weight;
    if(total==0)return records[std::uniform_int_distribution<size_t>(0,records.size()-1)(random)];
    int choice=std::uniform_int_distribution<int>(1,total)(random);for(auto& r:records){choice-=r.weight;if(choice<=0)return r;}
    return records.back();
}
}
std::vector<SrvRecord> ParseSrvRecords(const unsigned char* data,int size){
    std::vector<SrvRecord> result;
    if(!data||size<12||(data[3]&15))return result;
    unsigned questions=Word(data+4),answers=Word(data+6);int offset=12;std::string name;
    for(unsigned i=0;i<questions;++i){if(!Name(data,size,offset,name)||offset+4>size)return {};offset+=4;}
    for(unsigned i=0;i<answers;++i){
        if(!Name(data,size,offset,name)||offset+10>size)return {};
        unsigned type=Word(data+offset),recordClass=Word(data+offset+2),length=Word(data+offset+8);offset+=10;
        if(length>unsigned(size-offset))return {};
        int end=offset+int(length);
        if(type==33&&recordClass==1&&length>=7){int target=offset+6;std::string host;
            if(Name(data,size,target,host)&&target<=end&&!host.empty()&&Word(data+offset+4))
                result.push_back({host,int(Word(data+offset+4)),int(Word(data+offset)),int(Word(data+offset+2))});
        }
        offset=end;
    }
    return result;
}
SrvRecord LookupMinecraftSrv(const std::string& host){
    if(!netio::Initialize())return {};
    std::array<unsigned char,16> address{};
    if(host=="localhost"||inet_pton(AF_INET,host.c_str(),address.data())==1||inet_pton(AF_INET6,host.c_str(),address.data())==1)return {};
    std::string query="_minecraft._tcp."+host;
#ifdef _WIN32
    PDNS_RECORDA records=nullptr;
    if(DnsQuery_A(query.c_str(),DNS_TYPE_SRV,DNS_QUERY_STANDARD,nullptr,&records,nullptr)!=ERROR_SUCCESS)return {};
    std::vector<SrvRecord> found;
    for(auto* r=records;r;r=r->pNext)if(r->wType==DNS_TYPE_SRV&&r->Data.SRV.pNameTarget&&r->Data.SRV.wPort){
        std::string target=r->Data.SRV.pNameTarget;if(!target.empty()&&target.back()=='.')target.pop_back();
        if(!target.empty())found.push_back({target,r->Data.SRV.wPort,r->Data.SRV.wPriority,r->Data.SRV.wWeight});
    }
    DnsRecordListFree(reinterpret_cast<PDNS_RECORD>(records),DnsFreeRecordList);
    return Choose(std::move(found));
#else
    struct __res_state resolver{};if(res_ninit(&resolver)<0)return {};
    resolver.retrans=2;resolver.retry=1;
    std::array<unsigned char,65536> response{};
    int size=res_nquery(&resolver,query.c_str(),ns_c_in,ns_t_srv,response.data(),response.size());res_nclose(&resolver);
    if(size<=0||size>int(response.size()))return {};
    return Choose(ParseSrvRecords(response.data(),size));
#endif
}
}
