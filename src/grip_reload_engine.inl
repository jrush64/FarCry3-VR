// One inserted shell starts and completes one engine-owned reload transaction.
// The script's remaining-shell property is the loop count, not an ammo counter.
UINT ReloadCountProperty(UINT state)
{
    const UINT actor=ReloadRead(state,0),table=ReloadRead(actor,0),getter=ReloadRead(table,0x7c);
    if(!actor||!getter)return 0;
#ifndef FC3_AMMO_TEST
    const UINT base=reinterpret_cast<UINT>(GetModuleHandleW(L"FC3_d3d11.dll"));
    if(!base||getter<base||getter>=base+48332800)return 0;
#endif
    UINT properties=0;
    __try { properties=reinterpret_cast<UINT(__thiscall*)(void*)>(getter)(reinterpret_cast<void*>(actor)); }
    __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
    // Same hash table searched by native 10006680: EDI=properties+8.
    const UINT buckets=ReloadRead(properties,0x10),entries=ReloadRead(properties,0x14);
    if(!entries||!buckets||buckets>1048576)return 0;
    constexpr UINT key=0x3f3029ad;
    UINT node=ReloadRead(entries,((key^0xdeadbeef)%buckets)*4);
    for(int steps=0;node&&steps<64;++steps)
    {
        if(ReloadRead(node,4)==key)
        {
            const UINT property=ReloadRead(node,8);int remaining=0;
            MEMORY_BASIC_INFORMATION region={};
            if(property&&AmmoRead(property+8,&remaining,4)&&remaining>=-1&&remaining<=7&&
               VirtualQuery(reinterpret_cast<void*>(property+4),&region,sizeof(region))&&AmmoReadable(region)&&
               property+12<=reinterpret_cast<UINT>(region.BaseAddress)+region.RegionSize)return property;
            return 0;
        }
        const UINT next=ReloadRead(node,0);if(next==node)return 0;node=next;
    }
    return 0;
}
bool SetReloadRemaining(UINT property,int count)
{
    // Native start and advance write this exact property value/dirty pair.
    if(!property||count<0||count>1)return false;
    __try { *reinterpret_cast<int*>(property+8)=count;*reinterpret_cast<BYTE*>(property+4)=1; }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    return true;
}
UINT GripReloadReserve()
{
    for(const auto& widget:g_reload.widgets)
        if(static_cast<UINT>(widget.wrapper)==g_reload.wrapper)return ReloadRead(static_cast<UINT>(widget.hud),0x140);
    return 0;
}
// Physical approach requests one ordinary native shell animation. The game owns
// playback, transfer timing, reserve consumption, HUD and finish notifications.
bool NativeReloadActorLocal(UINT state)
{
    if(!g_nativeReloadAuthority||!g_reloadAuthorityManager)return false;
    const UINT actor=ReloadRead(state,0),reference=ReloadRead(actor,4),manager=ReloadRead(g_reloadAuthorityManager,0);
    UINT identity[2]={};
    if(!actor||!reference||!manager||!AmmoRead(reference,identity,sizeof(identity)))return false;
    __try { return g_nativeReloadAuthority(manager,identity[0],identity[1])!=0; }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool PrepareGripNativeBegin(UINT state,UINT wrapper)
{
    if(!g_gripReload.pending)
    {
        if(state==g_gripReload.state&&wrapper==g_gripReload.wrapper)
        {g_gripReload.finishing=0;g_gripReload.failed=0;g_gripReload.completed=0;}
        return false;
    }
    const UINT weapon=ReloadRead(wrapper,0x48);
    if(GetTickCount64()>g_gripReload.deadline||!g_motionGun||!fc3pad::Latest().active||
       g_frame-g_reload.shotgunFrame>2||!ReloadContext(state,wrapper,weapon)||
       (g_reload.ready&&(wrapper!=g_reload.wrapper||state!=g_reload.state))||!NativeReloadActorLocal(state))return false;
    if(!InterlockedExchange(&g_gripReload.pending,0))return false;
    fc3pad::CancelReloadRequest();
    g_reload.state=state;g_reload.wrapper=wrapper;g_reload.weapon=weapon;g_reload.thread=GetCurrentThreadId();g_reload.ready=1;
    g_reload.held=false;g_reload.mode=0;
    g_gripReload.state=state;g_gripReload.wrapper=wrapper;g_gripReload.weapon=weapon;
    g_gripReload.before=ReloadRead(weapon,0xcc);g_gripReload.credited=0;g_gripReload.completed=0;
    g_gripReload.finishing=1;g_gripReload.deadline=GetTickCount64()+6000;
    Log("[GRIP RELOAD] native animation began on game thread %lu; loaded %u; physical shell handed to stock animation",GetCurrentThreadId(),g_gripReload.before);
    return true;
}
void LimitGripNativeAnimation(UINT state,UINT wrapper,UINT caller)
{
    if(!g_gripReload.finishing||state!=g_gripReload.state||wrapper!=g_gripReload.wrapper)return;
    const UINT property=ReloadCountProperty(state),available=property?ReloadRead(property,8):0;
    ProbeNativeAnimation(state,wrapper,1,static_cast<int>(available),caller,13);
    if(!property||available<1||available>7||!SetReloadRemaining(property,1))
    {
        g_gripReload.failed=1;
        Log("[GRIP RELOAD] single-shell count limit unavailable: property %08X available %u; ammo allowance zero",property,available);
    }
    else Log("[GRIP RELOAD] native remaining-shell count %u -> 1; waiting for real insertion event",available);
    ProbeNativeAnimation(state,wrapper,1,property?static_cast<int>(ReloadRead(property,8)):0,caller,14);
}
bool GripNativeCycle(UINT state,UINT wrapper)
{
    return g_gripReload.finishing&&state==g_gripReload.state&&wrapper==g_gripReload.wrapper&&
        ReloadContext(state,wrapper,g_gripReload.weapon);
}
void ObserveGripNativeTransfer(UINT before,UINT after)
{
    if(after==before+1&&!g_gripReload.credited)
    {
        g_gripReload.credited=1;InterlockedIncrement(&g_reload.commits);fc3xr::Buzz(1);
        Log("[GRIP RELOAD] native animation inserted exactly one shell %u -> %u; awaiting normal animation finish",before,after);
    }
}
void ObserveGripNativeEnd(UINT state,UINT wrapper)
{
    if(!GripNativeCycle(state,wrapper))return;
    const UINT after=ReloadRead(g_gripReload.weapon,0xcc);
    g_gripReload.finishing=0;g_gripReload.completed=1;fc3pad::CancelReloadRequest();
    if(!g_gripReload.credited||after!=g_gripReload.before+1)g_gripReload.failed=1;
    Log("[GRIP RELOAD] stock animation finished: loaded %u -> %u, credited %ld, failed %ld",g_gripReload.before,after,g_gripReload.credited,g_gripReload.failed);
}
bool GripReloadContext()
{
    return g_gripReloadEnabled&&g_reload.installed&&g_nativeReload&&g_nativeReloadBegin&&g_nativeReloadEnd&&
        g_nativeReloadAuthority&&g_motionGun&&fc3pad::Latest().active&&g_frame-g_reload.shotgunFrame<=2&&
        (!g_reload.ready||ReloadContext(g_reload.state,g_reload.wrapper,g_reload.weapon));
}
