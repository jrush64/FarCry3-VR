// Bow replenishment uses actual local-owner layout and native Begin/one-arrow transfer/End, not render-thread ammo writes.
UINT g_bowFixtureHud=0,g_bowFixtureBeginCount=0,g_bowFixtureTransferCount=0,g_bowFixtureEndCount=0;
void __stdcall BowFixtureBegin(UINT state,UINT){++g_bowFixtureBeginCount;*reinterpret_cast<UINT*>(state+8)=1;}
void __stdcall BowFixtureTransfer(UINT state,UINT wrapper,UINT requested,int count){
 ++g_bowFixtureTransferCount;Check(requested==1&&count==1,"bow native transfer requests exactly one arrow");
 Check(ReloadRead(state,8)==1,"bow Begin precedes native transfer");
 const UINT weapon=ReloadRead(wrapper,0x48),loaded=ReloadRead(weapon,0xcc),reserve=ReloadRead(g_bowFixtureHud,0x140);
 if(loaded||!reserve)return;
 *reinterpret_cast<UINT*>(weapon+0xcc)=1;*reinterpret_cast<UINT*>(g_bowFixtureHud+0x134)=1;*reinterpret_cast<UINT*>(g_bowFixtureHud+0x140)=reserve-1;
}
void __stdcall BowFixtureEnd(UINT state,UINT,UINT){++g_bowFixtureEndCount;*reinterpret_cast<UINT*>(state+8)=0;}
void TestBowReload(){
 const auto oldBow=g_bow;const auto oldGate=g_reloadSignal;const auto oldReload=g_reload;const auto oldAuthority=g_singleReloadAuthority;const auto oldManager=g_singleReloadManager;
 const auto oldBegin=g_nativeReloadBegin;const auto oldTransfer=g_nativeReload;const auto oldEnd=g_nativeReloadEnd;
 const auto oldProfile=g_weaponMechanicProfile,oldProfileFrame=g_weaponMechanicFrame;const auto controllers=fc3pad::Latest();
 const bool oldLeft=g_handLeftValid,oldRight=g_handRightValid,oldEyes=g_eyeViewsSet,oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldMenu=g_pistolMenu.open;
 const auto paused=g_w7PausedAt;g_w7PausedAt=0;
 g_weaponMechanicProfile=10;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=g_pistolMenu.open=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"bow native owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;ObserveAmmoWidget(wrapper,hud);g_bowFixtureHud=hud;
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=BowFixtureBegin;g_nativeReload=BowFixtureTransfer;g_nativeReloadEnd=BowFixtureEnd;g_bowFixtureBeginCount=g_bowFixtureTransferCount=g_bowFixtureEndCount=0;
 auto counts=[&](UINT loaded,UINT reserve){*reinterpret_cast<UINT*>(weapon+0xcc)=loaded;*reinterpret_cast<UINT*>(hud+0x134)=loaded;*reinterpret_cast<UINT*>(hud+0x138)=1;*reinterpret_cast<UINT*>(hud+0x140)=reserve;};
 auto nock=[&](){g_bow=BowState{};g_bow.nocked=g_bow.pending=true;g_bow.nativeReady=false;g_bow.draw=.8;g_bow.deadline=W7Now()+3000;fc3pad::RequestReload();};
 UINT signal=0xaf41179b;counts(0,8);g_bow=BowState{};BowRequest(state,wrapper);
 Check(!g_bowFixtureBeginCount&&ReloadRead(weapon,0xcc)==0&&ReloadRead(hud,0x140)==8,"stock request without physical nock cannot load an arrow");
 Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"empty bow refuses stock automatic animated refill");
 nock();*reinterpret_cast<UINT*>(reference+4)=999;BowRequest(state,wrapper);Check(g_bow.pending&&!g_bowFixtureBeginCount&&ReloadRead(weapon,0xcc)==0,"wrong actor cannot authorize bow refill");*reinterpret_cast<UINT*>(reference+4)=202;
 g_pistolMenu.open=true;BowRequest(state,wrapper);Check(g_bow.pending&&!g_bowFixtureBeginCount,"calibration menu cannot load bow arrows");g_pistolMenu.open=false;
 Check(!BowFireState(true,W7Now()),"pending physical nock cannot fire before native arrow loaded");
 for(UINT shot=0;shot<3;++shot){
  counts(0,8-shot);nock();BowRequest(state,wrapper);
  Check(!g_bow.pending&&g_bow.nativeReady&&ReloadRead(weapon,0xcc)==1&&ReloadRead(hud,0x140)==7-shot,"each physical nock refills one real native arrow and debits one spare");
  Check(g_bowFixtureBeginCount==shot+1&&g_bowFixtureTransferCount==shot+1&&g_bowFixtureEndCount==shot+1&&ReloadRead(state,8)==0,"bow native transaction completes once and clears pending state");
  BowRequest(state,wrapper);Check(g_bowFixtureTransferCount==shot+1&&ReloadRead(hud,0x140)==7-shot,"repeated callback cannot duplicate nock refill");
  Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signal)),"physical refill never permits stock animated reload signal");
  Check(BowFireState(true,W7Now()),"native loaded arrow enables physical draw");Check(!BowFireState(false,W7Now()+kBowMinimumPrimeMs)&&g_bow.shots==1,"grip release produces one physical shot request");
  Check(ReloadRead(weapon,0xcc)==1&&ReloadRead(hud,0x140)==7-shot,"mod release does not directly write native ammo counts");
  counts(0,7-shot); // actual game shot consumes its loaded arrow; emulate that engine-owned operation.
 }
 counts(1,5);nock();BowRequest(state,wrapper);Check(g_bow.nativeReady&&!g_bow.pending&&g_bowFixtureTransferCount==3&&ReloadRead(hud,0x140)==5,"existing native loaded arrow needs no extra reserve debit");
 counts(0,5);nock();BowFall(HandIdentity(),W7Now());BowRequest(state,wrapper);Check(!g_bow.pending&&!g_bow.nocked&&g_bowFixtureTransferCount==3&&ReloadRead(hud,0x140)==5,"dropping before refill cancels grant without consuming ammo");
 counts(1,5);nock();BowRequest(state,wrapper);BowFall(HandIdentity(),W7Now());Check(ReloadRead(weapon,0xcc)==1&&ReloadRead(hud,0x140)==5,"dropping a nocked visual arrow never refunds or deletes native ammo");
 counts(0,0);nock();BowRequest(state,wrapper);Check(!g_bow.pending&&!g_bow.nativeReady&&g_bowFixtureTransferCount==3&&ReloadRead(weapon,0xcc)==0,"known empty reserve refuses nock without native refill call");
 counts(0,5);nock();g_bow.deadline=W7Now()-1;BowRequest(state,wrapper);Check(!g_bow.nativeReady&&g_bowFixtureTransferCount==3,"expired nock cannot authorize native refill");
 counts(0,5);g_reload=PhysicalReloadState{};nock();BowRequest(state,wrapper);Check(g_bow.nativeReady&&g_bowFixtureTransferCount==4&&ReloadRead(hud,0x140)==4,"missing HUD still uses native one-arrow transaction instead of guessing ammo");
 g_bow=oldBow;g_reloadSignal=oldGate;g_reload=oldReload;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;g_nativeReloadBegin=oldBegin;g_nativeReload=oldTransfer;g_nativeReloadEnd=oldEnd;
 g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldProfileFrame;g_handLeftValid=oldLeft;g_handRightValid=oldRight;g_eyeViewsSet=oldEyes;g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_pistolMenu.open=oldMenu;g_w7PausedAt=paused;
 fc3pad::CancelReloadRequest();fc3pad::SetBowInput(false,0);fc3pad::Publish(controllers);VirtualFree(page,0,MEM_RELEASE);g_bowFixtureHud=0;
 std::puts("PASS: bow local game-thread one-arrow native refill for repeated shots; owner/menu/deadline/reserve guards; existing arrow no debit; drop cancellation; no mod ammo writes; automatic animated reload denied.");
}
