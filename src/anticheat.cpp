#include "anticheat.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <wintrust.h>
#include <softpub.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cwctype>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace game::ac { namespace {
std::atomic<bool> tripped{false},running{false};std::thread worker;std::mutex sleepMutex;std::condition_variable wake;
std::map<std::wstring,bool> trusted;
std::wstring Lower(std::wstring value){for(auto& c:value)c=wchar_t(std::towlower(c));return value;}
bool KnownTool(const std::wstring& name){
    auto n=Lower(name);return n==L"processhacker.exe"||n==L"processhacker2.exe"||n==L"systeminformer.exe"||n==L"extremeinjector.exe"||n==L"extreme injector.exe"||n==L"extreme injector v3.exe"||n==L"xenos.exe"||n==L"xenos64.exe"||n==L"xenos32.exe"||n==L"cheatengine-x86_64.exe"||n==L"cheatengine-i386.exe";
}
std::wstring ProcessPath(HANDLE process){std::wstring path(32768,L'\0');DWORD length=DWORD(path.size());if(!QueryFullProcessImageNameW(process,0,path.data(),&length))return {};path.resize(length);return path;}
bool Signed(const std::wstring& path){
    if(path.empty())return false;
    auto key=Lower(path);auto it=trusted.find(key);if(it!=trusted.end())return it->second;
    WINTRUST_FILE_INFO file{};file.cbStruct=sizeof(file);file.pcwszFilePath=path.c_str();
    WINTRUST_DATA data{};data.cbStruct=sizeof(data);data.dwUIChoice=WTD_UI_NONE;data.fdwRevocationChecks=WTD_REVOKE_NONE;data.dwUnionChoice=WTD_CHOICE_FILE;data.pFile=&file;data.dwStateAction=WTD_STATEACTION_VERIFY;data.dwProvFlags=WTD_CACHE_ONLY_URL_RETRIEVAL;
    GUID policy=WINTRUST_ACTION_GENERIC_VERIFY_V2;bool ok=WinVerifyTrust(nullptr,&policy,&data)==ERROR_SUCCESS;
    data.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&policy,&data);trusted[key]=ok;return ok;
}
bool SystemModule(const std::wstring& path){
    wchar_t windows[MAX_PATH]{};if(!GetWindowsDirectoryW(windows,MAX_PATH))return false;
    auto base=Lower(windows),p=Lower(path);return p.rfind(base+L"\\system32\\",0)==0||p.rfind(base+L"\\syswow64\\",0)==0||p.rfind(base+L"\\winsxs\\",0)==0;
}
bool ToolsOrModules(){
    HANDLE processes=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(processes!=INVALID_HANDLE_VALUE){PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);if(Process32FirstW(processes,&entry))do{if(KnownTool(entry.szExeFile)){CloseHandle(processes);return true;}}while(Process32NextW(processes,&entry));CloseHandle(processes);}
    HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,GetCurrentProcessId());
    if(modules!=INVALID_HANDLE_VALUE){MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);if(Module32FirstW(modules,&entry))do{
        if(entry.hModule==GetModuleHandleW(nullptr))continue;
        if(!SystemModule(entry.szExePath)&&!Signed(entry.szExePath)){CloseHandle(modules);return true;}
    }while(Module32NextW(modules,&entry));CloseHandle(modules);}return false;
}
// Inspect only PROCESS handles. No driver, debug privilege or remote memory reads.
struct HandleEntry {void* object;ULONG_PTR pid,handle;ULONG access;USHORT trace,type;ULONG attributes,reserved;};
struct HandleList {ULONG_PTR count,reserved;HandleEntry entries[1];};
template<class T>T Procedure(const char* name){auto raw=GetProcAddress(GetModuleHandleW(L"ntdll.dll"),name);T result{};static_assert(sizeof(result)==sizeof(raw));std::memcpy(&result,&raw,sizeof(result));return result;}
bool ExternalAccess(){
    using Query=LONG (NTAPI*)(ULONG,void*,ULONG,ULONG*);
    auto query=Procedure<Query>("NtQuerySystemInformation");if(!query)return false;
    HANDLE marker=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,GetCurrentProcessId());if(!marker)return false;
    std::vector<unsigned char> bytes(1<<20);ULONG needed=0;LONG status;
    while((status=query(64,bytes.data(),ULONG(bytes.size()),&needed))==LONG(0xC0000004L)&&bytes.size()<32*1024*1024)bytes.resize(std::min(size_t(32*1024*1024),std::max(bytes.size()*2,size_t(needed)+4096)));
    if(status<0){CloseHandle(marker);return false;}
    auto list=reinterpret_cast<const HandleList*>(bytes.data());auto count=std::min(size_t(list->count),(bytes.size()-offsetof(HandleList,entries))/sizeof(HandleEntry));USHORT processType=0;void* ownObject=nullptr;
    for(size_t i=0;i<count;++i)if(list->entries[i].pid==GetCurrentProcessId()&&list->entries[i].handle==reinterpret_cast<ULONG_PTR>(marker)){processType=list->entries[i].type;ownObject=list->entries[i].object;break;}
    CloseHandle(marker);if(!processType||!ownObject)return false;
    std::map<DWORD,HANDLE> sources;bool found=false;
    for(size_t i=0;i<count&&!found&&running;++i){auto h=list->entries[i];
        if(h.type!=processType||h.object!=ownObject||h.pid==GetCurrentProcessId()||h.pid<=4||!(h.access&(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_CREATE_THREAD)))continue;
        auto pid=DWORD(h.pid);auto it=sources.find(pid);
        if(it==sources.end())it=sources.emplace(pid,OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)).first;
        if(!it->second)continue;
        auto path=ProcessPath(it->second);
        // Signed system/driver/security utilities commonly inspect processes.
        if(!path.empty()&&!Signed(path))found=true;
    }
    for(auto p:sources)if(p.second)CloseHandle(p.second);
    return found;
}
bool PrivateThread(){
    using Query=LONG (NTAPI*)(HANDLE,ULONG,void*,ULONG,ULONG*);
    auto query=Procedure<Query>("NtQueryInformationThread");if(!query)return false;
    HANDLE threads=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(threads==INVALID_HANDLE_VALUE)return false;
    bool found=false;THREADENTRY32 entry{};entry.dwSize=sizeof(entry);
    if(Thread32First(threads,&entry))do{if(entry.th32OwnerProcessID!=GetCurrentProcessId())continue;
        HANDLE thread=OpenThread(THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);if(!thread)continue;
        void* start=nullptr;if(query(thread,9,&start,sizeof(start),nullptr)>=0&&start){MEMORY_BASIC_INFORMATION memory{};
            if(VirtualQuery(start,&memory,sizeof(memory))&&memory.Type==MEM_PRIVATE&&(memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))found=true;
        }CloseHandle(thread);if(found)break;
    }while(Thread32Next(threads,&entry));CloseHandle(threads);return found;
}
struct CodeSection {const unsigned char* bytes;size_t size;uint64_t hash;};
std::vector<CodeSection> codeSections;
uint64_t CodeHash(const unsigned char* bytes,size_t size){uint64_t value=1469598103934665603ULL;for(size_t i=0;i<size;++i){value^=bytes[i];value*=1099511628211ULL;}return value;}
void CaptureCode(){
    codeSections.clear();auto base=reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return;
    auto image=reinterpret_cast<const IMAGE_NT_HEADERS*>(base+dos->e_lfanew);if(image->Signature!=IMAGE_NT_SIGNATURE)return;
    auto section=IMAGE_FIRST_SECTION(image);for(int i=0;i<image->FileHeader.NumberOfSections;++i){if(!(section[i].Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        size_t size=section[i].Misc.VirtualSize;if(!size||section[i].VirtualAddress>=image->OptionalHeader.SizeOfImage||size>image->OptionalHeader.SizeOfImage-section[i].VirtualAddress)continue;
        auto bytes=base+section[i].VirtualAddress;codeSections.push_back({bytes,size,CodeHash(bytes,size)});
    }
}
bool ChangedCode(){for(const auto& section:codeSections)if(CodeHash(section.bytes,section.size)!=section.hash)return true;return false;}
bool Scan(){return ChangedCode()||ToolsOrModules()||ExternalAccess()||PrivateThread();}
}
bool StartMonitor(){StopMonitor();trusted.clear();tripped=false;running=true;
    CaptureCode();if(Scan()){tripped=true;running=false;return false;}
    worker=std::thread([]{while(running){std::unique_lock<std::mutex> lock(sleepMutex);wake.wait_for(lock,std::chrono::seconds(2),[]{return !running;});lock.unlock();if(!running)break;if(Scan()){tripped=true;break;}}});return true;
}
void StopMonitor(){running=false;wake.notify_all();if(worker.joinable())worker.join();}
bool Detected(){return tripped;}
void ShowWarning(){MessageBoxA(nullptr,"LoL try to cheating.","MineClone",MB_OK|MB_ICONWARNING|MB_SETFOREGROUND);}
}
#else
namespace game::ac {bool StartMonitor(){return true;}void StopMonitor(){}bool Detected(){return false;}void ShowWarning(){}}
#endif
