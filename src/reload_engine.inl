// Native reload boundary, identified by live HUD writers and engine disassembly.
// The original game routine owns reserve consumption, loaded ammo and notifications.
using NativeReload = void (__stdcall*)(UINT,UINT,UINT,int);
NativeReload g_nativeReload=nullptr;void* g_nativeAmmoWidget=nullptr;
using NativeReloadBegin=void(__stdcall*)(UINT,UINT);
using NativeReloadEnd=void(__stdcall*)(UINT,UINT,UINT);
NativeReloadBegin g_nativeReloadBegin=nullptr;
NativeReloadEnd g_nativeReloadEnd=nullptr;
struct ReloadPoseCycle
{
    volatile LONG active=0;
    UINT state=0,wrapper=0,weapon=0;
    LONG sequence=0;
} g_reloadPose;
struct ReloadWidget { volatile LONG hud=0,wrapper=0; };
struct PhysicalReloadState
{
    ReloadWidget widgets[16];
    unsigned widgetNext324=0;
    volatile LONG lastWrapper=0;ULONGLONG lastWidgetAt=0; // the weapon the HUD ammo widget showed last (the equipped one)
    volatile LONG ready=0,mode=0,credit=0,pending=0,executing=0,commits=0,failed=0,calls=0,requests=0;
    UINT state=0,wrapper=0,weapon=0,thread=0;
    ULONGLONG deadline=0;
    bool held=false,grip=false,installed=false;
    LONG shotgunFrame=-100000;
} g_reload;
// Keep the unfinished physical ammo gate inactive during control isolation.
bool g_physicalReloadEnabled=false;
UINT ReloadRead(UINT base,UINT offset)
{UINT value=0;return base&&AmmoRead(base+offset,&value,sizeof(value))?value:0;}
void PistolNativeBegin(UINT,UINT);
void PistolNativeTransfer(UINT,UINT,UINT,UINT);
void PistolNativeEnd(UINT,UINT);
bool ReloadContext(UINT state,UINT wrapper,UINT weapon)
{
    const UINT actor=ReloadRead(state,0),component=ReloadRead(actor,0x24);
    return actor&&component&&state==component+0x718&&weapon&&ReloadRead(wrapper,0x48)==weapon;
}
bool ReloadLocal(UINT wrapper)
{
    if(KnownNonPumpModel()||g_frame-g_reload.shotgunFrame>90)return false;
    for(auto& widget:g_reload.widgets)
    {
        if(static_cast<UINT>(widget.wrapper)!=wrapper)continue;
        UINT w[5]={};if(AmmoRead(static_cast<UINT>(widget.hud)+0x134,w,sizeof(w))&&AutoAmmoLayout(w))return true;
    }
    return false;
}
bool SingleShellPoseOwner(UINT,UINT);
bool ReloadPoseContext(UINT state,UINT wrapper)
{
    if(KnownNonPumpModel())return false;
    const bool single=SingleShellPoseOwner(state,wrapper);
    return g_motionGun&&fc3pad::Latest().active&&g_frame-g_reload.shotgunFrame<=(single?90:2)&&
        (single||ReloadLocal(wrapper))&&ReloadContext(state,wrapper,ReloadRead(wrapper,0x48));
}
void ManualNativeBegin(UINT,UINT);
void ManualNativeTransfer(UINT,UINT,UINT,UINT);
void ManualNativeEnd(UINT,UINT);
#include "reload_count_trace.inl"
bool ReloadPoseActive()
{
    if(!InterlockedCompareExchange(&g_reloadPose.active,0,0))return false;
    if(!ReloadPoseContext(g_reloadPose.state,g_reloadPose.wrapper)||
       ReloadRead(g_reloadPose.wrapper,0x48)!=g_reloadPose.weapon)
    {
        if(InterlockedExchange(&g_reloadPose.active,0))Log("[RELOAD POSE] context ended; controller pose restored");
        return false;
    }
    return true;
}
void ObserveReloadPoseBegin(UINT state,UINT wrapper,const char* source)
{
    if(!ReloadPoseContext(state,wrapper))return;
    InterlockedExchange(&g_reloadPose.active,0);
    g_reloadPose.state=state;g_reloadPose.wrapper=wrapper;g_reloadPose.weapon=ReloadRead(wrapper,0x48);
    ++g_reloadPose.sequence;InterlockedExchange(&g_reloadPose.active,1);
    Log("[RELOAD POSE] cycle %ld started from %s; loaded %u; grip does not own animation",g_reloadPose.sequence,source,ReloadRead(g_reloadPose.weapon,0xcc));
}
void __stdcall ReloadBeginBoundary(UINT state,UINT wrapper)
{
    if(!g_nativeReloadBegin)return;
    const UINT caller=reinterpret_cast<UINT>(_ReturnAddress());
    ProbeNativeAnimation(state,wrapper,0,0,caller,9);
    ArmReloadCountTrace(state,wrapper);
    g_nativeReloadBegin(state,wrapper);
    if(ReloadRead(state,8)&1)PistolNativeBegin(state,wrapper);
    g_reloadCountTrace.scope=0;TraceReloadCountPhase(state,wrapper,"begin complete");
    if(ReloadRead(state,8)&1)ObserveReloadPoseBegin(state,wrapper,"native start event");
    ProbeNativeAnimation(state,wrapper,0,0,caller,10);
}
void __stdcall ReloadEndBoundary(UINT state,UINT wrapper,UINT flag)
{
    if(!g_nativeReloadEnd)return;
    const UINT caller=reinterpret_cast<UINT>(_ReturnAddress());
    ProbeNativeAnimation(state,wrapper,flag,0,caller,11);
    g_nativeReloadEnd(state,wrapper,flag);
    if(!(ReloadRead(state,8)&255))PistolNativeEnd(state,wrapper);
    TraceReloadCountPhase(state,wrapper,"native end",false,true);
    FinishSingleShellCycle(state,wrapper);
    if(g_reloadPose.active&&state==g_reloadPose.state&&wrapper==g_reloadPose.wrapper&&!(ReloadRead(state,8)&255))
    {
        InterlockedExchange(&g_reloadPose.active,0);
        Log("[RELOAD POSE] cycle %ld finished by native end event; loaded %u; controller pose restored",g_reloadPose.sequence,ReloadRead(g_reloadPose.weapon,0xcc));
    }
    ProbeNativeAnimation(state,wrapper,flag,0,caller,12);
}
bool ReloadCanGate(UINT state,UINT wrapper)
{
    return g_physicalReloadEnabled&&g_reload.ready&&g_reload.mode&&GetTickCount64()<g_reload.deadline&&
        wrapper==g_reload.wrapper&&state==g_reload.state&&ReloadLocal(wrapper)&&ReloadContext(state,wrapper,g_reload.weapon);
}
void ReloadExecute(UINT state,UINT wrapper)
{
    if(!g_nativeReload||InterlockedCompareExchange(&g_reload.executing,1,0))return;
    const UINT before=ReloadRead(g_reload.weapon,0xcc);
    // flag=1 selects explicit count; count=1 reaches the game's reserve helper.
    ProbeNativeAnimation(state,wrapper,1,1,reinterpret_cast<UINT>(_ReturnAddress()),7);
    g_nativeReload(state,wrapper,1,1);
    ProbeNativeAnimation(state,wrapper,1,1,reinterpret_cast<UINT>(_ReturnAddress()),8);
    const UINT after=ReloadRead(g_reload.weapon,0xcc);
    fc3pad::CancelReloadRequest(); // one insertion must not leave native X held
    if(after==before+1){InterlockedIncrement(&g_reload.commits);Log("[PHYSICAL RELOAD] native shell commit %u -> %u",before,after);fc3xr::Buzz(1);}
    else {InterlockedExchange(&g_reload.failed,1);InterlockedExchange(&g_reload.mode,0);Log("[PHYSICAL RELOAD] native commit returned %u -> %u; restoring ordinary reload",before,after);}
    InterlockedExchange(&g_reload.executing,0);
}
void __stdcall ReloadBoundary(UINT state,UINT wrapper,UINT explicitCount,int count)
{
    if(!g_nativeReload)return;
    const UINT caller=reinterpret_cast<UINT>(_ReturnAddress());
    ProbeNativeAnimation(state,wrapper,explicitCount,count,caller,4);
    if(ReloadCanGate(state,wrapper))
    {
        // Preserve the game's pending reload state while awaiting a hand insert.
        g_reload.thread=GetCurrentThreadId();InterlockedExchange(&g_reload.pending,1);
        if(InterlockedExchange(&g_reload.credit,0))
        {InterlockedExchange(&g_reload.pending,0);ReloadExecute(state,wrapper);}
        ProbeNativeAnimation(state,wrapper,explicitCount,count,caller,6);
        return;
    }
    const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
    TraceReloadCountPhase(state,wrapper,"before insertion event");
    g_nativeReload(state,wrapper,explicitCount,count);
    TraceReloadCountPhase(state,wrapper,"after insertion event",true);
    ProbeNativeAnimation(state,wrapper,explicitCount,count,caller,5);
    const UINT after=ReloadRead(weapon,0xcc);
    ManualNativeTransfer(state,wrapper,before,after);
    PistolNativeTransfer(state,wrapper,before,after);
    if(after==before+1&&!g_reloadPose.active&&ReloadPoseContext(state,wrapper))
        ObserveReloadPoseBegin(state,wrapper,"observed native transfer");
    const LONG calls=InterlockedIncrement(&g_reload.calls);
    if(calls<=12)Log("[PHYSICAL RELOAD] native boundary %ld state %08X wrapper %08X weapon %08X flag %u count %d loaded %u -> %u local=%d context=%d",calls,state,wrapper,weapon,explicitCount,count,before,after,ReloadLocal(wrapper),ReloadContext(state,wrapper,weapon));
    // Learn the local context only from a real one-shell engine commit.
    if(!g_reload.ready&&after==before+1&&after<=7&&ReloadLocal(wrapper)&&ReloadContext(state,wrapper,weapon))
    {
        g_reload.state=state;g_reload.wrapper=wrapper;g_reload.weapon=weapon;g_reload.thread=GetCurrentThreadId();
        InterlockedExchange(&g_reload.ready,1);Log("[PHYSICAL RELOAD] real local single-shell context validated; weapon %08X loaded +CC",weapon);
    }
}
void __cdecl ObserveAmmoWidget(UINT wrapper,UINT hud)
{
    if(!wrapper||!hud)return;
    // The old table stopped learning permanently after 16 distinct HUD objects.
    // Update an existing identity first, then use an empty slot or bounded eviction.
    ReloadWidget* target=nullptr;
    for(auto& widget:g_reload.widgets)if(static_cast<UINT>(widget.hud)==hud){target=&widget;break;}
    if(!target)for(auto& widget:g_reload.widgets)if(static_cast<UINT>(widget.wrapper)==wrapper){target=&widget;break;}
    if(!target)for(auto& widget:g_reload.widgets)if(!widget.hud){target=&widget;break;}
    if(!target)target=&g_reload.widgets[g_reload.widgetNext324++%16];
    InterlockedExchange(&target->wrapper,wrapper);InterlockedExchange(&target->hud,hud);
    InterlockedExchange(&g_reload.lastWrapper,static_cast<LONG>(wrapper));g_reload.lastWidgetAt=GetTickCount64();
    if(!g_reload.executing&&g_reload.pending&&g_reload.credit&&GetCurrentThreadId()==g_reload.thread&&ReloadCanGate(g_reload.state,g_reload.wrapper))
    {
        if(InterlockedExchange(&g_reload.credit,0))
        {InterlockedExchange(&g_reload.pending,0);ReloadExecute(g_reload.state,g_reload.wrapper);}
    }
}
__declspec(naked) void ReloadWidgetBoundary()
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
        push esi
        push edi
        call ObserveAmmoWidget
        add esp,8
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        jmp dword ptr [g_nativeAmmoWidget]
    }
}
bool ReloadInstall()
{
    if(g_reload.installed)return true;
    HMODULE module=GetModuleHandleW(L"FC3_d3d11.dll");if(!module)return false;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(module)+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)return false;
    BYTE* base=reinterpret_cast<BYTE*>(module);
    const BYTE commit[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x34,0x53,0x8b,0x5d,0x08};
    const BYTE widget[]={0x55,0x8b,0xec,0x53,0x85,0xff};
    if(memcmp(base+0x12e1790,commit,sizeof(commit))||memcmp(base+0xf83580,widget,sizeof(widget)))return false;
    // Native start: ret8, event kind0, resets count and sets pending word1.
    // Native end: ret12, event kind2, clears pending and publishes completion.
    const BYTE begin[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x44,0x53,0x8b,0x5d,0x08};
    const BYTE end[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x64,0x53,0x8b,0x5d,0x08};
    if(memcmp(base+0x12e19b0,begin,sizeof(begin))||memcmp(base+0x12e14b0,end,sizeof(end)))return false;
    if(memcmp(base+0x12e1aaf,kNativeReloadCountStore,sizeof(kNativeReloadCountStore)))return false;
    const BYTE authority[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x0c,0x8b,0x45,0x08,0x8b,0x40,0x0c};
    if(memcmp(base+0x175cfe0,authority,sizeof(authority)))return false;
    void* sites[]={base+0x12e1790,base+0xf83580,base+0x12e19b0,base+0x12e14b0,base+0x12e1aaf};
    void* hooks[]={reinterpret_cast<void*>(ReloadBoundary),reinterpret_cast<void*>(ReloadWidgetBoundary),reinterpret_cast<void*>(ReloadBeginBoundary),reinterpret_cast<void*>(ReloadEndBoundary),reinterpret_cast<void*>(ReloadCountStoreBoundary)};
    void** originals[]={reinterpret_cast<void**>(&g_nativeReload),&g_nativeAmmoWidget,reinterpret_cast<void**>(&g_nativeReloadBegin),reinterpret_cast<void**>(&g_nativeReloadEnd),&g_nativeReloadCountStore};
    int created=0,enabled=0;
    for(;created<5;++created)if(MH_CreateHook(sites[created],hooks[created],originals[created])!=MH_OK)break;
    if(created==5)for(;enabled<5;++enabled)if(MH_EnableHook(sites[enabled])!=MH_OK)break;
    if(created!=5||enabled!=5)
    {
        for(int i=0;i<enabled;++i)MH_DisableHook(sites[i]);
        for(int i=0;i<created;++i)MH_RemoveHook(sites[i]);
        return false;
    }
    g_singleReloadAuthority=reinterpret_cast<SingleReloadAuthority>(base+0x175cfe0);g_singleReloadManager=reinterpret_cast<UINT>(base+0x2829474);
    g_reload.installed=true;Log("[PHYSICAL RELOAD] native start/transfer/end/widget and actual script-count store observation installed; physical ammo gate disabled");return true;
}
bool ReloadShellDraw(ID3D11DeviceContext* context,UINT start,UINT count)
{
    if(!((start==18&&count==108)||(start==126&&count==360)))return false;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);
    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);SafeRelease(vs);
    return (hash==0xA38F7CD233BCE12Bull||hash==0xCB43EE096F50DBDBull||hash==0x4E79B18F7ABF8152ull)&&d.ByteWidth==257896&&stride==40&&offset==336;
}
void MarkReloadShotgun(ID3D11DeviceContext* context,UINT start,UINT count)
{
    if(!((start==486&&count==4071)||(start==4557&&count==5076)||(start==9633&&count==6795)||(start==16428&&count==7311)))return;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_BUFFER_DESC d={};if(vb)vb->GetDesc(&d);SafeRelease(vb);
    ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);SafeRelease(vs);
    if((hash==0xA38F7CD233BCE12Bull||hash==0xCB43EE096F50DBDBull||hash==0x4E79B18F7ABF8152ull)&&d.ByteWidth==257896&&stride==40&&offset==336)g_reload.shotgunFrame=g_frame;
}
