void TestClipPause223()
{
 const auto oldL=g_lever;const auto oldR=g_revolver;const auto oldS=g_spas;const auto oldB=g_r700;
 const auto oldFlare=g_flare;const auto oldRpg=g_rpg;const auto oldG=g_gm94;const auto oldF=g_flamer;const auto oldBow=g_bow;
 const auto oldReload=g_reload;const auto oldManual=g_manualReload;const auto oldLock=g_gm94Lock;
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const auto oldClock=g_fixtureClock171,oldUi=g_w7PausedAt;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand;
 const auto oldBegin=g_nativeReloadBegin;const auto oldEnd=g_nativeReloadEnd;const auto oldTransfer=g_nativeReload;
 const auto oldHud=g_leverFixtureHud;const bool oldOverfill=g_leverFixtureOverfill;
 g_fixtureClock171=900000;g_w7PausedAt=0;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 NativeAmmoFixture219 ammo;const UINT hud=reinterpret_cast<UINT>(ammo.page)+16384;
 g_reload={};g_reload.widgets[0].wrapper=LONG(ammo.wrapper);g_reload.widgets[0].hud=LONG(hud);
 *reinterpret_cast<UINT*>(hud+0x134)=3;*reinterpret_cast<UINT*>(hud+0x138)=6;*reinterpret_cast<UINT*>(hud+0x140)=40;
 g_leverFixtureHud=hud;g_leverFixtureOverfill=true;g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=SeatTransfer221;
 fc3pad::Controllers c={};c.active=true;c.leftGrip=c.rightGrip=1;fc3pad::Publish(c);
 auto tick=[&](int profile){g_weaponMechanicFrame=++g_frame;if(profile==42)TickRevolver(true);else if(profile==34)TickSpas(true);else if(profile==0)TickLever1887(true);else TickR700(true);};
 auto seconds=[&](int profile){return profile==42?RevolverClipSeconds():profile==34?SpasClipSeconds():profile==0?WeaponClipElapsed223(g_lever.clipAt,g_lever.clipPauseAt223):R700ClipSeconds();};
 auto pose=[&](int profile){return profile==42?RevolverClipPose().hand:profile==34?SpasClipPose().hand:profile==0?Lever1887SimplePose().hand:R700ClipPose().hand;};
 auto playing=[&](int profile){return profile==42?g_revolver.clip:profile==34?g_spas.clip:profile==0?g_lever.clip:g_r700.clip;};
 auto pause=[&](int profile){return profile==42?g_revolver.clipPauseAt223:profile==34?g_spas.clipPauseAt223:profile==0?g_lever.clipPauseAt223:g_r700.clipPauseAt223;};
 auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>2e-5)return false;return true;};
 for(int profile:{42,34,0,2,23})for(int kind=0;kind<(profile==42?2:profile==0?5:1);++kind){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_handLeftValid=true;ammo.count()=3;
  g_lever={};g_revolver={};g_spas={};g_r700={};
  if(profile==42){g_revolver.ammoState=ammo.state;g_revolver.ammoWrapper=ammo.wrapper;g_revolver.carrying=true;RevolverStartClip(kind);}
  else if(profile==34){g_spas.carrying=true;SpasStartClip();}
  else if(profile==0){g_lever.simple=true;g_lever.chambered=false;Lever1887StartClip(kind);}
  else{g_r700.physical=false;g_r700.chambered=false;g_r700.clip=0;g_r700.clipAt=g_fixtureClock171;}
  const auto initial=seconds(profile);const auto initialPose=pose(profile);const int initialClip=playing(profile);
  const ULONGLONG deadline=g_fixtureClock171+1000;
  if(profile==42){g_revolver.pending=true;g_revolver.deadline=deadline;}
  if(profile==34){g_spas.pending=true;g_spas.deadline=deadline;}
  if(profile==0){g_lever.pending=true;g_lever.deadline=deadline;}
  g_handLeftValid=false;tick(profile);Check(pause(profile)!=0,"223 tracking loss pauses active local clip");
  g_fixtureClock171+=5000;tick(profile);
  Check(playing(profile)==initialClip&&fabs(seconds(profile)-initial)<1e-8&&close(pose(profile),initialPose),"223 five seconds missing tracking cannot advance displayed clip pose or finish cycle");
  g_seatCalls221=0;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(g_seatCalls221==0&&ammo.count()==3,"223 paused timed insertion cannot enter native transfer");
  // UI sampling must freeze too; resuming the UI shifts both tracking origins.
  g_w7PausedAt=g_fixtureClock171;g_fixtureClock171+=700;
  Check(fabs(seconds(profile)-initial)<1e-8,"223 getter remains frozen during nested UI pause");
  g_w7PausedAt=0;W7Resume(700);
  fc3stereo::ResumeNativeWeaponTimers(g_fixtureClock171,900);g_fixtureClock171+=900;
  Check(fabs(seconds(profile)-initial)<1e-8,"223 Insert/native overlap does not double-count paused pose time");
  g_handLeftValid=true;tick(profile);
  Check(pause(profile)==0&&playing(profile)==initialClip&&fabs(seconds(profile)-initial)<1e-8,"223 recovered clip resumes at original time without hidden seating or chambering");
  if(profile==42)Check(g_revolver.pending&&g_revolver.deadline==deadline+6600,"223 speedloader lease survives tracking plus both menus exactly once");
  if(profile==34)Check(g_spas.pending&&g_spas.deadline==deadline+6600,"223 SPAS shell lease survives tracking plus both menus exactly once");
  if(profile==0)Check(g_lever.pending&&g_lever.deadline==deadline+6600,"223 Simple shell lease survives tracking plus both menus exactly once");
  if(profile==2||profile==23){
   Check(!g_r700.chambered,"223 interrupted Simple bolt does not chamber on recovery");
   g_fixtureClock171+=ULONGLONG(Bolt().cycleSeconds*1000)+1;tick(profile);
   Check(g_r700.chambered&&g_r700.clip<0&&g_r700.cycles==1,"223 real remaining Simple bolt duration still completes exactly once");
  }
  g_handLeftValid=false;tick(profile);g_weaponMechanicProfile=7;tick(profile);
  Check(pause(profile)==0&&playing(profile)<0,"223 switching weapons discards old clip pause origin");
  fc3pad::CancelReloadRequest();
 }
 // A request rejected by UI ownership must be retried after controls return.
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_handLeftValid=true;g_lever={};g_lever.simple=true;
 Lever1887StartClip(kL1887ShellFirst);g_lever.pending=g_lever.insertFired=true;g_lever.deadline=g_fixtureClock171+1500;g_lever.clipPulseAt223=g_fixtureClock171;
 fc3pad::CancelReloadRequest();fc3pad::SetPistolCalibration(true);fc3pad::RequestReload();fc3pad::SetPistolCalibration(false);
 g_fixtureClock171+=251;TickLever1887Simple(false,false);
 Check(g_lever.pending&&fc3pad::ReloadOutput(),"223 outstanding Simple shell retries the same lease after UI-rejected pulse");
 const auto& last223=kL1887Clips[kL1887ShellFirst];g_lever.clipAt=g_fixtureClock171-ULONGLONG(last223.keys[last223.count-1].t*1000)-1;
 TickLever1887Simple(false,false);Check(g_lever.clip==kL1887ShellFirst&&g_lever.pending,"223 Simple shell holds final seat pose while native lease remains unanswered");
 g_w7PausedAt=0;Check(WeaponClipElapsed223(g_fixtureClock171+1,0)==0,"223 future start cannot underflow elapsed time");
 g_lever=oldL;g_revolver=oldR;g_spas=oldS;g_r700=oldB;g_flare=oldFlare;g_rpg=oldRpg;g_gm94=oldG;g_flamer=oldF;g_bow=oldBow;g_reload=oldReload;g_manualReload=oldManual;g_gm94Lock=oldLock;memcpy(g_magazineStates,oldStates,sizeof(oldStates));
 g_nativeReloadBegin=oldBegin;g_nativeReloadEnd=oldEnd;g_nativeReload=oldTransfer;g_leverFixtureHud=oldHud;g_leverFixtureOverfill=oldOverfill;
 g_pistolMenu=oldMenu;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_fixtureClock171=oldClock;g_w7PausedAt=oldUi;
 g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;
 fc3pad::SetXOwned(false);fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);fc3pad::CancelReloadRequest();fc3pad::Publish(oldControls);
 puts("PASS: clips223 .44 insert/close, SPAS insert, five 1887 Simple clips and R700/Z93 bolts freeze time/pose on lost tracking, block native seating, resume menu overlaps/leases once, and clear on switch");
}
