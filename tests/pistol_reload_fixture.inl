#include "pistol_reload_data.inl"
void __stdcall PistolFixtureBegin(UINT state,UINT){*reinterpret_cast<UINT*>(state+8)=1;}
void __stdcall PistolFixtureEnd(UINT state,UINT,UINT){*reinterpret_cast<UINT*>(state+8)=0;}
UINT g_pistolFixtureHud=0;
void __stdcall PistolFixtureTransfer(UINT,UINT wrapper,UINT,int)
{
    UINT weapon=ReloadRead(wrapper,0x48);const UINT before=ReloadRead(weapon,0xcc);
    if(before>=8)return;
    *reinterpret_cast<UINT*>(weapon+0xcc)=8;
    *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=8;
    *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)-=8-before;
    *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x144)=8+ReloadRead(g_pistolFixtureHud,0x140);
}
HRESULT STDMETHODCALLTYPE PistolFixtureCreate(ID3D11Device* d,const D3D11_BUFFER_DESC* b,const D3D11_SUBRESOURCE_DATA* s,ID3D11Buffer** o)
{return d->CreateBuffer(b,s,o);}
void TestPistolReload(ID3D11Device* device,ID3D11DeviceContext* context)
{
    const auto prior=g_pistolReload;const LONG priorProfile=g_weaponMechanicProfile,priorFrame=g_weaponMechanicFrame;
    g_pistolReload=PistolReloadState{};g_pistolReload.enabled=true;
    g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;
    g_weaponMechanicProfile=20;g_weaponMechanicFrame=++g_frame;
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
    g_pistolReload.basis=true;g_pistolReload.idle=g_pistolReload.current=HandIdentity();g_pistolReload.basisFrame=g_frame;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"pistol native owner fixture");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    *reinterpret_cast<UINT*>(weapon+0xcc)=3;*reinterpret_cast<UINT*>(hud+0x134)=3;*reinterpret_cast<UINT*>(hud+0x138)=8;
    *reinterpret_cast<UINT*>(hud+0x140)=100;*reinterpret_cast<UINT*>(hud+0x144)=103;
    g_reload=PhysicalReloadState{};g_reload.installed=true;ObserveAmmoWidget(wrapper,hud);g_pistolFixtureHud=hud;
    const auto priorGate=g_reloadSignal;const auto priorAuthority=g_singleReloadAuthority;const auto priorManager=g_singleReloadManager;
    const UINT reference=actor+20480,managerAddress=actor+24576;
    *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(managerAddress)=actor;
    g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_reloadSignal.manualPistol=true;
    g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=managerAddress;
    g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=PistolFixtureTransfer;
    UINT signalHash=0xaf41179b;
    c.x=true;fc3pad::Publish(c);TickPistolReload(true);
    Check(g_pistolReload.magazinePresent&&!g_pistolReload.held&&!g_pistolReload.pending&&fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==3,"plain interaction X cannot eject, pick up or alter pistol ammo");
    c.x=false;fc3pad::Publish(c);TickPistolReload(true);
    c.x=true;c.leftGrip=1;c.rightTrigger=1;fc3pad::Publish(c);TickPistolReload(true);fc3pad::Publish(c);
    Check(!g_pistolReload.magazinePresent&&!g_pistolReload.held&&ReloadRead(weapon,0xcc)==3,"X ejects without native reload or ammo write");
    Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"ordinary X reload denied before insertion");
    Check(!fc3pad::FireOutput()&&!fc3pad::ReloadOutput(),"release chord blocks fire and cannot also interact");
    c.x=false;c.rightGrip=0;fc3pad::Publish(c);TickPistolReload(true);
    c.leftGrip=1;c.x=true;c.rightTrigger=1;fc3pad::Publish(c);TickPistolReload(true);fc3pad::Publish(c);
    Check(g_pistolReload.held&&!fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==3,"grip X pickup cannot reload or alter ammo");
    Check(!fc3pad::FireOutput(),"held magazine blocks pistol fire");
    c.x=false;fc3pad::Publish(c);
    auto& p=g_pistolReload;p.tip={0,0,0};p.port={1,0,0};p.handFrame=p.portFrame=g_frame;
    TickPistolReload(true);Check(p.held&&!p.pending,"outside port cannot request reload");
    p.port=p.tip;TickPistolReload(true);TickPistolReload(true);Check(p.held&&!p.pending,"same frame cannot double count contact");
    ++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);fc3pad::Publish(c);
    Check(p.pending&&!p.held&&fc3pad::ReloadOutput(),"two fresh contact frames send one generated reload pulse");
    TickPhysicalReload(true);fc3pad::Publish(c);Check(fc3pad::ReloadOutput(),"inactive shotgun cleanup preserves magazine request");
    PistolNativeBegin(state,wrapper+4);Check(p.pending&&!p.cycle,"wrong native owner cannot consume request");
    Check(!PistolContactReady(state,wrapper+4),"wrong wrapper cannot authorize physical contact");
    Check(!RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"physical insertion authorizes native reload signal");
    Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"duplicate reload signal cannot spend same insertion twice");
    ReloadBeginBoundary(state,wrapper);Check(p.cycle&&!p.pending&&!fc3pad::ReloadOutput(),"actual native begin consumes and cancels input pulse");
    Check(p.reloadChamberTracked&&!p.reloadWasEmpty&&!p.chamberReady&&!p.awaitingNativeClose,"partial physical reload snapshots count and waits for manual chambering");
    ReloadBoundary(state,wrapper,0,-1);Check(p.committed&&p.commits==1&&ReloadRead(weapon,0xcc)==8&&ReloadRead(hud,0x140)==95,"native magazine transfer confirms real loaded and reserve change");
    Check(!p.needsRack&&PistolBusy(),"1911 transfer requires no manual rack but fire stays blocked through native reload");
    Check(!p.chamberReady&&!p.awaitingNativeClose,"partial physical transfer waits for manual chambering");
    ReloadBoundary(state,wrapper,0,-1);Check(p.commits==1,"duplicate native completion cannot credit second magazine");
    *reinterpret_cast<UINT*>(hud+0x140)=0;*reinterpret_cast<UINT*>(hud+0x144)=8;
    Check(PistolLocal(state,wrapper),"native ownership remains valid when insertion consumes last reserve ammo");
    c.rightTrigger=1;fc3pad::Publish(c);
    ReloadEndBoundary(state,wrapper,0);Check(!p.cycle&&p.result==1&&p.needsRack&&!p.emptyLocked&&p.waitTriggerRelease,"native end preserves mandatory rack and held-trigger lock");
    c.rightTrigger=0;AdvancePistolRack(c,GetTickCount64());Check(PistolBusy(),"trigger release alone cannot bypass manual rack");Complete1911RackFixture50();
    fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"1911 next trigger pull fires after verified manual rack");
    std::puts("PASS: 1911 physical reload requires verified grip-only manual rack; held trigger and duplicate native ammo transfer guarded");
    *reinterpret_cast<UINT*>(weapon+0xcc)=0;*reinterpret_cast<UINT*>(hud+0x134)=0;*reinterpret_cast<UINT*>(hud+0x140)=20;*reinterpret_cast<UINT*>(hud+0x144)=20;
    p.pending=p.cycle=false;p.entryAccepted=false;p.magazinePresent=false;p.emptyLocked=true;c.rightTrigger=0;fc3pad::Publish(c);
    Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"empty 1911 still cannot start automatic reload without physical insertion");
    p.pending=true;p.entryAccepted=true;p.state=state;p.wrapper=wrapper;p.deadline=GetTickCount64()+3000;
    ReloadBeginBoundary(state,wrapper);ReloadBoundary(state,wrapper,0,-1);
    Check(p.reloadWasEmpty&&!p.chamberReady&&p.awaitingNativeClose,"empty reload remains explicitly unchambered after native magazine transfer");
    Check(p.cycle&&p.committed&&!p.needsRack&&PistolBusy()&&ReloadRead(weapon,0xcc)==8&&ReloadRead(hud,0x140)==12,"empty physical reload uses native ammo transfer and waits for automatic slide completion");
    PistolNativeEnd(state,wrapper+4);Check(!p.chamberReady&&p.awaitingNativeClose,"wrong native owner cannot chamber the empty reload");
    ReloadEndBoundary(state,wrapper,0);Check(PistolBusy()&&p.needsRack&&!p.slideClosing,"native end requires manual slide stroke");Complete1911RackFixture50();Check(!PistolBusy()&&p.chamberReady,"empty reload chambers through verified manual slide stroke");
    std::puts("PASS: explicit 1911 partial/empty chamber states snapshot native count; both require manual slide completion");
    std::puts("PASS: both partial and empty 1911 physical reloads require grip-only slide pull; automatic empty reload remains denied");
    // Reproduce first-full pistol insertion with no cached owner: native predicate
    // never emits its reload signal, so only the pre-predicate observer can recover.
    const auto fullOwnerState=p;
    p.state=p.wrapper=p.weapon=0;p.pending=true;p.entryAccepted=false;p.cycle=false;p.magazinePresent=false;p.held=false;p.deadline=GetTickCount64()+3000;
    ObserveMagazineReloadRequest(state,wrapper);
    Check(p.magazinePresent&&!p.pending&&!p.held&&p.state==state&&p.needsRack&&PistolBusy(),"first full insertion discovers exact owner and retains manual rack requirement");
    Check(ReloadRead(weapon,0xcc)==8,"first-full pistol recovery cannot fabricate ammo");
    p=fullOwnerState;
    // A verified already-full magazine cannot trigger the native reload predicate.
    const UINT reserveBefore=ReloadRead(hud,0x140),commitsBefore=p.commits;
    p.magazinePresent=false;p.held=true;p.rackMode=p.rackHeld=false;fc3pad::CancelReloadRequest();
    Check(PistolReseatFull()&&p.magazinePresent&&!p.held&&!p.pending&&p.needsRack&&PistolBusy()&&!fc3pad::ReloadOutput(),"full native magazine reseats without ammo request but requires manual rack");
    Check(ReloadRead(weapon,0xcc)==8&&ReloadRead(hud,0x140)==reserveBefore&&p.commits==commitsBefore,"reseating full magazine cannot create ammo or consume reserve");
    Complete1911RackFixture50();const UINT goodWrapper=p.wrapper;p.wrapper+=4;Check(!PistolReseatFull(),"foreign native owner cannot unblock full magazine");p.wrapper=goodWrapper;
    *reinterpret_cast<UINT*>(hud+0x138)=9;Check(!PistolReseatFull(),"capacity mismatch cannot fabricate full-magazine confirmation");*reinterpret_cast<UINT*>(hud+0x138)=8;
    std::puts("PASS: full native magazine reseats with unchanged loaded/reserve counts, no native reload pulse and verified local owner/capacity");
    // Actual solid-piece GPU path, using a live native magazine draw.
    context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
    ID3D11Buffer* vb=Buffer(device,177904,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,32682,D3D11_BIND_INDEX_BUFFER);
    ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
    UINT stride=32,offset=240;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
    globals->SetPrivateData(kCbCopyGuid,sizeof(kPistolMagGlobals),kPistolMagGlobals);
    CameraSummary cam={};memcpy(cam.right,kPistolCamera,12);memcpy(cam.up,kPistolCamera+3,12);memcpy(cam.forward,kPistolCamera+6,12);memcpy(cam.position,kPistolCamera+9,12);
    cam.xScale=kPistolCamera[12];cam.yScale=kPistolCamera[13];cam.zScale=kPistolCamera[14];cam.zOffset=kPistolCamera[15];cam.mainView=1;
    camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
    p.held=true;p.handFrame=g_frame;p.hand=Multiply(HullView(cam),Multiply(HullWorld(kPistolLeftGlobals),Multiply(BoneMatrix(kPistolLeftRaw,2),kPistolMagazineFromHand)));
    p.hand.m[3]+=.12;p.hand.m[7]+=.05;g_frameGunValid=true;g_frameGunMove=HandIdentity();
    auto* result=BuildWeaponWorld(context,8664,1338);Check(result!=nullptr,"actual magazine globals GPU override");
    float actual[88]={};Read(device,context,result,actual);
    const Mat4 rendered=Multiply(HullView(cam),HullWorld(actual));
    for(int i=0;i<16;++i)Check(std::fabs(rendered.m[i]-p.hand.m[i])<.0002,"live magazine geometry follows completed left hand instead of right hand");
    float source[88]={};ReadCopy(globals,source,sizeof(source));Compare(source,kPistolMagGlobals,88,"source native world globals untouched");
    p.held=false;p.cycle=true;result=BuildWeaponWorld(context,8664,1338);Read(device,context,result,actual);
    Check(std::fabs(actual[7])>100,"native insertion magazine hidden during cycle");
    // Correction preserves the wrist pivot and changes no native slide transform.
    p.current=HandIdentity();p.current.m[0]=p.current.m[5]=0;p.current.m[1]=-1;p.current.m[4]=1;p.idle=HandIdentity();p.basis=true;p.basisFrame=g_frame;
    ArmJoints j={{0,0,0},{0,0,.2},{.1,.2,.4},true};Mat4 move=HandIdentity();CorrectPistolHandMove(j,move);
    const V3 wrist={.1,.2,.4};Check(Length(Sub(Apply(move,wrist),wrist))<1e-6,"pistol reload correction rotates about existing wrist");
    Check(HandAngle(Multiply(move,p.current),p.idle)<.001,"native reload wrist rotation removed");
    p.cycle=false;p.pending=true;p.entryAccepted=false;p.requestAt=GetTickCount64()-251;p.deadline=GetTickCount64()+3000;
    fc3pad::CancelReloadRequest();TickPistolReload(true);Check(p.pending&&fc3pad::ReloadOutput(),"pending physical contact reissues a missed bounded X pulse without another insertion");
    p.deadline=GetTickCount64()-1;TickPistolReload(true);Check(!p.pending&&p.held&&p.result==2&&p.insertionRetryBlocked,"failed native start returns the same magazine to the hand instead of losing it");
    p.held=true;g_weaponMechanicProfile=8;TickPistolReload(true);Check(!p.held&&!p.cycle,"weapon switch cancels magazine ownership");
    g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;p.held=true;c.active=false;fc3pad::Publish(c);TickPistolReload(true);Check(p.held&&!p.pending&&!p.cycle&&p.handFrame<0,"tracking loss preserves held magazine and invalidates actionable contact without a native request");
    {
      const auto saved=p;const auto oldControls=c;const bool mode=fc3pad::GripOnlyAmmoEnabled();
      fc3pad::SetGripOnlyAmmo(true);p=saved;p.magazinePresent=false;p.pending=p.cycle=p.held=p.looseFlying=p.removalHeld=p.rackHeld=p.rackReturning=false;
      p.basis=true;p.nextAmmoAt=0;p.gripWas=false;p.palmFrame=p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootBody=HandIdentity();p.tip={8,8,8};
      const bool supported=g_twoHand;g_twoHand=false;c.active=true;c.x=false;c.leftGrip=1;
      TickPistolLoose(c,GetTickCount64());Check(p.held,"grip-only replacement magazine acquired without X away from mechanisms");
      const UINT identity=p.looseId;TickPistolLoose(c,GetTickCount64());Check(p.held&&p.looseId==identity,"held grip cannot create a second magazine");
      p=saved;g_twoHand=supported;c=oldControls;fc3pad::SetGripOnlyAmmo(mode);
      std::puts("PASS: offline28 real magazine pickup needs no X, preserves loose identity and does not repeat while held");
    }
    g_reloadSignal=priorGate;g_singleReloadAuthority=priorAuthority;g_singleReloadManager=priorManager;
    g_pistolReload=prior;g_weaponMechanicProfile=priorProfile;g_weaponMechanicFrame=priorFrame;fc3pad::SetMagazineHandling(false);fc3pad::CancelReloadRequest();
    g_reload=PhysicalReloadState{};g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
    context->ClearState();globals->Release();camera->Release();vb->Release();ib->Release();VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: 1911 X eject without ammo write; raw X reload denied; physical contact grants exactly one reload signal; local native ammo transfer, duplicate rejection, live solid magazine GPU attachment, source preservation, wrist rotation, switch/timeout cancellation and tracking-loss hold preservation.");
}
