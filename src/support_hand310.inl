//1911 left ownership: physical right support mirrors the native left support.
//Capture the game's own support pose, never a magazine/rack or animated free hand.
struct NativeSupport310 {
 Mat4 local[18]={};int profile315=-1;unsigned entity=0,epoch=~0u;bool valid=false,confirmed=false;
} g_nativeSupport310;
bool RoleRoot310(const PhysicalArmScope303& scope,Mat4& root){
 Mat4 grip={};if(!scope.active)return ControllerCurrentRoot124(root,grip);
 float physical[2][16];memcpy(physical,g_handBody,sizeof(physical));const bool valid[2]={g_handRightValid,g_handLeftValid};
 memcpy(g_handBody,scope.oldBody,sizeof(g_handBody));g_handRightValid=scope.oldValid[0];g_handLeftValid=scope.oldValid[1];
 const bool result=ControllerCurrentRoot124(root,grip);
 memcpy(g_handBody,physical,sizeof(physical));g_handRightValid=valid[0];g_handLeftValid=valid[1];return result;
}
bool SupportEligible310(bool presentation314=false){
 const auto held=presentation314?fc3hands303::ReadPresentation314():fc3hands303::Read();
 return fc3hands303::SupportedSidearm315(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&held.enabled&&held.active&&held.engaged&&!held.empty&&held.nativeHoldingSide==0&&held.entity&&
  !MacheteHeld()&&!PistolBusy()&&!g_pistolReload.held&&!g_pistolReload.rackHeld&&!g_pistolReload.rackReturning&&!g_reload.held&&!g_reload.pending&&!g_reloadAlignment&&!ReloadPoseActive()&&!g_pistolMenu.open;
}
void CaptureNativeSupport310(const PhysicalArmScope303& scope,int side,const Mat4& toView,const float* raw){
 if(side!=1||!SupportEligible310()||!g_frameGunValid||fc3pad::Latest().rightTrigger>.3f)return;
 const auto& contact=g_supportLocal236[g_weaponMechanicProfile];if(!contact.valid||contact.epoch!=g_armPoseEpoch)return;
 const auto held=fc3hands303::Read();auto& cache=g_nativeSupport310;
 if(cache.entity!=held.entity||cache.epoch!=g_armPoseEpoch||cache.profile315!=g_weaponMechanicProfile)cache={};
 const bool confirmed=false; // raw native seed is provisional, never a displayed support capture
 if(cache.valid)return;
 Mat4 root={},inverseRoot={},inverseView={};if(!RoleRoot310(scope,root)||!Invert(root,inverseRoot)||!Invert(ArmBodyToCamera(),inverseView))return;
 NativeSupport310 next;next.entity=held.entity;next.epoch=g_armPoseEpoch;next.profile315=g_weaponMechanicProfile;next.confirmed=confirmed;
 //LeftMove for a supported1911 is the current native gun move. Compute that
 //same support pose without changing any live support/ownership state.
 const Mat4 toLocal=Multiply(inverseRoot,Multiply(inverseView,Multiply(g_frameGunMove,toView)));
 for(int bone=2;bone<=19;++bone){next.local[bone-2]=Multiply(toLocal,BoneMatrix(raw,bone));for(double v:next.local[bone-2].m)if(!std::isfinite(v))return;}
 //The trigger centre is a reach test, not the displayed palm position.
 //Never translate this skin to the trigger centre; retain its actual gun fit.
 next.valid=true;cache=next;
 Log("[SUPPORT310] cached native1911 support entity=%08X epoch=%u confirmedRightSupport=%d",held.entity,g_armPoseEpoch,confirmed);
}
bool Right1911Support310(const PhysicalArmScope303& scope,int side,Mat4& desired){
 const auto held=fc3hands303::ReadPresentation314();const auto& cache=g_nativeSupport310;
 if(!scope.active||side!=0||held.hand!=1||!g_twoHand||!g_physicalValid303[0]||!g_physicalValid303[1]||!SupportEligible310(true)||!cache.valid||cache.entity!=held.entity||cache.epoch!=g_armPoseEpoch||cache.profile315!=g_weaponMechanicProfile)return false;
 Mat4 root={};if(!RoleRoot310(scope,root))return false;
 Mat4 mirror=HandIdentity();mirror.m[0]=-1;
 desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(mirror,Multiply(cache.local[0],mirror))));
 for(double v:desired.m)if(!std::isfinite(v))return false;
 return true;
}
void Right1911SupportFingers310(float* bones){
 const auto& cache=g_nativeSupport310;Mat4 inverse={},mirror=HandIdentity();mirror.m[0]=-1;
 if(!Invert(cache.local[0],inverse))return;
 const Mat4 wrist=BoneMatrix(bones,2);
 for(int bone=3;bone<=19;++bone)StoreBone(bones,bone,Multiply(wrist,Multiply(mirror,Multiply(Multiply(inverse,cache.local[bone-2]),mirror))));
}

//Read the completed support skin, after wrist placement, IK and final fingers.
//This is exactly the right-dominant reference the player sees, not a prediction.
void RecordDisplayedSupport311(const PhysicalArmScope303& scope,int side){
 const auto held=fc3hands303::Read();
 if(scope.active||side!=1||held.hand!=0||!g_twoHand||!SupportEligible310()||g_armFinalFrame[1]!=g_frame||fc3pad::Latest().rightTrigger>.3f)return;
 Mat4 root={},inverse={};if(!RoleRoot310(scope,root)||!Invert(root,inverse))return;
 NativeSupport310 next;next.entity=held.entity;next.epoch=g_armPoseEpoch;next.profile315=g_weaponMechanicProfile;next.confirmed=true;
 for(int bone=2;bone<=19;++bone){next.local[bone-2]=Multiply(inverse,g_armFinalBody[1][bone]);for(double v:next.local[bone-2].m)if(!std::isfinite(v))return;}
 next.valid=true;const bool announce=!g_nativeSupport310.confirmed||g_nativeSupport310.entity!=next.entity||g_nativeSupport310.epoch!=next.epoch||g_nativeSupport310.profile315!=next.profile315;g_nativeSupport310=next;
 if(announce)Log("[SUPPORT311] final displayed right-dominant support captured entity=%08X epoch=%u; no trigger-centre translation",held.entity,g_armPoseEpoch);
}
bool LeftSupportContact311(V3& point,V3& palm){
 const auto held=fc3hands303::Read();const auto& cache=g_nativeSupport310;
 if(held.hand!=1||!SupportEligible310()||!cache.valid||cache.entity!=held.entity||cache.epoch!=g_armPoseEpoch||cache.profile315!=g_weaponMechanicProfile)return false;
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip)||!FreeHandPalm(1,palm))return false;
 V3 local=Apply(cache.local[0],{-.6781,.0519,1.0387});local.x=-local.x;point=Apply(root,local);return true;
}
