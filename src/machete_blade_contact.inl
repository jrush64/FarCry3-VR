// Physical blade damage on the simulation thread. Four blade slices each
// run a scenery cast and a separate native body search. Candidate119 validates
// body contacts against shape rays along the blade instead of the camera cone.
// Native events own damage/reactions; configuration is copied per synchronous
// call. See DAMAGE_SYSTEM_20261015.md and HANDOFF_20261015_COLLIDER119.md.
struct MacheteBladeContactState
{
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool autoAim=false;double minSpeed=1.0;UINT cooldownMs=350;double strikeMaxPitch=.35; // tan of the strike's pitch limit (about 19 degrees)
    // Candidate 109 (the machete was too weak for a real fight): run 09:18
    // registered 11 enemy hits in a whole fight from 5135 sweeps; a 2.5 cm capsule only grazes a body
    // in the exact tick it passes. Thicker swing volume, plus a strike assist: a real swing (tip
    // speed >= strikeSpeed) whose sweep touched nothing lets the engine's own actor search run once,
    // at most every assistMs (the same search that makes the stock R3 swipe connect).
    float bladeRadius=.025f;bool strikeAssist=false;double strikeSpeed=2.5,rearmSpeed=1.2;float damageScale=2.f;
    // Candidate 113: run 10:06 had 139 strikes but 5 enemy hits. Head-to-tip points down into the ground during a
    // swing; the engine's hit test stops at that first world contact and its actor search only runs when nothing
    // was touched. The strike keeps the blade's horizontal direction but is held near level, like the stock
    // swipe along the view. Default damage x2 (it did too little damage); [Machete] BladeDamageScale.
    UINT strikeLog=0,eventsBeforeStrike=0,strikeContacts=0,strikeKept=0;
    // Candidate 111 (every real swipe must work and damage the enemy, with no
    // timeout): run 09:31 had 188 assists but only 2 extra enemy hits, because the actor search ran
    // along the blade's own axis. Now each real swing is ONE strike: when the tip first passes
    // strikeSpeed, the engine's own hit test runs with the stock swipe's reach and width (capsule
    // 0.25 m, the config range), aimed from the head through the blade tip, with the actor search
    // allowed and the per-target memory cleared. It re-arms when the tip slows below rearmSpeed, so
    // consecutive swings each strike; nothing is timed.
    bool strikeArmed=true,strikeCall=false;UINT strikes=0,strikeHits=0,strikeRejects=0,actorHits=0,objectHits=0;
    bool installed=false;UINT engine=0;DWORD installError=0;ULONGLONG retry=0;
    volatile LONG tickThread=0;
    UINT strategy=0,config=0;float damage=0,range=0;bool latchLogged=false;
    volatile LONG inCall=0;DWORD callThread=0;
    bool havePrev=false;V3 prevBase={},prevTip={};ULONGLONG prevMs=0;
    // sweep for the current call
    V3 start={},end={},axis={},base={},tip={};float radius=.07f,half=0;
    UINT calls=0,casts=0,contacts=0,kept=0,dropped=0,events=0,exceptions=0,rejects=0,queryWrites=0,poseWrites=0;
    UINT lastContacts=0,lastKept=0;float lastSpeed=0;ULONGLONG lastCall=0,lastSummary=0,lastEvent=0;
    struct Cooldown{UINT id;ULONGLONG until;} cooldown[16];UINT cooldownCount=0;
    UINT lastReject=0;
    UINT recovered123=0,recoveryReject123=0;
    bool queryApplied118=false;UINT callConfig118=0,actorContacts118=0;
    MacheteColliderPose118 previous118;
} g_macheteBladeContact;
void* g_macheteBladeTickOriginal=nullptr;
void* g_macheteBladePoseOriginal=nullptr;
constexpr UINT kMacheteBladeTickSite=0x13110a0,kMacheteBladePoseSite=0x12d7c4b;
constexpr BYTE kMacheteBladeTickSignature[16]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x2c,0x53,0x56,0x8b,0xf1,0x8b,0x5e,0x48};
constexpr BYTE kMacheteBladePoseSignature[16]={0x03,0x44,0x24,0x24,0x8b,0x74,0x24,0x34,0x8b,0x10,0x89,0x44,0x24,0x18,0x8d,0x44};
bool MacheteBladeContactActive()
{
    const auto& b=g_macheteBladeContact;
    return b.enabled&&b.installed&&b.strategy!=0;
}
bool MacheteBladeInCall()
{
    const auto& b=g_macheteBladeContact;
    return InterlockedCompareExchange(const_cast<volatile LONG*>(&b.inCall),0,0)&&b.callThread==GetCurrentThreadId();
}
// Latched at the engine's own 12D7B60 entry (kind 6): the exact pair it used.
void MacheteBladeLatch(UINT strategy,UINT config)
{
    auto& b=g_macheteBladeContact;if(!b.enabled||MacheteBladeInCall())return;
    if(!strategy||!config||!b.engine)return;
    if(MacheteNativeRead(strategy,0)!=b.engine+0x1f99568)return;
    if(!MacheteNativeRead(strategy,4)||!MacheteNativeRead(strategy,0x24))return;
    UINT bits=0;float damage=0,range=0;
    if(!ReloadTraceRead(config+8,&bits,4))return;memcpy(&damage,&bits,4);
    if(!ReloadTraceRead(config+0x10c,&bits,4))return;memcpy(&range,&bits,4);
    if(!(range>.5f&&range<5.f)||!(damage>0&&damage<10000))return;
    const bool changed=b.strategy!=strategy||b.config!=config;
    b.strategy=strategy;b.config=config;b.damage=damage;b.range=range;
    if(changed||!b.latchLogged){b.latchLogged=true;Log("[MACHETE BLADE] latched controller %08x config %08x (damage %.0f range %.2f) from the native swipe; blade contact %s",strategy,config,damage,range,b.installed?"armed":"waiting for tick hook");}
}
#include "machete_cuts121.inl"
bool MacheteBladeOnCooldown(UINT id,ULONGLONG now)
{
    auto& b=g_macheteBladeContact;const int contact=MacheteCutBlocked121(id,now);if(contact>=0)return contact!=0;
    for(UINT i=0;i<b.cooldownCount;++i)if(b.cooldown[i].id==id)return now<b.cooldown[i].until;
    return false;
}
void MacheteBladeSetCooldown(UINT id,ULONGLONG now)
{
    auto& b=g_macheteBladeContact;
    for(UINT i=0;i<b.cooldownCount;++i)if(b.cooldown[i].id==id){b.cooldown[i].until=now+b.cooldownMs;return;}
    if(b.cooldownCount<16){b.cooldown[b.cooldownCount++]={id,now+b.cooldownMs};return;}
    UINT oldest=0;for(UINT i=1;i<16;++i)if(b.cooldown[i].until<b.cooldown[oldest].until)oldest=i;
    b.cooldown[oldest]={id,now+b.cooldownMs};
}
#include "machete_collider_damage118.inl"
bool MachetePawnContact120(UINT wrapper,V3 point);
UINT MachetePawnComponent120(UINT entity);
#include "machete_actor119.inl"
#include "machete_pawn120.inl"
void MacheteBladeMatrix(const V3& dir,const V3& origin,float (&m)[16])
{
    V3 up=fabs(dir.z)<.9?V3{0,0,1}:V3{0,1,0};const V3 right=Unit(Cross(dir,up));up=Cross(right,dir);
    const float v[16]={float(right.x),float(right.y),float(right.z),0,float(dir.x),float(dir.y),float(dir.z),0,float(up.x),float(up.y),float(up.z),0,float(origin.x),float(origin.y),float(origin.z),1};
    memcpy(m,v,sizeof(v));
}
// 12D6069, inside the mod's own call only: the stock camera capsule becomes the blade
// motion sweep. Same stack locals the swept-contact writer proved live.
bool MacheteBladeWriteQuery(UINT stack)
{
    auto& b=g_macheteBladeContact;
    UINT vtable=0;float nativeRadius=0,nativeHalf=0;
    if(!ReloadTraceRead(stack+0xe8,&vtable,4)||!ReloadTraceRead(stack+0xfc,&nativeRadius,4)||!ReloadTraceRead(stack+0x100,&nativeHalf,4)){++b.rejects;b.lastReject=1;return false;}
    if(vtable!=b.engine+0x1f58ac0||fabs(nativeRadius-.25f)>.001f||fabs(nativeHalf-.05f)>.001f){++b.rejects;b.lastReject=2;return false;}
    const float startF[3]={float(b.start.x),float(b.start.y),float(b.start.z)},endF[3]={float(b.end.x),float(b.end.y),float(b.end.z)},dir[3]={float(b.axis.x),float(b.axis.y),float(b.axis.z)};
    float matrix[16];MacheteBladeMatrix(b.axis,b.start,matrix);
    __try {
        memcpy(reinterpret_cast<void*>(stack+0x5c),startF,12);memcpy(reinterpret_cast<void*>(stack+0x48),endF,12);memcpy(reinterpret_cast<void*>(stack+0x3c),dir,12);
        memcpy(reinterpret_cast<void*>(stack+0xf0),dir,12);memcpy(reinterpret_cast<void*>(stack+0xfc),&b.radius,4);memcpy(reinterpret_cast<void*>(stack+0x100),&b.half,4);
        memcpy(reinterpret_cast<void*>(stack+0x104),startF,12);memcpy(reinterpret_cast<void*>(stack+0x110),matrix,64);
    } __except(EXCEPTION_EXECUTE_HANDLER){++b.rejects;b.lastReject=3;return false;}
    ++b.queryWrites;b.queryApplied118=true;return true;
}
// The strike's direction: the head-to-tip direction in the horizontal plane, its pitch limited to
// atan(maxPitch) either way (world Z up). Zero vector when the tip is straight above or below the head.
V3 MacheteStrikeDirection(V3 toTip,double maxPitch)
{
    const double horizontal=std::hypot(toTip.x,toTip.y);if(!std::isfinite(horizontal)||horizontal<1e-4)return {0,0,0};
    const double limit=horizontal*maxPitch;toTip.z=(std::max)(-limit,(std::min)(limit,toTip.z));
    return Unit(toTip);
}
// One strike per swing: fires on the first sample at or above strikeSpeed, re-arms below rearmSpeed.
bool MacheteStrikeStep(bool& armed,double speed,double strikeSpeed,double rearmSpeed)
{
    if(armed&&speed>=strikeSpeed){armed=false;return true;}
    if(!armed&&speed<rearmSpeed)armed=true;
    return false;
}
// 12D608D, inside the mod's own call only: EDI = contact list (data,count), EBP = the
// hit test's frame. Drop contacts on cooldown; with nothing left and auto-aim
// off, set the flag argument so the camera-aimed actor search cannot fire.
void MacheteBladeAfterCast(const UINT* saved)
{
    auto& b=g_macheteBladeContact;++b.casts;const UINT list=saved[0],frame=saved[2];UINT count=0;
    if(!ReloadTraceRead(list+4,&count,4)||count>128){++b.rejects;return;}
    if(!g_macheteActor119.pass){b.contacts+=count;b.lastContacts=count;if(count)g_macheteActor119.contactMs=GetTickCount64();}
    // Never let an empty blade query inflict camera-aimed actor damage.
    // A failed query substitution is also forbidden from doing native damage.
    const BYTE one=1;const UINT zero=0;
    __try{memcpy(reinterpret_cast<void*>(frame+0x14),&one,1);if(!b.queryApplied118||g_macheteActor119.pass){memcpy(reinterpret_cast<void*>(list+4),&zero,4);count=0;}}
    __except(EXCEPTION_EXECUTE_HANDLER){++b.rejects;return;}
    b.kept+=count;b.lastKept=count;
}

