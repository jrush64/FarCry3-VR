// 1887 manual shells through the real MinHooked request + signal seams (no X reload, one shell per insertion).
UINT g_leverFixtureHud=0;bool g_leverFixtureOverfill=false; // live engine (log 37->38): a direct transfer ignores capacity
void __stdcall Lever1887FixtureTransfer(UINT state,UINT wrapper,UINT,int)
{ // single-round weapon: the engine adds exactly one shell per transfer (PID56072 log 3->4->...->11)
 if((ReloadRead(state,8)&255)==0)return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc),capacity=ReloadRead(g_leverFixtureHud,0x138),reserve=ReloadRead(g_leverFixtureHud,0x140);
 if((!g_leverFixtureOverfill&&before>=capacity)||!reserve)return;
 *reinterpret_cast<UINT*>(weapon+0xcc)=before+1;*reinterpret_cast<UINT*>(g_leverFixtureHud+0x134)=before+1;*reinterpret_cast<UINT*>(g_leverFixtureHud+0x140)=reserve-1;
 *reinterpret_cast<UINT*>(state+8)=0x100;
}
void TestLever1887Shells()
{
 const auto oldClock231=g_fixtureClock171;g_fixtureClock171=1000000;
 const auto oldLever=g_lever;const auto oldFree=g_freeHandAttachment[0];g_freeHandAttachment[0].epoch=g_armPoseEpoch+1;const LONG oldProfile=g_weaponMechanicProfile;const auto oldGate=g_reloadSignal;const auto oldAuthority=g_singleReloadAuthority;const UINT oldManager=g_singleReloadManager;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));const double oldScale=g_worldScale;g_worldScale=1;
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 auto& L=g_lever;fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,28672,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"1887 owner fixture");
 const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384,reference=actor+20480,manager=actor+24576;
 *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
 *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=101;*reinterpret_cast<UINT*>(reference+4)=202;*reinterpret_cast<UINT*>(manager)=actor;
 g_reload=PhysicalReloadState{};g_reload.installed=true;g_leverFixtureHud=hud;g_reload.widgets[0].wrapper=static_cast<LONG>(wrapper);g_reload.widgets[0].hud=static_cast<LONG>(hud);
 g_reloadSignal=ReloadSignalGate{};g_reloadSignal.enabled=true;g_singleReloadAuthority=SignalFixtureAuthority;g_singleReloadManager=manager;
 g_nativeReloadBegin=PistolFixtureBegin;g_nativeReloadEnd=PistolFixtureEnd;g_nativeReload=Lever1887FixtureTransfer;
 MH_Initialize();Check(InstallReloadRequestAt(reinterpret_cast<void*>(SignalFixtureNative)),"1887 pre-predicate MinHook installed");
 BYTE* fn=reinterpret_cast<BYTE*>(SignalFixtureNative);UINT site=0,fallback=0;const BYTE sig[]={0x8b,0,0x51,0x8b,0xcc};
 for(int i=0;i<128;++i){if(!memcmp(fn+i,sig,5))site=reinterpret_cast<UINT>(fn+i);if(fn[i]==0xe8&&fn[i+5]==0x84&&fn[i+6]==0xc0&&fn[i+7]==0x0f&&fn[i+8]==0x95&&fn[i+9]==0xc0)fallback=reinterpret_cast<UINT>(fn+i);}
 const auto oldSignalOriginal=g_reloadSignalOriginal;const auto oldSignalFallback=g_reloadSignalFallback;
 Check(site&&fallback&&InstallReloadSignalAt(reinterpret_cast<void*>(site),reinterpret_cast<void*>(fallback)),"1887 signal-site MinHook installed");
 UINT signalHash=0xaf41179b;
 Mat4 gate=HandIdentity(),shellInverse={};Invert(kL1887ShellFromHand,shellInverse);
 for(UINT loaded:{4u,0u,11u}){
  L=Lever1887{};L.hold=true;L.chambered=false;L.leverOpen=false;L.displayBody=HandIdentity();L.displayFrame=g_frame;
  gate.m[3]=L.gate[0];gate.m[7]=L.gate[1];gate.m[11]=L.gate[2];
  *reinterpret_cast<UINT*>(weapon+0xcc)=loaded;*reinterpret_cast<UINT*>(hud+0x134)=loaded;*reinterpret_cast<UINT*>(hud+0x138)=11;*reinterpret_cast<UINT*>(hud+0x140)=40;*reinterpret_cast<UINT*>(state+8)=0;
  c=fc3pad::Controllers{};c.active=true;c.leftGrip=1;fc3pad::Publish(c);TickLever1887(true);
  Check(RejectEmptyReloadSignal(state,wrapper,reinterpret_cast<UINT>(&signalHash)),"the game's X / automatic reload is refused for the 1887");
  fc3pad::CancelReloadRequest();c.x=true;fc3pad::Publish(c);Check(!fc3pad::ReloadOutput(),"X with the left hand on the 1887 never reaches the game");c.x=false;fc3pad::Publish(c);TickLever1887(true);
  L.rightHandBody=HandIdentity();L.rightHandBody.m[3]=2;L.rightHandFrame=g_frame;
  c.rightGrip=1;c.a=false;fc3pad::Publish(c);TickLever1887(true);Check(!L.shell,"closed 1887 lever keeps free hand empty");c.rightGrip=0;fc3pad::Publish(c);TickLever1887(true);L.leverOpen=true;c.rightGrip=1;fc3pad::Publish(c);TickLever1887(true);Check(L.shell,"open 1887 lever allows free right grip shell without A");
  {float fingers[576];memcpy(fingers,kL1887RightBones,sizeof(fingers));const Mat4 wrist=BoneMatrix(fingers,2);Check(Lever1887RightGrip(fingers),"right hand takes the native shell-carry fingers");
   for(int b=3;b<=19;++b){const Mat4 e=Multiply(wrist,BoneMatrix(kL1887ShellGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-e.m[i])<.001,"shell fingers keep the wrist");}}
  L.leverOpen=false; // engine seam still refuses insertion if the gate subsequently closes
  for(int n=0;n<3;++n){++g_frame;g_weaponMechanicFrame=g_frame;L.displayFrame=L.rightHandFrame=g_frame;L.rightHandBody=Multiply(gate,shellInverse);TickLever1887(true);}
  Check(!L.pending&&L.shell,"lever closed: the shell does not go in");
  L.leverOpen=true;
  for(int n=0;n<2;++n){++g_frame;g_weaponMechanicFrame=g_frame;L.displayFrame=L.rightHandFrame=g_frame;L.rightHandBody=Multiply(gate,shellInverse);TickLever1887(true);}
  Check(L.clip>=0&&!L.pending&&!L.shell&&!fc3pad::ReloadOutput(),"231 shell contact begins native seating clip before ammo transfer");
  g_fixtureClock171+=181;++g_frame;g_weaponMechanicFrame=g_frame;TickLever1887(true);
  Check(L.pending&&fc3pad::ReloadOutput(),"231 captured seat time requests exactly one engine transaction");
  g_fixturePredicate=1;g_fixtureReady=1;const UINT signals=g_signalCalls;InvokeSignalFixture(state,wrapper);
  if(loaded<11){
   Check(ReloadRead(weapon,0xcc)==loaded+1&&ReloadRead(hud,0x140)==39&&!L.pending&&!L.shell&&L.inserts==1,"exactly one round and one reserve round per shell");
   Check(L.leverOpen&&!L.chambered,"the lever stays open while loading; closing it chambers a round");
  }else Check(ReloadRead(weapon,0xcc)==11&&ReloadRead(hud,0x140)==40&&L.shell&&!L.pending,"full tube: nothing loads, the shell stays in the hand");
  Check(g_signalCalls==signals&&g_fixtureResult==1&&!fc3pad::ReloadOutput(),"the game's animated reload never starts; request pulse cancelled");
  InvokeSignalFixture(state,wrapper);Check(ReloadRead(weapon,0xcc)==(loaded<11?loaded+1:11),"a stray request cannot add a second round");
  g_fixtureClock171+=2000;TickLever1887(true);Check(L.clip<0,"231 short native seating returns control after transfer/refusal");
  if(L.shell){c.rightGrip=0;c.a=false;fc3pad::Publish(c);TickLever1887(true);Check(!L.shell&&ReloadRead(hud,0x140)==40,"letting go away from the gate drops the shell, no ammo change");}
  c=fc3pad::Controllers{};c.active=true;fc3pad::Publish(c);TickLever1887(true);
 }
 std::puts("PASS: 1887 manual shells: X and automatic reload refused; LEFT-hand hold, free RIGHT grip takes a shell (native carry fingers), the gate adds exactly one round per shell at the engine seam with no animation; shells only go into an open lever and closing it chambers; full tube or a dropped shell changes nothing.");
 MH_DisableHook(reinterpret_cast<void*>(SignalFixtureNative));MH_RemoveHook(reinterpret_cast<void*>(SignalFixtureNative));g_reloadRequestOriginal=nullptr;
 MH_DisableHook(reinterpret_cast<void*>(site));MH_RemoveHook(reinterpret_cast<void*>(site));g_reloadSignalOriginal=oldSignalOriginal;g_reloadSignalFallback=oldSignalFallback;
 VirtualFree(page,0,MEM_RELEASE);
 g_freeHandAttachment[0]=oldFree;g_lever=oldLever;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;g_reloadSignal=oldGate;g_singleReloadAuthority=oldAuthority;g_singleReloadManager=oldManager;
 memcpy(g_handBody,oldBody,sizeof(oldBody));g_worldScale=oldScale;g_twoHand=false;fc3pad::SetLeverHandling(false);fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::CancelReloadRequest();
 g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReload=nullptr;
 g_fixtureClock171=oldClock231;
}
