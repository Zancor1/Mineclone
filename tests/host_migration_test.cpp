#include "bedwars.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <csignal>
namespace {
pid_t children[2]{};
void Check(bool v,const char* text){if(!v){std::fprintf(stderr,"FAIL: %s\n",text);for(auto pid:children)if(pid>0){kill(pid,SIGTERM);waitpid(pid,nullptr,0);}std::exit(1);}}
struct Report {int kind=0,hosting=0,team=0,players=0,xp=0,block=0,bed=0,lobby=0,purchase=0,rootBedRemoved=0;float tnt=0;};
}
int main(){using namespace game;
    struct Sandbox {std::filesystem::path p;~Sandbox(){std::error_code ec;std::filesystem::remove_all(p,ec);}} sandbox{std::filesystem::temp_directory_path()/("mineclone-migration-"+std::to_string(getpid()))};progression::Directory=sandbox.p/"root"/"profiles";
    int probe=socket(AF_INET,SOCK_STREAM,0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);Check(bind(probe,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,"ephemeral migration port");socklen_t length=sizeof(address);getsockname(probe,reinterpret_cast<sockaddr*>(&address),&length);int port=ntohs(address.sin_port);close(probe);
    int commands[2][2],reports[2][2];for(int i=0;i<2;++i)Check(pipe(commands[i])==0&&pipe(reports[i])==0,"migration test pipes");
    for(int index=0;index<2;++index){pid_t pid=fork();Check(pid>=0,"spawn real clients");if(pid==0){for(int j=0;j<2;++j){close(commands[j][1]);close(reports[j][0]);if(j!=index){close(commands[j][0]);close(reports[j][1]);}}
        char command=0;if(read(commands[index][0],&command,1)!=1)_exit(1);fcntl(commands[index][0],F_SETFL,O_NONBLOCK);
        progression::Directory=sandbox.p/(index==0?"next":"other")/"profiles";progression::Profile profile;profile.Award(75+index);std::string error;progression::Save(index==0?"Next":"Other",profile,error);
        if(!bw::Join("127.0.0.1:"+std::to_string(port),index==0?"Next":"Other"))_exit(2);
        Player player;bool announced=false,promoted=false,secondReported=false;int hackAfter=-1;
        for(int frame=0;frame<12000&&bw::Active();++frame){if(read(commands[index][0],&command,1)==1){if(command=='Q')break;if(command=='K')hackAfter=frame+140;}
            if(hackAfter>=0&&frame>=hackAfter&&frame<hackAfter+70){auto pose=player.CaptureState();pose.position={10,90,10};player.RestoreState(pose);}
            bw::Update(player,.02f,true,0);
            if(bw::Ready()&&!announced){Report report;report.kind=0;report.team=bw::LocalTeam();write(reports[index][1],&report,sizeof(report));announced=true;}
            bool oldPresent=false;for(auto entry:bw::PlayerList())if(entry.name=="Root")oldPresent=true;
            if(bw::Ready()&&!bw::Migrating()&&!oldPresent&&!promoted){Report report;report.kind=1;report.hosting=bw::Hosting();report.team=bw::LocalTeam();report.players=int(bw::PlayerList().size());report.xp=bw::Progress().xp;report.lobby=bw::Lobby();auto base=bw::BasePosition(0);report.block=At(int(base.x)+1,26,int(base.z))==RedWool;report.rootBedRemoved=At(int(base.x),26,int(base.z)+3)==Air;auto own=bw::BasePosition(report.team);report.bed=At(int(own.x),26,int(own.z)+3)==Bed;auto tnt=bw::TntStates();report.tnt=tnt.empty()?0:tnt[0].remaining;if(index==0){bw::Buy(0);bw::Update(player,0.f,true,0);for(auto stack:Hotbar)if(stack.block==BlueWool&&stack.count)report.purchase=1;}write(reports[index][1],&report,sizeof(report));promoted=true;}
            if(index==1&&bw::Hosting()&&!bw::Migrating()&&!secondReported){Report report;report.kind=2;report.hosting=1;report.players=int(bw::PlayerList().size());write(reports[index][1],&report,sizeof(report));secondReported=true;}
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        bw::Stop();_exit(announced&&promoted?0:3);
    }children[index]=pid;}
    for(int i=0;i<2;++i){close(commands[i][0]);close(reports[i][1]);fcntl(reports[i][0],F_SETFL,O_NONBLOCK);}
    Check(bw::Host("Root",port),"host original room");Player root;
    auto tick=[&](int frames){for(int i=0;i<frames;++i){bw::Update(root,.05f,true,0);std::this_thread::sleep_for(std::chrono::milliseconds(2));}};
    auto getReport=[&](int index,int kind){Report report;for(int i=0;i<4000;++i){if(read(reports[index][0],&report,sizeof(report))==sizeof(report)&&report.kind==kind)return report;tick(1);}Check(false,"client report timeout");return report;};
    for(int i=0;i<2;++i){char command='J';write(commands[i][1],&command,1);getReport(i,0);}
    Check(bw::PlayerList().size()==3,"three real participants connected in order");bw::StartRound();tick(850);Check(!bw::Lobby(),"match running before handoff");
    bw::Buy(0);tick(2);int wool=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==RedWool)wool=i;Check(wool>=0,"buy authoritative inventory before migration");auto base=bw::BasePosition(0);bw::Place(int(base.x)+1,26,int(base.z),wool,&root);tick(2);Check(At(int(base.x)+1,26,int(base.z))==RedWool,"placed block before migration");
    bw::Buy(12);tick(2);int tnt=-1;for(int i=0;i<9;++i)if(Hotbar[i].block==Tnt)tnt=i;Check(tnt>=0,"buy TNT before migration");bw::Place(int(base.x)+2,26,int(base.z),tnt,&root);tick(1);Check(!bw::TntStates().empty(),"active fuse before migration");
    auto impossible=root.CaptureState();impossible.position={10,90,10};for(int i=0;i<14;++i){root.RestoreState(impossible);tick(1);}
    for(int i=0;i<4000&&bw::Active();++i)tick(1);
    Check(!bw::Active()&&bw::Status()=="LoL try to cheating.","host is kicked after automatic confirmations");
    Report next=getReport(0,1),other=getReport(1,1);Check(next.hosting&&!other.hosting,"second entrant assumes host and third reconnects");
    Check(next.players==2&&other.players==2&&!next.lobby&&!other.lobby,"same match continues with surviving players");Check(next.team==1&&other.team==2&&next.xp==75&&other.xp==76,"team IDs and profiles survive migration");Check(next.block&&other.block&&next.bed&&other.bed,"world edits and beds survive migration");Check(next.rootBedRemoved&&other.rootBedRemoved,"kicked host leaves no orphan team bed");Check(next.tnt>0&&other.tnt>0,"TNT fuse survives migration");Check(next.purchase,"new host routes its own inventory actions through its retained player ID");
    char hack='K';write(commands[0][1],&hack,1);auto finalHost=getReport(1,2);Check(finalHost.hosting&&finalHost.players==1,"a second host kick transfers authority again to the third entrant");
    {char quit='Q';write(commands[1][1],&quit,1);}
    for(auto& pid:children){int status=0;Check(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"clients exit cleanly");pid=0;}
    std::puts("PASS: host anti-cheat kick elects second entrant, reconnects third client and preserves the running match");
}