// 12D7C4B, inside the mod's own call only: narrow test and hit volume aimed from the
// blade base toward this contact instead of from the camera.
void __cdecl ObserveMacheteBladePose(const UINT* saved)
{
    if(!saved||!MacheteBladeInCall())return;
    auto& b=g_macheteBladeContact;const UINT stack=saved[3]+4,offset=saved[7];
    UINT data=0,count=0;float position[3]={};
    if(!ReloadTraceRead(stack+0x24,&data,4)||!ReloadTraceRead(stack+0x28,&count,4)||count>128||offset%0x30||offset/0x30>=count||!ReloadTraceRead(data+offset+0x14,position,12)){++b.rejects;b.lastReject=7;return;}
    const V3 contact={position[0],position[1],position[2]};
    V3 dir=Sub(contact,b.base);const double len=Length(dir);
    if(!std::isfinite(len)||len<.01||len>3.0){++b.rejects;b.lastReject=8;return;}
    dir=Scale(dir,1/len);float matrix[16];MacheteBladeMatrix(dir,b.base,matrix);
    const float origin[3]={float(b.base.x),float(b.base.y),float(b.base.z)};
    __try {memcpy(reinterpret_cast<void*>(stack+0x78),origin,12);memcpy(reinterpret_cast<void*>(stack+0xc0),matrix,64);memcpy(reinterpret_cast<void*>(stack+0x94),matrix+4,12);}
    __except(EXCEPTION_EXECUTE_HANDLER){++b.rejects;b.lastReject=9;return;}
    ++b.poseWrites;
}
// Called from ObserveMacheteQuick for every boundary. Returns true when the
// boundary belongs to the mod's own call (the probe then records nothing for it).
bool MacheteBladeObserveQuick(UINT kind,const UINT* saved,UINT stack)
{
    auto& b=g_macheteBladeContact;if(!b.enabled)return false;
    if(kind==6&&!MacheteBladeInCall()){MacheteBladeLatch(MacheteNativeRead(stack,4),MacheteNativeRead(stack,8));return false;}
    if(!MacheteBladeInCall())return false;
    if(kind==8&&MachetePawnEvent120(saved,stack))return true;
    if(kind==4)MacheteBladeWriteQuery(stack);
    else if(kind==5)MacheteBladeAfterCast(saved);
    else if(kind==8||kind==9){++b.events;b.lastEvent=GetTickCount64();if(kind==9){++b.actorHits;if(b.strikeCall)++b.strikeHits;}else ++b.objectHits;}
    return true;
}
void CallMacheteBladeContact(UINT fn,UINT strategy,UINT config)
{
    __asm { push ebx }
    __asm { push esi }
    __asm { push edi }
    __asm { push config }
    __asm { push strategy }
    __asm { call fn }
    __asm { add esp,8 }
    __asm { pop edi }
    __asm { pop esi }
    __asm { pop ebx }
}
bool MacheteBladeSweep(const V3& prevBase,const V3& prevTip,const V3& base,const V3& tip,ULONGLONG dtMs,double minSpeed,
    V3& start,V3& end,V3& axis,float& half,double& speed)
{
    speed=0;if(dtMs<4||dtMs>120)return false;
    const V3 d=Sub(tip,base);const double len=Length(d);if(!std::isfinite(len)||len<.05||len>2)return false;
    start=Scale(Add(prevBase,prevTip),.5);end=Scale(Add(base,tip),.5);
    const double travel=Length(Sub(end,start)),tipTravel=Length(Sub(tip,prevTip));
    if(!std::isfinite(travel)||!std::isfinite(tipTravel)||travel>.6||tipTravel>1)return false;
    speed=tipTravel*1000.0/double(dtMs);if(speed<minSpeed)return false;
    axis=Scale(d,1/len);half=float(len*.5);return true;
}
void MacheteBladeTick(ULONGLONG now)
{
    auto& b=g_macheteBladeContact;if(!b.enabled||!b.strategy)return;
    if(!MacheteHeld()||fc3aim::NativeActionOwnsArms128()||fc3pad::GameplayInputBlocked()||!g_inGameplay){b.havePrev=false;MacheteCutPause121();return;}
    MacheteColliderPose118 pose;
    if(!TryAcquireSRWLockShared(&g_macheteQuickPoseLock))return;
    pose=g_macheteColliderPose118;ReleaseSRWLockShared(&g_macheteQuickPoseLock);
    if((pose.flags&317)!=317||!pose.ms||now<pose.ms||now-pose.ms>100){b.havePrev=false;MacheteCutPause121();return;}
    if(!b.havePrev){MacheteCutPause121();b.previous118=pose;b.havePrev=true;return;}if(pose.ms==b.previous118.ms)return;
    const auto previous=b.previous118;b.previous118=pose;double speed=0;
    auto validation=pose;validation.minSpeed=0;
    if(!MacheteColliderMotion118(previous,validation,speed)){MacheteCutPause121();return;}
    const bool damaging=speed>=pose.minSpeed&&speed>.02;
    if(!damaging&&!MacheteCutNeedsProbe121()){MacheteCutPause121();return;}
    if(MacheteNativeRead(b.strategy,0)!=b.engine+0x1f99568||!MacheteNativeRead(b.strategy,0x24)){b.strategy=0;return;}
    // Copy config for this synchronous call. Never write the shared game
    // damage/range properties, and never request a native attack animation.
    __declspec(align(16)) BYTE config[0x200]={};if(!ReloadTraceRead(b.config,config,sizeof(config))){++b.rejects;return;}
    float damage=0;memcpy(&damage,config+8,4);damage*=pose.damageScale;if(!std::isfinite(damage)||damage<=0||damage>50000)return;memcpy(config+8,&damage,4);
    b.cooldownMs=pose.cooldownMs;b.lastSpeed=float(speed);b.strikeCall=false;b.callConfig118=reinterpret_cast<UINT>(config);
    MacheteCutBegin121(now,damaging);b.callThread=GetCurrentThreadId();InterlockedExchange(&b.inCall,1);
    for(int slice=0;slice<4&&!b.exceptions;++slice){
        MacheteColliderSlice118(previous,pose,slice,b.start,b.end,b.axis,b.half);b.radius=pose.radius;b.base=pose.base;b.tip=pose.tip;
        for(int pass=damaging?0:1;pass<2&&!b.exceptions;++pass){
            g_macheteActor119.pass=pass==1;g_macheteActor119.query=false;
            b.queryApplied118=false;++b.calls;b.lastCall=now;
            __try{CallMacheteBladeContact(b.engine+0x12d7b60,b.strategy,b.callConfig118);}
            __except(EXCEPTION_EXECUTE_HANDLER){++b.exceptions;}
        }
    }
    MacheteCutEnd121(!b.exceptions);g_machetePawn120.pending=false;g_machetePawn120.wrapper=0;g_macheteActor119.pass=false;g_macheteActor119.query=false;InterlockedExchange(&b.inCall,0);b.callConfig118=0;
    if(b.exceptions){b.enabled=false;Log("[MACHETE COLLIDER118] native contact exception; collision damage disabled for this session");}
}

