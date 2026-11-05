// Physical input requests the already validated native one-shell X transaction.
// No engine calls, ammo edits or pending-state gates on the render thread.
void ApplyManualReloadMode(){g_manualReloadEnabled=g_manualSettings.mode==0;g_singleShellEnabled=g_manualSettings.mode!=2;g_physicalReloadEnabled=false;}
struct ManualReloadState
{
    volatile LONG pending=0,cycle=0;
    UINT state=0,wrapper=0,weapon=0,before=0;
    LONG lastContact=-1,contactFrames=0;
    ULONGLONG deadline=0,tick=0;
    bool x=false,chord=false,wasInside=false,committed=false;
    bool reloadWasEmpty=false,hadChamberedRound=false;
    int result=0; // 0 idle,1 success,2 no native start,3 no ammo,4 save OK,5 save failed
} g_manualReload;
// After a verified +1, prepare the next shell while the same native cycle
// finishes its closing tail. Never overlap two engine reload transactions.
bool ManualCanPrepare250(){
 const auto& m=g_manualReload;
 return g_manualReloadEnabled&&M133SupportProfile56()&&m.cycle&&m.committed&&
  SingleShellPoseOwner(m.state,m.wrapper)&&ReloadContext(m.state,m.wrapper,m.weapon)&&
  ReloadRead(m.wrapper,0x48)==m.weapon&&ReloadRead(m.weapon,0xcc)==m.before+1;
}
bool ManualReloadOwnsLeftHand()
{
    if(AkReloadSupport())return false; // shotgun/native pose lease cannot reclaim a seated AK support hand
    return g_manualReloadEnabled&&(g_reload.held||g_reloadAlignment||
        g_manualReload.pending||g_manualReload.cycle||ReloadPoseActive());
}
bool ManualContact(double& distance)
{
    return ReloadContactDistance(distance)&&g_reloadContact.handFrame==g_frame&&g_reloadContact.portFrame==g_frame;
}
double ManualRadius(){return g_manualSettings.radius*(std::max)(.1f,g_worldScale);}
void ManualMarker()
{
    if(g_pistolMenu.open&&g_weaponMechanicProfile>=3&&g_weaponMechanicProfile<=6&&g_m133GuideTarget56==1){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
    static UINT visualEpoch=0;static LONG visualProfile=-100000;
    if(g_reloadContact.port&&g_reloadContact.portFrame==g_frame){visualEpoch=g_armPoseEpoch;visualProfile=g_weaponMechanicProfile;}
    const bool pausedVisual=g_pistolMenu.open&&visualEpoch==g_armPoseEpoch&&visualProfile==g_weaponMechanicProfile;
    if(!PhysicalReloadGuidesEnabled()){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
    const bool show=g_manualReloadEnabled&&PhysicalReloadGuidesEnabled()&&
        (pausedVisual||(g_motionGun&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid&&g_frame-g_reload.shotgunFrame<=2))&&
        g_reloadContact.port&&(pausedVisual||g_reloadContact.portFrame==g_frame);
    if(!show){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
    double distance=0;const bool inside=g_reload.held&&ManualContact(distance)&&distance<=ManualRadius();
    float port[3]={static_cast<float>(g_reloadContact.portBody.x),static_cast<float>(g_reloadContact.portBody.y),static_cast<float>(g_reloadContact.portBody.z)};
    float shell[3]={static_cast<float>(g_reloadContact.shellBody.x),static_cast<float>(g_reloadContact.shellBody.y),static_cast<float>(g_reloadContact.shellBody.z)};
    fc3xr::SetReloadMarkers(port,(g_reloadContact.handFrame==g_frame||(pausedVisual&&g_reloadContact.handFrame>=0))?shell:nullptr,static_cast<float>(ManualRadius()),inside?1:0);
    fc3xr::SetHandSphere();
    Mat4 visual=HandIdentity();HandBasis(g_reloadContact.portParent,visual);visual.m[3]=port[0];visual.m[7]=port[1];visual.m[11]=port[2];float matrix[16],size[3];
    for(int i=0;i<16;++i)matrix[i]=static_cast<float>(visual.m[i]);for(int i=0;i<3;++i)size[i]=static_cast<float>(ManualRadius()*2);
    fc3xr::SetWeaponBox(matrix,size,inside?1:0);
}
void ManualDrop()
{g_reload.held=false;g_reload.mode=0;g_manualReload.contactFrames=0;g_manualReload.lastContact=-1;g_manualReload.wasInside=false;g_reloadContact.hand=false;}
void ManualNativeBegin(UINT state,UINT wrapper)
{
    if(!g_manualReloadEnabled||!InterlockedExchange(&g_manualReload.pending,0))return;
    fc3pad::CancelReloadRequest();ManualDrop();
    g_manualReload.state=state;g_manualReload.wrapper=wrapper;g_manualReload.weapon=ReloadRead(wrapper,0x48);
    g_manualReload.before=ReloadRead(g_manualReload.weapon,0xcc);g_manualReload.committed=false;g_manualReload.result=0;
    g_manualReload.reloadWasEmpty=g_manualReload.before==0;
    g_manualReload.hadChamberedRound=!g_manualReload.reloadWasEmpty&&g_chambered;
    InterlockedExchange(&g_manualReload.cycle,1);
    // Native owner is already verified by SingleShellNativeContext on this thread.
    g_reload.state=state;g_reload.wrapper=wrapper;g_reload.weapon=g_manualReload.weapon;g_reload.ready=1;
    Log("[MANUAL RELOAD] real native start on game thread %lu, loaded %u; one-shell count-store cap owns finish",GetCurrentThreadId(),g_manualReload.before);
}
void ManualNativeTransfer(UINT state,UINT wrapper,UINT before,UINT after)
{
    if(!g_manualReload.cycle||state!=g_manualReload.state||wrapper!=g_manualReload.wrapper||g_manualReload.committed)return;
    if(after==before+1){
        // Filling an empty tube does not chamber its first shell. A partial
        // top-up preserves the current chamber, including an outstanding pump.
        if(g_pumpEnabled&&g_manualReload.reloadWasEmpty&&before==0){g_chambered=false;g_pumpStage=0;g_pumpBaseSet=false;g_pumpFire.passing=false;g_pumpFire.waitRelease=true;fc3pad::BlockFire(true);}
        g_manualReload.committed=true;InterlockedIncrement(&g_reload.commits);fc3xr::Buzz(1);
        Log("[SHOTGUN CHAMBER] insertion %u -> %u; started %s, retained round=%d; chamber now %d, physical pump remains authoritative",before,after,g_manualReload.reloadWasEmpty?"empty":"partial",g_manualReload.hadChamberedRound,g_chambered);}
}
void ManualNativeEnd(UINT state,UINT wrapper)
{
    if(!g_manualReload.cycle||state!=g_manualReload.state||wrapper!=g_manualReload.wrapper)return;
    InterlockedExchange(&g_manualReload.cycle,0);fc3pad::CancelReloadRequest();
    const UINT after=ReloadRead(g_manualReload.weapon,0xcc);
    g_manualReload.result=g_manualReload.committed&&after==g_manualReload.before+1?1:3;
    Log("[MANUAL RELOAD] native end loaded %u -> %u, result %d; consumed shell cannot repeat",g_manualReload.before,after,g_manualReload.result);
}
// Physical insertion is already performed by the player. Complete only the
// native data transaction at its game-thread request seam, without granting a
// second, timed reload animation. Engine callbacks own all ammo/reserve writes.
volatile LONG g_m133Instant251=0;
bool InstantM133Insert251(UINT state,UINT wrapper){
 if(!g_manualReloadEnabled||!M133SupportProfile56()||!g_manualReload.pending||
    GetTickCount64()>g_manualReload.deadline||!g_nativeReloadBegin||!g_nativeReload||!g_nativeReloadEnd||
    !g_nativeReloadCountStore||(ReloadRead(state,8)&255)||!SingleShellNativeContext(state,wrapper))return false;
 if(InterlockedCompareExchange(&g_m133Instant251,1,0))return false;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 bool transfer=false,ended=false;
 __try{ReloadBeginBoundary(state,wrapper);if(g_manualReload.cycle&&g_manualReload.state==state&&g_manualReload.wrapper==wrapper){ReloadBoundary(state,wrapper,1,1);transfer=true;}}
 __except(EXCEPTION_EXECUTE_HANDLER){}
 // Always attempt native cleanup, even if a callback failed. Never fall back
 // to granting a reload timeline after consuming this insertion request.
 __try{ReloadEndBoundary(state,wrapper,0);ended=true;}
 __except(EXCEPTION_EXECUTE_HANDLER){}
 const UINT after=ReloadRead(weapon,0xcc);
 const bool same=ReloadContext(state,wrapper,weapon)&&ReloadRead(wrapper,0x48)==weapon;
 const bool ok=transfer&&ended&&same&&after==before+1&&!(ReloadRead(state,8)&255);
 fc3pad::CancelReloadRequest();InterlockedExchange(&g_m133Instant251,0);
 if(!ok){g_manualReload.result=3;InterlockedExchange(&g_manualReload.pending,0);ManualDrop();}
 Log("[M133 INSERT251] synchronous Begin/one-shell-transfer/End %u -> %u; success%d native%08X; reload animation denied",before,after,ok,ReloadRead(state,8));
 return true;
}
void TickManualReload(bool handling,const fc3pad::Controllers& c)
{
    static bool gripWas=false;const bool grip=c.leftGrip>.7f,gripEdge=grip&&!gripWas;gripWas=fc3pad::GripOnlyAmmoEnabled()?(grip||(c.leftGrip>=.35f&&gripWas)):grip;
    const auto now=GetTickCount64();const bool xEdge=c.x&&!g_manualReload.x;
    const bool chord=!fc3pad::DedicatedInteractionLayout()&&c.leftGrip>.7f&&c.b,chordEdge=chord&&!g_manualReload.chord;
    g_manualReload.x=c.x;g_manualReload.chord=chord;
    const float dt=g_manualReload.tick?static_cast<float>((std::min)(now-g_manualReload.tick,50ull))*.001f:0;
    g_manualReload.tick=now;
    if(!handling||!g_handLeftValid||!g_handRightValid)
    {
        ManualDrop();InterlockedExchange(&g_manualReload.pending,0);fc3pad::CancelReloadRequest();
        g_reloadAlignment=false;fc3pad::SetManualReload(false,false);fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;
    }
    if(chordEdge&&!g_manualReload.pending&&!g_manualReload.cycle&&!ReloadPoseActive())
    {g_reloadAlignment=!g_reloadAlignment;ManualDrop();g_manualReload.result=0;fc3xr::Buzz(1);Log("[MANUAL RELOAD] alignment %s",g_reloadAlignment?"ON":"OFF");}
    fc3pad::SetManualReload(true,g_reloadAlignment);
    if(g_reloadAlignment)
    {
        const float rate=.035f*dt;
        auto axis=[](float v){return std::fabs(v)<.2f?0.f:v;};
        g_manualSettings.port[0]+=axis(c.leftX)*rate;g_manualSettings.port[1]+=axis(c.leftY)*rate;g_manualSettings.port[2]+=axis(c.rightY)*rate;
        for(auto& v:g_manualSettings.port)v=(std::max)(-.5f,(std::min)(.5f,v));
        if(xEdge){const bool saved=SaveManualReloadSettings();g_manualReload.result=saved?4:5;fc3xr::Buzz(saved?1:2);Log("[MANUAL RELOAD] alignment save %s %.6f %.6f %.6f",saved?"OK":"FAILED",g_manualSettings.port[0],g_manualSettings.port[1],g_manualSettings.port[2]);}
        ManualMarker();return;
    }
    if(g_manualReload.pending&&now>g_manualReload.deadline)
    {InterlockedExchange(&g_manualReload.pending,0);fc3pad::CancelReloadRequest();ManualDrop();g_manualReload.result=2;Log("[MANUAL RELOAD] generated request did not start; no ammo changed and no retry");}
    const bool prepare250=ManualCanPrepare250();
    if(g_manualReload.pending||((g_manualReload.cycle||ReloadPoseActive())&&!prepare250)){ManualMarker();return;}
    if((xEdge&&c.leftGrip>.7f)||(fc3pad::GripOnlyAmmoEnabled()&&gripEdge&&!g_twoHand&&g_handLeftValid&&g_handRightValid&&!M133SupportNear56()))
    {
        if(g_reload.installed&&ReloadShellReady()&&!g_reload.held)
        {ManualDrop();g_reload.held=true;g_reload.mode=1;g_twoHand=false;g_manualReload.result=0;fc3xr::Buzz(1);Log("[MANUAL RELOAD] grip+X pickup; finger pose/shell held, no reload or ammo request");}
        else Log("[MANUAL RELOAD] pickup unavailable hook=%d shell=%d held=%d",g_reload.installed,ReloadShellReady(),g_reload.held);
    }
    if(!g_reload.held){ManualMarker();return;}
    if(c.leftGrip<.35f){ManualDrop();Log("[MANUAL RELOAD] dropped shell without insertion; no ammo request");ManualMarker();return;}
    double distance=0;const bool inside=ManualContact(distance)&&distance<=ManualRadius();
    if(!inside){g_manualReload.contactFrames=0;g_manualReload.lastContact=-1;g_manualReload.wasInside=false;ManualMarker();return;}
    if(!g_manualReload.wasInside){g_manualReload.wasInside=true;fc3xr::Buzz(1);Log("[MANUAL RELOAD] shell contact %.4f m radius %.4f frame %ld",distance,ManualRadius(),g_frame);}
    if(g_manualReload.lastContact!=g_frame)
    {
        g_manualReload.lastContact=g_frame;
        if(++g_manualReload.contactFrames>=2)
        {
            if(prepare250){g_manualReload.contactFrames=2;ManualMarker();return;} // contact waits for the previous native End
            g_manualReload.deadline=now+3000;InterlockedExchange(&g_manualReload.pending,1);InterlockedIncrement(&g_reload.requests);
            ManualDrop();fc3pad::RequestReload();
            Log("[MANUAL RELOAD] ONE insertion request; distance %.4f; waits for ordinary native start/count/transfer/end",distance);
        }
    }
    ManualMarker();
}

bool ManualReloadNote()
{
    if(!g_manualReloadEnabled)return false;
    wchar_t note[512]={};
    if(g_reloadAlignment)
    {
        _snwprintf_s(note,512,_TRUNCATE,L"ALIGN LOADING RING%s | X SAVES\nLeft stick: X/Y. Right stick up/down: Z.\nPort %.3f / %.3f / %.3f",g_manualReload.result==4?L" - SAVED":g_manualReload.result==5?L" - SAVE FAILED":L"",g_manualSettings.port[0],g_manualSettings.port[1],g_manualSettings.port[2]);
    }
    else if(!g_reload.installed)wcscpy_s(note,L"RELOAD HOOK WAITING\nKeep the shotgun equipped.\nShell pickup waits for the hook.");
    else if(g_manualReload.cycle&&!ManualCanPrepare250())wcscpy_s(note,L"INSERTING ONE SHELL\nLeft hand stays under your control.\nWait for this shell to finish loading.");
    else if(g_manualReload.pending)wcscpy_s(note,L"SHELL ACCEPTED - WAITING FOR GAME\nOne reload request sent.\nNo repeated requests while holding still.");
    else if(g_reload.held)
    {
        double distance=0;
        if(ManualContact(distance))_snwprintf_s(note,512,_TRUNCATE,L"SHELL HELD - MOVE INTO BLUE RING\nContact dot %.1f cm from loading port.\nLoading radius %.1f cm. Keep LEFT grip held.",distance*100/(std::max)(.1f,g_worldScale),g_manualSettings.radius*100);
        else wcscpy_s(note,L"SHELL HELD - TRACKING CONTACT\nKeep the gun and shell visible.\nMove the shell under the receiver.");
    }
    else if(g_manualReload.result==2)wcscpy_s(note,L"GAME DID NOT START RELOAD\nNo ammo was added.\nCheck if the shotgun is full or reserve is empty.");
    else if(g_manualReload.result==3)wcscpy_s(note,L"NATIVE CYCLE ENDED WITHOUT A SHELL\nNo automatic retry.\nCheck loaded and reserve ammo.");
    else if(!ReloadShellReady())wcscpy_s(note,L"WAITING FOR SHELL VISUAL\nKeep the shotgun equipped.\nNo setup reload is required.");
    else _snwprintf_s(note,512,_TRUNCATE,L"%s - %ld INSERTED\nHold LEFT grip + tap X to take ONE shell.\nKeep grip held; move shell into loading ring.",g_manualReload.result==1?L"ONE SHELL CONFIRMED":L"MANUAL RELOAD READY",g_reload.commits);
    fc3xr::SetTestNote(note);return true;
}
