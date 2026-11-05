namespace skseat230 {
int mode=0,begins=0,fills=0,ends=0;
void __stdcall Begin(UINT state,UINT wrapper){++begins;PistolFixtureBegin(state,wrapper);}
void __stdcall Fill(UINT state,UINT wrapper,UINT flag,int count){
 ++fills;Check(flag==0&&count==-1,"230 original native transfer ABI");
 if(mode==1)return; // full/no-reserve native no-op
 if(mode==2)RaiseException(0xe0000230,0,0,nullptr);
 if(mode==3){*reinterpret_cast<UINT*>(wrapper+0x48)+=512;return;}
 PistolFixtureTransfer(state,wrapper,flag,count);
}
void __stdcall End(UINT state,UINT wrapper,UINT flag){++ends;PistolFixtureEnd(state,wrapper,flag);}
void Test(){
 PistolReloadState old[20];memcpy(old,g_magazineStates,sizeof(old));const auto reload=g_reload;const auto gate=g_reloadSignal;
 const auto begin=g_nativeReloadBegin;const auto end=g_nativeReloadEnd;const auto fill=g_nativeReload;const UINT hudOld=g_pistolFixtureHud;
 const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;
 const bool motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand;
 NativeAmmoFixture219 ammo;g_pistolFixtureHud=UINT(ammo.page)+16384;
 g_weaponMechanicProfile=39;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);g_reloadSignal.enabled=g_reloadSignal.manualPistol=true;
 g_nativeReloadBegin=Begin;g_nativeReload=Fill;g_nativeReloadEnd=End;
 for(int scenario=0;scenario<7;++scenario){
  auto& p=g_pistolReload;p={};p.enabled=p.pending=p.insertActive=true;p.magazinePresent=false;p.deadline=GetTickCount64()+3000;p.looseId=73;
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;*reinterpret_cast<UINT*>(ammo.state+8)=0;
  const UINT before=scenario==1?8:5;ammo.count()=before;
  const UINT hud[5]={before,8,0,scenario==2?0u:100u,before+(scenario==2?0u:100u)};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));
  g_reload={}; // deliberate equipped/HUD mismatch from live log; no matching widget
  mode=scenario==1||scenario==2?1:scenario==3?2:scenario==4?3:0;begins=fills=ends=0;
  if(scenario==5){p.chamberReady=false;p.needsRack=true;}
  c.rightTrigger=scenario==6?1.f:0.f;fc3pad::Publish(c);
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);
  Check(begins==1&&fills==1&&ends==1&&!ReloadRead(ammo.state,8),"230 loaded seating completes native callbacks and cleanup once without waiting for animation");
  if(scenario==3||scenario==4){Check(!p.magazinePresent&&p.held&&!p.pending&&p.insertionRetryBlocked&&p.looseId==73,"230 failed/changed-owner transfer returns same magazine without granting readiness");continue;}
  Check(p.magazinePresent&&!p.held&&!p.pending&&!p.cycle&&!p.removalHeld&&p.looseId==73,"230 partial/full/no-reserve Skorpion stays seated without HUD ownership dependency");
  Check(ammo.count()==(scenario==1||scenario==2?before:8),"230 seating uses native counts; no fabricated refill for no-op");
  Check(p.committed==(scenario!=1&&scenario!=2)&&p.needsRack==(scenario==5)&&p.waitTriggerRelease==(scenario==6),"230 retains chamber requirements, refill verdict and held-trigger guard");
  if(scenario==0){Check(!PistolBusy(),"230 loaded partial insertion fires without unnecessary charging/tail");fc3pad::SetMagazineHandling(true,PistolBusy());c.rightTrigger=1;fc3pad::Publish(c);Check(fc3pad::FireOutput()==255,"230 actual right trigger fires after partial seating");PistolXAction(c,false);Check(p.magazinePresent&&!p.held,"230 firing input cannot pick up seated magazine");}
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(begins==1,"230 repeated native requests cannot spend same insertion twice");
 }
 *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=0;auto& p=g_pistolReload;p={};p.enabled=p.pending=true;p.magazinePresent=false;p.deadline=GetTickCount64()+3000;begins=0;
 ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins&&p.pending&&PistolContactReady(ammo.state,ammo.wrapper),"230 accepted empty Skorpion native timeline remains available");
 ammo.count()=5;p.enabled=false;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins,"230 Simple mode cannot execute manual transaction");
 p.enabled=true;p.deadline=GetTickCount64()-1;ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(!begins,"230 expired insertion cannot execute callbacks");
 memcpy(g_magazineStates,old,sizeof(old));g_reload=reload;g_reloadSignal=gate;g_nativeReloadBegin=begin;g_nativeReloadEnd=end;g_nativeReload=fill;g_pistolFixtureHud=hudOld;
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;g_pistolMenu=menu;fc3pad::SetMagazineHandling(false,false);fc3pad::CancelReloadRequest();fc3pad::Publish(controls);
 puts("PASS: Skorpion230 partial/full/no-reserve insertion with mismatched HUD, native callback cleanup, failure/owner/repeat/expiry/Simple guards, held-trigger and charge preservation; empty timeline unchanged");
}
}
