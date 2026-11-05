// Shared angular aim correction. Raw tracking and reload state are never edited.
bool ScopeStabilizationActive72();bool HoloProfile131();
struct ScopeAimState72 {V3 raw={},filtered={},pivot={};Mat4 turn={},basis={};LONG frame=-1,profile=-1;UINT epoch=0;ULONGLONG ms=0;bool active=false,scoped129=true,supported129=false;}g_scopeAim72;
Mat4 ScopeBasis72(V3 forward,V3 up){
 forward=Unit(forward);up=Sub(up,Scale(forward,up.x*forward.x+up.y*forward.y+up.z*forward.z));
 if(Length(up)<1e-6){up=fabs(forward.y)<.8?V3{0,1,0}:V3{1,0,0};up=Sub(up,Scale(forward,up.x*forward.x+up.y*forward.y+up.z*forward.z));}
 up=Unit(up);const V3 right=Unit({up.y*forward.z-up.z*forward.y,up.z*forward.x-up.x*forward.z,up.x*forward.y-up.y*forward.x});
 up={forward.y*right.z-forward.z*right.y,forward.z*right.x-forward.x*right.z,forward.x*right.y-forward.y*right.x};
 Mat4 b=HandIdentity();for(int i=0;i<3;++i){b.m[i*4]=i==0?right.x:i==1?right.y:right.z;b.m[i*4+1]=i==0?up.x:i==1?up.y:up.z;b.m[i*4+2]=i==0?forward.x:i==1?forward.y:forward.z;}return b;
}
void ResetScopeStabilization72(){g_scopeAim72={};}
V3 ScopeFilterPosition72(V3 raw,V3 previous,double dt,double strength){
 const double distance=Length(Sub(raw,previous));if(strength<=0||dt>.25||distance>.05)return raw;if(dt<=0)return previous;
 const double base=dt/(dt+strength*.06),fast=(std::min)(1.,distance/.03);double alpha=base+(1-base)*fast*fast;
 if(distance>.008)alpha=(std::max)(alpha,1-.008/distance);
 return Add(previous,Scale(Sub(raw,previous),alpha));
}
V3 ScopeFilterDirection72(V3 raw,V3 previous,double dt,double strength){
 raw=Unit(raw);previous=Unit(previous);const double dot=(std::max)(-1.,(std::min)(1.,raw.x*previous.x+raw.y*previous.y+raw.z*previous.z));
 const double angle=std::acos(dot);if(strength<=0||dt>.25||angle>.35)return raw;if(dt<=0)return previous;
 const double base=dt/(dt+strength*.10),fast=(std::min)(1.,angle/.0872664626);
 double alpha=base+(1-base)*fast*fast;
 // Bound lag to 1.5 degrees; deliberate sweeping aim catches up immediately.
 if(angle>.0261799388)alpha=(std::max)(alpha,1-.0261799388/angle);
 return Unit(Add(Scale(previous,1-alpha),Scale(raw,alpha)));
}
// One filter result feeds the rendered rig, optic and native shot command.
// Raw tracking remains available for physical grabs, pumps and reload contacts.
float g_weaponSteady129=.45f;
bool FirearmSteadyAllowed129(){
 const auto c=fc3pad::Latest();Mat4 root={},grip={};
 return g_enabled&&g_inGameplay&&MotionInteractionEnabled()&&g_eyeViewsSet&&g_handRightValid&&c.active&&
  !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::WeaponSelectionRequested()&&
  !MacheteHeld()&&!BowModel()&&!PistolBusy()&&!g_reload.held&&!g_reload.pending&&!ReloadPoseActive()&&
  !ManualReloadOwnsLeftHand()&&!R700LeftHold()&&ControllerCurrentRoot124(root,grip);
}
bool FirearmAxis129(V3& axis){
 if(!FirearmSteadyAllowed129())return false;
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;
 const V3 forward={root.m[1],root.m[5],root.m[9]};if(Length(forward)<1e-6)return false;
 axis=Unit(forward);return true;
}
void ScopeAimCommand72(V3& direction){
 const bool scoped=ScopeStabilizationActive72();const float strength=scoped?g_scopeAimSteady72:(HoloProfile131()&&g_holoEnabled131?g_holoSteady131:g_weaponSteady129);
 if((!scoped&&!FirearmSteadyAllowed129())||strength<=0||!std::isfinite(direction.x)||!std::isfinite(direction.y)||!std::isfinite(direction.z)||Length(direction)<1e-6){ResetScopeStabilization72();return;}
 const auto now=GetTickCount64();const V3 raw=Unit(direction);
 const bool fresh=g_scopeAim72.active&&g_scopeAim72.scoped129==scoped&&g_scopeAim72.supported129==g_twoHand&&g_scopeAim72.profile==g_weaponMechanicProfile&&g_scopeAim72.epoch==g_armPoseEpoch&&now-g_scopeAim72.ms<=250;
 const V3 filtered=fresh?ScopeFilterDirection72(raw,g_scopeAim72.filtered,double(now-g_scopeAim72.ms)*.001,strength):raw;
 const V3 handForward=Unit({g_handBody[0][2],g_handBody[0][6],g_handBody[0][10]}),handUp={g_handBody[0][1],g_handBody[0][5],g_handBody[0][9]};
 const Mat4 supportTurn=TurnAndMove(handForward,raw,{},{});const V3 rawUp=Sub(Apply(supportTurn,handUp),Apply(supportTurn,{}));
 const Mat4 rawBasis=ScopeBasis72(raw,rawUp);const V3 oldUp={g_scopeAim72.basis.m[1],g_scopeAim72.basis.m[5],g_scopeAim72.basis.m[9]};
 const V3 up=fresh?ScopeFilterDirection72(rawUp,oldUp,double(now-g_scopeAim72.ms)*.001,strength):rawUp;
 const Mat4 filteredBasis=ScopeBasis72(filtered,up);Mat4 inverse={};if(!Invert(rawBasis,inverse)){ResetScopeStabilization72();return;}
 const V3 rawPivot=ControllerWrist(g_handBody[0],0),pivot=fresh?ScopeFilterPosition72(rawPivot,g_scopeAim72.pivot,double(now-g_scopeAim72.ms)*.001,strength):rawPivot;
 Mat4 turn=Multiply(filteredBasis,inverse);const V3 translation=Sub(pivot,Apply(turn,rawPivot));turn.m[3]=translation.x;turn.m[7]=translation.y;turn.m[11]=translation.z;
 g_scopeAim72.raw=raw;g_scopeAim72.filtered=filtered;g_scopeAim72.pivot=pivot;g_scopeAim72.turn=turn;g_scopeAim72.basis=filteredBasis;
 g_scopeAim72.frame=g_frame+1;g_scopeAim72.profile=g_weaponMechanicProfile;g_scopeAim72.epoch=g_armPoseEpoch;g_scopeAim72.ms=now;g_scopeAim72.active=true;g_scopeAim72.scoped129=scoped;g_scopeAim72.supported129=g_twoHand;
 direction=filtered;
}
bool ScopeAimValid72(){return (g_scopeAim72.scoped129?ScopeStabilizationActive72():(FirearmSteadyAllowed129()&&!ScopeStabilizationActive72()))&&g_scopeAim72.active&&g_scopeAim72.profile==g_weaponMechanicProfile&&g_scopeAim72.epoch==g_armPoseEpoch&&g_frame<=g_scopeAim72.frame+1&&g_frame+1>=g_scopeAim72.frame&&GetTickCount64()-g_scopeAim72.ms<=250;}
void ScopeDisplayTurn72(Mat4& placed,Mat4& forearm,Mat4& upper,const Mat4& view,int variant){
 if((variant!=1&&variant!=3&&variant!=4)||!ScopeAimValid72())return;
 const Mat4 bodyToCamera=ArmBodyToCamera();Mat4 cameraToBody={},inverseView={};
 if(!Invert(bodyToCamera,cameraToBody)||!Invert(view,inverseView))return;
 const Mat4 cameraTurn=Multiply(bodyToCamera,Multiply(g_scopeAim72.turn,cameraToBody));
 const Mat4 correction=Multiply(view,Multiply(cameraTurn,inverseView));
 placed=Multiply(correction,placed);forearm=Multiply(correction,forearm);upper=Multiply(correction,upper);
}
bool ScopeStableWorldAxis72(V3& axis){
 if(!ScopeAimValid72()||!g_bodyValid)return false;
 const auto h=g_scopeAim72.filtered;
 axis={g_bodyAxes[0][0]*h.x+g_bodyAxes[1][0]*h.y+g_bodyAxes[2][0]*h.z,
       g_bodyAxes[0][1]*h.x+g_bodyAxes[1][1]*h.y+g_bodyAxes[2][1]*h.z,
       g_bodyAxes[0][2]*h.x+g_bodyAxes[1][2]*h.y+g_bodyAxes[2][2]*h.z};
 return std::isfinite(axis.x)&&std::isfinite(axis.y)&&std::isfinite(axis.z);
}
