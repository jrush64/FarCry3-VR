// Verified engine CExplosive activation, not a timer freeze. 112B1530 takes
// the component on the stack (ret4) and an optional action event in EAX.
// It sets +234 active and initializes +238 from the configured fuse at +B8.
constexpr BYTE kActivateBytes155[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x74,0x53,0x8b,0x5d,0x08};
constexpr BYTE kCleanupBytes155[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x2c,0x53,0x8b,0x5d,0x08};
void* g_activateOriginal155=nullptr;
void* g_cleanupOriginal155=nullptr;
UINT g_stockSetter155=0;
bool g_safetyInstalled155=false;
bool g_pinIntent155=false;
bool g_molotovIntent158=false;
struct SafeGrenade155 {
    Prepared143 target;
    UINT component=0,stockData=0;
    bool deferredStock=false;
    BYTE event[24]={};
    bool eventValid=false,request=false,armed=false,cancelled=false,loose=false,launched=false;
}g_safe155;
unsigned g_blockedActivation155=0,g_armed155=0,g_safeStows155=0;
struct LooseSafe156 {UINT component=0,reference=0;ULONGLONG id=0,owner=0,archetype=0;}g_looseSafe156[64];
void BindPinSound156(const Prepared143&);
bool BlockLooseSafe156(UINT component){
    for(auto& item:g_looseSafe156){
        if(!item.id)continue;
        ULONGLONG id=0;if(!ReadAt(Word(item.reference,12),8,&id,8)||id!=item.id){item={};continue;}
        if(item.component==component&&Word(component,4)==item.reference)return true;
    }return false;
}
void RememberLoose156(){
    const auto& s=g_safe155;if(!s.loose||s.armed||!s.component)return;
    for(auto& item:g_looseSafe156){
        ULONGLONG id=0;const bool valid=item.id&&ReadAt(Word(item.reference,12),8,&id,8)&&id==item.id;
        if(!valid){item={s.component,s.target.projectileRef,s.target.projectile,s.target.owner,s.target.archetype};return;}
    }
}

