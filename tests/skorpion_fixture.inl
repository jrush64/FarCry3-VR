#include "skorpion_data.inl"
#include "skorpion_left_data.inl"
void __stdcall SkorpionFixtureTransfer(UINT,UINT wrapper,UINT,int)
{
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc),capacity=ReloadRead(g_pistolFixtureHud,0x138);
 const UINT reserve=ReloadRead(g_pistolFixtureHud,0x140),add=(std::min)(capacity-before,reserve);
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+add;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=before+add;
 *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)=reserve-add;
 *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x144)=before+reserve;
}
void TestSkorpion(ID3D11Device* device,ID3D11DeviceContext* context)
{
 float savedLeft52[16];memcpy(savedLeft52,g_handBody[1],sizeof(savedLeft52));auto raw52=[&](V3 point){for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;g_handBody[1][3]=float(point.x-g_wristOffset[1][0]);g_handBody[1][7]=float(point.y-g_wristOffset[1][1]);g_handBody[1][11]=float(point.z-g_wristOffset[1][2]);};
 const auto oldState=g_magazineStates[1],oldPistol=g_magazineStates[0];const auto oldGuide=g_magazineGuides[1];const LONG oldProfile=g_weaponMechanicProfile;const auto oldGate=g_reloadSignal;
 const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;g_weaponMechanicProfile=39;g_weaponMechanicFrame=++g_frame;
 g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_magazineGuides[1].box=false;
 g_magazineGuides[1].port[0]=g_magazineGuides[1].port[1]=g_magazineGuides[1].port[2]=0;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"Skorpion owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;ObserveAmmoWidget(wrapper,hud);g_pistolFixtureHud=hud;
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=SkorpionFixtureTransfer;UINT signal=0xaf41179b;
 for(UINT capacity:{20u,24u,30u})for(UINT loaded:{7u,0u}){
  g_pistolReload=PistolReloadState{};auto& p=g_pistolReload;p.enabled=p.basis=true;p.rootBody=HandIdentity();p.basisFrame=g_frame;
  *reinterpret_cast<UINT*>(weapon+0xcc)=loaded;*reinterpret_cast<UINT*>(hud+0x134)=loaded;*reinterpret_cast<UINT*>(hud+0x138)=capacity;*reinterpret_cast<UINT*>(hud+0x140)=100;*reinterpret_cast<UINT*>(hud+0x144)=loaded+100;
  c.x=false;c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickPistolReload(true);
  c.leftGrip=1;c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(!p.magazinePresent,"Skorpion LEFT grip + X ejects without native transfer");
  Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"Skorpion raw X and empty auto reload denied");
  c.x=false;c.rightGrip=0;c.leftGrip=1;fc3pad::Publish(c);TickPistolReload(true);c.x=true;fc3pad::Publish(c);TickPistolReload(true);Check(p.held&&!p.cycle&&!p.pending,"Skorpion grip X picks magazine without reload");
  c.x=false;fc3pad::Publish(c);p.tip=p.port={0,0,0};p.handFrame=p.portFrame=g_frame;TickPistolReload(true);
  ++g_frame;g_weaponMechanicFrame=p.handFrame=p.portFrame=g_frame;TickPistolReload(true);Check(p.pending&&!p.held,"Skorpion two contact frames authorize one native transaction");
  Check(!PistolContactReady(state,wrapper+4),"Skorpion foreign wrapper rejected");
  *reinterpret_cast<UINT*>(hud+0x134)=loaded+1; // HUD lags native count; engine owner is authoritative.
  Check(PistolContactReady(state,wrapper),"stale HUD display cannot reject a verified local physical contact");
  c.active=false;fc3pad::Publish(c);Check(PistolContactReady(state,wrapper),"accepted contact survives mouse input focus loss");
  Check(!RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"exact pending contact receives one permission with inactive controller actions");
  c.active=true;fc3pad::Publish(c);
  // Recreate the unspent permission for the existing original duplicate-rejection sequence.
  g_reloadSignal.pistolPermissionUntil=GetTickCount64()+3000;g_reloadSignal.pistolState=state;g_reloadSignal.pistolWrapper=wrapper;g_reloadSignal.pistolWeapon=weapon;
  *reinterpret_cast<UINT*>(hud+0x134)=loaded;
  Check(!RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"Skorpion contact authorizes native reload");
  Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"Skorpion permission cannot be spent twice");
  ReloadBeginBoundary(state,wrapper);
  const auto savedMenu=g_pistolMenu;g_pistolMenu.open=true;c.active=false;fc3pad::Publish(c);const auto cycleBefore=p;TickPistolReload(false);
  Check(p.cycle&&!p.committed&&p.weapon==cycleBefore.weapon,"menu polling cannot discard in-flight native magazine reload");
  const LONG nativeProfileFrame=g_weaponMechanicFrame;g_weaponMechanicFrame=g_frame-20;
  ReloadBoundary(state,wrapper,0,-1);Check(p.committed&&p.commits==1&&ReloadRead(weapon,0xcc)==capacity&&ReloadRead(hud,0x140)==100-(capacity-loaded),"Skorpion native transfer respects actual upgraded capacity and reserve");
  Check(!p.slideClosing&&!p.needsRack,"Skorpion does not acquire invented 1911 slide/rack");
  ReloadBoundary(state,wrapper,0,-1);Check(p.commits==1,"Skorpion duplicate transfer cannot grant second magazine");
  ReloadEndBoundary(state,wrapper,0);g_weaponMechanicFrame=nativeProfileFrame;g_pistolMenu=savedMenu;c.active=true;fc3pad::Publish(c);TickPistolMechanics(c);if(loaded==0){
   Check(p.needsRack&&!p.chamberReady&&PistolBusy()&&!p.rackHeld,"empty Skorpion insertion waits for physical side-handle charge");
   p.rootBody=HandIdentity();p.rootEpoch=g_armPoseEpoch;p.rootFrame=p.palmFrame=g_frame;p.tip=PistolRackGuide();raw52(p.tip);
   c.leftGrip=0;AdvancePistolRack(c,1000);c.leftGrip=1;AdvancePistolRack(c,1020);Check(p.rackHeld,"fresh left grip takes side handle after empty reload");
   p.tip.y-=.02;raw52(p.tip);AdvancePistolRack(c,1040);c.leftGrip=0;AdvancePistolRack(c,1060);Check(p.needsRack&&!p.chamberReady,"short Skorpion charging stroke cannot restore firing");
   p.tip=PistolRackGuide();raw52(p.tip);c.leftGrip=1;AdvancePistolRack(c,1080);p.tip.y-=.045;raw52(p.tip);AdvancePistolRack(c,1100);c.leftGrip=0;AdvancePistolRack(c,1120);Check(p.rackReturning&&p.needsRack,"full stroke still waits for physical release and return");
   AdvancePistolRack(c,1220);Check(!p.needsRack&&p.chamberReady&&!PistolBusy(),"full side-handle pull release return restores empty Skorpion firing");
  }else Check(!p.cycle&&p.chamberReady&&!PistolBusy()&&!p.needsRack,"partial Skorpion reload stays ready without charging");
  fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"Skorpion fires after completed physical reload");c.rightTrigger=0;
 }
 std::puts("PASS: lagging HUD and controller focus loss preserve exact one-contact native Skorpion reload; menu callbacks settle without discard");
 std::puts("PASS: Skorpion empty magazine requires full physical side-handle pull release return; partial reload preserves readiness and short stroke cannot fire");
 std::puts("PASS: Skorpion partial/empty native reload at capacities 20, 24 and 30; X eject/pickup isolation, one contact permission, local ammo/reserve transfer, duplicate rejection and restored fire with empty-only side-handle charge");
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;auto& p=g_pistolReload;p.held=p.pending=p.cycle=p.insertActive=false;p.poseHoldUntil=0;p.parentValid=false;p.magazinePresent=true;g_twoHand=false;
 Mat4 identity=HandIdentity();float oldController[16];memcpy(oldController,g_handBody[0],sizeof(oldController));for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(identity.m[i]);
 c.rightTrigger=0;fc3pad::Publish(c);g_frameGunValid=true;g_frameGunMove=HandIdentity();
 ID3D11Buffer* vb=Buffer(device,157984,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,35322,D3D11_BIND_INDEX_BUFFER),*globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,kSkFrameCamera,12);memcpy(cam.up,kSkFrameCamera+3,12);memcpy(cam.forward,kSkFrameCamera+6,12);memcpy(cam.position,kSkFrameCamera+9,12);cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 globals->SetPrivateData(kCbCopyGuid,sizeof(kSkFrameGlobals),kSkFrameGlobals);auto* output=BuildWeaponWorld(context,3072,10347);Check(output&&p.parentValid,"Skorpion captured FRAME learns current controller attachment");float frame[88];Read(device,context,output,frame);const Mat4 baselineBody=p.rootBody;
 float clipNative[88];memcpy(clipNative,kSkFrameGlobals,sizeof(clipNative));WritePistolWorld(clipNative,Multiply(HullWorld(kSkFrameGlobals),kSkorpionClipRest));
 ++g_frame;g_weaponMechanicFrame=g_frame;g_handBody[0][3]=.12f;globals->SetPrivateData(kCbCopyGuid,sizeof(clipNative),clipNative);output=BuildWeaponWorld(context,0,1548);float clip[88];Read(device,context,output,clip);Mat4 bodyInverse={};Invert(ArmBodyToCamera(),bodyInverse);const Mat4 clipBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(clip)));const Mat4 expected=Multiply(PistolParentBody(baselineBody),kSkorpionClipRest);
 for(int i=0;i<16;++i){if(std::fabs(clipBody.m[i]-expected.m[i])>=.0002)std::printf("sk mismatch %d %.9f %.9f enabled %d parent %d epoch %u/%u frame %ld/%ld\n",i,clipBody.m[i],expected.m[i],p.enabled,p.parentValid,p.parentEpoch,g_armPoseEpoch,g_frame,g_weaponMechanicFrame);Check(std::fabs(clipBody.m[i]-expected.m[i])<.0002,"Skorpion CLIP-before-FRAME uses current controller and draw camera");}
 p.held=true;p.hand=HandIdentity();p.hand.m[3]=.25;p.hand.m[7]=-.1;p.handFrame=g_frame;output=BuildWeaponWorld(context,0,1548);Read(device,context,output,clip);const Mat4 held=Multiply(HullView(cam),HullWorld(clip));for(int i=0;i<16;++i)Check(std::fabs(held.m[i]-p.hand.m[i])<.0002,"actual Skorpion CLIP follows completed left hand");
 p.held=false;p.magazinePresent=false;p.looseFlying=false;p.insertActive=p.dropActive=false;output=BuildWeaponWorld(context,0,1548);Read(device,context,output,clip);Check(std::fabs(clip[7])>100,"Skorpion detached native magazine is hidden");
 p.insertActive=p.pending=true;p.insertStart=kSkorpionClipRest;p.insertStart.m[11]-=.1;p.insertAt=GetTickCount64()-140;const auto insertBefore167=GetTickCount64();output=BuildWeaponWorld(context,0,1548);const auto insertAfter167=GetTickCount64();Read(device,context,output,clip);const Mat4 insertedBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(clip)));const Mat4 expectedStart167=Multiply(PistolParentBody(p.rootBody),PistolInsertRelative(static_cast<double>(insertBefore167-p.insertAt)/1000)),expectedEnd167=Multiply(PistolParentBody(p.rootBody),PistolInsertRelative(static_cast<double>(insertAfter167-p.insertAt)/1000));for(int i=0;i<16;++i)Check(insertedBody.m[i]>=(std::min)(expectedStart167.m[i],expectedEnd167.m[i])-.003&&insertedBody.m[i]<=(std::max)(expectedStart167.m[i],expectedEnd167.m[i])+.003,"Skorpion insertion uses visible seating transform before FRAME within actual upload interval");
 p.pending=p.insertActive=false;p.looseFlying=true;p.looseBody=HandIdentity();p.looseBody.m[3]=.6;output=BuildWeaponWorld(context,0,1548);Read(device,context,output,clip);const Mat4 flying=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(clip)));for(int i=0;i<16;++i)Check(std::fabs(flying.m[i]-p.looseBody.m[i])<.0002,"Skorpion flying clip uses independent trajectory before FRAME");
 // Actual native holding palette passes through stable pose, IK and upload.
 p.held=p.looseFlying=p.pending=p.cycle=false;p.magazinePresent=true;
 ID3D11Buffer* armVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*rawCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);UINT armStride=40;context->IASetVertexBuffers(0,1,&armVb,&armStride,&offset);context->VSSetConstantBuffers(3,1,&rawCb);rawCb->SetPrivateData(kCbCopyGuid,sizeof(kSkRaw),kSkRaw);globals->SetPrivateData(kCbCopyGuid,sizeof(kSkGlobals),kSkGlobals);
 CameraSummary armCam={};memcpy(armCam.right,kSkCamera,12);memcpy(armCam.up,kSkCamera+3,12);memcpy(armCam.forward,kSkCamera+6,12);memcpy(armCam.position,kSkCamera+9,12);armCam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(armCam),&armCam);const auto savedJoints=g_armJoints[0];g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};
 Mat4 armToView={},armFromView={};float rawBones[576];Check(DrawSpace(context,rawBones,armToView,armFromView),"Skorpion arm space");auto* palette=BuildArmPalette(context,0);Check(palette!=nullptr,"Skorpion actual controlled arm palette");float finalBones[576];Read(device,context,palette,finalBones);const Mat4 wrist=Multiply(armToView,BoneMatrix(finalBones,2)),desiredWrist=Multiply(ArmBodyToCamera(),Multiply(PistolParentBody(p.rootBody),kSkorpionHandFromFrame));for(int i=0;i<16;++i)Check(std::fabs(wrist.m[i]-desiredWrist.m[i])<.0002,"Skorpion uploaded right wrist shares gun FRAME");
 Mat4 rawWristInverse={};Invert(BoneMatrix(kSkRaw,2),rawWristInverse);for(int b=3;b<=19;++b){const Mat4 expectedBone=Multiply(wrist,Multiply(rawWristInverse,BoneMatrix(kSkRaw,b)));const Mat4 actualBone=Multiply(armToView,BoneMatrix(finalBones,b));for(int i=0;i<16;++i)Check(std::fabs(expectedBone.m[i]-actualBone.m[i])<.0002,"Skorpion uploaded fingers use its captured native holding pose");}
 g_armJoints[0]=savedJoints;rawCb->Release();armVb->Release();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
 ObservePistolHand(kSkRaw,kSkGlobals,armCam);Check(p.nativeGunFrame==g_frame,"Skorpion raw right hand supplies matching native root");
 float boltNative[88];memcpy(boltNative,kSkFrameGlobals,sizeof(boltNative));Mat4 rear=kSkorpionSlideRest;rear.m[7]-=.025;WritePistolWorld(boltNative,Multiply(HullWorld(kSkFrameGlobals),rear));globals->SetPrivateData(kCbCopyGuid,sizeof(boltNative),boltNative);p.rootFrame=g_frame-1;output=BuildWeaponWorld(context,1548,468);float bolt[88];Read(device,context,output,bolt);const Mat4 boltBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(bolt)));Mat4 inverseGun={};Invert(PistolParentBody(p.rootBody),inverseGun);const Mat4 boltRelative=Multiply(inverseGun,boltBody);for(int i=0;i<16;++i)Check(std::fabs(boltRelative.m[i]-rear.m[i])<.0004,"Skorpion bolt retains native travel using matching raw wrist before FRAME");
 g_twoHand=true;fc3pad::Publish(c);UpdateTwoHand();Check(!g_twoHand,"Skorpion remains one-handed while support grip is held");
 std::puts("PASS: Skorpion native bolt motion retained before FRAME with matching right-arm root; one-handed support policy");
 p.needsRack=p.rackHeld=true;p.slidePull=.035;globals->SetPrivateData(kCbCopyGuid,sizeof(boltNative),boltNative);output=BuildWeaponWorld(context,1548,468);Read(device,context,output,bolt);
 const Mat4 manualBoltBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(bolt)));const Mat4 manualRelative=Multiply(inverseGun,manualBoltBody);Mat4 expectedCharge=kSkorpionSlideRest;expectedCharge.m[7]-=.035;
 for(int i=0;i<16;++i)Check(std::fabs(manualRelative.m[i]-expectedCharge.m[i])<.0005,"actual Skorpion GPU side handle follows manual rearward local rail");
 p.rackHeld=p.needsRack=false;p.slidePull=0;
 std::puts("PASS: actual Skorpion charging-handle GPU movement stays local to gun and preserves native motion outside manual empty reload");

 // Separate silencer uses the same corrected gun root, including attachment-before-FRAME.
 ID3D11Buffer* silVb=Buffer(device,10560,D3D11_BIND_VERTEX_BUFFER),*silIb=Buffer(device,2160,D3D11_BIND_INDEX_BUFFER);
 context->IASetVertexBuffers(0,1,&silVb,&stride,&offset);context->IASetIndexBuffer(silIb,DXGI_FORMAT_R16_UINT,0);
 Mat4 nativeInverse={},silViewInverse={};Invert(HullView(cam),silViewInverse);const Mat4 nativeGun=Multiply(silViewInverse,Multiply(ArmBodyToCamera(),p.nativeGunBody));
 Mat4 silLocal=HandIdentity();silLocal.m[7]=.24;silLocal.m[11]=.03;
 float silSource[88];memcpy(silSource,kSkFrameGlobals,sizeof(silSource));WritePistolWorld(silSource,Multiply(nativeGun,silLocal));
 globals->SetPrivateData(kCbCopyGuid,sizeof(silSource),silSource); // native short Globals layout
 // Arm constants remain bound on some solid draws. They are not evidence of
 // skinning: the verified SMG mesh and suppressor still need Globals parenting.
 ID3D11Buffer* staleArmCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);staleArmCb->SetPrivateData(kCbCopyGuid,sizeof(kSkRaw),kSkRaw);context->VSSetConstantBuffers(3,1,&staleArmCb);
 Check(BuildWeaponPalette(context,0,1080)==nullptr,"SMG suppressor ignores an arm palette left bound in CB3");
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 Check(BuildWeaponPalette(context,3072,10347)==nullptr,"Skorpion FRAME cannot take the stale skinned-weapon path");
 context->IASetVertexBuffers(0,1,&silVb,&stride,&offset);context->IASetIndexBuffer(silIb,DXGI_FORMAT_R16_UINT,0);
 ID3D11Buffer* clearedArm=nullptr;context->VSSetConstantBuffers(3,1,&clearedArm);staleArmCb->Release();
 std::puts("PASS: Skorpion rigid FRAME and suppressor reject stale arm palette so Globals parenting owns every mapped solid draw");
 output=BuildWeaponWorld(context,0,1080);float silOut[88]={};Read(device,context,output,silOut);
 const Mat4 silBody=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(silOut)));Invert(PistolParentBody(p.rootBody),nativeInverse);
 const Mat4 correctedLocal=Multiply(nativeInverse,silBody);
 for(int i=0;i<16;++i)Check(std::fabs(correctedLocal.m[i]-kSkorpionSilencerFromFrame.m[i])<.0005,"silencer shares current corrected gun attachment before FRAME");

 // Animated native wrists and stale cache cannot move a rigid suppressor.
 const Mat4 savedNative=p.nativeGunBody,savedRoot=p.rootNative;
 p.nativeGunBody.m[3]+=2;p.nativeGunBody.m[7]-=1;p.silencerRelative.m[3]=9;p.silencerValid=true;
 p.rootFrame=g_frame-10;p.nativeGunFrame=g_frame;
 const bool oldLeftValid=g_handLeftValid;g_handLeftValid=false;c.active=false;fc3pad::Publish(c);
 const auto inventoryBefore=p;output=BuildWeaponWorld(context,0,1080);Check(output!=nullptr,"suppressor renders with support-hand actions unavailable");Read(device,context,output,silOut);
 Mat4 correctedRoot=PistolParentBody(p.rootBody),correctedInverse={};Invert(correctedRoot,correctedInverse);
 const Mat4 immutableLocal=Multiply(correctedInverse,Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(silOut))));
 for(int i=0;i<16;++i)Check(std::fabs(immutableLocal.m[i]-kSkorpionSilencerFromFrame.m[i])<.0005,"native reload wrist cannot contaminate SMG socket");
 Check(p.magazinePresent==inventoryBefore.magazinePresent&&p.needsRack==inventoryBefore.needsRack&&p.held==inventoryBefore.held,"visual attachment cannot change physical reload state");
 p.nativeGunBody=savedNative;p.rootNative=savedRoot;
 // Aim follows the uploaded FRAME barrel through yaw and translation. It is
 // independent of the suppressor position, left tracking and action focus.
 const Mat4 savedParent=p.controllerParent;Mat4 quarter=HandIdentity();quarter.m[0]=quarter.m[10]=0;quarter.m[2]=1;quarter.m[8]=-1;
 p.controllerParent=Multiply(quarter,savedParent);p.silencerRelative.m[3]=15;
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 globals->SetPrivateData(kCbCopyGuid,sizeof(kSkFrameGlobals),kSkFrameGlobals);output=BuildWeaponWorld(context,3072,10347);Check(output!=nullptr,"FRAME survives missing support-hand actions");Read(device,context,output,frame);
 const Mat4 actualAimRoot=Multiply(bodyInverse,Multiply(HullView(cam),HullWorld(frame)));V3 aim={};Check(SkorpionAimBody(aim),"corrected Skorpion barrel owns aim");
 const V3 expectedAim=Scale({actualAimRoot.m[1],actualAimRoot.m[5],actualAimRoot.m[9]},1/Length({actualAimRoot.m[1],actualAimRoot.m[5],actualAimRoot.m[9]}));
 Check(Length(Sub(aim,expectedAim))<.0002,"aim target matches actual uploaded GPU barrel after rotation and translation");
 g_handBody[0][3]+=.25f;V3 translatedAim={};Check(SkorpionAimBody(translatedAim)&&Length(Sub(aim,translatedAim))<.00001,"translation or floating attachment cannot steer aim");g_handBody[0][3]-=.25f;
 ++p.parentEpoch;Check(!SkorpionAimBody(aim),"stale calibration cannot own Skorpion aim");--p.parentEpoch;
 g_weaponMechanicProfile=20;Check(!SkorpionAimBody(aim),"Skorpion aim correction cannot affect 1911");g_weaponMechanicProfile=39;
 p.controllerParent=savedParent;g_handLeftValid=oldLeftValid;c.active=true;fc3pad::Publish(c);
 std::puts("PASS: Skorpion aim equals actual corrected GPU FRAME barrel and rigid SMG socket ignores native reload wrist, stale cache and missing support-hand actions");
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);silVb->Release();silIb->Release();
 std::puts("PASS: exact separate silencer GPU draw shares gun parent with current draw camera before FRAME");

 std::puts("PASS: actual Skorpion right arm GPU palette uses native holding pose and shared gun attachment");
 // Upload the left arm through the real stable-pose/IK/grip path. Native
 // charging fingers replace the magazine pose; wrist remains controller driven.
 ID3D11Buffer* leftVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*leftCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);UINT leftStride=40;
 context->IASetVertexBuffers(0,1,&leftVb,&leftStride,&offset);context->VSSetConstantBuffers(3,1,&leftCb);
 leftCb->SetPrivateData(kCbCopyGuid,sizeof(kSkLeftRaw),kSkLeftRaw);globals->SetPrivateData(kCbCopyGuid,sizeof(kSkLeftGlobals),kSkLeftGlobals);CameraSummary leftCam={};memcpy(leftCam.right,kSkLeftCamera,12);memcpy(leftCam.up,kSkLeftCamera+3,12);memcpy(leftCam.forward,kSkLeftCamera+6,12);memcpy(leftCam.position,kSkLeftCamera+9,12);leftCam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(leftCam),&leftCam);
 const auto leftJointsBefore=g_armJoints[1];g_armJoints[1]={{-.2,-.15,-.2},{-.35,-.3,.05},{-.35,-.2,.4},true};
 p.rackHeld=false;p.held=p.cycle=p.pending=false;auto* freePalette52=BuildArmPalette(context,1);Check(freePalette52!=nullptr,"free left wrist before charging grip");float freeHand52[576];Read(device,context,freePalette52,freeHand52);
 p.rackHeld=true;p.held=p.cycle=p.pending=false;p.slideGripValid=true;memcpy(p.slideGripRelative,kPistolGripRelative,sizeof(p.slideGripRelative));
 auto* leftPalette=BuildArmPalette(context,1);Check(leftPalette!=nullptr,"real GPU left charging-hand upload");float leftFinal[576];Read(device,context,leftPalette,leftFinal);
 Mat4 leftToView={},leftFromView={};Check(DrawSpace(context,nullptr,leftToView,leftFromView),"left charging draw space");const Mat4 leftWrist=BoneMatrix(leftFinal,2);double poseDifference=0;Mat4 nativeWrist52=kSkorpionChargingWristFromFrame52;const V3 nativePinch229=Apply(nativeWrist52,kSkorpionPinch229);const double knob229[3]={.026,.14320966,.04219692};for(int axis=0;axis<3;++axis)nativeWrist52.m[axis*4+3]+=knob229[axis]-(&nativePinch229.x)[axis];nativeWrist52.m[7]-=(std::max)(0.0,(std::min)(.05,p.slidePull));const Mat4 wantedWrist52=Multiply(ArmBodyToCamera(),Multiply(PistolInteractionRoot(p.rootBody),nativeWrist52)),actualWrist52=Multiply(leftToView,leftWrist);for(int i=0;i<12;++i)Check(fabs(wantedWrist52.m[i]-actualWrist52.m[i])<.0005,"229 actual Skorpion GPU wrist preserves native orientation and seats thumb/index pinch on real knob rail");
 for(int b=3;b<=19;++b){const Mat4 expected=Multiply(leftWrist,BoneMatrix(kSkorpionChargingGripRelative,b-3));for(int i=0;i<12;++i){Check(std::fabs(leftFinal[b*12+i]-expected.m[i])<.0002,"uploaded left fingers use decoded native charging pose");poseDifference+=std::fabs(kSkorpionChargingGripRelative[(b-3)*12+i]-kPistolGripRelative[(b-3)*12+i]);}}
 Check(poseDifference>.1,"charging fingers are distinct from magazine-holding fallback");
 p.rackHeld=false;leftPalette=BuildArmPalette(context,1);Check(leftPalette!=nullptr,"released left-hand upload");float released[576];Read(device,context,leftPalette,released);
 for(int i=0;i<12;++i)Check(std::fabs(released[24+i]-freeHand52[24+i])<.0002,"release returns wrist to free tracked pose");
 p.rackHeld=true;p.held=true;Check(PistolGrip(leftFinal),"magazine pose priority");for(int b=3;b<=19;++b){const Mat4 expected=Multiply(BoneMatrix(leftFinal,2),BoneMatrix(kPistolGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(leftFinal[b*12+i]-expected.m[i])<.0002,"magazine pickup retains its accepted fingers");}
 p.held=p.rackHeld=false;g_armJoints[1]=leftJointsBefore;ID3D11Buffer* noBones=nullptr;context->VSSetConstantBuffers(3,1,&noBones);leftCb->Release();leftVb->Release();
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);globals->SetPrivateData(kCbCopyGuid,sizeof(kSkFrameGlobals),kSkFrameGlobals);camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
 std::puts("PASS: actual Skorpion left-arm GPU palette uses native empty-reload charging fingers, preserves tracked wrist and keeps magazine grip separate");
 // Native directional throw: no upward boost; angular release spins about item centre.
 p.held=true;p.hand=HandIdentity();p.handFrame=p.palmFrame=g_frame;p.handVelocity={2,0,-3};p.handAngular={0,0,4};g_worldScale=1;g_bodyYaw=0;c.leftGrip=0;
 const ULONGLONG flightAt=GetTickCount64();PistolThrowLoose(flightAt,true);Check(p.looseVelocity.y==0&&p.looseVelocity.x==2&&p.looseVelocity.z==-3,"stationary vertical hand velocity receives no canned upward impulse");TickPistolLoose(c,flightAt+100);
 Check(std::fabs(p.looseBody.m[3]-.2)<.00001&&std::fabs(p.looseBody.m[7]+.04905)<.00001&&std::fabs(p.looseBody.m[11]+.3)<.00001,"directional throw travels along release velocity under real gravity");Check(std::fabs(p.looseBody.m[0]-std::cos(.4))<.00001,"release angular velocity tumbles magazine independently of translation");
 std::puts("PASS: directional throw uses measured hand velocity without upward boost; physical gravity and independent angular tumble");
 // Settings from each weapon are saved independently, without overwriting 1911.
 wchar_t temp[MAX_PATH];GetTempPathW(MAX_PATH,temp);wcscat_s(temp,L"fc3-skorpion-guide-fixture.ini");DeleteFileW(temp);g_guideLoadFamily=0;g_pistolGuides.hand[0]=.023f;Check(SavePistolGuides(temp),"save 1911 before Skorpion calibration");g_guideLoadFamily=1;g_pistolGuides.hand[0]=-.037f;g_pistolGuides.ammoPosition[1]=-.042f;g_pistolGuides.ammoRotation[0]=37;Check(SavePistolGuides(temp),"save separate Skorpion settings");g_pistolGuides.hand[0]=0;LoadPistolGuides(temp);Check(std::fabs(g_pistolGuides.hand[0]+.037)<.00001,"Skorpion restart restores own hand guide");Check(std::fabs(g_pistolGuides.ammoPosition[1]+.042)<.00001&&g_pistolGuides.ammoRotation[0]==37,"Skorpion magazine offset and angle persist independently");g_guideLoadFamily=0;g_pistolGuides.hand[0]=0;LoadPistolGuides(temp);Check(std::fabs(g_pistolGuides.hand[0]-.023)<.00001,"Skorpion saving leaves 1911 settings intact");g_guideLoadFamily=-1;DeleteFileW(temp);
 std::puts("PASS: separate Skorpion guide save/restart preserves 1911 calibration");
 std::puts("PASS: actual Skorpion GPU magazine uses current parent before FRAME, follows left hand, hides ejected native mesh, visibly inserts and independently flies");
 p.looseFlying=false;p.held=true;g_weaponMechanicProfile=20;TickPistolReload(true);Check(!g_magazineStates[1].held&&!g_magazineStates[1].pending,"Skorpion ownership cancelled on switch to 1911");
 memcpy(g_handBody[0],oldController,sizeof(oldController));context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();VirtualFree(page,0,MEM_RELEASE);
 g_magazineStates[1]=oldState;g_magazineStates[0]=oldPistol;g_magazineGuides[1]=oldGuide;g_weaponMechanicProfile=oldProfile;g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;
 g_reload=PhysicalReloadState{};g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;fc3pad::SetMagazineHandling(false);fc3pad::CancelReloadRequest();
 memcpy(g_handBody[1],savedLeft52,sizeof(savedLeft52));
}
