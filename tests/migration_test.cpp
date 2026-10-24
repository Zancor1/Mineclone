#include "world.h"
#include "chest.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
namespace {
void Check(bool ok,const char* text) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
void Word(std::vector<unsigned char>& data,size_t offset,uint32_t value) {for(int i=0;i<4;++i)data[offset+i]=static_cast<unsigned char>(value>>(8*i));}
uint32_t Sum(const std::vector<unsigned char>& data,size_t offset) {uint32_t sum=2166136261u;for(size_t i=offset;i<data.size();++i)sum=(sum^data[i])*16777619u;return sum;}
void Write(const std::filesystem::path& file,const std::vector<unsigned char>& data) {std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),data.size());Check(bool(out),"write fixture");}
}
int main() {
    using namespace game;
    auto folder=std::filesystem::temp_directory_path()/("mineclone-migration-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    auto path=folder/"world.dat";
    Change(8,70,8,Chest);(*ChestAt(8,70,8))[0]={DiamondPickaxe,1,10};Change(9,70,8,Water);
    PlayerState pose;pose.valid=true;pose.position={64.5f,75,64.5f};
    Check(SaveWorld(path.string(),pose),"write expanded save");
    std::ifstream in(path,std::ios::binary);std::vector<unsigned char> current((std::istreambuf_iterator<char>(in)),{});in.close();
    constexpr size_t oldWidth=128,cells=size_t(Width)*Height*Width;
    std::vector<unsigned char> old(current.begin(),current.begin()+24);
    Word(old,8,oldWidth);
    for(size_t component=0;component<3;++component)
        for(size_t x=0;x<oldWidth;++x)for(size_t y=0;y<Height;++y)for(size_t z=0;z<oldWidth;++z)
            old.push_back(current[24+component*cells+(x*Height+y)*Width+z]);
    old.insert(old.end(),current.begin()+24+cells*3,current.end());Word(old,20,Sum(old,24));Write(path,old);
    auto chunks=std::filesystem::path(path.string()+".chunks");std::filesystem::create_directories(chunks);
    std::vector<unsigned char> archive(20+ChunkSize*Height*ChunkSize*3,0);
    archive[0]='M';archive[1]='C';archive[2]='C';archive[3]='1';Word(archive,4,Seed);Word(archive,8,8);Word(archive,12,0);
    archive[20+(70*ChunkSize+8)*3]=PinkWool;Word(archive,16,Sum(archive,20));Write(chunks/"8_0.chunk",archive);
    Change(8,70,8,Air);
    Check(LoadWorld(path.string(),&pose),"128-block save loads into expanded world");
    Check(At(8,70,8)==Chest&&(*ChestAt(8,70,8))[0].damage==10&&IsWaterSource(9,70,8),"old blocks, tool wear, chests and water are preserved");
    Check(At(128,70,8)==PinkWool,"expansion restores archived construction instead of regenerating it");
    Check(pose.position.x==64.5f&&RegionOriginX==0,"migration preserves exact global pose");
    Change(20,70,20,BlueWool);archive.back()=99;Write(chunks/"8_0.chunk",archive);
    Check(!LoadWorld(path.string())&&At(20,70,20)==BlueWool,"corrupt expansion archive leaves active world untouched");
    std::filesystem::remove_all(folder);
    std::puts("PASS: 128-to-256 world migration, archived edits, inventories, pose and corruption safety");
}
