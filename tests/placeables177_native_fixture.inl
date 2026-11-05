#include "placeable_surface177_fixture.inl"
#include "placeable_throw178_fixture.inl"
#include "placeable_stock179_fixture.inl"
// Native service identity/queue tests use fixture memory, never game functions.
unsigned fixturePlaceCalls177=0,fixtureRemoteCalls177=0;
float fixtureMatrix177[16]={};
bool FixturePlace177(UINT strategy,PlaceableKind177 kind,const float* matrix){++fixturePlaceCalls177;memcpy(fixtureMatrix177,matrix,64);return strategy&&kind!=None177;}
bool __fastcall FixtureRemote177(void* self,void*,UINT){++fixtureRemoteCalls177;*reinterpret_cast<UINT*>(reinterpret_cast<UINT>(self)+0x80)=0;return true;}
void TestPlaceables177(){
 auto* memory=static_cast<BYTE*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(memory!=nullptr,"177 fixture memory");
 const UINT base=reinterpret_cast<UINT>(memory),strategy=base,wrapper=base+1024,pawnRef=base+2048,pawn=base+2304,equippedRef=base+2560,equipped=base+2816,itemRef=base+3072,item=base+3328;
 const auto engine=g_engine;const LONG enabled=g_enabled;auto reader=g_identity143;const auto local=fixtureLocal143;
 const auto oldDispatch=g_placeDispatch177;const auto oldRemote=g_remoteDispatch177;const auto oldContext=g_placeContext177;const auto oldTicket=g_placeTicket177;
 PlaceBinding177 oldBindings[2];PlaceableStatus177 oldStats[2];bool oldInstalled[2];void* oldOriginals[2];
 memcpy(oldBindings,g_placeBinding177,sizeof(oldBindings));memcpy(oldStats,g_placeStats177,sizeof(oldStats));memcpy(oldInstalled,g_placeInstalled177,sizeof(oldInstalled));memcpy(oldOriginals,g_placeOriginal177,sizeof(oldOriginals));
 g_engine=0x10000000;g_enabled=1;fixtureLocal143=0x1122334455667788ULL;SetIdentityReader143(ReadLocal143);g_placeDispatch177=FixturePlace177;g_remoteDispatch177=reinterpret_cast<RemoteDispatch177>(FixtureRemote177);
 *reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[0];*reinterpret_cast<UINT*>(strategy+0x48)=wrapper;
 *reinterpret_cast<UINT*>(wrapper+0x28)=pawnRef;*reinterpret_cast<UINT*>(wrapper+0x24)=equippedRef;*reinterpret_cast<UINT*>(strategy+0x88)=itemRef;
 *reinterpret_cast<UINT*>(pawnRef+12)=pawn;*reinterpret_cast<ULONGLONG*>(pawnRef)=*reinterpret_cast<ULONGLONG*>(pawn+8)=fixtureLocal143;
 *reinterpret_cast<UINT*>(equippedRef+12)=equipped;*reinterpret_cast<ULONGLONG*>(equippedRef)=*reinterpret_cast<ULONGLONG*>(equipped+8)=0x2233445566778899ULL;
 *reinterpret_cast<UINT*>(itemRef+12)=item;*reinterpret_cast<ULONGLONG*>(itemRef)=*reinterpret_cast<ULONGLONG*>(item+8)=0x33445566778899aaULL;
 float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,123,234,345,1};
 g_placeInstalled177[0]=g_placeInstalled177[1]=true;g_placeTicket177={};g_placeStats177[0]={};g_placeStats177[1]={};
 PublishPlaceableContext177(fixtureLocal143,7,true);PumpPlace177(strategy,C4177);
 Check(PlaceableNativeStatus177(C4177).prepared,"177 exact local C4 reference admitted");
 Check(QueuePlaceable177(C4177,fixtureLocal143,7,matrix),"177 C4 queue accepts current owner/context/item");
 Check(!QueuePlaceable177(C4177,fixtureLocal143,7,matrix),"177 duplicate request refused");
 const auto calls=fixturePlaceCalls177;PumpPlace177(strategy,C4177);Check(fixturePlaceCalls177==calls+1&&!memcmp(fixtureMatrix177,matrix,64)&&!PlaceableNativeStatus177(C4177).pending,"177 native thread dispatch exactly once with original controller matrix");
 Check(QueuePlaceable177(C4177,fixtureLocal143,7,matrix),"177 stale item queue setup");*reinterpret_cast<ULONGLONG*>(item+8)^=1;PumpPlace177(strategy,C4177);
 Check(fixturePlaceCalls177==calls+1,"177 entity GUID reuse refuses queued dispatch");*reinterpret_cast<ULONGLONG*>(item+8)^=1;
 PublishPlaceableContext177(fixtureLocal143,8,true);PumpPlace177(strategy,C4177);Check(!QueuePlaceable177(C4177,fixtureLocal143,7,matrix),"177 old transform epoch refused");
 Check(QueuePlaceable177(C4177,fixtureLocal143,8,matrix),"177 cancellation setup");PublishPlaceableContext177(fixtureLocal143,8,false);PumpPlace177(strategy,C4177);Check(fixturePlaceCalls177==calls+1,"177 inactive tracking cancels placement");
 PublishPlaceableContext177(fixtureLocal143,8,true);fixtureLocal143^=1;Check(!PlaceableNativeStatus177(C4177).prepared,"177 NPC identity cannot adopt local controls");fixtureLocal143^=1;
 g_placeDispatch177=nullptr;Check(!PlaceableNativeStatus177(C4177).supported&&!QueuePlaceable177(C4177,fixtureLocal143,8,matrix),"177 unverified dispatch preserves native fallback");g_placeDispatch177=FixturePlace177;
 PumpPlace177(strategy,C4177);Check(!QueueDetonate177(fixtureLocal143,8),"177 empty C4 placed list cannot detonate");
 *reinterpret_cast<UINT*>(strategy+0x7c)=base+4096;*reinterpret_cast<UINT*>(strategy+0x80)=1;
 Check(QueueDetonate177(fixtureLocal143,8),"177 explicit C4 remote request accepts placed list");const auto remoteCalls=fixtureRemoteCalls177;PumpPlace177(strategy,C4177);Check(fixtureRemoteCalls177==remoteCalls+1&&g_placeStats177[0].detonated==1,"177 native detonation acknowledged by removed placed entries");
 *reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[1];PumpPlace177(strategy,Mine177);Check(PlaceableNativeStatus177(Mine177).prepared,"177 distinct native mine strategy recognized");
 Check(QueuePlaceable177(Mine177,fixtureLocal143,8,matrix),"177 mine queue uses its own exact binding");PumpPlace177(strategy,Mine177);Check(fixturePlaceCalls177==calls+2,"177 mine dispatch separate from frag/Molotov path");
 TestSurface177(strategy,base+4096,matrix);TestThrow178(strategy,base+4096,matrix);TestThrowHooks178(strategy);TestLanding185(strategy,base+4096,base+5000,base+5200);TestGroundPose186();TestPlaceStock179(strategy,base+6000);matrix[0]=0;Check(!QueuePlaceable177(Mine177,fixtureLocal143,8,matrix),"177 degenerate placement basis refused");
 // Exact native update prologue relocation proves thiscall dt/ret4 through the hook.
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));Check(code&&MH_Initialize()==MH_OK,"177 update relocation setup");
 for(int i=0;i<2;++i){memset(code,0xcc,4096);memcpy(code,kPlaceUpdateBytes177[i],11);const BYTE tail[]={0x8b,0xe5,0x5d,0xc2,0x04,0x00};memcpy(code+11,tail,sizeof(tail));
  *reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[i];void* bridge=i?reinterpret_cast<void*>(MineUpdate177):reinterpret_cast<void*>(C4Update177);
  Check(MH_CreateHook(code,bridge,&g_placeOriginal177[i])==MH_OK&&MH_EnableHook(code)==MH_OK,"177 exact update detour enabled");
  const auto observed=g_placeStats177[i].observed;volatile UINT canary=0x17717717;reinterpret_cast<PlaceUpdate177>(code)(reinterpret_cast<void*>(strategy),.016f);
  Check(canary==0x17717717&&g_placeStats177[i].observed==observed+1,"177 native update hook keeps dt stack ABI and observes owner");
  Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"177 exact update detour cleanup");}
 Check(MH_Uninitialize()==MH_OK,"177 update relocation library cleanup");VirtualFree(code,0,MEM_RELEASE);VirtualFree(memory,0,MEM_RELEASE);
 g_engine=engine;g_enabled=enabled;fixtureLocal143=local;SetIdentityReader143(reader);g_placeDispatch177=oldDispatch;g_remoteDispatch177=oldRemote;g_placeContext177=oldContext;g_placeTicket177=oldTicket;
 memcpy(g_placeBinding177,oldBindings,sizeof(oldBindings));memcpy(g_placeStats177,oldStats,sizeof(oldStats));memcpy(g_placeInstalled177,oldInstalled,sizeof(oldInstalled));memcpy(g_placeOriginal177,oldOriginals,sizeof(oldOriginals));
 std::puts("PASS:177 distinct native C4/mine identity, queued native-thread dispatch, tracking/epoch/stale/NPC guards, once-only remote receipt and exact update MinHook ABI. No game launched.");
}

