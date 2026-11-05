#pragma once
#include <windows.h>
#include <cmath>
#include "handle_grab333.h"
#include "side_mirror338.h"
namespace fc3hands303 {
struct Settings {bool enabled=true,requireHolster=true;};
struct Snapshot {bool enabled=false,active=false,empty=false,engaged=false;int hand=0,slot=-1,nativeHoldingSide=0;unsigned entity=0,generation=0;unsigned long long at=0;int receiver=-1;bool presentation314=false;unsigned long long confirmed314=0;bool ownerContext326=false;};
Snapshot Read();
// Presentation may bridge a bounded publication stall; actions still use Read().
Snapshot ReadPresentation314();
// Conventional detachable/box-magazine paths; exclude bolt actions and PKM.
inline bool MagazineRifle317(int profile){switch(profile){case 8:case 7:case 41:case 28:case 14:case 9:case 31:case 25:case 21:case 17:case 35:case 36:case 22:return true;default:return false;}}
inline bool Bolt319(int p){return p==2||p==23;}
inline bool OtherMagazine319(int p){return Bolt319(p)||p==43;}
inline bool Other319(int p){return OtherMagazine319(p)||p==15||p==18||p==32||p==40;}
inline bool Shotgun318(int p){return p==0||(p>=3&&p<=6)||p==34;}
inline bool Sidearm315(int profile){return profile==1||profile==12||profile==16||profile==20||profile==39||profile==42;}
inline bool MagazineSidearm315(int profile){return profile==1||profile==12||profile==20||profile==39;}
inline bool SupportedSidearm315(int profile){return profile==1||profile==12||profile==20;}
// A2000, BZ19, Vector (including Shredder), MP5, Skorpion.
inline bool Smg332(int profile){return profile==7||profile==9||profile==21||profile==28||profile==39;}

Settings Configuration();
void AdjustRolePose(int role,float* pose);
bool Configure(const Settings&,const wchar_t* path=nullptr);
// Actual physical buttons are retained separately from weapon-role controls.
void RawControls(float& rightGrip,float& leftGrip,float& rightTrigger,float& leftTrigger);
struct Transfer307 {bool enabled=false,contact=false,allowed=false;unsigned generation=0,entity=0,epoch=0,reference=0;unsigned long long at=0;bool rearOnly316=false,modifier321=false;};
Transfer307 TransferContact307();
void PublishTransfer307(bool enabled,bool contact,bool allowed,unsigned generation,unsigned entity,unsigned epoch,bool rearOnly316=false,bool modifier321=false);
inline bool HeldVisible(const Snapshot& s,unsigned equipped,unsigned candidate){
 return candidate && !(s.enabled ? (!s.empty&&s.entity==candidate) : equipped==candidate);
}
inline int PhysicalHand(const Snapshot& s,int role){return s.enabled&&(s.engaged||s.presentation314||s.ownerContext326)&&!s.empty&&s.entity&&s.hand!=s.nativeHoldingSide?1-role:role;}
inline bool SuppressWeapon(const Snapshot& s){return s.enabled&&(s.engaged||s.ownerContext326)&&(s.empty||(!s.active&&!s.presentation314));}
//335: render ownership follows every supported inventory family, including
//the bow's reversed native holding side. This grants no gameplay input lease.
inline bool InventoryProfile335(int p){return Sidearm315(p)||MagazineRifle317(p)||Shotgun318(p)||Other319(p)||p==10;}
inline bool RemappedOwner335(const Snapshot& s,int profile){return InventoryProfile335(profile)&&s.enabled&&!s.empty&&s.entity&&s.hand>=0&&s.hand<2&&s.nativeHoldingSide>=0&&s.nativeHoldingSide<2&&s.hand!=s.nativeHoldingSide;}
inline bool NativeFallback326(const Snapshot& s,int profile,bool pose,bool motion){return RemappedOwner335(s,profile)&&s.ownerContext326&&(!pose||!motion||SuppressWeapon(s));}
inline bool FirstPersonCamera345(bool differentProjection,bool model,bool knownWeapon){return model&&(differentProjection||knownWeapon);}
constexpr float kTransferPull309=.06f;
inline float SidePull309(int receiving,float dx,float dz,float rightX,float rightZ){return (receiving==1?-1.f:1.f)*(dx*rightX+dz*rightZ);}
struct Input {
 bool context=false,canChange=false,strict=true,inventory=false;
 unsigned long long player=0;
 unsigned entities[4]={},equipped=0,reference=0,epoch=0;
 bool tracked[2]={};float grip[2]={};int hover[2]={-1,-1};
 bool nearHandle=false;int nativeHoldingSide=0;unsigned long long now=0;
 bool sphereTransfer307=false,sphereContact307=false,transferAllowed307=false,modifierTransfer321=false;
 bool supportKeep345=true; // explicit current XR hysteresis lease, never a reload action
 bool direct333=false,handle333=false,support333=false,reloadNear333=false,ammoNear333=false,busy333=false;
 float receivingRelative309[2][2]={},receivingPosition309[2][2]={};float viewRight309[2]={1,0};
};
struct Event {int drawSlot=-1,drawHand=-1;bool stow=false,transfer=false;int pulse=-1;};
struct State {
 bool initialized=false,empty=false,wasPaused=false,down[2]={true,true},ready[2]={false,false},exited=false;
 unsigned long long player=0,last=0,transferAt=0;
 int hand=0,slot=-1,pendingSlot=-1,pendingHand=-1,receiver=-1;
 unsigned entity=0,pendingEntity=0,generation=0,reference=0,epoch=0;
 bool modifierLatched321=false,modifierArmed324=false;
 int gripAction333[2]={};
 float armedRelative309[2]={},armedReceiver309[2]={},pullAxis309[2]={1,0};
 void Empty(){modifierArmed324=false;empty=true;entity=0;slot=-1;pendingSlot=pendingHand=receiver=-1;pendingEntity=0;exited=false;++generation;}
 void Pause(const Input& in){modifierArmed324=false;wasPaused=!in.context||(!in.canChange&&!in.transferAllowed307&&!in.direct333);receiver=-1;for(int h=0;h<2;++h){down[h]=in.grip[h]>.35f;ready[h]=in.context&&in.tracked[h]&&std::isfinite(in.grip[h])&&in.grip[h]<.25f;gripAction333[h]=down[h]?fc3grab333::WaitRelease:fc3grab333::None;}last=in.now;}
 Event Step(const Input& in){
  Event event;
  if(!in.inventory||!in.player){Pause(in);return event;}
  if(!initialized||player!=in.player){initialized=true;player=in.player;reference=in.reference;epoch=in.epoch;Empty();if(!in.strict){empty=false;entity=in.equipped;for(int i=0;i<4;++i)if(in.entities[i]==entity)slot=i;hand=in.nativeHoldingSide==1?1:0;}Pause(in);return event;}
  if(reference!=in.reference||epoch!=in.epoch){reference=in.reference;epoch=in.epoch;pendingSlot=pendingHand=receiver=-1;pendingEntity=0;Pause(in);return event;}
  if(pendingSlot>=0&&in.entities[pendingSlot]!=pendingEntity){pendingSlot=pendingHand=-1;pendingEntity=0;}
  if(!empty && pendingSlot<0 && in.context && (slot<0||in.entities[slot]!=entity||(in.equipped&&in.equipped!=entity)))Empty();
  if(!in.context||(!in.canChange&&!in.transferAllowed307&&!in.direct333)||last&&(in.now<last||in.now-last>150)){Pause(in);return event;}
  if(wasPaused){Pause(in);return event;}
  bool press[2]={},release[2]={};
  for(int h=0;h<2;++h){
   if(!in.tracked[h]||!std::isfinite(in.grip[h])){ready[h]=false;down[h]=true;continue;}
   const bool next=in.grip[h]>(down[h]?.35f:.75f);press[h]=next&&!down[h]&&ready[h];release[h]=!next&&down[h];down[h]=next;if(!next)ready[h]=true;
  }
  last=in.now;
  if(pendingSlot>=0)return event;
  if(in.direct333&&!empty){
   receiver=-1;modifierArmed324=modifierLatched321=false;
   for(int h=0;h<2;++h)if(!down[h])gripAction333[h]=fc3grab333::None;
   const int other=1-hand;
   if(gripAction333[other]==fc3grab333::Support&&!in.supportKeep345)gripAction333[other]=fc3grab333::WaitRelease;
   if(down[other]&&in.tracked[other]&&ready[other]&&std::isfinite(in.grip[other])&&gripAction333[other]==fc3grab333::None){
    if(in.busy333)gripAction333[other]=fc3grab333::Reload;
    else if(in.transferAllowed307&&in.handle333){
     const int old=hand;hand=other;gripAction333[hand]=fc3grab333::Holding;gripAction333[old]=fc3grab333::WaitRelease;++generation;event.transfer=true;event.pulse=hand;return event;
    }else if(in.reloadNear333||in.ammoNear333)gripAction333[other]=fc3grab333::Reload;
    else if(in.support333)gripAction333[other]=fc3grab333::Support;
   }
  }
  // .44 only: both grips arm; physical receiving-hand lateral pull passes.
  // Both grips must return neutral before another pass. Reload leases deny
  // transfer/stow without changing the physical reload controls.
  if(!down[0]&&!down[1])modifierLatched321=false;
  if(modifierArmed324&&!in.modifierTransfer321){receiver=-1;modifierArmed324=false;}
  if(!in.direct333&&!empty&&in.modifierTransfer321){
   const int other=1-hand;
   const auto& relative=in.receivingRelative309[other];const auto& position=in.receivingPosition309[other];
   const bool allowed=in.transferAllowed307&&in.tracked[hand]&&in.tracked[other]&&down[hand]&&down[other]&&std::isfinite(relative[0])&&std::isfinite(relative[1])&&std::isfinite(position[0])&&std::isfinite(position[1]);
   if(receiver>=0){
    if(!allowed){receiver=-1;modifierArmed324=false;}
    else if(SidePull309(other,relative[0]-armedRelative309[0],relative[1]-armedRelative309[1],pullAxis309[0],pullAxis309[1])>=kTransferPull309&&SidePull309(other,position[0]-armedReceiver309[0],position[1]-armedReceiver309[1],pullAxis309[0],pullAxis309[1])>=kTransferPull309){hand=other;receiver=-1;modifierArmed324=false;modifierLatched321=true;++generation;event.transfer=true;event.pulse=hand;return event;}
   }
   const float axisLength=in.viewRight309[0]*in.viewRight309[0]+in.viewRight309[1]*in.viewRight309[1];
   if(receiver<0&&!modifierLatched321&&allowed&&press[other]&&std::isfinite(axisLength)&&axisLength>.99f&&axisLength<1.01f){receiver=other;modifierArmed324=true;armedRelative309[0]=relative[0];armedRelative309[1]=relative[1];armedReceiver309[0]=position[0];armedReceiver309[1]=position[1];pullAxis309[0]=in.viewRight309[0];pullAxis309[1]=in.viewRight309[1];transferAt=in.now;event.pulse=other;}
  }
  // Preserve the existing return-to-holster release before considering a transfer.
  if(!empty&&in.canChange){if(in.hover[hand]!=slot)exited=true;if(exited&&release[hand]&&in.hover[hand]==slot){Empty();event.stow=true;event.pulse=hand;return event;}}
  //1911: an existing native two-hand support hold is the only transfer entry.
  //Keep that hold intact; receiving grip followed by a lateral pull changes owner.
  if(!in.direct333&&!empty&&in.sphereTransfer307&&!in.modifierTransfer321){
   const int other=1-hand;const auto& relative=in.receivingRelative309[other];
   const bool allowed=in.transferAllowed307&&in.tracked[hand]&&in.tracked[other]&&std::isfinite(relative[0])&&std::isfinite(relative[1])&&std::isfinite(in.receivingPosition309[other][0])&&std::isfinite(in.receivingPosition309[other][1]);
   if(receiver>=0){
    if(!allowed||!down[receiver])receiver=-1;
    else if(SidePull309(receiver,relative[0]-armedRelative309[0],relative[1]-armedRelative309[1],pullAxis309[0],pullAxis309[1])>=kTransferPull309&&SidePull309(receiver,in.receivingPosition309[receiver][0]-armedReceiver309[0],in.receivingPosition309[receiver][1]-armedReceiver309[1],pullAxis309[0],pullAxis309[1])>=kTransferPull309){hand=receiver;receiver=-1;++generation;event.transfer=true;event.pulse=hand;return event;}
   }
   const float axisLength=in.viewRight309[0]*in.viewRight309[0]+in.viewRight309[1]*in.viewRight309[1];
   if(receiver<0&&allowed&&in.sphereContact307&&press[other]&&std::isfinite(axisLength)&&axisLength>.99f&&axisLength<1.01f){
    receiver=other;armedReceiver309[0]=in.receivingPosition309[other][0];armedReceiver309[1]=in.receivingPosition309[other][1];armedRelative309[0]=relative[0];armedRelative309[1]=relative[1];pullAxis309[0]=in.viewRight309[0];pullAxis309[1]=in.viewRight309[1];transferAt=in.now;event.pulse=other;
   }
  }
  if(!in.canChange)return event;
  if(!empty){
   const int other=1-hand;
   if(!in.direct333&&!in.sphereTransfer307&&receiver>=0){
    if(!in.nearHandle||!down[receiver]||!in.tracked[hand]||in.now-transferAt>750){receiver=-1;}
    else if(!down[hand]){hand=receiver;receiver=-1;++generation;event.transfer=true;event.pulse=hand;return event;}
   }
   if(!in.direct333&&!in.sphereTransfer307&&press[other]&&in.nearHandle&&in.tracked[hand]){
    receiver=other;transferAt=in.now;
    if(!down[hand]){hand=other;receiver=-1;++generation;event.transfer=true;event.pulse=hand;return event;}
   }
   if(in.strict)return event;
  }
  for(int h=0;h<2;++h)if(press[h]&&in.hover[h]>=0&&in.hover[h]<4&&in.entities[in.hover[h]]&&
       (empty||in.entities[in.hover[h]]!=entity)){
   pendingSlot=in.hover[h];pendingHand=h;pendingEntity=in.entities[pendingSlot];ready[h]=false;
   event.drawSlot=pendingSlot;event.drawHand=h;return event;
  }
  return event;
 }
 bool Complete(const Input& in,bool success){
  if(pendingSlot<0)return false;
  const bool confirmed=success&&in.context&&in.tracked[pendingHand]&&in.reference==reference&&in.epoch==epoch&&in.inventory&&in.player==player&&in.entities[pendingSlot]==pendingEntity&&in.equipped==pendingEntity;
  if(confirmed){slot=pendingSlot;hand=pendingHand;entity=pendingEntity;empty=false;exited=false;++generation;}
  pendingSlot=pendingHand=-1;pendingEntity=0;receiver=-1;return confirmed;
 }
};
}
