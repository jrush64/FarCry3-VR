void TestFireOwnership222()
{
 const auto oldG=g_gm94;const auto oldLock=g_gm94Lock;const auto oldLever=g_lever;const auto oldRevolver=g_revolver;
 const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;
 const auto oldGC=g_gm94Clip;const auto oldFlare=g_flare;const auto oldRpg=g_rpg;const auto oldFlamer=g_flamer;
 const auto oldSpas=g_spas;const auto oldBow=g_bow;const auto oldBolt=g_r700;const auto oldReload=g_reload;const auto oldManual=g_manualReload;
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const UINT oldEpoch=g_armPoseEpoch;const auto oldClock=g_fixtureClock171,oldPause=g_w7PausedAt;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid;
 g_fixtureClock171=600000;g_w7PausedAt=0;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;
 NativeAmmoFixture219 ammo;fc3pad::Controllers c={};c.active=true;
 auto update=[&](){fc3pad::Publish(c);UpdateGm94Fire(c);};
 auto start=[&](){
  g_weaponMechanicProfile=18;g_weaponMechanicFrame=++g_frame;g_gm94={};g_gm94.known=true;
  g_gm94.ammoState=ammo.state;g_gm94.ammoWrapper=ammo.wrapper;
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=3;
  c={};c.active=true;update();c.rightTrigger=1;update();
  Check(g_gm94.shot.pending&&g_gm94.chambered&&!g_gm94.spent,"222 GM94 opens native request without inventing shot");
 };
 for(int fault=0;fault<6;++fault){
  start();ammo.count()=2;
  if(fault==1){*reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon+512;*reinterpret_cast<UINT*>(ammo.weapon+512+0xcc)=0;}
  if(fault==2)++g_armPoseEpoch;
  if(fault==3)g_handRightValid=false;
  if(fault==4)g_gm94.coverOpen=true;
  if(fault==5)TickGm94(false); // cancellation must work even when UpdatePump isn't called
  update();
  if(fault==0)Check(g_gm94.spent&&!g_gm94.chambered&&!g_gm94.shot.pending,"222 same-owner native decrement confirms spent chamber");
  else Check(!g_gm94.spent&&g_gm94.chambered&&!g_gm94.shot.pending,"222 replaced owner/epoch/tracking/open cover/inactive tick cannot confirm old shot");
  g_armPoseEpoch=oldEpoch;g_handRightValid=true;
 }
 // Timeout is followed by the existing native probe, using the retained owner.
 for(int result=0;result<3;++result){
  start();g_fixtureClock171+=181;update();
  Check(!g_gm94.shot.pending&&g_gm94.recheckShot222&&!g_gm94.spent&&!g_gm94.known,"222 timeout retains chamber and schedules native reconciliation");
  c.rightTrigger=0;update();c.rightTrigger=1;update();
  Check(!g_gm94.shot.pending&&fc3pad::FireOutput()==0,"222 retry cannot overwrite unresolved late-shot baseline");
  if(result==1)ammo.count()=2;
  if(result==2){*reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon+512;*reinterpret_cast<UINT*>(ammo.weapon+512+0xcc)=1;}
  g_gm94.probing=true;Gm94Request(ammo.state,ammo.wrapper);
  Check(!g_gm94.recheckShot222&&g_gm94.known,"222 real local probe completes shot reconciliation");
  Check(result==1?g_gm94.spent&&!g_gm94.chambered:!g_gm94.spent&&g_gm94.chambered,"222 late decrement confirms only unchanged native owner; unchanged/replaced owner preserves chamber");
 }
 start();const auto until=g_gm94.shot.until;
 W7Resume(500);g_fixtureClock171+=500;update();
 Check(g_gm94.shot.pending&&g_gm94.shot.until==until+500,"222 Insert pause preserves GM94 native request deadline");
 fc3stereo::ResumeNativeWeaponTimers(g_fixtureClock171,700);g_fixtureClock171+=700;update();
 Check(g_gm94.shot.pending&&g_gm94.shot.until==until+1200,"222 native-menu pause preserves same GM94 deadline");
 // The 1887 grace window must never override a mechanism/reload hard lock.
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;
 auto lock=[&](int kind){if(kind==0)g_lever.hold=true;if(kind==1)g_lever.leverOpen=true;if(kind==2)g_lever.grabbing=true;if(kind==3)g_lever.clip=0;};
 auto leverUpdate=[&](){fc3pad::Publish(c);UpdateLever1887Fire(c);};
 for(int kind=0;kind<4;++kind){
  g_lever={};c={};c.active=true;leverUpdate();c.rightTrigger=1;leverUpdate();
  Check(g_lever.passing&&!g_lever.chambered&&fc3pad::FireOutput()==255,"222 ordinary 1887 native input grace remains available");
  lock(kind);leverUpdate();Check(!g_lever.passing&&fc3pad::FireOutput()==0,"222 1887 hard lock immediately closes existing input grace");
  g_lever={};lock(kind);c.rightTrigger=0;leverUpdate();c.rightTrigger=1;leverUpdate();
  Check(g_lever.chambered&&!g_lever.passing&&fc3pad::FireOutput()==0,"222 blocked 1887 trigger request cannot consume physical chamber");
 }
 g_lever={};g_handRightValid=false;c.rightTrigger=1;leverUpdate();
 Check(g_lever.chambered&&!g_lever.passing,"222 missing tracking cannot consume 1887 chamber");g_handRightValid=true;
 // An interrupted cylinder grab/flick/tip must not retain an old motion origin.
 g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;g_revolver={};
 g_revolver.grab=true;g_revolver.grabStart={-10,0,0};g_revolver.tiltSince=g_revolver.rightPrevAt=1;
 c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);g_handLeftValid=false;TickRevolver(true);
 Check(!g_revolver.grab&&!g_revolver.tiltSince&&!g_revolver.rightPrevAt&&!g_revolver.open,"222 lost .44 tracking drops cylinder/flick/tip origins without opening");
 g_handLeftValid=true;g_revolver.leftFrame=g_revolver.frameFrame=g_frame;g_revolver.frameBody=g_revolver.leftWrist=HandIdentity();g_revolver.probeAt=g_fixtureClock171;
 TickRevolver(true);Check(!g_revolver.grab&&!g_revolver.open,"222 continued grip after tracking recovery cannot resume old cylinder stroke");
 fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::CancelReloadRequest();fc3pad::SetXOwned(false);
 g_gm94=oldG;g_gm94Lock=oldLock;g_lever=oldLever;g_revolver=oldRevolver;g_pistolMenu=oldMenu;
 g_gm94Clip=oldGC;g_flare=oldFlare;g_rpg=oldRpg;g_flamer=oldFlamer;g_spas=oldSpas;g_bow=oldBow;g_r700=oldBolt;g_reload=oldReload;g_manualReload=oldManual;
 memcpy(g_magazineStates,oldStates,sizeof(oldStates));
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_armPoseEpoch=oldEpoch;g_fixtureClock171=oldClock;g_w7PausedAt=oldPause;
 g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;fc3pad::Publish(oldControls);
 puts("PASS: fire222 GM94 owner/epoch/tracking/mechanism confirmation, delayed-shot probe and pause clocks; 1887 hard locks beat grace; .44 lost tracking resets cylinder/flick/tip origins");
}
