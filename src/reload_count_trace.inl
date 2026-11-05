// One local motion-shotgun native cycle loads one shell. Engine owns ammo.
bool g_singleShellEnabled=true;
using SingleReloadAuthority=UINT(__stdcall*)(UINT,UINT,UINT);
SingleReloadAuthority g_singleReloadAuthority=nullptr;
UINT g_singleReloadManager=0;
void* g_nativeReloadCountStore=nullptr;
constexpr BYTE kNativeReloadCountStore[]={0x89,0x48,0x08,0xc6,0x40,0x04,0x01};
struct ReloadCountTrace
{
    UINT state=0,wrapper=0,weapon=0,property=0,thread=0,lastLoaded=0;
    volatile LONG scope=0,writes=0,inserts=0,ends=0;
    LONG messages=0;
    bool single=false,cycle=false;
    UINT before=0,requested=0;
} g_reloadCountTrace;
bool ReloadTraceRead(UINT address,void* destination,SIZE_T size)
{
    MEMORY_BASIC_INFORMATION m={};
    if(!address||!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m))||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
       !(m.Protect&(PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
    const uintptr_t end=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
    if(address>=end||size>end-address)return false;
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),destination,size,&copied)&&copied==size;
}
bool SingleShellNativeContext(UINT state,UINT wrapper)
{
    if(KnownNonPumpModel()||!g_singleShellEnabled||!g_motionGun||!fc3pad::Latest().active||g_frame-g_reload.shotgunFrame>2||
       !ReloadContext(state,wrapper,ReloadRead(wrapper,0x48)))return false;
    if(ReloadLocal(wrapper))return true;
    if(!g_singleReloadAuthority||!g_singleReloadManager)return false;
    const UINT actor=ReloadRead(state,0),reference=ReloadRead(actor,4),manager=ReloadRead(g_singleReloadManager,0);
    UINT id[2]={};if(!reference||!manager||!ReloadTraceRead(reference,id,sizeof(id)))return false;
    __try {return g_singleReloadAuthority(manager,id[0],id[1])!=0;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool SingleShellPoseOwner(UINT state,UINT wrapper)
{return g_reloadCountTrace.single&&g_reloadCountTrace.cycle&&state==g_reloadCountTrace.state&&wrapper==g_reloadCountTrace.wrapper;}
void ArmReloadCountTrace(UINT state,UINT wrapper)
{
    g_reloadCountTrace.scope=0;
    const bool single=SingleShellNativeContext(state,wrapper);
    if(!g_nativeReloadCountStore||(!single&&!ReloadPoseContext(state,wrapper)))return;
    g_reloadCountTrace.state=state;g_reloadCountTrace.wrapper=wrapper;g_reloadCountTrace.weapon=ReloadRead(wrapper,0x48);
    g_reloadCountTrace.property=0;g_reloadCountTrace.thread=GetCurrentThreadId();
    g_reloadCountTrace.lastLoaded=ReloadRead(g_reloadCountTrace.weapon,0xcc);g_reloadCountTrace.messages=0;
    g_reloadCountTrace.single=single;g_reloadCountTrace.cycle=single;g_reloadCountTrace.before=g_reloadCountTrace.lastLoaded;
    if(single){fc3pad::BeginOneShellX();ManualNativeBegin(state,wrapper);}
    InterlockedExchange(&g_reloadCountTrace.scope,1);
}
int __cdecl ObserveReloadCountStore(UINT state,UINT wrapper,UINT property,int remaining)
{
    if(!g_reloadCountTrace.scope||GetCurrentThreadId()!=g_reloadCountTrace.thread||
       state!=g_reloadCountTrace.state||wrapper!=g_reloadCountTrace.wrapper)return remaining;
    UINT original[3]={};
    if(!property||remaining<0||remaining>1024)return remaining;
    if(!ReloadTraceRead(property,original,sizeof(original))&&!g_reloadCountTrace.single)return remaining;
    g_reloadCountTrace.requested=static_cast<UINT>(remaining);
    const int limited=g_reloadCountTrace.single&&remaining>1?1:remaining;
    if(g_reloadCountTrace.single)Log("[ONE SHELL X] native request %d -> %d; loaded %u; actual count object %08X",remaining,limited,g_reloadCountTrace.before,property);
    g_reloadCountTrace.property=property;InterlockedIncrement(&g_reloadCountTrace.writes);
    const UINT actor=ReloadRead(state,0),table=ReloadRead(actor,0);
    UINT getter=0;ReloadTraceRead(table+0x7c,&getter,sizeof(getter));
    MEMORY_BASIC_INFORMATION m={};VirtualQuery(reinterpret_cast<void*>(table+0x7c),&m,sizeof(m));
    Log("[RELOAD COUNT TRACE] actual start store thread %lu state %08X wrapper %08X property %08X remaining %d loaded %u getter %08X tableProtection %08lX formerReader %08X",
        GetCurrentThreadId(),state,wrapper,property,remaining,g_reloadCountTrace.lastLoaded,getter,m.Protect,ReloadRead(table,0x7c));
#ifdef FC3_AMMO_TEST
    // Confirm that the naked bridge restores SSE even when observation uses it.
    __asm { pxor xmm0,xmm0 }
#endif
    return limited;
}
__declspec(naked) void ReloadCountStoreBoundary()
{
    __asm
    {
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        mov ecx,[eax+8] // native EBP, whose [EBP+12] is wrapper
        push dword ptr [eax+24] // native ECX: next remaining count
        push dword ptr [eax+28] // native EAX: actual property object
        push dword ptr [ecx+12]
        push dword ptr [eax+16] // native EBX: reload state
        call ObserveReloadCountStore
        add esp,16
        mov edx,[esp+512]
        cmp eax,[edx+24]
        je countUnchanged
        mov [edx+24],eax // restored ECX for the original property store
        mov [edx+56],eax // original native ESP+14h: count also used by start notification
    countUnchanged:
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        jmp dword ptr [g_nativeReloadCountStore]
    }
}
void TraceReloadCountPhase(UINT state,UINT wrapper,const char* phase,bool afterTransfer=false,bool ended=false)
{
    if(!g_nativeReloadCountStore||state!=g_reloadCountTrace.state||wrapper!=g_reloadCountTrace.wrapper||
       GetCurrentThreadId()!=g_reloadCountTrace.thread||!g_reloadCountTrace.property||
       !ReloadContext(state,wrapper,g_reloadCountTrace.weapon))return;
    UINT data[3]={};if(!ReloadTraceRead(g_reloadCountTrace.property,data,sizeof(data)))return;
    const UINT loaded=ReloadRead(g_reloadCountTrace.weapon,0xcc);
    if(afterTransfer&&loaded==g_reloadCountTrace.lastLoaded+1)InterlockedIncrement(&g_reloadCountTrace.inserts);
    g_reloadCountTrace.lastLoaded=loaded;
    if(ended)InterlockedIncrement(&g_reloadCountTrace.ends);
    if(g_reloadCountTrace.messages++<64)
        Log("[RELOAD COUNT TRACE] %s property %08X remaining %d dirty %u loaded %u pending %u consumed %u",
            phase,g_reloadCountTrace.property,static_cast<int>(data[2]),data[1]&255,loaded,ReloadRead(state,8)&255,(ReloadRead(state,8)>>8)&255);
}

void FinishSingleShellCycle(UINT state,UINT wrapper)
{
    if(!g_reloadCountTrace.cycle||state!=g_reloadCountTrace.state||wrapper!=g_reloadCountTrace.wrapper)return;
    g_reloadCountTrace.cycle=false;fc3pad::EndOneShellX();ManualNativeEnd(state,wrapper);
    const UINT after=ReloadRead(g_reloadCountTrace.weapon,0xcc);
    Log("[ONE SHELL X] native end loaded %u -> %u; request %u; next shell requires a new X press",g_reloadCountTrace.before,after,g_reloadCountTrace.requested);
}
