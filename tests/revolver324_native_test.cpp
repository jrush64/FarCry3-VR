#define main ExistingAmmoSuite315
#include "ammo_probe_test.cpp"
#undef main
extern void SetRenderHands303(int,bool,unsigned);
extern void PublishSidearmControls315(float,float,float=0,float=0,bool=false,bool=false);
namespace native315 {
int begins=0,fills=0,ends=0;
void __stdcall Begin(UINT state,UINT wrapper){++begins;PistolFixtureBegin(state,wrapper);}
void __stdcall Fill(UINT,UINT wrapper,UINT flags,int amount){
 ++fills;Check(flags==0&&amount==-1,"315 native transfer ABI unchanged");
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc),capacity=ReloadRead(g_pistolFixtureHud,0x138),reserve=ReloadRead(g_pistolFixtureHud,0x140);
 const UINT n=before<capacity?(std::min)(capacity-before,reserve):0;
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=before+n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)=reserve-n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x144)=before+reserve;
}
void __stdcall End(UINT state,UINT wrapper,UINT flag){++ends;PistolFixtureEnd(state,wrapper,flag);}
}
int OriginalSidearms324(){
 g_fixtureClock171=1000000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_reloadAlignment=false;g_machete.held=false;
 fc3pad::SetGameplayControls(true);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetSwapBX(false);fc3pad::SetGripContext(true,true);
 NativeAmmoFixture219 native;g_pistolFixtureHud=UINT(native.page)+16384;
 g_nativeReloadBegin=native315::Begin;g_nativeReload=native315::Fill;g_nativeReloadEnd=native315::End;
 for(int owner:{0,1})for(int profile:{1,12,16,20,39,42}){
  printf("NATIVE315 profile%d owner%d\n",profile,owner);
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_twoHand=false;
  g_reload={};g_reloadSignal={};g_reloadSignal.enabled=g_reloadSignal.manualPistol=true;
  g_pistolReload={};g_pistolReload.enabled=true;g_pistolReload.pending=true;g_pistolReload.held=true;g_pistolReload.magazinePresent=false;g_pistolReload.chamberReady=true;g_pistolReload.deadline=GetTickCount64()+3000;
  g_flare={};g_revolver={};g_revolver.clip=-1;g_revolver.open=g_revolver.empty=g_revolver.carrying=g_revolver.pending=true;g_revolver.angle=100;g_revolver.deadline=GetTickCount64()+3000;
  g_flare.open=g_flare.carrying=g_flare.pending=g_flare.known=true;g_flare.chamber=0;g_flare.deadline=GetTickCount64()+3000;
  const UINT capacity=profile==16?1:profile==42?6:12,before=profile==16?0:2,reserve=100;native.count()=before;*reinterpret_cast<UINT*>(native.state+8)=0;
  const UINT hud[5]={before,capacity,0,reserve,before+reserve};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));g_reload.widgets[0].wrapper=LONG(native.wrapper);g_reload.widgets[0].hud=LONG(g_pistolFixtureHud);
  SetRenderHands303(owner,false,g_armPoseEpoch);const auto held=fc3hands303::Read();fc3hands303::PublishTransfer307(true,false,false,held.generation,held.entity,g_armPoseEpoch);
  PublishSidearmControls315(0,0);native315::begins=native315::fills=native315::ends=0;
  ObserveMagazineReloadRequest(native.state,native.wrapper);
  Check(native315::begins==1&&native315::fills==1&&native315::ends==1&&native.count()==capacity&&ReloadRead(g_pistolFixtureHud,0x140)==reserve-(capacity-before),"315 each owner/family completes one native fill with exact reserve debit");
  Check(!(ReloadRead(native.state,8)&255)&&fc3hands303::Read().hand==owner,"315 native reload cleanup preserves physical weapon owner");
  if(profile==42)Check(!g_revolver.carrying&&!g_revolver.pending&&g_revolver.filled,"315 revolver speedloader consumed only after native fill");
  else if(profile==16)Check(!g_flare.carrying&&!g_flare.pending&&g_flare.chamber==1,"315 flare shell consumed only after native fill");
  else Check(g_pistolReload.magazinePresent&&!g_pistolReload.held&&!g_pistolReload.pending&&!g_pistolReload.cycle,"315 seated magazine cannot remain loose or pending in either owner");
  ObserveMagazineReloadRequest(native.state,native.wrapper);Check(native315::fills==1&&native.count()==capacity,"315 repeated seam cannot double spend ammunition");
  if(fc3hands303::MagazineSidearm315(profile)&&g_pistolReload.needsRack){
   fc3pad::SetMagazineHandling(true,PistolBusy());PublishSidearmControls315(0,0);PublishSidearmControls315(owner?0:1,owner?1:0);Check(fc3pad::FireOutput()==0,"315 required manual rack still blocks owning trigger after native refill");
   PublishSidearmControls315(0,0);g_pistolReload.rackHeld=g_pistolReload.rackRear=true;g_pistolReload.slidePull=.05;
   AdvancePistolRack(fc3pad::Latest(),GetTickCount64());g_fixtureClock171+=101;AdvancePistolRack(fc3pad::Latest(),GetTickCount64());
   Check(g_pistolReload.chamberReady&&!g_pistolReload.needsRack,"315 simulated verified rear-release completes original chamber gate");
  }
  fc3pad::SetMagazineHandling(fc3hands303::MagazineSidearm315(profile),fc3hands303::MagazineSidearm315(profile)&&PistolBusy());
  fc3pad::BlockFire(false);PublishSidearmControls315(0,0);if(profile==42){g_revolver.open=false;g_revolver.angle=0;UpdateRevolverFire(fc3pad::Latest());}
  if(profile==16){g_flare.open=false;g_flare.angle=0;g_flare.hammerDown=false;UpdateFlareFire(fc3pad::Latest());}
  PublishSidearmControls315(owner?0:1,owner?1:0);if(profile==42)UpdateRevolverFire(fc3pad::Latest());if(profile==16)UpdateFlareFire(fc3pad::Latest());
  Check(fc3pad::FireOutput()>0,"315 physical owning trigger reaches native firing after reload mechanism is ready");
 }
 puts("PASS:315 all six sidearm families both owners complete original native reload requests,balanced cleanup,exact ammo/reserve,single item consumption,repeated request rejection and owning trigger firing;callbacks simulated,no game launched");
}

