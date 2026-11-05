// Read-only temporal integer probe. Candidates are hypotheses, never ammo writes.
struct AmmoCandidate { UINT address; BYTE initial,last; USHORT reserved=0; };
static_assert(sizeof(AmmoCandidate)==8,"compact ammo hypotheses");
#pragma pack(push,4)
struct AmmoPacket
{
    UINT magic=0x414d5031,version=1,type=0,phase=0,frame=0,milliseconds=0;
    UINT address=0,initial=0,value=0,nearBase=0,nearBytes=0;
    UINT nearby[65]={};
};
#pragma pack(pop)
static_assert(sizeof(AmmoPacket)==304,"ammo packet layout");
enum class AmmoPhase { Quiet, Seed, FirstShot, FirstSettle, FirstFilter, SecondShot,
                       SecondSettle, SecondFilter, Reload, Watching, Done };
enum class AmmoWait { Gameplay, Tracking, Reload, Trigger, Melee, Settling };
constexpr ULONGLONG kAmmoQuietMs=4000;
struct AmmoProbe
{
    std::vector<AmmoCandidate> candidates;
    volatile LONG worker=0,cancel=0,frame=0;
    AmmoPhase phase=AmmoPhase::Quiet;
    UINT job=0,round=0;
    AmmoWait wait=AmmoWait::Gameplay;
    LONG previousShot=-100000;
    ULONGLONG quiet=0,settle=0,begun=0;
    bool previousX=false,capped=false,rearm=false,pendingReload=false;
    UINT bytesWritten=0;
    uintptr_t ownerStart=0,ownerEnd=0;
    wchar_t tracePath[MAX_PATH]={};
} g_ammo;
#ifdef FC3_AMMO_TEST
static wchar_t g_ammoTestFolder[MAX_PATH]={};
#endif
const wchar_t* AmmoFolder()
{
#ifdef FC3_AMMO_TEST
    return g_ammoTestFolder;
#else
    return fc3::DataFolder();
#endif
}
bool AmmoReadable(const MEMORY_BASIC_INFORMATION& m)
{
    return m.State==MEM_COMMIT&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&
        (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
bool AmmoRead(UINT address,void* destination,SIZE_T bytes)
{
    MEMORY_BASIC_INFORMATION m={};
    if(!VirtualQuery(reinterpret_cast<void*>(static_cast<uintptr_t>(address)),&m,sizeof(m))||!AmmoReadable(m))return false;
    const uintptr_t end=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
    if(static_cast<uintptr_t>(address)>=end||bytes>end-address)return false;
    SIZE_T read=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(static_cast<uintptr_t>(address)),destination,bytes,&read)&&read==bytes;
}
bool AmmoWritePacket(HANDLE file,AmmoPacket& packet)
{
    if(g_ammo.bytesWritten+sizeof(packet)>64*1024*1024)return false;
    packet.phase=g_ammo.round;packet.frame=static_cast<UINT>(InterlockedCompareExchange(&g_ammo.frame,0,0));
    packet.milliseconds=static_cast<UINT>(GetTickCount64()-g_ammo.begun);
    DWORD written=0;bool ok=WriteFile(file,&packet,sizeof(packet),&written,nullptr)&&written==sizeof(packet);
    if(ok)g_ammo.bytesWritten+=written;return ok;
}
void AmmoSnapshot(HANDLE file,const AmmoCandidate& c,UINT type,UINT observed=0xffffffffu)
{
    AmmoPacket p;p.type=type;p.address=c.address;p.initial=c.initial;p.value=observed==0xffffffffu?c.last:observed;
    MEMORY_BASIC_INFORMATION m={};
    if(VirtualQuery(reinterpret_cast<void*>(static_cast<uintptr_t>(c.address)),&m,sizeof(m))&&AmmoReadable(m))
    {
        const uintptr_t lo=reinterpret_cast<uintptr_t>(m.BaseAddress),hi=lo+m.RegionSize;
        uintptr_t first=(std::max)(lo,static_cast<uintptr_t>(c.address>128?c.address-128:0));
        SIZE_T length=(std::min)(static_cast<SIZE_T>(sizeof(p.nearby)),static_cast<SIZE_T>(hi-first));
        if(AmmoRead(static_cast<UINT>(first),p.nearby,length)){p.nearBase=static_cast<UINT>(first);p.nearBytes=static_cast<UINT>(length);}
    }
    AmmoWritePacket(file,p);
}
// Group candidate reads by page so the game/render thread never performs a
// full-address-space scan or one kernel call per candidate.
void AmmoFilter(bool decrement)
{
    UINT page=0xffffffffu,words[1024]={};bool readable=false;size_t keep=0;
    for(size_t i=0;i<g_ammo.candidates.size();++i)
    {
        if(InterlockedCompareExchange(&g_ammo.cancel,0,0))break;
        auto c=g_ammo.candidates[i];const UINT next=c.address&~4095u;
        if(next!=page){page=next;readable=AmmoRead(page,words,sizeof(words));}
        if(!readable)continue;const UINT value=words[(c.address-page)/4];
        if(decrement?(c.last>0&&value==static_cast<UINT>(c.last)-1u):value==c.last)
        {c.last=static_cast<BYTE>(value);g_ammo.candidates[keep++]=c;}
    }
    g_ammo.candidates.resize(keep);
}
bool AmmoOutsideProbe(uintptr_t address,uintptr_t stackLo,uintptr_t stackHi)
{
    const uintptr_t vectorLo=reinterpret_cast<uintptr_t>(g_ammo.candidates.data());
    const uintptr_t vectorHi=vectorLo+g_ammo.candidates.capacity()*sizeof(AmmoCandidate);
    return !(address>=g_ammo.ownerStart&&address<g_ammo.ownerEnd)&&
        !(address>=stackLo&&address<stackHi)&&!(address>=vectorLo&&address<vectorHi);
}
void AmmoSeed()
{
    constexpr size_t limit=8000000; // 64 MiB, four times the original address coverage
    g_ammo.candidates.clear();g_ammo.candidates.reserve(limit);
    UINT words[16384]={};MEMORY_BASIC_INFORMATION stack={};VirtualQuery(words,&stack,sizeof(stack));
    uintptr_t stackLo=reinterpret_cast<uintptr_t>(stack.AllocationBase),stackHi=stackLo;
    MEMORY_BASIC_INFORMATION range={};
    while(VirtualQuery(reinterpret_cast<void*>(stackHi),&range,sizeof(range))&&range.AllocationBase==stack.AllocationBase)
    {uintptr_t next=reinterpret_cast<uintptr_t>(range.BaseAddress)+range.RegionSize;if(next<=stackHi)break;stackHi=next;}
    SYSTEM_INFO si={};GetSystemInfo(&si);
    uintptr_t address=reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress),end=reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    ULONGLONG bytes=0,start=GetTickCount64();
    while(address<end&&!InterlockedCompareExchange(&g_ammo.cancel,0,0))
    {
        MEMORY_BASIC_INFORMATION m={};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)))break;
        uintptr_t base=reinterpret_cast<uintptr_t>(m.BaseAddress),last=base+m.RegionSize;
        if(last<=address)break;
        if(AmmoReadable(m)&&(m.Type==MEM_PRIVATE||m.Type==MEM_IMAGE))
        {
            for(uintptr_t chunk=base;chunk<last;chunk+=sizeof(words))
            {
                if(InterlockedCompareExchange(&g_ammo.cancel,0,0))break;
                const SIZE_T size=(std::min)(static_cast<SIZE_T>(sizeof(words)),static_cast<SIZE_T>(last-chunk));
                if(!AmmoOutsideProbe(chunk,stackLo,stackHi)||!AmmoRead(static_cast<UINT>(chunk),words,size))continue;
                bytes+=size;
                for(SIZE_T at=0;at+4<=size;at+=4)
                {
                    const UINT value=words[at/4];if(value<2||value>16)continue;
                    const uintptr_t location=chunk+at;if(!AmmoOutsideProbe(location,stackLo,stackHi))continue;
                    if(g_ammo.candidates.size()==limit){g_ammo.capped=true;break;}
                    g_ammo.candidates.push_back({static_cast<UINT>(location),static_cast<BYTE>(value),static_cast<BYTE>(value)});
                }
                if(g_ammo.capped)break;
            }
        }
        if(g_ammo.capped||GetTickCount64()-start>20000){g_ammo.capped=true;break;}address=last;
    }
    Log("[AMMO PROBE] baseline: %u candidates, %.0f MiB read in %.2f sec%s; uint32 range 2..16",
        static_cast<UINT>(g_ammo.candidates.size()),bytes/1048576.0,(GetTickCount64()-start)/1000.0,g_ammo.capped?" (coverage capped)":"");
}
void AmmoOwnerObserved(UINT address,UINT loaded,UINT reserve,UINT matches);
void AmmoWatch(HANDLE file)
{
    const ULONGLONG started=GetTickCount64();UINT previous[4096]={},previousReserve[4096]={};
#ifdef FC3_AMMO_TEST
    const UINT watchMilliseconds=12000;
#else
    const UINT watchMilliseconds=60000;
#endif
    ULONGLONG nextFullSample=started;
    for(size_t i=0;i<g_ammo.candidates.size();++i)
    {previous[i]=g_ammo.candidates[i].last;previousReserve[i]=0xffffffffu;AmmoRead(g_ammo.candidates[i].address+12,&previousReserve[i],4);}
    // Snapshot all nearby values, not just the loaded-counter candidate. This
    // can expose reserve/capacity/state fields when shells commit during reload.
    while(GetTickCount64()-started<watchMilliseconds&&!InterlockedCompareExchange(&g_ammo.cancel,0,0))
    {
        const bool fullSample=GetTickCount64()>=nextFullSample;if(fullSample)nextFullSample=GetTickCount64()+1000;
        UINT page=0xffffffffu,words[1024]={};bool valid=false;
        UINT matches=0,matchedAddress=0,matchedLoaded=0,matchedReserve=0;
        for(size_t i=0;i<g_ammo.candidates.size();++i)
        {
            auto& c=g_ammo.candidates[i];UINT next=c.address&~4095u;
            if(next!=page){page=next;valid=AmmoRead(page,words,sizeof(words));}
            if(!valid)continue;UINT value=words[(c.address-page)/4];
            UINT layout[5]={};bool reserveChanged=false;
            if(AmmoRead(c.address,layout,sizeof(layout)))
            {
                const UINT reserve=layout[3],capacity=layout[1];
                if(value==layout[0]&&value==previous[i]+1&&previousReserve[i]>0&&previousReserve[i]!=0xffffffffu&&
                    reserve==previousReserve[i]-1&&capacity>=value&&capacity<=32&&layout[4]==value+reserve)
                {++matches;matchedAddress=c.address;matchedLoaded=value;matchedReserve=reserve;}
                reserveChanged=reserve!=previousReserve[i];previousReserve[i]=reserve;
            }
            else previousReserve[i]=0xffffffffu;
            c.last=static_cast<BYTE>((std::min)(value,255u));
            if(fullSample||value!=previous[i]||reserveChanged)AmmoSnapshot(file,c,value!=previous[i]?3:2,value);
            previous[i]=value;
        }
        if(matches)AmmoOwnerObserved(matchedAddress,matchedLoaded,matchedReserve,matches);
        if(g_ammo.bytesWritten>=64*1024*1024-sizeof(AmmoPacket))break;
        Sleep(200);
    }
    Log("[AMMO PROBE] reload observation saved: %u candidates, %u bytes; no ammo writes or generated reload input",
        static_cast<UINT>(g_ammo.candidates.size()),g_ammo.bytesWritten);
}
void AmmoModules(HANDLE file)
{
    const wchar_t* names[]={nullptr,L"FC3_d3d11.dll",L"FC3.dll"};
    for(auto* name:names)
    {
        HMODULE module=GetModuleHandleW(name);if(!module)continue;
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>65536)continue;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE)continue;
        AmmoPacket p;p.type=4;p.address=static_cast<UINT>(reinterpret_cast<uintptr_t>(module));
        p.initial=nt->OptionalHeader.SizeOfImage;p.value=nt->FileHeader.TimeDateStamp;
        const wchar_t* label=name?name:L"main.exe";memcpy(p.nearby,label,(wcslen(label)+1)*sizeof(wchar_t));
        AmmoWritePacket(file,p);
    }
}
DWORD WINAPI AmmoWorker(LPVOID)
{
    if(g_ammo.job==0)
    {
        _snwprintf_s(g_ammo.tracePath,MAX_PATH,_TRUNCATE,L"%sfc3_ammo_probe_%lu_%llu.bin",AmmoFolder(),GetCurrentProcessId(),GetTickCount64());
        Log("[AMMO PROBE] each attempt retained separately: %ls",g_ammo.tracePath);
    }
    HANDLE file=CreateFileW(g_ammo.tracePath,GENERIC_WRITE,FILE_SHARE_READ,nullptr,g_ammo.job==0?CREATE_NEW:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file!=INVALID_HANDLE_VALUE)
    {
        if(g_ammo.job!=0)SetFilePointer(file,0,nullptr,FILE_END);
        if(g_ammo.job==0){AmmoModules(file);AmmoSeed();}
        else if(g_ammo.job==1)
        {
            AmmoFilter(true);Sleep(150);AmmoFilter(false);
            Log("[AMMO PROBE] shot %u: %u stable decrement-by-one candidates",g_ammo.round,static_cast<UINT>(g_ammo.candidates.size()));
            if(g_ammo.round==2&&g_ammo.candidates.size()<=4096)
                for(const auto& c:g_ammo.candidates)AmmoSnapshot(file,c,1);
        }
        else AmmoWatch(file);
        AmmoPacket summary;summary.type=0;summary.initial=static_cast<UINT>(g_ammo.candidates.size());summary.value=g_ammo.capped?1:0;AmmoWritePacket(file,summary);
        CloseHandle(file);
        wchar_t latest[MAX_PATH]={};_snwprintf_s(latest,MAX_PATH,_TRUNCATE,L"%sfc3_ammo_probe.bin",AmmoFolder());
        if(!CopyFileW(g_ammo.tracePath,latest,FALSE))Log("[AMMO PROBE] latest-copy failed; unique attempt remains at %ls",g_ammo.tracePath);
    }
    else {g_ammo.candidates.clear();Log("[AMMO PROBE] output file open failed; diagnostic stopped");}
    InterlockedExchange(&g_ammo.worker,2);return 0;
}
bool AmmoStart(UINT job)
{
    g_ammo.job=job;InterlockedExchange(&g_ammo.worker,1);
    HANDLE thread=CreateThread(nullptr,0,AmmoWorker,nullptr,0,nullptr);
    if(!thread){InterlockedExchange(&g_ammo.worker,0);g_ammo.phase=AmmoPhase::Done;Log("[AMMO PROBE] worker creation failed");return false;}
    SetThreadPriority(thread,THREAD_PRIORITY_BELOW_NORMAL);CloseHandle(thread);return true;
}
void AmmoRestart(const char* why)
{
    InterlockedExchange(&g_ammo.cancel,1);g_ammo.rearm=true;
    Log("[AMMO PROBE] %s; restarting baseline after controls settle",why);
}
void AmmoWaiting(AmmoWait reason, bool gameplay, bool active)
{
    g_ammo.wait=reason;
    static int previous=-1;static ULONGLONG reported=0;
    const auto now=GetTickCount64();
    if(previous!=static_cast<int>(reason)||now-reported>=5000)
    {
        Log("[AMMO PROBE] setup wait=%d gameplay=%d tracking=%d visibleDraws=%ld weaponDraws=%ld quietMs=%llu",
            static_cast<int>(reason),gameplay,active,g_visibleWeaponDrawsLastFrame,g_weaponDrawsLastFrame,
            g_ammo.quiet?now-g_ammo.quiet:0);
        previous=static_cast<int>(reason);reported=now;
    }
}
void TickAmmoProbe(bool gameplay)
{
    InterlockedExchange(&g_ammo.frame,g_frame);
    const auto c=fc3pad::Latest();const ULONGLONG now=GetTickCount64();
    const bool shot=g_lastShotFrame>=0&&g_lastShotFrame!=g_ammo.previousShot;
    const bool reload=c.x&&!g_ammo.previousX;g_ammo.previousX=c.x;g_ammo.previousShot=g_lastShotFrame;
    if(g_ammo.phase==AmmoPhase::Done)return;
    const LONG worker=InterlockedCompareExchange(&g_ammo.worker,0,0);
    if(g_ammo.rearm)
    {
        if(worker==1)return; // never resize or free candidates while a worker owns them
        g_ammo=AmmoProbe{};g_ammo.previousX=c.x;g_ammo.previousShot=g_lastShotFrame;g_ammo.quiet=now;
        InterlockedExchange(&g_ammo.frame,g_frame);
        Log("[AMMO PROBE] fresh baseline pending; initial reload is allowed");return;
    }
    // Once normal reload starts, the read-only worker owns the observation.
    // A headset/controller/menu transition must not discard its saved samples.
    if(g_ammo.phase==AmmoPhase::Watching)
    {
        if(worker==2){InterlockedExchange(&g_ammo.worker,0);g_ammo.phase=AmmoPhase::Done;}
        return;
    }
    if(!gameplay||!c.active)
    {
        if(g_ammo.phase==AmmoPhase::Quiet){g_ammo.quiet=0;AmmoWaiting(!gameplay?AmmoWait::Gameplay:AmmoWait::Tracking,gameplay,c.active);return;}
        AmmoRestart("gameplay or controller tracking interrupted");return;
    }
    if(g_ammo.phase==AmmoPhase::Quiet)
    {
        // Draw classification is a rendering heuristic, not proof of ammo availability.
        // A missing weapon draw on one frame must not silently reset setup forever.
        if(c.x||c.rightTrigger>.3f||c.rightStickClick)
        {
            g_ammo.quiet=now;
            AmmoWaiting(c.x?AmmoWait::Reload:(c.rightTrigger>.3f?AmmoWait::Trigger:AmmoWait::Melee),gameplay,c.active);return;
        }
        if(!g_ammo.quiet)g_ammo.quiet=now;
        AmmoWaiting(AmmoWait::Settling,gameplay,c.active);
        if(now-g_ammo.quiet<kAmmoQuietMs)return;
        HMODULE owner=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&g_ammo),&owner);
        if(owner){auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(owner);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(owner)+dos->e_lfanew);g_ammo.ownerStart=reinterpret_cast<uintptr_t>(owner);g_ammo.ownerEnd=g_ammo.ownerStart+nt->OptionalHeader.SizeOfImage;}
        g_ammo.begun=now;g_ammo.phase=AmmoPhase::Seed;AmmoStart(0);return;
    }
    if(worker==2)
    {
        InterlockedExchange(&g_ammo.worker,0);
        if(g_ammo.candidates.empty()){g_ammo.phase=AmmoPhase::Done;Log("[AMMO PROBE] no candidates survived; no ammo address established");return;}
        if(g_ammo.phase==AmmoPhase::Seed){g_ammo.phase=AmmoPhase::FirstShot;fc3xr::Buzz(1);Log("[AMMO PROBE] READY: fire one shot; wait three seconds before the next");}
        else if(g_ammo.phase==AmmoPhase::FirstFilter)g_ammo.phase=AmmoPhase::SecondShot;
        else if(g_ammo.phase==AmmoPhase::SecondFilter)
        {
            if(g_ammo.candidates.size()>4096){g_ammo.phase=AmmoPhase::Done;Log("[AMMO PROBE] too many candidates for bounded reload observation; refine before writing ammo");}
            else {g_ammo.phase=AmmoPhase::Reload;Log("[AMMO PROBE] ready for normal X reload; nearby fields will be watched for twelve seconds");}
        }
        else if(g_ammo.phase==AmmoPhase::Watching)g_ammo.phase=AmmoPhase::Done;
    }
    if(reload&&(g_ammo.phase==AmmoPhase::FirstSettle||g_ammo.phase==AmmoPhase::SecondSettle))
    {g_ammo.pendingReload=true;Log("[AMMO PROBE] reload queued while shot settles");}
    if(reload&&(g_ammo.phase==AmmoPhase::FirstFilter||g_ammo.phase==AmmoPhase::SecondFilter))
    {g_ammo.pendingReload=true;Log("[AMMO PROBE] reload queued until current filter finishes");return;}
    if((reload||g_ammo.pendingReload)&&(g_ammo.phase==AmmoPhase::SecondShot||g_ammo.phase==AmmoPhase::Reload)&&
        g_ammo.round>=1&&g_ammo.candidates.size()<=4096)
    {
        g_ammo.pendingReload=false;g_ammo.phase=AmmoPhase::Watching;AmmoStart(2);
        Log("[AMMO PROBE] reload observation begins after %u shot sample(s); subsequent shots/reloads retained",g_ammo.round);return;
    }
    if(reload&&!g_ammo.pendingReload&&g_ammo.phase!=AmmoPhase::Reload&&g_ammo.phase!=AmmoPhase::Watching)
    {AmmoRestart("setup reload before two shot samples");return;}
    if(shot)
    {
        if(g_ammo.phase==AmmoPhase::FirstShot||g_ammo.phase==AmmoPhase::SecondShot)
        {g_ammo.phase=g_ammo.phase==AmmoPhase::FirstShot?AmmoPhase::FirstSettle:AmmoPhase::SecondSettle;g_ammo.settle=now;}
        else if(g_ammo.phase!=AmmoPhase::Watching)
        {AmmoRestart("shot overlapped scan");return;}
    }
    if((g_ammo.phase==AmmoPhase::FirstSettle||g_ammo.phase==AmmoPhase::SecondSettle)&&now-g_ammo.settle>=500)
    {++g_ammo.round;g_ammo.phase=g_ammo.round==1?AmmoPhase::FirstFilter:AmmoPhase::SecondFilter;AmmoStart(1);}
    if(reload&&g_ammo.phase==AmmoPhase::Reload){g_ammo.phase=AmmoPhase::Watching;AmmoStart(2);}
}
