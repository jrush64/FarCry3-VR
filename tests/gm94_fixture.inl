// GM-94: captured cover hinge / barrel stroke, and shot -> pump forward -> tilt drops the spent shell -> back chambers;
// cover open by hand -> one shell per grab into the port -> cover closed -> pump due.
void TestGm94()
{
 const auto old=g_gm94;const LONG oldProfile=g_weaponMechanicProfile;const double oldScale=g_worldScale;g_worldScale=1;const bool oldTwo=g_twoHand;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 {fc3weapon::Geometry g={243072,40776,32,640,2544,7356,57};Check(fc3weapon::ProfileIndex(g)==18&&strcmp(fc3weapon::kProfileNames[18],"gm94.xbg")==0,"GM-94 body draw = profile 18");}
 auto& G=g_gm94;G=Gm94{};
 G.coverDeg=kGm94CoverOpenAngleDeg;Check(closeMat(Gm94Cover(),Gm94KeyMat(kGm94CoverOpenFromFrame),2e-3),"cover at 142.85 deg = the captured open cover");G.coverDeg=0;
 Check(closeMat(Gm94Cover(),Gm94KeyMat(kGm94CoverClosedFromFrame),1e-9),"closed cover = its captured rest");
 G.offset=kGm94BarrelTravel;Check(closeMat(Gm94Barrel(),Gm94KeyMat(kGm94BarrelForwardFromFrame),1e-3),"full stroke = the captured forward barrel");G.offset=0;
 g_weaponMechanicProfile=18;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 G.known=true;G.chambered=true;
 const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 auto* ammoPage=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(ammoPage!=nullptr,"GM94 actual-shot native owner");
 const UINT actor=reinterpret_cast<UINT>(ammoPage),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;*reinterpret_cast<UINT*>(weapon+0xcc)=4;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;G.ammoState=state;G.ammoWrapper=wrapper;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 for(int i=0;i<16;++i)g_handBody[0][i]=g_handBody[1][i]=i%5==0?1.f:0.f;
 Mat4 level=HandIdentity();level.m[5]=0;level.m[6]=1;level.m[9]=-1;level.m[10]=0; // FRAME +Y -> body -Z, FRAME +Z -> body +Y (level)
 Mat4 gunFrame=level;
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;G.frameBody=gunFrame;G.frameFrame=g_frame;G.leftFrame=g_frame;G.lastTick=GetTickCount64()-100;fc3pad::Publish(c);TickGm94(true);};
 auto wristAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);const double* o=g_wristOffset[1];for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;g_handBody[1][3]=static_cast<float>(p.x-o[0]);g_handBody[1][7]=static_cast<float>(p.y-o[1]);g_handBody[1][11]=static_cast<float>(p.z-o[2]);};
 auto palmAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);G.leftWrist=HandIdentity();G.leftWrist.m[3]=p.x-(-.591+kGm94Hand[0]);G.leftWrist.m[7]=p.y-(.011+kGm94Hand[1]);G.leftWrist.m[11]=p.z-(1.149+kGm94Hand[2]);};
 palmAt({.5,.5,.5});wristAt({0,.2,0});tick();Check(Gm94Ready(),"ready to fire");
 c.rightTrigger=1;fc3pad::Publish(c);UpdateGm94Fire(c);
 Check(G.shot.pending&&G.chambered&&!G.spent&&fc3pad::FireOutput()==255,"GM94 first trigger remains available until actual native shot");
 UpdateGm94Fire(c);Check(G.shot.pending&&fc3pad::FireOutput()==255&&!G.spent,"next frame cannot invent a shot or lock before game consumes RT");
 *reinterpret_cast<UINT*>(weapon+0xcc)=3;UpdateGm94Fire(c);c.rightTrigger=0;UpdateGm94Fire(c);Check(G.spent&&!G.chambered&&!Gm94Ready(),"confirmed native shot: spent shell in, dry");
 // pump forward with both hands
 g_twoHand=true;tick();wristAt({0,.2+kGm94BarrelTravel,0});tick();Check(G.offset>kGm94BarrelTravel*.95&&G.reachedForward,"pushing the left hand forward slides the barrel 17 cm");
 tick();Check(G.spent,"level gun: the spent shell stays in");
 wristAt({0,.2,0});tick();Check(G.spent&&!G.chambered,"barrel back without tilting: still spent, dry");
 wristAt({0,.2+kGm94BarrelTravel,0});tick();
 gunFrame=Multiply(level,TurnAndMove({0,0,1},{.5,0,.866},{0,0,0},{0,0,0}));tick();Check(!G.spent&&G.falling,"chamber open (barrel forward) and tilted: the spent shell falls out at once");
 wristAt({0,.2,0});tick();
 gunFrame=level;tick();Check(G.chambered&&Gm94Ready(),"barrel back: chambered, fire live");
 // Alternate spent-shell removal: release the forward pump, grab the shell, toss, then close once.
 c.rightTrigger=1;UpdateGm94Fire(c);*reinterpret_cast<UINT*>(weapon+0xcc)=2;UpdateGm94Fire(c);c.rightTrigger=0;UpdateGm94Fire(c);
 g_twoHand=true;wristAt({0,.2,0});tick();wristAt({0,.2+kGm94BarrelTravel,0});tick();
 g_twoHand=false;c.leftGrip=0;tick();Check(G.offset>kGm94BarrelTravel*.95&&G.spent,"releasing the support grip leaves the chamber open with its spent shell");
 palmAt({kGm94ShellParkFromFrame[3],kGm94ShellParkFromFrame[7],kGm94ShellParkFromFrame[11]});c.leftGrip=1;tick();
 Check(G.spentCarrying&&!G.spent&&!G.carrying&&!G.pending,"fresh left grip removes the exposed spent shell without requesting ammo");
 G.spentSampleAt=GetTickCount64()-20;G.spentLastPosition.y-=.02;c.leftGrip=0;tick();
 Check(!G.spentCarrying&&G.falling&&!G.pending,"releasing the spent shell throws it without reloading");
 Check(!Gm94Ready(),"spent shell removed but pump still forward: cannot fire");
 g_twoHand=true;wristAt({0,.2+kGm94BarrelTravel,0});tick();wristAt({0,.2,0});tick();
 Check(Gm94Ready(),"after manual extraction a single closing stroke chambers the launcher");
 g_twoHand=false;tick();
 // reload: cover open by hand
 const Mat4 box=Gm94CoverBox();const V3 nativePalm=Apply(Gm94KeyMat(kGm94LeftOnCoverOpenFromFrame),{-.591+kGm94Hand[0],.011+kGm94Hand[1],1.149+kGm94Hand[2]});Check(Length(Sub(nativePalm,{box.m[3],box.m[7],box.m[11]}))<1e-6,"latch box matches the captured grabbing palm, not an estimated cover offset");const V3 boxC={box.m[3],box.m[7],box.m[11]};palmAt(boxC);c.leftGrip=1;tick();Check(G.coverGrab&&g_gm94Clip.kind==1&&g_gm94Clip.scrub==0,"LEFT grip on the cover: captured grip, zero opening progress");
 auto lift=[&](double dy){g_handBody[1][7]+=static_cast<float>(dy);tick();};
 gunFrame.m[7]+=.1;g_handBody[1][7]+=.1f;tick();Check(g_gm94Clip.scrub<1e-5,"moving the gun and hand together does not open the cover");gunFrame.m[7]-=.1;g_handBody[1][7]-=.1f;tick();
 lift(.04);Check(g_gm94Clip.kind==1&&std::fabs(g_gm94Clip.scrub-.5)<.01,"hand up 4 cm: the cover half open with it (Jason's open)");
 lift(-.005);Check(std::fabs(g_gm94Clip.scrub-.5)<.01,"small opening-hand reversal cannot close the latch");lift(.005);
 lift(.05);Check(G.coverOpen&&!G.chambered&&!g_gm94Clip.kind,"hand up 9 cm: cover open, gun dry");
 c.leftGrip=0;tick();tick();tick();
 palmAt({.5,.5,.5});c.leftGrip=1;tick();c.x=true;tick();c.x=false;Check(G.carrying,"LEFT grip + X: one shell");
 palmAt({kGm94PortBox[0],kGm94PortBox[1],kGm94PortBox[2]});tick();Check(G.pending,"shell at the port: engine fill requested");fc3pad::CancelReloadRequest();
 G.pending=false;G.carrying=false;G.needsPump=true; // (engine seam: AK / 1887 fixtures)
 c.leftGrip=0;tick();
 // close the cover
 const Mat4 ob=Gm94CoverBox();const V3 obC={ob.m[3],ob.m[7],ob.m[11]};
 palmAt({.8,.8,.8});c.leftGrip=1;tick();Check(!G.coverGrab,"grip away from cover does not operate it");
 const V3 boundary=Apply(ob,{kGm94CoverBoxSize[0]*.5+Gm94CoverReach(g_worldScale)-.001,0,0});palmAt(boundary);tick();
 Check(G.coverGrab,"held grip entering expanded open-cover contact acquires close without another squeeze");lift(-.03);Check(g_gm94Clip.kind==2,"hand down: it starts closing with the hand");
 lift(-.06);Check(!G.coverOpen,"hand down 9 cm: cover closed");c.leftGrip=0;for(int i=0;i<4;++i)tick();
 Check(!Gm94Ready(),"after the reload a pump is due");
 g_twoHand=true;wristAt({0,.2,0});tick();wristAt({0,.2+kGm94BarrelTravel,0});tick();wristAt({0,.2,0});tick();Check(Gm94Ready(),"pumped: ready");
 c.rightTrigger=1;fc3pad::Publish(c);UpdateGm94Fire(c);Check(G.shot.pending,"next native shot request starts");
 G.shot.until=W7Now()-1;UpdateGm94Fire(c);Check(!G.shot.pending&&!G.spent&&G.chambered&&G.shot.waitRelease,"missed native shot preserves chamber");
 G.probing=true;Gm94Request(state,wrapper);Check(!G.recheckShot222&&!G.spent,"unchanged native probe releases missed-shot reconciliation");
 c.rightTrigger=0;UpdateGm94Fire(c);c.rightTrigger=1;UpdateGm94Fire(c);Check(G.shot.pending,"fresh pull recovers missed native shot");
 c.rightTrigger=0;G.shot.pending=false;UpdateGm94Fire(c);
 g_twoHand=oldTwo;fc3pad::BlockFire(false);fc3pad::SetXOwned(false);
 g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;VirtualFree(ammoPage,0,MEM_RELEASE);
 G=old;g_weaponMechanicProfile=oldProfile;g_worldScale=oldScale;memcpy(g_handBody,oldBody,sizeof(oldBody));fc3pad::Publish(fc3pad::Controllers{});
 { // the gun = right controller x the fixed right hand x inverse of Jason's idle grip - never the game's animated placement
  auto& L=g_gm94Lock;L=Gm94Lock{};g_handRightValid=true;g_twoHand=false;G=Gm94{};
  const Mat4 ctrl=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.1,.2,.3});for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(ctrl.m[i]);
  const Mat4 hand=TurnAndMove({0,0,1},{0,.6,.8},{0,0,0},{.02,.15,-.05});const auto keepFree=g_freeRightFromController;const UINT keepEpoch=g_freeRightEpoch;
  g_freeRightFromController=hand;g_freeRightEpoch=g_armPoseEpoch;++g_frame;
  Mat4 b={},gi={};Check(Gm94LockedBody(b)&&Invert(Gm94KeyMat(kGm94RightHandFromFrame),gi),"the gun is placed from the fixed right hand");
  const Mat4 want=Multiply(Multiply(ctrl,hand),gi);double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(b.m[i]-want.m[i]));
  Check(d<1e-6,"gun = controller x drawn hand x inverse(Jason's idle grip): sits in the hand as in the game's idle, no learning");
  const Mat4 wrist=Multiply(b,Gm94KeyMat(kGm94RightHandFromFrame));d=0;const Mat4 hw=Multiply(ctrl,hand);for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(wrist.m[i]-hw.m[i]));Check(d<1e-6,"the drawn wrist is exactly on the grip");
  g_freeRightFromController=keepFree;g_freeRightEpoch=keepEpoch;L=Gm94Lock{};}
 std::puts("PASS: GM-94: pump slides the barrel, tilt drops the spent shell, cover by hand, one shell per grab, pump after the reload.");
}

