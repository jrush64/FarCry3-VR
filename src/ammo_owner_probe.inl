// Focused x86 write tracing. Never writes ammo or invokes engine reload calls.
#include <tlhelp32.h>
struct AmmoOwnerTarget { UINT address=0,loaded=0,reserve=0; };
struct AmmoOwnerThread { HANDLE handle=nullptr; DWORD id=0; UINT slots[2]={}; DWORD oldAddress[2]={}; DWORD oldBits[2]={}; volatile LONG armed=0; };
struct AmmoOwnerEvent
{
    UINT magic=0x41574f31,version=1,thread=0,frame=0,ms=0,address=0,loaded=0,reserve=0,eip=0;
    UINT registers[8]={},stack[32]={}; BYTE code[64]={}; UINT codeBase=0;
    volatile LONG ready=0;
};
static_assert(sizeof(AmmoOwnerEvent)==268,"AWO1 writer event format");
struct AmmoOwnerState
{
    AmmoOwnerTarget targets[4]={}; UINT count=0;
    AmmoOwnerThread threads[128]={}; volatile LONG threadCount=0;
    AmmoOwnerEvent events[256]={}; volatile LONG used=0,saved=0,worker=0,frame=0;
    volatile LONG loaded=0,reserve=0; ULONGLONG quiet=0,begun=0;
    bool done=false; PVOID handler=nullptr;
    UINT observedAddress=0,observedCommits=0;
} g_ammoOwner;
bool AmmoOwnerMatch(const UINT* w)
{
    // Structural signature from the successful live capture, not heap addresses.
    return w[0]<=7&&w[1]==7&&w[2]==0x3e99999a&&w[3]<=100000&&
        w[4]==w[0]+w[3]&&w[5]==0&&w[7]==2&&w[8]==1&&w[9]==5&&
        w[10]==0x40000000&&w[11]==0x40000000;
}
bool AmmoOwnerCopy(UINT address,void* out,SIZE_T bytes)
{
    MEMORY_BASIC_INFORMATION m={};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const uintptr_t end=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
    if(address>=end||bytes>end-address)return false;
    SIZE_T copied=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),out,bytes,&copied)&&copied==bytes;
}
DWORD& AmmoOwnerDr(CONTEXT& c,UINT slot)
{ switch(slot){case 0:return c.Dr0;case 1:return c.Dr1;case 2:return c.Dr2;default:return c.Dr3;} }
LONG CALLBACK AmmoOwnerException(EXCEPTION_POINTERS* exception)
{
    if(exception->ExceptionRecord->ExceptionCode!=EXCEPTION_SINGLE_STEP)return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT& c=*exception->ContextRecord;const DWORD id=GetCurrentThreadId();
    AmmoOwnerThread* watched=nullptr;
    const LONG threadCount=InterlockedCompareExchange(&g_ammoOwner.threadCount,0,0);
    for(LONG i=0;i<threadCount;++i)if(g_ammoOwner.threads[i].id==id&&InterlockedCompareExchange(&g_ammoOwner.threads[i].armed,0,0)){watched=&g_ammoOwner.threads[i];break;}
    if(!watched)return EXCEPTION_CONTINUE_SEARCH;
    DWORD ours=0;
    for(UINT i=0;i<2;++i)if((c.Dr6&(1u<<watched->slots[i]))&&AmmoOwnerDr(c,watched->slots[i])==g_ammoOwner.targets[0].address+i*12)ours|=1u<<watched->slots[i];
    if(!ours)return EXCEPTION_CONTINUE_SEARCH;
    UINT loaded=0,reserve=0;const UINT address=g_ammoOwner.targets[0].address;
    const bool valid=AmmoOwnerCopy(address,&loaded,4)&&AmmoOwnerCopy(address+12,&reserve,4);
    const UINT oldLoaded=static_cast<UINT>(InterlockedExchange(&g_ammoOwner.loaded,loaded));
    const UINT oldReserve=static_cast<UINT>(InterlockedExchange(&g_ammoOwner.reserve,reserve));
    if(valid&&(loaded!=oldLoaded||reserve!=oldReserve))
    {
        const LONG index=InterlockedIncrement(&g_ammoOwner.used)-1;
        if(index<256)
        {
            auto& e=g_ammoOwner.events[index];e.thread=id;e.frame=InterlockedCompareExchange(&g_ammoOwner.frame,0,0);e.ms=static_cast<UINT>(GetTickCount64()-g_ammoOwner.begun);
            e.address=address;e.loaded=loaded;e.reserve=reserve;e.eip=c.Eip;
            const UINT regs[8]={c.Eax,c.Ebx,c.Ecx,c.Edx,c.Esi,c.Edi,c.Ebp,c.Esp};memcpy(e.registers,regs,sizeof(regs));
            AmmoOwnerCopy(c.Esp,e.stack,sizeof(e.stack));e.codeBase=c.Eip>32?c.Eip-32:c.Eip;AmmoOwnerCopy(e.codeBase,e.code,sizeof(e.code));
            InterlockedExchange(&e.ready,1);
        }
    }
    c.Dr6&=~ours;
    // Preserve exceptions belonging to someone else's debugger/watchpoints.
    return (c.Dr6&0xe00f)?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_CONTINUE_EXECUTION;
}
bool AmmoOwnerArm(AmmoOwnerThread& t,UINT address)
{
    if(SuspendThread(t.handle)==static_cast<DWORD>(-1))return false;
    CONTEXT c={};c.ContextFlags=CONTEXT_DEBUG_REGISTERS;bool ok=GetThreadContext(t.handle,&c)!=FALSE;UINT freeSlots[4]={},freeCount=0;
    if(ok)for(UINT i=0;i<4;++i)if(!(c.Dr7&(3u<<(i*2))))freeSlots[freeCount++]=i;
    if(ok&&freeCount>=2)
    {
        for(UINT i=0;i<2;++i)
        {
            const UINT slot=freeSlots[i];t.slots[i]=slot;t.oldAddress[i]=AmmoOwnerDr(c,slot);
            const DWORD mask=(3u<<(slot*2))|(15u<<(16+slot*4));t.oldBits[i]=c.Dr7&mask;
            AmmoOwnerDr(c,slot)=address+i*12;c.Dr7=(c.Dr7&~mask)|(1u<<(slot*2))|(13u<<(16+slot*4));
        }
        InterlockedExchange(&t.armed,1);ok=SetThreadContext(t.handle,&c)!=FALSE;
        if(!ok)InterlockedExchange(&t.armed,0);
    }
    else ok=false;
    ResumeThread(t.handle);return ok;
}
bool AmmoOwnerDisarm(AmmoOwnerThread& t,UINT address)
{
    if(!InterlockedCompareExchange(&t.armed,0,0))return true;
    DWORD exited=0;if(GetExitCodeThread(t.handle,&exited)&&exited!=STILL_ACTIVE){InterlockedExchange(&t.armed,0);return true;}
    if(SuspendThread(t.handle)==static_cast<DWORD>(-1))return false;
    CONTEXT c={};c.ContextFlags=CONTEXT_DEBUG_REGISTERS;bool ok=GetThreadContext(t.handle,&c)!=FALSE;
    if(ok)
    {
        for(UINT i=0;i<2;++i)
        {
            const UINT slot=t.slots[i];const DWORD mask=(3u<<(slot*2))|(15u<<(16+slot*4));
            if(AmmoOwnerDr(c,slot)==address+i*12)
            {AmmoOwnerDr(c,slot)=t.oldAddress[i];c.Dr7=(c.Dr7&~mask)|t.oldBits[i];c.Dr6&=~(1u<<slot);}
        }
        ok=SetThreadContext(t.handle,&c)!=FALSE;if(ok)InterlockedExchange(&t.armed,0);
    }
    ResumeThread(t.handle);return ok;
}
void AmmoOwnerDiscover()
{
    UINT words[16384+12]={};MEMORY_BASIC_INFORMATION sm={};VirtualQuery(words,&sm,sizeof(sm));
    const uintptr_t stackBase=reinterpret_cast<uintptr_t>(sm.AllocationBase);
    HMODULE own=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&g_ammoOwner),&own);
    SYSTEM_INFO si={};GetSystemInfo(&si);uintptr_t address=reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress),end=reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
    const auto start=GetTickCount64();
    while(address<end&&GetTickCount64()-start<20000)
    {
        MEMORY_BASIC_INFORMATION m={};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)))break;
        const uintptr_t base=reinterpret_cast<uintptr_t>(m.BaseAddress),last=base+m.RegionSize;if(last<=address)break;
        if(AmmoReadable(m)&&m.Type==MEM_PRIVATE&&m.AllocationBase!=own&&reinterpret_cast<uintptr_t>(m.AllocationBase)!=stackBase)
        for(uintptr_t chunk=base;chunk+48<=last;chunk+=65536)
        {
            const SIZE_T bytes=(std::min)(static_cast<SIZE_T>(sizeof(words)),static_cast<SIZE_T>(last-chunk));
            if(!AmmoRead(static_cast<UINT>(chunk),words,bytes))continue;
            for(SIZE_T at=0;at<65536&&at+48<=bytes;at+=4)
            if(AmmoOwnerMatch(words+at/4))
            {
                const UINT found=static_cast<UINT>(chunk+at);UINT stable[12]={};if(!AmmoRead(found,stable,sizeof(stable))||!AmmoOwnerMatch(stable))continue;
                if(g_ammoOwner.count<4)g_ammoOwner.targets[g_ammoOwner.count]={found,stable[0],stable[3]};
                ++g_ammoOwner.count;if(g_ammoOwner.count>4)return;
            }
        }
        address=last;
    }
    if(address<end){g_ammoOwner.count=0;Log("[AMMO OWNER] scan time cap; refusing partial match");}
}
DWORD WINAPI AmmoOwnerWorker(LPVOID)
{
    Log("[AMMO OWNER] live reload transfer selected %u pair(s)",g_ammoOwner.count);
    // A unique layout match is a hypothesis; capture its writers before any commit work.
    if(g_ammoOwner.count!=1){Log("[AMMO OWNER] unique ammo pair not established; no watchpoints installed");InterlockedExchange(&g_ammoOwner.worker,2);return 0;}
    const UINT address=g_ammoOwner.targets[0].address;
    g_ammoOwner.loaded=g_ammoOwner.targets[0].loaded;g_ammoOwner.reserve=g_ammoOwner.targets[0].reserve;
    g_ammoOwner.begun=GetTickCount64();g_ammoOwner.handler=AddVectoredExceptionHandler(1,AmmoOwnerException);
    if(!g_ammoOwner.handler){InterlockedExchange(&g_ammoOwner.worker,2);return 0;}
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);THREADENTRY32 entry={};entry.dwSize=sizeof(entry);
    if(snapshot!=INVALID_HANDLE_VALUE&&Thread32First(snapshot,&entry))do
    {
        if(entry.th32OwnerProcessID!=GetCurrentProcessId()||entry.th32ThreadID==GetCurrentThreadId()||g_ammoOwner.threadCount==128)continue;
        HANDLE handle=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);if(!handle)continue;
        auto& t=g_ammoOwner.threads[g_ammoOwner.threadCount];t.handle=handle;t.id=entry.th32ThreadID;InterlockedIncrement(&g_ammoOwner.threadCount);
        if(!AmmoOwnerArm(t,address)){CloseHandle(handle);t.handle=nullptr;}
    }while(Thread32Next(snapshot,&entry));if(snapshot!=INVALID_HANDLE_VALUE)CloseHandle(snapshot);
    UINT armed=0;for(UINT i=0;i<g_ammoOwner.threadCount;++i)armed+=g_ammoOwner.threads[i].armed?1:0;
    Log("[AMMO OWNER] watched pair %08X/+12, loaded %u reserve %u, %u existing threads; ready for firing and normal reload",address,g_ammoOwner.targets[0].loaded,g_ammoOwner.targets[0].reserve,armed);
    if(armed)fc3xr::Buzz(1);
    wchar_t path[MAX_PATH]={};_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_ammo_owner_%lu_%llu.bin",AmmoFolder(),GetCurrentProcessId(),GetTickCount64());
    HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,0,nullptr);
    if(file==INVALID_HANDLE_VALUE){Log("[AMMO OWNER] output open failed; removing watchpoints immediately");armed=0;}
    else
    {
        HMODULE module=GetModuleHandleW(L"FC3_d3d11.dll");
        if(module)
        {
            auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);
            AmmoOwnerEvent metadata;metadata.address=reinterpret_cast<UINT>(module);metadata.loaded=nt->OptionalHeader.SizeOfImage;metadata.reserve=nt->FileHeader.TimeDateStamp;
            DWORD n=0;if(!WriteFile(file,&metadata,sizeof(metadata),&n,nullptr)||n!=sizeof(metadata))armed=0;
        }
        Log("[AMMO OWNER] writer trace: %ls",path);
    }
    UINT flushed=0;
    while(armed&&GetTickCount64()-g_ammoOwner.begun<60000&&flushed<256)
    {
        const UINT count=(std::min)(static_cast<UINT>(InterlockedCompareExchange(&g_ammoOwner.used,0,0)),256u);
        while(flushed<count&&InterlockedCompareExchange(&g_ammoOwner.events[flushed].ready,0,0))
        {DWORD n=0;if(!WriteFile(file,&g_ammoOwner.events[flushed],sizeof(AmmoOwnerEvent),&n,nullptr)||n!=sizeof(AmmoOwnerEvent)){armed=0;break;}++flushed;InterlockedExchange(&g_ammoOwner.saved,flushed);}
        Sleep(50);
    }
    bool removed=true;
    for(UINT i=0;i<g_ammoOwner.threadCount;++i)if(g_ammoOwner.threads[i].handle)
    {auto& t=g_ammoOwner.threads[i];if(AmmoOwnerDisarm(t,address)){CloseHandle(t.handle);t.handle=nullptr;}else removed=false;}
    // Keep the handler and thread records alive if OS context restoration failed.
    if(removed){RemoveVectoredExceptionHandler(g_ammoOwner.handler);g_ammoOwner.handler=nullptr;}
    const UINT count=(std::min)(static_cast<UINT>(InterlockedCompareExchange(&g_ammoOwner.used,0,0)),256u);
    while(file!=INVALID_HANDLE_VALUE&&flushed<count&&InterlockedCompareExchange(&g_ammoOwner.events[flushed].ready,0,0))
    {DWORD n=0;if(!WriteFile(file,&g_ammoOwner.events[flushed],sizeof(AmmoOwnerEvent),&n,nullptr)||n!=sizeof(AmmoOwnerEvent))break;++flushed;InterlockedExchange(&g_ammoOwner.saved,flushed);}
    if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);
    Log("[AMMO OWNER] saved %u writer changes; cleanup %s; no ammo writes or generated input",flushed,removed?"complete":"handler retained for unremoved watchpoints");
    InterlockedExchange(&g_ammoOwner.worker,2);return 0;
}
void AmmoOwnerObserved(UINT address,UINT loaded,UINT reserve,UINT matches)
{
    if(InterlockedCompareExchange(&g_ammoOwner.worker,0,0)||g_ammoOwner.done)return;
    if(matches!=1)
    {g_ammoOwner.observedAddress=0;g_ammoOwner.observedCommits=0;Log("[AMMO OWNER] ambiguous live transfer; no writer selected");return;}
    if(g_ammoOwner.observedAddress!=address)
    {g_ammoOwner.observedAddress=address;g_ammoOwner.observedCommits=0;}
    ++g_ammoOwner.observedCommits;
    Log("[AMMO OWNER] correlated insertion %u: %08X loaded %u reserve %u",g_ammoOwner.observedCommits,address,loaded,reserve);
    if(g_ammoOwner.observedCommits<2)return;
    g_ammoOwner.targets[0]={address,loaded,reserve};g_ammoOwner.count=1;
#ifdef FC3_AMMO_TEST
    // Real hardware arm/handler/cleanup are tested separately on a fixture thread.
    InterlockedExchange(&g_ammoOwner.worker,2);
#else
    HMODULE module=GetModuleHandleW(L"FC3_d3d11.dll");if(!module)return;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)return;
    InterlockedExchange(&g_ammoOwner.worker,1);HANDLE thread=CreateThread(nullptr,0,AmmoOwnerWorker,nullptr,0,nullptr);
    if(thread){SetThreadPriority(thread,THREAD_PRIORITY_BELOW_NORMAL);CloseHandle(thread);}else InterlockedExchange(&g_ammoOwner.worker,2);
#endif
}
void TickAmmoOwnerProbe(bool gameplay)
{
    InterlockedExchange(&g_ammoOwner.frame,g_frame);if(g_ammoOwner.done)return;
    const LONG worker=InterlockedCompareExchange(&g_ammoOwner.worker,0,0);
    if(worker==1)return;
    if(worker==2)
    {
        if(g_ammoOwner.count==1||g_ammoOwner.handler){g_ammoOwner.done=true;return;}
        // Menus/loading may precede a live shotgun object. Retry discovery only,
        // with another quiet interval; never reset an armed watchpoint/handler.
        g_ammoOwner.count=0;g_ammoOwner.quiet=0;InterlockedExchange(&g_ammoOwner.worker,0);
        Log("[AMMO OWNER] waiting for live shotgun object; focused discovery will retry");return;
    }
    // Discovery now comes from two actual loaded/reserve insertion correlations,
    // driven by the successful broad probe. No stale signature scan at startup.
}
