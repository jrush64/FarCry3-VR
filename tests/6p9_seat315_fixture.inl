namespace seat315test {
int mode=0,begins=0,fills=0,ends=0;
void __stdcall Begin(UINT state,UINT wrapper){++begins;PistolFixtureBegin(state,wrapper);}
void __stdcall Fill(UINT,UINT wrapper,UINT flag,int count){
 ++fills;Check(flag==0&&count==-1,"315 native1911 transfer uses established callback ABI");
 if(mode==1)return;
 if(mode==2)RaiseException(0xe0000315,0,0,nullptr);
 if(mode==3){*reinterpret_cast<UINT*>(wrapper+0x48)+=512;return;}
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc),reserve=ReloadRead(g_pistolFixtureHud,0x140);
 const UINT amount=before<12?(std::min)(12-before,reserve):0;
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+amount;
 *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x134)=before+amount;
 *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x140)=reserve-amount;
 *reinterpret_cast<UINT*>(g_pistolFixtureHud+0x144)=before+reserve;
}
void __stdcall End(UINT state,UINT wrapper,UINT flag){++ends;PistolFixtureEnd(state,wrapper,flag);}
void Test(){
 PistolReloadState old[20];memcpy(old,g_magazineStates,sizeof(old));const auto reload=g_reload;const auto gate=g_reloadSignal;const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;
 const auto begin=g_nativeReloadBegin;const auto end=g_nativeReloadEnd;const auto fill=g_nativeReload;const UINT oldHud=g_pistolFixtureHud;const auto oldClock=g_fixtureClock171;
 const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;
 const bool motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand,gripOnly=fc3pad::GripOnlyAmmoEnabled();
 NativeAmmoFixture219 ammo;g_pistolFixtureHud=UINT(ammo.page)+16384;g_fixtureClock171=2000000;
 g_weaponMechanicProfile=1;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetGripContext(true,true);
 TickPistolReload(true); // synchronize the existing per-profile slot before fixture state
 g_reloadSignal.enabled=g_reloadSignal.manualPistol=true;g_nativeReloadBegin=Begin;g_nativeReload=Fill;g_nativeReloadEnd=End;
 for(int scenario=0;scenario<13;++scenario){
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;*reinterpret_cast<UINT*>(ammo.state+8)=scenario==8?1:0;
  const UINT before=scenario==1||scenario==7?0:scenario==2?12:10,reserve=scenario==3?0:100;ammo.count()=before;
  const UINT hud[5]={before,12,0,reserve,before+reserve};memcpy(reinterpret_cast<void*>(g_pistolFixtureHud+0x134),hud,sizeof(hud));
  g_reload={}; // live315 failure: request sees equipped owner, HUD wrapper is unavailable/different
  auto& p=g_pistolReload;p={};p.enabled=scenario!=11;p.pending=scenario!=12;p.insertActive=true;p.magazinePresent=false;p.looseId=93;p.deadline=g_fixtureClock171+(scenario==10?0:3000);if(scenario==10)--p.deadline;
  begins=fills=ends=0;mode=scenario==4||scenario==7?1:scenario==5?2:scenario==6?3:0;
  c={};c.active=true;fc3pad::Publish(c);fc3pad::CancelReloadRequest();
  ObserveMagazineReloadRequest(ammo.state,scenario==9?ammo.wrapper+4:ammo.wrapper);
  if(scenario>=8){Check(!begins&&!fills&&!ends,"315 busy/wrong-owner/expired/Simple/noncontact request cannot perform native reload");continue;}
  Check(begins==1&&fills==1&&ends==1&&!(ReloadRead(ammo.state,8)&255),"315 one native transaction and cleanup, independent of missing HUD widget");
  if(scenario==5||scenario==6||scenario==7){Check(!p.magazinePresent&&p.held&&!p.pending&&!p.cycle&&p.insertionRetryBlocked&&p.looseId==93,"315 callback fault/owner change/empty refusal retains same magazine without false seating");continue;}
  const bool refill=scenario==0||scenario==1;
  Check(p.magazinePresent&&!p.held&&!p.looseFlying&&!p.pending&&!p.cycle&&!p.removalHeld&&p.looseId==93,"315 verified seated magazine has no simultaneous loose or pending ownership");
  Check(ammo.count()==(refill?12:before)&&ReloadRead(g_pistolFixtureHud,0x140)==reserve-(refill?12-before:0)&&p.commits==(refill?1:0)&&p.committed==refill,"315 only native callbacks change ammo/reserve; no-op reseat never invents refill");
  Check((p.needsRack==(before==0))&&(p.rackMode==(before==0))&&(p.chamberReady==(before>0)),"3156P9 preserves loaded chamber and requires rack only after empty reload");
  ObserveMagazineReloadRequest(ammo.state,ammo.wrapper);Check(begins==1&&fills==1&&ends==1,"315 repeated request cannot spend same insertion again");
  UINT signal315=0xaf41179b;Check(RejectEmptyReloadSignal(ammo.state,ammo.wrapper,UINT(&signal315)),"315 completed seating cannot grant duplicate native reload animation permission");
  if(scenario==0){
   g_fixtureClock171+=4000;
   for(int input=0;input<5;++input){c={};c.active=true;c.leftGrip=input>=4?1.f:0.f;c.x=input==0;c.y=input==1;c.a=input==2;c.b=input==3;fc3pad::Publish(c);TickPistolReload(true);
    Check(p.magazinePresent&&!p.held&&!p.looseFlying&&!p.pending&&!p.cycle&&p.nextLooseId==1&&ammo.count()==12,"315 after old timeout,face buttons/grip cannot resurrect or drop inserted magazine");}
   c={};c.active=true;fc3pad::Publish(c);
  }
 }
 memcpy(g_magazineStates,old,sizeof(old));g_reload=reload;g_reloadSignal=gate;g_nativeReloadBegin=begin;g_nativeReloadEnd=end;g_nativeReload=fill;g_pistolFixtureHud=oldHud;g_fixtureClock171=oldClock;
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;g_pistolMenu=menu;fc3pad::SetGripOnlyAmmo(gripOnly);fc3pad::SetMagazineHandling(false,false);fc3pad::CancelReloadRequest();fc3pad::Publish(controls);
 puts("PASS:315 native6P9 request-seam seating,live loaded10/missing-HUD reproduction,late buttons/no ghost magazine,ammo authority,manual rack,repeat/fault/owner/empty/busy/expiry/Simple guards");
}
}
