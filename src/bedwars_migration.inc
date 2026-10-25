// Included inside the BedWars implementation namespace: checkpoint + two-phase handoff.
LanTransport nextHostWire,migrationProbe;
std::string migrationAddress,probeNonce;std::array<bool,MaxPlayers> reached{};bool probeAnswered=false,testingRoutes=false,migrationOwnCorrection=false;
Buffer migrationData;
std::string migrationToken,resumeTicket;
std::array<std::string,MaxPlayers> resumeTickets;
std::array<bool,MaxPlayers> resumed{},delivered{},migrationTried{};
size_t migrationOffset=0;int migrationSequence=0,migrationExpected=0;
bool commitSent=false;double resumeUntil=0;
constexpr int MigrationAck=44,MigrationCancel=45,MigrationResume=46,MigrationTest=47,MigrationReachable=48,ProbeQuestion=49,ProbeAnswer=50;
std::string RandomTicket(){std::random_device r;std::string out;for(int i=0;i<4;++i)out+=std::to_string(r())+"-";return out;}
void ResetMigration(){migrationProbe.Stop();migrationAddress.clear();probeNonce.clear();reached={};probeAnswered=testingRoutes=migrationOwnCorrection=false;nextHostWire.Stop();migrationMode=MigrationMode::None;migrationTarget=removedHost=-1;migrationDeadline=resumeUntil=0;migrationData={};migrationOffset=0;migrationSequence=migrationExpected=0;commitSent=false;changedTransport=false;migrationToken.clear();resumeTicket.clear();resumeTickets={};resumed={};delivered={};migrationTried={};}
template<bool Read,class T>void TransferValue(Buffer& b,T& value){
    if constexpr(std::is_same_v<T,float>){if constexpr(Read)value=b.Float();else b.Float(value);}
    else if constexpr(std::is_same_v<T,int>){if constexpr(Read)value=b.Int();else b.Int(value);}
    else if constexpr(std::is_same_v<T,bool>){if constexpr(Read){int n=b.Byte();if(n>1)throw std::runtime_error("Booleano de migracao invalido");value=n!=0;}else b.Byte(value);}
    else if constexpr(std::is_same_v<T,std::string>){if constexpr(Read)value=b.Text();else b.Text(value);}
    else if constexpr(std::is_same_v<T,Vector3>){if constexpr(Read)value=b.Vec();else b.Vec(value);}
    else if constexpr(std::is_same_v<T,ItemStack>){if constexpr(Read)value=Stack(b);else Stack(b,value);}
    else if constexpr(std::is_same_v<T,Block>){if constexpr(Read){int n=b.Byte();if(n>=BlockCount)throw std::runtime_error("Bloco de migracao invalido");value=Block(n);}else b.Byte(value);}
}
template<bool Read,class... T>void Transfer(Buffer& b,T&... values){(TransferValue<Read>(b,values),...);}
int TransferCount(Buffer& b,int limit){int n=b.Int();if(n<0||n>limit)throw std::runtime_error("Tamanho de migracao invalido");return n;}
template<bool Read>void TransferExtras(Buffer& b){
    Transfer<Read>(b,participants,serverTitle,roomIdentity,joinCounter,evidenceSerial);
    for(auto& ticket:resumeTickets)Transfer<Read>(b,ticket);
    for(auto& f:fighters){Transfer<Read>(b,f.joinOrder,f.lastAttacker,f.knockSpeed,f.knockUntil,f.lastPosition,f.lastAction,f.lastChat,f.lastHurt,f.absorbTime,f.digStart,f.healClock,f.regenClock,f.eatHeartbeat,f.fallDistance,f.hurtClock,f.hitFlash,f.meleeImmuneUntil,f.bowHeartbeat,f.lastBowShot,f.eatSlot,f.bowSlot,f.digSlot,f.digTool,f.blocking);
        Transfer<Read>(b,f.digging.found,f.digging.x,f.digging.y,f.digging.z,f.digging.nx,f.digging.ny,f.digging.nz);
    }
    for(auto& shot:shots)Transfer<Read>(b,shot.owner,shot.life,shot.tick,shot.support,shot.damage);
    if constexpr(Read){int n=TransferCount(b,192);droppedItems.resize(n);}else b.Int(int(droppedItems.size()));
    for(auto& item:droppedItems)Transfer<Read>(b,item.stack,item.position,item.velocity,item.age,item.life);
    for(auto& penguin:penguins)Transfer<Read>(b,penguin.active,penguin.p,penguin.display,penguin.direction,penguin.age,penguin.velocityY);
    if constexpr(Read){changedCells.clear();int n=TransferCount(b,Width*Height*Width);for(int i=0;i<n;++i){int cell=b.Int();Block block=Air;Transfer<true>(b,block);if(cell<0||cell>=Width*Height*Width)throw std::runtime_error("Celula de migracao invalida");changedCells[cell]=block;int x=cell/(Height*Width),y=(cell/Width)%Height,z=cell%Width;Change(x,y,z,block);}}
    else{b.Int(int(changedCells.size()));for(auto entry:changedCells){b.Int(entry.first);Transfer<false>(b,entry.second);}}
    if constexpr(Read){placed.clear();int n=TransferCount(b,Width*Height*Width);for(int i=0;i<n;++i){int cell=b.Int();if(cell<0||cell>=Width*Height*Width)throw std::runtime_error("Bloco colocado invalido");placed.insert(cell);}}
    else{b.Int(int(placed.size()));for(int cell:placed)b.Int(cell);}
    if constexpr(Read){int n=TransferCount(b,128);chatMessages.resize(n);}else b.Int(int(chatMessages.size()));
    for(auto& message:chatMessages)Transfer<Read>(b,message);
}
Buffer MigrationCheckpoint(){Buffer b;auto snapshot=StatePacket();b.Int(int(snapshot.bytes.size()));b.bytes.insert(b.bytes.end(),snapshot.bytes.begin(),snapshot.bytes.end());TransferExtras<false>(b);return b;}
void LoadMigrationCheckpoint(Buffer& b){int n=TransferCount(b,256*1024);if(size_t(n)>b.bytes.size()-b.offset)throw std::runtime_error("Snapshot de migracao incompleto");Buffer snapshot;snapshot.bytes.assign(b.bytes.begin()+b.offset,b.bytes.begin()+b.offset+n);b.offset+=n;Receive(0,snapshot);TransferExtras<true>(b);
    for(const auto& f:fighters)if(f.joinOrder<0||f.lastAttacker< -1||f.lastAttacker>=MaxPlayers||f.eatSlot< -1||f.eatSlot>8||f.bowSlot< -1||f.bowSlot>8||f.digSlot< -1||f.digSlot>8)throw std::runtime_error("Estado de jogador invalido na migracao");
    for(const auto& shot:shots)if(shot.owner< -1||shot.owner>=MaxPlayers)throw std::runtime_error("Dono de projetil invalido na migracao");
    if(b.offset!=b.bytes.size())throw std::runtime_error("Dados extras na migracao");}
