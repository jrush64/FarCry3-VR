UINT fixturePhysics179[24]={},fixturePhysicsVtable179[64]={};float fixtureFinalVelocity179[3]={};unsigned fixtureFinalWrites179=0;
UINT FixtureResolvePhysics179(UINT){return reinterpret_cast<UINT>(fixturePhysics179);}
void __fastcall FixtureVelocity179(void*,void*,const float* velocity,UINT flag){
 Check(flag==0,"179 native velocity setter preserves original flag");memcpy(fixtureFinalVelocity179,velocity,12);++fixtureFinalWrites179;
}
bool fixtureThrowAck178=true,fixtureThrowVelocity178=true;unsigned fixtureThrowCalls178=0;
float fixtureThrowMatrix178[16],fixtureThrowVelocityOut178[3];
bool __fastcall FixtureThrowUse178(void* self,void*,UINT){
 const UINT strategy=reinterpret_cast<UINT>(self);auto& s=g_throwScope178;++fixtureThrowCalls178;
 Check(Word(strategy,0x98)==~0u&&Word(strategy,0x9c)==~0u&&*reinterpret_cast<BYTE*>(strategy+(s.index?0x140:0xa0))==0,"178 native use receives no stale camera surface");
 fixturePhysics179[0]=reinterpret_cast<UINT>(fixturePhysicsVtable179);fixturePhysics179[1]=s.binding.reference;
 fixturePhysics179[0x54/4]=123;
 fixturePhysicsVtable179[0xec/4]=reinterpret_cast<UINT>(FixtureVelocity179);
 UINT local[96]={},regs[8]={};regs[6]=reinterpret_cast<UINT>(fixturePhysics179);const UINT stack=reinterpret_cast<UINT>(local);
 regs[3]=stack-4;regs[0]=strategy;regs[7]=stack+32;local[0]=Word(s.binding.reference,12);
 RewriteThrow178(s.index,regs);memcpy(fixtureThrowMatrix178,local+8,64);
 regs[2]=stack+192;local[50]=strategy;local[0]=stack+128;
 if(fixtureThrowVelocity178)RewriteThrow178(s.index+2,regs);memcpy(fixtureThrowVelocityOut178,local+32,12);
 // Wrong entity/stack scopes are never touched.
 const auto n=s.velocities;regs[2]=stack+196;RewriteThrow178(s.index+2,regs);Check(n==s.velocities,"178 wrong strategy frame rejects velocity write");
 if(fixtureThrowAck178){*reinterpret_cast<ULONGLONG*>(Word(strategy,0x7c))=s.binding.item;*reinterpret_cast<UINT*>(strategy+0x80)=1;}
 // Model a later native activation callback clearing the initial body velocity.
 memset(fixtureFinalVelocity179,0,12);
 return true;
}
void TestThrow178(UINT strategy,UINT list,float* matrix){
 const auto oldResolve=g_resolvePhysics179;g_resolvePhysics179=FixtureResolvePhysics179;auto oldDispatch=g_throwDispatch178;auto use0=g_surfaceUse177[0],use1=g_surfaceUse177[1];g_throwDispatch178=DispatchThrow178;
 *reinterpret_cast<UINT*>(strategy+0x7c)=list;float velocity[3]={0,0,0};
 for(int index=0;index<2;++index){const auto kind=index?Mine177:C4177;*reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[index];*reinterpret_cast<UINT*>(strategy+0x80)=0;
  g_surfaceUse177[index]=reinterpret_cast<SurfaceUse177>(FixtureThrowUse178);fixtureThrowAck178=true;fixtureThrowVelocity178=true;
  const auto writesAtStart180=fixtureFinalWrites179;
  Check(DispatchThrow178(strategy,kind,matrix,velocity),"178 stationary release acknowledges native transform plus zero velocity");
  Check(!memcmp(matrix,fixtureThrowMatrix178,64)&&!memcmp(velocity,fixtureThrowVelocityOut178,12),"178 native initial state is exact hand matrix and drop velocity");
  Check(g_deferredVelocity179[index].active&&fixtureFinalWrites179==writesAtStart180,"180 consumed release defers final body velocity until the enclosing native update completes");
  memset(fixtureFinalVelocity179,0,12);PumpFinalVelocity179(strategy,kind);
  Check(!g_deferredVelocity179[index].active&&!memcmp(velocity,fixtureFinalVelocity179,12),"180 post-update final commit restores stationary release after native update continuation");
  Check(!DispatchThrow178(strategy,kind,matrix,velocity),"178 already thrown native GUID cannot repeat");
  *reinterpret_cast<UINT*>(strategy+0x80)=0;velocity[0]=4;velocity[1]=-3;velocity[2]=8;
  Check(DispatchThrow178(strategy,kind,matrix,velocity)&&!memcmp(velocity,fixtureThrowVelocityOut178,12),"178 lateral upward throw preserves measured world velocity");
  // Simulate the native strategy update continuing after use and overwriting
  // the initial velocity. The post-update correction must win once.
  memset(fixtureFinalVelocity179,0,12);PumpFinalVelocity179(strategy,kind);
  Check(!memcmp(velocity,fixtureFinalVelocity179,12),"179 final native body velocity survives later use activation callback");
  *reinterpret_cast<UINT*>(strategy+0x80)=0;fixtureThrowVelocity178=false;
  const auto consumed=g_placeStats177[index].consumed178,failed=g_placeStats177[index].correctionFailed178;
  Check(DispatchThrow178(strategy,kind,matrix,velocity)&&g_placeStats177[index].consumed178==consumed+1,"180 native consumption is acknowledged while velocity validation remains pending");
  PumpFinalVelocity179(strategy,kind);
  Check(g_placeStats177[index].correctionFailed178==failed+1,"180 consumed throw with missing native velocity reports a failed correction after update");
  *reinterpret_cast<UINT*>(strategy+0x80)=0;fixtureThrowVelocity178=true;fixtureThrowAck178=false;
  *reinterpret_cast<ULONGLONG*>(strategy+0x98)=0x1122334455667788ULL;*reinterpret_cast<BYTE*>(strategy+(index?0x140:0xa0))=1;
  Check(!DispatchThrow178(strategy,kind,matrix,velocity)&&Word(strategy,0x98)==0x55667788&&*reinterpret_cast<BYTE*>(strategy+(index?0x140:0xa0))==1,"178 rejected native use restores preview and no false receipt");
  PublishPlaceableContext177(fixtureLocal143,8,true);PumpPlace177(strategy,kind);fixtureThrowAck178=true;
  Check(QueueThrowPlaceable178(kind,fixtureLocal143,8,matrix,velocity),"178 physical throw ticket queued");const auto calls=fixtureThrowCalls178;PumpPlace177(strategy,kind);PumpPlace177(strategy,kind);
  Check(fixtureThrowCalls178==calls+1,"178 queued physical release is dispatched once on native update");velocity[0]=36;
  Check(!QueueThrowPlaceable178(kind,fixtureLocal143,8,matrix,velocity),"178 impossible over-bound velocity refused");velocity[0]=0;velocity[1]=0;velocity[2]=0;
 }
 // Finalization rejects a different body, owner, or changed virtual method.
 auto& scope=g_throwScope178;scope={};scope.transforms=scope.velocities=1;scope.physics179=reinterpret_cast<UINT>(fixturePhysics179);
 scope.physicsVtable179=fixturePhysics179[0];scope.setter179=fixturePhysicsVtable179[0xec/4];scope.index=1;
 scope.epoch179=g_placeContext177.epoch;Check(BindPlace177(strategy,Mine177,scope.binding),"179 finalization identity setup");
 scope.entity179=Word(scope.binding.reference,12);const UINT entity=scope.entity179;
 UINT body=99;fixturePhysics179[0x54/4]=0;
 Check(BodyReady179(g_engine+0x1f566d8,g_engine+0x1684a20,reinterpret_cast<UINT>(fixturePhysics179),body)==0&&body==0,"179 verified rigid setter null body is not a velocity acknowledgement");
 fixturePhysics179[0x54/4]=123;
 Check(BodyReady179(g_engine+0x1f566d8,g_engine+0x1684a20,reinterpret_cast<UINT>(fixturePhysics179),body)==1&&body==123,"179 verified rigid body readiness is recognized");
 Check(BodyReady179(1,2,0,body)==-1,"179 unknown component never reads guessed body layout");
 auto& deferred=g_deferredVelocity179[1];deferred.scope=scope;deferred.deadline=GetTickCount64()+250;deferred.active=true;
 const auto finalBefore=fixtureFinalWrites179;PumpFinalVelocity179(strategy,Mine177);PumpFinalVelocity179(strategy,Mine177);
 Check(fixtureFinalWrites179==finalBefore+1&&!deferred.active,"179 deferred velocity commits once on matching native update");
 deferred.scope=scope;deferred.deadline=1;deferred.active=true;PumpFinalVelocity179(strategy,Mine177);
 Check(fixtureFinalWrites179==finalBefore+1&&!deferred.active,"179 expired deferred launch never changes a flying body");
 deferred.scope=scope;deferred.scope.epoch179++;deferred.deadline=GetTickCount64()+250;deferred.active=true;PumpFinalVelocity179(strategy,Mine177);
 Check(fixtureFinalWrites179==finalBefore+1&&!deferred.active,"179 stale tracking epoch cancels deferred velocity");
 scope.entity179=0;const auto writes=fixtureFinalWrites179;
 Check(!FinalizeThrowVelocity179(scope)&&fixtureFinalWrites179==writes,"179 final velocity never writes another or invalid body");scope.entity179=entity;
 fixturePhysicsVtable179[0xec/4]=0;Check(!FinalizeThrowVelocity179(scope),"179 changed body virtual method rejects finalization");scope={};
 g_resolvePhysics179=oldResolve;g_throwDispatch178=oldDispatch;g_surfaceUse177[0]=use0;g_surfaceUse177[1]=use1;
 std::puts("PASS:178 separate C4/mine physical release: exact hand origin/orientation and zero/lateral/upward velocity, native free-toss selection, identity guards, once-only update dispatch, and transform plus physics plus inventory receipt.");
}
void TestThrowHooks178(UINT strategy){
 auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));Check(code&&MH_Initialize()==MH_OK,"178 exact seams setup");
 void* bridges[4]={reinterpret_cast<void*>(C4Transform178),reinterpret_cast<void*>(MineTransform178),reinterpret_cast<void*>(C4Velocity178),reinterpret_cast<void*>(MineVelocity178)};
 for(int site=0;site<4;++site){memset(code,0xcc,4096);code[0]=0x6a;code[1]=0;code[2]=0x6a;code[3]=0;BYTE* seam=code+4;memcpy(seam,kThrowBytes178[site],6);code[100]=0xc2;code[101]=8;code[102]=0;
  const UINT sink=reinterpret_cast<UINT>(code+100);UINT vtable[64]={};vtable[0xec/4]=sink;
  if(site<2){*reinterpret_cast<INT*>(seam+1)=91;seam[5]=0xc3;}else if(site==2)seam[5]=0xc3;else{seam[6]=0xff;seam[7]=0xd2;seam[8]=0xc3;}
  auto original=g_throwOriginal178[site];Check(MH_CreateHook(seam,bridges[site],&g_throwOriginal178[site])==MH_OK&&MH_EnableHook(seam)==MH_OK,"178 real CALL/virtual seam MinHook relocation");
  UINT flags=0,ecxOut=0,input[4]={9,8,7,6},output[4]={};g_throwScope178={};
  __asm { mov edi,strategy }
  __asm { lea eax,vtable }
  __asm { mov edx,sink }
  __asm { mov ecx,17817817h }
  __asm { movdqu xmm1,input }
  __asm { stc }
  __asm { std }
  __asm { call code }
  __asm { pushfd }
  __asm { pop flags }
  __asm { cld }
  __asm { mov ecxOut,ecx }
  __asm { movdqu output,xmm1 }
  Check((flags&0x401)==0x401&&ecxOut==0x17817817&&!memcmp(input,output,16),"178 actual native call seam retains ret8 arguments flags registers and SSE");
  Check(MH_DisableHook(seam)==MH_OK&&MH_RemoveHook(seam)==MH_OK,"178 exact seam cleanup");g_throwOriginal178[site]=original;
 }
 Check(MH_Uninitialize()==MH_OK,"178 native hook cleanup");VirtualFree(code,0,MEM_RELEASE);
}

