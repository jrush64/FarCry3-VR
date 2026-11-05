namespace sound225test {
using namespace fc3throw;
ULONGLONG fixtureLocal225=0;UINT g_audioCalls225=0;
bool ReadLocal225(ULONGLONG& out){out=fixtureLocal225;return out!=0;}
void __fastcall AudioOriginal225(void*,void*,UINT,UINT,UINT){++g_audioCalls225;}
UINT g_executed225=0,g_route225=0;
void __fastcall SoundExecuteFixture225(void* event,void*,UINT* token,UINT reference,UINT zero){
 auto* e=static_cast<BYTE*>(event);
 Check(Word(UINT(e),0x18)==0x173068&&Word(UINT(e),0x1c)==15&&!zero,"225 fresh scalar event has exact native sound/type and Execute argument");
 Check(*reinterpret_cast<UINT*>(reference+8)==6,"225 native Execute receives one freshly retained actor reference");
 g_route225=e[0x20];++g_executed225;InterlockedDecrement(reinterpret_cast<volatile LONG*>(reference+8));token[0]=77;token[1]=88;
}
void TestD50Sound225(){
 const auto oldReader=g_identity143;const auto oldLocal=fixtureLocal225;const auto oldExecute=g_soundExecute225;const auto oldOriginal=g_soundOriginal156;
 const auto oldSound=g_d50Sound225;const auto oldContext=g_d50AudioContext170;const auto oldStats=g_d50AudioStats170;const UINT oldQueued=g_d50AudioQueued170,oldGeneration=g_d50AudioGeneration170;
 D50AudioRecord170 oldQueue[64];memcpy(oldQueue,g_d50AudioQueue170,sizeof(oldQueue));
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid;const auto oldClock=g_fixtureClock171;
 NativeAmmoFixture219 ammo;const UINT reference=Word(Word(ammo.state),4),entity=UINT(ammo.page)+27000,receiver=UINT(ammo.page)+28000,event=UINT(ammo.page)+28100;
 fixtureLocal225=(ULONGLONG(202)<<32)|101;SetIdentityReader143(ReadLocal225);
 *reinterpret_cast<UINT*>(reference+8)=5;*reinterpret_cast<UINT*>(reference+12)=entity;*reinterpret_cast<ULONGLONG*>(entity+8)=fixtureLocal225;*reinterpret_cast<UINT*>(receiver-8)=reference;
 *reinterpret_cast<UINT*>(event)=g_engine+0x1f4d618;*reinterpret_cast<UINT*>(event+0x18)=0x173068;*reinterpret_cast<UINT*>(event+0x1c)=15;
 g_soundExecute225=reinterpret_cast<void*>(SoundExecuteFixture225);g_soundOriginal156=reinterpret_cast<void*>(AudioOriginal225);
 g_fixtureClock171=1200000;g_weaponMechanicProfile=12;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto begin=[&](UINT flags=5){PublishD50Audio170(1,ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch,flags);g_executed225=0;};
 auto native=[&](){PinSoundGate156(reinterpret_cast<void*>(receiver),nullptr,3,1,event);};
 auto pump=[&](){return PumpD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch);};
 for(BYTE route:{BYTE(0),BYTE(1)}){
  begin();*reinterpret_cast<BYTE*>(event+0x20)=route;const auto calls=g_audioCalls225;native();
  Check(g_audioCalls225==calls&&g_d50Sound225.deferred&&!g_d50Sound225.played,"225 premature local empty-D50 slide event is deferred");
  Check(!pump()&&!g_executed225,"225 insertion or native event alone cannot play slide sound");
  auto& p=g_pistolReload;p={};p.enabled=p.committed=p.reloadWasEmpty=true;p.state=ammo.state;p.wrapper=ammo.wrapper;p.weapon=ammo.weapon;p.rackHeld=true;p.rackRear=false;
  AdvancePistolRack(c,g_fixtureClock171);Check(!g_d50Sound225.released,"225 short pull does not authorize sound");
  p.rackHeld=p.rackRear=true;p.slidePull=.05;AdvancePistolRack(c,g_fixtureClock171);
  Check(p.rackReturning&&g_d50Sound225.released&&fc3pad::ReloadOutput(),"225 full rear/release requests game-thread audio dispatch");
  const UINT count=ammo.count();ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(g_executed225==1&&g_route225==route&&ammo.count()==count&&Word(reference,8)==5,"225 actual native request seam executes sound once with original route and balanced reference, no ammo change");
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);native();Check(g_executed225==1&&g_audioCalls225==calls,"225 repeated requests/native duplicate cannot replay slide sound");
 }
 begin(4);const auto calls=g_audioCalls225;native();Check(g_audioCalls225==calls+1&&!g_d50Sound225.active,"225 nonempty reload remains native passthrough");
 begin();*reinterpret_cast<UINT*>(event+0x18)=0x173067;native();Check(g_audioCalls225==calls+2&&!g_d50Sound225.deferred,"225 magazine/handling event is untouched");*reinterpret_cast<UINT*>(event+0x18)=0x173068;
 begin();RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch);native();Check(g_audioCalls225==calls+3&&!pump(),"225 physical rack before native event allows original once, no duplicate");
 begin();native();RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch);++g_d50Sound225.thread;
 Check(!pump()&&!g_executed225,"225 different thread cannot execute native audio");--g_d50Sound225.thread;Check(pump()&&g_executed225==1,"225 verified native thread consumes pending sound");
 begin();native();RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch);*reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon+512;
 Check(!pump()&&!g_executed225&&!g_d50Sound225.active,"225 weapon replacement cancels deferred sound");*reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;
 begin();native();RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch);
 Check(!PumpD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch+1)&&!g_executed225,"225 stale pose epoch cannot replay");
 begin();native();g_fixtureClock171+=10000;PublishD50Audio170(4,ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch,3);
 Check(RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch)&&pump(),"225 delayed physical rack works beyond bounded observation window");
 begin();native();PublishD50Audio170(0,0,0,0,0,0);Check(!RequestD50Sound225(ammo.state,ammo.wrapper,ammo.weapon,g_armPoseEpoch)&&!pump(),"225 switching/manual context cancellation drops queued sound");
 g_soundExecute225=oldExecute;g_soundOriginal156=oldOriginal;SetIdentityReader143(oldReader);fixtureLocal225=oldLocal;
 g_d50Sound225=oldSound;g_d50AudioContext170=oldContext;g_d50AudioStats170=oldStats;g_d50AudioQueued170=oldQueued;g_d50AudioGeneration170=oldGeneration;memcpy(g_d50AudioQueue170,oldQueue,sizeof(oldQueue));memcpy(g_magazineStates,oldStates,sizeof(oldStates));
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_fixtureClock171=oldClock;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_pistolMenu=oldMenu;
 fc3pad::CancelReloadRequest();fc3pad::Publish(oldControls);
 puts("PASS: sound225 empty-D50 slide event deferral, real rear/release and request-seam Execute ABI, both routing modes, balanced fresh reference, no ammo change, exactly-once, owner/epoch/thread guards and delayed rack");
}

}
