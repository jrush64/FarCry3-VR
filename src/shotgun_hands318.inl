void AkMirrorFingers316(float*,const float*);
// Fit the opposite anatomical hand to an authored wrist on the weapon. Two
// reflections keep the wrist proper. Unmirrored meshes retain the authored
// contact point; full mirrored meshes use the weapon centre plane instead.
Mat4 ReloadMirrorHand341(const Mat4& root,const Mat4& hand,int source,bool contact,bool mirroredMesh){
 const Mat4 bind=SidearmMirror315();Mat4 plane=bind;
 if(contact&&!mirroredMesh)plane.m[3]=2*Apply(hand,{source?-.6781:.6781,.0519,1.0387}).x;
 return Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply(hand,bind))));
}
struct ShotgunSkin318 {Mat4 left[18]={};float rightFingers[204]={};unsigned entity=0,epoch=~0u;int profile=-1;bool support=false,right=false;Mat4 nativeRight={};LONG nativeFrame=-100000;};
ShotgunSkin318 g_shotgunSkin318;
struct ShotgunLogicalScope318 {
 float body[2][16],poses[2][16];bool valid[2];bool active;
 ShotgunLogicalScope318(const PhysicalArmScope303& s):active(s.active){if(!active)return;memcpy(body,g_handBody,sizeof(body));memcpy(poses[0],g_handRight,64);memcpy(poses[1],g_handLeft,64);valid[0]=g_handRightValid;valid[1]=g_handLeftValid;memcpy(g_handBody,s.oldBody,sizeof(body));memcpy(g_handRight,s.oldPose[0],64);memcpy(g_handLeft,s.oldPose[1],64);g_handRightValid=s.oldValid[0];g_handLeftValid=s.oldValid[1];}
 ~ShotgunLogicalScope318(){if(!active)return;memcpy(g_handBody,body,sizeof(body));memcpy(g_handRight,poses[0],64);memcpy(g_handLeft,poses[1],64);g_handRightValid=valid[0];g_handLeftValid=valid[1];}
};
bool ShotgunGrip318(Mat4& grip){
 if(M133SupportProfile56()){grip=kShotgunHandFromGun;return true;}
 if(SpasModel()){grip=SpasKeyMat(kSpasRightHandFromFrame);return true;}
 if(Lever1887Model()){grip=kL1887RightHandFromFrame;return true;}return false;
}
bool ShotgunRoot318(Mat4& root){
 Mat4 grip={};if(!ShotgunGrip318(grip))return false;
 if(Lever1887Model()&&Lever1887ParentReady()){
  root=Lever1887GunBody();if(Lever1887Holding()&&!Lever1887LeftBody(root))root=g_lever.lastHoldBody;
  for(double v:root.m)if(!std::isfinite(v))return false;return true;
 }
 Mat4 ignored={};if(ControllerCurrentRoot124(root,ignored))return true;return ControllerGripRoot124(grip,nullptr,root);
}
bool ShotgunBusy318(){
 if(M133SupportProfile56())return g_reload.held||g_manualReload.pending||g_manualReload.cycle||g_reloadAlignment||g_pumpStage==1;
 if(SpasModel())return g_spas.carrying||g_spas.pending||(g_spas.grab&&g_spas.needPump)||g_spas.pump>0||SpasClipPlaying()||SpasNativeLeft();
 if(Lever1887Model())return Lever1887Holding()||g_lever.grabbing||g_lever.shell||g_lever.pending||g_lever.clip>=0||g_lever.leverOpen||g_lever.frac>0;
 return true;
}
void ShotgunRole318(float* pose){
 if(!pose||!LeftShotgun318(true)||g_freeRightEpoch!=g_armPoseEpoch||(Lever1887Model()&&Lever1887Holding()))return;
 Mat4 grip={},inverse={};if(!ShotgunGrip318(grip)||!Invert(grip,inverse))return;
 const Mat4 root=Multiply(g_freeRightFromController,inverse);const double dx=2*Apply(grip,{.6781,.0519,1.0387}).x;
 const double delta[3]={root.m[0]*dx,root.m[4]*dx,root.m[8]*dx};for(int r=0;r<3;++r)for(int c=0;c<3;++c)pose[r*4+3]+=float(pose[r*4+c]*delta[c]);
}
void CaptureShotgun318(const PhysicalArmScope303& scope,int side,const Mat4& toView,const float* bones){
 if(!M133SupportProfile56()||g_reload.held||g_manualReload.pending||g_manualReload.cycle||ReloadPoseActive()||!g_chambered||fc3pad::Latest().rightTrigger>.3f)return;
 const auto h=fc3hands303::Read();if(!h.enabled||!h.active||!h.entity)return;auto& cache=g_shotgunSkin318;
 if(cache.entity!=h.entity||cache.epoch!=g_armPoseEpoch||cache.profile!=g_weaponMechanicProfile){cache={};cache.entity=h.entity;cache.epoch=g_armPoseEpoch;cache.profile=g_weaponMechanicProfile;}
 if(side==0){cache.nativeRight=Multiply(toView,BoneMatrix(bones,2));cache.nativeFrame=g_frame;}
 if(side==0&&!cache.right){Mat4 inv={};if(Invert(BoneMatrix(bones,2),inv)){for(int b=3;b<=19;++b)StoreBone(cache.rightFingers,b-3,Multiply(inv,BoneMatrix(bones,b)));cache.right=true;}}
 if(side==1&&!cache.support&&g_frame-cache.nativeFrame<=1){Mat4 gripInverse={},nativeInverse={};if(!Invert(kShotgunHandFromGun,gripInverse)||!Invert(Multiply(cache.nativeRight,gripInverse),nativeInverse))return;const Mat4 local=Multiply(nativeInverse,toView);for(int b=2;b<=19;++b)cache.left[b-2]=Multiply(local,BoneMatrix(bones,b));cache.support=true;}
}
bool M133SupportTarget342(V3& point){
 if(!M133SupportProfile56())return false;const auto h=fc3hands303::Read();const auto& cache=g_shotgunSkin318;Mat4 root={};
 LoadM133Support56();if(g_m133Support56.positioned){Mat4 box={};if(M133SupportBox56(box)){point={box.m[3],box.m[7],box.m[11]};return true;}}
 if(cache.support&&cache.entity==h.entity&&cache.epoch==g_armPoseEpoch&&cache.profile==g_weaponMechanicProfile&&ShotgunRoot318(root)){
  auto local=Apply(cache.left[0],{-.6781,.0519,1.0387});if(LeftShotgunOwner318())local.x=-local.x;point=Apply(root,local);return true;
 }
 Mat4 box={};if(!M133SupportBox56(box))return false;point={box.m[3],box.m[7],box.m[11]};return true;
}
bool ShotgunArm318(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(!scope.active||!LeftShotgun318(true))return false;ShotgunLogicalScope318 logical(scope);Mat4 root={},grip={};if(!ShotgunRoot318(root)||!ShotgunGrip318(grip))return false;
 if(side==1){
  if(Lever1887Model()&&Lever1887Holding()&&g_lever.clip<0)return false; // free physical-left lever/shell hand
  if(Lever1887Model()&&Lever1887SimplePoseActive()){
   const auto& pose=Lever1887SimplePose();const bool shell=g_lever.clip==kL1887ShellFirst||g_lever.clip==kL1887ShellLoop;
   const Mat4& part=shell?pose.shell:pose.lever;Mat4 inverse={};if(!Invert(part,inverse))return false;
   // Mirror around the operating part's own plane. The mirrored native
   // hand-to-part attachment then reconstructs that exact part transform.
   const auto bind=SidearmMirror315();const auto plane=Multiply(part,Multiply(bind,inverse));
   desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply(pose.hand,bind))));return true;
  }
  desired=ReloadMirrorHand341(root,grip,0,false,false);return true;
 }
 Mat4 hand={};
 if(SpasModel()){
  // SPAS seating reflects around the shell's contact plane, not its wrist.
  if(SpasClipPlaying()){const auto& clip=SpasClipPose();hand=clip.hand;const auto bind=SidearmMirror315();Mat4 plane=bind;plane.m[3]=2*clip.shell.m[3];desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply(hand,bind))));return true;}
  else if(SpasForendHeld()||g_twoHand)hand=SpasForendHand();
  else return false;
 }else if(Lever1887Model()){
  if(!Lever1887Holding()&&!g_twoHand)return false;
  hand=kL1887LeftSupportFromFrame;
 }else{
  const auto& c=g_shotgunSkin318;if(!g_twoHand||g_reload.held||!c.support||c.profile!=g_weaponMechanicProfile||c.entity!=fc3hands303::ReadPresentation314().entity||c.epoch!=g_armPoseEpoch)return false;
  hand=c.left[0];
 }
 //1887's forend grasp crosses to the opposite face of the wood. Its held
 // gun parent is anchored to that same authored mirrored wrist.
 desired=ReloadMirrorHand341(root,hand,1,false,false);
 if(M133SupportProfile56()){const double d=(std::max)(-g_pumpRange,(std::min)(.02,g_slide-g_slideGrab));desired.m[3]+=g_slideAxis[0]*d;desired.m[7]+=g_slideAxis[1]*d;desired.m[11]+=g_slideAxis[2]*d;}
 return true;
}
bool ShotgunFingers318(int side,float* bones){
 if(!LeftShotgun318())return false;const float* fingers=nullptr;float relative[204];
 if(side==1){
  if(SpasModel())fingers=kSpasRightIdleFingers;
  else if(Lever1887Model())fingers=Lever1887SimplePoseActive()?Lever1887SimplePose().fingers:Lever1887Holding()?(g_lever.shell?kL1887ShellGripRelative:g_lever.grabbing?kL1887RightLeverGripRelative:nullptr):kL1887RightQuietGripRelative;
  else if(g_shotgunSkin318.right)fingers=g_shotgunSkin318.rightFingers;
 }else{
  if(SpasModel())fingers=SpasClipPlaying()?SpasClipPose().fingers:g_spas.carrying?kSpasClipShellInsert[0].leftFingers:kSpasLeftIdleFingers;
  else if(Lever1887Model()){if(Lever1887Holding()||g_twoHand)fingers=kL1887LeftQuietGripRelative;}
  else if(g_reload.held)fingers=kCapturedGripRelative;
  else if(g_twoHand&&g_shotgunSkin318.support){Mat4 inv={};if(Invert(g_shotgunSkin318.left[0],inv)){for(int b=3;b<=19;++b)StoreBone(relative,b-3,Multiply(inv,g_shotgunSkin318.left[b-2]));fingers=relative;}}
 }
 if(!fingers)return false;AkMirrorFingers316(bones,fingers);return true;
}
void RecordShotgunHands318(ID3D11DeviceContext* context,int side,const float* bones){
 if(!LeftShotgun318())return;Mat4 view={},qi={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),qi))return;
 if(side==0){
  if(SpasModel()){g_spas.leftWrist=Multiply(Multiply(qi,view),SidearmMirror315());g_spas.leftFrame=g_frame;}
  if(M133SupportProfile56()){
   if(!g_reload.held||!g_reload.mode){g_reloadContact.hand=false;return;}
   g_reloadContact.view=Multiply(view,SidearmMirror315());const Mat4 shell=Multiply(Multiply(g_reloadContact.view,kCapturedShellToHand),SidearmMirror315());
   g_reloadContact.shellBody=Apply(Multiply(qi,shell),kCapturedShellCentre);g_reloadContact.handFrame=g_frame;g_reloadContact.hand=true;g_reloadContact.mirrored318=true;
  }
 }
 if(Lever1887Model()){Mat4 actual={};const int role=1-side;if(!ShotgunPhysicalWrist318(role,actual))return;const Mat4 canonical=ShotgunCanonicalWrist318(role,actual);if(role==0){g_lever.rightHandBody=canonical;g_lever.rightHandFrame=g_frame;}else{g_lever.leftHandBody=canonical;g_lever.leftHandFrame=g_frame;}}
}
void PublishShotgunTransfer318(){
 const auto h=fc3hands303::Read();const bool enabled=fc3hands303::Shotgun318(g_weaponMechanicProfile)&&h.enabled&&h.engaged&&!h.empty&&h.nativeHoldingSide==0;
 bool allowed=false,contact=false;Mat4 root={},grip={};
 if(enabled&&h.active&&g_frame-g_weaponMechanicFrame<=2&&h.hand>=0&&h.hand<2){
  allowed=g_physicalValid303[0]&&g_physicalValid303[1]&&!ShotgunBusy318()&&!ReloadPoseActive()&&!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&ShotgunRoot318(root)&&ShotgunGrip318(grip);
  const int other=1-h.hand;const auto& a=g_freeHandAttachment[other];if(allowed&&a.profile==g_weaponMechanicProfile&&a.epoch==g_armPoseEpoch&&g_frame-a.frame<=2){
   Mat4 controller={},inverse={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[other][i];const Mat4 wrist=Multiply(controller,a.relative);
   const double x=other?-.6781:.6781;const V3 sphere=Apply(wrist,{x,.0519,1.0387});
   if(Invert(root,inverse)){const V3 local=Apply(inverse,sphere),rear=Apply(grip,{.6781,.0519,1.0387});double radius=.05*(std::max)(.1f,g_worldScale),frontDistance=1e9;
    if(SpasModel()){const V3 front=Apply(SpasKeyMat(kSpasLeftSupportFromFrame),{-.6781,.0519,1.0387});frontDistance=Length(Sub(local,front));}
    if(Lever1887Model())frontDistance=Length(Sub(local,Apply(kL1887LeftSupportFromFrame,{-.6781,.0519,1.0387})));
    if(M133SupportProfile56()){Mat4 box={};if(M133SupportBox56(box))frontDistance=Length(Sub(sphere,V3{box.m[3],box.m[7],box.m[11]}));}
    const double d=Length(Sub(local,rear));contact=d<=radius+.035*(std::max)(.1f,g_worldScale)&&frontDistance>d;
    if(SpasModel())for(int b=0;b<2;++b)if(SpasBoxDistance(b,local)<=g_spas.handDiameter*.5&&Length(Sub(local,SpasBoxCentre(b)))<=d)contact=false;
    if(Lever1887Model()){
     const V3 loop={g_lever.lever[0],g_lever.lever[1],g_lever.lever[2]},gate={g_lever.gate[0],g_lever.gate[1],g_lever.gate[2]};
     if((Lever1887GuideContact52(0,local)&&Length(Sub(local,loop))<=d)||(Lever1887GuideContact52(1,local)&&Length(Sub(local,gate))<=d))contact=false;
    }
    if(M133SupportProfile56()&&g_reloadContact.port&&Length(Sub(sphere,g_reloadContact.portBody))<=d)contact=false;
   }
  }
 }
 fc3hands303::PublishTransfer307(enabled,contact,allowed,h.generation,h.entity,g_armPoseEpoch,true);
}

void RecordShotgunSupport318(const PhysicalArmScope303& scope,int side){
 if(scope.active||side!=1||!M133SupportProfile56()||!g_twoHand||!g_chambered||g_reload.held||g_manualReload.pending||g_manualReload.cycle||g_armFinalFrame[1]!=g_frame||fc3pad::Latest().rightTrigger>.3f)return;
 const auto h=fc3hands303::Read();if(!h.enabled||!h.active||h.empty)return;Mat4 root={},inverse={};if(!ShotgunRoot318(root)||!Invert(root,inverse))return;auto& c=g_shotgunSkin318;if(c.entity!=h.entity||c.epoch!=g_armPoseEpoch||c.profile!=g_weaponMechanicProfile)return;
 for(int b=2;b<=19;++b)c.left[b-2]=Multiply(inverse,g_armFinalBody[1][b]);c.support=true;
}