// Landing regression: native queries mutate their referenced item as well as
// strategy hit fields. The next held item must never be that target.
UINT fixtureQueryLive185=0,fixtureQueryReleased185=0;int fixtureQueryMode185=0;
bool __stdcall FixtureLandingQuery185(UINT strategy){
 Check(strategy!=fixtureQueryLive185,"185 flight query never receives live held-item strategy");
 const int index=g_surfaceScope177.index;
 const UINT entity=Word(Word(strategy,0x88),12);
 Check(entity==fixtureQueryReleased185,"185 flight collision metadata targets released entity only");
 *reinterpret_cast<UINT*>(entity+0x50)=185;
 ++g_surfaceScope177.rewrites;
 *reinterpret_cast<ULONGLONG*>(strategy+0x98)=fixtureQueryMode185==1?*reinterpret_cast<ULONGLONG*>(entity+8):0xabcdefULL;
 *reinterpret_cast<BYTE*>(strategy+(index?0x140:0xa0))=0;
 const float point[3]={1,2,3},normal[3]={0,0,1};
 memcpy(reinterpret_cast<void*>(strategy+(index?0xa0:0xa4)),point,12);
 memcpy(reinterpret_cast<void*>(strategy+(index?0xb8:0xbc)),normal,12);
 if(fixtureQueryMode185==2)RaiseException(0xe1850001,0,0,nullptr);
 return true;
}
void TestLanding185(UINT strategy,UINT list,UINT replacementRef,UINT replacementEntity){
 BYTE saved[0x150];memcpy(saved,reinterpret_cast<void*>(strategy),sizeof(saved));
 const UINT originalRef=Word(strategy,0x88),released=Word(originalRef,12),oldMarker=Word(released,0x50);
 const ULONGLONG releasedId=*reinterpret_cast<ULONGLONG*>(released+8);
 auto query0=g_surfaceQuery177[0],query1=g_surfaceQuery177[1];
 *reinterpret_cast<ULONGLONG*>(replacementRef)=*reinterpret_cast<ULONGLONG*>(replacementEntity+8)=releasedId+1;
 *reinterpret_cast<UINT*>(replacementRef+12)=replacementEntity;
 *reinterpret_cast<UINT*>(replacementEntity+0x50)=991;
 fixtureQueryLive185=strategy;fixtureQueryReleased185=released;
 for(int index=0;index<2;++index){
  *reinterpret_cast<UINT*>(strategy)=g_engine+kPlaceVtable177[index];
  *reinterpret_cast<UINT*>(strategy+0x88)=originalRef;
  *reinterpret_cast<UINT*>(strategy+0x7c)=list;*reinterpret_cast<UINT*>(strategy+0x80)=1;
  *reinterpret_cast<ULONGLONG*>(list)=releasedId;
  Settle181 w={};Check(BindPlace177(strategy,index?Mine177:C4177,w.scope.binding),"185 released binding fixture");w.scope.entity179=released;
  *reinterpret_cast<UINT*>(strategy+0x88)=replacementRef;
  BYTE heldBefore[0x150];memcpy(heldBefore,reinterpret_cast<void*>(strategy),sizeof(heldBefore));
  g_surfaceQuery177[index]=FixtureLandingQuery185;
  const float origin[3]={1,2,3},dir[3]={1,0,0};float point[3]={},normal[3]={};ULONGLONG surface=0;bool terrain=false;
  fixtureQueryMode185=0;
  Check(SurfaceAhead182(w,index,origin,dir,.2f,point,surface,normal,terrain)&&surface==0xabcdefULL&&point[1]==2&&normal[2]==1,"185 C4/mine flight query returns native collision data");
  Check(!memcmp(heldBefore,reinterpret_cast<void*>(strategy),sizeof(heldBefore))&&Word(replacementEntity,0x50)==991,"185 replacement reference, preview fields and entity remain untouched after landing query");
  Check(Word(released,0x50)==185&&!g_surfaceScope177.active,"185 only released entity receives collision metadata; scope cleared");
  fixtureQueryMode185=1;Check(!SurfaceAhead182(w,index,origin,dir,.2f,point,surface,normal,terrain),"185 released item cannot stick to itself");
  fixtureQueryMode185=2;Check(!SurfaceAhead182(w,index,origin,dir,.2f,point,surface,normal,terrain)&&!g_surfaceScope177.active,"185 query fault clears scope and leaves live strategy untouched");
  Check(!memcmp(heldBefore,reinterpret_cast<void*>(strategy),sizeof(heldBefore))&&Word(replacementEntity,0x50)==991,"185 failure cannot mutate replacement item");
  w.scope.binding.item++;Check(!SurfaceAhead182(w,index,origin,dir,.2f,point,surface,normal,terrain),"185 recycled released entity rejected before query");w.scope.binding.item--;
  *reinterpret_cast<UINT*>(strategy+0x80)=0;Check(!SurfaceAhead182(w,index,origin,dir,.2f,point,surface,normal,terrain),"185 removed or detonated released item rejected");
 }
 memcpy(reinterpret_cast<void*>(strategy),saved,sizeof(saved));*reinterpret_cast<UINT*>(released+0x50)=oldMarker;
 g_surfaceQuery177[0]=query0;g_surfaceQuery177[1]=query1;
 std::puts("PASS:185 landing query isolates both C4/mine replacement entities and strategy state, self-hit, fault, recycled identity and removed-item guards.");
}

void TestGroundPose186(){
 const float release[16]={1,0,0,0,0,1,0,0,0,0,1,0,100,200,300,1};
 const float hit[3]={1,2,3},flat[3]={0,0,1},slope[3]={.2f,-.1f,.9746794f},wall[3]={1,0,0};float pose[16]={};
 Check(GroundPose186(release,hit,flat,pose)&&MatrixValid177(pose),"186 flat ground produces valid face-up transform");
 Check(pose[8]==0&&pose[9]==0&&pose[10]==1&&pose[14]==hit[2],"186 floor keeps full placeable face up and pivot at actual hit height");
 Check(GroundPose186(release,hit,slope,pose)&&fabsf(pose[8]-.2f)<.001f&&fabsf(pose[9]+.1f)<.001f&&fabsf(pose[10]-.9746794f)<.001f,"186 sloped ground follows contact normal rather than standing on edge");
 Check(!GroundPose186(release,hit,wall,pose),"186 walls retain the accepted native orientation path");
 std::puts("PASS:186 C4/mine floor and slope face alignment, exact hit height and wall-path preservation.");
}
