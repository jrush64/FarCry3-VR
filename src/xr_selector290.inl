#include "selector290.h"
#include "weapon_hands303.h"
#include "selector_policy290.h"
#include "selector_pickup292.h"
#include "selector_wrist293.h"
#include "selector_grab295.h"
#include "holster297.h"
fc3holster297::Settings g_holsters297;bool g_holstersLoaded297=false;
bool g_singleHeld297=false,g_stickWait297=false;
int g_singleSlot297=0;fc3holster297::Flick g_flick297;
XrQuaternionf g_gripRelative297={0,0,0,1};ULONGLONG g_anchorAt297=0;
bool g_bodyVisible297=false,g_bodyPlaced297=false,g_bodyReady297=false,g_bodyGripWas297=true,g_bodyRenderFailed297=false;
ULONGLONG g_bodyLast297=0;
bool g_zonesVisible304=false,g_stowReady304=false,g_stowCandidate304=false;
void LoadHolsters297(const wchar_t* explicitPath304=nullptr){if(g_holstersLoaded297)return;g_holstersLoaded297=true;wchar_t path[MAX_PATH];if(explicitPath304)wcsncpy_s(path,explicitPath304,_TRUNCATE);else _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_stereo.ini",fc3::DataFolder());
 g_holsters297.mode=GetPrivateProfileIntW(L"WeaponHolsters",L"Mode",0,path);
 wchar_t text[64];GetPrivateProfileStringW(L"WeaponHolsters",L"HeightOffset",L"0",text,64,path);g_holsters297.height=float(wcstod(text,nullptr));GetPrivateProfileStringW(L"WeaponHolsters",L"GrabRadius",L"0.12",text,64,path);g_holsters297.radius=float(wcstod(text,nullptr));if(!fc3holster297::Valid(g_holsters297))g_holsters297={};
 g_holsters297.guides=GetPrivateProfileIntW(L"WeaponHolsters",L"ShowZones",0,path)!=0;
 g_holsters297.stowOutline=GetPrivateProfileIntW(L"WeaponHolsters",L"StowOutline",1,path)!=0;
 for(int i=0;i<4;++i){wchar_t key[32],value[128];swprintf_s(key,L"Slot%d",i+1);GetPrivateProfileStringW(L"WeaponHolsters",key,L"",value,128,path);fc3holster297::Point point;if(swscanf_s(value,L"%f,%f,%f",&point.x,&point.y,&point.z)==3&&fc3holster297::ValidPoint(point)){g_holsters297.positions[i]=point;g_holsters297.custom[i]=true;}}

}
fc3select290::Gesture g_selectGesture290;
fc3select290::Inventory g_selectShown290;
XrPosef g_selectPose290={{0,0,0,1},{0,0,0}};
ULONGLONG g_selectAllowedAt290=0;
bool g_selectAllowed290=false;
float g_selectorWristBody292[16]={};ULONGLONG g_selectorWristAt292=0;
void PublishSelectorWrist293(const float* nativeBody){g_selectorWristAt292=fc3wrist293::ChangeLocalFrame(nativeBody,g_selectorWristBody292)?GetTickCount64():0;}
XrPosef g_rackWrist292={{0,0,0,1},{0,0,0}},g_grabStart292,g_drawHeld292;
XrPosef g_grabController294={};
bool g_rackBasisReady292=false;
float g_selectorGrip292=0;
bool g_previousPalmValid292=false;XrVector3f g_previousPalm292={};
fc3pickup292::Pickup g_pick292;
struct Warm292 {int original=-1,target=-1;bool sent=false,restore=false,done=false,failed=false,cancel=false,inflight=false,restoreRequest=false;ULONGLONG at=0;} g_warm292;
bool SelectorPreparing293(){return g_selectGesture290.open&&!g_warm292.done;}
bool SelectorWrist292(XrPosef& pose){return g_selectorWristAt292&&GetTickCount64()-g_selectorWristAt292<120&&fc3xr::BodyToReferencePose(g_selectorWristBody292,(std::max)(.1f,fc3stereo::WorldScale()),g_referenceYaw,g_referencePosition,pose);}
XrPosef RackHandle292(int slot){
 if(g_holsters297.mode!=fc3holster297::Front){
  const auto p=fc3holster297::Centre(g_holsters297,slot);const auto palm=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,{p.x,p.y,p.z}));
  const float a=-.7853981634f;const auto down=XrQuaternionf{std::sin(a),0,0,std::cos(a)};
  XrPosef pose;pose.orientation=testnotes::Multiply(testnotes::Multiply(g_selectPose290.orientation,down),g_gripRelative297);
  const float scale=(std::max)(.1f,fc3stereo::WorldScale());const auto offset=testnotes::Rotate(pose.orientation,{.0871f/scale,.0409f/scale,-.1103f/scale});pose.position=testnotes::Sub(palm,offset);return pose;
 }
 const XrVector3f centre={slot%2?.25f:-.25f,slot<2?-.12f:-.43f,-.60f};
 XrPosef pose=g_rackWrist292;pose.position=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,centre));return pose;
}
XrQuaternionf BlendRotation292(XrQuaternionf a,XrQuaternionf b,float t){
 if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0){b={-b.x,-b.y,-b.z,-b.w};}
 XrQuaternionf q={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t};const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return n>1e-6f?XrQuaternionf{q.x/n,q.y/n,q.z/n,q.w/n}:a;
}
void BodyContact292(XrPosef pose,int slot,float grip){
 const XrQuaternionf inverse={0,-std::sin(g_referenceYaw*.5f),0,std::cos(g_referenceYaw*.5f)};
 const auto p=testnotes::Rotate(inverse,testnotes::Sub(pose.position,g_referencePosition));const auto q=testnotes::Multiply(inverse,pose.orientation);
 const XrVector3f axes[3]={testnotes::Rotate(q,{1,0,0}),testnotes::Rotate(q,{0,1,0}),testnotes::Rotate(q,{0,0,1})};const float scale=(std::max)(.1f,fc3stereo::WorldScale());
 float body[16]={};for(int col=0;col<3;++col){const auto& a=axes[col];body[col]=a.x*(col==2?-1:1);body[4+col]=a.y*(col==2?-1:1);body[8+col]=-a.z*(col==2?-1:1);}body[3]=p.x*scale;body[7]=p.y*scale;body[11]=-p.z*scale;body[15]=1;fc3visual292::Contact(body,slot,grip);
}
XrVector3f Palm292(XrPosef wrist){const float scale=(std::max)(.1f,fc3stereo::WorldScale());return testnotes::Add(wrist.position,testnotes::Rotate(wrist.orientation,{.0871f/scale,.0409f/scale,-.1103f/scale}));}
void UpdateAnchor297(XrPosef wrist,bool tracked,ULONGLONG now){
 if(g_holsters297.mode==fc3holster297::Front)return;
 const XrVector3f head={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
 bool reaching=g_singleHeld297||g_pick292.requested;
 if(tracked){const auto hand=Palm292(wrist);for(int i=0;i<4;++i){const auto d=testnotes::Sub(hand,Palm292(RackHandle292(i)));if(d.x*d.x+d.y*d.y+d.z*d.z<.35f*.35f)reaching=true;}}
 const auto f=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float horizontal=std::sqrt(f.x*f.x+f.z*f.z);
 if(!reaching)g_selectPose290.position=head;
 if(!reaching&&horizontal>.35f){const auto old=testnotes::Rotate(g_selectPose290.orientation,{0,0,-1});const float current=std::atan2(-old.x,-old.z),wanted=std::atan2(-f.x,-f.z);float delta=std::atan2(std::sin(wanted-current),std::cos(wanted-current));const float dt=g_anchorAt297&&now>=g_anchorAt297?(std::min)(.05f,float(now-g_anchorAt297)/1000.f):0;const float limit=dt*1.5f;delta=(std::max)(-limit,(std::min)(limit,delta));g_selectPose290.orientation={0,std::sin((current+delta)*.5f),0,std::cos((current+delta)*.5f)};}
 g_anchorAt297=now;
}
int Collision292(XrVector3f wanted,XrVector3f& resolved){
 if(!g_previousPalmValid292){g_previousPalm292=wanted;g_previousPalmValid292=true;}
 const auto delta=testnotes::Sub(wanted,g_previousPalm292);const float length=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z;float first=2;int slot=-1;resolved=wanted;
 const float scale=(std::max)(.1f,fc3stereo::WorldScale());
 for(int i=0;i<4;++i){const auto* visual=fc3visual292::Read(g_selectShown290,i);if(!visual)continue;const auto pose=RackHandle292(i);const auto inverse=testnotes::Inverse(pose.orientation);
  const auto previous=testnotes::Rotate(inverse,testnotes::Sub(g_previousPalm292,pose.position)),point=testnotes::Rotate(inverse,testnotes::Sub(wanted,pose.position));
  fc3contact292::P hit;if(!visual->collider.Sweep({previous.x*scale,previous.y*scale,previous.z*scale},{point.x*scale,point.y*scale,point.z*scale},.025f*scale,hit))continue;
  const auto world=testnotes::Add(pose.position,testnotes::Rotate(pose.orientation,{hit.x/scale,hit.y/scale,hit.z/scale}));const auto moved=testnotes::Sub(world,g_previousPalm292);const float fraction=length>1e-10f?(moved.x*delta.x+moved.y*delta.y+moved.z*delta.z)/length:0;
  if(fraction<first){first=fraction;slot=i;resolved=world;}
 }return slot;
}
void WarmTick292(ULONGLONG now){
 auto& w=g_warm292;const auto& inv=g_selectShown290;if(w.done||!inv.valid)return;
 fc3visual292::Preload296(inv); // restore before choosing any native preparation request
 // A queued native draw can complete after cancellation. Drain its exact
 // readback before deciding whether the original weapon needs restoration.
 if(w.inflight){const int result=fc3select290::Result();if(!result)return;w.inflight=false;
  if(result<0){if(w.restoreRequest){Log("[SELECT295] native original-weapon restoration refused; cancelled");g_selectGesture290.Close();return;}w.failed=true;w.restore=true;w.target=-1;return;}
 }

 if(w.target>=0&&now-w.at>6000){if(w.restore||w.failed||w.cancel){Log("[SELECT295] restoration deadline expired; selector cancelled");g_selectGesture290.Close();return;}w.failed=true;w.restore=true;w.target=-1;return;}
 if(w.original<0){for(int i=0;i<4;++i)if(inv.weapons[i].entity&&inv.weapons[i].entity==inv.equipped)w.original=i;if(w.original<0){w.failed=true;g_selectGesture290.Close();return;}}
 int desired=-1;if(w.restore||w.cancel||w.failed)desired=w.original;else for(int i=0;i<4;++i)if(inv.weapons[i].entity&&!fc3visual292::Read(inv,i)){desired=i;break;}
 if(desired<0){w.restore=true;desired=w.original;}
 if(desired!=w.target){w.target=desired;w.sent=false;w.at=now;fc3visual292::Target(inv,desired);}
 // A native request is complete only after291's exact equipped readback.
 if(w.sent&&fc3select290::Result()<0){if(w.restore||w.failed||w.cancel){Log("[SELECT295] original weapon restoration refused; selector cancelled");g_selectGesture290.Close();return;}w.failed=true;w.restore=true;w.target=-1;return;}
 if(inv.equipped!=inv.weapons[desired].entity){
  if(!w.sent&&fc3select290::Request(inv,desired)){w.sent=true;w.inflight=true;w.restoreRequest=w.restore||w.cancel||w.failed;Log("[SELECT295] preparing owned appearance slot=%d",desired+1);}return;
 }
 if(w.sent&&fc3select290::Result()!=1)return;
 fc3visual292::Target(inv,desired);
 if(w.restore||w.failed||w.cancel){w.done=true;fc3visual292::Target(inv,-1);fc3xr::SetTestNote(L"");if(w.failed||w.cancel)g_selectGesture290.Close();return;}
 if(fc3visual292::Read(inv,desired)){w.target=-1;return;}
 if(now-w.at>5000){Log("[SELECT295] native visual not ready slot=%d; restoring original; no base-mesh fallback",desired+1);w.failed=true;w.restore=true;w.target=-1;}
}
void BodyHolstersControls297(fc3pad::Controllers& c){
 auto& g=g_selectGesture290;const auto now=GetTickCount64();const float rawX=c.rightX,rawY=c.rightY;const bool wasHeld=g_singleHeld297;g_selectorGrip292=c.rightGrip;
 const bool allowed=g_selectAllowed290&&now-g_selectAllowedAt290<=120&&c.active&&!fc3pad::MenuOpen()&&!fc3pad::GameplayInputBlocked()&&!c.menu&&!c.x&&!c.y&&!c.b&&!c.leftStickClick&&!c.rightStickClick&&(g.open||c.leftGrip<.35f)&&c.leftTrigger<.25f&&c.rightTrigger<.25f;
 XrSpaceLocation loc={XR_TYPE_SPACE_LOCATION};const bool rawTracked=g_viewsTracked&&g_referenceValid&&g_handSpaces[0]!=XR_NULL_HANDLE&&XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[0],g_space,g_displayTime,&loc))&&fc3xr::TrackedHandLocation(loc);
 XrPosef wrist={};const bool tracked=rawTracked&&SelectorWrist292(wrist),down=std::isfinite(c.rightGrip)&&c.rightGrip>(g_bodyGripWas297?.35f:.75f);const bool gap=g_bodyLast297&&(now<g_bodyLast297||now-g_bodyLast297>120);g_bodyLast297=now;const bool edge=down&&!g_bodyGripWas297&&!gap;g_bodyGripWas297=down;
 fc3select290::KeepAlive(allowed&&rawTracked);
 const auto fresh=fc3select290::Read();bool changed=g_selectShown290.valid&&fresh.valid&&fresh.owner!=g_selectShown290.owner;
 for(int i=0;i<4;++i)if(g_selectShown290.valid&&fresh.valid&&(fresh.weapons[i].id!=g_selectShown290.weapons[i].id||fresh.weapons[i].entity!=g_selectShown290.weapons[i].entity))changed=true;
 const bool eligible=allowed&&rawTracked&&fresh.valid&&std::isfinite(c.rightGrip)&&!g_bodyRenderFailed297;
 if(!eligible||changed||(g.open&&(!tracked||gap||c.a))){if(g.open)Log("[HOLSTER297] active selection cancelled eligible=%d loadoutChanged=%d A=%d",eligible,changed,c.a);g.Close();g_pick292.Cancel();g_singleHeld297=false;g_bodyReady297=false;}
 g_bodyVisible297=false;
 if(eligible){
  g_selectShown290=fresh;fc3visual292::Preload296(fresh);g_warm292.done=true;
  if(!g_bodyPlaced297){const auto f=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});if(f.x*f.x+f.z*f.z>.12f){const float yaw=std::atan2(-f.x,-f.z);g_selectPose290.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};}g_anchorAt297=now;g_bodyPlaced297=true;g_rackBasisReady292=false;}
  if(tracked&&!g_rackBasisReady292){g_gripRelative297=testnotes::Multiply(testnotes::Inverse(loc.pose.orientation),wrist.orientation);g_rackBasisReady292=true;}
  UpdateAnchor297(wrist,tracked,now);
  if(!g.open&&g_holsters297.mode==fc3holster297::Single){
   if(g_singleSlot297<0||g_singleSlot297>3||!fc3visual292::Read(fresh,g_singleSlot297)||fresh.weapons[g_singleSlot297].entity==fresh.equipped){
    for(int i=0;i<4;++i)if(fresh.weapons[i].entity!=fresh.equipped&&fc3visual292::Read(fresh,i)){g_singleSlot297=i;break;}
   }
  }
  bool occupied[4];for(int i=0;i<4;++i){occupied[i]=fc3visual292::Read(fresh,i)!=nullptr&&fresh.weapons[i].entity!=fresh.equipped;g_bodyVisible297=g_bodyVisible297||occupied[i];}
  if(gap||!tracked)g_bodyReady297=false;if(!down&&tracked&&!c.a)g_bodyReady297=true;
  const int previousHover=g.hover;g.hover=-1;
  if(tracked&&!g.open){float handles[24];const auto p=Palm292(wrist);const float palm[]={p.x,p.y,p.z};bool selectable[4];for(int i=0;i<4;++i){const auto pose=RackHandle292(i);const auto centre=Palm292(pose),axis=testnotes::Rotate(pose.orientation,{0,fc3grab295::HalfLength,0});const auto a=testnotes::Sub(centre,axis),b=testnotes::Add(centre,axis);const float points[]={a.x,a.y,a.z,b.x,b.y,b.z};memcpy(handles+i*6,points,sizeof(points));selectable[i]=occupied[i]&&(g_holsters297.mode!=fc3holster297::Single||i==g_singleSlot297);}g.hover=fc3grab295::Handle(handles,palm,selectable,previousHover,g_holsters297.radius,g_holsters297.radius+.025f);}
  if(!g.open&&g.hover>=0&&g.hover!=previousHover)fc3xr::InteractionPulse285(0);
  if(!g.open&&edge&&g_bodyReady297&&g.hover>=0&&!c.a&&!changed){const int slot=g.hover;g_bodyReady297=false;g.open=true;g.ready=false;g_grabStart292=g_drawHeld292=wrist;g_pick292.Begin(slot,loc.pose.position.x,loc.pose.position.y,loc.pose.position.z,now);fc3pad::SetSelector290(true);fc3xr::InteractionPulse285(0);
   if(g_holsters297.mode==fc3holster297::Single){g_singleSlot297=slot;g_singleHeld297=true;g_grabController294=loc.pose;g_flick297.Begin(rawX);Log("[HOLSTER297] direct single grip slot=%d",slot+1);}
   else if(fc3select290::Request(fresh,slot)){g_pick292.requested=true;g.pending=true;Log("[HOLSTER297] direct body grip equips slot=%d entity=%08X",slot+1,fresh.weapons[slot].entity);}
   else{g.Close();g_pick292.Cancel();}
  }
  fc3visual292::ClearContact();
  if(g.open&&g_singleHeld297){const int step=g_flick297.Step(rawX,rawY),next=fc3holster297::Next(g_singleSlot297,step,occupied);if(next!=g_singleSlot297){g_singleSlot297=next;g_pick292.slot=next;fc3xr::InteractionPulse285(0);Log("[HOLSTER297] single preview slot=%d entity=%08X",next+1,fresh.weapons[next].entity);}
   const auto delta=testnotes::Sub(loc.pose.position,g_grabController294.position);g_drawHeld292.position=testnotes::Add(g_grabStart292.position,delta);g_drawHeld292.orientation=testnotes::Multiply(testnotes::Multiply(loc.pose.orientation,testnotes::Inverse(g_grabController294.orientation)),g_grabStart292.orientation);
   if(c.rightGrip<.35f){g_singleHeld297=false;if(fc3select290::Request(fresh,g_singleSlot297)){g_pick292.requested=true;g.pending=true;Log("[HOLSTER297] single release equips slot=%d entity=%08X",g_singleSlot297+1,fresh.weapons[g_singleSlot297].entity);}else{g.Close();g_pick292.Cancel();}}
   else BodyContact292(g_drawHeld292,g_pick292.slot,1);
  }
  if(g.open&&g_pick292.requested){const int result=fc3select290::Result();if(result>0){fc3xr::InteractionPulse285(0);g.Close();g_pick292.Cancel();}else if(result<0){g.Close();g_pick292.Cancel();}else BodyContact292(g_drawHeld292,g_pick292.slot,1);}
 }else{g_bodyReady297=false;g_bodyPlaced297=false;fc3visual292::ClearContact();}
 fc3pad::SetSelector290(g.open);
 if(!g.open){g_pick292.Cancel();fc3visual292::ClearContact();fc3visual292::Passive295(eligible?fresh:fc3select290::Inventory{});}fc3xr::SetTestNote(L"");
 const bool cycling=wasHeld||g_singleHeld297;if(cycling)g_stickWait297=true;
 if(fc3pad::MenuOpen()||fc3pad::GameplayInputBlocked())g_stickWait297=false;
 else if(g_stickWait297){if(!cycling&&std::isfinite(rawX)&&std::isfinite(rawY)&&std::fabs(rawX)<.25f&&std::fabs(rawY)<.25f)g_stickWait297=false;else c.rightX=c.rightY=0;}
}
#include "xr_holsters303.inl"
#include "xr_locker_prepare330.inl"
void SelectorControls290(fc3pad::Controllers& c){
 LoadHolsters297();LoadHands303();
 if(LockerPrepareControls330(c))return;
 if(g_handSettings303.enabled&&g_holsters297.mode!=fc3holster297::Front){BodyHolsters303(c);HolsterGuides304(c);return;}
 if(g_hands303.initialized){g_hands303={};PublishHands303(false);}
 if(g_holsters297.mode!=fc3holster297::Front){BodyHolstersControls297(c);HolsterGuides304(c);return;}g_bodyVisible297=g_zonesVisible304=g_stowReady304=false;
 auto& g=g_selectGesture290;const auto now=GetTickCount64();XrPosef wrist={};g_selectorGrip292=c.rightGrip;
 const bool allowed=g_selectAllowed290&&now-g_selectAllowedAt290<=120&&c.active&&!fc3pad::MenuOpen()&&!fc3pad::GameplayInputBlocked()&&
  !c.menu&&!c.x&&!c.y&&!c.b&&!c.leftStickClick&&!c.rightStickClick&&(g.open||c.leftGrip<.35f)&&c.leftTrigger<.25f&&c.rightTrigger<.25f;
 XrSpaceLocation loc={XR_TYPE_SPACE_LOCATION};
 const bool rawTracked=g_viewsTracked&&g_referenceValid&&g_handSpaces[0]!=XR_NULL_HANDLE&&XR_SUCCEEDED(p_xrLocateSpace(g_handSpaces[0],g_space,g_displayTime,&loc))&&fc3xr::TrackedHandLocation(loc);
 const bool tracked=rawTracked&&SelectorWrist292(wrist);
 int touch=-1;
 if(tracked&&g.open&&g_warm292.done&&g_pick292.slot<0){float handles[24],palm[]={Palm292(wrist).x,Palm292(wrist).y,Palm292(wrist).z};bool occupied[4];for(int i=0;i<4;++i){const auto pose=RackHandle292(i);const auto centre=Palm292(pose);const auto axis=testnotes::Rotate(pose.orientation,{0,fc3grab295::HalfLength,0});const auto a=testnotes::Sub(centre,axis),b=testnotes::Add(centre,axis);const float points[6]={a.x,a.y,a.z,b.x,b.y,b.z};memcpy(handles+i*6,points,sizeof(points));occupied[i]=fc3visual292::Read(g_selectShown290,i)!=nullptr&&g_selectShown290.weapons[i].entity!=g_selectShown290.equipped;}touch=fc3grab295::Handle(handles,palm,occupied,g.hover);}
 const bool was=g.open;const int previousHover=g.hover;
 // A wrist sample is first published by the native arm draw after opening.
 const bool openingTracking=rawTracked;
 // Render sampling can skip an arm draw or arrive late during native loading.
 // Keep the visible rack while XR tracking remains valid, but never grab from
 // an absent/stale rendered wrist. Actual tracking/action loss still closes it.
 const int grab=g.Step(allowed,openingTracking,c.a,c.rightGrip,touch,now);
 if(!tracked){g.ready=false;g.hover=-1;}
 if(was&&!g.open)Log("[SELECT295] rack close allowed=%d rawTracked=%d wrist=%d age=%llu A=%d warm=%d pending=%d",allowed,rawTracked,tracked,g_selectorWristAt292?now-g_selectorWristAt292:~0ull,c.a,g_warm292.done,g.pending);
 static bool wristMissing293=false;
 if(g.open&&g_warm292.done&&!tracked&&!wristMissing293)Log("[SELECT295] waiting for rendered wrist; rack retained; age=%llu",g_selectorWristAt292?now-g_selectorWristAt292:~0ull);
 wristMissing293=g.open&&g_warm292.done&&!tracked;
 if(g.open&&!was){
  const auto forward=testnotes::Rotate(g_views[0].pose.orientation,{0,0,-1});const float yaw=std::atan2(-forward.x,-forward.z);
  g_selectPose290.orientation={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};g_selectPose290.position={(g_views[0].pose.position.x+g_views[1].pose.position.x)*.5f,(g_views[0].pose.position.y+g_views[1].pose.position.y)*.5f,(g_views[0].pose.position.z+g_views[1].pose.position.z)*.5f};
  // The controller-owned native wrist basis is shared with normal held weapons.
  g_rackWrist292=tracked?wrist:g_selectPose290;g_rackBasisReady292=tracked;g_selectShown290={};g_warm292={};g_pick292.Cancel();g_previousPalmValid292=false;fc3xr::SetTestNote(L"Preparing weapons...");
  Log("[SELECT295] opening native-owned rack; release A/grip; grip a HANDLE to equip immediately");
 }
 if(was&&!g.open&&allowed&&tracked&&!g_warm292.done&&g_warm292.original>=0){g_warm292.cancel=true;g_warm292.restore=true;g_warm292.target=-1;g.open=true;g.pending=false;}
 const bool passive295=allowed&&rawTracked;
 fc3select290::KeepAlive(g.open||passive295);
 if(g.open&&tracked&&!g_rackBasisReady292){g_rackWrist292=wrist;g_rackBasisReady292=true;}
 if(g.open){
  const auto fresh=fc3select290::Read();
  if(fresh.valid){bool changed=g_selectShown290.valid&&fresh.owner!=g_selectShown290.owner;for(int i=0;i<4;++i)if(g_selectShown290.valid&&(fresh.weapons[i].id!=g_selectShown290.weapons[i].id||fresh.weapons[i].entity!=g_selectShown290.weapons[i].entity))changed=true;
   if(changed){g.Close();Log("[SELECT295] owned loadout changed; cancelled without further native requests");}else g_selectShown290=fresh;
  }else if(g_selectShown290.valid&&now-g_selectShown290.at>150)g.Close();
 }
 if(g.open&&!g_warm292.done)WarmTick292(now);
 if(grab>=0&&g.open&&g_warm292.done){
  g.ready=false;g_grabStart292=RackHandle292(grab);g_drawHeld292=g_grabStart292;
  g_pick292.Begin(grab,loc.pose.position.x,loc.pose.position.y,loc.pose.position.z,now);
  if(fc3select290::Request(g_selectShown290,grab)){g_pick292.requested=true;g.pending=true;fc3xr::InteractionPulse285(0);Log("[SELECT295] grip equips immediately slot=%d exact entity=%08X",grab+1,g_selectShown290.weapons[grab].entity);}
  else{g_pick292.Cancel();g.pending=false;Log("[SELECT295] grip request refused slot=%d",grab+1);}
 }
 // No mesh pushback or hover pose pinning. The hand can reach the forgiving
 // handle volume through the preview geometry. Grip queues equip immediately.
 fc3visual292::ClearContact();
 if(g.open&&g_pick292.requested){
  const int result=fc3select290::Result();
  if(result>0){fc3xr::InteractionPulse285(0);g.Close();}
  else if(result<0){g_pick292.Cancel();g.pending=false;g.ready=false;}
  else BodyContact292(g_drawHeld292,g_pick292.slot,1);
 }
 if(g.open&&g.hover>=0&&g.hover!=previousHover)fc3xr::InteractionPulse285(0);
 fc3pad::SetSelector290(g.open);
 if(!g.open){g_previousPalmValid292=false;g_pick292.Cancel();fc3visual292::ClearContact();fc3xr::SetTestNote(L"");
  fc3select290::KeepAlive(passive295); // refresh read-only inventory while eligible on foot
  fc3visual292::Passive295(passive295?fc3select290::Read():fc3select290::Inventory{});
 }
}
#include "selector_render292.inl"
