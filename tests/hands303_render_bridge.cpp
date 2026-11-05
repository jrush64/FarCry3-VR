#include "../src/xr_presenter.cpp"
bool PublishedForegripSupport343(){
 const auto b=fc3grab333::Read();fc3hands303::Input input;input.context=input.canChange=true;input.reference=b.reference;input.now=GetTickCount64();input.tracked[0]=input.tracked[1]=true;
 XrSpaceLocation loc[2]={};for(auto& l:loc)l.pose.orientation.w=1;
 XrVector3f palm[2]={};const int owner=g_hands303.hand;palm[owner]={b.handle[0],b.handle[1],b.handle[2]};palm[1-owner]={b.foregrip[0],b.foregrip[1],b.foregrip[2]};
 const auto a=g_views[0].pose,z=g_views[1].pose;g_views[0].pose.position=g_views[1].pose.position={100,100,100};fc3pad::Controllers c={};c.active=true;if(owner)c.rightGrip=1;else c.leftGrip=1;
 ResolveHandleGrab333(input,c,loc,palm,input.now);g_views[0].pose=a;g_views[1].pose=z;
 return input.direct333&&input.support333&&!input.handle333&&!input.reloadNear333&&!input.ammoNear333;
}
void PublishLeverSupport340(int owner,bool held,bool busy){
 fc3grab333::Binding b;b.mode=b.enabled=true;b.busy=busy;b.profile=0;b.entity=g_hands303.entity;b.generation=g_hands303.generation;b.epoch=g_roleEpoch303;b.reference=fc3xr::ReferenceGeneration286();b.at=GetTickCount64();fc3grab333::Publish(b);
 g_hands303.gripAction333[1-owner]=held?fc3grab333::Support:fc3grab333::None;
 fc3pad::Controllers c={};c.active=true;if(owner==0)c.leftGrip=held?1.f:0.f;else c.rightGrip=held?1.f:0.f;
 RememberRaw303(c);WeaponRoleControls303(c);fc3pad::Publish(c);
}
// Test-only injection at the immutable XR/render publication boundary.
void SetRenderHands303(int hand,bool empty,unsigned epoch){g_roleEpoch303=epoch;
 g_hands303.initialized=true;g_hands303.hand=hand;g_hands303.empty=empty;g_hands303.slot=0;g_hands303.entity=100;g_hands303.player=123;++g_hands303.generation;
 g_roleContext303=true;g_roleAt303=GetTickCount64();g_handSettings303.enabled=true;g_handsLoaded303=true;PublishHands303(true);
}

void SetRenderReceiver309(int receiver){g_hands303.receiver=receiver;PublishHands303(true);}

void PublishReloadControls312(float rightGrip,float leftGrip,bool b=false,bool x=false){
 fc3pad::Controllers c={};c.active=true;c.rightGrip=rightGrip;c.leftGrip=leftGrip;c.b=b;c.x=x;
 RememberRaw303(c);WeaponRoleControls303(c);fc3pad::Publish(c);
}

void StallRender314(){const auto now=GetTickCount64();g_handsSnapshot303.at=now-400;g_handsSnapshot303.confirmed314=now-400;g_roleAt303=now-400;PublishHands303(false);}

void PublishSidearmControls315(float rt,float lt,float rg=0,float lg=0,bool b=false,bool x=false){
 fc3pad::Controllers c={};c.active=true;c.rightTrigger=rt;c.leftTrigger=lt;c.rightGrip=rg;c.leftGrip=lg;c.b=b;c.x=x;
 RememberRaw303(c);WeaponRoleControls303(c);fc3pad::Publish(c);
}

void SetRenderNative319(int side){g_nativeHoldingSide303=side;PublishHands303(true);}

void AgeRender326(unsigned age){const auto now=GetTickCount64();g_handsSnapshot303.at=now-age;g_handsSnapshot303.confirmed314=now-age;g_roleAt303=now-age;PublishHands303(false);}
void PresentationOnly326(){fc3xr::SetHandsContext303(false,false,g_roleEpoch303,0,true);PublishHands303(false);}
void EndRenderContext326(){fc3xr::SetHandsContext303(false,false,g_roleEpoch303,0);}

#include "handle333_consume_bridge.inl"

unsigned RoutePhysicalGrip350(const float* right350,const float* left350,float scale350,float squeeze350,bool reset350){
 const float* bodies350[2]={right350,left350};const int owner350=g_hands303.hand;const auto b350=fc3grab333::Read();
 fc3hands303::Input in350;in350.context=in350.canChange=in350.inventory=true;in350.player=g_hands303.player;in350.equipped=in350.entities[0]=g_hands303.entity;in350.epoch=g_roleEpoch303;in350.reference=b350.reference;in350.nativeHoldingSide=g_nativeHoldingSide303;in350.now=GetTickCount64();in350.tracked[0]=in350.tracked[1]=true;
 XrSpaceLocation loc350[2]={};XrVector3f palm350[2]={};
 for(int h350=0;h350<2;++h350){loc350[h350].pose.orientation.w=1;loc350[h350].pose.position={bodies350[h350][3]/scale350,bodies350[h350][7]/scale350,-bodies350[h350][11]/scale350};palm350[h350]=Palm303(loc350[h350].pose,h350);}
 fc3pad::Controllers c350={};c350.active=true;if(owner350)c350.rightGrip=squeeze350;else c350.leftGrip=squeeze350;in350.grip[1-owner350]=squeeze350;
 if(reset350){g_hands303.reference=in350.reference;g_hands303.epoch=in350.epoch;g_hands303.wasPaused=false;g_hands303.last=in350.now;for(int h350=0;h350<2;++h350){g_hands303.down[h350]=false;g_hands303.ready[h350]=true;g_hands303.gripAction333[h350]=fc3grab333::None;}}
 ResolveHandleGrab333(in350,c350,loc350,palm350,in350.now);const auto event350=g_hands303.Step(in350);PublishHands303(true);RememberRaw303(c350);WeaponRoleControls303(c350);fc3pad::Publish(c350);
 return (in350.handle333?1u:0u)|(in350.support333?2u:0u)|(in350.reloadNear333?4u:0u)|(in350.ammoNear333?8u:0u)|(event350.transfer?16u:0u)|(unsigned(g_hands303.gripAction333[1-owner350])<<8);
}
void PublishRawPouch361(float rightGrip){fc3pad::Controllers raw={};raw.active=true;raw.rightGrip=rightGrip;RememberRaw303(raw);auto normalized=raw;normalized.rightGrip=0;fc3pad::Publish(normalized);}
void DoorFixtureReference363(){g_referenceValid=g_viewsValid=true;g_weaponReloadGroup=g_twoHandGroup=false;g_views[0].pose.orientation={0,0,0,1};g_views[0].pose.position={0,0,0};}
unsigned DoorFixtureStatus363(){return GuideMarkerVisible(19)&&GuideMarkerVisible(20)?1u|(g_doorHighlightHeld363?2u:0u):0u;}
bool DoorFixtureExpired363(){g_doorHighlightAt363=GetTickCount64()-151;return !GuideMarkerVisible(19)&&!GuideMarkerVisible(20);}
