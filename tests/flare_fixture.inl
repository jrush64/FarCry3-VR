// Flare gun: captured hinge math, and the play cycle shot -> open by hand -> spent shell out -> new shell -> close
// only by a backwards flick (the engine fill itself is the revolver/1887 seam, exercised there).
void TestFlare()
{
 const auto old=g_flare;const LONG oldProfile=g_weaponMechanicProfile;const double oldScale=g_worldScale;g_worldScale=1;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 {fc3weapon::Geometry g={91584,18168,32,0,3708,4002,57};Check(fc3weapon::ProfileIndex(g)==16&&strcmp(fc3weapon::kProfileNames[16],"flare_gun.xbg")==0,"flare gun main draw = profile 16");}
 auto& F=g_flare;F=Flare{};
 F.angle=kFlareBarrelOpenDeg;Check(closeMat(FlareBarrel(),FlareKeyMat(kFlareBarrelOpenFromFrame),2e-3),"hinge at 72.4 deg = the captured open barrel");
 {const Mat4 box=FlareBox(1);Check(Length(Sub(V3{box.m[3],box.m[7],box.m[11]},V3{kFlareChamberMouthFromFrameOpen[0],kFlareChamberMouthFromFrameOpen[1],kFlareChamberMouthFromFrameOpen[2]}))<1e-3,"chamber box sits on the captured chamber mouth (open)");
  const Mat4 seat=Multiply(FlareBarrel(),FlareKeyMat(kFlareShellFromBarrelSeat));Check(Length(Sub(V3{seat.m[3],seat.m[7],seat.m[11]},V3{kFlareChamberSeatFromFrameOpen[0],kFlareChamberSeatFromFrameOpen[1],kFlareChamberSeatFromFrameOpen[2]}))<1e-3,"seated shell rides the open barrel at the captured seat");}
 F.angle=0;Check(closeMat(FlareBarrel(),FlareBarrelRest(),1e-9)&&std::fabs(FlareBarrelRest().m[7]-.1957)<.001,"closed barrel = its recorded rest (0, .196, .030), not FRAME");
 {const Mat4 h=FlareHammer(kFlareHammerCockedDeg);Check(Length(Sub(Apply(h,{kFlareHammerPivot[0],kFlareHammerPivot[1],kFlareHammerPivot[2]}),{kFlareHammerPivot[0],kFlareHammerPivot[1],kFlareHammerPivot[2]}))<1e-9,"hammer turns about its pivot");}
 // play
 g_weaponMechanicProfile=16;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 F.known=true;F.chamber=1;
 NativeAmmoFixture219 native;F.ammoState=native.state;F.ammoWrapper=native.wrapper;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 for(int i=0;i<16;++i)g_handBody[0][i]=g_handBody[1][i]=i%5==0?1.f:0.f;
 Mat4 gunFrame=HandIdentity();gunFrame.m[5]=0;gunFrame.m[6]=1;gunFrame.m[9]=-1;gunFrame.m[10]=0; // FRAME +Y -> body -Z (level), FRAME +Z -> body +Y
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;F.frameBody=gunFrame;F.frameFrame=g_frame;F.leftFrame=g_frame;F.lastTick=GetTickCount64()-100;fc3pad::Publish(c);TickFlare(true);};
 auto palmAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);F.leftWrist=HandIdentity();F.leftWrist.m[3]=p.x-(-.591+kFlareHand[0]);F.leftWrist.m[7]=p.y-(.011+kFlareHand[1]);F.leftWrist.m[11]=p.z-(1.149+kFlareHand[2]);};
 palmAt({.5,.5,.5});tick();Check(FlareActive()&&!F.open,"flare gun active, closed");
 c.rightTrigger=1;UpdateFlareFire(c);Check(F.chamber==1&&F.shot.pending,"trigger request preserves flare until native shot");native.count()=0;UpdateFlareFire(c);c.rightTrigger=0;UpdateFlareFire(c);Check(F.chamber==2&&F.hammerDown,"shot: spent shell in, hammer down");
 c.rightTrigger=1;UpdateFlareFire(c);Check(F.chamber==2,"dry: a second pull does not fire");c.rightTrigger=0;UpdateFlareFire(c);
 // open: grip the barrel and pull the muzzle down (FRAME -Z)
 palmAt({0,kFlareBarrelRest[1],kFlareBarrelRest[2]+.02f});c.leftGrip=1;tick();Check(F.grab,"LEFT grip on the barrel grabs it");
 palmAt({0,kFlareBarrelRest[1],kFlareBarrelRest[2]-.02f});tick();Check(F.open&&!F.grab,"pulled the muzzle down 3 cm: the barrel breaks open");
 c.leftGrip=0;tick();tick();tick();tick();Check(std::fabs(F.angle-kFlareBarrelOpenDeg)<.5,"barrel swings fully open");
 // new shell refused while the spent one is in
 c.leftGrip=1;c.x=true;palmAt({.5,.5,.5});tick();Check(!F.carrying,"LEFT grip + X with the spent shell still in: nothing");c.x=false;c.leftGrip=0;tick();
 // spent shell out by hand
 palmAt({kFlareChamberMouthFromFrameOpen[0],kFlareChamberMouthFromFrameOpen[1],kFlareChamberMouthFromFrameOpen[2]});c.leftGrip=1;tick();
 Check(F.carrying&&F.carryingSpent&&F.chamber==0,"LEFT grip at the chamber: spent shell in the left hand");
 c.leftGrip=0;tick();Check(!F.carrying&&F.falling,"let go: it falls");
 // new shell
 palmAt({.5,.5,.5});c.leftGrip=1;tick();c.x=true;tick();Check(F.carrying&&!F.carryingSpent,"LEFT grip + X: new shell");c.x=false;
 palmAt({kFlareChamberMouthFromFrameOpen[0],kFlareChamberMouthFromFrameOpen[1],kFlareChamberMouthFromFrameOpen[2]});tick();
 Check(F.pending,"new shell at the chamber: engine fill requested");fc3pad::CancelReloadRequest();
 F.pending=false;F.carrying=false;F.chamber=1;native.count()=1; // (the engine seam fills it: see the revolver / 1887 fixtures)
 c.leftGrip=0;tick();
 // close only by a backwards flick
 c.x=true;tick();c.x=false;tick();Check(F.open,"X does not close it");
 tick();g_handBody[0][11]+=.02f;tick();Check(F.open,"slow move back: still open");
 F.rightPrevAt=GetTickCount64()-20;F.rightPrev={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};g_handBody[0][11]+=.05f;g_handBody[0][7]+=.02f;
 ++g_frame;g_weaponMechanicFrame=g_frame;F.frameBody=gunFrame;F.frameFrame=g_frame;F.leftFrame=g_frame;fc3pad::Publish(c);TickFlare(true);
 Check(!F.open&&!F.hammerDown,"backwards flick (toward you, up): closed, hammer cocked");
 for(int i=0;i<6;++i)tick();Check(F.angle<.5,"barrel back closed");
 c.rightTrigger=1;UpdateFlareFire(c);native.count()=0;UpdateFlareFire(c);Check(F.chamber==2,"fires again");c.rightTrigger=0;UpdateFlareFire(c);
 fc3pad::BlockFire(false);fc3pad::SetXOwned(false);
 F=old;g_weaponMechanicProfile=oldProfile;g_worldScale=oldScale;memcpy(g_handBody,oldBody,sizeof(oldBody));fc3pad::Publish(fc3pad::Controllers{});
 std::puts("PASS: flare gun: captured hinge/seat, shot -> open by hand -> spent out -> new shell -> close only by a backwards flick.");
}
