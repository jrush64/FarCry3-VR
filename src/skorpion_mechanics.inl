// Skorpion selectors are taken from the verified VZ61 asset, not pistol offsets.
#include "skorpion_charge_wrist52.inl"
#include "skorpion_pinch229.inl"
bool SkorpionChargeHandMove52(const Mat4& toView,const float* bones,Mat4& move){
 const auto& p=g_pistolReload;
 if(!SkorpionModel()||!PistolHandling()||!p.rackHeld||p.held||p.removalHeld||p.pending||p.cycle||!g_handLeftValid||!g_handRightValid||g_frame-p.rootFrame>1||p.rootEpoch!=g_armPoseEpoch)return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 Mat4 hand=kSkorpionChargingWristFromFrame52;
 // Seat the native thumb/index pinch on the actual knob. Contact-box sliders
 // must not drag the rendered grip away from the mechanism.
 const V3 pinch=Apply(hand,kSkorpionPinch229),knob={.026,.14320966,.04219692};
 hand.m[3]+=knob.x-pinch.x;hand.m[7]+=knob.y-pinch.y;hand.m[11]+=knob.z-pinch.z;
 hand.m[7]-=(std::max)(0.0,(std::min)(.05,p.slidePull));
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(p.rootBody,hand)),inverse);return true;
}
bool SkorpionFrame(UINT start,UINT count){return (start==2016&&count==1056)||(start==3072&&count==10347);}
void SkorpionWorld(ID3D11DeviceContext*,UINT start,UINT count,const Mat4& nativeToView,Mat4& handMove,float* globals)
{
 auto& p=g_pistolReload;
 if(SkorpionFrame(start,count)){
  Mat4 basis={};if(HandBasis(Multiply(nativeToView,kSkorpionHandFromFrame),basis)){p.current=basis;p.basisFrame=g_frame;if(!PistolPoseOwned()&&fc3pad::Latest().rightTrigger<.3f){p.idle=basis;p.basis=true;}if(PistolOwnsLeft())g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);RightHandMove(handMove);}
 }
 if(start==0&&count==1548){
  if((p.held||PistolAmmoPreviewActive())&&g_frame-p.handFrame<=1){Mat4 inverse={};if(Invert(nativeToView,inverse))handMove=Multiply(p.hand,inverse);}
  else if(!p.magazinePresent||p.pending||p.cycle)globals[7]=-10000;
 }
}
void SkorpionFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=157984||d.indexBytes!=35322||d.stride!=32||d.offset!=0||d.format!=57)return;
 auto& p=g_pistolReload;CameraSummary camera={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(1,1,&cb);UINT bytes=sizeof(camera);
 const bool cameraOk=cb&&SUCCEEDED(cb->GetPrivateData(kCameraSummaryGuid,&bytes,&camera))&&bytes==sizeof(camera);SafeRelease(cb);
 Mat4 viewInverse={},bodyInverse={};if(!cameraOk||!Invert(HullView(camera),viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return;
 if(SkorpionFrame(start,count)){
  p.rootNative=nativeWorld;p.rootBody=PistolParentBody(Multiply(bodyInverse,Multiply(HullView(camera),HullWorld(globals))));p.rootFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootContext=context;
  p.rootWorld=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.rootBody));
  p.port=Apply(p.rootBody,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]});p.portFrame=g_frame;
  WritePistolWorld(globals,p.rootWorld);return;
 }
 // VZ61 draws CLIP before FRAME. Resolve a learned controller attachment with
 // this draw's camera, rather than using the previous camera/world position.
 if(!p.parentValid||p.parentEpoch!=g_armPoseEpoch)return;
 const Mat4 body=PistolParentBody(p.rootBody),root=Multiply(viewInverse,Multiply(ArmBodyToCamera(),body));
 if(start==0&&count==1548){
  if(p.removalHeld){Mat4 clipBody=Multiply(p.palmBody,MagazineHeldFromHand());if(A2000LeftCharge336())clipBody=Multiply(clipBody,SidearmMirror315());const Mat4 world=Multiply(viewInverse,Multiply(ArmBodyToCamera(),clipBody));WritePistolWorld(globals,world);p.clipWorld=world;p.clipFrame=g_frame;return;}
  if(p.held||PistolAmmoPreviewActive())return;
  if(p.looseFlying){WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.looseBody)));return;}
  if(p.insertActive&&(p.pending||p.cycle||p.magazinePresent)){const Mat4 clip=Multiply(root,PistolInsertRelative(static_cast<double>(GetTickCount64()-p.insertAt)/1000));WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;}
  if(p.dropActive){const double age=static_cast<double>(GetTickCount64()-p.dropAt)/1000;if(age<.65){WritePistolWorld(globals,PistolDropWorld(p.dropWorld,age));return;}p.dropActive=false;}
  if(!p.magazinePresent||p.pending||p.cycle){globals[7]=-10000;return;}
  const Mat4 clip=Multiply(root,kSkorpionClipRest);WritePistolWorld(globals,clip);p.clipWorld=clip;p.clipFrame=g_frame;return;
 }
 if(start==1548&&count==468){
  // Retain native bolt motion when its matching FRAME has been observed.
  // Partial reload and ordinary cycling retain native bolt motion.
  // Confirmed in headset testing: an empty reload requires a manual side-handle charge below.
  Mat4 relative=kSkorpionSlideRest,inverse={};
  if(p.rootFrame==g_frame&&Invert(p.rootNative,inverse))relative=Multiply(inverse,nativeWorld);
  else if(p.nativeGunFrame==g_frame){const Mat4 nativeRoot=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.nativeGunBody));if(Invert(nativeRoot,inverse))relative=Multiply(inverse,nativeWorld);}
  p.nativeSlidePull=(std::max)(0.0,(std::min)(.05,kSkorpionSlideRest.m[7]-relative.m[7]));p.nativeSlideFrame=g_frame;
  if((p.cycle&&p.reloadWasEmpty)||p.needsRack||p.rackHeld||p.rackReturning){relative=kSkorpionSlideRest;if(p.rackHeld||p.rackReturning)relative.m[7]-=p.slidePull;}
  WritePistolWorld(globals,Multiply(root,relative));return;
 }
 if(start==13419&&count==4242){Mat4 relative=kSkorpionStockRest,inverse={};if(p.rootFrame==g_frame&&Invert(p.rootNative,inverse))relative=Multiply(inverse,nativeWorld);else if(p.nativeGunFrame==g_frame){const Mat4 nativeRoot=Multiply(viewInverse,Multiply(ArmBodyToCamera(),p.nativeGunBody));if(Invert(nativeRoot,inverse))relative=Multiply(inverse,nativeWorld);}WritePistolWorld(globals,Multiply(root,relative));}
}
