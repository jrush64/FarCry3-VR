void TestManualInsertion(UINT state,UINT wrapper,UINT weapon,UINT hud,ID3D11Device* device,ID3D11DeviceContext* context)
{
    const bool priorManual=g_manualReloadEnabled,priorChamber=g_chambered;const auto priorSettings=g_manualSettings;g_chambered=false;
    const auto priorShell=g_reloadShell;g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=true;
    g_manualReloadEnabled=true;g_reloadAlignment=false;g_manualSettings=ManualReloadSettings{};g_singleShellEnabled=true;
    g_manualReload=ManualReloadState{};g_reload=PhysicalReloadState{};g_reloadCountTrace=ReloadCountTrace{};g_reloadPose=ReloadPoseCycle{};
    g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_handLeftValid=g_handRightValid=true;g_worldScale=1;
    g_singleFixtureCapacity=8;*reinterpret_cast<UINT*>(weapon+0xcc)=2;
    *reinterpret_cast<UINT*>(hud+0x134)=2;*reinterpret_cast<UINT*>(hud+0x138)=8;*reinterpret_cast<UINT*>(hud+0x140)=992;*reinterpret_cast<UINT*>(hud+0x144)=994;
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);TickPhysicalReload(true);
    c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(fc3pad::ReloadOutput()&&!g_reload.held&&!g_manualReload.pending,"manual X reaches interaction without spawning shell or insertion request");
    TickPhysicalReload(false);fc3pad::Publish(c);
    Check(fc3pad::ReloadOutput(),"interaction X stays available through temporary handling loss");
    TickPhysicalReload(true);
    c.x=false;c.leftGrip=1;fc3pad::Publish(c);TickPhysicalReload(true);
    c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(g_reload.held&&g_reload.mode&&!g_manualReload.pending&&!fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==2,"grip+X pickup displays one shell without native reload/ammo");
    c.x=false;fc3pad::Publish(c);
    g_reloadContact=ReloadHandContact{};g_reloadContact.hand=g_reloadContact.port=true;
    g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;g_reloadContact.portBody={.3,-.15,.4};g_reloadContact.shellBody={.5,-.15,.4};
    for(int i=0;i<10;++i)TickPhysicalReload(true);
    Check(!g_manualReload.pending&&g_reload.held&&!fc3pad::ReloadOutput(),"distant shell and stationary grip never request reload");
    g_reloadContact.shellBody=g_reloadContact.portBody;g_reloadContact.handFrame=g_frame-1;TickPhysicalReload(true);
    Check(!g_manualReload.pending&&g_manualReload.contactFrames==0,"mixed-frame hand/port samples cannot insert");
    g_reloadContact.handFrame=g_frame;TickPhysicalReload(true);TickPhysicalReload(true);
    Check(g_manualReload.contactFrames==1&&!g_manualReload.pending,"repeated calls in one frame cannot duplicate contact");
    ++g_frame;g_reload.shotgunFrame=g_frame;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;TickPhysicalReload(true);
    Check(g_manualReload.pending&&!g_reload.held&&g_reload.requests==1&&fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==2,"two fresh visual contacts consume shell and send exactly one request, no early ammo");
    for(int i=0;i<30;++i)TickPhysicalReload(true);Check(g_reload.requests==1,"pending insertion never automatically retries");
    ReloadBeginBoundary(state,wrapper);
    Check(g_manualReload.cycle&&!g_manualReload.pending&&ReloadRead(g_traceFixtureProperty,8)==1&&g_traceFixtureStackCount==1&&!fc3pad::ReloadOutput(),"physical request claims real native Begin and validated ECX/stack cap, then releases X");
    c.x=true;c.leftTrigger=1;fc3pad::Publish(c);TickPhysicalReload(true);c.leftGrip=0;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(!g_reload.held&&!g_manualReload.pending&&g_manualReload.cycle&&ReloadPoseActive(),"grip/back trigger/X cannot pick up or restart an active native cycle");
    ReloadBoundary(state,wrapper,0,-1);ReloadBoundary(state,wrapper,0,-1);FixtureSingleAdvance(state,wrapper);
    Check(ReloadRead(weapon,0xcc)==3&&ReloadRead(hud,0x140)==991&&g_reload.commits==1&&g_manualReload.result==1&&!g_manualReload.cycle&&!ReloadPoseActive(),"physical insertion adds exactly one shell/reserve minus one, native End completes");
    Check(!g_chambered,"physical shell insertion/native End cannot bypass a required pump");
    {
      const auto savedCycle=g_manualReload;const auto savedPump=g_pumpFire;const int savedStage=g_pumpStage;const bool savedBase=g_pumpBaseSet,savedChamber=g_chambered,savedEnabled=g_pumpEnabled;const auto savedCommits=g_reload.commits;
      for(UINT loaded:{0u,2u}){
        g_manualReload=ManualReloadState{};g_manualReload.pending=1;g_pumpEnabled=true;g_chambered=true;
        *reinterpret_cast<UINT*>(weapon+0xcc)=loaded;ManualNativeBegin(state,wrapper);ManualNativeTransfer(state,wrapper,loaded,loaded+1);
        Check(g_manualReload.reloadWasEmpty==(loaded==0)&&g_chambered==(loaded!=0),"shotgun empty insertion requires pump; partial top-up retains existing chamber");
        *reinterpret_cast<UINT*>(weapon+0xcc)=loaded+1;ManualNativeEnd(state,wrapper);Check(g_chambered==(loaded!=0),"shotgun native end never manufactures a chambered shell");
      }
      *reinterpret_cast<UINT*>(weapon+0xcc)=3;g_manualReload=savedCycle;g_pumpFire=savedPump;g_pumpStage=savedStage;g_pumpBaseSet=savedBase;g_chambered=savedChamber;g_pumpEnabled=savedEnabled;g_reload.commits=savedCommits;
      std::puts("PASS: explicit shotgun empty/partial reload preserves chamber and never bypasses required physical pump");
    }
    c.leftGrip=1;fc3pad::Publish(c);for(int i=0;i<100;++i)TickPhysicalReload(true);
    Check(!g_reload.held&&!g_manualReload.pending&&!fc3pad::ReloadOutput()&&g_reload.requests==1,"holding X/grip and staying at receiver after End cannot repeat pickup/reload");
    c.x=false;fc3pad::Publish(c);TickPhysicalReload(true);c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(g_reload.held,"fresh pickup allowed after completed insertion");
    c.x=false;c.leftGrip=0;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(!g_reload.held&&!g_manualReload.pending&&ReloadRead(weapon,0xcc)==3,"release away from insertion drops shell without ammo");
    // Timeout and tracking/weapon loss cancel the generated pulse, never mutate ammo.
    g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()-1;fc3pad::RequestReload();TickPhysicalReload(true);
    Check(!g_manualReload.pending&&!fc3pad::ReloadOutput()&&g_manualReload.result==2,"no native start cancels request once without retries");
    g_reload.held=true;g_reload.mode=1;g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+3000;fc3pad::RequestReload();TickPhysicalReload(false);
    Check(!g_reload.held&&!g_manualReload.pending&&!fc3pad::ReloadOutput(),"leaving gameplay cancels stale physical shell/pulse");
    c={};c.active=true;fc3pad::Publish(c);fc3pad::SetDedicatedInteractions(true);fc3pad::SetGameplayControls(true);
    c.leftGrip=0;c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(!g_reloadAlignment&&!g_reload.held&&fc3pad::ReloadOutput(),"plain X interacts without calibration, shell pickup or reload ownership");
    c.x=false;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);
    // Alignment keeps raw stick inputs, suppresses game movement/look/fire/reload.
    c={};c.active=true;fc3pad::Publish(c);TickPhysicalReload(true);c.leftGrip=1;c.b=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(g_reloadAlignment,"grip+B enters loading-port alignment");
    c.leftX=1;c.leftY=-1;c.rightY=1;c.rightTrigger=1;c.x=true;fc3pad::Publish(c);
    Check(!fc3pad::ReloadOutput()&&!fc3pad::FireOutput()&&(fc3pad::ButtonsOutput()&0x2000)==0,"alignment X/save and B/chord cannot reload/crouch/fire");
    // Replay an actual captured receiver with a changed local anchor.
    const auto& nativePort=kNativePortFixture;
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    globals->SetPrivateData(kCbCopyGuid,sizeof(nativePort.globals),nativePort.globals);const auto summary=FixtureHandCamera(nativePort);
    camera->SetPrivateData(kCameraSummaryGuid,sizeof(summary),&summary);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
    g_manualSettings.port[0]=.0123f;g_manualSettings.port[1]=-.044f;g_manualSettings.port[2]=.081f;FixtureBodyTurn(nativePort);
    float gunBones[576]={};memcpy(gunBones,nativePort.bones,sizeof(nativePort.bones));RecordReloadPort(context,gunBones,16428,7311);
    Mat4 inverse={};Check(Invert(ArmBodyToCamera(),inverse),"captured port body frame");
    const Mat4 root=Multiply(HullView(summary),Multiply(HullWorld(nativePort.globals),BoneMatrix(gunBones,0)));
    const V3 expected=Apply(inverse,Apply(root,{.0123,-.044,.081}));
    Check(Length(Sub(expected,g_reloadContact.portBody))<.0003,"saved local loading point follows the actual rotated rendered receiver");
    context->ClearState();globals->Release();camera->Release();g_aimActive=false;
    // Saved values use the same gun-local anchor as contact and preserve other INI sections.
    wchar_t tmp[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,tmp);GetTempFileNameW(tmp,L"f3r",0,path);
    WritePrivateProfileStringW(L"Stereo",L"WorldScale",L"1.25",path);
    g_manualSettings.port[0]=.0123f;g_manualSettings.port[1]=-.044f;g_manualSettings.port[2]=.081f;
    Check(SaveManualReloadSettings(path)&&std::fabs(ManualReadSetting(path,L"PortX",0)-.0123f)<1e-6&&ManualReadSetting(path,L"Mode",9)==0,"loading-port position and mode persist to INI");
    wchar_t text[32];GetPrivateProfileStringW(L"Stereo",L"WorldScale",L"",text,32,path);Check(wcscmp(text,L"1.25")==0,"reload save preserves stereo settings");
    WritePrivateProfileStringW(L"Reload",L"Radius",L"nan",path);Check(ManualReadSetting(path,L"Radius",.065f)==.065f,"nonfinite calibration setting rejected");DeleteFileW(path);
    g_manualSettings.mode=2;ApplyManualReloadMode();
    Check(!g_manualReloadEnabled&&!g_singleShellEnabled&&!g_physicalReloadEnabled,"configured bulk fallback disables manual interaction/count cap without old gate");
    g_manualSettings.mode=1;ApplyManualReloadMode();Check(!g_manualReloadEnabled&&g_singleShellEnabled,"configured one-shell X fallback retains accepted native cycle");
    g_manualSettings.mode=0;ApplyManualReloadMode();Check(g_manualReloadEnabled&&g_singleShellEnabled,"configured physical mode uses same single-shell cap");
    // Regular and signature share the same ready-after-transfer behavior.
    {const auto savedProfile250=g_weaponMechanicProfile;const auto savedContact250=g_reloadContact;
     for(int profile250:{3,6}){
      g_weaponMechanicProfile=profile250;g_weaponMechanicFrame=g_frame;g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_manualReload=ManualReloadState{};g_manualReload.pending=1;g_reloadPose={};g_reloadCountTrace={};g_reloadAlignment=false;
      *reinterpret_cast<UINT*>(weapon+0xcc)=2;*reinterpret_cast<UINT*>(hud+0x134)=2;*reinterpret_cast<UINT*>(hud+0x138)=8;*reinterpret_cast<UINT*>(hud+0x140)=20;*reinterpret_cast<UINT*>(hud+0x144)=22;
      c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);ReloadBeginBoundary(state,wrapper);Check(!ManualCanPrepare250(),"250 next shell blocked before native transfer");
      ReloadBoundary(state,wrapper,0,-1);Check(ManualCanPrepare250()&&g_manualReload.cycle,"250 regular/signature can prepare next shell immediately after verified +1");
      g_reloadContact.hand=g_reloadContact.port=true;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;g_reloadContact.portBody={.3,-.15,.4};g_reloadContact.shellBody={.6,-.15,.4};
      TickManualReload(true,c);c.x=true;fc3pad::Publish(c);TickManualReload(true,c);Check(g_reload.held&&!g_manualReload.pending&&ReloadRead(weapon,0xcc)==3,"250 next shell pickup does not wait for native closing sounds or mutate ammo");
      c.x=false;fc3pad::Publish(c);g_reloadContact.hand=true;g_reloadContact.shellBody=g_reloadContact.portBody;
      for(int f=0;f<3;++f){++g_frame;g_reload.shotgunFrame=g_frame;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;TickManualReload(true,c);}
      Check(g_reload.held&&!g_manualReload.pending&&!fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==3,"250 next contact cannot overlap existing native reload or add duplicate shell");
      FixtureSingleAdvance(state,wrapper);Check(!g_manualReload.cycle&&g_reload.held,"250 original native end preserves already prepared shell");
      ++g_frame;g_reload.shotgunFrame=g_frame;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;TickManualReload(true,c);Check(g_manualReload.pending&&!g_reload.held,"250 waiting contact issues one request when native end permits it");
      ReloadBeginBoundary(state,wrapper);ReloadBoundary(state,wrapper,0,-1);ReloadBoundary(state,wrapper,0,-1);FixtureSingleAdvance(state,wrapper);Check(ReloadRead(weapon,0xcc)==4&&ReloadRead(hud,0x140)==18,"250 two physical insertions produce exactly two native shells and reserve debits");
     }
     g_weaponMechanicProfile=savedProfile250;g_reloadContact=savedContact250;
    }
    // Actual request gate -> typed native Begin/count-store/transfer/End.
    // Twelve shells can be inserted without any animation-advance call or delay.
    {const auto savedProfile251=g_weaponMechanicProfile;const auto savedFrame251=g_weaponMechanicFrame;
     const bool gate251=g_reloadSignal.enabled;g_reloadSignal.enabled=false; // optional empty-reload toggle is unrelated
     UINT signal251=0xaf41179b;
     for(int profile251:{3,6}){
      g_weaponMechanicProfile=profile251;g_weaponMechanicFrame=g_frame;g_reload.shotgunFrame=g_frame;g_singleFixtureCapacity=12;
      g_manualReload={};g_reloadPose={};g_reloadCountTrace={};*reinterpret_cast<WORD*>(state+8)=0;
      *reinterpret_cast<UINT*>(weapon+0xcc)=0;*reinterpret_cast<UINT*>(hud+0x134)=0;*reinterpret_cast<UINT*>(hud+0x138)=12;*reinterpret_cast<UINT*>(hud+0x140)=20;*reinterpret_cast<UINT*>(hud+0x144)=20;g_chambered=false;g_pumpEnabled=true;
      for(UINT shell251=0;shell251<12;++shell251){
       g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+3000;fc3pad::RequestReload();
       Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal251)),"251 physical shell denies timed native reload signal");
       Check(ReloadRead(weapon,0xcc)==shell251+1&&ReloadRead(hud,0x140)==19-shell251,"251 request gate commits exactly one native shell/reserve debit");
       Check(!g_manualReload.pending&&!g_manualReload.cycle&&!ReloadPoseActive()&&!g_reloadCountTrace.cycle&&!(ReloadRead(state,8)&255),"251 next insertion immediately ready without native animation advance");
       Check(!g_chambered&&!g_pumpFire.passing&&!fc3pad::ReloadOutput(),"251 instant shell completion cannot bypass pump or retain X pulse");
       Check(!InstantM133Insert251(state,wrapper)&&ReloadRead(weapon,0xcc)==shell251+1,"251 consumed request cannot repeat while stationary");
      }
      g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+3000;
      Check(InstantM133Insert251(state,wrapper)&&ReloadRead(weapon,0xcc)==12&&ReloadRead(hud,0x140)==8,"251 full tube cannot debit reserve or overfill");
      *reinterpret_cast<UINT*>(weapon+0xcc)=2;*reinterpret_cast<UINT*>(hud+0x134)=2;g_chambered=true;
      g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+3000;
      Check(InstantM133Insert251(state,wrapper)&&ReloadRead(weapon,0xcc)==3&&ReloadRead(hud,0x140)==7&&g_chambered,"251 partial top-up retains existing chambered round");
      g_chambered=false;g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()+3000;
      Check(InstantM133Insert251(state,wrapper)&&ReloadRead(weapon,0xcc)==4&&!g_chambered,"251 partial top-up cannot clear an outstanding pump");
      g_manualReload.pending=1;g_manualReload.deadline=GetTickCount64()-1;Check(!InstantM133Insert251(state,wrapper),"251 expired insertion cannot execute callbacks");g_manualReload.pending=0;
     }
     g_reloadSignal.enabled=gate251;g_weaponMechanicProfile=savedProfile251;g_weaponMechanicFrame=savedFrame251;
    }
    g_manualReloadEnabled=priorManual;g_chambered=priorChamber;g_manualSettings=priorSettings;g_reloadAlignment=false;g_manualReload=ManualReloadState{};
    g_reloadShell=priorShell;g_reload=PhysicalReloadState{};g_reloadContact=ReloadHandContact{};g_reloadCountTrace=ReloadCountTrace{};g_reloadPose=ReloadPoseCycle{};
    fc3pad::SetManualReload(false,false);fc3pad::CancelReloadRequest();fc3pad::Publish(fc3pad::Controllers{});
    std::puts("PASS: manual pickup/contact through real native store hook: 2/8 ->3/8, reserve992->991, End; no early ammo/loops/stale contact, drop/timeout/scope loss, calibration input and persisted values");
}
