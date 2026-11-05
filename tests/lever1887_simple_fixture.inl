// 1887 Simple mode: buttons step through the game's own captured animation clips; one shell per RIGHT grip at the native seam.
void TestLever1887Simple()
{
 const auto oldLever=g_lever;const LONG oldProfile=g_weaponMechanicProfile;const auto oldGate=g_reloadSignal;const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));const double oldScale=g_worldScale;g_worldScale=1;
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 auto& L=g_lever;fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"1887 simple owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;g_leverFixtureHud=hud;
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=Lever1887FixtureTransfer;
 MH_Initialize();Check(InstallReloadRequestAt(reinterpret_cast<void*>(SignalFixtureNative)),"1887 simple pre-predicate MinHook installed");
 BYTE* fn=reinterpret_cast<BYTE*>(SignalFixtureNative);UINT site=0,fallback=0;const BYTE sig[]={0x8b,0,0x51,0x8b,0xcc};
 for(int i=0;i<128;++i){if(!memcmp(fn+i,sig,5))site=reinterpret_cast<UINT>(fn+i);if(fn[i]==0xe8&&fn[i+5]==0x84&&fn[i+6]==0xc0&&fn[i+7]==0x0f&&fn[i+8]==0x95&&fn[i+9]==0xc0)fallback=reinterpret_cast<UINT>(fn+i);}
 const auto oldSignalOriginal=g_reloadSignalOriginal;const auto oldSignalFallback=g_reloadSignalFallback;
 Check(site&&fallback&&InstallReloadSignalAt(reinterpret_cast<void*>(site),reinterpret_cast<void*>(fallback)),"1887 simple signal-site MinHook installed");
 UINT signalHash=0xaf41179b;
 *reinterpret_cast<UINT*>(weapon+0xcc)=3;*reinterpret_cast<UINT*>(hud+0x134)=3;*reinterpret_cast<UINT*>(hud+0x138)=11;*reinterpret_cast<UINT*>(hud+0x140)=40;*reinterpret_cast<UINT*>(state+8)=0;
 L=Lever1887{};L.simple=true;L.chambered=true;L.displayBody=HandIdentity();L.displayFrame=g_frame;
 const auto oldWidgets=g_reload.widgets[0];g_reload.widgets[0].wrapper=static_cast<LONG>(wrapper);g_reload.widgets[0].hud=static_cast<LONG>(hud);
 auto setAmmo=[&](UINT n){*reinterpret_cast<UINT*>(weapon+0xcc)=n;*reinterpret_cast<UINT*>(hud+0x134)=n;*reinterpret_cast<UINT*>(hud+0x144)=n+*reinterpret_cast<UINT*>(hud+0x140);};
 setAmmo(3);g_leverFixtureOverfill=true;
 for(int i=0;i<16;++i)g_handBody[0][i]=i%5==0?1.f:0.f;
 auto tick=[&](){++g_frame;g_weaponMechanicFrame=g_frame;fc3pad::Publish(c);TickLever1887(true);};
 auto finish=[&](){L.clipAt-=20000;tick();};
 auto shoot=[&](){c.rightTrigger=1;fc3pad::Publish(c);UpdateLever1887Fire(c);c.rightTrigger=0;fc3pad::Publish(c);UpdateLever1887Fire(c);};
 tick();
 c.rightGrip=1;tick();Check(L.clip<0,"loaded: RIGHT grip does nothing");c.rightGrip=0;tick();
 shoot();Check(!L.chambered,"a shot empties the chamber");
 c.rightGrip=1;tick();Check(L.clip<0,"one hand: RIGHT grip does not work the lever");c.rightGrip=0;tick();
 g_handBody[0][3]=.25f;tick();Check(L.clip<0,"one hand: a shake does not work the lever");g_handBody[0][3]=0;tick();
 c.x=true;tick();Check(L.clip<0&&L.probeClip<0,"one hand: X does nothing");c.x=false;tick();
 g_twoHand=true;c.leftGrip=1;
 c.rightGrip=1;tick();Check(L.clip==kL1887Cycle&&!L.chambered,"two hands: after a shot, RIGHT grip plays the game's lever-cycle animation");c.rightGrip=0;
 {const LONG shotFrame=L.shotFrame;c.rightTrigger=1;fc3pad::Publish(c);UpdateLever1887Fire(c);Check(L.shotFrame==shotFrame&&!L.chambered,"no shot while the lever animation plays");c.rightTrigger=0;fc3pad::Publish(c);UpdateLever1887Fire(c);}
 {Lever1887::Pose mid={};L1887SampleClip(kL1887Cycle,kL1887ClipCycle[8].t,mid);Check(std::fabs(mid.lever.m[5]-kL1887ClipCycle[8].lever[5])<1e-5,"clip sampling hits the captured key");}
 finish();Check(L.clip<0&&L.chambered&&!L.leverOpen,"lever cycle done: chambered");
 shoot();Check(!L.chambered,"second shot");
 g_handBody[0][3]=0;for(auto& a:L.shakeAt)a=0;tick();tick();Check(L.clip<0,"a still controller is not a shake");
 g_handBody[0][3]=.25f;tick();Check(L.clip==kL1887Cycle,"shaking the right controller works the lever");finish();Check(L.chambered,"shake cycle chambers");g_handBody[0][3]=0;
 c.x=true;tick();Check(L.clip<0&&L.probeClip==kL1887Open&&L.probing,"weapon not learned yet: X sends one request probe first");c.x=false;
 g_fixturePredicate=1;g_fixtureReady=1;InvokeSignalFixture(state,wrapper);Check(L.ammoWrapper==wrapper&&!L.probing&&ReloadRead(weapon,0xcc)==3,"probe names the weapon, loads nothing");
 tick();Check(L.clip==kL1887Open,"X pulls the lever down (game's reload-start animation)");
 Check(!fc3pad::ReloadOutput(),"X never reaches the game");tick();
 c.rightGrip=1;tick();Check(L.clip==kL1887Open,"one step at a time: input during a clip is ignored");c.rightGrip=0;
 finish();Check(L.leverOpen&&!L.chambered&&Lever1887SimplePoseActive(),"lever stays down");
 {const auto& s=Lever1887SimplePose();const auto& k=kL1887ClipOpen[sizeof(kL1887ClipOpen)/sizeof(*kL1887ClipOpen)-1];Check(std::fabs(s.lever.m[6]-k.lever[6])<1e-5&&std::fabs(s.hand.m[3]-k.hand[3])<1e-5,"held pose = the last frame of the game's animation");
  float bones[576];memcpy(bones,kL1887RightBones,sizeof(bones));const Mat4 wrist=BoneMatrix(bones,2);Check(Lever1887RightGrip(bones),"right fingers from the clip");
  const Mat4 e=Multiply(wrist,BoneMatrix(k.fingers,0));bool ok=true;for(int i=0;i<12;++i)ok=ok&&std::fabs(bones[36+i]-e.m[i])<1e-3;Check(ok,"fingers follow the animation");}
 for(UINT n=0;n<2;++n){
  c.rightGrip=1;tick();Check(L.clip==(n?kL1887ShellLoop:kL1887ShellFirst)&&!L.pending,n?"next RIGHT grip: the next-shell animation":"RIGHT grip: the first-shell animation");c.rightGrip=0;tick();
  const float ins=kL1887Clips[L.clip].insert;L.clipAt=GetTickCount64()-static_cast<ULONGLONG>(ins*1000)-5;tick();
  Check(L.pending&&fc3pad::ReloadOutput(),"at the seat moment the engine seam is requested once");
  g_fixturePredicate=1;g_fixtureReady=1;InvokeSignalFixture(state,wrapper);
  Check(ReloadRead(weapon,0xcc)==4u+n&&ReloadRead(hud,0x140)==39u-n&&!L.pending,"exactly one round per RIGHT grip");
  InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==4u+n,"no second round from a stray request");
  finish();Check(L.clip<0&&L.leverOpen,"lever still down between shells");}
 setAmmo(11);c.rightGrip=1;tick();Check(L.clip<0&&L.probeClip<0&&!L.pending,"tube full: RIGHT grip plays nothing and loads nothing");c.rightGrip=0;tick();
 L.pending=true;L.deadline=GetTickCount64()+1000;InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==11&&ReloadRead(hud,0x140)==38,"a forced insert into a full tube is refused (engine transfer would overfill)");
 Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"the game's own reload stays refused");
 c.x=true;tick();Check(L.clip==kL1887Close,"X again: the game's close animation");c.x=false;finish();Check(!L.leverOpen&&L.chambered&&!Lever1887SimplePoseActive(),"closed and chambered");
 c.x=true;tick();Check(L.clip<0&&!L.leverOpen,"full and chambered: X does not open the lever");c.x=false;tick();
 setAmmo(10);c.x=true;tick();Check(L.clip==kL1887Open,"room for a shell: X opens again");c.x=false;finish();setAmmo(11);c.x=true;tick();c.x=false;finish();
 {wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"l8s",0,path);Check(SaveLever1887Settings(path),"Save All - 1887 simple");
  L.simple=false;LoadLever1887Settings(path);Check(L.simple,"Reload mode Simple survives save and reload");L.simple=false;SaveLever1887Settings(path);L.simple=true;LoadLever1887Settings(path);Check(!L.simple,"Manual survives too");DeleteFileW(path);}
 std::puts("PASS: 1887 Simple: both hands required; full tube refuses shells, lever-down and forced inserts (HUD capacity); first X probes the weapon without loading; after a shot RIGHT grip or a shake plays the game's lever cycle and chambers; X plays the reload start and the lever stays down; each RIGHT grip plays one shell animation and adds exactly one round at the native seam; X closes and chambers; one step at a time, no firing mid-clip, X kept from the game; Reload mode saves.");
 MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));MH_RemoveHook(reinterpret_cast<void*>(SignalFixtureNative));g_reloadRequestOriginal=nullptr;
 MH_DisableHook(reinterpret_cast<void*>(site));MH_RemoveHook(reinterpret_cast<void*>(site));g_reloadSignalOriginal=oldSignalOriginal;g_reloadSignalFallback=oldSignalFallback;
 VirtualFree(page,0,MEM_RELEASE);
 g_leverFixtureOverfill=false;g_reload.widgets[0]=oldWidgets;g_lever=oldLever;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;
 memcpy(g_handBody,oldBody,sizeof(oldBody));g_worldScale=oldScale;g_twoHand=false;fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::CancelReloadRequest();
 g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
}

