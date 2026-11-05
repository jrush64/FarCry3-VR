// .44 Magnum revolver: profile, captured crane math, cylinder/cartridge bone override, Manual open/close (X and thumb
// push), speedloader pickup/seat through the real request + signal seams, capacity, Simple native-reload arm glue.
void TestRevolver()
{
 const auto oldRev=g_revolver;const LONG oldProfile=g_weaponMechanicProfile;const auto oldGate=g_reloadSignal;const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));const double oldScale=g_worldScale;g_worldScale=1;const auto oldEyes=g_eyeViewsBody[0];
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 // 1. profile
 {fc3weapon::Geometry g={186104,31002,40,224,12,9432,57};Check(fc3weapon::ProfileIndex(g)==42&&strcmp(fc3weapon::kProfileNames[42],"stealthhunter.xbg")==0,"revolver main skinned draw = profile 42");}
 // 2. crane = captured crane_open100 (PID3384 f12780)
 {const Mat4 c=RevolverCrane(100);Mat4 e=HandIdentity();const double v[12]={-0.17357689620267538,-0.00010391987848426567,-0.9848202668249549,0.04262858560307753,-5.5160363712984915e-05,0.9999999082073077,-9.581368621901794e-05,-6.927689897408273e-05,0.9848203066004256,3.764288209551118e-05,-0.1735768828854413,0.05078646872801353};
  for(int i=0;i<12;++i)e.m[i]=v[i];Check(closeMat(c,e,.002),"crane 100 deg matches the captured native crane");Check(closeMat(RevolverCrane(80),kRevolverCraneOpen80,.002),"crane 80 deg matches too");
  const V3 cyl=Apply(c,{kRevolverCylinderAxis[0],.05,kRevolverCylinderAxis[2]});Check(cyl.x<-.02,"open swings the cylinder out to the left");}
 g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 auto& R=g_revolver;R=Revolver{};
 // 3. bone override
 {float bones[48*12]={};Mat4 F=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});
  const Mat4 hammer=Multiply(F,RevolverCrane(7)),cyl=Multiply(F,TurnAndMove({1,0,0},{0,1,0},{0,0,0},{0,0,.07}));
  StoreBone(bones,0,F);StoreBone(bones,1,hammer);StoreBone(bones,2,F);StoreBone(bones,3,cyl);float before[48*12];memcpy(before,bones,sizeof(bones));
  RevolverAdjustBones(bones,12,9432,HandIdentity());Check(!memcmp(before,bones,sizeof(bones))&&R.frameFrame==g_frame,"closed cylinder: bones untouched; FRAME recorded");
  R.angle=100;RevolverAdjustBones(bones,12,9432,HandIdentity());Mat4 Fi={};Invert(F,Fi);
  Check(closeMat(BoneMatrix(bones,2),Multiply(F,RevolverCrane(100)),1e-6)&&closeMat(BoneMatrix(bones,3),Multiply(F,Multiply(RevolverCrane(100),Multiply(Fi,cyl))),1e-6),"open: crane and cylinder swing on the crane axis in FRAME");
  Check(closeMat(BoneMatrix(bones,1),hammer,1e-6)&&closeMat(BoneMatrix(bones,0),F,1e-6),"frame and hammer untouched");
  memcpy(bones,before,sizeof(bones));RevolverAdjustBones(bones,9444,2625,HandIdentity());Check(closeMat(BoneMatrix(bones,2),F,1e-6)&&!closeMat(BoneMatrix(bones,3),cyl,1e-6),"cylinder draw: only slot 3 swings");
  for(int b=0;b<7;++b)StoreBone(bones,b,F);RevolverAdjustBones(bones,12069,1800,HandIdentity());Check(closeMat(BoneMatrix(bones,6),Multiply(F,RevolverCrane(100)),1e-6),"cartridges ride the open crane");
  R.simple=true;memcpy(bones,before,sizeof(bones));RevolverAdjustBones(bones,12,9432,HandIdentity());Check(!memcmp(before,bones,sizeof(bones)),"Simple: the game's own animation owns every bone");R.simple=false;R.angle=0;}
 // engine fixture (same as the 1887 tests)
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"revolver owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;g_leverFixtureHud=hud;g_reload.widgets[0].wrapper=static_cast<LONG>(wrapper);g_reload.widgets[0].hud=static_cast<LONG>(hud);
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=Lever1887FixtureTransfer;g_leverFixtureOverfill=true;
 MH_Initialize();Check(InstallReloadRequestAt(reinterpret_cast<void*>(SignalFixtureNative)),"revolver pre-predicate MinHook installed");
 BYTE* fn=reinterpret_cast<BYTE*>(SignalFixtureNative);UINT site=0,fallback=0;const BYTE sig[]={0x8b,0,0x51,0x8b,0xcc};
 for(int i=0;i<128;++i){if(!memcmp(fn+i,sig,5))site=reinterpret_cast<UINT>(fn+i);if(fn[i]==0xe8&&fn[i+5]==0x84&&fn[i+6]==0xc0&&fn[i+7]==0x0f&&fn[i+8]==0x95&&fn[i+9]==0xc0)fallback=reinterpret_cast<UINT>(fn+i);}
 const auto oldSignalOriginal=g_reloadSignalOriginal;const auto oldSignalFallback=g_reloadSignalFallback;
 Check(site&&fallback&&InstallReloadSignalAt(reinterpret_cast<void*>(site),reinterpret_cast<void*>(fallback)),"revolver signal-site MinHook installed");
 UINT signalHash=0xaf41179b;
 auto setAmmo=[&](UINT n){*reinterpret_cast<UINT*>(weapon+0xcc)=n;*reinterpret_cast<UINT*>(hud+0x134)=n;*reinterpret_cast<UINT*>(hud+0x138)=6;};
 *reinterpret_cast<UINT*>(hud+0x140)=40;setAmmo(2);*reinterpret_cast<UINT*>(state+8)=0;
 R.ammoState=state;R.ammoWrapper=wrapper; // weapon known (the probe is tested in 7d)
 for(int i=0;i<16;++i){g_eyeViewsBody[0].view[i]=i%5==0?1.f:0.f;g_handBody[0][i]=g_handBody[1][i]=i%5==0?1.f:0.f;}
 Mat4 level=HandIdentity();level.m[5]=0;level.m[6]=1;level.m[9]=-1;level.m[10]=0;Mat4 gunFrame=level; // FRAME +Y -> body -Z (level gun)
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;R.frameBody=gunFrame;R.frameFrame=g_frame;R.leftFrame=g_frame;R.lastTick=GetTickCount64()-100;fc3pad::Publish(c);TickRevolver(true);};
 auto palmTo=[&](const V3& p){R.leftWrist=HandIdentity();R.leftWrist.m[3]=p.x+.591;R.leftWrist.m[7]=p.y-.011;R.leftWrist.m[11]=p.z-1.149;}; // FRAME = body here
 const V3 hip={-.18,-.65,.03};
 palmTo({.5,.5,.5});tick();
 // hand sphere centre = 1911 formula: wrist x (-.591+HandX, .011+HandY, 1.149+HandZ); palmTo puts the SPHERE CENTRE at p
 auto palmAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);R.leftWrist=HandIdentity();R.leftWrist.m[3]=p.x-(-.591+R.hand[0]);R.leftWrist.m[7]=p.y-(.011+R.hand[1]);R.leftWrist.m[11]=p.z-(1.149+R.hand[2]);};
 auto palmBodyAt=[&](const V3& p){R.leftWrist=HandIdentity();R.leftWrist.m[3]=p.x-(-.591+R.hand[0]);R.leftWrist.m[7]=p.y-(.011+R.hand[1]);R.leftWrist.m[11]=p.z-(1.149+R.hand[2]);};
 auto openAndSettle=[&](){tick();tick();tick();};
 auto dump=[&](){const Mat4 keep=gunFrame;gunFrame=HandIdentity();tick();R.tiltSince-=150;tick();R.dropAt-=2000;tick();gunFrame=keep;tick();};
 const V3 away={.5,.5,.5};
 // 4. Defaults copied from the tuned 1911: 10 cm sphere offset from the palm, Grip + X pickup
 Check(std::fabs(R.handDiameter-.10f)<1e-6&&std::fabs(R.hand[2]+.1174f)<1e-6&&R.pickup==2,"defaults = the 1911 hand sphere (10 cm, palm offset) and Grip + X pickup");
 {V3 s={};palmBodyAt(away);RevolverPalmBody(s);Check(Length(Sub(s,away))<1e-9,"sphere centre uses the palm offset");}
 Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"Manual: the game's X / automatic reload is refused");
 palmAt(away);tick();
 // 5. X alone never opens or closes
 c.x=true;tick();Check(!R.open&&fc3pad::ReloadOutput()&&!fc3pad::AmmoControls().x,"X alone interacts and cannot open cylinder or trigger physical ammo actions");c.x=false;tick();
 // 6. boxes: adjustable, apart, contact with the big hand sphere
 {const V3 side=RevolverBoxCentre(1);Check(side.x>RevolverBoxCentre(0).x+.02,"closed: the CYLINDER box is on the cylinder's right side");
  Check(Length(Sub(RevolverBoxCentre(0),side))>.045,"the ROUNDS and CYLINDER boxes are apart (rear vs side)");
  Check(RevolverBoxDistance(1,Add(side,{.08,0,0}))<=.05&&RevolverBoxDistance(1,Add(side,{.10,0,0}))>.05,"10 cm hand sphere reaches 5 cm past the box surface");
  R.boxMove[1][0]=.05f;Check(std::fabs(RevolverBoxCentre(1).x-side.x-.05)<1e-6,"INSERT X moves the box");R.boxMove[1][0]=0;
  R.boxSize[1][0]=.3f;Check(RevolverBoxDistance(1,Add(side,{.14,0,0}))<1e-9,"INSERT Width grows the box");R.boxSize[1][0]=.07f;
  R.boxRot[1][2]=90;const Mat4 m=RevolverBoxFrame(1);Check(std::fabs(m.m[0])<1e-6&&std::fabs(m.m[4])>.99,"INSERT Rotate Z turns the box");R.boxRot[1][2]=0;
  R.handDiameter=.02f;palmAt(Add(side,{.07,0,0}));c.leftGrip=1;tick();Check(!R.grab,"INSERT Diameter: a small sphere misses");c.leftGrip=0;tick();R.handDiameter=.10f;}
 // 7. physical open: grip the cylinder box and pull it out; push it back in to close
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();Check(R.grab&&!R.open,"LEFT grip in the CYLINDER box: cylinder grabbed");
 palmAt(Add(R.grabStart,{-.015,0,0}));tick();Check(!R.open,"short pull does nothing");
 palmAt(Add(R.grabStart,{-.03,0,0}));tick();Check(R.open&&!R.grab,"pull it out: cylinder opens");
 c.leftGrip=0;openAndSettle();Check(R.angle==100,"swung fully out");
 c.rightTrigger=1;fc3pad::Publish(c);UpdateRevolverFire(c);Check(fc3pad::FireOutput()==0,"cylinder open: the trigger is locked");c.rightTrigger=0;fc3pad::Publish(c);
 palmAt(away);c.x=true;tick();Check(R.open&&!R.empty,"X alone does NOT close it and does NOT empty it (reported: X made the rounds vanish)");c.x=false;tick();
 {const V3 side=RevolverBoxCentre(1);Check(side.x<RevolverBoxCentre(0).x-.02,"open: the CYLINDER box follows the cylinder out to the left");}
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();Check(R.grab&&!R.carrying,"open: LEFT grip in the CYLINDER box grabs it");
 {V3 up={};RevolverPalmBody(up);palmBodyAt(Add(up,{0,.05,0}));}tick();Check(!R.open,"held-grip upward swipe: cylinder closes");
 c.leftGrip=0;openAndSettle();Check(R.angle==0&&!R.grab,"closed");
 palmAt(away);c.leftGrip=1;tick();Check(!R.grab&&!R.carrying,"LEFT grip away from every box does nothing");c.leftGrip=0;tick();
 setAmmo(6);
 // 8. unload, two ways (native screenshots): tip the muzzle up -> rounds fall out; or X -> gone
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();palmAt(Add(R.grabStart,{-.03,0,0}));tick();c.leftGrip=0;openAndSettle();Check(R.open&&!R.empty,"opened by hand again, rounds still in");
 palmAt(away);c.leftGrip=1;tick();c.x=true;tick();Check(!R.carrying&&!R.empty&&!R.dropping,"6 of 6 loaded: LEFT grip + X gives nothing (the cylinder is full)");c.x=false;c.leftGrip=0;tick();
 {// Reported: with 3 of 6 loaded, the left hand should get only the 3 missing rounds
  setAmmo(3);c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(R.carrying&&R.carryCount==3&&!R.empty&&!R.dropping,"3 of 6: LEFT grip + X -> speedloader with the 3 missing rounds, nothing dumped");
  float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());
  const Mat4 hand=Multiply(ArmBodyToCamera(),Multiply(R.leftWrist,RevolverRoundsFromWrist()));int inHand=0,inCyl=0;
  for(int i=0;i<6;++i){const Mat4 m=BoneMatrix(bones,kRevolverRing[i]);if(closeMat(m,hand,1e-5))++inHand;else if(closeMat(m,RevolverCrane(R.angle),1e-5))++inCyl;}
  Check(inHand==3&&inCyl==3,"3 rounds in the hand, 3 stay in the cylinder");
  c.leftGrip=0;palmAt(away);tick();Check(!R.carrying,"let go: dropped");setAmmo(6);}
 dump();Check(R.empty,"tipping empties it");
 {float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());Check(bones[7]==-100,"empty cylinder: the rounds are hidden");}
 // close, reopen, then dump them by tipping
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();{V3 up={};RevolverPalmBody(up);palmBodyAt(Add(up,{0,.05,0}));}tick();c.leftGrip=0;openAndSettle();openAndSettle();Check(!R.open&&R.empty,"Reported: closed after emptying - it STAYS empty");
 {float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());Check(bones[7]==-100,"closed + emptied: no rounds drawn in the cylinder");}
 c.rightTrigger=1;fc3pad::Publish(c);UpdateRevolverFire(c);Check(fc3pad::FireOutput()==0,"closed + emptied: the trigger is dry");c.rightTrigger=0;fc3pad::Publish(c);
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();palmAt(Add(R.grabStart,{-.03,0,0}));tick();c.leftGrip=0;openAndSettle();Check(R.open&&R.empty,"reopened: still empty");
 {const Revolver keep=R;g_weaponMechanicProfile=1;tick();g_weaponMechanicProfile=42;tick();Check(R.empty,"weapon switch: still empty");}
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();{V3 up={};RevolverPalmBody(up);palmBodyAt(Add(up,{0,.05,0}));}tick();c.leftGrip=0;openAndSettle();openAndSettle();
 R.empty=false; // as if reloaded (the fill is tested below)
 palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();palmAt(Add(R.grabStart,{-.03,0,0}));tick();c.leftGrip=0;openAndSettle();Check(R.open&&!R.empty,"open again");
 palmAt(away);tick();tick();Check(!R.empty&&!R.dropping,"gun level: rounds stay in");
 gunFrame=HandIdentity();tick();Check(!R.dropping,"muzzle up: needs a moment (no accidental dump)");
 R.tiltSince-=150;tick();Check(R.dropping&&R.empty,"muzzle up: the rounds fall out");
 {float a[48*12]={},b2[48*12]={};for(int b=0;b<7;++b){StoreBone(a,b,HandIdentity());StoreBone(b2,b,HandIdentity());}
  RevolverAdjustBones(a,12069,1800,HandIdentity());R.dropAt-=400;RevolverAdjustBones(b2,12069,1800,HandIdentity());
  Check(a[7]!=-100&&b2[7]<a[7]-.3,"falling rounds are drawn and drop under gravity");
  bool apart=false;for(int b=1;b<6;++b)for(int i=0;i<12;++i)if(std::fabs(b2[b*12+i]-b2[i])>1e-3)apart=true;Check(apart,"each casing falls and tumbles on its own");
  Check(b2[6*12+7]==-100,"bullet tips hidden while dumping");}
 R.dropAt-=1000;tick();Check(!R.dropping&&R.empty,"then they are gone; cylinder empty");gunFrame=level;tick();
 // 9. speedloader: only now (open + empty). Grip + X (default), and the waist / chest zones
 c.leftGrip=1;tick();c.x=true;tick();Check(R.carrying&&!R.spent,"open and empty: Grip + X puts the speedloader in the left hand");c.x=false;tick();
 {float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());
  Check(closeMat(BoneMatrix(bones,0),Multiply(ArmBodyToCamera(),Multiply(R.leftWrist,RevolverRoundsFromWrist())),1e-5),"the six rounds ride in the hand");
  R.ammoPos[1]=.02f;float moved[48*12];memcpy(moved,bones,sizeof(bones));RevolverAdjustBones(moved,12069,1800,HandIdentity());
  Check(std::fabs(moved[7]-bones[7])>1e-4||std::fabs(moved[3]-bones[3])>1e-4||std::fabs(moved[11]-bones[11])>1e-4,"INSERT Ammo position moves the speedloader in the hand");R.ammoPos[1]=0;
  float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Check(RevolverLeftGrip(arm),"left fingers take the native speedloader grip");
  const Mat4 seatWrist=Multiply(R.frameBody,Multiply(kRevolverCraneOpen80,Mat4{}));(void)seatWrist;
  Mat4 inv={};Invert(kRevolverCartridgesFromLeftWrist,inv);const Mat4 w=Multiply(Multiply(R.frameBody,kRevolverCraneOpen80),inv);const Mat4 keep=R.leftWrist;R.leftWrist=w;
  Mat4 loaderInFrame={};{Mat4 fi={};Invert(R.frameBody,fi);loaderInFrame=Multiply(fi,RevolverLoaderBody());}
  Check(std::fabs(loaderInFrame.m[3]+0.0278)<.002&&std::fabs(loaderInFrame.m[7]-0.0710)<.002&&std::fabs(loaderInFrame.m[11]-0.0483)<.002,"speedloader body sits where the game draws it at the seat (capture f12826)");R.leftWrist=keep;}
 palmAt(away);c.leftGrip=0;tick();Check(!R.carrying,"let go: dropped, no ammo change");
 R.pickup=0;palmBodyAt(hip);c.leftGrip=1;tick();Check(R.carrying,"Waist pickup: LEFT grip at the waist");c.leftGrip=0;palmAt(away);tick();
 R.pickup=1;palmBodyAt({0,-.38,.12});c.leftGrip=1;tick();Check(R.carrying,"Chest pickup: LEFT grip at the chest");c.leftGrip=0;palmAt(away);tick();
 R.pickup=2;palmBodyAt(hip);c.leftGrip=1;tick();Check(!R.carrying,"Grip + X mode: grip alone at the waist does nothing");c.leftGrip=0;tick();
 // 10. load: the hand sphere into the ROUNDS box of the open cylinder fills it
 setAmmo(2);
 palmAt(away);c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(R.carrying,"speedloader in hand");
 palmAt(RevolverBoxCentre(0));tick();
 Check(R.clip==0&&!R.pending&&R.clipFirst>0,"speedloader at the cylinder: Jason's insert animation starts (from the loader-at-cylinder key)");
 {const auto& c=RevolverClipPose();Check(c.loaderOn,"the clip shows the speedloader");
  Mat4 in={};Invert(R.frameBody,in);float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Mat4 mv={};
  Check(RevolverLeftClipMove(HandIdentity(),arm,mv)&&closeMat(Multiply(mv,BoneMatrix(arm,2)),Multiply(ArmBodyToCamera(),Multiply(R.frameBody,c.hand)),1e-5),"left wrist placed by Jason's clip on the gun");
  Check(RevolverLeftGrip(arm),"left fingers from the clip");
  float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());Check(closeMat(BoneMatrix(bones,0),c.cart,1e-5),"rounds follow the clip");}
 c.rightTrigger=1;fc3pad::Publish(c);UpdateRevolverFire(c);Check(fc3pad::FireOutput()==0,"no firing during the animation");c.rightTrigger=0;fc3pad::Publish(c);
 R.clipAt=GetTickCount64()-static_cast<ULONGLONG>((kRevolverInsertSeat-kRevolverClipInsert[R.clipFirst].t)*1000)-5;tick();
 Check(R.pending&&fc3pad::ReloadOutput()&&R.seatFired,"at Jason's seat moment: engine fill requested");
 g_fixturePredicate=1;g_fixtureReady=1;InvokeSignalFixture(state,wrapper);
 Check(ReloadRead(weapon,0xcc)>2&&!R.pending&&!R.empty&&R.loads==1,"native Begin/transfer/End fill it");
 R.clipAt-=5000;tick();Check(R.clip<0&&!R.carrying&&R.open,"animation done; cylinder stays open until closed");
 setAmmo(6); // the real engine fills all six (this fixture adds one round per transfer)
 palmAt(away);c.leftGrip=0;tick();c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(!R.carrying&&!R.empty,"full after loading: Grip + X gives nothing");
 c.leftGrip=0;tick();dump();Check(R.empty,"tip to empty");
 {// Reported: the insert sometimes stopped with the cylinder empty: the fill request never reached the seam
  setAmmo(2);c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(R.carrying&&R.carryCount==6,"speedloader (6)");
  palmAt(RevolverBoxCentre(0));tick();Check(R.clip==0,"insert animation");
  R.clipAt=GetTickCount64()-static_cast<ULONGLONG>((kRevolverInsertSeat-kRevolverClipInsert[R.clipFirst].t)*1000)-5;tick();Check(R.pending,"fill requested");
  fc3pad::CancelReloadRequest();R.pulseAt-=300;tick();Check(fc3pad::ReloadOutput(),"no answer yet: the request is re-sent");
  R.clipAt-=5000;tick();Check(R.clip==0&&R.pending,"clip holds its last frame while the fill is pending");
  R.deadline=GetTickCount64()-1;tick();tick();Check(R.clip<0&&R.carrying&&R.empty&&!R.pending,"the engine never answered: the speedloader stays in the hand (no lost loader)");
  tick();Check(R.clip<0,"no instant retry loop while the hand stays in the box");
  palmAt(away);tick();palmAt(RevolverBoxCentre(0));tick();Check(R.clip==0,"try again: insert animation");
  R.clipAt=GetTickCount64()-static_cast<ULONGLONG>((kRevolverInsertSeat-kRevolverClipInsert[R.clipFirst].t)*1000)-5;tick();
  InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==3&&R.filled&&!R.pending,"second try fills (fixture adds one round per transfer; the game fills all)");
  R.clipAt-=5000;tick();Check(R.clip<0&&!R.carrying&&!R.empty,"done, loaded");c.leftGrip=0;palmAt(away);tick();dump();}
 // dumped while the engine count is still full: the rounds simply go back in (no ammo change)
 setAmmo(6);c.leftGrip=0;palmAt(away);tick();c.leftGrip=1;tick();c.x=true;tick();c.x=false;tick();Check(R.carrying&&R.carryCount==6,"speedloader (cylinder shown empty)");
 palmAt(RevolverBoxCentre(0));tick();Check(R.clip==0,"insert animation");
 R.clipAt-=5000;tick();if(R.pending)InvokeSignalFixture(state,wrapper);
 Check(ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)>=38&&!R.empty&&!R.carrying,"engine full: rounds back in, no ammo change");
 palmAt(away);c.leftGrip=0;tick();R.clipAt-=5000;tick();
 {// Reported: closing the cylinder eventually stopped working. Sphere inside BOTH boxes (log 0.000/0.000).
  const V3 mid=Scale(Add(RevolverBoxCentre(0),RevolverBoxCentre(1)),.5);palmAt(mid);
  {V3 p={};RevolverPalmFrame(p);Check(RevolverBoxDistance(0,p)<=.05&&RevolverBoxDistance(1,p)<=.05,"repro: the 10 cm sphere is inside both boxes");}
  c.leftGrip=1;tick();Check(R.grab,"open, hand in both boxes: LEFT grip grabs the cylinder");
  {V3 up={};RevolverPalmBody(up);palmBodyAt(Add(up,{0,.05,0}));}tick();c.leftGrip=0;openAndSettle();Check(!R.open,"and swiping upward closes it");}
 // 7d. Reported: after firing 3 of 6 the cylinder still showed 6; it should show 3
 {R.ammoState=0;R.ammoWrapper=0;R.probeAt=0;R.empty=false;tick();Check(R.probing&&fc3pad::ReloadOutput(),"unknown weapon: one request probe");
  g_fixturePredicate=1;g_fixtureReady=1;const UINT before=ReloadRead(weapon,0xcc);InvokeSignalFixture(state,wrapper);
  Check(R.ammoWrapper==wrapper&&!R.probing&&ReloadRead(weapon,0xcc)==before,"probe learns the weapon and loads nothing");
  setAmmo(3);float bones[48*12]={};for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());
  int shown=0;for(int b=0;b<6;++b)if(bones[b*12+7]!=-100)++shown;Check(shown==3&&bones[6*12+7]!=-100,"3 rounds loaded: exactly 3 casings drawn (tips still shown)");
  setAmmo(0);for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());shown=0;for(int b=0;b<7;++b)if(bones[b*12+7]!=-100)++shown;Check(shown==0,"0 loaded: nothing drawn");
  setAmmo(6);for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());shown=0;for(int b=0;b<6;++b)if(bones[b*12+7]!=-100)++shown;Check(shown==6,"6 loaded: all six");
  setAmmo(3);R.angle=100;for(int b=0;b<7;++b)StoreBone(bones,b,HandIdentity());RevolverAdjustBones(bones,12069,1800,HandIdentity());shown=0;for(int b=0;b<6;++b)if(bones[b*12+7]>-50)++shown;Check(shown==3,"cylinder swung out: still 3");R.angle=0;setAmmo(6);}
 // 7e. fired dry (0 loaded) with the cylinder open: the speedloader comes without dumping
 {setAmmo(0);R.empty=false;palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();if(!R.open){palmAt(Add(R.grabStart,{-.03,0,0}));tick();}c.leftGrip=0;openAndSettle();openAndSettle();
  palmAt(away);c.leftGrip=1;tick();c.x=true;tick();c.x=false;Check(R.carrying,"fired dry: LEFT grip + X gives the speedloader");c.leftGrip=0;tick();
  palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();{V3 up={};RevolverPalmBody(up);palmBodyAt(Add(up,{0,.05,0}));}tick();c.leftGrip=0;R.clipAt-=5000;openAndSettle();openAndSettle();setAmmo(6);}
 // 7c. right-hand flick to the right closes the open cylinder; a slow move does not
 {openAndSettle();palmAt(RevolverBoxCentre(1));c.leftGrip=1;tick();if(!R.open){palmAt(Add(R.grabStart,{-.03,0,0}));tick();}c.leftGrip=0;openAndSettle();openAndSettle();Check(R.open,"open for the flick test");
  palmAt(away);g_handBody[0][3]=0;tick();R.rightPrevAt=GetTickCount64()-20;R.rightPrev={0,0,0};g_handBody[0][3]=.005f;tick();Check(R.open,"slow right-hand move: stays open");
  R.rightPrevAt=GetTickCount64()-20;R.rightPrev={0,0,0};g_handBody[0][3]=.05f;tick();Check(R.clip==1,"flick right (2.5 m/s along the gun's +X): Jason's close animation plays");
  {float arm[576]={};for(int b=0;b<25;++b)StoreBone(arm,b,HandIdentity());Mat4 mv={};Check(RevolverLeftClipMove(HandIdentity(),arm,mv),"Jason's left hand pushes the cylinder (clip)");}
  R.clipAt=GetTickCount64()-300;tick();Check(R.open&&R.angle>1&&R.angle<99,"the cylinder swings back with the clip");
  R.clipAt-=5000;tick();Check(!R.open&&R.angle==0&&R.clip<0,"closed at the end of the animation");g_handBody[0][3]=0;tick();}
 // 7b. guides: the box renderer needs a point marker with a radius first; the hand sphere is the INSERT diameter
 {R.showGuides=true;g_hideGameplayGuides=false;g_pistolMenu.open=false;
  fc3xr::ForceReferenceValid(true);RevolverGuideMarkers();Check(fc3xr::ReloadMarkerBoxVisible(),"Manual play: the CYLINDER box is drawn by default");
  g_weaponGuideShown[42]=false;RevolverGuideMarkers();Check(!fc3xr::ReloadMarkerBoxVisible(),"guide boxes off: nothing drawn");g_weaponGuideShown[42]=true;fc3xr::ForceReferenceValid(false);}
 // 7. Simple
 R.simple=true;tick();Check(!RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash))||ReloadRead(weapon,0xcc)==6,"Simple: the game's reload is allowed");
 fc3pad::CancelReloadRequest();c.leftGrip=1;tick();Check(fc3pad::ReloadOutput(),"Simple: LEFT grip runs the game's reload");c.leftGrip=0;tick();fc3pad::CancelReloadRequest();
 c.x=true;fc3pad::Publish(c);Check(fc3pad::ReloadOutput(),"Simple: X goes to the game");c.x=false;tick();
 *reinterpret_cast<UINT*>(state+8)=1;RevolverNativeBegin(state,wrapper);Check(RevolverNativeLeft()&&RevolverNativeLeftWeight()<1,"native begin: left arm starts following the gun (blend in)");
 R.nativeAt-=1000;Check(RevolverNativeLeftWeight()==1,"left arm fully on Jason's animation");
 *reinterpret_cast<UINT*>(state+8)=0;RevolverNativeEnd(state,wrapper);R.nativeEndAt-=1000;Check(!RevolverNativeLeft(),"native end: left arm back to the controller");
 // 8. settings
 {wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"rev",0,path);R.boxMove[0][1]=.01f;R.boxSize[1][2]=.09f;R.boxRot[0][0]=15;R.hand[0]=-.07f;R.handDiameter=.12f;R.ammoRot[2]=30;R.pickup=1;R.showGuides=false;Check(SaveRevolverSettings(path),"Save All - Revolver");
  R=Revolver{};Check(R.showGuides,"guide boxes default ON while playing");LoadRevolverSettings(path);Check(R.simple&&R.pickup==1&&!R.showGuides&&std::fabs(R.boxMove[0][1]-.01f)<1e-6&&std::fabs(R.boxSize[1][2]-.09f)<1e-6&&std::fabs(R.boxRot[0][0]-15)<1e-4&&std::fabs(R.hand[0]+.07f)<1e-6&&std::fabs(R.handDiameter-.12f)<1e-6&&std::fabs(R.ammoRot[2]-30)<1e-4,"revolver settings (both boxes, hand sphere, ammo in hand, pickup) survive save and reload");DeleteFileW(path);}
 std::puts("PASS: .44 Magnum revolver: profile 42; crane matches the captured native swing; open crane moves cylinder/cartridge bones only; Manual copies the 1911 model: 10 cm hand sphere at the palm offset (INSERT X/Y/Z/Diameter), Waist / Chest / Grip + X speedloader pickup, speedloader position/rotation in hand; cylinder opens ONLY by gripping its box and pulling out, closes ONLY by pushing in (X alone does nothing), LEFT grip + X in the ROUNDS box pulls the rounds out, hand into the ROUNDS box loads through the engine fill (capacity checked); both boxes INSERT-adjustable; guide box drawn by default; Simple: X or LEFT grip runs the game's reload with the left arm on Jason's animation; settings save.");
 MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));MH_RemoveHook(reinterpret_cast<void*>(SignalFixtureNative));g_reloadRequestOriginal=nullptr;
 MH_DisableHook(reinterpret_cast<void*>(site));MH_RemoveHook(reinterpret_cast<void*>(site));g_reloadSignalOriginal=oldSignalOriginal;g_reloadSignalFallback=oldSignalFallback;
 VirtualFree(page,0,MEM_RELEASE);g_leverFixtureOverfill=false;g_reload=PhysicalReloadState{};
 g_revolver=oldRev;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;g_eyeViewsBody[0]=oldEyes;
 memcpy(g_handBody,oldBody,sizeof(oldBody));g_worldScale=oldScale;g_twoHand=false;fc3pad::SetXOwned(false);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::CancelReloadRequest();
 g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
}