void SendMigrationChunk(){
    size_t count=std::min(size_t(48000),migrationData.bytes.size()-migrationOffset);
    auto chunk=Packet(MigrationChunk);chunk.Text(migrationToken);chunk.Int(migrationSequence);chunk.Int(int(count));chunk.bytes.insert(chunk.bytes.end(),migrationData.bytes.begin()+migrationOffset,migrationData.bytes.begin()+migrationOffset+count);wire.Send(fighters[migrationTarget].peer,chunk.bytes);migrationOffset+=count;migrationDeadline=ConnectionNow()+10;
}
bool StartMigrationCandidate(){
    int successor=-1;
    for(int i=0;i<MaxPlayers;++i)if(i!=me&&fighters[i].connected&&!migrationTried[i]&&(successor<0||fighters[i].joinOrder<fighters[successor].joinOrder))successor=i;
    if(successor<0){status="LoL try to cheating.";Stop();return false;}
    migrationTarget=successor;migrationTried[successor]=true;removedHost=me;migrationToken=RandomTicket();
    for(auto& ticket:resumeTickets)ticket=RandomTicket();
    migrationData=MigrationCheckpoint();if(migrationData.bytes.size()>32*1024*1024){status="Transferencia excedeu limite de estado.";Stop();return false;}
    migrationOffset=0;migrationSequence=-1;commitSent=false;testingRoutes=false;reached={};delivered={};migrationMode=MigrationMode::Preparing;migrationDeadline=ConnectionNow()+10;
    auto offer=Packet(MigrationOffer);offer.Text(migrationToken);offer.Byte(successor);offer.Byte(me);offer.Int(int(migrationData.bytes.size()));wire.Send(fighters[successor].peer,offer.bytes);
    status="Transferindo sala para "+fighters[successor].name;ConnectionTrace(status);return true;
}
bool BeginHostMigration(){if(!host||tester)return false;if(migrationMode!=MigrationMode::None)return true;migrationTried={};return StartMigrationCandidate();}
void RetryMigrationCandidate(){migrationProbe.Stop();testingRoutes=false;auto cancel=Packet(MigrationCancel);cancel.Text(migrationToken);wire.Broadcast(cancel.bytes);StartMigrationCandidate();}
void SendResume(){auto hello=Packet(ResumeHello);hello.Int(Version);hello.Text(migrationToken);hello.Byte(me);hello.Text(resumeTicket);wire.Send(0,hello.bytes);}
void FinishPromotion(){
    std::array<bool,Teams> departedTeams{};if(removedHost>=0)departedTeams[fighters[removedHost].team]=true;
    for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected&&!resumed[i]){departedTeams[fighters[i].team]=true;fighters[i].connected=false;fighters[i].eliminated=true;fighters[i].bed=false;}
    if(phase!=0)for(int team=0;team<TeamLimit();++team)if(departedTeams[team]&&TeamCount(team)==0)RemoveBed(team,false);
    auto resume=Packet(MigrationResume);resume.Text(migrationToken);wire.Broadcast(resume.bytes);
    migrationData={};migrationMode=MigrationMode::None;status="Host: "+fighters[me].name;positionClock=snapshotClock=0;UpdateDiscovery();SendState();
}
bool StartProbe(const std::string& address){probeNonce=RandomTicket();probeAnswered=false;return migrationProbe.Join(address);}
int PollProbe(){
    try{for(auto event:migrationProbe.Poll()){
        if(event.kind==LanTransport::Event::Connected){auto question=Packet(ProbeQuestion);question.Text(migrationToken);question.Text(probeNonce);migrationProbe.Send(0,question.bytes);}
        else if(event.kind==LanTransport::Event::Data){Buffer reply{event.data};if(reply.Byte()==ProbeAnswer&&reply.Text()==migrationToken&&reply.Text()==probeNonce){probeAnswered=true;migrationProbe.Stop();return 1;}}
        else if(event.kind==LanTransport::Event::Disconnected)return -1;
    }}catch(const std::exception&){migrationProbe.Stop();return -1;}
    return migrationProbe.Alive()?0:-1;
}
void SendMigrationCommit(){commitSent=true;testingRoutes=false;migrationDeadline=ConnectionNow()+3;
    for(int i=0;i<MaxPlayers;++i)if(i!=me&&fighters[i].connected){auto commit=Packet(MigrationCommit);commit.Text(migrationToken);commit.Byte(migrationTarget);commit.Byte(me);commit.Text(migrationAddress);commit.Text(resumeTickets[i]);wire.Send(fighters[i].peer,commit.bytes);}
}
void TickMigration(){
    if(migrationMode==MigrationMode::Receiving&&nextHostWire.Alive()){
        for(auto event:nextHostWire.Poll())if(event.kind==LanTransport::Event::Data){try{Buffer question{event.data};
            if(question.Byte()==ProbeQuestion&&question.Text()==migrationToken){auto nonce=question.Text();auto answer=Packet(ProbeAnswer);answer.Text(migrationToken);answer.Text(nonce);nextHostWire.Send(event.peer,answer.bytes);}}catch(const std::exception&){}nextHostWire.Drop(event.peer);
        }
    }
    if(migrationMode==MigrationMode::Probing&&!probeAnswered){int result=PollProbe();if(result){auto reachable=Packet(MigrationReachable);reachable.Text(migrationToken);reachable.Byte(result>0);wire.Send(0,reachable.bytes);probeAnswered=true;}}
    if(migrationMode==MigrationMode::Preparing){
        if(testingRoutes&&!reached[me]){int result=PollProbe();if(result<0){RetryMigrationCandidate();return;}if(result>0)reached[me]=true;}
        if(testingRoutes){bool reachable=true;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected&&!reached[i])reachable=false;if(reachable)SendMigrationCommit();}
        bool all=true;for(int i=0;i<MaxPlayers;++i)if(i!=me&&fighters[i].connected&&!delivered[i])all=false;
        if(commitSent&&(all||ConnectionNow()>migrationDeadline)){status="LoL try to cheating.";Stop();return;}
        if(!commitSent&&ConnectionNow()>migrationDeadline)RetryMigrationCandidate();
    }else if(migrationMode==MigrationMode::Hosting){bool all=true;for(int i=0;i<MaxPlayers;++i)if(fighters[i].connected&&!resumed[i])all=false;if(all||ConnectionNow()>migrationDeadline)FinishPromotion();}
    else if(migrationMode!=MigrationMode::None&&ConnectionNow()>migrationDeadline){status="Nao foi possivel transferir a sala. Confira rede e firewall.";Stop();}
}
bool HandleMigration(int peer,Buffer& b,bool server){
    if(b.bytes.empty())return false;
    int kind=b.bytes[0];
    if(kind<MigrationOffer||kind>MigrationReachable)return false;
    b.Byte();
    if(server){
        if(kind==ResumeHello){int version=b.Int();auto token=b.Text();int id=b.Byte();auto ticket=b.Text();
            if(version!=Version||token!=migrationToken||ConnectionNow()>resumeUntil||id>=MaxPlayers||id==me||!fighters[id].connected||resumed[id]||ticket!=resumeTickets[id]||peerTeam.count(peer)){wire.Drop(peer);return true;}
            peerTeam[peer]=id;fighters[id].peer=peer;resumed[id]=true;fighters[id].guard.ResetMotion(clock);auto welcome=Packet(ResumeWelcome);welcome.Byte(id);welcome.Byte(me);wire.Send(peer,welcome.bytes);SendState();return true;
        }
        auto known=peerTeam.find(peer);if(migrationMode!=MigrationMode::Preparing||known==peerTeam.end())return true;
        if(kind==MigrationReachable){auto token=b.Text();int success=b.Byte();if(testingRoutes&&token==migrationToken){if(success==1)reached[known->second]=true;else RetryMigrationCandidate();}return true;}
        if(kind==MigrationAck){auto token=b.Text();if(commitSent&&token==migrationToken)delivered[known->second]=true;return true;}
        if(known->second!=migrationTarget||b.Text()!=migrationToken)return true;
        if(kind==MigrationChunkAck){int sequence=b.Int();if(sequence!=migrationSequence)return true;if(migrationOffset<migrationData.bytes.size()){++migrationSequence;SendMigrationChunk();}return true;}
        if(kind==MigrationReady){if(migrationOffset!=migrationData.bytes.size())return true;int port=b.Int();if(port<1||port>65535){RetryMigrationCandidate();return true;}std::string ip=wire.PeerIP(peer);if(ip.empty()){RetryMigrationCandidate();return true;}
            migrationAddress=ip+":"+std::to_string(port);testingRoutes=true;reached={};reached[migrationTarget]=true;migrationDeadline=ConnectionNow()+10;
            if(!StartProbe(migrationAddress)){RetryMigrationCandidate();return true;}
            for(int i=0;i<MaxPlayers;++i)if(i!=me&&i!=migrationTarget&&fighters[i].connected){auto test=Packet(MigrationTest);test.Text(migrationToken);test.Byte(migrationTarget);test.Byte(me);test.Text(migrationAddress);wire.Send(fighters[i].peer,test.bytes);}
            return true;
        }return true;
    }
    if(kind==MigrationOffer){auto token=b.Text();int successor=b.Byte(),old=b.Byte(),size=b.Int();
        if(!ready||successor!=me||old!=authorityId||size<1||size>32*1024*1024||migrationMode!=MigrationMode::None)return true;
        migrationToken=token;migrationTarget=successor;removedHost=old;migrationExpected=size;migrationData={};migrationData.bytes.reserve(size);migrationSequence=0;migrationMode=MigrationMode::Receiving;status="Recebendo estado da sala...";migrationDeadline=ConnectionNow()+10;
        auto ack=Packet(MigrationChunkAck);ack.Text(token);ack.Int(-1);wire.Send(0,ack.bytes);return true;
    }
    if(kind==MigrationChunk){auto token=b.Text();int sequence=b.Int(),size=b.Int();if(migrationMode!=MigrationMode::Receiving||token!=migrationToken||sequence!=migrationSequence||size<0||size>48000||size_t(size)>b.bytes.size()-b.offset||migrationData.bytes.size()+size>size_t(migrationExpected))return true;
        migrationData.bytes.insert(migrationData.bytes.end(),b.bytes.begin()+b.offset,b.bytes.begin()+b.offset+size);migrationDeadline=ConnectionNow()+10;
        if(migrationData.bytes.size()==size_t(migrationExpected)){LoadMigrationCheckpoint(migrationData);auto prepared=Packet(MigrationReady);prepared.Text(migrationToken);bool listening=nextHostWire.Host(0);prepared.Int(listening?nextHostWire.Port():0);migrationDeadline=ConnectionNow()+20;wire.Send(0,prepared.bytes);}
        else{auto ack=Packet(MigrationChunkAck);ack.Text(token);ack.Int(migrationSequence);wire.Send(0,ack.bytes);++migrationSequence;}return true;
    }
    if(kind==MigrationCancel){if(b.Text()==migrationToken&&(migrationMode==MigrationMode::Receiving||migrationMode==MigrationMode::Probing)){nextHostWire.Stop();migrationProbe.Stop();migrationData={};migrationMode=MigrationMode::None;}return true;}
    if(kind==MigrationTest){auto token=b.Text();int successor=b.Byte(),old=b.Byte();auto address=b.Text();if(!ready||successor>=MaxPlayers||old!=authorityId||successor==me||migrationMode!=MigrationMode::None)return true;
        migrationToken=token;migrationTarget=successor;removedHost=old;migrationMode=MigrationMode::Probing;migrationDeadline=ConnectionNow()+15;status="Verificando conexao com o novo host...";
        if(!StartProbe(address)){auto failure=Packet(MigrationReachable);failure.Text(token);failure.Byte(0);wire.Send(0,failure.bytes);probeAnswered=true;}return true;
    }
    if(kind==MigrationCommit){auto token=b.Text();int successor=b.Byte(),old=b.Byte();auto address=b.Text(),ticket=b.Text();if(!ready||successor>=MaxPlayers||old!=authorityId||!fighters[successor].connected)return true;
        if(successor==me&&(migrationMode!=MigrationMode::Receiving||token!=migrationToken||!nextHostWire.Alive()))return true;
        auto ack=Packet(MigrationAck);ack.Text(token);wire.Send(0,ack.bytes);
        migrationToken=token;resumeTicket=ticket;migrationTarget=successor;removedHost=old;authorityId=successor;migrationOwnCorrection=true;for(auto& f:fighters){f.guard.observedGeneration=-1;f.guard.observedEpoch=-1;}changedTransport=true;positionClock=0;
        if(successor==me){wire.Swap(nextHostWire);nextHostWire.Stop();host=true;roomPort=wire.Port();peerTeam.clear();resumed={};
            for(int i=0;i<MaxPlayers;++i){fighters[i].peer=-1;fighters[i].guard.ResetMotion(clock);++fighters[i].teleport;resumed[i]=!fighters[i].connected||i==me||i==old;}
            fighters[me].peer=0;fighters[old].connected=false;fighters[old].eliminated=true;fighters[old].bed=false;migrationMode=MigrationMode::Hosting;migrationDeadline=ConnectionNow()+8;resumeUntil=ConnectionNow()+15;discovery.StopHosting();status="Assumindo a sala...";
        }else{wire.Stop();if(!wire.Join(address)){status=wire.Error();Stop();return true;}migrationMode=MigrationMode::Reconnecting;migrationDeadline=ConnectionNow()+15;status="Conectando ao novo host...";}
        return true;
    }
    if(kind==MigrationResume){if(migrationMode==MigrationMode::Reconnecting&&b.Text()==migrationToken){migrationMode=MigrationMode::None;status="Host: "+fighters[authorityId].name;}return true;}
    if(kind==ResumeWelcome){int id=b.Byte(),newHost=b.Byte();if(migrationMode!=MigrationMode::Reconnecting||id!=me||newHost!=authorityId)return true;status="Aguardando outros jogadores...";return true;}
    return true;
}
