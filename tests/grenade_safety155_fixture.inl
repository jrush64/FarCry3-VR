#include "molotov_flame160_fixture.inl"
#include "molotov_trail176_fixture.inl"
volatile unsigned activationEntries155=0,debitEntries155=0,cancelEntries155=0;
volatile UINT idleAtActivation158=0;
volatile UINT activationEvent155=0;volatile ULONGLONG activationGuid155=0;
__declspec(naked) void DebitFixture155(){
    __asm mov [ecx+60h],eax
    __asm inc dword ptr [debitEntries155]
    __asm ret
}
void __fastcall CancelFixture155(void* self,void*,UINT){
    const UINT strategy=reinterpret_cast<UINT>(self);
    Check(*reinterpret_cast<BYTE*>(strategy+0x6b)==1,"safe stow chooses native deactivate branch, never native auto-throw branch");
    ++cancelEntries155;
}
__declspec(naked) void CallActivateFixture155(UINT,UINT){
    __asm push ebp
    __asm mov ebp,esp
    __asm push ebx
    __asm push esi
    __asm push edi
    __asm mov eax,[ebp+0ch]
    __asm fninit
    __asm fld dword ptr [kX87]
    __asm movups xmm0,[g_seed]
    __asm push dword ptr [ebp+8]
    __asm stc
    __asm std
    __asm call dword ptr [g_entry]
    __asm movups [g_sse],xmm0
    __asm fstp dword ptr [g_x87]
    __asm pushfd
    __asm pop eax
    __asm mov [g_flags],eax
    __asm cld
    __asm pop edi
    __asm pop esi
    __asm pop ebx
    __asm pop ebp
    __asm ret
}
void TestGrenadeSafety155(UINT strategy,UINT execution,UINT projectile,UINT inventory,UINT event){
    auto* component=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(component&&code&&MH_Initialize()==MH_OK,"native grenade safety fixture allocation");
    const UINT comp=reinterpret_cast<UINT>(component),ref=Word(strategy,0xf4);
    *reinterpret_cast<UINT*>(component)=g_engine+0x1fc67c0;*reinterpret_cast<UINT*>(component+4)=ref;
    memcpy(code,kActivateBytes155,sizeof(kActivateBytes155));UINT at=sizeof(kActivateBytes155);
    const BYTE marker[]={0xff,0x05};memcpy(code+at,marker,2);at+=2;
    const UINT counter=reinterpret_cast<UINT>(&activationEntries155);memcpy(code+at,&counter,4);at+=4;
    // Minimal native effects from the VERIFIED activation instructions: active
    // becomes1, fuse initializes from config. The bridge must prevent BOTH.
    const BYTE idleRead158[]={0x50,0x0f,0xb6,0x83,0xcc,0,0,0,0xa3};memcpy(code+at,idleRead158,sizeof(idleRead158));at+=sizeof(idleRead158);
    const UINT idleOutput158=reinterpret_cast<UINT>(&idleAtActivation158);memcpy(code+at,&idleOutput158,4);at+=4;code[at++]=0x58;
    const BYTE body[]={0xc6,0x83,0x34,0x02,0,0,1,0xd9,0x83,0xb8,0,0,0,0xd9,0x9b,0x38,0x02,0,0,0xa3};
    memcpy(code+at,body,sizeof(body));at+=sizeof(body);const UINT observed=reinterpret_cast<UINT>(&activationEvent155);memcpy(code+at,&observed,4);at+=4;
    const BYTE guid[]={0xf3,0x0f,0x7e,0x40,0x10,0x66,0x0f,0xd6,0x05};memcpy(code+at,guid,sizeof(guid));at+=sizeof(guid);const UINT guidOut=reinterpret_cast<UINT>(&activationGuid155);memcpy(code+at,&guidOut,4);at+=4;
    const BYTE tail[]={0x5b,0x8b,0xe5,0x5d,0xc2,0x04,0};memcpy(code+at,tail,sizeof(tail));
    FlushInstructionCache(GetCurrentProcess(),code,128);g_entry=code;
    Check(MH_CreateHook(code,reinterpret_cast<void*>(ActivationBoundary155),&g_activateOriginal155)==MH_OK&&MH_EnableHook(code)==MH_OK,"exact CExplosive activation MinHook relocation");
    fixtureLocal143=0x1111222233334444ULL;SetIdentityReader143(ReadLocal143);SetPinIntent155(true);PublishContext143(fixtureLocal143,3,true);
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x03c006e5ac354e19ULL;
    *reinterpret_cast<ULONGLONG*>(projectile+8)+=100;
    UINT regs[9]={strategy,execution,0,0,strategy,0,projectile,0,0};Observe(1,regs);Prepared143 target;Check(CurrentPrepared143(target),"exact local frag acquired before activation");
    *reinterpret_cast<float*>(component+0xb8)=3;*reinterpret_cast<float*>(component+0x238)=-1;
    *reinterpret_cast<UINT*>(event+0x10)=0x12345678;*reinterpret_cast<UINT*>(event+0x14)=0x87654321;
    volatile UINT canary=0x76543210;
    for(int i=0;i<1000;++i)CallActivateFixture155(comp,event);
    Check(!component[0x234]&&*reinterpret_cast<float*>(component+0x238)==-1&&activationEntries155==0,"1000 stock activation attempts cannot start or reset a pin-in fuse");
    Check(canary==0x76543210&&(g_flags&0x401)==0x401&&!memcmp(g_seed,g_sse,sizeof(g_seed))&&g_x87==kX87,"blocked activation preserves ret4 stack CF DF SSE x87");
    Check(g_safe155.eventValid&&!memcmp(g_safe155.event,reinterpret_cast<void*>(event),24),"deferred activation copies consumed owner event fields");
    Check(ManualReleaseBlocked155(strategy,execution),"stock animation cannot release held pin-in grenade");
    SetUnlimited147(false);*reinterpret_cast<UINT*>(inventory+0x60)=2;
    Check(PreserveSafeStock155(strategy,inventory)&&Word(inventory,0x60)==2,"safe hold defers finite inventory debit");
    g_stockSetter155=reinterpret_cast<UINT>(DebitFixture155);
    Check(RequestPinOut155(target),"mechanical extraction requests exact native arming");
    // Pause cannot arm. Recovery runs the captured activation once on the
    // native update and never from the rendering/controller thread.
    PublishContext143(0,0,false);PumpArm155(strategy,execution);Check(!component[0x234],"tracking pause cannot start deferred native fuse");
    PublishContext143(fixtureLocal143,3,true);PumpArm155(strategy,execution);PumpArm155(strategy,execution);
    Check(component[0x234]&&*reinterpret_cast<float*>(component+0x238)==3&&activationEntries155==1&&g_safe155.armed,"pin completion executes original activation exactly once");
    Check(Word(inventory,0x60)==1&&debitEntries155==1,"finite stock consumed once at actual arming");
    Check(activationEvent155!=event&&activationGuid155==*reinterpret_cast<ULONGLONG*>(event+0x10),"native arming keeps copied owner GUID");
    Check(ManualReleaseBlocked155(strategy,execution),"stock animation remains blocked while physical grip holds armed grenade");
    g_pending143.target=target;g_pending143.active=g_pending143.scheduled=true;
    Check(!ManualReleaseBlocked155(strategy,execution),"explicit native update release ticket can throw armed grenade");g_pending143={};
    // Another fresh unarmed grenade can be stowed even after frontend context
    // teardown. It must not activate just because input ownership was cleared.
    g_safe155={};CaptureSafe155(target);g_cleanupOriginal155=reinterpret_cast<void*>(CancelFixture155);
    CancelRelease143();SetPinIntent155(false);CleanupGate155(reinterpret_cast<void*>(strategy),nullptr,execution);
    Check(cancelEntries155==1&&!g_safe155.target.projectile&&Word(inventory,0x60)==1,"safe stow retires native item without fuse or inventory loss");
    component[0x234]=0;CallActivateFixture155(comp,event);Check(activationEntries155==2,"unowned explosive still uses native activation");
    // Reuse the exact activation ABI for Molotov: no automatic native fire,
    // contact requests activation on the native update, idle FX bit restored.
    const ULONGLONG oldArchetype158=*reinterpret_cast<ULONGLONG*>(strategy+0x40);
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x000006f6a4abae8dULL;
    auto molotov158=target;molotov158.archetype=0x000006f6a4abae8dULL;++molotov158.serial;
    SetIdentityReader143(ReadLocal143);PublishContext143(fixtureLocal143,3,true);SetMolotovIntent158(true);
    g_safe155={};CaptureSafe155(molotov158);component[0x234]=0;component[0xcc]=1;
    TestMolotovFlame160(molotov158,comp);
    TestMolotovTrail176(molotov158,comp);
    const unsigned beforeMolotov158=activationEntries155;
    CallActivateFixture155(comp,event);Check(!component[0x234]&&activationEntries155==beforeMolotov158,"manual Molotov blocks original automatic activation before fire becomes live");
    Check(IgniteMolotov158(molotov158)&&!MolotovLit158(molotov158),"wick contact requests exact native Molotov ignition; request alone cannot show lit");
    PumpArm155(strategy,execution);PumpArm155(strategy,execution);
    Check(MolotovLit158(molotov158)&&component[0x234]&&component[0xcc]==1&&idleAtActivation158==1&&activationEntries155==beforeMolotov158+1,"Molotov original activation once on native update; native rag flame enabled at ignition");
    g_cleanupOriginal155=reinterpret_cast<void*>(CancelFixture155);const auto cancels158=cancelEntries155;
    CleanupGate155(reinterpret_cast<void*>(strategy),nullptr,execution);
    Check(cancelEntries155==cancels158+1&&!g_safe155.target.projectile,"stowing lit Molotov uses native cancel/deactivate, never hidden stock auto-throw");
    g_safe155={};CaptureSafe155(molotov158);
    auto wrongMolotov158=molotov158;wrongMolotov158.projectile++;
    Check(!IgniteMolotov158(wrongMolotov158),"another projectile cannot receive ignition");
    g_safe155={};component[0x234]=0;CaptureSafe155(molotov158);CallActivateFixture155(comp,event);
    Check(LooseRelease156(molotov158)&&!component[0x234],"unlit Molotov physical release stays inactive and reuses protected loose-item path");
    g_safe155={};SetMolotovIntent158(false);*reinterpret_cast<ULONGLONG*>(strategy+0x40)=oldArchetype158;
    std::puts("PASS:158 exact Molotov activation deferral, native-update ignition acknowledgement, original idle config restoration, unlit release and wrong-projectile refusal.");
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK&&MH_Uninitialize()==MH_OK,"grenade safety hook cleanup");
    g_activateOriginal155=g_cleanupOriginal155=nullptr;g_stockSetter155=0;g_safe155={};SetIdentityReader143(nullptr);SetUnlimited147(true);
    VirtualFree(code,0,MEM_RELEASE);VirtualFree(component,0,MEM_RELEASE);
    std::puts("PASS:155 exact activation ABI; pin-in fuse suppression; copied owner event; paused arming rejection; native-update arming once; finite debit once; safe native stow; explicit physical release gate; unowned explosive passthrough.");
}
