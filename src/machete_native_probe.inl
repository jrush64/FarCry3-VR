// Read-only local-player melee impact timeline. Never changes targeting or health.
// Native callback 124B270 verifies the local actor before calling 124AD30.
#pragma pack(push,4)
struct MacheteNativeRecord
{
    UINT magic=0x4D4E5031,version=1,bytes=sizeof(MacheteNativeRecord),phase=0;
    ULONGLONG ms=0;LONG frame=0;UINT thread=0,sequence=0,flags=0;
    UINT strategy=0,wrapper=0,actor=0,nativeEntity=0,target=0,hitCount=0;
    float entityOrient[3]={},entityPosition[3]={};
    UINT registers[9]={},scratch[48]={},event[32]={},hit[12]={};
};
#pragma pack(pop)
static_assert(sizeof(MacheteNativeRecord)==492,"native melee probe layout");
struct MacheteNativeProbe
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool installed=false,full=false;UINT engine=0,bytes=0;
    HANDLE file=INVALID_HANDLE_VALUE;wchar_t path[MAX_PATH]={};DWORD error=0;
    ULONGLONG retry=0;volatile LONG phases[5]={},lost=0;
} g_macheteNative;
void* g_macheteNativeOriginal[4]={};
SRWLOCK g_macheteNativeLock=SRWLOCK_INIT;
MacheteNativeRecord g_macheteNativeQueue[64];UINT g_macheteNativeQueued=0;
UINT MacheteNativeRead(UINT address,UINT offset)
{UINT value=0;if(address&&offset<=0xffffffffu-address)ReloadTraceRead(address+offset,&value,4);return value;}
void QueueMacheteNative(const MacheteNativeRecord& r)
{
    AcquireSRWLockExclusive(&g_macheteNativeLock);
    if(g_macheteNativeQueued<64)g_macheteNativeQueue[g_macheteNativeQueued++]=r;
    else InterlockedIncrement(&g_macheteNative.lost);
    ReleaseSRWLockExclusive(&g_macheteNativeLock);
}
void __cdecl ObserveMacheteNative(UINT phase,const UINT* saved)
{
    auto& p=g_macheteNative;if(!p.enabled||phase<1||phase>4||!saved)return;
    ObserveMacheteRouting(phase,saved); // count entry before the class filter
    const UINT stack=saved[3]+4; // pushad saved ESP follows the pushed EFLAGS
    const UINT strategy=phase==1?MacheteNativeRead(stack,4):MacheteNativeRead(saved[2],8);
    if(!p.engine||MacheteNativeRead(strategy,0)!=p.engine+0x1fba818)return;
    MacheteNativeRecord r;r.phase=phase;r.ms=GetTickCount64();r.frame=g_frame;
    r.thread=GetCurrentThreadId();r.sequence=g_macheteMelee.sequence;
    r.flags=(MacheteHeld()?1u:0u)|(g_macheteMelee.pending?2u:0u);
    r.strategy=strategy;r.wrapper=MacheteNativeRead(strategy,0x48);
    r.actor=MacheteNativeRead(MacheteNativeRead(r.wrapper,0x28),0xc);
    r.nativeEntity=MacheteNativeRead(MacheteNativeRead(r.wrapper,4),0xc);
    if(r.nativeEntity)
    {
        if(ReloadTraceRead(r.nativeEntity+0x30,r.entityOrient,12))r.flags|=4;
        if(ReloadTraceRead(r.nativeEntity+0x50,r.entityPosition,12))r.flags|=8;
    }
    memcpy(r.registers,saved,sizeof(r.registers));
    // Only these two live points have the documented scratch stack allocation.
    // End may contain released references: never follow scratch pointers there.
    if(phase==2||phase==3)
    {
        if(ReloadTraceRead(stack,r.scratch,sizeof(r.scratch)))r.flags|=16;
        if(phase==2)
        {
            r.hitCount=MacheteNativeRead(stack,0x18);
            const UINT hit=MacheteNativeRead(stack,0x14);
            if(r.hitCount&&r.hitCount<=4096&&hit&&ReloadTraceRead(hit,r.hit,sizeof(r.hit)))r.flags|=32;
        }
        else
        {
            r.target=saved[6]; // ECX is target entity immediately before its event dispatch
            if(ReloadTraceRead(stack+0xf0,r.event,sizeof(r.event)))r.flags|=64;
        }
    }
    InterlockedIncrement(&p.phases[phase]);QueueMacheteNative(r);
#ifdef FC3_AMMO_TEST
    // ABI fixture proves the bridge restores these deliberate probe clobbers.
    __asm { pxor xmm0,xmm0 }
#endif
}
// All four boundaries preserve the complete integer, EFLAGS and x87/SSE state.
// The original trampoline executes every displaced instruction unchanged.
#define MACHETE_NATIVE_BOUNDARY(name,number,slot) \
__declspec(naked) void name() { \
    __asm pushfd \
    __asm pushad \
    __asm mov eax,esp \
    __asm sub esp,544 \
    __asm and esp,0fffffff0h \
    __asm mov [esp+512],eax \
    __asm fxsave [esp] \
    __asm push eax \
    __asm push number \
    __asm call ObserveMacheteNative \
    __asm add esp,8 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [g_macheteNativeOriginal+slot] \
}
MACHETE_NATIVE_BOUNDARY(MacheteImpactEntry,1,0)
MACHETE_NATIVE_BOUNDARY(MacheteImpactQuery,2,4)
MACHETE_NATIVE_BOUNDARY(MacheteImpactDispatch,3,8)
MACHETE_NATIVE_BOUNDARY(MacheteImpactEnd,4,12)
#undef MACHETE_NATIVE_BOUNDARY
constexpr UINT kMacheteNativeSites[4]={0x124ad30,0x124b03b,0x124b178,0x124b25c};
constexpr BYTE kMacheteNativeSignatures[4][16]={
    {0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0xd4,0x01,0x00,0x00,0x53,0x8b,0x5d,0x08},
    {0x83,0x7c,0x24,0x18,0x00,0x0f,0x86,0xf1,0x01,0x00,0x00,0x8b,0x5c,0x24,0x14,0x8b},
    {0x8d,0x84,0x24,0xf0,0x00,0x00,0x00,0x50,0xff,0xd2,0x0f,0x57,0xc0,0x8b,0x76,0x04},
    {0x5f,0x5e,0x5b,0x8b,0xe5,0x5d,0xc2,0x04,0x00,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc}
};
bool MacheteNativeSignaturesMatch(const BYTE* base)
{
    for(int i=0;i<4;++i)if(memcmp(base+kMacheteNativeSites[i],kMacheteNativeSignatures[i],16))return false;
    return true;
}
bool InstallMacheteNativeProbe()
{
    auto& p=g_macheteNative;if(p.installed)return true;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return false;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800||
       !MacheteNativeSignaturesMatch(base))return false;
    void* hooks[]={reinterpret_cast<void*>(MacheteImpactEntry),reinterpret_cast<void*>(MacheteImpactQuery),
                   reinterpret_cast<void*>(MacheteImpactDispatch),reinterpret_cast<void*>(MacheteImpactEnd)};
    int created=0,enabled=0;
    for(;created<4;++created)if(MH_CreateHook(base+kMacheteNativeSites[created],hooks[created],&g_macheteNativeOriginal[created])!=MH_OK)break;
    // Set the immutable image identity before another thread can reach a hook.
    p.engine=reinterpret_cast<UINT>(base);
    if(created==4)for(;enabled<4;++enabled)if(MH_EnableHook(base+kMacheteNativeSites[enabled])!=MH_OK)break;
    if(created!=4||enabled!=4)
    {
        for(int i=0;i<enabled;++i)MH_DisableHook(base+kMacheteNativeSites[i]);
        for(int i=0;i<created;++i)MH_RemoveHook(base+kMacheteNativeSites[i]);
        p.engine=0;return false;
    }
    p.installed=true;Log("[MACHETE NATIVE] four read-only impact/query/dispatch/end probes installed; target and damage rules unchanged");return true;
}
bool OpenMacheteNativeProbe()
{
    auto& p=g_macheteNative;if(p.file!=INVALID_HANDLE_VALUE)return true;if(p.full)return false;
    wchar_t folder[MAX_PATH]={};
#ifdef FC3_AMMO_TEST
    GetTempPathW(MAX_PATH,folder);
#else
    wcscpy_s(folder,fc3::DataFolder());
#endif
    _snwprintf_s(p.path,MAX_PATH,_TRUNCATE,L"%sfc3_machete_native_%lu_%llu.bin",folder,GetCurrentProcessId(),GetTickCount64());
    p.file=CreateFileW(p.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(p.file==INVALID_HANDLE_VALUE){p.error=GetLastError();Log("[MACHETE NATIVE] capture open failed %lu",p.error);return false;}
    p.error=0;Log("[MACHETE NATIVE] automatic capture %ls",p.path);return true;
}
void CloseMacheteNativeProbe()
{
    auto& p=g_macheteNative;if(p.file==INVALID_HANDLE_VALUE)return;
    FlushFileBuffers(p.file);CloseHandle(p.file);p.file=INVALID_HANDLE_VALUE;
}
void FlushMacheteNativeProbe()
{
    auto& p=g_macheteNative;std::vector<MacheteNativeRecord> records;
    AcquireSRWLockExclusive(&g_macheteNativeLock);
    records.assign(g_macheteNativeQueue,g_macheteNativeQueue+g_macheteNativeQueued);g_macheteNativeQueued=0;
    ReleaseSRWLockExclusive(&g_macheteNativeLock);
    for(const auto& r:records)
    {
        float damage=0;if(r.flags&64)memcpy(&damage,r.event+19,4);
        Log("[MACHETE NATIVE] seq%u phase%u thread%u owned%u strategy%08X target%08X queryHits%u dispatchLevel%.3f validity%X",
            r.sequence,r.phase,r.thread,r.flags&1,r.strategy,r.target,r.hitCount,damage,r.flags);
        if(p.full)continue;
        if(!OpenMacheteNativeProbe()){InterlockedIncrement(&p.lost);continue;}
        if(p.bytes+sizeof(r)>8*1024*1024){p.full=true;CloseMacheteNativeProbe();continue;}
        DWORD wrote=0;if(!WriteFile(p.file,&r,sizeof(r),&wrote,nullptr)||wrote!=sizeof(r))
        {p.error=GetLastError();if(!p.error)p.error=ERROR_WRITE_FAULT;CloseMacheteNativeProbe();InterlockedIncrement(&p.lost);continue;}
        p.bytes+=wrote;
    }
    if(!records.empty()&&p.file!=INVALID_HANDLE_VALUE&&!FlushFileBuffers(p.file))p.error=GetLastError();
}
void TickMacheteNativeProbe()
{
    auto& p=g_macheteNative;if(!p.enabled)return;
    const auto now=GetTickCount64();
    if(!p.installed&&now>=p.retry)
    {p.retry=now+1000;InstallMacheteNativeProbe();}
    if(p.installed)FlushMacheteNativeProbe();
}
