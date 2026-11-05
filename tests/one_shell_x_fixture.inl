static UINT g_singleFixtureActor=0,g_singleFixtureCapacity=7;
UINT __stdcall FixtureSingleAuthority(UINT manager,UINT lo,UINT hi)
{return manager==0x123456&&lo==g_singleFixtureActor&&hi==0x5544?lo:0;}
void __stdcall FixtureSingleBegin(UINT state,UINT wrapper)
{
    FixtureReloadBegin(state,wrapper);
    const UINT count=(std::min)(g_singleFixtureCapacity-ReloadRead(g_reloadFixtureWeapon,0xcc),ReloadRead(g_reloadFixtureHud,0x140));
    FixtureTraceNativeStore(state,wrapper,g_traceFixtureProperty,count);
}
void __stdcall FixtureSingleTransfer(UINT state,UINT,UINT explicitCount,int count)
{
    const UINT loaded=ReloadRead(g_reloadFixtureWeapon,0xcc),reserve=ReloadRead(g_reloadFixtureHud,0x140);
    if(!(ReloadRead(state,8)&255)||loaded>=g_singleFixtureCapacity||!reserve)return;
    const UINT n=(std::min)((std::min)(explicitCount?static_cast<UINT>(count):1,reserve),g_singleFixtureCapacity-loaded);
    *reinterpret_cast<UINT*>(g_reloadFixtureWeapon+0xcc)=loaded+n;
    *reinterpret_cast<UINT*>(g_reloadFixtureHud+0x134)=loaded+n;
    *reinterpret_cast<UINT*>(g_reloadFixtureHud+0x140)=reserve-n;
    *reinterpret_cast<WORD*>(state+8)=n?256:0;
}
void FixtureSingleAdvance(UINT state,UINT wrapper)
{
    int count=static_cast<int>(ReloadRead(g_traceFixtureProperty,8));
    if((ReloadRead(state,8)>>8)&255)
    {
        --count;FixtureTraceNativeStore(state,wrapper,g_traceFixtureProperty,static_cast<UINT>(count));
        *reinterpret_cast<BYTE*>(state+9)=0;
    }
    *reinterpret_cast<BYTE*>(state+8)=count>0?1:0;
    if(count<=0)ReloadEndBoundary(state,wrapper,1);
}
#include "manual_reload_fixture.inl"
void TestOneShellX(ID3D11Device* device,ID3D11DeviceContext* context)
{
    const LONG priorFrame=g_frame;const bool priorMotion=g_motionGun,priorEnabled=g_singleShellEnabled;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&code,"single shell native fixture allocation");
    memcpy(code,kNativeReloadCountStore,sizeof(kNativeReloadCountStore));code[sizeof(kNativeReloadCountStore)]=0xc3;DWORD protection=0;
    Check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection)!=0,"single-shell real native count store code");FlushInstructionCache(GetCurrentProcess(),code,8);
    const auto init=MH_Initialize();Check(init==MH_OK||init==MH_ERROR_ALREADY_INITIALIZED,"single-shell hook library");
    Check(MH_CreateHook(code,reinterpret_cast<void*>(ReloadCountStoreBoundary),&g_nativeReloadCountStore)==MH_OK&&MH_EnableHook(code)==MH_OK,"actual count-store MinHook for one-shell mode");
    g_traceFixtureStore=code;
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192;
    const UINT weapon=actor+12288,hud=actor+16384,property=actor+20480,reference=actor+24576,manager=reference+256;
    *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=actor;*reinterpret_cast<UINT*>(reference+4)=0x5544;
    *reinterpret_cast<UINT*>(manager)=0x123456;*reinterpret_cast<UINT*>(actor+0x24)=component;
    *reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    g_traceFixtureProperty=property;g_singleFixtureActor=actor;g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;
    g_singleReloadAuthority=FixtureSingleAuthority;g_singleReloadManager=manager;
    g_nativeReloadBegin=FixtureSingleBegin;g_nativeReloadEnd=FixtureReloadEnd;g_nativeReload=FixtureSingleTransfer;
    g_frame=80000;g_motionGun=true;g_singleShellEnabled=true;g_physicalReloadEnabled=false;
    TestManualInsertion(state,wrapper,weapon,hud,device,context);
    // 2/7, upgraded 2/8, upgraded 11/12, and last reserve shell; no initial HUD discovery.
    const UINT scenarios[][3]={{7,2,992},{8,2,992},{12,11,992},{8,3,1}};
    for(const auto& scenario:scenarios)
    {
        g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_reloadCountTrace=ReloadCountTrace{};
        g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_singleFixtureCapacity=scenario[0];
        *reinterpret_cast<UINT*>(weapon+0xcc)=scenario[1];*reinterpret_cast<UINT*>(hud+0x134)=scenario[1];
        *reinterpret_cast<UINT*>(hud+0x138)=scenario[0];*reinterpret_cast<UINT*>(hud+0x140)=scenario[2];
        *reinterpret_cast<UINT*>(hud+0x144)=scenario[1]+scenario[2];
        fc3pad::SetOneShellX(false);fc3pad::SetShotgunHandling(true);
        fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);TickPhysicalReload(true);
        c.leftGrip=1;c.leftTrigger=1;c.x=true;fc3pad::Publish(c);
        Check(fc3pad::ReloadOutput(),"X starts one-shell reload regardless of grip or back trigger");
        ReloadBeginBoundary(state,wrapper);
        Check(g_reloadCountTrace.single&&g_reloadCountTrace.cycle&&g_reloadCountTrace.property==property&&ReloadRead(property,8)==1&&g_traceFixtureEcx==1&&g_traceFixtureStackCount==1,
              "native property ECX and start-notification stack count both limited to one, no getter/HUD discovery needed");
        Check(ReloadRead(weapon,0xcc)==scenario[1]&&ReloadRead(hud,0x140)==scenario[2]&&ReloadPoseActive(),"one-shell start grants no ammo early and owns native posing");
        for(int i=0;i<100;++i){fc3pad::Publish(c);Check(!fc3pad::ReloadOutput(),"held X cannot send another native start while reloading");}
        TickPhysicalReload(false);fc3pad::Publish(c);
        Check(!fc3pad::ReloadOutput(),"consumed X stays owned through temporary gameplay/handling loss");
        TickPhysicalReload(true);fc3pad::Publish(c);
        Check(!fc3pad::ReloadOutput(),"handling recovery cannot re-emit a held X press");
        c.x=false;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);Check(!fc3pad::ReloadOutput(),"new X during active reload is consumed without queueing next cycle");
        ReloadBoundary(state,wrapper,0,-1);ReloadBoundary(state,wrapper,0,-1); // native duplicate no-op
        Check(ReloadRead(weapon,0xcc)==scenario[1]+1&&ReloadRead(hud,0x140)==scenario[2]-1,"one actual insertion changes +1 loaded/-1 reserve exactly once");
        FixtureSingleAdvance(state,wrapper);
        Check(!g_reloadCountTrace.cycle&&!ReloadPoseActive()&&ReloadRead(property,8)==0&&(ReloadRead(state,8)&65535)==0,"normal native advance ends one shell cleanly without a refill loop");
        for(int i=0;i<100;++i){fc3pad::Publish(c);Check(!fc3pad::ReloadOutput(),"held busy-time press stays consumed after native end");}
        c.x=false;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);Check(fc3pad::ReloadOutput(),"release and next press permits a separate next shell");
        ReloadBeginBoundary(state,wrapper);const UINT available=(std::min)(scenario[0]-scenario[1]-1,scenario[2]-1);
        Check(ReloadRead(property,8)==(available?1:0)&&g_traceFixtureStackCount==(available?1u:0u),"next cycle preserves actual capacity and empty-reserve limits");
        ReloadBoundary(state,wrapper,0,-1);FixtureSingleAdvance(state,wrapper);
        Check(ReloadRead(weapon,0xcc)==scenario[1]+1+(available?1:0)&&ReloadRead(hud,0x140)==scenario[2]-1-(available?1:0),"second press loads one more, while full or no-reserve loads none");
    }
    // HUD identity accepts upgraded capacity as reported, independent of the authority fallback.
    *reinterpret_cast<UINT*>(hud+0x140)=10;*reinterpret_cast<UINT*>(hud+0x144)=ReloadRead(weapon,0xcc)+10;
    ObserveAmmoWidget(wrapper,hud);Check(ReloadLocal(wrapper),"HUD validation uses reported capacity instead of hardcoded seven");
    // Leaving motion shotgun scope restores ordinary input and native start count.
    fc3pad::SetShotgunHandling(false);fc3pad::SetOneShellX(false);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);
    Check(fc3pad::ReloadOutput(),"other weapon/menu scope restores ordinary X");
    g_reload.shotgunFrame=g_frame-3;g_singleFixtureCapacity=7;*reinterpret_cast<UINT*>(weapon+0xcc)=2;*reinterpret_cast<UINT*>(hud+0x140)=992;
    ReloadBeginBoundary(state,wrapper);Check(ReloadRead(property,8)==5&&g_traceFixtureStackCount==5,"non-shotgun native cycle count passes through unchanged");
    // A just-selected pistol must bypass one-shell logic even while the
    // previous shotgun draw timestamp is still fresh.
    const int priorProfile=g_weaponMechanicProfile;
    g_weaponMechanicProfile=fc3weapon::ProfileIndex({177904,32682,32,240,294,4470,57});
    g_reload.shotgunFrame=g_frame;*reinterpret_cast<UINT*>(weapon+0xcc)=2;
    Check(!SingleShellNativeContext(state,wrapper)&&!ReloadPoseContext(state,wrapper),"known pistol rejects stale shotgun reload ownership");
    ReloadBeginBoundary(state,wrapper);Check(ReloadRead(property,8)==5&&g_traceFixtureStackCount==5,"known pistol retains native full-magazine count immediately after shotgun switch");
    g_weaponMechanicProfile=priorProfile;
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"one-shell actual hook cleanup");if(init==MH_OK)Check(MH_Uninitialize()==MH_OK,"one-shell hook library cleanup");
    g_nativeReloadCountStore=nullptr;g_traceFixtureStore=nullptr;g_singleReloadAuthority=nullptr;g_singleReloadManager=0;
    g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_reloadCountTrace=ReloadCountTrace{};
    g_frame=priorFrame;g_motionGun=priorMotion;g_singleShellEnabled=priorEnabled;
    fc3pad::SetShotgunHandling(false);fc3pad::SetOneShellX(false);fc3pad::Publish(fc3pad::Controllers{});
    VirtualFree(page,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS: one X -> one shell via actual engine store; property and start-event stack count agree; 2/7->3/7->4/7; capacities8/12, full/no-reserve, no setup discovery, grip/back trigger, held/busy X, native advance/end and other-weapon passthrough");
}
