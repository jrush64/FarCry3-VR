// Read-only routing coverage. MRT1 complements, and never changes, MNP1.
// Counts every boundary before filtering; detailed records use the held window.
// Native threads copy an immutable render snapshot and never write files.
#pragma pack(push,4)
struct MacheteRoutingPose
{
    ULONGLONG ms=0;LONG frame=0;UINT epoch=0;float scale=0;
    float controllerBody[16]={},controllerCamera[16]={},hand[16]={};
    float cameraPosition[3]={},cameraBasis[9]={};
};
struct MacheteRoutingRecord
{
    UINT magic=0x4d525431,version=1,bytes=sizeof(MacheteRoutingRecord),kind=0;
    ULONGLONG ms=0;LONG frame=0;UINT thread=0,sequence=0,flags=0;
    UINT engine=0,site=0,strategy=0,strategyVtable=0,receiver=0,eventVtable=0,gate=0;
    UINT registers[9]={},event[8]={};
    UINT raw[10]={},matched[5]={};
    UINT lost=0,installError=0,installed=0,ioError=0,poseFlags=0;
    MacheteRoutingPose pose;
};
#pragma pack(pop)
static_assert(sizeof(MacheteRoutingRecord)==476,"MRT1 routing layout");
struct MacheteRoutingProbe
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool installed=false,full=false;UINT bytes=0,sequence=0;volatile LONG engine=0;
    DWORD installError=0,error=0;ULONGLONG retry=0,lastStatus=0,fileRetry=0;
    HANDLE file=INVALID_HANDLE_VALUE;wchar_t path[MAX_PATH]={};
    volatile LONG raw[10]={},lost=0,window=0;
} g_macheteRouting;
void* g_macheteRoutingOriginal[5]={};
SRWLOCK g_macheteRoutingPoseLock=SRWLOCK_INIT,g_macheteRoutingQueueLock=SRWLOCK_INIT;
MacheteRoutingPose g_macheteRoutingPose;UINT g_macheteRoutingPoseFlags=0,g_macheteRoutingSequence=0;
MacheteRoutingRecord g_macheteRoutingQueue[128];UINT g_macheteRoutingQueued=0;
constexpr UINT kMacheteRoutingSites[5]={0x124b270,0x124b2d9,0x11a1f58,0x11a1fe5,0x11a2085};
constexpr BYTE kMacheteRoutingSignatures[5][16]={
    {0x55,0x8b,0xec,0x83,0xe4,0xf8,0x56,0x57,0x8b,0xf9,0x8b,0x47,0x48,0x85,0xc0,0x74},
    {0x85,0xc0,0x74,0x06,0x57,0xe8,0x4d,0xfa,0xff,0xff,0x5f,0xb0,0x01,0x5e,0x8b,0xe5},
    {0x8d,0x45,0xe4,0x50,0xff,0xd2,0x8b,0xe5,0x5d,0xc2,0x04,0x00,0xcc,0xcc,0xcc,0xcc},
    {0x8d,0x44,0x24,0x08,0x50,0xff,0xd2,0x83,0xc8,0xff,0xf0,0x0f,0xc1,0x07,0x48,0x85},
    {0x8d,0x44,0x24,0x08,0x50,0xff,0xd2,0x83,0xc8,0xff,0xf0,0x0f,0xc1,0x07,0x48,0x85}
};
void SnapshotMacheteRouting()
{
    MacheteRoutingPose pose;pose.ms=GetTickCount64();pose.frame=g_frame;
    pose.epoch=g_armPoseEpoch;pose.scale=g_worldScale;
    memcpy(pose.controllerBody,g_handBody[0],sizeof(pose.controllerBody));
    memcpy(pose.controllerCamera,g_handRight,sizeof(pose.controllerCamera));
    for(int i=0;i<16;++i)pose.hand[i]=static_cast<float>(g_machete.hand.m[i]);
    memcpy(pose.cameraPosition,g_frameCamera.position,sizeof(pose.cameraPosition));
    memcpy(pose.cameraBasis,g_frameCamera.right,12);memcpy(pose.cameraBasis+3,g_frameCamera.up,12);
    memcpy(pose.cameraBasis+6,g_frameCamera.forward,12);
    const UINT flags=(MacheteHeld()?1u:0u)|(g_macheteMelee.pending?2u:0u)|
        (g_handRightValid?4u:0u)|(g_frameCameraValid?8u:0u)|
        (g_machete.handFrame==g_frame?16u:0u)|(MacheteReady()?32u:0u);
    AcquireSRWLockExclusive(&g_macheteRoutingPoseLock);
    g_macheteRoutingPose=pose;g_macheteRoutingPoseFlags=flags;
    g_macheteRoutingSequence=g_macheteMelee.sequence;
    ReleaseSRWLockExclusive(&g_macheteRoutingPoseLock);
    InterlockedExchange(&g_macheteRouting.window,(flags&3)?1:0);
}
void MacheteRoutingEvidence(MacheteRoutingRecord& r)
{
    auto& p=g_macheteRouting;r.ms=GetTickCount64();r.thread=GetCurrentThreadId();
    r.engine=static_cast<UINT>(InterlockedCompareExchange(&p.engine,0,0));
    if(TryAcquireSRWLockShared(&g_macheteRoutingPoseLock))
    {
        r.pose=g_macheteRoutingPose;r.frame=r.pose.frame;r.poseFlags=g_macheteRoutingPoseFlags;
        r.sequence=g_macheteRoutingSequence;r.flags|=16;
        ReleaseSRWLockShared(&g_macheteRoutingPoseLock);
    }
    for(int i=1;i<10;++i)r.raw[i]=static_cast<UINT>(InterlockedCompareExchange(&p.raw[i],0,0));
    for(int i=1;i<5;++i)r.matched[i]=static_cast<UINT>(InterlockedCompareExchange(&g_macheteNative.phases[i],0,0));
    r.lost=static_cast<UINT>(InterlockedCompareExchange(&p.lost,0,0));
    // Installation and file state are recorded by the Present status record.
}
void QueueMacheteRouting(const MacheteRoutingRecord& r)
{
    if(!TryAcquireSRWLockExclusive(&g_macheteRoutingQueueLock))
    {InterlockedIncrement(&g_macheteRouting.lost);return;}
    if(g_macheteRoutingQueued<128)g_macheteRoutingQueue[g_macheteRoutingQueued++]=r;
    else InterlockedIncrement(&g_macheteRouting.lost);
    ReleaseSRWLockExclusive(&g_macheteRoutingQueueLock);
}
void __cdecl ObserveMacheteRouting(UINT kind,const UINT* saved)
{
    auto& p=g_macheteRouting;if(!p.enabled||kind<1||kind>9||!saved)return;
    InterlockedIncrement(&p.raw[kind]);
    if(!InterlockedCompareExchange(&p.window,0,0))return;
    MacheteRoutingRecord r;r.kind=kind;MacheteRoutingEvidence(r);
    memcpy(r.registers,saved,sizeof(r.registers));const UINT stack=saved[3]+4;
    if(kind<=6)
    {
        r.site=kind<=4?kMacheteNativeSites[kind-1]:kMacheteRoutingSites[kind-5];
        r.strategy=kind==1?MacheteNativeRead(stack,4):kind<=4?MacheteNativeRead(saved[2],8):
            kind==5?saved[6]:saved[0];
        r.strategyVtable=MacheteNativeRead(r.strategy,0);
        if(r.engine&&r.strategyVtable==r.engine+0x1fba818)r.flags|=4;
        // These live functions themselves use +48/+28/+4 reference chains.
        // Keep opaque owner candidates to correlate the gate with event receivers.
        // Payload at kind 4 stays empty: no return-stage reference chasing.
        if(kind!=4)
        {
            r.event[0]=MacheteNativeRead(r.strategy,0x48);
            r.event[1]=MacheteNativeRead(MacheteNativeRead(r.event[0],0x28),0xc);
            r.event[2]=MacheteNativeRead(MacheteNativeRead(r.event[0],4),0xc);
        }
        if(kind==6)r.gate=saved[7];
    }
    else
    {
        r.site=kMacheteRoutingSites[kind-5];r.receiver=saved[6];
        const UINT event=kind==7?saved[2]-0x1c:stack+8;
        const UINT bytes=kind==7?0x1c:0x20;
        if(ReloadTraceRead(event,r.event,bytes))
        {
            r.flags|=32;r.eventVtable=r.event[0];
            if(r.engine&&r.eventVtable==r.engine+(kind==7?0x1fb141c:0x2015214))r.flags|=8;
        }
    }
    QueueMacheteRouting(r);
#ifdef FC3_AMMO_TEST
    __asm { pxor xmm0,xmm0 }
#endif
}
#define MACHETE_ROUTING_BOUNDARY(name,number,slot) \
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
    __asm call ObserveMacheteRouting \
    __asm add esp,8 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [g_macheteRoutingOriginal+slot] \
}
MACHETE_ROUTING_BOUNDARY(MacheteCallbackEntry,5,0)
MACHETE_ROUTING_BOUNDARY(MacheteAuthorityReturn,6,4)
MACHETE_ROUTING_BOUNDARY(MacheteMeleeEvent,7,8)
MACHETE_ROUTING_BOUNDARY(MacheteBeginEvent,8,12)
MACHETE_ROUTING_BOUNDARY(MacheteEndEvent,9,16)
#undef MACHETE_ROUTING_BOUNDARY
bool MacheteRoutingSignaturesMatch(const BYTE* base)
{
    for(int i=0;i<5;++i)if(memcmp(base+kMacheteRoutingSites[i],kMacheteRoutingSignatures[i],16))return false;
    return true;
}
bool InstallMacheteRoutingProbe()
{
    auto& p=g_macheteRouting;if(p.installed)return true;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base){p.installError=ERROR_MOD_NOT_FOUND;return false;}
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800||
        !MacheteRoutingSignaturesMatch(base)){p.installError=ERROR_BAD_EXE_FORMAT;return false;}
    void* hooks[]={reinterpret_cast<void*>(MacheteCallbackEntry),reinterpret_cast<void*>(MacheteAuthorityReturn),
        reinterpret_cast<void*>(MacheteMeleeEvent),reinterpret_cast<void*>(MacheteBeginEvent),reinterpret_cast<void*>(MacheteEndEvent)};
    int created=0,enabled=0;MH_STATUS result=MH_OK;
    for(;created<5;++created)
    {result=MH_CreateHook(base+kMacheteRoutingSites[created],hooks[created],&g_macheteRoutingOriginal[created]);if(result!=MH_OK)break;}
    InterlockedExchange(&p.engine,static_cast<LONG>(reinterpret_cast<UINT>(base)));
    if(created==5)for(;enabled<5;++enabled){result=MH_EnableHook(base+kMacheteRoutingSites[enabled]);if(result!=MH_OK)break;}
    if(created!=5||enabled!=5)
    {
        for(int i=0;i<enabled;++i)MH_DisableHook(base+kMacheteRoutingSites[i]);
        for(int i=0;i<created;++i)MH_RemoveHook(base+kMacheteRoutingSites[i]);
        p.installError=0x10000u+static_cast<UINT>(result);return false;
    }
    p.installed=true;p.installError=0;
    Log("[MACHETE ROUTING] callback/local-authority/melee-event/machete-begin/end probes installed; no target or damage writes");return true;
}
void CloseMacheteRoutingProbe()
{
    auto& p=g_macheteRouting;if(p.file==INVALID_HANDLE_VALUE)return;
    if(!FlushFileBuffers(p.file))p.error=GetLastError();CloseHandle(p.file);p.file=INVALID_HANDLE_VALUE;
}
bool OpenMacheteRoutingProbe(ULONGLONG now)
{
    auto& p=g_macheteRouting;if(p.file!=INVALID_HANDLE_VALUE)return true;
    if(p.full||now<p.fileRetry)return false;p.fileRetry=now+1000;
    wchar_t folder[MAX_PATH]={};
#ifdef FC3_AMMO_TEST
    GetTempPathW(MAX_PATH,folder);
#else
    wcscpy_s(folder,fc3::DataFolder());
#endif
    _snwprintf_s(p.path,MAX_PATH,_TRUNCATE,L"%sfc3_machete_routing_%lu_%llu.bin",folder,GetCurrentProcessId(),now);
    p.file=CreateFileW(p.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(p.file==INVALID_HANDLE_VALUE){p.error=GetLastError();Log("[MACHETE ROUTING] capture open failed %lu",p.error);return false;}
    p.error=0;p.bytes=0;Log("[MACHETE ROUTING] automatic coverage capture %ls",p.path);return true;
}
void FlushMacheteRoutingProbe()
{
    auto& p=g_macheteRouting;std::vector<MacheteRoutingRecord> records;
    AcquireSRWLockExclusive(&g_macheteRoutingQueueLock);
    records.assign(g_macheteRoutingQueue,g_macheteRoutingQueue+g_macheteRoutingQueued);g_macheteRoutingQueued=0;
    ReleaseSRWLockExclusive(&g_macheteRoutingQueueLock);
    if(records.empty())return;
    if(!OpenMacheteRoutingProbe(GetTickCount64())){InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));return;}
    const DWORD bytes=static_cast<DWORD>(records.size()*sizeof(MacheteRoutingRecord));
    if(p.bytes+bytes>8u*1024*1024){p.full=true;InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));CloseMacheteRoutingProbe();return;}
    DWORD wrote=0;const BOOL ok=WriteFile(p.file,records.data(),bytes,&wrote,nullptr);
    if(!ok||wrote!=bytes)
    {p.error=ok?ERROR_WRITE_FAULT:GetLastError();InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));CloseMacheteRoutingProbe();return;}
    p.bytes+=wrote;
    // Flush once per nonempty batch, so a zero-hit status also survives exit.
    if(!FlushFileBuffers(p.file)){p.error=GetLastError();CloseMacheteRoutingProbe();}
}
void TickMacheteRoutingProbe()
{
    auto& p=g_macheteRouting;if(!p.enabled)return;SnapshotMacheteRouting();
    const auto now=GetTickCount64();bool attempted=false;
    if(!p.installed&&now>=p.retry){p.retry=now+5000;InstallMacheteRoutingProbe();attempted=true;}
    const bool request=p.sequence!=g_macheteMelee.sequence;p.sequence=g_macheteMelee.sequence;
    if(attempted||request||((MacheteHeld()||g_macheteMelee.pending)&&now-p.lastStatus>=1000))
    {
        MacheteRoutingRecord r;MacheteRoutingEvidence(r);r.flags|=request?64u:0u;
        r.installed=(g_macheteNative.installed?1u:0u)|(p.installed?2u:0u);
        r.installError=p.installError;r.ioError=p.error;QueueMacheteRouting(r);p.lastStatus=now;
        Log("[MACHETE ROUTING] seq%u coverage%u callback%u gate%u helper%u filtered%u meleeEvent%u begin%u end%u lost%u installError%lu fileError%lu",
            r.sequence,r.installed,r.raw[5],r.raw[6],r.raw[1],r.matched[1],r.raw[7],r.raw[8],r.raw[9],r.lost,p.installError,p.error);
    }
    FlushMacheteRoutingProbe();
}
