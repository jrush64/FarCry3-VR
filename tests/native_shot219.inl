// Real local-authority seam with writable native-count fixture memory. The
// implementation observes these counts; only the fixture simulates a shot.
struct NativeAmmoFixture219 {
 BYTE* page=nullptr;UINT state=0,wrapper=0,weapon=0;
 decltype(g_singleReloadAuthority) authority=g_singleReloadAuthority;
 UINT manager=g_singleReloadManager;
 NativeAmmoFixture219(){
  page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"219 native ammo fixture page");
  const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,reference=actor+20480;
  state=component+0x718;wrapper=actor+8192;weapon=actor+12288;
  *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
  *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;
  g_singleReloadManager=actor+24576;*reinterpret_cast<UINT*>(g_singleReloadManager)=actor;g_singleReloadAuthority=SignalFixtureAuthority;count()=1;
 }
 UINT& count(){return *reinterpret_cast<UINT*>(weapon+0xcc);}
 ~NativeAmmoFixture219(){g_singleReloadAuthority=authority;g_singleReloadManager=manager;VirtualFree(page,0,MEM_RELEASE);}
};
void TestNativeSingleShots219()
{
 const auto oldRpg=g_rpg;const auto oldFlare=g_flare;const auto oldControls=fc3pad::Latest();const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const bool oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard;
 const auto oldClock=g_fixtureClock171;g_fixtureClock171=100000;
 g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;
 NativeAmmoFixture219 ammo;fc3pad::Controllers c={};c.active=true;
 for(int profile:{16,32}){
  g_flare={};g_rpg={};g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;ammo.count()=1;
  UINT& state=profile==16?g_flare.ammoState:g_rpg.ammoState;UINT& wrapper=profile==16?g_flare.ammoWrapper:g_rpg.ammoWrapper;
  NativeShot219& shot=profile==16?g_flare.shot:g_rpg.shot;
  if(profile==16)g_flare.known=true;else g_rpg.known=true;
  auto ready=[&](){return profile==16?g_flare.chamber==1&&!g_flare.hammerDown:g_rpg.loaded;};
  auto update=[&](){fc3pad::Publish(c);if(profile==16)UpdateFlareFire(c);else UpdateRpgFire(c);};
  c.rightTrigger=0;update();c.rightTrigger=1;update();
  Check(!shot.pending&&ready()&&fc3pad::FireOutput()==0,"219 unknown native owner cannot fake a single shot");
  Check(profile==16?!g_flare.known:!g_rpg.known,"219 stale local ammo binding automatically requests native reacquisition instead of permanent dry lock");
  c.rightTrigger=0;state=ammo.state;wrapper=ammo.wrapper;update();c.rightTrigger=1;update();
  Check(shot.pending&&ready()&&fc3pad::FireOutput()==255,"219 real trigger reaches game while physical chamber remains live");
  update();Check(shot.pending&&ready(),"219 held request cannot invent spent shell or empty tube");
  g_fixtureClock171+=181;update();Check(!shot.pending&&ready()&&shot.waitRelease&&fc3pad::FireOutput()==0,"219 missed native shot retains chamber and requires trigger release");
  update();Check(!shot.pending&&ready(),"219 timeout cannot auto-retry held trigger");
  c.rightTrigger=0;update();c.rightTrigger=1;update();ammo.count()=0;update();
  Check(!shot.pending&&!ready()&&shot.waitRelease,"219 actual native decrement confirms exactly one physical shot");
  c.rightTrigger=0;update();c.rightTrigger=1;update();Check(!shot.pending&&!ready(),"219 empty weapon cannot produce a second physical shot");
  if(profile==16){g_flare.chamber=1;g_flare.hammerDown=false;}else g_rpg.loaded=true;
  ammo.count()=1;c.rightTrigger=0;update();c.rightTrigger=1;update();Check(shot.pending,"219 next native round starts request");
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon+512;*reinterpret_cast<UINT*>(ammo.weapon+512+0xcc)=0;update();
  Check(!shot.pending&&ready(),"219 wrapper replacement cannot masquerade as a shot decrement");*reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;
  c.rightTrigger=0;update();c.rightTrigger=1;update();g_handLeftValid=false;ammo.count()=0;update();
  Check(!shot.pending&&ready(),"219 inactive tracking cancels request without changing chamber");g_handLeftValid=true;
  ammo.count()=1;c.rightTrigger=0;update();c.rightTrigger=1;update();++g_armPoseEpoch;ammo.count()=0;update();
  Check(!shot.pending&&ready(),"219 pose epoch change rejects stale shot confirmation");--g_armPoseEpoch;
  ammo.count()=1;c.rightTrigger=0;update();c.rightTrigger=1;update();g_fixtureClock171+=181;update();
  Check(ready()&&(profile==16?!g_flare.known:!g_rpg.known),"219 expired request schedules native reconciliation without consuming chamber");
  ammo.count()=0;
  if(profile==16){g_flare.probing=true;FlareRequest(ammo.state,ammo.wrapper);}else{g_rpg.probing=true;RpgRequest(ammo.state,ammo.wrapper);}
  Check(!ready()&&ammo.count()==0,"219 delayed native shot reconciles via existing probe, with no ammo write or stuck live chamber");
 }
 g_rpg=oldRpg;g_flare=oldFlare;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;
 g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_fixtureClock171=oldClock;
 fc3pad::Publish(oldControls);fc3pad::BlockFire(false);
 puts("PASS: single shots219 RPG/flare retain chamber on missed requests, confirm actual local native decrement, reject owner/tracking/epoch changes and require release before retry");
}
