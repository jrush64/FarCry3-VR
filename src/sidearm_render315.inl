struct SidearmGrip315 {float fingers[204]={};unsigned entity=0,epoch=~0u;bool valid=false;};
SidearmGrip315 g_sidearmGrip315[64];
bool SidearmGripFrame315(Mat4& grip){
 if(fc3hands303::MagazineSidearm315(g_weaponMechanicProfile)){grip=MagazineHandFromFrame();return true;}
 if(RevolverModel()){grip=kRevolverRightHandFromFrame;return true;}
 if(FlareModel()){grip=FlareKeyMat(kFlareRightHandFromFrame);return true;}return false;
}
void SidearmRole315(float* pose){
 if(!pose||g_weaponMechanicProfile==20||!LeftSidearm315(true)||g_freeRightEpoch!=g_armPoseEpoch)return;
 Mat4 grip={},inverse={};if(!SidearmGripFrame315(grip)||!Invert(grip,inverse))return;
 const Mat4 root=Multiply(g_freeRightFromController,inverse);const double dx=2*Apply(grip,{.6781,.0519,1.0387}).x;
 const double delta[3]={root.m[0]*dx,root.m[4]*dx,root.m[8]*dx};
 for(int row=0;row<3;++row)for(int col=0;col<3;++col)pose[row*4+3]+=float(pose[row*4+col]*delta[col]);
}
void CaptureSidearmGrip315(int side,const float* raw){
 if(side!=0||!fc3hands303::Sidearm315(g_weaponMechanicProfile)||g_weaponMechanicProfile==20||ReloadPoseActive()||fc3pad::Latest().rightTrigger>.3f)return;
 const auto s=fc3hands303::Read();if(!s.active||!s.entity)return;
 auto& c=g_sidearmGrip315[g_weaponMechanicProfile];if(c.valid&&c.entity==s.entity&&c.epoch==g_armPoseEpoch)return;
 Mat4 inverse={};if(!Invert(BoneMatrix(raw,2),inverse))return;
 for(int b=3;b<=19;++b)StoreBone(c.fingers,b-3,Multiply(inverse,BoneMatrix(raw,b)));
 c.entity=s.entity;c.epoch=g_armPoseEpoch;c.valid=true;
}
bool LeftSidearmHolding315(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(side!=1||!scope.active||g_weaponMechanicProfile==20||!LeftSidearm315(true))return false;
 Mat4 root={},grip={};if(!RoleRoot310(scope,root)||!SidearmGripFrame315(grip))return false;
 const Mat4 mirror=SidearmMirror315();desired=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(mirror,Multiply(grip,mirror))));return true;
}
void LeftSidearmFingers315(float* bones){
 const float* pose=P6p9Model()?kPistolRightGripRelative:D50Model()?kD50RightGripRelative:SkorpionModel()?kSkorpionRightGripRelative:nullptr;
 const auto& c=g_sidearmGrip315[g_weaponMechanicProfile];const auto s=fc3hands303::ReadPresentation314();
 if(!pose&&c.valid&&c.entity==s.entity&&c.epoch==g_armPoseEpoch)pose=c.fingers;
 if(!pose){CurlFingers(bones,1);return;}
 const Mat4 wrist=BoneMatrix(bones,2),mirror=SidearmMirror315();
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(pose,b-3),mirror))));
}
bool RightSidearmReloadMove315(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(side!=0||!scope.active||!LeftSidearm315())return false;
 if(RevolverClipPlaying()&&RevolverFrameReady()){
  //325 mirrors the entire .44 reload arrangement about FRAME X. Its clip
  // hand is already the proper right-hand pose beside the right-side cylinder.
  const auto& clip=RevolverClipPose();
  desired=Multiply(ArmBodyToCamera(),Multiply(g_revolver.frameBody,clip.hand));return true;
 }return false;
}
bool RightSidearmReloadFingers315(int side,float* bones){
 if(side!=0||!LeftSidearm315())return false;
 const float* pose=nullptr;
 if(RevolverManual()&&(g_revolver.carrying||RevolverClipPlaying()))pose=RevolverClipPlaying()?RevolverClipPose().fingers:kRevolverLeftSeatFingers;
 if(FlareActive()&&g_flare.carrying)pose=g_flare.carryingSpent?kFlareLeftSpentShellFingers:kFlareLeftShellFingers;
 if(!pose)return false;const Mat4 wrist=BoneMatrix(bones,2),mirror=SidearmMirror315();
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,Multiply(mirror,Multiply(BoneMatrix(pose,b-3),mirror))));return true;
}
void RecordRightSidearmReload315(ID3D11DeviceContext* context,int side,const float* bones){
 if(side!=0||!LeftSidearm315()||(!RevolverModel()&&!FlareModel()))return;
 Mat4 wrist={},inverse={};if(!HandDrawView(context,bones,2,wrist)||!Invert(ArmBodyToCamera(),inverse))return;
 const Mat4 semantic=Multiply(Multiply(inverse,wrist),SidearmMirror315());
 if(RevolverModel()){g_revolver.leftWrist=semantic;g_revolver.leftFrame=g_frame;}
 if(FlareModel()){g_flare.leftWrist=semantic;g_flare.leftFrame=g_frame;}
}
