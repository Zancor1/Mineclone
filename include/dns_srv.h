#pragma once
#include <string>
#include <vector>
namespace game::net {
struct SrvRecord {std::string host;int port=0,priority=0,weight=0;};
std::vector<SrvRecord> ParseSrvRecords(const unsigned char* data,int size);
SrvRecord LookupMinecraftSrv(const std::string& host);
}
