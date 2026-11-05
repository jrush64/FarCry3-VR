// Exercise real contact/reload code. Fixture memory alone simulates native ammo.
UINT g_seatCalls221=0;bool g_seatRefuse221=false;
void __stdcall SeatTransfer221(UINT state,UINT wrapper,UINT flag,int count)
{
 ++g_seatCalls221;
 if(!g_seatRefuse221)Lever1887FixtureTransfer(state,wrapper,flag,count);
}
void TestShellRecovery221()
{
 const auto oldLever=g_lever;const auto oldSpas=g_spas;const auto oldRevolver=g_revolver;
 const auto oldFlare=g_flare;const auto oldRpg=g_rpg;const auto oldGm=g_gm94;const auto oldFlamer=g_flamer;
 const auto oldReload=g_reload;const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;
 const auto oldBegin=g_nativeReloadBegin;const auto oldEnd=g_nativeReloadEnd;const auto oldTransfer=g_nativeReload;
 const auto oldClock=g_fixtureClock171;const UINT oldHud=g_leverFixtureHud;const bool oldOverfill=g_leverFixtureOverfill;
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand;
 g_fixtureClock171=400000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 NativeAmmoFixture219 ammo;const UINT hud=reinterpret_cast<UINT>(ammo.page)+16384;
 auto word=[&](UINT off)->UINT&{return *reinterpret_cast<UINT*>(hud+off);};
 g_leverFixtureHud=hud;g_leverFixtureOverfill=true;g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=SeatTransfer221;
 fc3pad::Controllers c={};c.active=true;c.leftGrip=c.rightGrip=1;fc3pad::Publish(c);
 for(int profile:{0,34,42,16,32,18,15})for(int failure=0;failure<7;++failure){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;
  g_lever={};g_spas={};g_revolver={};g_flare={};g_rpg={};g_gm94={};g_flamer={};g_reload={};
  g_reload.widgets[0].wrapper=static_cast<LONG>(ammo.wrapper);g_reload.widgets[0].hud=static_cast<LONG>(hud);
  ammo.count()=profile==16||profile==32?0:3;word(0x134)=ammo.count();word(0x138)=profile==16||profile==32?1:6;word(0x140)=40;
  *reinterpret_cast<UINT*>(ammo.state+8)=0;
  g_lever.hold=g_lever.leverOpen=g_lever.pending=true;g_lever.chambered=false;g_lever.deadline=g_fixtureClock171+1000;
  g_spas.pending=g_spas.carrying=true;g_spas.deadline=g_fixtureClock171+1000;
  g_revolver.pending=g_revolver.carrying=g_revolver.empty=true;g_revolver.deadline=g_fixtureClock171+1000;
  g_flare.pending=g_flare.carrying=true;g_flare.deadline=g_fixtureClock171+1000;
  g_rpg.pending=true;g_rpg.loaded=false;g_rpg.deadline=g_fixtureClock171+1000;
  g_gm94.pending=g_gm94.carrying=true;g_gm94.deadline=g_fixtureClock171+1000;
  g_flamer.pending=g_flamer.carrying=true;g_flamer.seated=false;g_flamer.deadline=g_fixtureClock171+1000;
  g_seatCalls221=0;g_seatRefuse221=failure==5;
  if(failure==0)g_reload.widgets[0]={}; // absent matching capacity widget
  if(failure==1)word(0x138)=0; // malformed capacity
  if(failure==2)g_reload.widgets[0].wrapper=static_cast<LONG>(ammo.wrapper+512);
  if(failure==3){ammo.count()=word(0x134)=word(0x138);g_revolver.empty=false;}
  if(failure==4)word(0x140)=0;
  const UINT before=ammo.count(),spare=word(0x140);
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  const bool pending=profile==0?g_lever.pending:profile==34?g_spas.pending:profile==42?g_revolver.pending:profile==16?g_flare.pending:profile==32?g_rpg.pending:profile==18?g_gm94.pending:g_flamer.pending;
  Check(pending==(profile==42&&failure<3),"324 revolver missing HUD retains bounded retry; other seat leases consumed once");
  if(failure<6){
   Check(ammo.count()==before&&word(0x140)==spare,"221 missing/invalid/full/no-reserve/refused seat changes no ammo");
   Check(g_seatCalls221==(failure==5?1u:0u),"221 unsafe capacity cannot enter native transfer");
   if(profile==15&&(failure==3||failure==4))
    Check(g_flamer.seated&&!g_flamer.carrying&&!g_flamer.fills,"221 full/no-reserve flamethrower retains existing native-fuel reseat behavior");
   else Check(profile==0?g_lever.shell:profile==34?!g_spas.filled:profile==42?!g_revolver.filled:profile==16?g_flare.carrying:profile==32?!g_rpg.carrying:profile==18?g_gm94.carrying:g_flamer.carrying&&!g_flamer.seated&&!g_flamer.replaced&&!g_flamer.fills,"221 refused seat does not consume the physical shell/fill");
  }else{
   Check(g_seatCalls221==1&&ammo.count()==before+1&&word(0x140)==spare-1,"221 valid seat retains native one-round conservation");
  }
 }
 // A physical lever may close after contact but before the native request arrives.
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_lever={};g_lever.hold=g_lever.pending=true;g_lever.deadline=g_fixtureClock171+1000;
 g_seatCalls221=0;const UINT before=ammo.count();ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
 Check(g_lever.shell&&!g_lever.pending&&g_seatCalls221==0&&ammo.count()==before,"221 closed gate at native seam returns shell without transfer");
 g_lever={};g_lever.hold=g_lever.leverOpen=g_lever.pending=true;g_lever.chambered=false;g_lever.rightGripWas=true;g_lever.deadline=g_fixtureClock171-1;
 TickLever1887(true);
 Check(g_lever.shell&&!g_lever.pending&&!g_lever.chambered&&ammo.count()==before,"221 unanswered physical shell seat returns shell, preserves chamber and ammo");
 // Probe expiry is separate: it must not conjure a physical shell in Simple mode.
 g_lever={};g_lever.simple=g_lever.pending=g_lever.probing=true;g_lever.probeClip=kL1887Open;g_lever.deadline=g_lever.probeDeadline=g_fixtureClock171-1;
 TickLever1887(true);Check(!g_lever.shell&&!g_lever.pending&&!g_lever.probing,"221 expired simple probe cannot manufacture a held shell");
 fc3pad::CancelReloadRequest();fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);
 g_lever=oldLever;g_spas=oldSpas;g_revolver=oldRevolver;g_flare=oldFlare;g_rpg=oldRpg;g_gm94=oldGm;g_flamer=oldFlamer;g_reload=oldReload;g_nativeReloadBegin=oldBegin;g_nativeReloadEnd=oldEnd;g_nativeReload=oldTransfer;
 g_fixtureClock171=oldClock;g_leverFixtureHud=oldHud;g_leverFixtureOverfill=oldOverfill;g_seatRefuse221=false;
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;g_pistolMenu=oldMenu;fc3pad::Publish(oldControls);
 puts("PASS: seats221 1887/SPAS/revolver/flare/RPG/GM94/flamethrower missing/invalid/mismatched capacity, full/no reserve, refused/successful native transfer; closed 1887 gate and unanswered request preserve shell/ammo");
}

