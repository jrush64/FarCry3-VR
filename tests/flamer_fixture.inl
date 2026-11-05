// Flamethrower: captured lever kinematics, palette posing from FRAME, and lever down -> canister out by hand ->
// new canister -> seat -> lever up.
void TestFlamer()
{
 const auto old=g_flamer;const LONG oldProfile=g_weaponMechanicProfile;const double oldScale=g_worldScale;g_worldScale=1;
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 {fc3weapon::Geometry g={349160,68436,40,0,20838,13380,57};Check(fc3weapon::ProfileIndex(g)==15&&strcmp(fc3weapon::kProfileNames[15],"flamethrower.xbg")==0,"flamethrower draw = profile 15");}
 auto& F=g_flamer;F=Flamer{};
 Check(closeMat(FlamerHandle(),FlamerKeyMat(kFlamerLeverUpFromFrame),1e-9),"lever up, not held: handle at its rest");
 F.lever=1;Check(closeMat(Multiply(FlamerCrank(),FlamerKeyMat(kFlamerCrankUpFromFrame)),FlamerKeyMat(kFlamerCrankDownFromFrame),1e-3),"crank down = the captured crank");
 Check(closeMat(FlamerHandle(),FlamerKeyMat(kFlamerLeverDownFromFrame),5e-3),"handle down = the captured handle");
 F.lever=0;F.leverGrab=true;Check(closeMat(FlamerHandle(),FlamerKeyMat(kFlamerLeverUnfoldedFromFrame),.03),"grabbed: the handle unfolds");F.leverGrab=false;
 // palette posing: the canister slot sits at FRAME x rest x T(-bind)
 {float bones[48*12]={};const Mat4 W=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});
  const Mat4 framePal=W;F.framePal=framePal;F.palFrame=g_frame;
  Check(closeMat(Multiply(BoneMatrix(bones,0),HandIdentity()),BoneMatrix(bones,0),1e-12),"fixture");
  FlamerStore(bones,0,framePal,FlamerCanisterRest(),kFlamerBindClip);
  Check(closeMat(BoneMatrix(bones,0),Multiply(framePal,Multiply(FlamerCanisterRest(),FlamerTranslate(kFlamerBindClip,-1))),1e-5),"canister slot = FRAME x rest x T(-bind)");}
 g_weaponMechanicProfile=15;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 F=Flamer{};F.known=true;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 Mat4 gunFrame=HandIdentity();gunFrame.m[5]=0;gunFrame.m[6]=1;gunFrame.m[9]=-1;gunFrame.m[10]=0;
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;F.frameBody=gunFrame;F.frameFrame=g_frame;F.leftFrame=g_frame;fc3pad::Publish(c);TickFlamer(true);};
 auto palmAt=[&](const V3& f){const V3 p=Apply(gunFrame,f);F.leftWrist=HandIdentity();F.leftWrist.m[3]=p.x-(-.591+kFlamerHand[0]);F.leftWrist.m[7]=p.y-(.011+kFlamerHand[1]);F.leftWrist.m[11]=p.z-(1.149+kFlamerHand[2]);};
 auto canisterAt=[&](const V3& f){const Mat4 want=Multiply(gunFrame,TurnAndMove({0,1,0},{0,1,0},{0,0,0},f));Mat4 i={};Invert(FlamerKeyMat(kFlamerCanisterFromLeftWristReturn),i);F.leftWrist=Multiply(want,i);};
 palmAt({.5,.5,.5});tick();Check(FlamerReady(),"ready");
 const Mat4 lb=FlamerLeverBox();const V3 lever={lb.m[3],lb.m[7],lb.m[11]};
 palmAt(lever);c.leftGrip=1;tick();Check(F.leverGrab&&g_flamerClip.kind==1&&g_flamerClip.scrub==0,"LEFT grip captures the lever pose without advancing it");
 // the lever is driven by the REAL left controller (the drawn hand is pinned to the handle): up/back opens, down/forward closes
 auto ctrlMove=[&](double dx,double dy,double dz){g_handBody[1][3]+=static_cast<float>(dx);g_handBody[1][7]+=static_cast<float>(dy);g_handBody[1][11]+=static_cast<float>(dz);};
 gunFrame.m[7]+=.1;ctrlMove(0,.1,0);tick();Check(g_flamerClip.scrub==0,"moving the gun and controller together does not operate the lever");gunFrame.m[7]-=.1;ctrlMove(0,-.1,0);tick();
 // gunFrame: FRAME +Y (muzzle) = body -Z, so BACK = body +Z, UP = body +Y
 ctrlMove(.08,0,0);tick();Check(!g_flamerClip.kind||g_flamerClip.scrub<.1,"sideways: the lever does not move");ctrlMove(-.08,0,0);
 ctrlMove(0,0,-.09);tick();Check(!g_flamerClip.kind||g_flamerClip.scrub<.1,"forward/back: the lever does not move (height only)");ctrlMove(0,0,.09);
 ctrlMove(0,-.04,0);tick();Check(g_flamerClip.kind==1&&std::fabs(g_flamerClip.scrub-.5)<.01&&!F.leverDown&&!FlamerReady(),"hand down 4 cm: lever half way down, with the hand");
 {const auto& p=FlamerClipPose();Check(Length(Sub(V3{p.handle.m[3],p.handle.m[7],p.handle.m[11]},V3{kFlamerLeverUpFromFrame[3],kFlamerLeverUpFromFrame[7],kFlamerLeverUpFromFrame[11]}))<.05,"the clip starts from the lever at rest");}
 ctrlMove(0,.04,0);tick();Check(g_flamerClip.scrub==0,"returning to the start rewinds the lever");ctrlMove(0,-.04,0);tick();
 ctrlMove(0,-.05,0);tick();const Mat4 settledDown=FlamerHandle();c.leftGrip=0;tick();Check(F.leverDown&&F.lever>.99&&!g_flamerClip.kind,"hand down 9 cm: lever all the way down and stays");
 {const Mat4 after=FlamerHandle();double jump=0;for(int i=0;i<12;++i)jump=(std::max)(jump,std::fabs(after.m[i]-settledDown.m[i]));Check(jump<1e-6,"letting go retains the completed captured lever pose");}
 const V3 seat={kFlamerCanisterRestFromFrame[3],kFlamerCanisterRestFromFrame[7],kFlamerCanisterRestFromFrame[11]};
 palmAt(seat);c.leftGrip=1;tick();Check(F.carrying&&F.carryingOld&&!F.seated,"LEFT grip on the canister: out into the left hand");
 canisterAt(Add(seat,{0,-.3,-.2}));c.leftGrip=0;tick();Check(!F.carrying&&F.falling,"let go away from the gun: it falls");
 palmAt({.5,.5,.5});c.leftGrip=1;tick();c.x=true;tick();c.x=false;Check(F.carrying&&!F.carryingOld,"LEFT grip + X: new canister");
 canisterAt(Add(seat,{0,-.3,-.2}));tick();canisterAt(seat);tick();Check(F.pending,"new canister at its seat: engine fill requested");fc3pad::CancelReloadRequest();
 F.pending=false;F.carrying=false;F.seated=true;F.replaced=true; // (engine seam: AK / 1887 fixtures)
 c.leftGrip=0;tick();Check(!FlamerReady(),"lever still down: dry");
 const Mat4 lb2=FlamerLeverBox();const V3 leverDown={lb2.m[3],lb2.m[7],lb2.m[11]};
 palmAt(leverDown);c.leftGrip=1;tick();Check(F.leverGrab,"grab the lever again");
 ctrlMove(0,.03,0);tick();Check(g_flamerClip.kind==2,"hand up 3 cm: the lever starts up with it");
 c.leftGrip=0;tick();Check(F.leverDown&&!g_flamerClip.kind&&F.lever<.7&&F.lever>.5,"let go part-way: preserves partial position");
 const Mat4 partialBox=FlamerLeverBox();c.leftGrip=1;palmAt({partialBox.m[3],partialBox.m[7],partialBox.m[11]});tick();Check(F.leverGrab&&g_flamerClip.scrub>.3,"regrab resumes the partial lever position");
 ctrlMove(0,.09,0);tick();Check(!F.leverDown&&F.lever<.01,"hand up 9 cm: lever up, locked");
 c.leftGrip=0;tick();Check(FlamerReady(),"fire live");
 fc3pad::BlockFire(false);fc3pad::SetXOwned(false);
 F=old;g_weaponMechanicProfile=oldProfile;g_worldScale=oldScale;fc3pad::Publish(fc3pad::Controllers{});
 std::puts("PASS: flamethrower: captured lever, lever down -> canister out by hand -> new canister -> seat -> lever up.");
}
