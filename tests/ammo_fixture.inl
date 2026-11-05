//Wait for the real recorder receipt instead of racing a 350ms sleep against a worker.
//Only test synchronization changes; all original packet/value/commit assertions remain.
bool AmmoFixtureReceipt338(const wchar_t* path,UINT address,UINT loaded,UINT reserve,UINT begin=0){
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
 LARGE_INTEGER offset={};offset.QuadPart=begin;SetFilePointerEx(file,offset,nullptr,FILE_BEGIN);AmmoPacket packet={};DWORD bytes=0;bool found=false;
 while(ReadFile(file,&packet,sizeof(packet),&bytes,nullptr)&&bytes==sizeof(packet))if(packet.magic==0x414d5031&&packet.type>=1&&packet.type<=3&&packet.address==address&&packet.value==loaded&&packet.nearby[35]==reserve){found=true;break;}
 CloseHandle(file);return found;
}
void AwaitAmmoFixture338(const wchar_t* path,UINT address,UINT loaded,UINT reserve,const char* label,UINT begin=0){const auto start=GetTickCount64();while(!AmmoFixtureReceipt338(path,address,loaded,reserve,begin)){Check(GetTickCount64()-start<8000,label);Sleep(20);}}
void AwaitAmmoCommits338(UINT count){const auto start=GetTickCount64();for(;;){UINT seen=0;AmmoRead(reinterpret_cast<UINT>(&g_ammoOwner.observedCommits),&seen,sizeof(seen));if(seen>=count)return;Check(GetTickCount64()-start<8000,"recorder confirms each conserved synthetic insertion before next write");Sleep(20);}}
void TestAmmoLifecycle(bool earlyReload=false)
{
    GetTempPathW(MAX_PATH,g_ammoTestFolder);
    auto* page=static_cast<UINT*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(page!=nullptr,"lifecycle counter allocation");volatile UINT* observed=page;observed[32]=6;observed[33]=7;observed[34]=6;observed[35]=12;observed[36]=18;
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    g_ammo=AmmoProbe{};g_ammo.previousShot=g_lastShotFrame;g_ammo.quiet=GetTickCount64()-12001;
    g_visibleWeaponDrawsLastFrame=0;g_ammo.quiet=GetTickCount64()-kAmmoQuietMs+1000;
    TickAmmoProbe(true);Check(g_ammo.phase==AmmoPhase::Quiet&&g_ammo.wait==AmmoWait::Settling&&g_ammo.quiet,"missing visible draw preserves setup countdown");
    controls.x=true;fc3pad::Publish(controls);TickAmmoProbe(true);
    Check(g_ammo.wait==AmmoWait::Reload&&g_ammo.phase==AmmoPhase::Quiet,"held reload has explicit waiting reason");
    controls.x=false;controls.rightTrigger=1;fc3pad::Publish(controls);TickAmmoProbe(true);
    Check(g_ammo.wait==AmmoWait::Trigger,"held trigger has explicit waiting reason");
    controls.rightTrigger=0;controls.rightStickClick=true;fc3pad::Publish(controls);TickAmmoProbe(true);
    Check(g_ammo.wait==AmmoWait::Melee,"held melee has explicit waiting reason");
    controls.rightStickClick=false;fc3pad::Publish(controls);g_ammo.quiet=GetTickCount64()-kAmmoQuietMs-1;
    auto waitWorker=[](){const auto start=GetTickCount64();while(InterlockedCompareExchange(&g_ammo.worker,0,0)==1){Check(GetTickCount64()-start<25000,"real worker timeout");Sleep(10);}};
    TickAmmoProbe(true);Check(g_ammo.phase==AmmoPhase::Seed,"quiet starts real baseline worker");
    waitWorker();TickAmmoProbe(true);Check(g_ammo.phase==AmmoPhase::FirstShot,"real baseline arms first shot");
    wchar_t trace[MAX_PATH]={};wcscpy_s(trace,g_ammo.tracePath);
    const int shots=earlyReload?1:2;
    for(int shot=0;shot<shots;++shot)
    {
        --observed[32];observed[36]=observed[32]+observed[35];++g_lastShotFrame;TickAmmoProbe(true);
        Check(g_ammo.phase==(shot?AmmoPhase::SecondSettle:AmmoPhase::FirstSettle),"shot edge enters correct settle phase");
        g_ammo.settle=GetTickCount64()-501;TickAmmoProbe(true);waitWorker();TickAmmoProbe(true);
        Check(g_ammo.phase==(shot?AmmoPhase::Reload:AmmoPhase::SecondShot),"real filter advances capture sequence");
    }
    observed[36]=observed[32]+observed[35];const UINT watchStart338=g_ammo.bytesWritten;controls.x=true;fc3pad::Publish(controls);TickAmmoProbe(true);
    Check(g_ammo.phase==AmmoPhase::Watching&&g_ammo.worker==1,"X starts real twelve-second observation");
    AwaitAmmoFixture338(trace,reinterpret_cast<UINT>(page+32),earlyReload?5:4,12,"watcher samples initial value before synthetic reload",watchStart338);observed[32]=earlyReload?6:5;observed[35]=11;AwaitAmmoFixture338(trace,reinterpret_cast<UINT>(page+32),earlyReload?6:5,11,"watcher samples first synthetic reload",watchStart338);AwaitAmmoCommits338(1);
    if(earlyReload)
    {
        observed[32]=5;observed[36]=16;++g_lastShotFrame;TickAmmoProbe(true);
        Check(g_ammo.phase==AmmoPhase::Watching&&!g_ammo.rearm,"subsequent shot retained inside early reload observation");
        controls.x=false;fc3pad::Publish(controls);TickAmmoProbe(true);AwaitAmmoFixture338(trace,reinterpret_cast<UINT>(page+32),5,11,"watcher samples intervening synthetic shot",watchStart338);
        observed[32]=6;observed[35]=10;controls.x=true;fc3pad::Publish(controls);TickAmmoProbe(true);
        Check(g_ammo.phase==AmmoPhase::Watching&&!g_ammo.rearm,"second reload retained rather than restarting baseline");
    }
    else {observed[32]=6;observed[35]=10;}
    AwaitAmmoFixture338(trace,reinterpret_cast<UINT>(page+32),6,10,"watcher samples second synthetic reload",watchStart338);AwaitAmmoCommits338(2);
    controls.active=false;fc3pad::Publish(controls);TickAmmoProbe(false);
    Check(g_ammo.phase==AmmoPhase::Watching&&!g_ammo.cancel&&!g_ammo.rearm,"tracking/gameplay interruption preserves ongoing reload observation");
    waitWorker();TickAmmoProbe(false);Check(g_ammo.phase==AmmoPhase::Done,"reload worker completes while headset inactive");
    HANDLE file=CreateFileW(trace,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    Check(file!=INVALID_HANDLE_VALUE,"unique capture remains readable");
    bool values[3]={};UINT snapshots=0;AmmoPacket p;DWORD bytes=0;
    const UINT address=static_cast<UINT>(reinterpret_cast<uintptr_t>(page+32));
    while(ReadFile(file,&p,sizeof(p),&bytes,nullptr)&&bytes)
    {
        Check(bytes==sizeof(p)&&p.magic==0x414d5031&&p.version==1,"real worker writes complete AMP1 packets");
        if(p.address==address&&p.type>=1&&p.type<=3&&p.value>=4&&p.value<=6)
        {
            values[p.value-4]=true;++snapshots;
            Check(p.nearby[32]+p.nearby[35]==p.nearby[36]&&p.nearby[33]==7,"actual worker records conserved reserve and unchanged capacity");
        }
    }
    CloseHandle(file);Check((earlyReload||values[0])&&values[1]&&values[2]&&snapshots>3,"full lifecycle retains shot decrements and both single-shell reload commits");
    Check(g_ammoOwner.count==1&&g_ammoOwner.observedCommits==2&&g_ammoOwner.targets[0].address==address,"real reload worker selects writer target only after two conserved insertions");
    g_ammoOwner=AmmoOwnerState{};
    // Successful completion stays Done; restart retention concerns interrupted attempts.
    g_ammo.phase=AmmoPhase::FirstShot;
    AmmoRestart("fixture interrupted attempt");TickAmmoProbe(true);Check(g_ammo.tracePath[0]==0,"new baseline receives separate trace identity");
    Check(GetFileAttributesW(trace)!=INVALID_FILE_ATTRIBUTES,"restart cannot erase previous attempt");
    DeleteFileW(trace);wchar_t latest[MAX_PATH];_snwprintf_s(latest,MAX_PATH,_TRUNCATE,L"%sfc3_ammo_probe.bin",g_ammoTestFolder);DeleteFileW(latest);
    VirtualFree(page,0,MEM_RELEASE);g_ammo=AmmoProbe{};
    std::puts(earlyReload?"PASS: real worker one-shot early reload, subsequent shot/reload retained, two conserved inserts select relocated writer":"PASS: real worker full baseline -> two shots -> reload; inactive headset preserves commits; restart retains capture");
}
void TestAmmoProbe()
{
    const LONG savedVisible=g_visibleWeaponDrawsLastFrame;g_visibleWeaponDrawsLastFrame=1;
    auto* page=static_cast<UINT*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"ammo fixture pages");
    page[32]=6;page[33]=12;page[34]=6;page[64]=6;page[96]=6;page[120]=3;
    InterlockedExchange(&g_ammo.cancel,0);g_ammo.capped=false;
    HMODULE owner=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&g_ammo),&owner);
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(owner);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(owner)+dos->e_lfanew);
    g_ammo.ownerStart=reinterpret_cast<uintptr_t>(owner);g_ammo.ownerEnd=g_ammo.ownerStart+nt->OptionalHeader.SizeOfImage;
    AmmoSeed();bool found=false;
    for(const auto& c:g_ammo.candidates)
    {
        if(c.address==static_cast<UINT>(reinterpret_cast<uintptr_t>(page+32))&&c.initial==6)found=true;
        const uintptr_t own=reinterpret_cast<uintptr_t>(g_ammo.candidates.data());
        Check(c.address<own||c.address>=own+g_ammo.candidates.capacity()*sizeof(AmmoCandidate),"scanner excludes its own candidate copies");
    }
    Check(found&&page[32]==6&&page[33]==12,"full process baseline finds real heap counter without writing it");
    g_ammo.candidates.clear();InterlockedExchange(&g_ammo.cancel,0);g_ammo.bytesWritten=0;g_ammo.begun=GetTickCount64();g_ammo.round=2;InterlockedExchange(&g_ammo.frame,42);
    auto candidate=[&](int at){UINT address=static_cast<UINT>(reinterpret_cast<uintptr_t>(page+at));g_ammo.candidates.push_back({address,static_cast<BYTE>(page[at]),static_cast<BYTE>(page[at])});};
    candidate(32);candidate(64);candidate(96);candidate(120);
    page[32]=page[64]=5;page[120]=2;UINT expected[1024];memcpy(expected,page,sizeof(expected));
    AmmoFilter(true);Check(g_ammo.candidates.size()==3,"shot removes unchanged capacity and unrelated integer");Check(memcmp(page,expected,sizeof(expected))==0,"filter never writes source process memory");
    page[120]=1;AmmoFilter(false);Check(g_ammo.candidates.size()==2,"settled second read removes transient decrement");
    page[32]=page[64]=4;AmmoFilter(true);AmmoFilter(false);Check(g_ammo.candidates.size()==2&&g_ammo.candidates[0].initial==6&&g_ammo.candidates[0].last==4,"two shots preserve exact initial-to-current ammo hypotheses");
    wchar_t temp[MAX_PATH],filePath[MAX_PATH];GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"amp",0,filePath);
    HANDLE file=CreateFileW(filePath,GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,nullptr);Check(file!=INVALID_HANDLE_VALUE,"ammo fixture trace");
    AmmoSnapshot(file,g_ammo.candidates[0],1);page[32]=5;page[33]=11;g_ammo.candidates[0].last=5;AmmoSnapshot(file,g_ammo.candidates[0],3);
    Check(g_ammo.bytesWritten==2*sizeof(AmmoPacket),"bounded ammo packet serialization");SetFilePointer(file,0,nullptr,FILE_BEGIN);
    for(int i=0;i<2;++i)
    {
        AmmoPacket p;DWORD n=0;Check(ReadFile(file,&p,sizeof(p),&n,nullptr)&&n==sizeof(p),"ammo trace read");
        Check(p.magic==0x414d5031&&p.version==1&&p.phase==2&&p.frame==42,"ammo packet version and timeline");
        Check(p.initial==6&&p.value==static_cast<UINT>(i?5:4)&&p.nearBytes==260,"candidate and readable-neighbor size");
        Check(p.nearby[32]==static_cast<UINT>(i?5:4)&&p.nearby[33]==static_cast<UINT>(i?11:12)&&p.nearby[34]==6,"single-shell load/reserve decrement and capacity visible in independent fields");
    }
    CloseHandle(file);DeleteFileW(filePath);
    DWORD old=0;Check(VirtualProtect(page+1024,4096,PAGE_READWRITE|PAGE_GUARD,&old)!=FALSE,"guard page");UINT out=0;
    Check(!AmmoRead(static_cast<UINT>(reinterpret_cast<uintptr_t>(page+1024)),&out,4),"guard page rejected before read");
    Check(!AmmoRead(static_cast<UINT>(reinterpret_cast<uintptr_t>(page+1023)),&out,8),"read across region boundary rejected");
    MEMORY_BASIC_INFORMATION region={};VirtualQuery(page+1024,&region,sizeof(region));Check((region.Protect&PAGE_GUARD)!=0,"probe never consumes the game's guard page");
    const UINT stale=static_cast<UINT>(reinterpret_cast<uintptr_t>(page+32));VirtualFree(page,0,MEM_RELEASE);g_ammo.candidates={{stale,6,5}};AmmoFilter(true);Check(g_ammo.candidates.empty(),"released source allocation safely rejected");
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);g_ammo.phase=AmmoPhase::FirstShot;g_ammo.previousShot=8000;g_ammo.previousX=false;g_lastShotFrame=8001;
    const bool chamber=g_chambered;const int pump=g_pumpStage;TickAmmoProbe(true);
    Check(g_ammo.phase==AmmoPhase::FirstSettle&&g_chambered==chamber&&g_pumpStage==pump,"shot request schedules observation without changing pump/chamber state");
    TickAmmoProbe(false);Check(g_ammo.rearm&&g_ammo.cancel==1&&g_chambered==chamber&&g_pumpStage==pump,"gameplay interruption rearms without touching the weapon");
    TickAmmoProbe(false);Check(g_ammo.phase==AmmoPhase::Quiet&&!g_ammo.cancel,"interrupted idle probe returns to quiet baseline");
    TickAmmoProbe(false);Check(g_ammo.quiet==0,"no quiet countdown outside gameplay");
    g_ammo.phase=AmmoPhase::FirstShot;controls.active=false;fc3pad::Publish(controls);
    InterlockedExchange(&g_ammo.worker,1);g_ammo.candidates={{0x2000,6,6}};const auto* lostTrackingOwned=g_ammo.candidates.data();
    TickAmmoProbe(true);Check(g_ammo.rearm&&g_ammo.cancel,"controller tracking loss rearms probe");
    TickAmmoProbe(true);Check(g_ammo.candidates.data()==lostTrackingOwned,"tracking interruption retains live worker storage");
    InterlockedExchange(&g_ammo.worker,2);TickAmmoProbe(true);TickAmmoProbe(true);
    Check(g_ammo.phase==AmmoPhase::Quiet&&g_ammo.quiet==0,"inactive controllers cannot start baseline");
    controls.active=true;fc3pad::Publish(controls);TickAmmoProbe(true);
    Check(g_ammo.phase==AmmoPhase::Quiet&&g_ammo.quiet!=0,"tracking recovery starts fresh quiet interval");
    // A setup reload after READY must rearm instead of permanently ending capture.
    g_ammo=AmmoProbe{};g_ammo.phase=AmmoPhase::FirstShot;g_ammo.previousShot=g_lastShotFrame;
    controls.x=true;fc3pad::Publish(controls);InterlockedExchange(&g_ammo.worker,1);
    g_ammo.candidates={{0x2000,6,6}};const auto* owned=g_ammo.candidates.data();TickAmmoProbe(true);
    Check(g_ammo.rearm&&g_ammo.cancel==1,"early reload schedules automatic baseline restart");
    TickAmmoProbe(true);Check(g_ammo.candidates.data()==owned&&g_ammo.candidates.size()==1,"restart cannot free data owned by an active worker");
    InterlockedExchange(&g_ammo.worker,2);TickAmmoProbe(true);
    Check(g_ammo.phase==AmmoPhase::Quiet&&!g_ammo.rearm&&!g_ammo.cancel&&g_ammo.candidates.empty(),"finished worker permits fresh post-reload baseline");
    g_ammo=AmmoProbe{};g_ammo.phase=AmmoPhase::FirstFilter;g_ammo.round=1;g_ammo.previousShot=g_lastShotFrame;g_ammo.previousX=false;
    InterlockedExchange(&g_ammo.worker,1);TickAmmoProbe(true);
    Check(g_ammo.pendingReload&&!g_ammo.cancel&&!g_ammo.rearm,"reload during live filter queues observation without discarding sample");
    InterlockedExchange(&g_ammo.worker,0);g_ammo=AmmoProbe{};
    g_ammo.phase=AmmoPhase::FirstSettle;g_ammo.settle=GetTickCount64();g_ammo.previousShot=g_lastShotFrame;
    TickAmmoProbe(true);Check(g_ammo.pendingReload&&!g_ammo.rearm&&!g_ammo.worker,"reload immediately after shot queues without losing settling sample");g_ammo=AmmoProbe{};
    std::puts("PASS: read-only ammo candidate narrowing, stable two-shot decrement, independent reserve/capacity reload fields, trace bounds, guard/deallocated pages and unchanged pump state");
    TestAmmoLifecycle();
    TestAmmoLifecycle(true);
    g_visibleWeaponDrawsLastFrame=savedVisible;
}
