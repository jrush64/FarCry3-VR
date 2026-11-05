// SPAS-12: profile, captured clip, Manual one-shell-at-a-time loading through the real request + signal seams (one shell
// per transfer, capacity checked, retried), pump only after an EMPTY reload, guides, Simple native-reload arm glue, settings.
void TestSpas()
{
 const auto oldSpas=g_spas;const LONG oldProfile=g_weaponMechanicProfile;const auto oldGate=g_reloadSignal;const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));const double oldScale=g_worldScale;g_worldScale=1;const auto oldEyes=g_eyeViewsBody[0];
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 // 1. profile and clip data
 {fc3weapon::Geometry g={127600,30018,32,240,4908,7401,57};Check(fc3weapon::ProfileIndex(g)==34&&strcmp(fc3weapon::kProfileNames[34],"spas12.xbg")==0,"SPAS main draw = profile 34");}
 Check(SpasClipStart()<kSpasShellInsertSeat&&kSpasShellInsertSeat<SpasClipEnd(),"the played window contains the native transfer moment");
 {const float* s=kSpasClipShellInsert[SpasSeatKey()].shell;Check(std::fabs(s[7]+.005f)<.003f&&std::fabs(s[11]+.015f)<.003f,"seat key: the shell is at the loading port (capture f10780)");
  const float* g=kSpasClipShellInsert[SpasSeatKey()].gate;Check(std::fabs(std::atan2(g[9],g[5])*180/3.14159265-24.9)<.5,"loading gate fully open at the seat");}
 g_weaponMechanicProfile=34;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 auto& S=g_spas;S=Spas{};
 Check(SpasModel()&&!RevolverModel()&&KnownNonPumpModel(),"SPAS is its own model, never the M133 pump rules");
 // engine fixture (same as the 1887 / revolver tests)
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"SPAS owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;g_leverFixtureHud=hud;g_reload.widgets[0].wrapper=static_cast<LONG>(wrapper);g_reload.widgets[0].hud=static_cast<LONG>(hud);
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=Lever1887FixtureTransfer;g_leverFixtureOverfill=true;
 MH_Initialize();Check(InstallReloadRequestAt(reinterpret_cast<void*>(SignalFixtureNative)),"SPAS pre-predicate MinHook installed");
 BYTE* fn=reinterpret_cast<BYTE*>(SignalFixtureNative);UINT site=0,fallback=0;const BYTE sig[]={0x8b,0,0x51,0x8b,0xcc};
 for(int i=0;i<128;++i){if(!memcmp(fn+i,sig,5))site=reinterpret_cast<UINT>(fn+i);if(fn[i]==0xe8&&fn[i+5]==0x84&&fn[i+6]==0xc0&&fn[i+7]==0x0f&&fn[i+8]==0x95&&fn[i+9]==0xc0)fallback=reinterpret_cast<UINT>(fn+i);}
 const auto oldSignalOriginal=g_reloadSignalOriginal;const auto oldSignalFallback=g_reloadSignalFallback;
 Check(site&&fallback&&InstallReloadSignalAt(reinterpret_cast<void*>(site),reinterpret_cast<void*>(fallback)),"SPAS signal-site MinHook installed");
 UINT signalHash=0xaf41179b;
 // capacity 6 here (the real SPAS has 11; the check reads the HUD widget either way)
 auto setAmmo=[&](UINT n){*reinterpret_cast<UINT*>(weapon+0xcc)=n;*reinterpret_cast<UINT*>(hud+0x134)=n;*reinterpret_cast<UINT*>(hud+0x138)=6;};
 *reinterpret_cast<UINT*>(hud+0x140)=40;setAmmo(3);*reinterpret_cast<UINT*>(state+8)=0;
 for(int i=0;i<16;++i){g_eyeViewsBody[0].view[i]=i%5==0?1.f:0.f;g_handBody[0][i]=g_handBody[1][i]=i%5==0?1.f:0.f;}
 Mat4 gunFrame=HandIdentity();gunFrame.m[5]=0;gunFrame.m[6]=1;gunFrame.m[9]=-1;gunFrame.m[10]=0; // FRAME +Y -> body -Z (level gun)
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;S.frameBody=gunFrame;S.frameFrame=g_frame;S.leftFrame=g_frame;S.lastTick=GetTickCount64()-16;fc3pad::Publish(c);TickSpas(true);};
 auto palmAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);S.leftWrist=HandIdentity();S.leftWrist.m[3]=p.x-(-.591+S.hand[0]);S.leftWrist.m[7]=p.y-(.011+S.hand[1]);S.leftWrist.m[11]=p.z-(1.149+S.hand[2]);};
 auto palmBodyAt=[&](const V3& p){S.leftWrist=HandIdentity();S.leftWrist.m[3]=p.x-(-.591+S.hand[0]);S.leftWrist.m[7]=p.y-(.011+S.hand[1]);S.leftWrist.m[11]=p.z-(1.149+S.hand[2]);};
 auto realAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);const double* o=g_wristOffset[1];g_handBody[1][3]=static_cast<float>(p.x-o[0]);g_handBody[1][7]=static_cast<float>(p.y-o[1]);g_handBody[1][11]=static_cast<float>(p.z-o[2]);}; // controller wrist at FRAME point f
 auto fires=[&](){c.rightTrigger=1;fc3pad::Publish(c);UpdateSpasFire(c);const bool f=fc3pad::FireOutput()!=0;c.rightTrigger=0;fc3pad::Publish(c);return f;};
 auto seat=[&](){S.clipAt=GetTickCount64()-static_cast<ULONGLONG>((kSpasShellInsertSeat-SpasClipStart())*1000)-5;tick();};
 const V3 away={.5,.5,.5},axis={kSpasForendAxis[0],kSpasForendAxis[1],kSpasForendAxis[2]};
 {
  const auto original=S;const auto buttons=c;const bool mode=fc3pad::GripOnlyAmmoEnabled();fc3pad::SetGripOnlyAmmo(true);
  c={};c.active=true;S.carrying=S.pending=S.grab=false;S.leftGripWas=false;S.clip=-1;palmAt(away);tick();
  const UINT reserve=ReloadRead(hud,0x140);c.leftGrip=1;tick();
  Check(S.carrying&&!S.grab,"grip-only SPAS pickup away from mechanisms without X");
  Check(ReloadRead(weapon,0xcc)==3&&ReloadRead(hud,0x140)==reserve,"grip pickup cannot transfer native ammo before insertion");
  S.carrying=false;c.leftGrip=.5f;tick();c.leftGrip=1;tick();Check(!S.carrying,"half-release and regrip cannot create a replacement");
  c.leftGrip=0;tick();S=original;palmAt(SpasBoxCentre(1));realAt(SpasBoxCentre(1));c.leftGrip=1;tick();
  Check(S.grab&&!S.carrying,"SPAS physical forend wins over grip-only shell pickup");
  c.leftGrip=0;tick();S=original;S.ammoState=state;S.ammoWrapper=wrapper;palmAt(away);setAmmo(6);c.leftGrip=1;tick();Check(!S.carrying,"full SPAS cannot supply another shell through grip-only");setAmmo(3);
  c.leftGrip=0;tick();S=original;c=buttons;fc3pad::SetGripOnlyAmmo(mode);
  std::puts("PASS: offline28 real SPAS grip-only pickup, native ammo conservation, half-release guard, forend priority and full capacity refusal");
 }
 // 2. defaults: the 1911 hand sphere, Grip + X; Manual refuses the game's reload
 Check(std::fabs(S.handDiameter-.10f)<1e-6&&std::fabs(S.hand[2]+.1174f)<1e-6&&S.pickup==2&&!S.simple&&S.showGuides,"defaults = the 1911 hand sphere (10 cm), Grip + X pickup, Manual, guides on");
 palmAt(away);tick();
 Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"Manual: the game's X / automatic reload is refused");
 Check(fires(),"loaded and chambered: semi-auto, fires (no pump between shots)");
 c.x=true;tick();Check(!S.carrying&&fc3pad::ReloadOutput()&&!fc3pad::AmmoControls().x,"X alone interacts and never picks up a SPAS shell");c.x=false;tick();
 // 3. one shell: LEFT grip + X; rides in the hand as Jason carries it; release drops it
 c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(S.carrying,"LEFT grip + X: ONE shell in the left hand");
 {const SpasKey& k=kSpasClipShellInsert[0];Check(closeMat(Multiply(SpasKeyMat(k.leftHand),SpasShellFromWrist()),SpasKeyMat(k.shell),1e-5),"the shell sits in the hand exactly as Jason carries it (clip key 0)");
  S.ammoPos[1]=.02f;Check(!closeMat(Multiply(SpasKeyMat(k.leftHand),SpasShellFromWrist()),SpasKeyMat(k.shell),1e-3),"INSERT Ammo position moves it");S.ammoPos[1]=0;
  float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Check(SpasLeftGrip(arm)&&closeMat(BoneMatrix(arm,3),BoneMatrix(k.leftFingers,0),1e-5),"left fingers hold the shell (Jason's carry)");}
 c.leftGrip=0;tick();Check(!S.carrying,"let go: dropped, no ammo change");
 {float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Check(SpasLeftGrip(arm)&&closeMat(BoneMatrix(arm,3),BoneMatrix(kSpasLeftIdleFingers,0),1e-5),"Reported (splayed hand): the free left hand always shows Jason's own fingers");}
 S.pickup=0;palmBodyAt({-.18,-.65,.03});c.leftGrip=1;tick();Check(S.carrying,"Waist pickup");c.leftGrip=0;palmAt(away);tick();
 S.pickup=1;palmBodyAt({0,-.38,.12});c.leftGrip=1;tick();Check(S.carrying,"Chest pickup");c.leftGrip=0;palmAt(away);tick();
 S.pickup=2;palmBodyAt({-.18,-.65,.03});c.leftGrip=1;tick();Check(!S.carrying,"Grip + X mode: grip alone at the waist does nothing");c.leftGrip=0;palmAt(away);tick();
 // 4. boxes: PORT under the receiver, FOREND on the forend, apart, adjustable
 {const V3 port=SpasBoxCentre(0),fore=SpasBoxCentre(1);
  Check(port.z<-.03&&std::fabs(port.y)<.03,"PORT box just under the loading port");
  Check(fore.y>.1&&Length(Sub(port,fore))>.15,"FOREND box forward on the forend, well apart from the PORT box");
  S.boxMove[0][0]=.05f;Check(std::fabs(SpasBoxCentre(0).x-port.x-.05)<1e-6,"INSERT X moves the box");S.boxMove[0][0]=0;
  S.boxSize[0][0]=.3f;Check(SpasBoxDistance(0,Add(port,{.14,0,0}))<1e-9,"INSERT Width grows the box");S.boxSize[0][0]=.08f;
  S.boxRot[1][2]=90;const Mat4 m=SpasBoxFrame(1);Check(std::fabs(m.m[0])<1e-6&&std::fabs(m.m[4])>.99,"INSERT Rotate Z turns the box");S.boxRot[1][2]=0;}
 // 5. partial reload (3 loaded): shell into the PORT box -> Jason pushes it in -> exactly +1, fires straight away
 c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();palmAt(SpasBoxCentre(0));tick();
 Check(S.clip==0&&!S.pending,"shell at the loading port: Jason's shell-insert animation starts");
 {float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Mat4 mv=HandIdentity();S.clipAt-=200;
  Check(SpasLeftClipMove(HandIdentity(),arm,mv)&&closeMat(Multiply(mv,BoneMatrix(arm,2)),Multiply(ArmBodyToCamera(),Multiply(S.frameBody,SpasClipPose().hand)),1e-5),"left wrist placed by Jason's clip on the gun");
  Check(SpasLeftGrip(arm),"left fingers from the clip");S.clipAt+=200;}
 Check(!fires(),"no firing while Jason pushes the shell in");
 seat();Check(S.pending&&fc3pad::ReloadOutput()&&S.seatFired,"at the native transfer moment: engine transfer requested");
 g_fixturePredicate=1;g_fixtureReady=1;InvokeSignalFixture(state,wrapper);
 Check(ReloadRead(weapon,0xcc)==4&&!S.pending&&S.filled&&S.loads==1,"native Begin/transfer/End: exactly ONE shell (3 -> 4)");
 Check(!S.needPump,"partial reload: no pump needed");
 S.clipAt-=5000;tick();Check(S.clip<0&&!S.carrying,"animation done, hand back on the controller");
 Check(fires(),"partial reload: fires straight away");
 // 6. full tube: no shell; a shell that reaches a full gun is refused and stays in the hand
 setAmmo(6);palmAt(away);c.leftGrip=0;tick();c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(!S.carrying,"tube full: LEFT grip + X gives nothing");
 setAmmo(5);c.leftGrip=0;tick();c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(S.carrying,"5 of 6: shell");
 palmAt(SpasBoxCentre(0));tick();setAmmo(6);seat();InvokeSignalFixture(state,wrapper);
 Check(ReloadRead(weapon,0xcc)==6&&S.refused&&!S.filled,"filled up meanwhile: refused at the seam (the engine transfer ignores capacity)");
 S.clipAt-=5000;tick();Check(S.clip<0&&S.carrying,"refused: the shell stays in the hand");tick();Check(S.clip<0,"no retry loop while the hand stays in the box");
 c.leftGrip=0;palmAt(away);tick();
 // 7. the seam never answers: re-sent, then the shell stays in the hand
 setAmmo(4);c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();palmAt(SpasBoxCentre(0));tick();seat();Check(S.pending,"transfer requested");
 fc3pad::CancelReloadRequest();S.pulseAt-=300;tick();Check(fc3pad::ReloadOutput(),"no answer yet: the request is re-sent");
 S.clipAt-=5000;tick();Check(S.clip==0&&S.pending,"clip holds its last frame while the transfer is pending");
 S.deadline=GetTickCount64()-1;tick();tick();Check(S.clip<0&&S.carrying&&!S.pending&&ReloadRead(weapon,0xcc)==4,"never answered: the shell stays in the hand, no ammo change");
 palmAt(away);tick();palmAt(SpasBoxCentre(0));tick();seat();InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==5,"second try: in");
 S.clipAt-=5000;tick();c.leftGrip=0;palmAt(away);tick();
 // 8. EMPTY reload: the first shell into an empty gun locks the trigger until the forend is cycled
 setAmmo(0);c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();palmAt(SpasBoxCentre(0));tick();seat();InvokeSignalFixture(state,wrapper);
 Check(ReloadRead(weapon,0xcc)==1&&S.needPump,"empty gun: shell in (0 -> 1), pump required");
 S.clipAt-=5000;tick();c.leftGrip=0;palmAt(away);tick();
 Check(!fires(),"empty reload: trigger locked until the forend is pulled back and pushed forward");
 c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();palmAt(SpasBoxCentre(0));tick();seat();InvokeSignalFixture(state,wrapper);S.clipAt-=5000;tick();c.leftGrip=0;palmAt(away);tick();
 Check(ReloadRead(weapon,0xcc)==2&&S.needPump&&!fires(),"more shells: still needs the pump");
 {fc3xr::ForceReferenceValid(true);g_hideGameplayGuides=false;SpasGuideMarkers();Check(fc3xr::ReloadMarkerBoxVisible(),"pump due: the FOREND guide box is drawn");fc3xr::ForceReferenceValid(false);}
 // Reported: no pump due -> grabbing the forend is a plain two-handed hold; it never slides and the gun fires
 {S.needPump=false;palmAt(SpasBoxCentre(1));realAt(SpasBoxCentre(1));c.leftGrip=1;tick();Check(S.grab&&SpasForendHeld(),"no pump due: LEFT grip on the FOREND box = two-handed hold (Jason's hand on it)");
  realAt(Add(S.grabReal,Scale(axis,.09)));tick();Check(S.pump==0&&!S.reachedBack&&S.pumps==0,"pulled 9 cm: the forend does NOT move (no pump due)");
  Check(fires(),"and it fires while held two-handed, hand pulled back");
  c.leftGrip=0;palmAt(away);tick();S.needPump=true;}
 palmAt(SpasBoxCentre(1));realAt(SpasBoxCentre(1));c.leftGrip=1;tick();Check(S.grab,"pump due: LEFT grip in the FOREND box: forend grabbed");
 {float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Mat4 mv=HandIdentity();
  Check(SpasLeftSupportMove(HandIdentity(),arm,mv)&&closeMat(Multiply(mv,BoneMatrix(arm,2)),Multiply(ArmBodyToCamera(),Multiply(S.frameBody,SpasKeyMat(kSpasLeftSupportFromFrame))),1e-5),"Reported: Jason's left hand takes his own forend hold the moment it is grabbed");
  Check(SpasLeftGrip(arm)&&closeMat(BoneMatrix(arm,3),BoneMatrix(kSpasLeftIdleFingers,0),1e-5),"Jason's forend grip fingers");}
 const V3 start=S.grabReal;
 palmAt(away);realAt(Add(start,Scale(axis,.015)));tick();Check(S.grab&&S.pump==0,"Reported: hand drifting 1.5 cm on the forend while aiming two-handed does not move it");
 realAt(Add(start,Scale(axis,-.015)));tick();Check(S.pump==0,"nor forward");
 realAt(Add(start,Scale(axis,.04)));tick();Check(S.grab&&std::fabs(S.pump-(.04-kSpasForendDeadZone)/(kSpasForendTravel-kSpasForendDeadZone))<1e-6&&!S.reachedBack,"forend follows the REAL controller along its rail (the drawn hand is pinned to it)");
 {float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Mat4 mv=HandIdentity();SpasLeftSupportMove(HandIdentity(),arm,mv);
  const Mat4 at=Multiply(mv,BoneMatrix(arm,2)),rest=Multiply(ArmBodyToCamera(),Multiply(S.frameBody,SpasKeyMat(kSpasLeftSupportFromFrame)));
  const V3 slid=Sub({at.m[3],at.m[7],at.m[11]},{rest.m[3],rest.m[7],rest.m[11]});Check(std::fabs(Length(slid)-S.pump*kSpasForendTravel)<1e-5,"Jason's hand rides the forend by exactly the stroke");}
 realAt(start);tick();Check(S.needPump&&S.pump==0,"half a stroke does not count");
 realAt(Add(start,Scale(axis,.095)));tick();Check(S.reachedBack&&S.pump>.99,"pulled fully back (9 cm, the native stroke)");
 Check(!fires(),"forend back: still locked");
 realAt(Add(start,Scale(axis,.004)));tick();Check(!S.needPump&&S.pumps==1,"pushed forward: chambered");
 realAt(Add(start,Scale(axis,.012)));tick();Check(S.grab&&S.pump==0&&fires(),"Reported: still holding the forend two-handed after the pump: it FIRES and the forend stays put");
 c.leftGrip=0;palmAt(away);tick();Check(fires(),"after the pump: fires");
 // spring: pull back and let go, it returns forward by itself and counts
 S.needPump=true;palmAt(SpasBoxCentre(1));realAt(SpasBoxCentre(1));c.leftGrip=1;tick();realAt(Add(S.grabReal,Scale(axis,.095)));tick();c.leftGrip=0;tick();
 Check(SpasForendHeld(),"springing back: Jason's hand stays on the forend");for(int i=0;i<12;++i)tick();Check(S.pump==0&&S.pumps==2&&!S.needPump&&!SpasForendHeld(),"released at the back: the forend springs forward and chambers, then the hand is free");
 palmAt(away);tick();
 // 9. guides
 {fc3xr::ForceReferenceValid(true);SpasGuideMarkers();Check(fc3xr::ReloadMarkerBoxVisible(),"Manual play: the PORT box is drawn by default");
  const bool menu=g_pistolMenu.open,pumpDue=S.needPump;const int target=g_spasGuideTarget52;g_pistolMenu.open=true;
  for(int selected=0;selected<2;++selected){g_spasGuideTarget52=selected;S.needPump=selected==0;SpasGuideMarkers();const auto snapshot=fc3xr::BodyReloadGuideSnapshot51();const Mat4 expected=Multiply(S.frameBody,SpasBoxFrame(selected));Check(snapshot.box&&fabs(snapshot.transform[3]-expected.m[3])<1e-5&&fabs(snapshot.transform[7]-expected.m[7])<1e-5&&fabs(snapshot.transform[11]-expected.m[11])<1e-5,"SPAS Insert preview follows selected calibrated box independent of opposite pump state");}
  g_pistolMenu.open=menu;S.needPump=pumpDue;g_spasGuideTarget52=target;
  g_weaponGuideShown[34]=false;SpasGuideMarkers();Check(!fc3xr::ReloadMarkerBoxVisible(),"guide boxes off: nothing drawn");g_weaponGuideShown[34]=true;fc3xr::ForceReferenceValid(false);}
 // 10. the pump requirement survives a weapon switch; X ownership is released for other guns
 {S.needPump=true;g_weaponMechanicProfile=42;tick();Check(S.needPump&&!S.xOwned,"weapon switch: pump still due; X released for the other gun");
  g_weaponMechanicProfile=34;tick();Check(S.xOwned,"back on the SPAS: X owned again");S.needPump=false;}
 // 11. Simple
 S.simple=true;tick();Check(!RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash))||ReloadRead(weapon,0xcc)>=2,"Simple: the game's reload is allowed");
 fc3pad::CancelReloadRequest();c.x=true;fc3pad::Publish(c);tick();Check(fc3pad::ReloadOutput()&&!S.xOwned,"Simple: X goes to the game");c.x=false;tick();
 *reinterpret_cast<UINT*>(state+8)=1;SpasNativeBegin(state,wrapper);Check(SpasNativeLeft()&&SpasNativeLeftWeight()<1,"native begin: left arm starts following the gun (blend in)");
 S.nativeAt-=1000;Check(SpasNativeLeftWeight()==1,"left arm fully on Jason's animation");
 *reinterpret_cast<UINT*>(state+8)=0;SpasNativeEnd(state,wrapper);S.nativeEndAt-=1000;Check(!SpasNativeLeft(),"native end: left arm back to the controller");
 Check(fires(),"Simple: semi-auto fire");
 // 12. settings
 {wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"spa",0,path);S.boxMove[0][1]=.01f;S.boxSize[1][2]=.09f;S.boxRot[0][0]=15;S.hand[0]=-.07f;S.handDiameter=.12f;S.ammoRot[2]=30;S.pickup=1;S.showGuides=false;Check(SaveSpasSettings(path),"Save All - Spas12");
  S=Spas{};LoadSpasSettings(path);Check(S.simple&&S.pickup==1&&!S.showGuides&&std::fabs(S.boxMove[0][1]-.01f)<1e-6&&std::fabs(S.boxSize[1][2]-.09f)<1e-6&&std::fabs(S.boxRot[0][0]-15)<1e-4&&std::fabs(S.hand[0]+.07f)<1e-6&&std::fabs(S.handDiameter-.12f)<1e-6&&std::fabs(S.ammoRot[2]-30)<1e-4,"SPAS settings (both boxes, hand sphere, shell in hand, pickup, mode) survive save and reload");DeleteFileW(path);}
 std::puts("PASS: SPAS-12: profile 34; semi-auto, no pump between shots; Manual: LEFT grip + X (or Waist / Chest) gives ONE shell held as Jason carries it, the PORT box plays Jason's shell-insert animation and the engine adds exactly one shell at the native transfer moment (capacity checked, re-sent, refused shell stays in the hand); a partial reload fires straight away, an EMPTY reload locks the trigger until the forend is pulled back 9 cm and pushed forward; PORT / FOREND boxes and hand sphere INSERT-adjustable; Simple: X runs the game's reload with the left arm on Jason's animation; settings save.");
 MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));MH_RemoveHook(reinterpret_cast<void*>(SignalFixtureNative));g_reloadRequestOriginal=nullptr;
 MH_DisableHook(reinterpret_cast<void*>(site));MH_RemoveHook(reinterpret_cast<void*>(site));g_reloadSignalOriginal=oldSignalOriginal;g_reloadSignalFallback=oldSignalFallback;
 VirtualFree(page,0,MEM_RELEASE);g_leverFixtureOverfill=false;g_reload=PhysicalReloadState{};
 g_spas=oldSpas;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;g_eyeViewsBody[0]=oldEyes;
 memcpy(g_handBody,oldBody,sizeof(oldBody));g_worldScale=oldScale;g_twoHand=false;fc3pad::SetXOwned(false);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::CancelReloadRequest();
 g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
}

