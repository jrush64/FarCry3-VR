namespace seat226test {
using namespace fc3throw;
UINT begins=0,fills=0,ends=0,mode=0,seatSounds=0,slideSounds=0;
void __stdcall Begin(UINT state,UINT wrapper){++begins;PistolFixtureBegin(state,wrapper);}
void __stdcall Fill(UINT state,UINT wrapper,UINT flag,int count){
 ++fills;Check(flag==0&&count==-1,"226 native magazine transfer ABI");
 if(mode==1||!ReloadRead(g_pistolFixtureHud,0x140))return;
 if(mode==3)RaiseException(0xe0000226,0,0,nullptr);
 PistolFixtureTransfer(state,wrapper,flag,count);
 if(mode==2)*reinterpret_cast<UINT*>(wrapper+0x48)=ReloadRead(wrapper,0x48)+512;
}
void __stdcall End(UINT state,UINT wrapper,UINT flag){++ends;PistolFixtureEnd(state,wrapper,flag);}
void __fastcall Sound(void* event,void*,UINT* token,UINT reference,UINT zero){
 const UINT id=Word(UINT(event),0x18);
 Check((id==0x173067||id==0x173068)&&Word(UINT(event),0x1c)==15&&Word(UINT(event),0x20)==1&&!zero,"226 exact native scalar event and actor routing");
 Check(Word(reference,8)==6,"226 sound receives fresh retained reference");
 if(id==0x173067)++seatSounds;else ++slideSounds;
 InterlockedDecrement(reinterpret_cast<volatile LONG*>(reference+8));token[0]=77;token[1]=88;
}
void TestD50Seat226(){
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldReload=g_reload;const auto oldGate=g_reloadSignal;const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;const auto oldGuides=g_pistolGuides;
 const auto oldBegin=g_nativeReloadBegin;const auto oldEnd=g_nativeReloadEnd;const auto oldFill=g_nativeReload;const UINT oldHud=g_pistolFixtureHud;
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;const auto oldClock=g_fixtureClock171;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand;
 float hand[16];memcpy(hand,g_handBody[1],sizeof(hand));
 const auto oldReader=g_identity143;const auto oldExecute=g_soundExecute225;const auto oldSound=g_d50Sound225;const auto oldContext=g_d50AudioContext170;const auto oldStats=g_d50AudioStats170;
 const UINT oldGeneration=g_d50AudioGeneration170;
 NativeAmmoFixture219 ammo;const UINT reference=Word(Word(ammo.state),4),entity=UINT(ammo.page)+27000;
 const auto oldLocal=sound225test::fixtureLocal225;sound225test::fixtureLocal225=(ULONGLONG(202)<<32)|101;SetIdentityReader143(sound225test::ReadLocal225);
 *reinterpret_cast<UINT*>(reference+8)=5;*reinterpret_cast<UINT*>(reference+12)=entity;*reinterpret_cast<ULONGLONG*>(entity+8)=sound225test::fixtureLocal225;
 g_soundExecute225=reinterpret_cast<void*>(Sound);
 g_fixtureClock171=1400000;g_weaponMechanicProfile=12;g_weaponMechanicFrame=++g_frame;
 g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 g_reloadSignal.enabled=g_reloadSignal.manualPistol=true;g_nativeReloadBegin=Begin;g_nativeReload=Fill;g_nativeReloadEnd=End;
 g_pistolFixtureHud=UINT(ammo.page)+16384;g_reload={};g_reload.widgets[0].wrapper=LONG(ammo.wrapper);g_reload.widgets[0].hud=LONG(g_pistolFixtureHud);
 auto hud=[&](UINT off)->UINT&{return *reinterpret_cast<UINT*>(g_pistolFixtureHud+off);};
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);
 for(UINT before:{0u,3u,8u})for(UINT failure:{0u,1u,2u,3u,4u}){
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=before;hud(0x134)=before;hud(0x138)=8;hud(0x140)=failure==4?0:100;hud(0x144)=before+hud(0x140);
  *reinterpret_cast<UINT*>(ammo.state+8)=0;
  auto& p=g_pistolReload;p={};p.enabled=p.pending=true;p.magazinePresent=false;p.deadline=g_fixtureClock171+3000;p.looseId=77;
  p.chamberReady=before!=0;p.needsRack=before==0;mode=failure==4?0:failure;begins=fills=ends=seatSounds=slideSounds=0;
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  const bool reseat=before==8||(before>0&&failure==4);
  const bool success=reseat||failure==0;
  Check(!p.pending&&!p.entryAccepted&&!p.cycle&&!g_reloadSignal.pistolPermissionUntil&&!fc3pad::ReloadOutput(),"226 request seam closes insertion without animated reload lease");
  Check(begins==(reseat?0:1)&&fills==begins&&ends==begins&&!(ReloadRead(ammo.state,8)&255),"226 one native data transaction, cleanup even when transfer throws, no tail state");
  UINT signal=0xaf41179b;
  Check(RejectEmptyReloadSignal(ammo.state,ammo.wrapper,UINT(&signal)),"226 actual reload signal remains denied after request seam");
  if(!success){
   Check(!p.magazinePresent&&p.held&&!p.committed&&p.insertionRetryBlocked&&p.looseId==77&&!seatSounds&&!slideSounds,"226 refusal, native fault or replaced weapon returns same magazine without sound or fake seating");continue;
  }
  Check(p.magazinePresent&&!p.held&&p.committed&&seatSounds==1&&!slideSounds&&Word(reference,8)==5,"226 verified seating emits only seat event with balanced reference");
  Check(p.commits==(reseat?0:1)&&ammo.count()==(reseat?before:8)&&hud(0x140)==(reseat?(failure==4?0:100):100-(8-before)),"226 real native ammo/reserve accounting or existing-round reseat");
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(seatSounds==1&&begins==(reseat?0:1),"226 repeat predicate cannot repeat transfer or seat sound");
  if(before){Check(!p.needsRack&&p.chamberReady&&!p.rackMode,"226 partial reload retains chamber and has no slide sound");continue;}
  Check(p.needsRack&&!p.chamberReady&&PistolBusy(),"226 empty magazine remains fire locked until physical rack");
  p.rootBody=HandIdentity();p.rootEpoch=g_armPoseEpoch;p.rootFrame=p.palmFrame=g_frame;p.tip={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};
  for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;
  c.leftGrip=0;AdvancePistolRack(c,g_fixtureClock171);c.leftGrip=1;AdvancePistolRack(c,g_fixtureClock171+1);
  Check(p.rackHeld&&!p.cycle&&!slideSounds,"226 slide grabbable immediately after seating without advancing native timeline");
  c.leftGrip=0;AdvancePistolRack(c,g_fixtureClock171+2);ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(!slideSounds&&p.needsRack,"226 short pull has no return sound or chamber credit");
  c.leftGrip=1;AdvancePistolRack(c,g_fixtureClock171+3);g_handBody[1][7]-=.045f;AdvancePistolRack(c,g_fixtureClock171+4);
  Check(p.rackRear&&!slideSounds,"226 controller movement reaches full physical rear before sound");
  c.leftGrip=0;AdvancePistolRack(c,g_fixtureClock171+5);ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(slideSounds==1&&Word(reference,8)==5,"226 full physical release dispatches slide event from actual game request seam");
  AdvancePistolRack(c,g_fixtureClock171+106);ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(!p.needsRack&&p.chamberReady&&!PistolBusy()&&slideSounds==1&&ammo.count()==8,"226 return releases fire with no native tail, duplicate sound or ammo change");
 }
 // A loaded but unracked magazine cannot bypass chambering by removal/reinsertion.
 *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=3;hud(0x134)=3;hud(0x138)=8;hud(0x140)=0;hud(0x144)=3;
 auto& uncharged=g_pistolReload;uncharged={};uncharged.enabled=uncharged.pending=uncharged.needsRack=true;uncharged.magazinePresent=uncharged.chamberReady=false;uncharged.deadline=g_fixtureClock171+3000;
 seatSounds=slideSounds=begins=fills=ends=0;mode=0;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
 Check(uncharged.magazinePresent&&uncharged.needsRack&&!uncharged.chamberReady&&!begins&&seatSounds==1&&!slideSounds,"226 remaining rounds reseat preserves prior unchambered requirement");
 Complete1911RackFixture50(g_fixtureClock171+200);
 Check(uncharged.d50RackComplete168&&!slideSounds,"226 completed nonempty uncharged rack retains dispatch intent beyond return");
 ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
 Check(slideSounds==1&&ammo.count()==3&&!hud(0x140),"226 nonempty uncharged reseat racks once without ammo change");
 // Manual mode and local authority are mandatory; Simple and remote actors keep their native path.
 *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;auto& p=g_pistolReload;p={};p.enabled=false;p.pending=true;p.deadline=g_fixtureClock171+3000;begins=0;
 ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins&&p.pending,"226 Simple handling does not execute manual seating");
 p.enabled=true;g_reloadSignal.manualPistol=false;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins&&p.pending,"226 native mode does not execute manual seating");
 g_reloadSignal.manualPistol=true;*reinterpret_cast<UINT*>(Word(Word(ammo.state),4))=999;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins&&p.pending,"226 nonlocal request cannot execute manual seating");
 memcpy(g_magazineStates,oldStates,sizeof(oldStates));g_reload=oldReload;g_reloadSignal=oldGate;g_pistolMenu=oldMenu;g_pistolGuides=oldGuides;
 g_nativeReloadBegin=oldBegin;g_nativeReloadEnd=oldEnd;g_nativeReload=oldFill;g_pistolFixtureHud=oldHud;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_fixtureClock171=oldClock;
 g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;memcpy(g_handBody[1],hand,sizeof(hand));
 SetIdentityReader143(oldReader);sound225test::fixtureLocal225=oldLocal;g_soundExecute225=oldExecute;g_d50Sound225=oldSound;g_d50AudioContext170=oldContext;g_d50AudioStats170=oldStats;g_d50AudioGeneration170=oldGeneration;
 fc3pad::CancelReloadRequest();fc3pad::Publish(oldControls);
 puts("PASS: D50 seat226 actual pre-predicate insertion, denied native timeline, immediate controller slide acquisition/rear/release, physical sound dispatch, partial/empty/full/no-reserve, failure/owner/mode guards and native cleanup");
}
}