bool SameItem155(const Prepared143& a,const Prepared143& b){
    return a.projectile&&a.owner==b.owner&&a.projectile==b.projectile&&a.strategy==b.strategy&&
        a.projectileRef==b.projectileRef&&a.nativeWrapper149==b.nativeWrapper149&&a.nativeEntityId149==b.nativeEntityId149;
}
bool SafeIdentity155(const SafeGrenade155& s){
    ULONGLONG owner=0,id=0;UINT entity=0;const auto& t=s.target;
    return t.projectile&&Word(t.strategy)==g_engine+kStrategyVtable&&SameExecution149(t)&&
        Word(t.strategy,0xf4)==t.projectileRef&&ReadAt(Word(t.projectileRef,12),8,&id,8)&&id==t.projectile&&
        PawnIdentity145(Word(t.strategy,0x38),owner,entity)&&owner==t.owner;
}
void CaptureSafe155(const Prepared143& t){
    if(((g_pinIntent155&&t.archetype==0x03c006e5ac354e19ULL)||(g_molotovIntent158&&t.archetype==0x000006f6a4abae8dULL))&&!SameItem155(t,g_safe155.target)){
        g_safe155={};g_safe155.target=t;if(t.archetype==0x03c006e5ac354e19ULL)BindPinSound156(t);
    }
}
bool PreserveSafeStock155(UINT strategy,UINT data){
    AcquireSRWLockExclusive(&g_manualLock143);auto& s=g_safe155;
    const bool hold=!s.armed&&SafeIdentity155(s)&&s.target.strategy==strategy;
    if(hold){s.stockData=data;s.deferredStock=true;}
    ReleaseSRWLockExclusive(&g_manualLock143);return hold;
}
void DebitSafeStock155(UINT data){
    const UINT setter=g_stockSetter155,count=Word(data,0x60);BYTE infinite=0;ReadAt(data,0x64,&infinite,1);
    if(!setter||!count||infinite)return;
    const UINT remaining=count-1;
    __asm mov ecx,data
    __asm mov eax,remaining
    __asm call setter
}
// Block the stock animation's activation before it changes ANY explosive state.
// The event's only consumed fields are its native type getter and owner GUID
// at +10/+14. Retain those values, never a pointer to the animation's stack.
bool __cdecl BlockActivation155(const UINT* saved){
    UINT component=Word(saved[3],8);bool block=false;
    AcquireSRWLockExclusive(&g_manualLock143);
    auto& s=g_safe155;
    if(InterlockedCompareExchange(&g_enabled,0,0)&&BlockLooseSafe156(component)){block=true;++g_blockedActivation155;}
    else if(InterlockedCompareExchange(&g_enabled,0,0)&&!s.armed&&SafeIdentity155(s)&&
       Word(component)==g_engine+0x1fc67c0&&Word(component,4)==s.target.projectileRef){
        block=true;s.component=component;
        if(!s.eventValid&&(!saved[7]||Read(saved[7],s.event,sizeof(s.event))))s.eventValid=true;
        ++g_blockedActivation155;
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
#ifdef FC3_THROW_PROBE_TEST
    __asm { pxor xmm0,xmm0 }
    __asm { fninit }
#endif
    return block;
}
__declspec(naked) void ActivationBoundary155(){
    __asm pushfd
    __asm pushad
    __asm cld
    __asm mov eax,esp
    __asm sub esp,544
    __asm and esp,0fffffff0h
    __asm mov [esp+512],eax
    __asm fxsave [esp]
    __asm push eax
    __asm call BlockActivation155
    __asm add esp,4
    __asm test al,al
    __asm jnz blocked155
    __asm fxrstor [esp]
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_activateOriginal155]
blocked155:
    __asm fxrstor [esp]
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm ret 4
}
void CallActivation155(UINT component,const void* event){
    __asm mov eax,event
    __asm push component
    __asm call dword ptr [g_activateOriginal155]
}
bool ManualReleaseBlocked155(UINT strategy,UINT wrapper){
    AcquireSRWLockShared(&g_manualLock143);
    const bool block=SafeIdentity155(g_safe155)&&g_safe155.target.strategy==strategy&&g_safe155.target.nativeWrapper149==wrapper&&
        !(g_pending143.active&&g_pending143.scheduled&&SameItem155(g_pending143.target,g_safe155.target));
    ReleaseSRWLockShared(&g_manualLock143);return block;
}
void PumpArm155(UINT strategy,UINT wrapper){
    SafeGrenade155 copy;bool arm=false;
    AcquireSRWLockExclusive(&g_manualLock143);auto& s=g_safe155;
    const auto now=GetTickCount64();ULONGLONG local=0;
    if(s.request&&!s.armed&&!s.cancelled&&s.component&&s.eventValid&&g_activateOriginal155&&SafeIdentity155(s)&&
       s.target.strategy==strategy&&s.target.nativeWrapper149==wrapper&&LocalIdentity143(local)&&local==s.target.owner&&
       g_context143.active&&g_context143.owner==local&&g_context143.epoch==s.target.epoch&&now>=g_context143.ms&&now-g_context143.ms<=250){
        copy=s;s.request=false;arm=true;
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
    if(arm){
        // Only physical flame-to-rag contact authorizes this activation.
        // Keep its native idle effect: the exact held-effect hook attaches it
        // to the rag; native impact/fire configuration remains intact.
        CallActivation155(copy.component,Word(reinterpret_cast<UINT>(copy.event))?copy.event:nullptr);
        BYTE active=0;ReadAt(copy.component,0x234,&active,1);
        AcquireSRWLockExclusive(&g_manualLock143);
        if(SameItem155(copy.target,g_safe155.target)&&active){g_safe155.armed=true;g_safe155.deferredStock=false;++g_armed155;}
        ReleaseSRWLockExclusive(&g_manualLock143);
        if(active&&copy.deferredStock&&!InterlockedCompareExchange(&g_unlimited147,0,0))DebitSafeStock155(copy.stockData);
    }
}
// Stock cleanup normally THROWS a cancelled frag (1128DDAF). Its existing
// +6B cancel state instead sends 'deactivate' and retires the SAME entity.
// Select that native branch only for the mod's never-activated, exact held frag.
void __fastcall CleanupGate155(void* self,void*,UINT wrapper){
    const UINT strategy=reinterpret_cast<UINT>(self);bool safe=false;
    AcquireSRWLockExclusive(&g_manualLock143);auto& s=g_safe155;
    if((!s.armed||s.target.archetype==0x000006f6a4abae8dULL)&&!s.launched&&SafeIdentity155(s)&&s.target.strategy==strategy&&s.target.nativeWrapper149==wrapper&&Writable143(strategy+0x6b,1)){
        *reinterpret_cast<BYTE*>(strategy+0x6b)=1;s.cancelled=true;s.request=false;safe=true;++g_safeStows155;
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
    reinterpret_cast<NativeRelease146>(g_cleanupOriginal155)(self,wrapper);
    if(safe){AcquireSRWLockExclusive(&g_manualLock143);if(g_safe155.target.strategy==strategy)g_safe155={};ReleaseSRWLockExclusive(&g_manualLock143);}
}
