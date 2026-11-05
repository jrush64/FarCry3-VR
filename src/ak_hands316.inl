void AkRole316(float* pose){
 if(!pose||!LeftAk316(true)||g_freeRightEpoch!=g_armPoseEpoch)return;
 Mat4 inverse={};if(!Invert((*CurrentRifle().hand),inverse))return;
 const Mat4 localRoot=Multiply(g_freeRightFromController,inverse);
 const double dx=2*Apply((*CurrentRifle().hand),{.6781,.0519,1.0387}).x;
 const double delta[3]={localRoot.m[0]*dx,localRoot.m[4]*dx,localRoot.m[8]*dx};
 for(int r=0;r<3;++r)for(int c=0;c<3;++c)pose[r*4+3]+=float(pose[r*4+c]*delta[c]);
}
bool LeftAkHolding316(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(side!=1||!scope.active||!LeftAk316(true))return false;
 Mat4 root={};if(!RoleRoot310(scope,root))return false;const Mat4 mirror=SidearmMirror315();
 desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(mirror,Multiply((*CurrentRifle().hand),mirror))));return true;
}
void AkMirrorFingers316(float* bones,const float* source){
 const Mat4 wrist=BoneMatrix(bones,2),mirror=SidearmMirror315();
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(source,b-3),mirror))));
}
bool RightAkSupport316(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(side!=0||!scope.active||!LeftAk316(true)||!g_twoHand||PistolBusy()||g_pistolReload.held||g_pistolReload.removalHeld)return false;
 Mat4 root={};if(!RoleRoot310(scope,root))return false;
 // Keep the support at the real foregrip. Reflect the anatomical wrist at
 // its native contact plane, not at the rear grip or an editable trigger.
 // F1/P416 keep their centre-line charging mechanisms unmirrored, but the
 // opposite anatomical support hand still grasps the other foregrip face.
 const bool centre343=A2000LeftCharge336()||g_weaponMechanicProfile==14||g_weaponMechanicProfile==31;
 Mat4 plane=SidearmMirror315();plane.m[3]=(centre343?0:2)*Apply((*CurrentRifle().support),{-.6781,.0519,1.0387}).x;
 desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply((*CurrentRifle().support),SidearmMirror315()))));return true;
}
// Scoped magazine families use rear-grip contact rather than palm proximity.
// Foregrip = support; a fresh squeeze at the rear grip = transfer entry.
void PublishAkTransfer316(){
 const auto h=fc3hands303::Read();const auto& p=g_pistolReload;
 const bool enabled=fc3hands303::MagazineRifle317(g_weaponMechanicProfile)&&h.enabled&&h.engaged&&!h.empty&&h.nativeHoldingSide==0;
 bool allowed=false,contact=false,reloadContact332=false;Mat4 root={},grip={};
 if(enabled&&h.active&&g_frame-g_weaponMechanicFrame<=2&&h.hand>=0&&h.hand<2){
  allowed=g_physicalValid303[0]&&g_physicalValid303[1]&&!PistolBusy()&&!p.held&&!p.removalHeld&&!p.rackHeld&&!p.rackReturning&&!p.pending&&!p.cycle&&(!p.insertActive||AkInsertionSeated())&&!g_reload.held&&!g_reload.pending&&!g_reloadAlignment&&!ReloadPoseActive()&&!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&ControllerCurrentRoot124(root,grip);
  const int other=1-h.hand;const auto& a=g_freeHandAttachment[other];
  if(allowed&&a.epoch==g_armPoseEpoch&&a.profile==g_weaponMechanicProfile&&g_frame-a.frame<=2){
   Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[other][i];
   const V3 sphere=TransferSphere307(Multiply(controller,a.relative),other);Mat4 inverse={};
   if(Invert(root,inverse)){
    const V3 point=Apply(inverse,sphere),rear=Apply(grip,{.6781,.0519,1.0387}),front=Apply((*CurrentRifle().support),{-.6781,.0519,1.0387});
    const V3 port={g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]},charge=PistolRackGuide();
    const double distance=Length(Sub(point,rear)),radius=.5*g_pistolGuides.handDiameter*(std::max)(.1f,g_worldScale);
    contact=std::isfinite(radius)&&radius>0&&distance<=radius+.035*(std::max)(.1f,g_worldScale)&&Length(Sub(point,front))>distance;
    if((PistolGuideContact(root,0,sphere)&&Length(Sub(point,port))<=distance)||(PistolGuideContact(root,3,sphere)&&Length(Sub(point,charge))<=distance))contact=false;
    const double holdDistance=(std::min)(distance,Length(Sub(point,front)));
    reloadContact332=(PistolGuideContact(root,0,sphere)&&Length(Sub(point,port))<=holdDistance)||(PistolGuideContact(root,3,sphere)&&Length(Sub(point,charge))<=holdDistance);
   }
  }
 }
 if(fc3hands303::Smg332(g_weaponMechanicProfile)){PublishSmgTransfer332(h,enabled,contact,allowed,true,reloadContact332);return;}
 fc3hands303::PublishTransfer307(enabled,contact,allowed,h.generation,h.entity,g_armPoseEpoch,true);
}
