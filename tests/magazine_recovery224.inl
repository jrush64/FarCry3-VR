UINT g_transferCalls224=0,g_transferMode224=0,g_transferHud224=0;
void __stdcall MagazineTransfer224(UINT,UINT wrapper,UINT,int)
{
 ++g_transferCalls224;if(g_transferMode224==0||g_transferMode224==4)return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+1;
 *reinterpret_cast<UINT*>(g_transferHud224+0x134)=before+1;
 --*reinterpret_cast<UINT*>(g_transferHud224+0x140);
 if(g_transferMode224==2)*reinterpret_cast<UINT*>(wrapper+0x48)=weapon+512;
}
void __stdcall MagazineEnd224(UINT state,UINT wrapper,UINT flag)
{
 if(g_transferMode224==3)RaiseException(0xe0000224,0,0,nullptr);
 PistolFixtureEnd(state,wrapper,flag);
}
void TestMagazineRecovery224()
{
 PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 const auto oldReload=g_reload;const auto oldGate=g_reloadSignal;const auto oldControls=fc3pad::Latest();const auto oldMenu=g_pistolMenu;
 const auto oldBegin=g_nativeReloadBegin;const auto oldEnd=g_nativeReloadEnd;const auto oldTransfer=g_nativeReload;
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame;
 const auto oldClock=g_fixtureClock171;const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldTwo=g_twoHand;
 g_fixtureClock171=1000000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 NativeAmmoFixture219 ammo;g_transferHud224=reinterpret_cast<UINT>(ammo.page)+16384;
 auto word=[&](UINT off)->UINT&{return *reinterpret_cast<UINT*>(g_transferHud224+off);};
 g_reload={};g_reload.widgets[0].wrapper=LONG(ammo.wrapper);g_reload.widgets[0].hud=LONG(g_transferHud224);
 g_reloadSignal.enabled=g_reloadSignal.manualAk=true;g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=MagazineEnd224;g_nativeReload=MagazineTransfer224;
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;fc3pad::Publish(c);
 for(int profile:{8,7,41,28,14,9,31,25,21,17,35,36,22})for(UINT before:{0u,7u})for(UINT mode:{0u,1u,2u,3u,4u}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;auto& p=g_pistolReload;p={};p.enabled=true;
  p.pending=true;p.magazinePresent=false;p.chamberReady=false;p.needsRack=true;p.looseId=1234;p.nextLooseId=1235;
  p.state=ammo.state;p.wrapper=ammo.wrapper;p.weapon=ammo.weapon;
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=before;word(0x134)=before;word(0x138)=30;word(0x140)=mode==4?0:40;word(0x144)=before+word(0x140);
  g_transferCalls224=0;g_transferMode224=mode;InstantAkInsert(ammo.state,ammo.wrapper);
  Check(g_transferCalls224==1&&!p.pending&&!p.cycle&&!p.entryAccepted,"224 generic rifle insertion has one native transaction and closes request lease");
  if(mode==4&&before>0){
   Check(p.magazinePresent&&!p.held&&!p.committed&&!p.commits&&!p.chamberReady&&p.needsRack,"224 verified remaining native rounds reseat with no reserve, without inventing refill or chamber");
  }else if(mode==0||mode==2||mode==4){
   Check(!p.magazinePresent&&p.held&&!p.committed&&!p.insertActive&&p.insertionRetryBlocked&&p.result==2,"224 refused/replaced-owner fill returns replacement without invented seating");
   Check(!p.chamberReady&&p.needsRack&&p.looseId==1234&&p.nextLooseId==1235&&!p.commits,"224 rejected fill preserves chamber/rack state and exact loose-item identity");
  }else{
   Check(p.magazinePresent&&!p.held&&p.committed&&p.commits==1,"224 same-owner native increase confirms seating even if End raises after transfer");
   Check(p.needsRack==(CurrentRifle().emptyNeedsCharge&&before==0),"224 successful empty/partial insert preserves per-weapon charging requirement");
  }
  Check(ammo.count()==before+(mode&&mode!=4?1:0)&&word(0x140)==(mode==4?0:40)-(mode&&mode!=4?1:0),"224 mod never rolls back or fabricates native loaded/reserve changes");
 }
 // Full native magazines use verified HUD/owner reseating without any transfer.
 for(int profile:{8,7,41,28,14,9,31,25,21,17,35,36,22}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;auto& p=g_pistolReload;p={};p.enabled=true;p.pending=true;p.magazinePresent=false;
  *reinterpret_cast<UINT*>(ammo.wrapper+0x48)=ammo.weapon;ammo.count()=30;word(0x134)=word(0x138)=30;word(0x140)=40;word(0x144)=70;
  g_transferCalls224=0;g_transferMode224=0;InstantAkInsert(ammo.state,ammo.wrapper);
  Check(p.magazinePresent&&!p.held&&!p.pending&&g_transferCalls224==0&&ammo.count()==30&&word(0x140)==40,"224 verified full generic rifle reseats without refill call or ammo change");
 }
 // Use the same helper that the actual Insert-close path calls. A pre-existing
 // tracking pause must exclude the menu interval before the next real tick.
 for(int profile:{1,20,39,8,7,41,28,14,9,31,25,12,21,17,35,2,36,22,23,43}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;TickPistolReload(false);
  auto& p=g_pistolReload;p={};p.enabled=true;p.basis=true;p.pending=true;p.magazinePresent=false;p.looseId=55;
  p.deadline=g_fixtureClock171+3000;p.inputPauseAt=g_fixtureClock171;p.requestAt=g_fixtureClock171;
  const auto deadline=p.deadline,origin=p.inputPauseAt;
  g_fixtureClock171+=500;ResumeMagazineInsertPause224(2000);g_fixtureClock171+=2000;
  Check(p.inputPauseAt==origin+2000&&p.deadline==deadline+2000,"224 Insert shifts existing magazine tracking origin with insertion deadline");
  TickPistolReload(true);
  Check(!p.inputPauseAt&&p.pending&&!p.held&&p.deadline==deadline+2500&&p.looseId==55,"224 pending magazine resumes after tracking plus menu once, with same lease/object");
 }
 // Falling-magazine ages use the same exclusion and must not land in the future.
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=++g_frame;g_pistolReload={};auto& p=g_pistolReload;
 p.looseFlying=true;p.inputPauseAt=g_fixtureClock171;p.looseTick=g_fixtureClock171;p.looseAt=g_fixtureClock171-100;
 const auto started=p.inputPauseAt,fall=p.looseAt;
 g_fixtureClock171+=2000;ResumeMagazineInsertPause224(2000);
 Check(p.inputPauseAt==started+2000&&p.looseAt==fall+2000&&p.looseTick==g_fixtureClock171,"224 falling magazine menu shift preserves age and avoids double tracking interval");
 memcpy(g_magazineStates,oldStates,sizeof(oldStates));g_reload=oldReload;g_reloadSignal=oldGate;g_pistolMenu=oldMenu;
 g_nativeReloadBegin=oldBegin;g_nativeReloadEnd=oldEnd;g_nativeReload=oldTransfer;
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_fixtureClock171=oldClock;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_twoHand=oldTwo;
 fc3pad::CancelReloadRequest();fc3pad::SetMagazineHandling(false,false);fc3pad::Publish(oldControls);
 puts("PASS: magazines224 13 rifle families reject unconfirmed/wrong-owner fills without losing replacement; accepted transfers/full reseats conserve native ammo; all magazine profiles exclude nested Insert interval from tracking pause");
}
