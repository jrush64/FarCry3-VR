#include "ak_live_data.inl"
void TestAk(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const auto oldState=g_magazineStates[2];const auto oldGuide=g_magazineGuides[2];const auto oldState3=g_magazineStates[3],oldState4=g_magazineStates[4];const auto oldGuide3=g_magazineGuides[3],oldGuide4=g_magazineGuides[4];const LONG oldProfile=g_weaponMechanicProfile;
 const auto oldGate=g_reloadSignal;const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;
 g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;g_magazineGuides[2].box=false;
 for(int f=2;f<=4;++f){g_magazineGuides[f].box=false;for(int i=0;i<3;++i)g_magazineGuides[f].port[i]=0;}
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"AK owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;ObserveAmmoWidget(wrapper,hud);g_pistolFixtureHud=hud;
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=SkorpionFixtureTransfer;UINT signal=0xaf41179b;
 MH_Initialize();Check(InstallReloadRequestAt(reinterpret_cast<void*>(SignalFixtureNative)),"AK real pre-predicate MinHook installed");
 const auto oldSignalOriginal=g_reloadSignalOriginal;const auto oldSignalFallback=g_reloadSignalFallback;
 BYTE* akFn=reinterpret_cast<BYTE*>(SignalFixtureNative);UINT akSite=0,akFallback=0;const BYTE akSig[]={0x8b,0,0x51,0x8b,0xcc};
 for(int i=0;i<128;++i){if(!memcmp(akFn+i,akSig,5))akSite=reinterpret_cast<UINT>(akFn+i);if(akFn[i]==0xe8&&akFn[i+5]==0x84&&akFn[i+6]==0xc0&&akFn[i+7]==0x0f&&akFn[i+8]==0x95&&akFn[i+9]==0xc0)akFallback=reinterpret_cast<UINT>(akFn+i);}
 Check(akSite&&akFallback&&InstallReloadSignalAt(reinterpret_cast<void*>(akSite),reinterpret_cast<void*>(akFallback)),"AK signal-site MinHook installed for denial checks");
 for(UINT capacity:{30u,45u}){
  auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.rootBody=HandIdentity();
  *reinterpret_cast<UINT*>(weapon+0xcc)=capacity;*reinterpret_cast<UINT*>(hud+0x134)=capacity;*reinterpret_cast<UINT*>(hud+0x138)=capacity;*reinterpret_cast<UINT*>(hud+0x140)=100;*reinterpret_cast<UINT*>(hud+0x144)=capacity+100;
  c.x=false;c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickPistolReload(true);
  c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(p.magazinePresent,"reload26 full AK magazine stays seated on X");c.x=false;fc3pad::Publish(c);TickPistolReload(true);
  p.magazinePresent=false; // already out (dropped by hand earlier): the insertion below is the subject
  c.leftGrip=1;fc3pad::Publish(c);TickPistolReload(true);c.x=true;fc3pad::Publish(c);TickPistolReload(true);
  c.x=false;fc3pad::Publish(c);p.tip={};p.handFrame=p.portFrame=g_frame;TickPistolReload(true);++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);
  Check(p.pending&&!p.state&&!p.wrapper&&fc3pad::ReloadOutput(),"first full AK insertion has no cached native owner and requests X");
  g_fixturePredicate=0;g_fixtureReady=1;const UINT signals=g_signalCalls,readiness=g_readyCalls;
  MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));InvokeSignalFixture(state,wrapper);
  Check(p.pending&&!p.state&&!p.wrapper&&!p.magazinePresent&&g_signalCalls==signals,"unpatched first-full AK path cannot seat or discover owner because predicate rejects reload");
  Check(MH_EnableHook(reinterpret_cast<void*>(SignalFixtureNative))==MH_OK,"restore AK pre-predicate observer after reproducing old failure");
  const UINT readinessAfterReproduction=g_readyCalls;
  InvokeSignalFixture(state,wrapper);
  Check(p.magazinePresent&&!p.pending&&!p.held&&p.state==state&&p.wrapper==wrapper&&p.weapon==weapon,"pre-predicate native owner seats first full AK replacement");
  Check(g_fixtureResult==1&&g_readyCalls==readinessAfterReproduction+1&&g_signalCalls==signals&&g_signalStackBefore==g_signalStackAfter,"full AK native readiness and ret4 remain unchanged; no reload signal");
  for(int n=0;n<4;++n)Check(g_signalSse[n]==g_signalSeed[n],"AK pre-predicate hook preserves live SSE");
  Check(ReloadRead(weapon,0xcc)==capacity&&ReloadRead(hud,0x140)==100&&!fc3pad::ReloadOutput(),"full AK reseat preserves all ammo and cancels generated X");
  ++g_frame;g_weaponMechanicFrame=p.rootFrame=g_frame;fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"first full AK reseat restores held-trigger firing after actual normal frame");
  c.rightTrigger=0;fc3pad::Publish(c);
 }
 std::puts("PASS: first full AK magazine at 30/45 capacity traverses real pre-predicate MinHook, preserves native readiness/ret4/SSE/ammo and restores firing without native reload Begin.");
 for(LONG rifle:{8L,7L,41L,28L,14L,9L,31L,25L})for(UINT capacity:{30u,45u})for(UINT loaded:{5u,0u}){
  g_weaponMechanicProfile=rifle;g_weaponMechanicFrame=++g_frame;
  auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.rootBody=HandIdentity();
  Check(AkModel()&&CurrentRifle().profile==rifle&&&p==&g_magazineStates[MagazineFamilyOfProfile(rifle)],"rifle uses its own table and magazine slot");
  *reinterpret_cast<UINT*>(weapon+0xcc)=loaded;*reinterpret_cast<UINT*>(hud+0x134)=loaded;*reinterpret_cast<UINT*>(hud+0x138)=capacity;*reinterpret_cast<UINT*>(hud+0x140)=100;*reinterpret_cast<UINT*>(state+8)=0;
  c.x=false;c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickPistolReload(true);
  { // reload26: full protection restored; partial/no-reserve/unknown remain independent
   *reinterpret_cast<UINT*>(weapon+0xcc)=capacity;*reinterpret_cast<UINT*>(hud+0x134)=capacity;ObserveAmmoWidget(wrapper,hud);
   c.leftGrip=1;c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(p.magazinePresent&&ReloadRead(weapon,0xcc)==capacity,"full magazine refuses X eject without native ammo write");p.magazinePresent=true;c.x=false;fc3pad::Publish(c);TickPistolReload(true);
   *reinterpret_cast<UINT*>(weapon+0xcc)=capacity-2;*reinterpret_cast<UINT*>(hud+0x134)=capacity-2;*reinterpret_cast<UINT*>(hud+0x140)=0;
   c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(!p.magazinePresent&&ReloadRead(hud,0x140)==0,"physical eject is independent of spare ammo");p.magazinePresent=true;c.x=false;fc3pad::Publish(c);TickPistolReload(true);
   *reinterpret_cast<UINT*>(hud+0x140)=100;*reinterpret_cast<UINT*>(weapon+0xcc)=loaded;*reinterpret_cast<UINT*>(hud+0x134)=loaded;}
  c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(!p.magazinePresent&&ReloadRead(weapon,0xcc)==loaded,"AK X eject (magazine not full) has no reload or ammo write");
  Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"AK stock X and empty auto reload blocked");
  c.x=false;c.rightGrip=0;c.leftGrip=1;fc3pad::Publish(c);TickPistolReload(true);c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(p.held&&!p.pending,"AK grip X takes replacement only");
  c.x=false;fc3pad::Publish(c);p.tip=p.port={};p.handFrame=p.portFrame=g_frame;TickPistolReload(true);
  ++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);Check(p.pending&&!p.held&&fc3pad::ReloadOutput(),"AK physical contact requests the engine seam once");
  Check(!PistolContactReady(state,wrapper)&&RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"AK never grants the animated native reload");
  g_fixturePredicate=1;g_fixtureReady=1;const UINT signals=g_signalCalls,readiness=g_readyCalls;
  InvokeSignalFixture(state,wrapper);
  Check(!p.pending&&!p.cycle&&!p.held&&p.magazinePresent&&p.committed&&p.commits==1,"AK request seam seats and commits in one call");
  Check(ReloadRead(weapon,0xcc)==capacity&&ReloadRead(hud,0x140)==100-(capacity-loaded),"AK ammo and reserve change exactly once at the seam");
  Check(ReloadRead(state,8)==0&&!fc3pad::ReloadOutput(),"native pending word cleared by synchronous End; generated X cancelled");
  Check(g_signalCalls==signals&&g_readyCalls==readiness+1&&g_fixtureResult==1&&g_signalStackBefore==g_signalStackAfter,"animated reload signal denied after instant seat; readiness/ret4 intact");
  for(int n=0;n<4;++n)Check(g_signalSse[n]==g_signalSeed[n],"AK instant insert preserves live SSE");
  if(CurrentRifle().emptyNeedsCharge&&loaded==0){
   Check(p.needsRack&&p.rackMode&&!p.chamberReady&&PistolBusy()&&PistolGameplayGuideTarget()==3,"A2000 / MP5 / F1 empty insert: ammo in, fire locked until the charging handle is pulled");
   fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==0,"A2000 cannot fire before the knob pull");c.rightTrigger=0;fc3pad::Publish(c);
   p.rootBody=HandIdentity();p.rootEpoch=g_armPoseEpoch;p.rootFrame=p.palmFrame=g_frame;p.tip=PistolRackGuide();
   c.leftGrip=0;AdvancePistolRack(c,1000);c.leftGrip=1;AdvancePistolRack(c,1020);Check(p.rackHeld,"left grip acquires the A2000 knob box");
   if(CurrentRifle().chargeHand||g_weaponMechanicProfile==7||g_weaponMechanicProfile==28)g_handBody[1][7]-=.045f;else p.tip.y-=.045;AdvancePistolRack(c,1040);Check(p.rackRear,"charging handle full rear stroke (MP5 / F1: read from the real controller, the drawn hand is on the handle)");if(CurrentRifle().chargeHand||g_weaponMechanicProfile==7||g_weaponMechanicProfile==28)g_handBody[1][7]+=.045f;c.leftGrip=0;AdvancePistolRack(c,1060);Check(p.rackReturning&&p.needsRack,"A2000 still locked until knob returns");
   AdvancePistolRack(c,1200);Check(p.akRackComplete&&!p.needsRack&&p.chamberReady,"A2000 knob pull/release chambers");
   ++g_frame;g_weaponMechanicFrame=p.rootFrame=g_frame;
  }
  if(!CurrentRifle().emptyNeedsCharge||loaded)Check(!p.needsRack,"no charge rule / partial reload: no handle pull needed");
  Check(!p.needsRack&&!p.rackMode&&p.chamberReady&&!p.awaitingNativeClose&&!PistolOwnsLeft()&&!PistolPoseOwned()&&!PistolBusy(),"rifle insert = ready to fire (charge rifles empty: after the handle)");
  fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"AK fires on the very next trigger pull after seating");c.rightTrigger=0;fc3pad::Publish(c);
  InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==capacity&&ReloadRead(hud,0x140)==100-(capacity-loaded)&&p.commits==1&&g_signalCalls==signals,"stray X after seating cannot reload, debit or animate again");
 }
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=++g_frame;
 std::puts("PASS: AK partial/empty insertion at 30/45 capacity runs native Begin/transfer/End synchronously at the request seam, denies the animation, and fires immediately.");
 std::puts("PASS: A2000 and STG-90 use the same instant seam with their own slots: STG-90 always fires at once; A2000 partial fires at once, A2000 empty fires only after a full knob pull and release.");
 {
  const auto settings=g_pistolGuides;wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"akc",0,path);
  g_pistolGuides.charge[0]=.063f;g_pistolGuides.charge[1]=.30f;g_pistolGuides.angles[2][2]=35;g_pistolGuides.size[2][1]=.12f;
  Check(SavePistolGuides(path),"AK separate charging box Save All");g_pistolGuides=PistolGuideSettings{};LoadPistolGuides(path);
  Check(std::fabs(g_pistolGuides.charge[0]-.063f)<.00001&&std::fabs(g_pistolGuides.size[2][1]-.12f)<.00001&&g_pistolGuides.angles[2][2]==35,"charging position shape rotation survive settings reload");
  Check(PistolGuideContact(HandIdentity(),3,{.063,.30,g_pistolGuides.charge[2]}),"charging visual box and contact share saved centre");
  g_pistolGuides.box=false;Check(PistolGuideContact(HandIdentity(),3,{.063,.30,g_pistolGuides.charge[2]}),"AK charging target remains a box independently of legacy circle toggle");
  DeleteFileW(path);g_pistolGuides=settings;
 }
 std::puts("PASS: separate charge box settings still save and reload (inert for AK firing).");
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=true;g_twoHand=false;
 float oldController[16];memcpy(oldController,g_handBody[0],sizeof(oldController));const Mat4 identity=HandIdentity();for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(identity.m[i]);
 c.x=false;c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);g_frameGunValid=true;g_frameGunMove=identity;
 ID3D11Buffer* vb=Buffer(device,250208,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,51150,D3D11_BIND_INDEX_BUFFER),*globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,kAkCamera,12);memcpy(cam.up,kAkCamera+3,12);memcpy(cam.forward,kAkCamera+6,12);memcpy(cam.position,kAkCamera+9,12);cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 globals->SetPrivateData(kCbCopyGuid,sizeof(kAkFrameGlobals),kAkFrameGlobals);auto* output=BuildWeaponWorld(context,1056,17421);Check(output&&p.parentValid,"AK actual GPU FRAME learns controller attachment");float frame[88];Read(device,context,output,frame);const Mat4 fixed=HullWorld(frame);
 V3 aim={};Check(PistolAimBody(aim),"AK aim uses completed FRAME barrel");
 const Mat4 aimRoot=PistolParentBody(p.rootBody);const V3 expectedAim=Unit({aimRoot.m[1],aimRoot.m[5],aimRoot.m[9]});Check(Length(Sub(aim,expectedAim))<.0001,"AK aim equals corrected gun +Y, not controller forward");
 const auto learned=p;const Mat4 oneHand124=AkParentBody();p.parentValid=false;g_twoHand=true;PistolParentBody(identity);g_twoHand=false;const Mat4 restored124=AkParentBody();for(int i=0;i<16;++i)Check(fabs(oneHand124.m[i]-restored124.m[i])<1e-9,"two-handed AK cannot poison one-handed controller attachment");p=learned;
 std::puts("PASS: AK barrel aim and one-handed parent reject two-handed calibration.");
 float normalGrip[576];memcpy(normalGrip,kAkRightBones,sizeof(normalGrip));Mat4 normalMove=identity;
 Check(!PistolRightHoldingGrip(normalGrip)&&!PistolRightHoldingMove(identity,normalGrip,normalMove)&&memcmp(normalGrip,kAkRightBones,sizeof(normalGrip))==0,"ordinary one-handed AK keeps native rifle grip instead of pistol neutral override");
 std::puts("PASS: normal AK holding preserves native right-hand grip; reload owns its separate suppression.");
 {const auto saved=p;p.fireResumeFrame=g_frame;p.poseHoldUntil=0;p.cycle=p.pending=false;g_twoHand=true;
  Check(PistolBusy(),"AK post-End old FRAME remains blocked even with trigger held");++g_frame;g_weaponMechanicFrame=g_frame;
  globals->SetPrivateData(kCbCopyGuid,sizeof(kAkFrameGlobals),kAkFrameGlobals);output=BuildWeaponWorld(context,1056,17421);float resumed[88];Read(device,context,output,resumed);
  Check(!PistolBusy()&&!PistolPoseOwned()&&p.rootFrame>p.fireResumeFrame,"actual completed normal AK GPU FRAME releases post-End fire gate without timer");
  Mat4 bodyInverse={};Invert(ArmBodyToCamera(),bodyInverse);const Mat4 resumedBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(resumed)));V3 resumedAim={};Check(PistolAimBody(resumedAim),"first resumed AK aim uses normal completed GPU gun");Check(Length(Sub(resumedAim,Unit({resumedBody.m[1],resumedBody.m[5],resumedBody.m[9]})))<.0001,"first AK fire barrel equals normal completed draw, not reload-held root");
  p=saved;g_twoHand=false;
 }
 std::puts("PASS: actual post-End AK GPU FRAME and barrel aim release fire without reusing reload pose or arbitrary hold timer.");
  {const auto saved=p;const auto rightJ=g_armJoints[0],leftJ=g_armJoints[1];float leftPose[16],leftBody[16];memcpy(leftPose,g_handLeft,sizeof(leftPose));memcpy(leftBody,g_handBody[1],sizeof(leftBody));
  p.cycle=true;p.insertActive=true;p.insertAt=GetTickCount64()-300;
  Check(!PistolOwnsLeft()&&!ManualReloadOwnsLeftHand()&&PistolBusy(),"seated AK releases both hand leases without releasing firing");
  V3 support={};Check(AkSupportPosition(support),"seated AK exposes native idle foregrip during hidden animation");
  for(int i=0;i<16;++i)g_handLeft[i]=static_cast<float>(identity.m[i]);g_handLeft[3]=static_cast<float>(support.x-g_wristOffset[1][0]);g_handLeft[7]=static_cast<float>(support.y-g_wristOffset[1][1]);g_handLeft[11]=static_cast<float>(support.z-g_wristOffset[1][2]);
  g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};g_armJoints[1]={{-.2,-.15,-.2},{-.35,-.3,.05},{-1,-1,-1},true};
  Mat4 qi={},cameraHand={},bodyHand={},supportMove={};Check(Invert(ArmBodyToCamera(),qi)&&GunMove(g_armJoints[0],g_armJoints[1],supportMove),"AK contact fixture current support frame");
  for(int i=0;i<16;++i)cameraHand.m[i]=g_handLeft[i];bodyHand=Multiply(qi,cameraHand);for(int i=0;i<16;++i)g_handBody[1][i]=float(bodyHand.m[i]);g_frameGunMove=supportMove;
  V3 contactTarget={},contactPalm={};Check(GrabGizmoPoints(support,contactTarget,contactPalm),"AK contact fixture actual palm centers");const V3 shift=Sub(contactTarget,contactPalm);
  g_handBody[1][3]+=float(shift.x);g_handBody[1][7]+=float(shift.y);g_handBody[1][11]+=float(shift.z);for(int i=0;i<16;++i)bodyHand.m[i]=g_handBody[1][i];cameraHand=Multiply(ArmBodyToCamera(),bodyHand);for(int i=0;i<16;++i)g_handLeft[i]=float(cameraHand.m[i]);
  c.leftGrip=1;c.rightTrigger=0;fc3pad::Publish(c);UpdateTwoHand();Check(g_twoHand&&p.cycle,"AK regrip works before End even when native animated left wrist is far away");
  Mat4 handMove={};Check(AkSupportHandMove(identity,kAkLeftBones,handMove),"AK support hand overrides hidden reload pose");
  const Mat4 wrist=Multiply(handMove,BoneMatrix(kAkLeftBones,2)),desired=Multiply(ArmBodyToCamera(),Multiply(AkParentBody(),kAkSupportHandFromFrame));for(int i=0;i<16;++i)Check(std::fabs(wrist.m[i]-desired.m[i])<.001,"left foregrip wrist follows same root as gun during reload");
  float supportBones[576];memcpy(supportBones,kAkLeftBones,sizeof(supportBones));Check(PistolGrip(supportBones),"AK support restores original foregrip fingers before End");
  g_handLeft[3]+=.03f;const Mat4 turnedRoot=AkParentBody();V3 supportedAim={};Check(PistolAimBody(supportedAim),"supported AK aim remains active during native cycle");Check(Length(Sub(supportedAim,Unit({turnedRoot.m[1],turnedRoot.m[5],turnedRoot.m[9]})))<.0001,"two-hand reload aim follows actual turned barrel");
  c.leftGrip=0;fc3pad::Publish(c);UpdateTwoHand();Check(!g_twoHand&&p.cycle&&PistolBusy(),"support release cannot cancel native reload or unlock firing");
  {
   const bool autoLong=g_autoLongGunSupport,autoPistol=g_autoPistolSupport;
   g_autoLongGunSupport=true;c.leftGrip=0;fc3pad::Publish(c);UpdateTwoHand();Check(g_twoHand,"long-gun auto option grabs near support with grip released");
   UpdateTwoHand();Check(g_twoHand,"automatic support remains held without grip");
   const V3 away=Sub(Apply(qi,{.5,0,0}),Apply(qi,{0,0,0}));
   g_handLeft[3]+=.5f;g_handBody[1][3]+=float(away.x);g_handBody[1][7]+=float(away.y);g_handBody[1][11]+=float(away.z);UpdateTwoHand();Check(!g_twoHand,"automatic support releases by distance without following its own hand");
   g_handLeft[3]-=.5f;g_handBody[1][3]-=float(away.x);g_handBody[1][7]-=float(away.y);g_handBody[1][11]-=float(away.z);g_autoLongGunSupport=false;UpdateTwoHand();Check(!g_twoHand,"manual long guns cannot auto-grab");
   const LONG profile=g_weaponMechanicProfile;g_weaponMechanicProfile=20;g_autoPistolSupport=true;
   Check(AutoSupportForWeapon(),"pistol auto setting independent of disabled long-gun auto");
   g_autoPistolSupport=false;Check(!AutoSupportForWeapon(),"pistol automatic support can be disabled");
   g_weaponMechanicProfile=profile;g_autoLongGunSupport=autoLong;g_autoPistolSupport=autoPistol;
   puts("PASS: controls22 automatic support contact, held stability, distance release, manual long-gun default and independent pistol setting");
  }
  {
   const bool enabled=g_enabled,game=g_inGameplay,gripOnly=fc3pad::GripOnlyAmmoEnabled();fc3pad::SetGripOnlyAmmo(false);float leftBody[16];memcpy(leftBody,g_handBody[1],sizeof(leftBody));
   g_enabled=g_inGameplay=true;g_twoHand=false;
   p.palmFrame=p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.magazinePresent=true;p.held=p.removalHeld=p.rackHeld=false;
   c.leftGrip=0;c.x=c.y=false;fc3pad::Publish(c);fc3pad::SetGameplayControls(true);
   UpdateGripContext();c.leftGrip=1;fc3pad::Publish(c);Check(!fc3pad::WheelOwnsGrip(),"actual rifle support geometry reserves first squeeze for weapon");
   c.leftGrip=0;fc3pad::Publish(c);g_handLeft[3]+=2;g_handBody[1][3]+=10;p.tip={10,10,10};
   UpdateGripContext();c.leftGrip=1;fc3pad::Publish(c);Check(fc3pad::WheelOwnsGrip(),"actual free-space rifle geometry opens wheel without Y");
   const LONG keepProfile=g_weaponMechanicProfile;const auto keepSpas=g_spas;const auto keepLever=g_lever;const auto keepRpg=g_rpg;
   for(const LONG model:{34L,0L,32L}){
    c.leftGrip=0;fc3pad::Publish(c);g_weaponMechanicProfile=model;
    g_spas.simple=false;g_spas.pickup=2;g_spas.ammoState=g_spas.ammoWrapper=0;
    g_lever.enabled=true;g_lever.pickup=2;g_lever.ammoState=g_lever.ammoWrapper=0;g_rpg.ammoState=g_rpg.ammoWrapper=0;
    UpdateGripContext();c.leftGrip=1;fc3pad::Publish(c);
    Check(!fc3pad::WheelOwnsGrip()&&fc3pad::Latest().leftGrip>.5,"button shell/rocket pickup reserves first grip before X, instead of opening wheel");
   }
   g_weaponMechanicProfile=keepProfile;g_spas=keepSpas;g_lever=keepLever;g_rpg=keepRpg;
   c.leftGrip=0;fc3pad::Publish(c);g_handLeft[3]-=2;memcpy(g_handBody[1],leftBody,sizeof(leftBody));
   fc3pad::SetGripContext(false,true);fc3pad::SetGameplayControls(false);fc3pad::SetGripOnlyAmmo(gripOnly);g_enabled=enabled;g_inGameplay=game;
   puts("PASS: controls23 real rifle support geometry reserves grip; free-space geometry opens contextual wheel");
  }
  {
   const auto keep=p;const bool two=g_twoHand;
   p.magazinePresent=true;p.pending=p.cycle=p.slideClosing=p.rackHeld=p.rackReturning=p.removalHeld=p.held=p.looseFlying=false;p.caughtFrame=g_frame-1;p.ammoWrapper=0;
   g_twoHand=true;c.leftGrip=1;c.rightGrip=0;c.x=true;
   const UINT beforeCount=ReloadRead(weapon,0xcc);PistolXAction(c,true);
   Check(p.magazinePresent&&ReloadRead(weapon,0xcc)==beforeCount,"supported rifle keeps verified full magazine seated without ammo mutation");
   *reinterpret_cast<UINT*>(weapon+0xcc)=beforeCount-1;PistolXAction(c,true);Check(!p.magazinePresent&&ReloadRead(weapon,0xcc)==beforeCount-1,"supported rifle partial magazine still ejects normally");*reinterpret_cast<UINT*>(weapon+0xcc)=beforeCount;
   p=keep;g_twoHand=two;c.leftGrip=c.rightGrip=0;c.x=false;fc3pad::Publish(c);
   puts("PASS: supported rifle full-magazine protection and partial X eject without ammo writes");
  }
  p=saved;g_armJoints[0]=rightJ;g_armJoints[1]=leftJ;memcpy(g_handLeft,leftPose,sizeof(leftPose));memcpy(g_handBody[1],leftBody,sizeof(leftBody));g_frameGunMove=identity;g_frameGunValid=true;
 }
 std::puts("PASS: AK actual regrip before native End uses fixed foregrip wrist/fingers and shared supported barrel, never early fire.");
 {const Mat4 savedRoot=p.rootBody;p.rootBody=Multiply(TurnAndMove({0,1,0},{1,0,0},{0,0,0},{0,0,0}),savedRoot);c.rightTrigger=1;fc3pad::Publish(c);
  Check(PistolAimBody(aim),"AK firing aim retains shared controller root");const Mat4 controlled124=AkParentBody();const V3 barrel=Unit({controlled124.m[1],controlled124.m[5],controlled124.m[9]});Check(Length(Sub(aim,barrel))<.0001,"AK aim rejects native recoil root and follows the controller-owned display");p.rootBody=savedRoot;c.rightTrigger=0;fc3pad::Publish(c);}
 {
  const Mat4 savedFree124=g_freeRightFromController;g_freeRightFromController=*CurrentRifle().hand;const auto saved=p;const float oldAssist=g_rifleRecoilAssist;const bool oldTwo=g_twoHand;float hand[16];memcpy(hand,g_handBody[0],sizeof(hand));
  g_twoHand=false;p.cycle=p.pending=p.rackHeld=false;p.parentValid=true;p.parentEpoch=p.rootEpoch=g_armPoseEpoch;p.rootFrame=g_frame;
  p.controllerParent=identity;for(int i=0;i<16;++i)g_handBody[0][i]=float(identity.m[i]);
  c.rightTrigger=1;fc3pad::Publish(c);const V3 kick=Unit({.15,1,0});V3 aim=kick;
  g_rifleRecoilAssist=0;RifleAimAssist(aim);Check(Length(Sub(aim,kick))<1e-6,"zero assist preserves original rifle aim");
  g_rifleRecoilAssist=.5f;RifleAimAssist(aim);Check(aim.x>0&&aim.x<kick.x,"half assist reduces native aim kick");
  aim=kick;g_rifleRecoilAssist=1;RifleAimAssist(aim);Check(Length(Sub(aim,{0,1,0}))<1e-6,"full assist uses tracked quiet grip");
  c.rightTrigger=0;fc3pad::Publish(c);aim=kick;RifleAimAssist(aim);Check(Length(Sub(aim,kick))<1e-6,"trigger release restores original direction");
  {
   const float oldSupported=g_twoHandRecoilAssist;float leftPose[16],leftBody124[16];memcpy(leftPose,g_handLeft,sizeof(leftPose));memcpy(leftBody124,g_handBody[1],64);
   const V3 support=Apply(Multiply(identity,(*CurrentRifle().support)),{-.591,.011,1.149});
   const V3 cameraSupport=Apply(ArmBodyToCamera(),support);
   for(int i=0;i<16;++i)g_handLeft[i]=float(identity.m[i]);
   g_handLeft[3]=float(cameraSupport.x-g_wristOffset[1][0]);g_handLeft[7]=float(cameraSupport.y-g_wristOffset[1][1]);g_handLeft[11]=float(cameraSupport.z-g_wristOffset[1][2]);
   Mat4 bi124={};Invert(ArmBodyToCamera(),bi124);Mat4 cp124={};for(int i=0;i<16;++i)cp124.m[i]=g_handLeft[i];cp124=Multiply(bi124,cp124);for(int i=0;i<16;++i)g_handBody[1][i]=float(cp124.m[i]);g_twoHand=true;g_twoHandRecoilAssist=1;g_rifleRecoilAssist=0;aim=kick;RifleAimAssist(aim);
   Check(Length(Sub(aim,{0,1,0}))<1e-5,"supported aim uses independent full assistance through trigger release");
   g_twoHand=false;g_rifleRecoilAssist=1;g_twoHandRecoilAssist=oldSupported;memcpy(g_handLeft,leftPose,sizeof(leftPose));memcpy(g_handBody[1],leftBody124,64);
  }
  c.rightTrigger=1;fc3pad::Publish(c);p.parentValid=false;aim=kick;RifleAimAssist(aim);Check(Length(Sub(aim,kick))<1e-6,"unknown grip cannot apply aim assist");
  g_freeRightFromController=savedFree124;p=saved;g_rifleRecoilAssist=oldAssist;g_twoHand=oldTwo;memcpy(g_handBody[0],hand,sizeof(hand));c.rightTrigger=0;fc3pad::Publish(c);
  puts("PASS: controls21 rifle aim assist zero/half/full strength, trigger release and missing-grip rejection");
 }
 p.cycle=true;++g_frame;g_weaponMechanicFrame=g_frame;Mat4 tilted=HullWorld(kAkFrameGlobals);tilted.m[3]+=.4;float animated[88];memcpy(animated,kAkFrameGlobals,sizeof(animated));WritePistolWorld(animated,tilted);globals->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);output=BuildWeaponWorld(context,1056,17421);Read(device,context,output,frame);
 for(int i=0;i<16;++i)Check(std::fabs(HullWorld(frame).m[i]-fixed.m[i])<.001,"AK native reload tilt cannot move whole rifle");
 Mat4 invNative={};Invert(HullWorld(kAkFrameGlobals),invNative);Mat4 boltRelative=Multiply(invNative,HullWorld(kAkBoltGlobals));boltRelative.m[7]-=.055;
 float bolt[88];memcpy(bolt,kAkBoltGlobals,sizeof(bolt));WritePistolWorld(bolt,Multiply(tilted,boltRelative));globals->SetPrivateData(kCbCopyGuid,sizeof(bolt),bolt);output=BuildWeaponWorld(context,24369,276);Read(device,context,output,bolt);
 Mat4 quietBolt=kAkBoltRest;quietBolt.m[7]=.3180705;const Mat4 expectedBolt=Multiply(fixed,quietBolt);for(int i=0;i<16;++i)Check(std::fabs(HullWorld(bolt).m[i]-expectedBolt.m[i])<.001,"native automatic AK handle suppressed with steady FRAME");
 ++g_frame;g_weaponMechanicFrame=g_frame;p.nativeGunFrame=-100000;
 // Actual live ordering: handle, accessory, ring precede every FRAME material.
 for(int j=0;j<3;++j){const UINT starts[3]={24369,24645,25395},counts[3]={276,750,180};const Mat4 relatives[3]={boltRelative,kAkRingRest,kAkAccessoryRest};
  float part[88];memcpy(part,kAkFrameGlobals,sizeof(part));WritePistolWorld(part,Multiply(tilted,relatives[j]));globals->SetPrivateData(kCbCopyGuid,sizeof(part),part);output=BuildWeaponWorld(context,starts[j],counts[j]);Read(device,context,output,part);
  const Mat4 expected=Multiply(fixed,j==0?quietBolt:j==1?kAkRingRest:relatives[j]);for(int i=0;i<16;++i)Check(std::fabs(HullWorld(part).m[i]-expected.m[i])<.001,"pre-FRAME AK piece retains local parent without fresh native arm");
 }
 globals->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);output=BuildWeaponWorld(context,1056,17421);
 std::puts("PASS: AK live parts-before-FRAME order keeps automatic bolt still and every accessory attached.");
 {
  const auto saved=p;p.cycle=false;p.needsRack=p.rackMode=p.rackHeld=true;p.slidePull=.045;
  const Mat4 parent=AkParentBody();Mat4 viewInverse={};Check(Invert(HullView(cam),viewInverse),"AK charging draw camera invertible");
  globals->SetPrivateData(kCbCopyGuid,sizeof(kAkBoltGlobals),kAkBoltGlobals);output=BuildWeaponWorld(context,24369,276);Read(device,context,output,bolt);
  Mat4 manualBolt=kAkBoltRest;manualBolt.m[7]=.3180705-.045;
  const Mat4 expected=Multiply(Multiply(viewInverse,Multiply(ArmBodyToCamera(),parent)),manualBolt);
  for(int i=0;i<16;++i)Check(std::fabs(HullWorld(bolt).m[i]-expected.m[i])<.001,"manual AK charging moves only handle on FRAME local rear rail");
  float chargeBones[576];memcpy(chargeBones,kAkLeftBones,sizeof(chargeBones));const Mat4 oldWrist=BoneMatrix(chargeBones,2);
  Check(PistolGrip(chargeBones),"AK manual charge selects native finger pose");
  for(int i=0;i<12;++i)Check(chargeBones[24+i]==kAkLeftBones[24+i],"charging pose leaves tracked wrist untouched");
  double difference=0;
  for(int b=3;b<=19;++b){const Mat4 finger=Multiply(oldWrist,BoneMatrix(kAkChargingGripRelative,b-3));for(int i=0;i<12;++i){Check(std::fabs(chargeBones[b*12+i]-finger.m[i])<.001,"AK charging fingers use captured empty reload pose");difference+=std::fabs(kAkChargingGripRelative[(b-3)*12+i]-kAkMagazineGripRelative[(b-3)*12+i]);}}
  Check(difference>.1,"charging fingers differ from magazine-holding pose");p=saved;
 }
 std::puts("PASS: AK actual GPU handle upload follows manual rail; native charging finger pose keeps tracked wrist.");
 p.cycle=false;p.held=true;p.hand=identity;p.hand.m[3]=.27;p.handFrame=g_frame;globals->SetPrivateData(kCbCopyGuid,sizeof(kAkClipGlobals),kAkClipGlobals);output=BuildWeaponWorld(context,21603,2766);float clip[88];Read(device,context,output,clip);const Mat4 held=Multiply(HullView(cam),HullWorld(clip));for(int i=0;i<16;++i)Check(std::fabs(held.m[i]-p.hand.m[i])<.001,"AK magazine actual GPU follows completed left hand");
 p.held=false;p.magazinePresent=true;p.rootBody=identity;p.rootFrame=p.palmFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.palmBody=identity;
 g_pistolGuides.slide[0]=0;g_pistolGuides.slide[1]=.27f;g_pistolGuides.slide[2]=-.06f;p.tip={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};
 c.leftGrip=0;c.x=false;TickAkRemoval(c);c.leftGrip=1;TickAkRemoval(c);Check(!p.removalHeld&&p.magazinePresent,"LEFT grip alone on the seated magazine does NOT remove it (reported: grabbing two-handed pulled the clip)");
 c.y=true;TickAkRemoval(c);Check(!p.removalHeld,"LEFT grip + Y is the weapon wheel, never removal");c.y=false;
 c.x=true;c.rightGrip=1;TickAkRemoval(c);PistolXAction(c,true);Check(p.removalHeld&&p.magazinePresent&&!p.held&&PistolOwnsLeft(),"manual seated-magazine contact outranks both-grip pickup and button release");c.rightGrip=0;
 p.tip.z-=.015;TickAkRemoval(c);Check(p.magazinePresent,"short AK pull cannot eject");c.leftGrip=0;TickAkRemoval(c);Check(!p.removalHeld&&p.magazinePresent,"short pull release leaves existing magazine seated");
 {
  const auto savedGripState=p;const auto savedControls=c;const bool mode=fc3pad::GripOnlyAmmoEnabled();
  fc3pad::SetGripOnlyAmmo(true);p.removalGripWait=false;c.x=false;c.leftGrip=1;
  TickAkRemoval(c);Check(p.removalHeld,"grip-only AK magazine contact grabs without X");
  TickAkRemoval(c);Check(p.removalHeld,"held grip does not repeat magazine grab");
  p=savedGripState;c=savedControls;fc3pad::SetGripOnlyAmmo(mode);
  std::puts("PASS: offline28 real AK contact grabs seated magazine with grip only and preserves ownership while held");
 }
 c.x=false;TickAkRemoval(c);p.tip={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};c.leftGrip=1;c.x=true;TickAkRemoval(c);p.tip.z-=.06;p.palmBody.m[11]-=.06;TickAkRemoval(c);Check(p.removalHeld&&p.removalDetached&&!p.magazinePresent&&!p.held&&!p.pending,"full physical pull detaches original magazine without generating replacement/reload");
 globals->SetPrivateData(kCbCopyGuid,sizeof(kAkClipGlobals),kAkClipGlobals);output=BuildWeaponWorld(context,21603,2766);Read(device,context,output,clip);
 Mat4 bodyInverse={};Invert(ArmBodyToCamera(),bodyInverse);const Mat4 pulled=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(clip))),expectedHeld=Multiply(p.palmBody,MagazineHeldFromHand());for(int i=0;i<16;++i)Check(std::fabs(pulled.m[i]-expectedHeld.m[i])<.001,"removed old magazine uses native holding attachment and calibration");
 float fingers[576];memcpy(fingers,kAkLeftBones,sizeof(fingers));Check(PistolGrip(fingers),"old magazine removal owns native left fingers");const Mat4 wrist=BoneMatrix(fingers,2);for(int b=3;b<=19;++b){const Mat4 expected=Multiply(wrist,BoneMatrix(kAkMagazineGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-expected.m[i])<.001,"removed magazine native finger pose retained");}
 c.leftGrip=0;c.x=false;TickAkRemoval(c);Check(!p.removalHeld&&!p.magazinePresent&&p.dropActive,"release drops original magazine and frees replacement pickup");
 p.magazinePresent=true;p.dropActive=false;fc3pad::Publish(c);Check(PistolGameplayGuideTarget()==0,"idle AK shows insertion guide without changing grip priority");c.leftGrip=1;fc3pad::Publish(c);Check(PistolGameplayGuideTarget()==1,"AK left grip reveals removal guide");c.leftGrip=0;fc3pad::Publish(c);
 std::puts("PASS: AK removal guide is grip-only and removed magazine uses native holding attachment/fingers.");
 Check(AkRigidDraw(context,21603,2766)&&!AkRigidDraw(context,21603,2765),"AK rigid geometry dispatch exact draw guard");
 std::puts("PASS: actual AK GPU frame rejects reload tilt, native bolt moves independently, held magazine follows left hand; fresh-grip physical pull, short-pull cancel, old-magazine drop and no ammo writes.");
 const bool oldHide=g_hideGameplayGuides;g_hideGameplayGuides=true;PistolGuideMarkers();Check(PistolGuideContact(identity,1,{g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]}),"hidden AK guides retain contact detection");g_hideGameplayGuides=oldHide;
 std::puts("PASS: hiding gameplay guides leaves physical contact unchanged.");
 const auto oldMenu=g_pistolMenu;const int oldMenuFamily=g_menuGuideFamily;
 g_pistolMenu.open=true;g_menuGuideFamily=2;g_pistolGuides.ammoPreview=true;
 CameraSummary leftCamera={};memcpy(leftCamera.right,kAkLeftCamera,12);memcpy(leftCamera.up,kAkLeftCamera+3,12);memcpy(leftCamera.forward,kAkLeftCamera+6,12);memcpy(leftCamera.position,kAkLeftCamera+9,12);leftCamera.mainView=1;
 camera->SetPrivateData(kCameraSummaryGuid,sizeof(leftCamera),&leftCamera);globals->SetPrivateData(kCbCopyGuid,sizeof(kAkLeftGlobals),kAkLeftGlobals);
 CacheAmmoPreviewPose(context,kAkLeftBones);Check(AmmoPreviewPoseAvailable(),"AK caches its actual native left-arm preview independently");
 camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);globals->SetPrivateData(kCbCopyGuid,sizeof(kAkClipGlobals),kAkClipGlobals);
 c.active=false;fc3pad::Publish(c);g_handLeftValid=g_handRightValid=false;p.pending=true;
 const Mat4 expectedPreview=Multiply(g_ammoPreviewPoses[2].palm,MagazineHeldFromHand());
 for(int i=0;i<90;++i){
  ++g_frame;g_weaponMechanicFrame=g_frame;output=BuildWeaponWorld(context,21603,2766);Check(output!=nullptr,"AK preview keeps magazine GPU draw during mouse focus loss");Read(device,context,output,clip);
  const Mat4 previewBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(clip)));
  for(int k=0;k<16;++k)Check(std::fabs(previewBody.m[k]-expectedPreview.m[k])<.001,"AK persistent preview remains attached to cached left hand");
  TickPistolReload(false);Check(p.pending&&!p.held,"AK visual preview never manufactures ammo or discards pending reload");
 }
 g_pistolMenu=oldMenu;g_menuGuideFamily=oldMenuFamily;g_handLeftValid=g_handRightValid=true;c.active=true;fc3pad::Publish(c);
 std::puts("PASS: AK actual GPU magazine preview persists for 90 mouse/untracked frames on its own left-arm cache without inventory mutation.");
 {
  p=PistolReloadState{};p.enabled=p.basis=true;p.rootBody=HandIdentity();p.magazinePresent=false;
  p.pending=p.insertActive=true;p.looseId=77;p.nextLooseId=78;p.deadline=GetTickCount64()-1;
  p.tip={};p.handFrame=p.portFrame=g_frame;c.x=false;c.leftGrip=1;c.rightTrigger=1;fc3pad::Publish(c);
  const UINT before=ReloadRead(weapon,0xcc),reserve=ReloadRead(hud,0x140);TickPistolReload(true);
  Check(p.held&&!p.pending&&!p.insertActive&&!p.magazinePresent&&p.looseId==77&&p.nextLooseId==78,"unstarted AK reload preserves and returns the exact held magazine");
  for(int n=0;n<4;++n){++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);}
  Check(p.held&&!p.pending&&!fc3pad::ReloadOutput()&&ReloadRead(weapon,0xcc)==before&&ReloadRead(hud,0x140)==reserve,"failed AK insertion cannot loop or invent loaded ammo while hand stays in box");
  p.tip={10,10,10};++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);Check(!p.insertionRetryBlocked,"fresh tracked exit rearms physical insertion");
  p.tip={};for(int n=0;n<2;++n){++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);}
  Check(p.pending&&!p.held&&p.looseId==77&&fc3pad::ReloadOutput(),"fresh manual contact requests same AK magazine once after failure");fc3pad::CancelReloadRequest();
  std::puts("PASS: captured AK no-native-start timeout returns same magazine, no ammo writes or stationary retry loop; fresh contact alone retries.");
 }
 context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();memcpy(g_handBody[0],oldController,sizeof(oldController));
 MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));MH_RemoveHook(reinterpret_cast<void*>(SignalFixtureNative));g_reloadRequestOriginal=nullptr;
 MH_DisableHook(reinterpret_cast<void*>(akSite));MH_RemoveHook(reinterpret_cast<void*>(akSite));g_reloadSignalOriginal=oldSignalOriginal;g_reloadSignalFallback=oldSignalFallback;
 VirtualFree(page,0,MEM_RELEASE);g_magazineStates[2]=oldState;g_magazineGuides[2]=oldGuide;g_magazineStates[3]=oldState3;g_magazineStates[4]=oldState4;g_magazineGuides[3]=oldGuide3;g_magazineGuides[4]=oldGuide4;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;
 g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;fc3pad::SetMagazineHandling(false);
}






