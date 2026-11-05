// 12D2250 is the rigid/static/compound event-route classifier, NOT a universal
// damageability predicate. The native false branch at 12D7FB5..12D7FFA calls
// 12D5C50 -> 12D3970 for pawn damage. Candidate119 discarded that entire path.
struct MachetePawnState120 {
    bool pending=false;UINT wrapper=0;V3 point={};
    UINT contacts=0,attempts=0,dispatches=0,blocked=0,nonPawns=0;
}g_machetePawn120;
UINT MachetePawnComponent120(UINT entity)
{
    const UINT descriptor=g_macheteBladeContact.engine+0x2535c0;
    const UINT lookup=g_macheteBladeContact.engine+0x1706640;
    UINT component=0;
    __asm call descriptor
    __asm add eax,14h
    __asm push eax
    __asm mov eax,entity
    __asm call lookup
    __asm mov component,eax
    return component;
}
void CallMachetePawnContact120(UINT fn,UINT strategy,UINT wrapper,UINT config,UINT contact)
{
    // Native EAX = contact; three cdecl arguments. 12D5C50 consumes its one
    // by-value target-wrapper reference and owns event allocation/destruction.
    __asm push config
    __asm push wrapper
    __asm push strategy
    __asm mov eax,contact
    __asm call fn
    __asm add esp,12
}
bool MachetePawnContact120(UINT wrapper,V3 point)
{
    auto& p=g_machetePawn120;auto& b=g_macheteBladeContact;
    const UINT entity=MacheteNativeRead(wrapper,12);
    if(!entity||!MachetePawnComponent120(entity)){++p.nonPawns;return false;}
    ++p.contacts;if(g_macheteCuts121.active&&!g_macheteCuts121.damage)return false;
    if(MacheteBladeOnCooldown(entity,GetTickCount64()))return false;
    if(g_macheteCuts121.active&&!MacheteCutTarget121(entity,true))return false;
    BYTE contact[0x30]={};const float xyz[3]={float(point.x),float(point.y),float(point.z)};
    memcpy(contact+0x14,xyz,12);contact[0x23]=10;
    p.pending=true;p.wrapper=wrapper;p.point=point;const UINT before=p.dispatches;++p.attempts;
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(wrapper+8));
    CallMachetePawnContact120(b.engine+0x12d5c50,b.strategy,wrapper,b.callConfig118,reinterpret_cast<UINT>(contact));
    p.pending=false;p.wrapper=0;
    const bool dispatched=p.dispatches!=before;if(!dispatched)++p.blocked;
    if(p.attempts<=12)Log("[MACHETE PAWN120] body contact entity %08x vtableRVA %08x: native pawn event %s at %.3f/%.3f/%.3f",entity,MacheteNativeRead(entity,0)-b.engine,dispatched?"DISPATCHED":"rejected by native builder",point.x,point.y,point.z);
    return dispatched;
}
bool MachetePawnEvent120(const UINT* saved,UINT stack)
{
    auto& p=g_machetePawn120;auto& b=g_macheteBladeContact;
    if(!p.pending||!saved)return false;
    const UINT wrapper=MacheteNativeRead(saved[2],12),event=saved[1];
    if(wrapper!=p.wrapper||MacheteNativeRead(event,0)!=b.engine+0x1f55bcc)return false;
    // The native builder uses the view's cached impact position. Supply the
    // shape ray's real blade contact at the same two native event fields.
    const float xyz[3]={float(p.point.x),float(p.point.y),float(p.point.z)};
    memcpy(reinterpret_cast<void*>(event+0x18),xyz,12);memcpy(reinterpret_cast<void*>(event+0x5c),xyz,12);
    *reinterpret_cast<BYTE*>(event+0x6a)=1;
    const UINT entity=MacheteNativeRead(wrapper,12);
    MacheteBladeSetCooldown(entity,GetTickCount64());MacheteCutHit121(entity);
    if(g_macheteCollider118.haptics){fc3xr::MacheteHit();++g_macheteCuts121.feedback;}
    ++p.dispatches;++g_macheteActor119.selected;++g_macheteActor119.dispatches;
    g_macheteActor119.dispatchMs=GetTickCount64();++b.events;++b.actorHits;b.lastEvent=g_macheteActor119.dispatchMs;
    (void)stack;return true;
}
bool ValidateMachetePawn120(const BYTE* base)
{
    const BYTE helper[]={0x55,0x8b,0xec,0x81,0xec,0x08,0x01,0x00};
    const BYTE builder[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec};
    return !memcmp(base+0x12d5c50,helper,8)&&!memcmp(base+0x12d3970,builder,8);
}
