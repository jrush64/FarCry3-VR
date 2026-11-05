// RPG-7: clip data sanity, and the cycle shot -> rocket in the left hand -> tail at the tube mouth -> Jason's insert
// (left + right hand and the rocket from the captured clip) -> seated, fire live. The engine load itself is the AK/1887 seam.
void TestRpg()
{
 const auto old=g_rpg;const LONG oldProfile=g_weaponMechanicProfile;const double oldScale=g_worldScale;g_worldScale=1;
 {fc3weapon::Geometry g={201024,43572,32,0,0,21786,57};Check(fc3weapon::ProfileIndex(g)==32&&strcmp(fc3weapon::kProfileNames[32],"rpg7.xbg")==0,"RPG-7 body draw = profile 32");}
 {const auto& k=kRpgClipReload[RpgFirstKey()];const V3 o={k.rocket[3],k.rocket[7],k.rocket[11]};
  Check(Length(Sub(o,{kRpgTubeMouthRocketOriginFromFrame[0],kRpgTubeMouthRocketOriginFromFrame[1],kRpgTubeMouthRocketOriginFromFrame[2]}))<.01,"the insert starts with the rocket at the tube mouth");
  const auto& e=kRpgClipReload[kRpgClipReloadCount-1];Check(std::fabs(e.rocket[7]-kRpgRocketRestFromFrame[7])<.003&&std::fabs(e.rocket[11]-kRpgRocketRestFromFrame[11])<.003,"the insert ends with the rocket seated");
  bool mono=true;for(int i=1;i<kRpgClipReloadCount;++i)mono=mono&&kRpgClipReload[i].t>kRpgClipReload[i-1].t;Check(mono,"clip time increases");}
 g_weaponMechanicProfile=32;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 auto& R=g_rpg;R=Rpg{};R.known=true;R.loaded=true;
 NativeAmmoFixture219 native;R.ammoState=native.state;R.ammoWrapper=native.wrapper;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 const Mat4 gun=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.2,-.1,.3});
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;R.frameBody=gun;R.frameFrame=g_frame;R.leftFrame=g_frame;fc3pad::Publish(c);TickRpg(true);};
 auto rocketAt=[&](const V3& f){Mat4 local=HandIdentity();const V3 point=RpgInsertionAt(HandIdentity());local.m[3]=f.x-point.x;local.m[7]=f.y-point.y;local.m[11]=f.z-point.z;R.leftWrist=Multiply(gun,local);};
 rocketAt({.5,.5,.5});tick();
 c.rightTrigger=1;UpdateRpgFire(c);Check(R.loaded&&R.shot.pending,"trigger request preserves rocket until native shot");native.count()=0;UpdateRpgFire(c);c.rightTrigger=0;UpdateRpgFire(c);Check(!R.loaded,"shot: tube empty");
 c.rightTrigger=1;UpdateRpgFire(c);c.rightTrigger=0;UpdateRpgFire(c);Check(!R.loaded,"dry while empty");
 c.leftGrip=1;tick();c.x=true;tick();c.x=false;Check(R.pending,"LEFT grip + X: rocket requested from the engine");fc3pad::CancelReloadRequest();
 R.pending=false;R.carrying=true;native.count()=1; // (the engine seam loads it: AK / 1887 fixtures)
 tick();Check(R.carrying&&!R.clip,"rocket rides in the left hand");
 c.leftGrip=0;tick();Check(R.carrying,"letting go does not drop it");
 rocketAt({kRpgTubeMouthRocketOriginFromFrame[0],kRpgTubeMouthRocketOriginFromFrame[1]+.05f,kRpgTubeMouthRocketOriginFromFrame[2]});tick();
 Check(R.clip&&!R.carrying,"palm at the tube mouth: Jason's insert plays");
 {const auto& p=RpgClipPose();const auto& k=kRpgClipReload[RpgFirstKey()];Check(std::fabs(p.left.m[3]-k.leftHand[3])<.02&&std::fabs(p.right.m[7]-k.rightHand[7])<.02,"both wrists come from the clip");}
 c.rightTrigger=1;UpdateRpgFire(c);c.rightTrigger=0;UpdateRpgFire(c);Check(!R.loaded,"dry while Jason inserts");
 R.clipAt-=10000;tick();Check(!R.clip&&R.loaded,"clip done: seated");
 c.rightTrigger=1;UpdateRpgFire(c);native.count()=0;UpdateRpgFire(c);Check(!R.loaded,"fires");c.rightTrigger=0;UpdateRpgFire(c);
 fc3pad::BlockFire(false);fc3pad::SetXOwned(false);
 R=old;g_weaponMechanicProfile=oldProfile;g_worldScale=oldScale;fc3pad::Publish(fc3pad::Controllers{});
 std::puts("PASS: RPG-7: rocket by hand, Jason's insert from the tube mouth (both hands + rocket), seated before it fires.");
}
