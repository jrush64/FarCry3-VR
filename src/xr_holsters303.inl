bool g_single303=false;int g_singleHand303=0,g_singleChoice303=0;
fc3holster297::Flick g_singleFlick303;
XrVector3f Palm303(XrPosef controller,int hand);
XrPosef Wrist303(const XrSpaceLocation& loc,int hand){
 XrPosef wrist=loc.pose;wrist.orientation=testnotes::Multiply(loc.pose.orientation,g_gripRelative297);
 const float scale=(std::max)(.1f,fc3stereo::WorldScale());wrist.position=testnotes::Sub(Palm303(loc.pose,hand),testnotes::Rotate(wrist.orientation,{.0871f/scale,.0409f/scale,-.1103f/scale}));
 return wrist;
}
XrVector3f Palm303(XrPosef controller,int hand){
 // Intrinsic palm position comes from that anatomical hand's actual neutral
 // mesh binding, then follows current tracking. No guessed opposite-hand pose.
 float p[3];AcquireSRWLockShared(&g_handsLock303);const bool valid=g_handPalmEpoch303[hand]==g_roleEpoch303;memcpy(p,g_handPalm303[hand],12);ReleaseSRWLockShared(&g_handsLock303);if(!valid)return controller.position;
 const float scale=(std::max)(.1f,fc3stereo::WorldScale());
 return testnotes::Add(controller.position,testnotes::Rotate(controller.orientation,{p[0]/scale,p[1]/scale,-p[2]/scale}));
}
#include "xr_handle_grab333.inl"
fc3hands303::Input HandInput303(const fc3select290::Inventory& inv,const fc3pad::Controllers& c,ULONGLONG now){
 fc3hands303::Input input;input.inventory=inv.valid;input.player=inv.owner;input.equipped=inv.equipped;input.now=now;
 input.nativeHoldingSide=g_nativeHoldingSide303;input.reference=fc3xr::ReferenceGeneration286();input.epoch=g_roleEpoch303;input.strict=g_handSettings303.requireHolster;input.grip[0]=c.rightGrip;input.grip[1]=c.leftGrip;
 for(int i=0;i<4;++i)input.entities[i]=inv.weapons[i].entity;return input;
}
// An empty logical hand has no held-weapon operation to protect. Locker
// preparation may leave the native presentation on a different weapon; its
// old change gate must not indefinitely deny a fresh visible-owned draw.
// Context, current inventory owner, rendering, tracking, neutral grip and
// exact native equip readback remain independent requirements below.
bool EmptyHolsterDraw351(const fc3select290::Inventory& inv){
 return g_handSettings303.enabled&&g_holsters297.mode!=fc3holster297::Front&&g_hands303.initialized&&g_hands303.empty&&
  inv.valid&&inv.owner&&inv.owner==g_hands303.player&&!fc3visual292::Preparing330();
}
void BodyHolsters303(fc3pad::Controllers& c){
 const auto now=GetTickCount64();auto& gesture=g_selectGesture290;
 const bool role=g_roleContext303&&now-g_roleAt303<=120&&c.active&&g_viewsTracked&&g_referenceValid&&
  !fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&!c.menu;
 fc3select290::KeepAlive(role);const auto fresh=fc3select290::Read();auto input=HandInput303(fresh,c,now);
 const bool emptyRecovery351=EmptyHolsterDraw351(fresh);
 input.context=role;input.canChange=role&&(g_changeContext303||emptyRecovery351)&&!g_bodyRenderFailed297&&!c.x&&!c.y&&!c.a&&!c.b&&c.rightTrigger<.25f&&c.leftTrigger<.25f;
 XrSpaceLocation loc[2]={{XR_TYPE_SPACE_LOCATION},{XR_TYPE_SPACE_LOCATION}};XrPosef wrist[2];XrVector3f palm[2];
 for(int h=0;h<2;++h){input.tracked[h]=g_handSpaces[h]&&p_xrLocateSpace&&
  XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[h],g_space,g_displayTime,&loc[h]))&&fc3xr::TrackedHandLocation(loc[h]);
  if(input.tracked[h]){wrist[h]=Wrist303(loc[h],h);palm[h]=Palm303(loc[h].pose,h);}}
 input.context=role&&fresh.valid&&(g_hands303.empty||input.tracked[g_hands303.hand]);
 if(input.tracked[0]&&!g_rackBasisReady292){XrPosef native;if(SelectorWrist292(native)){
  g_gripRelative297=testnotes::Multiply(testnotes::Inverse(loc[0].pose.orientation),native.orientation);g_rackBasisReady292=true;
  for(int h=0;h<2;++h)if(input.tracked[h]){wrist[h]=Wrist303(loc[h],h);palm[h]=Palm303(loc[h].pose,h);}
 }}
 input.canChange=input.canChange&&g_rackBasisReady292;
 const auto transfer307=fc3hands303::TransferContact307();input.sphereTransfer307=transfer307.enabled;
 const bool transferFresh307=transfer307.generation==g_hands303.generation&&transfer307.entity==g_hands303.entity&&transfer307.epoch==g_roleEpoch303&&transfer307.reference==input.reference;
 input.transferAllowed307=transfer307.enabled&&transfer307.allowed&&transferFresh307&&input.context&&g_rackBasisReady292&&!g_bodyRenderFailed297&&!c.x&&!c.y&&!c.a&&!c.b&&!c.menu&&c.rightTrigger<.25f&&c.leftTrigger<.25f;
 input.sphereContact307=transferFresh307&&transfer307.contact;
 input.modifierTransfer321=transferFresh307&&transfer307.modifier321;
 ResolveHandleGrab333(input,c,loc,palm,now);

 if(input.tracked[0]&&input.tracked[1]){
  for(int receiving=0;receiving<2;++receiving){const auto d=testnotes::Sub(loc[receiving].pose.position,loc[1-receiving].pose.position);input.receivingPosition309[receiving][0]=loc[receiving].pose.position.x;input.receivingPosition309[receiving][1]=loc[receiving].pose.position.z;input.receivingRelative309[receiving][0]=d.x;input.receivingRelative309[receiving][1]=d.z;}
  const auto forward=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float flat=std::sqrt(forward.x*forward.x+forward.z*forward.z);
  if(flat>.1f){input.viewRight309[0]=-forward.z/flat;input.viewRight309[1]=forward.x/flat;}else input.viewRight309[0]=input.viewRight309[1]=0;
 }
 if(fresh.valid){g_selectShown290=fresh;fc3visual292::Preload296(fresh);g_warm292.done=true;}
 g_bodyVisible297=false;
 if(role&&fresh.valid){
  if(!g_bodyPlaced297){const auto f=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});if(f.x*f.x+f.z*f.z>.12f){const float yaw=std::atan2(-f.x,-f.z);g_selectPose290.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};}g_bodyPlaced297=true;g_anchorAt297=now;}
  int anchor=input.tracked[0]?0:1;
  if(input.tracked[1])for(int i=0;i<4;++i){const auto d=testnotes::Sub(palm[1],Palm292(RackHandle292(i)));if(d.x*d.x+d.y*d.y+d.z*d.z<.35f*.35f)anchor=1;}
  if(input.tracked[anchor])UpdateAnchor297(wrist[anchor],true,now);
  if(g_holsters297.mode==fc3holster297::Single&&!g_single303){
   bool available[4];for(int i=0;i<4;++i)available[i]=fc3visual292::Read(fresh,i)!=nullptr&&(g_hands303.empty||fresh.weapons[i].entity!=g_hands303.entity);
   if(g_singleChoice303<0||g_singleChoice303>3||!available[g_singleChoice303])g_singleChoice303=fc3holster297::Next(0,1,available);
   g_singleSlot297=g_singleChoice303;
  }
  float handles[24];bool available[4];
  for(int i=0;i<4;++i){
   const auto pose=RackHandle292(i);const auto centre=Palm292(pose);const auto axis=testnotes::Rotate(pose.orientation,{0,fc3grab295::HalfLength,0});
   const auto a=testnotes::Sub(centre,axis),b=testnotes::Add(centre,axis);const float ends[]={a.x,a.y,a.z,b.x,b.y,b.z};memcpy(handles+i*6,ends,sizeof(ends));
   available[i]=fc3visual292::Read(fresh,i)!=nullptr&&(g_holsters297.mode!=fc3holster297::Single||i==g_singleChoice303);
   if(available[i]&&(g_hands303.empty||fresh.weapons[i].entity!=g_hands303.entity))g_bodyVisible297=true;
  }
  for(int h=0;h<2;++h)if(input.tracked[h]){const float point[]={palm[h].x,palm[h].y,palm[h].z};
   input.hover[h]=fc3grab295::Handle(handles,point,available,-1,g_holsters297.radius,g_holsters297.radius+.025f);
   // A held weapon returns to the same visible capsule even without cache.
   if(!g_hands303.empty&&g_hands303.hand==h&&g_hands303.slot>=0&&g_hands303.slot<4){
    bool own345[4]={};own345[g_hands303.slot]=true;
    const int target345=fc3grab295::Handle(handles,point,own345,g_hands303.slot,g_holsters297.radius,g_holsters297.radius+.025f);
    if(target345==g_hands303.slot)input.hover[h]=target345;
   }
  }
  if(input.tracked[0]&&input.tracked[1]){const auto d=testnotes::Sub(palm[0],palm[1]);input.nearHandle=d.x*d.x+d.y*d.y+d.z*d.z<.09f*.09f;}
 }
 if(g_hands303.pendingSlot>=0&&(!input.context||g_hands303.pendingHand<0||!input.tracked[g_hands303.pendingHand]||input.reference!=g_hands303.reference||input.epoch!=g_hands303.epoch||(g_single303&&!input.canChange))){
  g_hands303.Complete(input,false);g_single303=false;gesture.Close();g_pick292.Cancel();fc3select290::KeepAlive(false);
 }
 if(gesture.open&&g_pick292.requested&&g_hands303.pendingSlot>=0){
  const int result=fc3select290::Result();
  if(result){const int hand=g_hands303.pendingHand;const bool complete=g_hands303.Complete(input,result>0);
   if(complete){fc3xr::InteractionPulse285(hand);Log("[HANDS303] exact native equip confirmed hand=%d slot=%d entity=%08X",hand,g_hands303.slot+1,g_hands303.entity);}
   gesture.Close();g_pick292.Cancel();
  }
 }
 g_stowCandidate304=!g_hands303.empty&&g_hands303.hand>=0&&g_hands303.hand<2&&input.context&&input.canChange&&input.tracked[g_hands303.hand]&&g_hands303.exited&&input.hover[g_hands303.hand]==g_hands303.slot;
 // One bounded receipt per real squeeze, including misses and denied draws.
 // This covers empty hands too, which the older weapon-grab log omitted.
 static bool gripWas351[2]={};static unsigned receipts351=0;
 for(int h=0;h<2;++h){const bool down=input.grip[h]>(gripWas351[h]?.35f:.75f);if(down&&!gripWas351[h]&&receipts351++<128){unsigned visible=0;for(int i=0;i<4;++i)if(fc3visual292::Read(fresh,i))visible|=1u<<i;
  Log("[HOLSTER351] hand=%d hover=%d empty=%d role=%d context=%d change=%d regular=%d recovery=%d tracked=%d/%d inventory=%d readyMask=%X renderFail=%d rack=%d preparing=%d paused=%d gripReady=%d pending=%d equipped=%08X held=%08X owner=%llX logicalOwner=%llX rawGrip=%.3f/%.3f buttons=%d%d%d%d trigger=%.3f/%.3f",h,input.hover[h],g_hands303.empty,role,input.context,input.canChange,g_changeContext303,emptyRecovery351,input.tracked[0],input.tracked[1],fresh.valid,visible,g_bodyRenderFailed297,g_rackBasisReady292,fc3visual292::Preparing330(),g_hands303.wasPaused,g_hands303.ready[h],g_hands303.pendingSlot,fresh.equipped,g_hands303.entity,fresh.owner,g_hands303.player,input.grip[0],input.grip[1],c.a,c.b,c.x,c.y,c.rightTrigger,c.leftTrigger);
 }gripWas351[h]=down;}
 const int prior333=g_hands303.hand,action333[2]={g_hands303.gripAction333[0],g_hands303.gripAction333[1]};
 const auto event=g_hands303.Step(input);
 if(input.direct333){const auto binding345=fc3grab333::Read();static unsigned reports=0;for(int h=0;h<2;++h)if(action333[h]!=g_hands303.gripAction333[h]&&reports++<256)Log("[GRAB345] hand=%d action=%d->%d handle=%d support=%d keep=%d reload=%d pouch=%d busy=%d allowed=%d owner=%d->%d rawGrip=%.3f/%.3f tracked=%d/%d context=%d change=%d entity=%08X gen=%u epoch=%u sync=%ld profile=%d bindingAge=%llu bindingGeneration=%u",h,action333[h],g_hands303.gripAction333[h],input.handle333,input.support333,input.supportKeep345,input.reloadNear333,input.ammoNear333,input.busy333,input.transferAllowed307,prior333,g_hands303.hand,input.grip[0],input.grip[1],input.tracked[0],input.tracked[1],input.context,input.canChange,g_hands303.entity,g_hands303.generation,input.epoch,long(g_lastSync345),binding345.profile,now>=binding345.at?now-binding345.at:~0ull,binding345.generation);}
 if(event.stow){Log("[HANDS303] holstered slot; empty hands, native inventory retained");fc3xr::InteractionPulse285(event.pulse);}
 if(event.pulse>=0&&!event.stow&&!event.transfer){Log(input.modifierTransfer321?"[MODIFIER324] both grips armed; pull receiving hand toward its side":"[HANDS309] support grip armed; pull toward receiving-hand side");fc3xr::InteractionPulse285(event.pulse);}
 if(event.transfer){Log("[HANDS303] transferred same entity=%08X to physical hand=%d",g_hands303.entity,g_hands303.hand);fc3xr::InteractionPulse285(event.pulse);}
 if(event.drawSlot>=0){
  gesture.open=true;gesture.pending=false;g_grabStart292=g_drawHeld292=wrist[event.drawHand];
  g_pick292.Begin(event.drawSlot,loc[event.drawHand].pose.position.x,loc[event.drawHand].pose.position.y,loc[event.drawHand].pose.position.z,now);
  if(g_holsters297.mode==fc3holster297::Single){g_single303=true;g_singleHand303=event.drawHand;g_singleChoice303=event.drawSlot;g_singleFlick303.Begin(c.rightX);}
  else if(fc3select290::Request(fresh,event.drawSlot)){g_pick292.requested=gesture.pending=true;}
  else{g_hands303.Complete(input,false);gesture.Close();g_pick292.Cancel();}
 }
 if(g_single303&&(g_hands303.pendingSlot<0||g_hands303.pendingHand<0)){g_single303=false;gesture.Close();g_pick292.Cancel();}
 if(g_single303){
  const int h=g_singleHand303;
  if(!role||!input.tracked[h]||!fresh.valid){g_single303=false;g_hands303.Complete(input,false);gesture.Close();g_pick292.Cancel();}
  else{
   bool available[4];for(int i=0;i<4;++i)available[i]=fc3visual292::Read(fresh,i)!=nullptr&&(g_hands303.empty||fresh.weapons[i].entity!=g_hands303.entity);
   g_singleChoice303=fc3holster297::Next(g_singleChoice303,g_singleFlick303.Step(c.rightX,c.rightY),available);
   g_singleSlot297=g_singleChoice303;g_hands303.pendingSlot=g_singleChoice303;g_hands303.pendingEntity=fresh.weapons[g_singleChoice303].entity;g_pick292.slot=g_singleChoice303;
   g_drawHeld292=wrist[h];
   if(input.grip[h]<.35f){g_single303=false;if(fc3select290::Request(fresh,g_singleChoice303)){g_pick292.requested=gesture.pending=true;}else{g_hands303.Complete(input,false);gesture.Close();g_pick292.Cancel();}}
   g_stickWait297=true;
  }
 }
 if(gesture.open&&g_pick292.requested){
  const int h=g_hands303.pendingHand;if(h>=0&&input.tracked[h]){g_drawHeld292=wrist[h];BodyContact292(wrist[h],g_pick292.slot,1);}
 }
 if(!role){g_bodyVisible297=false;g_hands303.receiver=-1;if(!gesture.pending&&!g_single303){gesture.Close();g_pick292.Cancel();}}
 gesture.hover=-1;for(int h=0;h<2;++h)if(input.hover[h]>=0)gesture.hover=input.hover[h];
 g_singleSlot297=g_singleChoice303;
 fc3pad::SetSelector290(gesture.open);if(!gesture.open){g_pick292.Cancel();fc3visual292::ClearContact();fc3visual292::Passive295(role?fresh:fc3select290::Inventory{});}
 if(gesture.open&&g_hands303.pendingSlot<0&&!g_single303){gesture.Close();g_pick292.Cancel();}
 PublishHands303(role&&fresh.valid&&(g_hands303.empty||(input.tracked[g_hands303.hand]&&fresh.equipped==g_hands303.entity)));
 if(!c.active||!g_viewsTracked||!g_referenceValid||!input.tracked[g_hands303.hand]||
    fc3pad::MenuOpen()||fc3pad::NativeMenuOpen()||fc3pad::GameplayInputBlocked()||c.menu||
    (fresh.valid&&(!g_hands303.empty&&(fresh.equipped!=g_hands303.entity||fresh.owner!=g_hands303.player)))){
  AcquireSRWLockExclusive(&g_handsLock303);g_handsSnapshot303.presentation314=false;ReleaseSRWLockExclusive(&g_handsLock303);
 }
 if(fc3pad::MenuOpen()||fc3pad::NativeMenuOpen()||fc3pad::GameplayInputBlocked()||c.menu)g_stickWait297=false;
 else if(g_stickWait297){if(!g_single303&&fabs(c.rightX)<.25f&&fabs(c.rightY)<.25f)g_stickWait297=false;else c.rightX=c.rightY=0;}
}

// Display-only state. Ownership/transfer and weapon mechanisms stay in303.
void HolsterGuides304(const fc3pad::Controllers& c){
 const bool wasReady=g_stowReady304;g_stowReady304=false;g_zonesVisible304=false;
 if(g_holsters297.mode==fc3holster297::Front||!c.active||!g_viewsTracked||!g_referenceValid)return;
 const bool menu=fc3pad::MenuOpen();const bool live=g_roleContext303&&GetTickCount64()-g_roleAt303<=120;
 if(!menu&&!live)return;
 g_zonesVisible304=g_holsters297.guides;
 // A menu can show and adjust boundaries without allowing any selection.
 if(menu&&g_zonesVisible304){if(!g_bodyPlaced297){const auto f=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float yaw=std::atan2(-f.x,-f.z);g_selectPose290.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};g_bodyPlaced297=true;}
 g_selectPose290.position={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};}
 const auto held=fc3hands303::Read();
 if(menu||!held.enabled||!held.active||!held.engaged||held.empty||held.slot<0||held.slot>3||!g_holsters297.stowOutline)return;
 g_stowReady304=g_stowCandidate304;
 if(g_stowReady304&&!wasReady)fc3xr::InteractionPulse285(held.hand);
}