int main(){
 OriginalSidearms324();
 for(int owner:{0,1}){
  NativeAmmoFixture219 native;g_pistolFixtureHud=UINT(native.page)+16384;
  g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;SetRenderHands303(owner,false,g_armPoseEpoch);
  g_reload={};g_reloadSignal={};g_reloadSignal.enabled=g_reloadSignal.manualPistol=true;g_revolver={};
  auto& r=g_revolver;r.open=r.empty=r.carrying=true;r.angle=100;r.ammoState=native.state;r.ammoWrapper=native.wrapper;
  native.count()=0;const UINT hud[5]={0,6,0,30,30};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));
  // Reproduce the full table: a 17th distinct HUD must replace a stale entry.
  for(int i=0;i<16;++i)ObserveAmmoWidget(native.wrapper+1024+i*32,g_pistolFixtureHud+1024+i*32);
  const UINT alias=native.wrapper+512;*reinterpret_cast<UINT*>(alias+0x48)=ReloadRead(native.wrapper,0x48);ObserveAmmoWidget(alias,g_pistolFixtureHud);
  UINT loaded=0,capacity=0,reserve=0;
  Check(RevolverAmmo324(native.wrapper,loaded,capacity,reserve)&&capacity==6&&reserve==30,"324 full HUD cache accepts 17th object and same-weapon wrapper alias");
  *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=1;
  Check(!RevolverAmmo324(native.wrapper,loaded,capacity,reserve),"324 stale HUD count cannot authorize native transfer");
  *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=0;
  RevolverStartClip(0);r.seatFired=r.pending=true;r.deadline=GetTickCount64()+3000;r.seatWaitAt324=GetTickCount64();r.clipAt=GetTickCount64()-ULONGLONG((kRevolverInsertSeat-kRevolverClipInsert[r.clipFirst].t)*1000);
  const auto saved=g_reload;g_reload={};native315::begins=native315::fills=native315::ends=0;
  g_fixtureClock171+=700;RevolverRequest(native.state,native.wrapper);
  Check(r.pending&&r.carrying&&!r.filled&&native315::fills==0,"324 unavailable ammo keeps pending loader and never invokes unchecked fill");
  r.poseFrame=-100000;
  Check(RevolverClipSeconds()<=kRevolverInsertSeat+.001&&RevolverClipPose().loaderOn,"324 delayed seam holds visible loader at seat instead of disappearing");
  g_reload=saved;RevolverRequest(native.state,native.wrapper);
  Check(r.filled&&!r.pending&&native.count()==6&&ReloadRead(g_pistolFixtureHud,0x140)==24&&native315::fills==1,"324 delayed recovered HUD completes exactly one native six-round fill with reserve debit");
  Check(RevolverClipSeconds()<=kRevolverInsertSeat+.002,"324 confirmation resumes animation from seat without catchup disappearance");
  RevolverRequest(native.state,native.wrapper);Check(native315::fills==1,"324 repeated request cannot debit reserve twice");
  g_fixtureClock171+=150;r.poseFrame=-100000;Check(RevolverClipPose().loaderOn,"324 loaded speedloader remains visible through withdrawal");
  Check(fc3hands303::Read().hand==owner,"324 reload never changes physical owner");
  printf("PASS:324 revolver owner%d full HUD table,wrapper alias,stale rejection,missing HUD retention,delayed native fill and visible withdrawal\n",owner);
 }
 return 0;
}
