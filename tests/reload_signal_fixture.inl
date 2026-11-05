UINT g_signalCalls=0,g_readyCalls=0,g_predicateCalls=0;
BYTE g_fixturePredicate=0,g_fixtureReady=1,g_fixtureResult=0;
UINT g_fixtureHash=0xaf41179b,g_fixtureSignalSite=0,g_fixtureFallback=0;
UINT g_signalStackBefore=0,g_signalStackAfter=0;
float g_signalSse[4]={},g_signalSeed[4]={1,2,3,4};
UINT __stdcall SignalFixtureAuthority(UINT,UINT a,UINT b){return a==101&&b==202;}
__declspec(naked) void SignalFixturePredicate(){__asm{
inc g_predicateCalls
mov al,g_fixturePredicate
ret 4
}}
__declspec(naked) void SignalFixtureHash(){__asm{
lea eax,g_fixtureHash
ret
}}
__declspec(naked) void SignalFixturePublish(){__asm{
inc g_signalCalls
pop eax
add esp,4
jmp eax
}}
__declspec(naked) void SignalFixtureReadiness(){__asm{
inc g_readyCalls
mov al,g_fixtureReady
ret
}}
// Native request4 control flow, including true signal path and false readiness
// path. The hook is at the exact five-byte signal construction instructions.
__declspec(naked) void SignalFixtureNative()
{
    __asm {
        push ebp
        mov ebp,esp
        mov eax,[esi+48h]
        push edi
        push 1
        call SignalFixturePredicate
        test al,al
        jz fallback
        mov edi,[esi+48h]
        call SignalFixtureHash
    signalSite:
        mov eax,[eax]
        push ecx
        mov ecx,esp
        mov [ecx],eax
        call SignalFixturePublish
        mov al,1
        pop edi
        pop ebp
        ret 4
    fallback:
        call SignalFixtureReadiness
        test al,al
        setne al
        pop edi
        pop ebp
        ret 4
    }
}
__declspec(naked) void __cdecl InvokeSignalFixture(UINT,UINT)
{
    __asm {
        push ebp
        mov ebp,esp
        push esi
        mov esi,[ebp+12]
        mov eax,[ebp+8]
        mov g_signalStackBefore,esp
        movups xmm0,[g_signalSeed]
        push eax
        call SignalFixtureNative
        mov g_fixtureResult,al
        mov g_signalStackAfter,esp
        movups [g_signalSse],xmm0
        pop esi
        pop ebp
        ret
    }
}
void TestReloadSignal()
{
    const auto priorGate=g_reloadSignal;const auto priorManual=g_manualReload;
    const auto priorAuthority=g_singleReloadAuthority;const UINT priorManager=g_singleReloadManager;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"signal fixture allocation");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,ref=actor+20480,mgr=actor+24576;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(actor+4)=ref;*reinterpret_cast<UINT*>(state)=actor;
    *reinterpret_cast<UINT*>(wrapper+0x48)=weapon;*reinterpret_cast<UINT*>(ref)=101;*reinterpret_cast<UINT*>(ref+4)=202;*reinterpret_cast<UINT*>(mgr)=actor;
    g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=mgr;
    g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_reloadSignal.manualPistol=false;g_reloadSignal.manualAk=false;g_reloadSignal.manualSkorpion=false;g_motionGun=true;g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;
    g_manualReload=ManualReloadState{};g_manualReloadEnabled=false;
    fc3pad::Controllers c={};c.active=true;c.rightTrigger=1;fc3pad::Publish(c);
    fc3pad::SetManualReload(false,false);fc3pad::CancelReloadRequest();
    BYTE* fn=reinterpret_cast<BYTE*>(SignalFixtureNative);
    // /O2 has no incremental-link thunks. Find the exact construction and the
    // unique call/readiness/test/setne sequence within this single function.
    const BYTE sig[]={0x8b,0,0x51,0x8b,0xcc};
    for(int i=0;i<128;++i){if(!memcmp(fn+i,sig,5))g_fixtureSignalSite=reinterpret_cast<UINT>(fn+i);
        if(fn[i]==0xe8&&fn[i+5]==0x84&&fn[i+6]==0xc0&&fn[i+7]==0x0f&&fn[i+8]==0x95&&fn[i+9]==0xc0)g_fixtureFallback=reinterpret_cast<UINT>(fn+i);}
    Check(g_fixtureSignalSite&&g_fixtureFallback,"native signal and readiness branches located");
    MH_Initialize();Check(InstallReloadSignalAt(reinterpret_cast<void*>(g_fixtureSignalSite),reinterpret_cast<void*>(g_fixtureFallback)),"actual signal MinHook installed");
    *reinterpret_cast<UINT*>(weapon+0xcc)=8;g_fixturePredicate=0;
    for(int i=0;i<100;++i){InvokeSignalFixture(state,wrapper);Check(g_fixtureResult==1,"loaded native readiness stays true");}
    Check(g_readyCalls==100&&g_signalCalls==0&&g_reloadSignal.blocked==0,"loaded checks never reach reload gate");
    *reinterpret_cast<UINT*>(weapon+0xcc)=0;g_fixturePredicate=1;
    const int profiles[]={3,20,39,8,34,0,31,10};
    for(int p:profiles){g_weaponMechanicProfile=p;for(int i=0;i<100;++i)InvokeSignalFixture(state,wrapper);}
    Check(g_signalCalls==0&&g_readyCalls==900&&g_reloadSignal.blocked==800,"800 empty reload signals suppressed with native readiness executed");
    Check(g_fixtureResult==1&&g_signalStackBefore==g_signalStackAfter,"blocked signal retains native return and ret4 stack");Compare(g_signalSse,g_signalSeed,4,"signal decision preserves SSE");
    g_fixtureReady=0;InvokeSignalFixture(state,wrapper);Check(g_fixtureResult==0,"native false readiness stays false");g_fixtureReady=1;
    g_weaponMechanicProfile=3; // explicit native-X policy; bow (last loop profile) deliberately refuses X reload
    c.x=true;fc3pad::Publish(c);fc3pad::RequestReload();InvokeSignalFixture(state,wrapper);Check(g_signalCalls==1,"explicit native reload request reaches publication");fc3pad::CancelReloadRequest();
    c.x=false;fc3pad::Publish(c);g_weaponMechanicProfile=3;g_manualReloadEnabled=true;g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+2000;
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==1,"251 physical shotgun insertion without native data callbacks cannot publish timed reload");g_manualReload.pending=0;g_manualReloadEnabled=false;
    *reinterpret_cast<UINT*>(weapon+0xcc)=2;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==2,"partial-magazine native reload remains available");*reinterpret_cast<UINT*>(weapon+0xcc)=0;
    g_fixtureHash=0x8bdd95c1;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==3,"AI reload signal untouched");g_fixtureHash=0xaf41179b;
    *reinterpret_cast<UINT*>(ref)=999;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==4,"nonlocal actor untouched");*reinterpret_cast<UINT*>(ref)=101;
    g_weaponMechanicFrame=g_frame-3;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==5,"stale identity passes through");g_weaponMechanicFrame=g_frame;
    c.active=false;fc3pad::Publish(c);InvokeSignalFixture(state,wrapper);Check(g_signalCalls==5,"inactive controller does not authorize an automatic reload");
    // New manual-pistol policy: preserve raw X interactions but deny its reload
    // transition at every loaded count. Normal loaded readiness never sees hook.
    c.active=true;c.x=true;fc3pad::Publish(c);g_weaponMechanicProfile=20;g_reloadSignal.manualPistol=true;
    const UINT beforeX=g_signalCalls;
    for(UINT loaded=0;loaded<=8;++loaded){*reinterpret_cast<UINT*>(weapon+0xcc)=loaded;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX,"1911 X cannot reload at any loaded count");}
    Check((fc3pad::ButtonsOutput()&0x4000)!=0,"raw X remains available for native interactions");
    // Permission tests represent an insertion request, not the interaction-only
    // button held by the preceding negative control.
    c.x=false;fc3pad::Publish(c);
    g_fixturePredicate=0;InvokeSignalFixture(state,wrapper);Check(g_fixtureResult==1,"loaded gun native readiness unaffected by X reload policy");g_fixturePredicate=1;
    Check(GrantPistolReloadPermission(state,wrapper),"manual insertion can grant exact native owner permission");
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+1,"manual permission reaches one native reload publication");
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+1,"same insertion permission cannot publish twice");
    Check(GrantPistolReloadPermission(state,wrapper),"new insertion permission available");g_reloadSignal.pistolPermissionUntil=GetTickCount64()-1;
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+1,"expired insertion permission cannot reload");
    Check(GrantPistolReloadPermission(state,wrapper),"owner permission available");g_reloadSignal.pistolWeapon=weapon+4;
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+1,"changed weapon cannot spend insertion permission");g_reloadSignal.pistolPermissionUntil=0;
    g_reloadSignal.manualPistol=false;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+2,"configured native X fallback restored");
    g_reloadSignal.manualPistol=true;g_weaponMechanicProfile=8;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeX+3,"AK native reload unaffected by 1911 policy");
    std::puts("PASS: 1911 native X reload blocked at loaded 0..8; raw X interactions and native readiness preserved; exact-owner single-use insertion permission, expiry/weapon-change rejection, configured native fallback and other weapons preserved.");
    g_weaponMechanicProfile=3;g_weaponMechanicFrame=g_frame;g_manualReloadEnabled=true;
    fc3pad::SetShotgunHandling(true);fc3pad::SetManualReload(true,false);g_manualReload.pending=0;
    c={};c.active=true;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);
    const UINT beforePump=g_signalCalls;
    for(UINT loaded=0;loaded<=8;++loaded){*reinterpret_cast<UINT*>(weapon+0xcc)=loaded;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforePump,"interaction X cannot bulk-reload manual shotgun at any count");}
    Check(fc3pad::ButtonsOutput()&0x4000,"manual shotgun raw X reaches mission interaction");
    g_fixturePredicate=0;InvokeSignalFixture(state,wrapper);Check(g_fixtureResult==1,"manual shotgun readiness remains native");g_fixturePredicate=1;
    g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+2000;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforePump,"251 physical shotgun insertion without callbacks denies timed animation");
    g_manualReload.deadline=GetTickCount64()-1;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforePump,"expired shotgun insertion cannot publish reload");
    g_manualReload.pending=0;g_fixtureHash=0x8bdd95c1;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforePump+1,"non-reload mission signal is unaffected");g_fixtureHash=0xaf41179b;
    fc3pad::SetManualReload(false,false);fc3pad::SetShotgunHandling(false);g_manualReloadEnabled=false;c.x=false;fc3pad::Publish(c);
    puts("PASS: actual native signal hook preserves manual-shotgun interaction and readiness; blocks raw-X reload at loaded0..8; insertion/expiry and unrelated signals checked");
    // Dedicated B uses native interaction X but must not initiate a native reload.
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetDedicatedInteractions(true);fc3pad::SetGameplayControls(true);
    g_weaponMechanicProfile=3;g_weaponMechanicFrame=g_frame;g_manualReloadEnabled=false;g_reloadSignal.enabled=false;
    c.x=true;fc3pad::Publish(c);g_fixturePredicate=1;g_fixtureHash=0xaf41179b;
    for(UINT loaded=0;loaded<=8;++loaded){
      *reinterpret_cast<UINT*>(weapon+0xcc)=loaded;const UINT before=g_signalCalls;
      InvokeSignalFixture(state,wrapper);Check(g_signalCalls==before&&g_fixtureResult==1,"dedicated B interaction denies reload at every loaded count even with auto-reload setting off");
    }
    const UINT beforeStale=g_signalCalls;g_weaponMechanicProfile=-1;g_weaponMechanicFrame=g_frame-100;g_motionGun=false;
    InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeStale,"B remains interaction-only through stale/unknown scripted weapon transition");
    *reinterpret_cast<UINT*>(ref)=999;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeStale+1,"B filter never blocks nonlocal actor");*reinterpret_cast<UINT*>(ref)=101;
    g_weaponMechanicProfile=3;g_weaponMechanicFrame=g_frame;g_motionGun=true;
    const UINT beforeInsertion=g_signalCalls;fc3pad::RequestReload();InvokeSignalFixture(state,wrapper);
    Check(g_signalCalls==beforeInsertion+1,"real insertion pulse is not mistaken for dedicated interaction");fc3pad::CancelReloadRequest();
    g_fixtureHash=0x12345678;const UINT beforeOther=g_signalCalls;InvokeSignalFixture(state,wrapper);Check(g_signalCalls==beforeOther+1,"B does not filter unrelated native signals");g_fixtureHash=0xaf41179b;
    c.x=false;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
    puts("PASS: controls20 real native hook: dedicated B cannot reload, loaded0..8, genuine insertion pulse and unrelated signals preserved");
    MH_DisableHook(reinterpret_cast<void*>(g_fixtureSignalSite));MH_RemoveHook(reinterpret_cast<void*>(g_fixtureSignalSite));g_reloadSignalOriginal=g_reloadSignalFallback=nullptr;
    g_reloadSignal=priorGate;g_manualReload=priorManual;g_singleReloadAuthority=priorAuthority;g_singleReloadManager=priorManager;
    g_reload=PhysicalReloadState{};VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: reload signal MinHook preserves loaded/native readiness and ret4/SSE; 800 empty reload events suppressed; explicit X, shotgun insertion, partial reload, AI/nonlocal/stale passthrough and inactive automatic-reload suppression.");
}
