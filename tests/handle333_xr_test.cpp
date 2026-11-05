#include "smgs332_xr_fixture.inl"
extern unsigned short PadButtons333(const fc3pad::Controllers&);
void ResetGrab333(int owner){fc3grab333::Publish({});Reset321(owner);g_hands303.gripAction333[0]=g_hands303.gripAction333[1]=fc3grab333::None;g_hands303.ready[0]=g_hands303.ready[1]=true;}
fc3grab333::Binding Binding333(int owner,int profile){
 fc3grab333::Binding b;b.mode=b.enabled=true;b.profile=profile;b.support=profile!=10&&profile!=16&&profile!=39&&profile!=42;
 const auto point=Palm303(pose303[owner],owner),local=testnotes::Rotate(testnotes::Inverse(pose303[owner].orientation),testnotes::Sub(point,pose303[owner].position));b.handle[0]=local.x;b.handle[1]=local.y;b.handle[2]=local.z;b.foregrip[0]=local.x;b.foregrip[1]=local.y;b.foregrip[2]=local.z-.25f;return b;
}
void PlaceGrab333(int hand,int owner,const fc3grab333::Binding& b,bool support=false){const float* p=support?b.foregrip:b.handle;const auto target=testnotes::Add(pose303[owner].position,testnotes::Rotate(pose303[owner].orientation,{p[0],p[1],p[2]}));pose303[hand].position=testnotes::Add(pose303[hand].position,testnotes::Sub(target,Palm303(pose303[hand],hand)));}
void PollGrab333(fc3grab333::Binding b){
 fc3xr::SetHandsContext303(true,!b.busy,1,b.profile==10?1:0);inventory303.at=GetTickCount64();SeedInventory303(inventory303,0);b.entity=g_hands303.entity;b.generation=g_hands303.generation;b.epoch=g_roleEpoch303;b.reference=fc3xr::ReferenceGeneration286();b.at=GetTickCount64();fc3grab333::Publish(b);
 fc3hands303::PublishTransfer307(b.enabled,false,b.enabled&&!b.busy,g_hands303.generation,g_hands303.entity,g_roleEpoch303,false,false);SyncControllers();
}
#ifndef FC3_HANDLE333_ENTRY
#define FC3_HANDLE333_ENTRY main
#endif
int FC3_HANDLE333_ENTRY(){
 OriginalHands326XR();
 for(int profile:{0,1,2,3,4,5,6,7,8,9,10,12,14,15,16,17,18,20,21,22,23,25,28,31,32,34,35,36,39,40,41,42,43})for(int owner:{0,1}){
  ResetGrab333(owner);const int other=1-owner,receive=other?kLeftGrip:kRightGrip,hold=owner?kLeftGrip:kRightGrip;auto b=Binding333(owner,profile);pose303[other].orientation={0,0,1,0};PlaceGrab333(other,owner,b);PollGrab333(b);
  input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==other&&g_hands303.entity==100&&g_hands303.pendingSlot<0,"333 actual XR handle grip transfers immediately without owner grip,pull or re-equip");
  input303[hold]=1;PollGrab333(b);Check303(g_hands303.hand==other,"333 retained old grip cannot bounce weapon back");input303[hold]=0;PollGrab333(b);b=Binding333(other,profile);PlaceGrab333(owner,other,b);input303[hold]=1;PollGrab333(b);Check303(g_hands303.hand==owner,"333 one old-hand release then handle grip returns weapon");
  ResetGrab333(owner);b=Binding333(owner,profile);pose303[other].position.x+=2;PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(fc3pad::AmmoControls().leftGrip<.1f,"333 grip in empty space cannot spawn ammo");PlaceGrab333(other,owner,b);PollGrab333(b);Check303(g_hands303.hand==other,"333 already-held grip entering handle transfers without re-squeezing");
  if(profile!=10&&profile!=16&&profile!=39&&profile!=42){ResetGrab333(owner);b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==owner&&g_hands303.gripAction333[other]==fc3grab333::Support&&fc3pad::Latest().leftGrip>.9f&&((profile==0||(profile>=3&&profile<=6)||profile==34)?fc3pad::AmmoControls().leftGrip>.9f:fc3pad::AmmoControls().leftGrip<.1f),"342 shotgun foregrip retains native holding/pump grip;other ammo/transfer isolated");PlaceGrab333(other,owner,b);PollGrab333(b);Check303(g_hands303.hand==owner,"333 support squeeze retains its action while moving");input303[receive]=0;PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==other,"333 let go of foregrip then grip handle transfers");}
  ResetGrab333(owner);b=Binding333(owner,profile);PlaceGrab333(other,owner,b);b.busy=true;PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==owner&&(profile==10?fc3pad::Latest().rightGrip:fc3pad::AmmoControls().leftGrip)>.9f,"333 active reload at handle keeps its physical grip");b.busy=false;PollGrab333(b);Check303(g_hands303.hand==owner,"333 ending reload cannot turn held action into a grab");
  printf("PASS:333 profile%d owner%d current-input handle grab,roll/held-entry independence,no bounce,support/reload/empty-space ammo isolation\n",profile,owner);
 }
 // Existing explicit reload buttons remain usable away from handle/support.
 for(int owner:{0,1}){ResetGrab333(owner);const int other=1-owner,receive=other?kLeftGrip:kRightGrip;auto b=Binding333(owner,39);pose303[other].position.x+=2;PollGrab333(b);input303[receive]=1;input303[owner?kButtonB:kButtonX]=1;PollGrab333(b);Check303(g_hands303.hand==owner&&g_hands303.gripAction333[other]==fc3grab333::Reload&&fc3pad::AmmoControls().leftGrip>.9f&&fc3pad::AmmoControls().x,"333 deliberate existing magazine-button command retains reload input outside pouch");}
 for(int owner:{0,1}){
  ResetGrab333(owner);const int other=1-owner,receive=other?kLeftGrip:kRightGrip;auto b=Binding333(owner,39);
  const XrVector3f pouch={0,1.22f,-.12f};pose303[other].position=testnotes::Add(pose303[other].position,testnotes::Sub(pouch,Palm303(pose303[other],other)));PollGrab333(b);input303[receive]=1;PollGrab333(b);
  Check303(g_hands303.hand==owner&&fc3pad::AmmoControls().ammoPouch334&&fc3pad::AmmoControls().leftGrip>.9f,"334 actual right or left free hand pouch reaches normalized Skorpion reload");
  input303[receive]=0;PollGrab333(b);pose303[other].position.x+=2;PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(!fc3pad::AmmoControls().ammoPouch334,"334 leaving pouch cannot retain ammo pickup authorization");
 }
 fc3hands303::Snapshot visual334;visual334.enabled=visual334.ownerContext326=visual334.engaged=true;visual334.entity=100;visual334.hand=1;visual334.nativeHoldingSide=0;
 Check303(fc3hands303::NativeFallback326(visual334,39,false,true),"334 Skorpion left owner rejects uncorrected native right-hand draw");
 visual334.active=visual334.presentation314=true;Check303(!fc3hands303::NativeFallback326(visual334,39,true,true),"334 correctly posed left Skorpion remains visible");
 visual334.ownerContext326=false;Check303(!fc3hands303::NativeFallback326(visual334,39,false,false),"334 native context immediately revokes Skorpion visual retention");
 puts("PASS:334 Skorpion physical pouch routing and visual fallback ownership");
 //335 every supported inventory family retains its confirmed physical owner
 //through fresh, inactive and expired presentation without granting actions.
 for(int profile=0;profile<44;++profile){
  if(!fc3hands303::InventoryProfile335(profile))continue;
  const int native=profile==10?1:0,owner=1-native;ResetGrab333(owner);
  fc3xr::SetHandsContext303(true,true,1,native);PublishHands303(true);
  auto snap=fc3hands303::Read();
  Check303(fc3hands303::RemappedOwner335(snap,profile)&&!fc3hands303::NativeFallback326(snap,profile,true,true),"335 corrected live weapon stays visible in confirmed remapped owner");
  Check303(fc3hands303::NativeFallback326(snap,profile,false,true),"335 uncorrected native weapon draw cannot appear in other hand");
  fc3xr::SetHandsContext303(false,false,1,native,true);PublishHands303(false);snap=fc3hands303::Read();
  Check303(!snap.active&&snap.ownerContext326&&fc3hands303::PhysicalHand(snap,native)==owner&&fc3hands303::NativeFallback326(snap,profile,true,false),"335 motion pause retains render owner while refusing actions/native fallback");
  g_handsSnapshot303.confirmed314=GetTickCount64()-1800;PublishHands303(false);snap=fc3hands303::Read();
  Check303(!snap.active&&!snap.presentation314&&fc3hands303::SuppressWeapon(snap)&&fc3hands303::NativeFallback326(snap,profile,true,true),"335 expired corrected pose hides weapon instead of displaying it in native hand");
  fc3xr::SetHandsContext303(false,false,1,native);PublishHands303(false);snap=fc3hands303::Read();
  Check303(!snap.ownerContext326&&!fc3hands303::NativeFallback326(snap,profile,false,false),"335 explicit native context loss restores native renderer");
  snap.hand=native;snap.ownerContext326=true;Check303(!fc3hands303::NativeFallback326(snap,profile,false,false),"335 native-side ownership is never fallback-suppressed");
  printf("PASS:335 profile%d owner%d native%d live pose,stalled pose,expired pose,context revocation;offline only\n",profile,owner,native);
 }
 {fc3hands303::Snapshot s;s.enabled=s.ownerContext326=true;s.hand=1;s.entity=100;Check303(!fc3hands303::NativeFallback326(s,-1,false,false)&&!fc3hands303::NativeFallback326(s,44,false,false),"335 unrecognized geometry never gains inventory fallback permission");s.hand=-1;Check303(!fc3hands303::RemappedOwner335(s,7),"335 invalid owner fails closed");}
 // Trigger pressure and an unavailable holster basis cannot disable an
 // otherwise valid handle grab; firing stays guarded through the transfer.
 for(int owner:{0,1}){ResetGrab333(owner);const int other=1-owner,receive=other?kLeftGrip:kRightGrip;auto b=Binding333(owner,28);PlaceGrab333(other,owner,b);PollGrab333(b);g_rackBasisReady292=false;input303[owner?kLeftTrigger:kRightTrigger]=1;input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==other&&fc3pad::FireOutput()==0,"333 held trigger/holster lease cannot veto primary grab or fire through ownership change");g_rackBasisReady292=true;}
 // Close/overlapping regions must use current geometry, not last-frame flags.
 for(int owner:{0,1}){
  ResetGrab333(owner);const int other=1-owner,receive=other?kLeftGrip:kRightGrip;auto b=Binding333(owner,28);b.actionCount=1;for(int k=0;k<3;++k)b.actions[0].point[k]=b.handle[k];b.actions[0].point[1]-=.07f;b.actions[0].radius=.12f;
  const auto point=testnotes::Add(pose303[owner].position,testnotes::Rotate(pose303[owner].orientation,{b.actions[0].point[0],b.actions[0].point[1],b.actions[0].point[2]}));pose303[other].position=testnotes::Add(pose303[other].position,testnotes::Sub(point,Palm303(pose303[other],other)));PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==owner&&g_hands303.gripAction333[other]==fc3grab333::Reload,"333 current magazine target beats overlapping handle reach without stale contact flags");
  ResetGrab333(owner);b=Binding333(owner,28);b.actionCount=1;for(int k=0;k<3;++k)b.actions[0].point[k]=b.handle[k];b.actions[0].point[1]-=.07f;PlaceGrab333(other,owner,b);PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.hand==other,"333 exact primary handle still beats the nearby magazine target");
 }
 fc3grab333::Publish({});fc3pad::SetThrowableInput144(false,false);fc3pad::Controllers c={};c.active=true;c.rightGrip=1;
 for(int key=0;key<2;++key){c.a=key==0;c.b=key==1;Check303(!(PadButtons333(c)&0x0200),"333 right grip+A/B emits no native grenade shoulder input");}
 fc3pad::SetThrowableInput144(true,true);Check303(!(PadButtons333(c)&0x0200),"333 owned-grenade path cannot re-inject disabled shoulder");c.active=false;Check303(!(PadButtons333(c)&0x0200),"333 tracking-loss hold cannot re-inject disabled grenade input");fc3pad::SetThrowableInput144(false,false);
 puts("PASS:333 all33 inventory weapon profiles,both owners,shipping XR action latches and native grenade input disable;offline only");
 return 0;
}
