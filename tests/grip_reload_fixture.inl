static UINT g_hybridProperty=0,g_hybridDictionary=0,g_hybridNode=0,g_hybridActor=0;
static UINT g_hybridBeginCalls=0,g_hybridTransferCalls=0,g_hybridEndCalls=0,g_hybridNativeThread=0;
static UINT g_hybridState=0,g_hybridWrapper=0;
static HANDLE g_hybridEvents[6]={};
UINT __fastcall FixtureHybridProperties(void*,void*){return g_hybridDictionary;}
UINT __stdcall FixtureHybridAuthority(UINT manager,UINT lo,UINT hi)
{return manager==0x123456&&lo==g_hybridActor&&hi==0x5544?g_hybridActor:0;}
void __stdcall FixtureHybridBegin(UINT state,UINT wrapper)
{
    ++g_hybridBeginCalls;FixtureReloadBegin(state,wrapper);
    *reinterpret_cast<UINT*>(g_hybridNode+4)=0x3f3029ad;*reinterpret_cast<UINT*>(g_hybridNode+8)=g_hybridProperty;
    *reinterpret_cast<UINT*>(g_hybridProperty+8)=(std::min)(7-ReloadRead(g_reloadFixtureWeapon,0xcc),ReloadRead(g_reloadFixtureHud,0x140));
    ObserveAmmoWidget(wrapper,g_reloadFixtureHud,g_hybridActor);
}
void __stdcall FixtureHybridTransfer(UINT state,UINT,UINT flag,int count)
{
    ++g_hybridTransferCalls;
    auto* weapon=reinterpret_cast<UINT*>(g_reloadFixtureWeapon);auto* hud=reinterpret_cast<UINT*>(g_reloadFixtureHud);
    *reinterpret_cast<BYTE*>(state+9)=1;if(!*reinterpret_cast<BYTE*>(state+8))return;
    const UINT requested=flag?static_cast<UINT>(count):1;
    const UINT n=(std::min)(requested,(std::min)(7-weapon[0xcc/4],hud[0x140/4]));
    weapon[0xcc/4]+=n;hud[0x134/4]=weapon[0xcc/4];hud[0x140/4]-=n;hud[0x144/4]=hud[0x134/4]+hud[0x140/4];
    *reinterpret_cast<UINT*>(state+4)+=n;*reinterpret_cast<BYTE*>(state+8)=0;
}
void FixtureHybridAdvance(UINT state)
{
    auto& remaining=*reinterpret_cast<int*>(g_hybridProperty+8);
    if(*reinterpret_cast<BYTE*>(state+9)){--remaining;*reinterpret_cast<BYTE*>(state+9)=0;}
    *reinterpret_cast<BYTE*>(state+8)=remaining>0?1:0;
}
void __stdcall FixtureHybridEnd(UINT state,UINT wrapper,UINT flag)
{++g_hybridEndCalls;FixtureReloadEnd(state,wrapper,flag);}
DWORD WINAPI FixtureHybridGameThread(LPVOID)
{
    g_hybridNativeThread=GetCurrentThreadId();volatile UINT stackCanary=0x4321abcd;
    if(WaitForSingleObject(g_hybridEvents[0],5000)!=WAIT_OBJECT_0)return 1;
    ReloadBeginBoundary(g_hybridState,g_hybridWrapper);SetEvent(g_hybridEvents[1]);
    if(WaitForSingleObject(g_hybridEvents[2],5000)!=WAIT_OBJECT_0)return 2;
    ReloadBoundary(g_hybridState,g_hybridWrapper,0,-1);
    ReloadBoundary(g_hybridState,g_hybridWrapper,0,-1); // ordinary intermediate no-op event
    FixtureHybridAdvance(g_hybridState);SetEvent(g_hybridEvents[3]);
    if(WaitForSingleObject(g_hybridEvents[4],5000)!=WAIT_OBJECT_0)return 3;
    ReloadEndBoundary(g_hybridState,g_hybridWrapper,0);
    Check(stackCanary==0x4321abcd,"typed native start/transfer/end and authority preserve game-thread stack");SetEvent(g_hybridEvents[5]);return 0;
}
void TestGripReloadIsolation()
{
    const LONG priorFrame=g_frame;const bool priorMotion=g_motionGun,priorLeft=g_handLeftValid,priorRight=g_handRightValid,priorTwo=g_twoHand,priorChamber=g_chambered;
    g_frame=70000;g_motionGun=g_handLeftValid=g_handRightValid=true;g_twoHand=false;
    g_physicalReloadEnabled=false;g_gripReloadEnabled=true;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_gripReload=GripReloadState{};g_reloadContact=ReloadHandContact{};
    g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=true;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"hybrid reload context");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
    const UINT table=actor+20480,dictionary=table+256,buckets=dictionary+256,node=buckets+256,property=node+256,reference=actor+24576,manager=reference+256;
    *reinterpret_cast<UINT*>(actor)=table;*reinterpret_cast<UINT*>(table+0x7c)=reinterpret_cast<UINT>(FixtureHybridProperties);
    *reinterpret_cast<UINT*>(actor+4)=reference;*reinterpret_cast<UINT*>(reference)=actor;*reinterpret_cast<UINT*>(reference+4)=0x5544;*reinterpret_cast<UINT*>(manager)=0x123456;
    *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
    *reinterpret_cast<UINT*>(dictionary+0x10)=1;*reinterpret_cast<UINT*>(dictionary+0x14)=buckets;*reinterpret_cast<UINT*>(buckets)=node;
    // Native start creates the missing property. No normal-X learning reload.
    *reinterpret_cast<UINT*>(node+4)=123;*reinterpret_cast<UINT*>(node+8)=property;
    *reinterpret_cast<UINT*>(weapon+0xcc)=2;*reinterpret_cast<UINT*>(hud+0x134)=2;*reinterpret_cast<UINT*>(hud+0x138)=7;
    *reinterpret_cast<UINT*>(hud+0x140)=992;*reinterpret_cast<UINT*>(hud+0x144)=994;
    g_reloadFixtureHud=hud;g_reloadFixtureWeapon=weapon;g_hybridProperty=property;g_hybridDictionary=dictionary;g_hybridNode=node;g_hybridActor=actor;
    g_hybridBeginCalls=g_hybridEndCalls=g_hybridTransferCalls=0;g_hybridState=state;g_hybridWrapper=wrapper;
    g_nativeReloadBegin=FixtureHybridBegin;g_nativeReloadEnd=FixtureHybridEnd;g_nativeReload=FixtureHybridTransfer;
    g_nativeReloadAuthority=FixtureHybridAuthority;g_reloadAuthorityManager=manager;
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);TickPhysicalReload(true);
    c.leftGrip=c.rightGrip=c.leftTrigger=1;c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(g_reload.held&&!g_reload.ready&&!fc3pad::ReloadOutput()&&!g_hybridBeginCalls,"shell pickup works before HUD-owner discovery without native X or ammo");
    g_chambered=false;UpdatePump(g_armJoints[0],g_armJoints[1]);Check(!g_chambered,"pickup X cannot bypass physical pump lock");
    for(int i=0;i<50;++i){fc3pad::Publish(c);TickPhysicalReload(true);}
    Check(!g_hybridBeginCalls&&!g_hybridTransferCalls&&ReloadRead(weapon,0xcc)==2,"holding shell still cannot start an animation or auto-fill ammo");
    c.x=false;fc3pad::Publish(c);
    // Actual failed live approach frame8067:13.71cm, formerly rejected at12cm.
    const V3 capturedShell={.20842177323640745,-.20545624294961257,.42359155768024637};
    const V3 capturedPort={.20452584507595928,-.1249143907262686,.3127181941743744};
    g_reloadContact.hand=g_reloadContact.port=true;g_reloadContact.shellBody=capturedShell;g_reloadContact.portBody=capturedPort;
    g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;TickPhysicalReload(true);TickPhysicalReload(true);
    Check(g_gripReload.contactFrames==1&&!g_gripReload.pending,"one render frame cannot repeatedly count contact");
    for(int i=0;i<2;++i){++g_frame;g_reload.shotgunFrame=g_frame;g_reloadContact.handFrame=g_reloadContact.portFrame=g_frame;TickPhysicalReload(true);}
    Check(g_gripReload.pending&&g_reload.held&&g_reload.requests==1&&fc3pad::ReloadOutput(),"captured near-receiver approach requests one game animation while retaining held shell until start");
    TickPhysicalReload(true);Check(g_gripReload.pending&&fc3pad::ReloadOutput()&&!g_hybridBeginCalls&&ReloadRead(weapon,0xcc)==2,"next render tick retains one X request without direct native calls or ammo");
    *reinterpret_cast<UINT*>(reference)=actor+4;
    Check(!PrepareGripNativeBegin(state,wrapper)&&g_gripReload.pending,"native authority rejects a different actor without stealing physical request");
    *reinterpret_cast<UINT*>(reference)=actor;
    for(auto& e:g_hybridEvents){e=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(e!=nullptr,"hybrid engine event");}
    HANDLE thread=CreateThread(nullptr,0,FixtureHybridGameThread,nullptr,0,nullptr);Check(thread!=nullptr,"separate native game thread");
    SetEvent(g_hybridEvents[0]);Check(WaitForSingleObject(g_hybridEvents[1],5000)==WAIT_OBJECT_0,"native start observed");
    Check(g_hybridNativeThread!=GetCurrentThreadId()&&g_reload.thread==g_hybridNativeThread,"engine owns callback thread; render never calls native reload routines");
    Check(g_reload.ready&&!g_reload.held&&!g_reload.mode&&!g_gripReload.pending&&g_gripReload.finishing&&ReloadPoseActive(),"physical shell handed to native animation with validated local owner");
    Check(ReloadRead(property,8)==1&&ReloadRead(weapon,0xcc)==2&&ReloadRead(hud,0x140)==992&&!fc3pad::ReloadOutput(),"stock start limits five-shell fill to one, releases X and leaves ammo untouched before insertion event");
    for(int i=0;i<20;++i)TickPhysicalReload(true);
    Check(ReloadRead(weapon,0xcc)==2&&g_hybridTransferCalls==0,"animation wait cannot add ammo ahead of actual native event");
    c.leftGrip=0;fc3pad::Publish(c);TickPhysicalReload(true);Check(g_gripReload.finishing&&ReloadPoseActive(),"grip release cannot cancel the started native insertion animation");
    SetEvent(g_hybridEvents[2]);Check(WaitForSingleObject(g_hybridEvents[3],5000)==WAIT_OBJECT_0,"native insertion event observed");
    Check(ReloadRead(weapon,0xcc)==3&&ReloadRead(hud,0x140)==991&&g_reload.commits==1&&g_gripReload.credited,"native event adds exactly one loaded round and consumes one reserve shell");
    Check(g_gripReload.finishing&&ReloadPoseActive()&&ReloadRead(property,8)==0&&!(ReloadRead(state,8)&65535),"pending and loop clear after native advance while closing animation retains posing until end");
    SetEvent(g_hybridEvents[4]);Check(WaitForSingleObject(g_hybridEvents[5],5000)==WAIT_OBJECT_0,"native animation end observed");
    Check(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"native game thread exited");CloseHandle(thread);for(auto& e:g_hybridEvents){CloseHandle(e);e=nullptr;}
    Check(!g_gripReload.finishing&&!g_gripReload.failed&&!ReloadPoseActive()&&g_hybridEndCalls==1,"real native end restores ordinary hand policy and reports one-shell success");
    // Normal X remains a complete native reload, including all remaining shells.
    c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);Check(fc3pad::ReloadOutput(),"X without grip remains ordinary reload");
    ReloadBeginBoundary(state,wrapper);Check(ReloadRead(property,8)==4&&!g_gripReload.finishing,"ordinary X retains native remaining-shell count");
    while(ReloadRead(property,8)>0){ReloadBoundary(state,wrapper,0,-1);FixtureHybridAdvance(state);}
    ReloadEndBoundary(state,wrapper,0);
    Check(ReloadRead(weapon,0xcc)==7&&ReloadRead(hud,0x140)==987&&g_reload.commits==1&&!ReloadPoseActive(),"ordinary reload fills all four remaining rounds normally and does not count as physical inserts");
    // Stale geometry and tracking loss cannot generate a new animation.
    *reinterpret_cast<UINT*>(weapon+0xcc)=4;*reinterpret_cast<UINT*>(hud+0x134)=4;*reinterpret_cast<UINT*>(hud+0x140)=10;*reinterpret_cast<UINT*>(hud+0x144)=14;
    c.x=false;c.leftGrip=1;fc3pad::Publish(c);TickPhysicalReload(true);c.x=true;fc3pad::Publish(c);TickPhysicalReload(true);
    g_reloadContact.hand=g_reloadContact.port=true;g_reloadContact.handFrame=g_frame;g_reloadContact.portFrame=g_frame-3;
    TickPhysicalReload(true);Check(g_reload.held&&!g_gripReload.pending,"stale receiver cannot start native animation");
    c.leftGrip=0;fc3pad::Publish(c);TickPhysicalReload(true);Check(!g_reload.held&&!g_gripReload.pending,"release away drops shell without reload input");
    g_gripReload.pending=1;g_gripReload.deadline=GetTickCount64()+3000;fc3pad::RequestReload();c.active=false;fc3pad::Publish(c);TickPhysicalReload(true);
    Check(!g_gripReload.pending&&!g_reload.held&&!fc3pad::ShellPickupOwnsX(),"tracking loss cancels unstarted animation request");
    g_nativeReload=nullptr;g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;g_nativeReloadAuthority=nullptr;g_reloadAuthorityManager=0;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_gripReload=GripReloadState{};g_reloadContact=ReloadHandContact{};
    g_reloadShell.parts[0].ready=g_reloadShell.parts[1].ready=false;g_frame=priorFrame;g_motionGun=priorMotion;
    g_handLeftValid=priorLeft;g_handRightValid=priorRight;g_twoHand=priorTwo;g_chambered=priorChamber;
    fc3pad::SetShellPickupBinding(false);fc3pad::SetShotgunHandling(false);fc3pad::Publish(fc3pad::Controllers{});VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: captured receiver approach -> one native animation; pickup before HUD discovery; native authority/ABI and separate game thread; ammo unchanged until real event; 2/992 ->3/991 exactly once; native finish; normal X fills to7; stale/drop/tracking recovery");
}
