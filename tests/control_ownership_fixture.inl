void TestShotgunControlOwnership()
{
    Check(!g_physicalReloadEnabled,"unfinished physical ammo path starts inactive");
    const LONG savedFrame=g_frame;
    const bool savedMotion=g_motionGun,savedRight=g_handRightValid,savedLeft=g_handLeftValid;
    g_frame=30000;g_motionGun=g_handRightValid=g_handLeftValid=true;
    g_reload=PhysicalReloadState{};g_reload.installed=true;g_reload.shotgunFrame=g_frame;
    g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=true;
    float savedBody[2][16];memcpy(savedBody,g_handBody,sizeof(savedBody));
    for(auto& pose:g_handBody){memset(pose,0,sizeof(pose));pose[0]=pose[5]=pose[10]=pose[15]=1;}
    g_handBody[1][3]=-.3f;g_handBody[1][7]=-.55f;g_handBody[1][11]=.2f;
    fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::SuppressLeftGrip(false);
    fc3pad::Controllers c={};c.active=true;
    for(int combo=0;combo<8;++combo)
    {
        c.leftGrip=(combo&1)?1.0f:0.0f;c.leftTrigger=(combo&2)?1.0f:0.0f;c.rightGrip=(combo&4)?1.0f:0.0f;
        for(int frame=0;frame<20;++frame)
        {
            fc3pad::Publish(c);TickPhysicalReload(true);
            Check(!fc3pad::ReloadOutput()&&fc3pad::AimOutput()==0&&(fc3pad::ButtonsOutput()&0x300)==0,
                "held grip/trigger combinations cannot send native reload, aim or shoulder actions");
            Check(!g_reload.held&&!g_reload.mode&&!g_reload.credit&&!g_reload.pending&&g_reload.requests==0,
                "waist input cannot start unfinished physical reload during control isolation");
            const auto raw=fc3pad::Latest();
            Check(raw.leftGrip==c.leftGrip&&raw.rightGrip==c.rightGrip&&raw.leftTrigger==c.leftTrigger,
                "physical pump and finger input retains original controller values");
        }
    }
    c.leftGrip=c.rightGrip=0;c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(fc3pad::ReloadOutput(),"a single ordinary X press reaches the game with grips/trigger held");
    for(int i=0;i<20;++i){fc3pad::Publish(c);TickPhysicalReload(true);Check(fc3pad::ReloadOutput(),"ordinary held X is not converted into generated or double-tap reload");}
    c.x=false;fc3pad::Publish(c);TickPhysicalReload(true);Check(!fc3pad::ReloadOutput(),"X release clears game input immediately");
    c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);Check(fc3pad::ReloadOutput(),"second X press remains a separate ordinary press");
    c.x=false;c.leftGrip=0;c.rightTrigger=1;c.a=c.b=c.y=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(fc3pad::FireOutput()==255&&(fc3pad::ButtonsOutput()&0xb000)==0xb000,"other buttons and firing are retained");
    // Native forwarding must remain intact even if an old credit/mode survived.
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,24576,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(page!=nullptr,"isolated native reload context");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    *reinterpret_cast<UINT*>(weapon+0xcc)=5;*reinterpret_cast<UINT*>(hud+0x134)=5;*reinterpret_cast<UINT*>(hud+0x138)=7;*reinterpret_cast<UINT*>(hud+0x140)=997;*reinterpret_cast<UINT*>(hud+0x144)=1002;
    g_nativeReload=FixtureNativeReload;g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;g_reloadFixtureCalls=0;
    ObserveAmmoWidget(wrapper,hud);g_reload.state=state;g_reload.wrapper=wrapper;g_reload.weapon=weapon;g_reload.ready=g_reload.mode=g_reload.credit=g_reload.pending=1;g_reload.deadline=GetTickCount64()+10000;
    *reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)==996&&!*reinterpret_cast<BYTE*>(state+8)&&g_reload.commits==0,
        "native ordinary reload consumes reserve and clears pending without physical gate interference");
    TickPhysicalReload(true);*reinterpret_cast<BYTE*>(state+8)=1;ReloadBoundary(state,wrapper,0,-1);
    Check(ReloadRead(weapon,0xcc)==7&&ReloadRead(hud,0x140)==995&&g_reloadFixtureCalls==2&&g_reload.requests==0,
        "captured 5-to-7 ordinary reload fills both shells rather than becoming stuck behind credits");
    // Scope exits on weapon switch, menu, controller loss and motion disable.
    fc3pad::CancelReloadRequest();c={};c.active=true;c.leftGrip=c.rightGrip=c.leftTrigger=1;fc3pad::Publish(c);
    g_reload.shotgunFrame=g_frame-3;TickPhysicalReload(true);
    Check(fc3pad::AimOutput()==255&&(fc3pad::ButtonsOutput()&0x300)==0,"weapon switch restores aim while paused grenade input and grip-only wheel remain blocked");
    g_reload.shotgunFrame=g_frame;TickPhysicalReload(false);Check(fc3pad::AimOutput()==255,"menus retain normal controller mapping");
    TickPhysicalReload(true);Check(fc3pad::AimOutput()==0,"returning to shotgun gameplay resumes handling mapping");
    c.active=false;fc3pad::Publish(c);TickPhysicalReload(true);Check(fc3pad::AimOutput()==0&&fc3pad::ButtonsOutput()==0,"controller inactivity clears scoped mapping and emits neutral input");
    c.active=true;fc3pad::Publish(c);g_motionGun=false;TickPhysicalReload(true);Check(fc3pad::AimOutput()==255,"motion toggle restores stock aim");
    memcpy(g_handBody,savedBody,sizeof(savedBody));g_motionGun=savedMotion;g_handRightValid=savedRight;g_handLeftValid=savedLeft;g_frame=savedFrame;
    g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=false;g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_nativeReload=nullptr;
    fc3pad::SetShotgunHandling(false);fc3pad::Publish(fc3pad::Controllers{});VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: shotgun control ownership; all held grip/trigger combinations, raw physical input, single/double/held X, native reload passthrough and mapping scope recovery");
}