void TestBoltRegrab221()
{
 const auto oldR=g_r700;const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldG14=g_magazineGuides[14],oldG17=g_magazineGuides[17];
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand;
 const double oldScale=g_worldScale;float oldHands[2][16];memcpy(oldHands,g_handBody,sizeof(oldHands));
 g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;g_worldScale=1;
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;
 for(int profile:{2,23})for(double fraction:{.2,.5,.8,1.0}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_r700={};auto& R=g_r700;
  R.chambered=false;R.hold=R.reachedBack=true;R.s=fraction;R.holdBody0=HandIdentity();R.holdLeft0={};
  auto& p=g_pistolReload;p={};p.rootBody=HandIdentity();p.rootFrame=g_frame;
  for(auto& hand:g_handBody)for(int i=0;i<16;++i)hand[i]=i%5==0?1.f:0.f;
  // Known contact box and recorded drawn wrist; controller starts independently.
  for(int i=0;i<3;++i){g_pistolGuides.charge[i]=0;g_pistolGuides.size[2][i]=.2f;}
  R.rightHandBody=HandIdentity();R.rightHandBody.m[3]=-.591-R.rightPalm[0];R.rightHandBody.m[7]=-.011-R.rightPalm[1];R.rightHandBody.m[11]=-1.149-R.rightPalm[2];R.rightHandFrame=g_frame;
  g_handBody[0][7]=.7f;c.rightGrip=1;fc3pad::Publish(c);TickR700(true);
  Check(R.grabbing&&R.reachedBack&&!R.chambered&&fabs(R.s-fraction)<1e-5,"221 R700/Z93 partial closing regrab preserves exact bolt pose");
  const float origin=g_handBody[0][7];g_handBody[0][7]=origin+float(fraction*.05*.5);TickR700(true);
  Check(!R.chambered&&fabs(R.s-fraction*.5)<1e-4,"221 new closing stroke advances from retained partial position");
  g_handBody[0][7]=origin+float(fraction*.05+.001);TickR700(true);
  Check(R.chambered&&!R.hold&&!R.grabbing&&R.s==0&&R.cycles==1,"221 only remaining real forward travel completes bolt once");
 }
 g_r700=oldR;memcpy(g_magazineStates,oldStates,sizeof(oldStates));g_magazineGuides[14]=oldG14;g_magazineGuides[17]=oldG17;
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;g_pistolMenu=oldMenu;g_worldScale=oldScale;memcpy(g_handBody,oldHands,sizeof(oldHands));fc3pad::Publish(oldControls);
 puts("PASS: bolts221 R700/Z93 regrab during four closing positions retains pose and requires remaining physical forward travel");
}
