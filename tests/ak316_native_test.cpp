#define main ExistingAmmoSuite315
#include "ammo_probe_test.cpp"
#undef main
extern void SetRenderHands303(int,bool,unsigned);
extern void PublishSidearmControls315(float,float,float=0,float=0,bool=false,bool=false);
namespace native316 {
int begins=0,fills=0,ends=0;
void __stdcall Begin(UINT state,UINT wrapper){++begins;PistolFixtureBegin(state,wrapper);}
void __stdcall Fill(UINT,UINT wrapper,UINT flags,int amount){
 ++fills;Check(flags==0&&amount==-1,"316 native transfer ABI unchanged");
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc),capacity=ReloadRead(g_pistolFixtureHud,0x138),reserve=ReloadRead(g_pistolFixtureHud,0x140);
 const UINT n=before<capacity?(std::min)(capacity-before,reserve):0;
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=before+n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)=reserve-n;*reinterpret_cast<UINT*>(g_pistolFixtureHud+0x144)=before+reserve;
}
void __stdcall End(UINT state,UINT wrapper,UINT flag){++ends;PistolFixtureEnd(state,wrapper,flag);}
}
int main(){
 g_fixtureClock171=1000000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_reloadAlignment=false;g_machete.held=false;
 fc3pad::SetGameplayControls(true);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetSwapBX(false);fc3pad::SetGripContext(true,true);
 NativeAmmoFixture219 native;g_pistolFixtureHud=UINT(native.page)+16384;
 g_nativeReloadBegin=native316::Begin;g_nativeReload=native316::Fill;g_nativeReloadEnd=native316::End;
 for(int owner:{0,1})for(int profile:{8})for(int loaded:{0,7}){
  printf("NATIVE316 profile%d owner%d loaded%d\n",profile,owner,loaded);
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_twoHand=false;
  g_reload={};g_reloadSignal={};g_reloadSignal.enabled=g_reloadSignal.manualPistol=g_reloadSignal.manualAk=true;
  g_pistolReload={};g_pistolReload.enabled=true;g_pistolReload.pending=true;g_pistolReload.held=true;g_pistolReload.magazinePresent=false;g_pistolReload.chamberReady=true;g_pistolReload.deadline=GetTickCount64()+3000;
  g_flare={};g_revolver={};g_revolver.clip=-1;g_revolver.open=g_revolver.empty=g_revolver.carrying=g_revolver.pending=true;g_revolver.angle=100;g_revolver.deadline=GetTickCount64()+3000;
  g_flare.open=g_flare.carrying=g_flare.pending=g_flare.known=true;g_flare.chamber=0;g_flare.deadline=GetTickCount64()+3000;
  const UINT capacity=30,before=loaded,reserve=100;native.count()=before;*reinterpret_cast<UINT*>(native.state+8)=0;
  const UINT hud[5]={before,capacity,0,reserve,before+reserve};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));g_reload.widgets[0].wrapper=LONG(native.wrapper);g_reload.widgets[0].hud=LONG(g_pistolFixtureHud);
  SetRenderHands303(owner,false,g_armPoseEpoch);const auto held=fc3hands303::Read();fc3hands303::PublishTransfer307(true,false,false,held.generation,held.entity,g_armPoseEpoch,true);
  PublishSidearmControls315(0,0);native316::begins=native316::fills=native316::ends=0;
  ObserveMagazineReloadRequest(native.state,native.wrapper);
  Check(native316::begins==1&&native316::fills==1&&native316::ends==1&&native.count()==capacity&&ReloadRead(g_pistolFixtureHud,0x140)==reserve-(capacity-before),"316 each owner/family completes one native fill with exact reserve debit");
  Check(!(ReloadRead(native.state,8)&255)&&fc3hands303::Read().hand==owner,"316 native reload cleanup preserves physical weapon owner");
  if(profile==42)Check(!g_revolver.carrying&&!g_revolver.pending&&g_revolver.filled,"316 revolver speedloader consumed only after native fill");
  else if(profile==16)Check(!g_flare.carrying&&!g_flare.pending&&g_flare.chamber==1,"316 flare shell consumed only after native fill");
  else Check(g_pistolReload.magazinePresent&&!g_pistolReload.held&&!g_pistolReload.pending&&!g_pistolReload.cycle,"316 seated magazine cannot remain loose or pending in either owner");
  ObserveMagazineReloadRequest(native.state,native.wrapper);Check(native316::fills==1&&native.count()==capacity,"316 repeated seam cannot double spend ammunition");
  if((profile==8)&&g_pistolReload.needsRack){
   fc3pad::SetMagazineHandling(true,PistolBusy());PublishSidearmControls315(0,0);PublishSidearmControls315(owner?0:1,owner?1:0);Check(fc3pad::FireOutput()==0,"316 required manual rack still blocks owning trigger after native refill");
   PublishSidearmControls315(0,0);g_pistolReload.rackHeld=g_pistolReload.rackRear=true;g_pistolReload.slidePull=.05;
   AdvancePistolRack(fc3pad::Latest(),GetTickCount64());g_fixtureClock171+=101;AdvancePistolRack(fc3pad::Latest(),GetTickCount64());
   Check(g_pistolReload.chamberReady&&!g_pistolReload.needsRack,"316 simulated verified rear-release completes original chamber gate");
  }
  fc3pad::SetMagazineHandling((profile==8),(profile==8)&&PistolBusy());
  fc3pad::BlockFire(false);PublishSidearmControls315(0,0);if(profile==42){g_revolver.open=false;g_revolver.angle=0;UpdateRevolverFire(fc3pad::Latest());}
  if(profile==16){g_flare.open=false;g_flare.angle=0;g_flare.hammerDown=false;UpdateFlareFire(fc3pad::Latest());}
  PublishSidearmControls315(owner?0:1,owner?1:0);if(profile==42)UpdateRevolverFire(fc3pad::Latest());if(profile==16)UpdateFlareFire(fc3pad::Latest());
  Check(fc3pad::FireOutput()>0,"316 physical owning trigger reaches native firing after reload mechanism is ready");
 }
 puts("PASS:316 AK empty/partial in both owners complete original native reload requests,balanced cleanup,exact ammo/reserve,single item consumption,repeated request rejection and owning trigger firing;callbacks simulated,no game launched");
}
