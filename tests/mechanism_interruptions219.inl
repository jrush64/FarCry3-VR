void TestMechanismInterruptions219()
{
 const auto oldG=g_gm94;const auto oldGC=g_gm94Clip;const auto oldF=g_flamer;const auto oldFC=g_flamerClip;const auto oldFlare=g_flare;const auto oldR=g_rpg;const auto oldSpas=g_spas;const auto oldBolt=g_r700;
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const bool oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldTwo=g_twoHand;
 const auto oldClock=g_fixtureClock171;g_fixtureClock171=200000;
 g_pistolMenu.open=false;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);
 auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>2e-5)return false;return true;};
 for(int kind:{1,2}){
  g_weaponMechanicProfile=18;g_weaponMechanicFrame=++g_frame;g_gm94={};g_gm94Clip={};
  auto& G=g_gm94;G.known=true;G.coverOpen=kind==2;G.coverGrab=G.pumping=true;G.offset=.067;
  g_gm94Clip.kind=kind;g_gm94Clip.scrub=.42;g_gm94Clip.poseFrame=-100000;const auto cover=Gm94ClipPose();
  const double angle=kGm94CoverOpenAngleDeg*(kind==1?.42:.58);
  g_handLeftValid=false;TickGm94(true);
  Check(!G.coverGrab&&!G.pumping&&!g_gm94Clip.kind&&G.coverPoseKnown&&G.coverGripSpent,"219 lost GM94 tracking releases grip and rebases next pump");
  Check(close(G.settledCover,cover.cover)&&close(G.settledCoverHand,cover.hand)&&fabs(G.coverDeg-angle)<1e-5&&G.offset==.067,"219 GM94 interruption preserves partial cover and barrel, no phantom stroke");
  g_handLeftValid=true;g_weaponMechanicProfile=7;g_weaponMechanicFrame=g_frame;TickGm94(true);Check(close(Gm94Cover(),cover.cover),"219 switching weapons cannot restart old GM94 cover clip");
  g_weaponMechanicProfile=15;g_weaponMechanicFrame=++g_frame;g_flamer={};g_flamerClip={};
  auto& F=g_flamer;F.known=true;F.seated=true;F.leverGrab=true;F.leverDown=kind==2;
  g_flamerClip.kind=kind;g_flamerClip.scrub=.42;g_flamerClip.poseFrame=-100000;const auto lever=FlamerClipPose();
  g_handRightValid=false;TickFlamer(true);
  Check(!F.leverGrab&&!g_flamerClip.kind&&F.leverPoseKnown&&F.seated,"219 lost flamethrower tracking releases lever without losing canister");
  Check(close(F.settledHandle,lever.handle)&&close(F.settledCrank,lever.crank)&&close(F.settledHydraulic,lever.hydraulic)&&fabs(F.lever-(kind==1?.42:.58))<1e-5,"219 interrupted flamethrower retains all articulated parts at current position");g_handRightValid=true;
 }
 g_weaponMechanicProfile=16;g_weaponMechanicFrame=++g_frame;g_flare={};g_flare.known=true;g_flare.grab=true;g_flare.rightPrevAt=g_fixtureClock171-10;g_flare.tiltSince=1;
 g_handLeftValid=false;TickFlare(true);Check(!g_flare.grab&&!g_flare.rightPrevAt&&!g_flare.tiltSince&&g_flare.chamber==1,"219 flare interruption discards grab/flick/tilt origins without changing chamber");g_handLeftValid=true;
 g_weaponMechanicProfile=32;g_weaponMechanicFrame=++g_frame;g_rpg={};g_rpg.known=true;g_rpg.loaded=false;g_rpg.clip=true;g_rpg.clipAt=g_fixtureClock171-50;
 g_handLeftValid=false;TickRpg(true);const double paused=RpgClipSeconds();g_fixtureClock171+=5000;++g_frame;g_weaponMechanicFrame=g_frame;TickRpg(true);
 Check(g_rpg.clip&&!g_rpg.loaded&&fabs(RpgClipSeconds()-paused)<1e-6,"219 tracking loss pauses RPG insert instead of finishing unseen");
 W7Resume(2500);g_fixtureClock171+=2500;Check(fabs(RpgClipSeconds()-paused)<1e-6,"219 Insert pause nested inside tracking loss shifts both RPG clocks once");
 g_handLeftValid=true;TickRpg(true);Check(g_rpg.clip&&!g_rpg.loaded&&fabs(RpgClipSeconds()-paused)<1e-6,"219 recovered RPG continues from paused insert time");
 g_weaponMechanicProfile=7;g_weaponMechanicFrame=g_frame;TickRpg(true);Check(!g_rpg.clip&&g_rpg.carrying&&!g_rpg.loaded,"219 weapon switch during RPG insertion returns existing preloaded rocket to hand");
 g_weaponMechanicProfile=32;g_weaponMechanicFrame=g_frame;g_rpg.frameBody=HandIdentity();g_rpg.frameFrame=g_frame;g_rpg.leftFrame=-100000;TickRpg(true);Check(g_rpg.carrying&&!g_rpg.pending,"219 interrupted RPG rocket remains available without requesting another native round");
 for(int profile:{1,7,8,9,12,14,17,20,21,22,23,25,28,31,35,36,39,41,43}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;TickPistolReload(false);g_pistolReload={};auto& p=g_pistolReload;
  p.enabled=true;p.magazinePresent=true;p.needsRack=true;p.chamberReady=false;p.rackHeld=p.rackRear=p.rackWristValid167=true;p.slidePull=.04;
  const bool locked=profile==12;p.emptyLocked=locked;TickPistolReload(false);
  Check(!p.rackHeld&&!p.rackRear&&!p.rackArmed&&!p.rackWristValid167&&p.needsRack&&!p.chamberReady&&p.magazinePresent,"219 every magazine weapon cancels interrupted held rack without chambering or ejecting");
  Check(p.slidePull==(locked?.05:0),"219 interrupted slide retains native empty-lock state");
  p.rootBody=HandIdentity();p.rootFrame=p.palmFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.tip={100,100,100};AdvancePistolRack(c,g_fixtureClock171+10);
  Check(!p.rackHeld&&!p.chamberReady,"219 holding grip through recovery cannot finish old charging stroke");
 }
 g_weaponMechanicProfile=34;g_weaponMechanicFrame=++g_frame;g_spas={};g_spas.needPump=g_spas.grab=true;g_spas.pump=.4;
 g_handLeftValid=false;TickSpas(true);Check(!g_spas.grab&&g_spas.needPump&&!g_spas.reachedBack&&g_spas.pump==.4,"219 SPAS tracking loss releases partial pump without phantom chambering");g_handLeftValid=true;
 for(int profile:{2,23}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_r700={};g_r700.physical=g_r700.hold=g_r700.grabbing=true;g_r700.chambered=false;g_r700.s=.4;
  g_handRightValid=false;TickR700(true);Check(!g_r700.grabbing&&g_r700.hold&&!g_r700.chambered&&g_r700.s==.4,"219 R700/Z93 missing right tracking releases bolt grab without changing stroke/chamber");g_handRightValid=true;
  g_r700.grabbing=true;g_handLeftValid=false;TickR700(true);Check(!g_r700.grabbing&&g_r700.hold&&!g_r700.chambered&&g_r700.s==.4,"219 R700/Z93 missing holding-hand tracking cannot advance a bolt stroke");g_handLeftValid=true;
 }
 fc3pad::CancelReloadRequest();fc3pad::SetXOwned(false);g_gm94=oldG;g_gm94Clip=oldGC;g_flamer=oldF;g_flamerClip=oldFC;g_flare=oldFlare;g_rpg=oldR;g_spas=oldSpas;g_r700=oldBolt;memcpy(g_magazineStates,oldStates,sizeof(oldStates));
 g_pistolMenu=oldMenu;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_twoHand=oldTwo;g_fixtureClock171=oldClock;fc3pad::Publish(oldControls);
 puts("PASS: mechanisms219 GM94/flamethrower/flare/SPAS/R700/Z93 interrupted grips, RPG insert pause/switch recovery, and 19 magazine-weapon charging interruptions");
}
