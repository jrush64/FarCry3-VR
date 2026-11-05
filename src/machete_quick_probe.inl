// Read-only quick-melee contact evidence. QMP1 leaves MRT1/MNP1 unchanged.
// Counts every boundary before filtering; detailed records use the held window.
// Native threads copy an immutable render snapshot and never write files.
#pragma pack(push,4)
struct MacheteQuickPose
{
    ULONGLONG ms=0;LONG frame=0;UINT epoch=0;float scale=0;
    float controllerBody[16]={},controllerCamera[16]={},hand[16]={};
    float cameraPosition[3]={},cameraBasis[9]={};
};
struct MacheteQuickRecord
{
    UINT magic=0x514d5031,version=1,bytes=sizeof(MacheteQuickRecord),kind=0;
    ULONGLONG ms=0;LONG frame=0;UINT thread=0,sequence=0,flags=0;
    UINT engine=0,site=0,strategy=0,strategyVtable=0,receiver=0,eventVtable=0,gate=0;
    UINT registers[9]={},event[8]={};
    UINT raw[10]={},matched[5]={};
    UINT lost=0,installError=0,installed=0,ioError=0,poseFlags=0;
    UINT payload[64]={};
    MacheteQuickPose pose;
};
#pragma pack(pop)
static_assert(sizeof(MacheteQuickRecord)==732,"QMP1 routing layout");
struct MacheteQuickProbe
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool installed=false,full=false;UINT bytes=0,sequence=0;volatile LONG engine=0;
    DWORD installError=0,error=0;ULONGLONG retry=0,lastStatus=0,fileRetry=0;
    HANDLE file=INVALID_HANDLE_VALUE;wchar_t path[MAX_PATH]={};
    volatile LONG raw[10]={},lost=0,window=0,corrected=0;
} g_macheteQuick;
void* g_macheteQuickOriginal[9]={};
SRWLOCK g_macheteQuickPoseLock=SRWLOCK_INIT,g_macheteQuickQueueLock=SRWLOCK_INIT;
MacheteQuickPose g_macheteQuickPose;UINT g_macheteQuickPoseFlags=0,g_macheteQuickSequence=0;
MacheteQuickRecord g_macheteQuickQueue[128];UINT g_macheteQuickQueued=0;
constexpr UINT kMacheteQuickSites[9]={0x11a2ce0,0x11a2e4a,0x12d8570,0x12d6069,0x12d608d,0x12d7b60,0x12d7b95,0x12d5c8f,0x12d7f57};
constexpr BYTE kMacheteQuickSignatures[9][16]={
    {0x55,0x8b,0xec,0x83,0xec,0x28,0x53,0x8b,0x5d,0x08,0x8b,0x43,0x24,0x83,0xb8,0x24},
    {0x8b,0x00,0x8b,0xcc,0x89,0x01,0x8b,0x13,0x8b,0x42,0x78,0x8b,0xcb,0xff,0xd0,0x50},
    {0x55,0x8b,0xec,0xa1,0x28,0x95,0x82,0x12,0x8a,0x40,0x08,0x83,0xec,0x10,0x53,0x8b},
    {0x8d,0x54,0x24,0x48,0x8b,0x4c,0x24,0x20,0x8b,0x7d,0x10,0x52,0x8b,0x91,0x90,0x00},
    {0x8b,0x5b,0x04,0x89,0x5c,0x24,0x18,0x8d,0x43,0x08,0xb9,0x01,0x00,0x00,0x00,0xf0},
    {0x55,0x8b,0xec,0x83,0xe4,0xf0,0x81,0xec,0xe4,0x01,0x00,0x00,0x53,0x56,0x8b,0x75},
    {0x88,0x44,0x24,0x17,0x84,0xc0,0x0f,0x84,0xae,0x04,0x00,0x00,0x8b,0x5e,0x04,0x8b},
    {0x8b,0x55,0x0c,0x8b,0x4a,0x0c,0x8b,0x01,0x8b,0x50,0x28,0x56,0xff,0xd2,0x3b,0x35},
    {0x8b,0x4b,0x0c,0x8b,0x11,0x8b,0x52,0x28,0x8b,0xc7,0x50,0xff,0xd2,0x8d,0x44,0x24}
};
#include "machete_swept_contact.inl"
#include "machete_collider118.inl"
void SnapshotMacheteQuick()
{
    MacheteQuickPose pose;pose.ms=GetTickCount64();pose.frame=g_frame;
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
    const UINT colliderFlags118=flags|((!g_pistolMenu.open&&g_inGameplay&&g_enabled&&MotionInteractionEnabled()&&!fc3pad::GameplayInputBlocked()&&fc3pad::Latest().active)?256u:0u);
    AcquireSRWLockExclusive(&g_macheteQuickPoseLock);
    SnapshotMacheteCollider118(pose,colliderFlags118);
    CaptureMacheteBladePose(pose,flags);
    g_macheteQuickPose=pose;g_macheteQuickPoseFlags=flags;
    g_macheteQuickSequence=g_macheteMelee.sequence;
    ReleaseSRWLockExclusive(&g_macheteQuickPoseLock);
    InterlockedExchange(&g_macheteQuick.window,(flags&3)?1:0);
}
void MacheteQuickEvidence(MacheteQuickRecord& r)
{
    auto& p=g_macheteQuick;r.ms=GetTickCount64();r.thread=GetCurrentThreadId();
    r.engine=static_cast<UINT>(InterlockedCompareExchange(&p.engine,0,0));
    if(TryAcquireSRWLockShared(&g_macheteQuickPoseLock))
    {
        r.pose=g_macheteQuickPose;r.frame=r.pose.frame;r.poseFlags=g_macheteQuickPoseFlags;
        r.sequence=g_macheteQuickSequence;r.flags|=16;
        ReleaseSRWLockShared(&g_macheteQuickPoseLock);
    }
    for(int i=1;i<10;++i)r.raw[i]=static_cast<UINT>(InterlockedCompareExchange(&p.raw[i],0,0));
    // matched[] is reserved; these sites do not filter guessed strategy classes.
    r.lost=static_cast<UINT>(InterlockedCompareExchange(&p.lost,0,0));
    // Installation and file state are recorded by the Present status record.
}
void QueueMacheteQuick(const MacheteQuickRecord& r)
{
    if(!TryAcquireSRWLockExclusive(&g_macheteQuickQueueLock))
    {InterlockedIncrement(&g_macheteQuick.lost);return;}
    if(g_macheteQuickQueued<128)g_macheteQuickQueue[g_macheteQuickQueued++]=r;
    else InterlockedIncrement(&g_macheteQuick.lost);
    ReleaseSRWLockExclusive(&g_macheteQuickQueueLock);
}
// payload layouts are phase-specific and use only addresses the native code owns.
// An opaque pawn pointer is evidence for correlation, not local-player attribution.
void MacheteQuickContacts(MacheteQuickRecord& r,UINT data,UINT count)
{
    r.event[3]=count;const UINT take=(count<3?count:3);
    if(count>65536){r.flags|=128;return;}
    if(!take||ReloadTraceRead(data,r.payload,take*0x30))r.flags|=32;
    else r.flags|=128;
}
void __cdecl ObserveMacheteQuick(UINT kind,const UINT* saved)
{
    auto& p=g_macheteQuick;if(!p.enabled||kind<1||kind>9||!saved)return;
    InterlockedIncrement(&p.raw[kind]);
    if(MacheteBladeObserveQuick(kind,saved,saved[3]+4))return; // the mod's own blade-contact call: handled, not recorded
    if(!InterlockedCompareExchange(&p.window,0,0))return;
    MacheteQuickRecord r;r.kind=kind;MacheteQuickEvidence(r);
    memcpy(r.registers,saved,sizeof(r.registers));const UINT stack=saved[3]+4;
    const UINT frame=saved[2];r.site=kMacheteQuickSites[kind-1];
    if(kind==1||kind==3||kind==6)
    {
        r.strategy=MacheteNativeRead(stack,4);r.strategyVtable=MacheteNativeRead(r.strategy,0);
        if(kind==6)
        {
            r.event[0]=MacheteNativeRead(stack,8);
            r.event[4]=MacheteNativeRead(r.event[0],0x10c);r.event[5]=MacheteNativeRead(r.event[0],0x110);
            r.event[6]=MacheteNativeRead(r.event[0],0x11c);
        }
    }
    else if(kind==2)
    {
        r.strategy=saved[4];r.strategyVtable=MacheteNativeRead(r.strategy,0);
        r.event[0]=saved[7];r.gate=MacheteNativeRead(saved[7],0); // selected signal CRC, before stock send
    }
    else if(kind==4)
    {
        r.strategy=saved[4];r.strategyVtable=MacheteNativeRead(r.strategy,0);
        r.event[0]=MacheteNativeRead(frame,12);r.event[1]=MacheteNativeRead(frame,16);
        // Original native pose, endpoint and full attachment matrix; never substituted.
        const bool ok=ReloadTraceRead(stack+0x5c,r.payload,12)&&
            ReloadTraceRead(stack+0x48,r.payload+3,12)&&ReloadTraceRead(stack+0x110,r.payload+6,64)&&
            ReloadTraceRead(stack+0x3c,r.payload+22,12);
        r.event[4]=MacheteNativeRead(r.event[0],0x10c);r.event[5]=MacheteNativeRead(r.event[0],0x110);
        r.flags|=ok?32u:128u;
    }
    else if(kind==5)
    {
        r.strategy=saved[4];r.strategyVtable=MacheteNativeRead(r.strategy,0);
        r.event[0]=saved[0];r.event[1]=MacheteNativeRead(saved[0],0);r.event[2]=MacheteNativeRead(saved[0],4);
        MacheteQuickContacts(r,r.event[1],r.event[2]);
    }
    else if(kind==7)
    {
        r.strategy=MacheteNativeRead(frame,8);r.strategyVtable=MacheteNativeRead(r.strategy,0);
        r.event[0]=MacheteNativeRead(frame,12);r.event[1]=MacheteNativeRead(stack,0x24);
        r.event[2]=MacheteNativeRead(stack,0x28);r.gate=saved[7]&0xff;
        MacheteQuickContacts(r,r.event[1],r.event[2]);
    }
    else // Native event dispatches, before the original virtual call.
    {
        r.strategy=MacheteNativeRead(frame,8);r.strategyVtable=MacheteNativeRead(r.strategy,0);
        const UINT target=kind==8?MacheteNativeRead(frame,12):saved[4];
        r.receiver=MacheteNativeRead(target,12);r.event[0]=target;
        r.event[1]=kind==8?saved[1]:saved[0];r.eventVtable=MacheteNativeRead(r.event[1],0);
        const UINT contact=kind==8?saved[0]:MacheteNativeRead(stack,0x18);r.event[2]=contact;
        const bool ok=ReloadTraceRead(r.event[1],r.payload,208)&&ReloadTraceRead(contact,r.payload+52,48);
        r.flags|=ok?32u:128u;
    }
    if(kind==1)BindMacheteBladeRequest(r);
    if(kind==4)CorrectMacheteSweptQuery(r,stack);
    QueueMacheteQuick(r);
#ifdef FC3_AMMO_TEST
    __asm { pxor xmm0,xmm0 }
#endif
}
#define MACHETE_QUICK_BOUNDARY(name,number,slot) \
__declspec(naked) void name() { \
    __asm pushfd \
    __asm pushad \
    __asm cld \
    __asm mov eax,esp \
    __asm sub esp,544 \
    __asm and esp,0fffffff0h \
    __asm mov [esp+512],eax \
    __asm fxsave [esp] \
    __asm push eax \
    __asm push number \
    __asm call ObserveMacheteQuick \
    __asm add esp,8 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [g_macheteQuickOriginal+slot] \
}
MACHETE_QUICK_BOUNDARY(QuickRequest35,1,0)
MACHETE_QUICK_BOUNDARY(QuickSelectedSignal,2,4)
MACHETE_QUICK_BOUNDARY(QuickNativeContact,3,8)
MACHETE_QUICK_BOUNDARY(QuickQueryPose,4,12)
MACHETE_QUICK_BOUNDARY(QuickQueryBroadResult,5,16)
MACHETE_QUICK_BOUNDARY(QuickContactEntry,6,20)
MACHETE_QUICK_BOUNDARY(QuickQueryGate,7,24)
MACHETE_QUICK_BOUNDARY(QuickObjectDispatch,8,28)
MACHETE_QUICK_BOUNDARY(QuickActorDispatch,9,32)
#undef MACHETE_QUICK_BOUNDARY
void MacheteQuickExpectedSignature(const BYTE* base,int index,BYTE (&expected)[16])
{
    memcpy(expected,kMacheteQuickSignatures[index],16);
    // PE HIGHLOW relocation at RVA 12D8574: A1 [engine+2829528].
    // Compare the precise loaded address, not a wildcard or the preferred base.
    if(index==2)
    {
        const UINT address=reinterpret_cast<UINT>(base)+0x2829528u;
        memcpy(expected+4,&address,4);
    }
}
UINT MacheteQuickSignatureMismatch(const BYTE* base)
{
    for(int i=0;i<9;++i)
    {
        BYTE expected[16];MacheteQuickExpectedSignature(base,i,expected);
        if(memcmp(base+kMacheteQuickSites[i],expected,16))return static_cast<UINT>(i+1);
    }
    return 0;
}
bool MacheteQuickSignaturesMatch(const BYTE* base){return !MacheteQuickSignatureMismatch(base);}
bool InstallMacheteQuickProbe()
{
    auto& p=g_macheteQuick;if(p.installed)return true;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base){p.installError=ERROR_MOD_NOT_FOUND;return false;}
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)
    {p.installError=ERROR_BAD_EXE_FORMAT;Log("[MACHETE QUICK] engine PE identity mismatch");return false;}
    const UINT mismatch=MacheteQuickSignatureMismatch(base);
    if(mismatch)
    {
        p.installError=ERROR_BAD_EXE_FORMAT;
        Log("[MACHETE QUICK] signature mismatch site%u RVA%08x loadedBase%08x",mismatch,
            kMacheteQuickSites[mismatch-1],reinterpret_cast<UINT>(base));return false;
    }
    void* hooks[]={reinterpret_cast<void*>(QuickRequest35),reinterpret_cast<void*>(QuickSelectedSignal),reinterpret_cast<void*>(QuickNativeContact),reinterpret_cast<void*>(QuickQueryPose),reinterpret_cast<void*>(QuickQueryBroadResult),reinterpret_cast<void*>(QuickContactEntry),reinterpret_cast<void*>(QuickQueryGate),reinterpret_cast<void*>(QuickObjectDispatch),reinterpret_cast<void*>(QuickActorDispatch)};
    int created=0,enabled=0;MH_STATUS result=MH_OK;
    for(;created<9;++created)
    {result=MH_CreateHook(base+kMacheteQuickSites[created],hooks[created],&g_macheteQuickOriginal[created]);if(result!=MH_OK)break;}
    InterlockedExchange(&p.engine,static_cast<LONG>(reinterpret_cast<UINT>(base)));
    if(created==9)for(;enabled<9;++enabled){result=MH_EnableHook(base+kMacheteQuickSites[enabled]);if(result!=MH_OK)break;}
    if(created!=9||enabled!=9)
    {
        for(int i=0;i<enabled;++i)MH_DisableHook(base+kMacheteQuickSites[i]);
        for(int i=0;i<created;++i)MH_RemoveHook(base+kMacheteQuickSites[i]);
        p.installError=0x10000u+static_cast<UINT>(result);return false;
    }
    p.installed=true;p.installError=0;
    Log("[MACHETE QUICK] request35/selected-signal/native-contact/query-pose/query-result/contact/gate/object/actor probes installed; original queries and damage unchanged");return true;
}
void CloseMacheteQuickProbe()
{
    auto& p=g_macheteQuick;if(p.file==INVALID_HANDLE_VALUE)return;
    if(!FlushFileBuffers(p.file))p.error=GetLastError();CloseHandle(p.file);p.file=INVALID_HANDLE_VALUE;
}
bool OpenMacheteQuickProbe(ULONGLONG now)
{
    auto& p=g_macheteQuick;if(p.file!=INVALID_HANDLE_VALUE)return true;
    if(p.full||now<p.fileRetry)return false;p.fileRetry=now+1000;
    wchar_t folder[MAX_PATH]={};
#ifdef FC3_AMMO_TEST
    GetTempPathW(MAX_PATH,folder);
#else
    wcscpy_s(folder,fc3::DataFolder());
#endif
    _snwprintf_s(p.path,MAX_PATH,_TRUNCATE,L"%sfc3_machete_quick_%lu_%llu.bin",folder,GetCurrentProcessId(),now);
    p.file=CreateFileW(p.path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(p.file==INVALID_HANDLE_VALUE){p.error=GetLastError();Log("[MACHETE QUICK] capture open failed %lu",p.error);return false;}
    p.error=0;p.bytes=0;Log("[MACHETE QUICK] automatic coverage capture %ls",p.path);return true;
}
void FlushMacheteQuickProbe()
{
    auto& p=g_macheteQuick;std::vector<MacheteQuickRecord> records;
    AcquireSRWLockExclusive(&g_macheteQuickQueueLock);
    records.assign(g_macheteQuickQueue,g_macheteQuickQueue+g_macheteQuickQueued);g_macheteQuickQueued=0;
    ReleaseSRWLockExclusive(&g_macheteQuickQueueLock);
    if(records.empty())return;
    if(!OpenMacheteQuickProbe(GetTickCount64())){InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));return;}
    const DWORD bytes=static_cast<DWORD>(records.size()*sizeof(MacheteQuickRecord));
    if(p.bytes+bytes>64u*1024*1024){p.full=true;InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));CloseMacheteQuickProbe();return;}
    DWORD wrote=0;const BOOL ok=WriteFile(p.file,records.data(),bytes,&wrote,nullptr);
    if(!ok||wrote!=bytes)
    {p.error=ok?ERROR_WRITE_FAULT:GetLastError();InterlockedExchangeAdd(&p.lost,static_cast<LONG>(records.size()));CloseMacheteQuickProbe();return;}
    p.bytes+=wrote;
    // Flush once per nonempty batch, so a zero-hit status also survives exit.
    if(!FlushFileBuffers(p.file)){p.error=GetLastError();CloseMacheteQuickProbe();}
}
void TickMacheteQuickProbe()
{
    auto& p=g_macheteQuick;if(!p.enabled)return;SnapshotMacheteQuick();
    static LONG lastCorrected=-1;const LONG corrected=InterlockedCompareExchange(&p.corrected,0,0);
    if(corrected!=lastCorrected){Log("[MACHETE CONTACT] native capsule queries corrected %ld; health loss still unverified",corrected);lastCorrected=corrected;}
    const auto now=GetTickCount64();bool attempted=false;
    if(!p.installed&&now>=p.retry){p.retry=now+5000;InstallMacheteQuickProbe();attempted=true;}
    const bool request=p.sequence!=g_macheteMelee.sequence;p.sequence=g_macheteMelee.sequence;
    if(attempted||request||((MacheteHeld()||g_macheteMelee.pending)&&now-p.lastStatus>=1000))
    {
        MacheteQuickRecord r;MacheteQuickEvidence(r);r.flags|=request?64u:0u;
        r.installed=p.installed?1u:0u;
        r.installError=p.installError;r.ioError=p.error;QueueMacheteQuick(r);p.lastStatus=now;
        Log("[MACHETE QUICK] seq%u installed%u request%u signal%u native%u queryPose%u broad%u contact%u gate%u object%u actor%u lost%u installError%lu fileError%lu",
            r.sequence,r.installed,r.raw[1],r.raw[2],r.raw[3],r.raw[4],r.raw[5],r.raw[6],r.raw[7],r.raw[8],r.raw[9],r.lost,p.installError,p.error);
    }
    FlushMacheteQuickProbe();
}