#include "machete_recovery123.inl"
void __cdecl ObserveMacheteBladeTick(const UINT* saved)
{
    auto& b=g_macheteBladeContact;if(!b.enabled||!saved)return;
    MacheteRefreshController123(saved[6]); // PUSHAD ECX: actual native tick receiver
    InterlockedExchange(&b.tickThread,static_cast<LONG>(GetCurrentThreadId()));
    MacheteBladeTick(GetTickCount64());
}
#define MACHETE_BLADE_BOUNDARY(name,fn,original) \
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
    __asm call fn \
    __asm add esp,4 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [original] \
}
MACHETE_BLADE_BOUNDARY(MacheteBladeTickBoundary,ObserveMacheteBladeTick,g_macheteBladeTickOriginal)
MACHETE_BLADE_BOUNDARY(MacheteBladePoseBoundary,ObserveMacheteBladePose,g_macheteBladePoseOriginal)
#undef MACHETE_BLADE_BOUNDARY
bool InstallMacheteBladeContact()
{
    auto& b=g_macheteBladeContact;if(b.installed)return true;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base){b.installError=ERROR_MOD_NOT_FOUND;return false;}
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800){b.installError=ERROR_BAD_EXE_FORMAT;Log("[MACHETE BLADE] engine PE identity mismatch");return false;}
    if(!MacheteRecoverySignatures123(base)||!ValidateMachetePawn120(base)||memcmp(base+kMacheteBladeTickSite,kMacheteBladeTickSignature,16)||memcmp(base+kMacheteActorSite118,kMacheteActorSignature118,16)){b.installError=ERROR_BAD_EXE_FORMAT;Log("[MACHETE BLADE] signature mismatch at tick/pose/pawn sites");return false;}
    if(!g_macheteQuick.installed){b.installError=ERROR_NOT_READY;return false;} // the three boundary sites belong to the quick probe
    b.engine=reinterpret_cast<UINT>(base);
    g_macheteActorEvent118=b.engine+0x12d7e96;g_macheteActorSkip118=b.engine+0x12d7ffd;
    MH_STATUS r=MH_CreateHook(base+kMacheteBladeTickSite,reinterpret_cast<void*>(MacheteBladeTickBoundary),&g_macheteBladeTickOriginal);
    if(r==MH_OK)r=MH_CreateHook(base+kMacheteActorSite118,reinterpret_cast<void*>(MacheteActorBoundary118),&g_macheteActorOriginal118);
    if(r==MH_OK)r=MH_EnableHook(base+kMacheteBladeTickSite);
    if(r==MH_OK)r=MH_EnableHook(base+kMacheteActorSite118);
    if(r!=MH_OK)
    {
        MH_DisableHook(base+kMacheteBladeTickSite);MH_DisableHook(base+kMacheteActorSite118);
        MH_RemoveHook(base+kMacheteBladeTickSite);MH_RemoveHook(base+kMacheteActorSite118);
        b.installError=0x10000u+static_cast<UINT>(r);Log("[MACHETE BLADE] hook install failed %d",static_cast<int>(r));return false;
    }
    if(!InstallMacheteActors119(base)){
        RemoveMacheteActors119(base);MH_DisableHook(base+kMacheteBladeTickSite);MH_DisableHook(base+kMacheteActorSite118);
        MH_RemoveHook(base+kMacheteBladeTickSite);MH_RemoveHook(base+kMacheteActorSite118);
        b.installError=ERROR_BAD_EXE_FORMAT;Log("[MACHETE ACTOR119] actor hook signature or installation failed; collider damage stays disabled");return false;
    }
    b.installed=true;b.installError=0;
    Log("[MACHETE ACTOR119] installed native character broad phase + body shape rays + native character damage dispatch; no camera cone/distance acceptance");
    Log("[MACHETE PAWN120] native pawn contact/event route enabled separately from rigid/static event classification");
    Log("[MACHETE COLLIDER118] installed: physical blade capsule + direct native damage-event path; auto-aim %s, min speed %.2f m/s, cooldown %u ms",b.autoAim?"ON":"off",b.minSpeed,b.cooldownMs);
    return true;
}
void TickMacheteBladeContact()
{
    auto& b=g_macheteBladeContact;if(!b.enabled)return;const auto now=GetTickCount64();
    if(!b.installed&&now>=b.retry){b.retry=now+5000;InstallMacheteBladeContact();}
    if(MacheteHeld()&&now-b.lastSummary>=3000)
    {
        b.lastSummary=now;
        const auto& a=g_macheteActor119;
        const auto& p=g_machetePawn120;
        Log("[MACHETE WORLD126] native surface paths %u rejected %u cooldown suppressed %u feedback %u; breakage remains native",g_macheteWorld126.allowed,g_macheteWorld126.rejected,g_macheteWorld126.suppressed,g_macheteWorld126.feedback);
        Log("[MACHETE RECOVERY123] refreshed %u rejected %u; current native controller, no source swipe requested",b.recovered123,b.recoveryReject123);
        Log("[MACHETE CUTS121] pawnHits %u rearms %u feedbackRequests %u; exit/reentry active, 70ms clear + 90ms duplicate guard",g_macheteCuts121.hits,g_macheteCuts121.rearms,g_macheteCuts121.feedback);
        Log("[MACHETE PAWN120] pawnContacts %u attempts %u pawnDamageDispatches %u nativeBuilderBlocked %u nonPawnContacts %u",p.contacts,p.attempts,p.dispatches,p.blocked,p.nonPawns);
        Log("[MACHETE ACTOR119] searches %u bodies %u rays %u shapeContacts %u selected %u bodyDamageDispatches %u errors %u",a.searches,a.bodies,a.rays,a.intersections,a.selected,a.dispatches,a.errors);
        Log("[MACHETE COLLIDER118] summary: installed %u latched %08x/%08x calls %u casts %u contacts %u kept %u dropped %u damageEvents %u (entity %u prop %u) strikes %u strikeHits %u strikeRejects %u queryWrites %u poseWrites %u rejects %u (last %u) exceptions %u lastSpeed %.2f",
            b.installed?1u:0u,b.strategy,b.config,b.calls,b.casts,b.contacts,b.kept,b.dropped,b.events,b.actorHits,b.objectHits,b.strikes,b.strikeHits,b.strikeRejects,b.queryWrites,b.poseWrites,b.rejects,b.lastReject,b.exceptions,b.lastSpeed);
    }
}
void MacheteBladeContactNoteLine(wchar_t* out,size_t count)
{
    const auto& b=g_macheteBladeContact;
    if(!b.enabled){_snwprintf_s(out,count,_TRUNCATE,L"Physical swing: native attack damage");return;}
    _snwprintf_s(out,count,_TRUNCATE,L"BLADE DAMAGE %ls | strikes %u enemy hits %u",
        !b.installed?(b.installError?L"HOOK FAILED":L"waiting"):!b.strategy?L"armed after first swipe":L"ON",b.strikes,b.actorHits);
}
void LoadMacheteBladeContactSettings(const wchar_t* path)
{
    auto& b=g_macheteBladeContact;
#ifndef FC3_AMMO_TEST
    b.enabled=GetPrivateProfileIntW(L"Machete",L"BladeDamage",1,path)!=0;
#endif
    b.autoAim=false;b.strikeAssist=false;b.minSpeed=g_macheteCollider118.minSpeed;
    b.bladeRadius=g_macheteCollider118.radius;b.damageScale=g_macheteCollider118.damageScale;b.cooldownMs=UINT(g_macheteCollider118.cooldownMs);
    Log("[MACHETE COLLIDER118] damage %s, capsule length %.3f radius %.3f, minimum physical speed %.2f, damage x%.2f, per-enemy cooldown %u ms; native attack gestures OFF; camera-aimed damage OFF",b.enabled?"ON":"off",g_macheteCollider118.length,b.bladeRadius,b.minSpeed,b.damageScale,b.cooldownMs);
}
