struct OtherSkin319 {float fingers[2][204]={};Mat4 nativeView[2][18]={};LONG frame[2]={-100000,-100000};bool valid[2]={};unsigned entity=0,epoch=~0u;int profile=-1;} g_otherSkin319;
bool OtherGrip319(Mat4& grip,Mat4& support){
 if(fc3hands303::OtherMagazine319(g_weaponMechanicProfile)){grip=*CurrentRifle().hand;support=*CurrentRifle().support;return true;}
 if(RpgModel()){grip=RpgKeyMat(kRpgRightHandFromFrame);support=RpgKeyMat(kRpgLeftSupportFromFrame);return true;}
 if(Gm94Model()){grip=Gm94KeyMat(kGm94RightHandFromFrame);support=Gm94Support();return true;}
 if(FlamerModel()){grip=FlamerKeyMat(kFlamerRightHandFromFrame);support=FlamerKeyMat(kFlamerLeftSupportFromFrame);return true;}
 if(RepairModel247()){grip=kRepairRightHand247;support=kRepairKnobHand250;return true;}return false;
}
bool OtherRoot319(Mat4& root){if(R700Hold()){root=R700HoldBody();return true;}Mat4 grip={};return ControllerCurrentRoot124(root,grip);}
bool OtherBusy319(){
 if(!fc3hands303::Other319(g_weaponMechanicProfile))return false;const auto& p=g_pistolReload;
 if((fc3hands303::OtherMagazine319(g_weaponMechanicProfile)&&(PistolBusy()||p.held||p.removalHeld||p.pending||p.cycle||p.rackHeld||p.rackReturning||(p.insertActive&&!AkInsertionSeated())))||ReloadPoseActive()||g_reload.held||g_reload.pending||g_reloadAlignment)return true;
 if(R700Model())return R700Hold()||R700ClipPlaying()||g_r700.native||g_r700.grabbing;
 if(PkmModel())return PkmNativeActive();
 if(RpgModel())return g_rpg.carrying||g_rpg.pending||RpgClipPlaying();
 if(Gm94Model())return Gm94BusyLeft()||g_gm94.pumping||g_gm94.offset>.001;
 if(FlamerModel())return FlamerBusyLeft();return false;
}
void OtherRole319(float* pose){
 if(!pose||!LeftOther319(true)||g_freeRightEpoch!=g_armPoseEpoch||(R700Model()&&R700Hold()))return;
 Mat4 grip={},support={},inverse={};if(!OtherGrip319(grip,support)||!Invert(grip,inverse))return;
 const auto root=Multiply(g_freeRightFromController,inverse);const double dx=2*Apply(grip,{.6781,.0519,1.0387}).x;
 const double d[3]={root.m[0]*dx,root.m[4]*dx,root.m[8]*dx};for(int r=0;r<3;++r)for(int c=0;c<3;++c)pose[r*4+3]+=float(pose[r*4+c]*d[c]);
}
void CaptureOther319(int side,const Mat4& toView,const float* raw){
 if(!fc3hands303::Other319(g_weaponMechanicProfile)||side<0||side>1)return;const auto h=fc3hands303::Read();if(!h.enabled||!h.active||h.empty)return;
 auto& c=g_otherSkin319;if(c.entity!=h.entity||c.epoch!=g_armPoseEpoch||c.profile!=g_weaponMechanicProfile){c={};c.entity=h.entity;c.epoch=g_armPoseEpoch;c.profile=g_weaponMechanicProfile;}
 for(int b=2;b<=19;++b)c.nativeView[side][b-2]=Multiply(toView,BoneMatrix(raw,b));c.frame[side]=g_frame;
 if(!c.valid[side]&&!OtherBusy319()&&fc3pad::Latest().rightTrigger<.3f){Mat4 inv={};if(Invert(BoneMatrix(raw,2),inv)){for(int b=3;b<=19;++b)StoreBone(c.fingers[side],b-3,Multiply(inv,BoneMatrix(raw,b)));c.valid[side]=true;}}
}
Mat4 OtherMirrorHand319(const Mat4& root,const Mat4& hand,int source,bool contact){return ReloadMirrorHand341(root,hand,source,contact,A2000LeftCharge336());}
bool OtherNative319(int side,Mat4& hand){
 const int source=1-side;const auto& c=g_otherSkin319;const auto h=fc3hands303::ReadPresentation314();if(!PkmArmsNative()||c.entity!=h.entity||c.epoch!=g_armPoseEpoch||c.profile!=g_weaponMechanicProfile||g_frame-c.frame[source]>1||g_rifleAttachFrame!=g_frame||g_rifleAttachEpoch!=g_armPoseEpoch)return false;
 Mat4 root={},qi={},ri={};if(!OtherRoot319(root)||!Invert(ArmBodyToCamera(),qi)||!Invert(root,ri))return false;
 const auto local=Multiply(ri,Multiply(qi,Multiply(g_rifleAttachMove,c.nativeView[source][0])));hand=OtherMirrorHand319(root,local,source,true);return true;
}
bool OtherArm319(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(!scope.active||!LeftOther319(true))return false;ShotgunLogicalScope318 logical(scope);Mat4 root={},grip={},support={};if(!OtherRoot319(root)||!OtherGrip319(grip,support))return false;
 if(OtherNative319(side,desired))return true;
 if(side==1){
  if(R700Model()&&R700Hold()&&!R700RightOnBolt())return false;
  if(R700Model()&&R700RightOnBolt()){desired=OtherMirrorHand319(root,R700ClipPose().hand,0,true);return true;}
  if(RpgClipPlaying()){desired=OtherMirrorHand319(root,RpgClipPose().right,0,true);return true;}
  desired=OtherMirrorHand319(root,grip,0,false);return true;
 }
 if(RpgClipPlaying()){desired=OtherMirrorHand319(root,RpgClipPose().left,1,true);return true;}
 if(Gm94ClipPlaying()){desired=OtherMirrorHand319(g_gm94.frameBody,Gm94ClipPose().hand,1,true);return true;}
 if(FlamerActive()&&(FlamerClipPlaying()||g_flamer.leverGrab)){const auto hand=FlamerClipPlaying()?FlamerClipPose().hand:Multiply(FlamerHandle(),FlamerKeyMat(kFlamerLeftWristFromLever));desired=ReloadMirrorHand341(root,hand,1,true,true);return true;}
 const bool supportBolt=R700Model()&&(R700Hold()||R700ClipPlaying());
 if(!(g_twoHand||supportBolt)||g_pistolReload.held||g_pistolReload.removalHeld||PistolOwnsLeft()||(RpgModel()&&g_rpg.carrying)||(Gm94Model()&&(g_gm94.carrying||g_gm94.spentCarrying))||(FlamerModel()&&g_flamer.carrying))return false;
 if(RpgModel()){desired=ReloadMirrorHand341(root,support,1,true,false);return true;}
 desired=ReloadMirrorHand341(root,support,1,false,true);return true;
}
// Right-owner RPG support uses its captured grip, not a generic animated free hand.
bool RpgSupportArm349(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(scope.active||side!=1||!RpgActive()||!g_twoHand||g_rpg.carrying||g_rpg.pending||RpgClipPlaying())return false;
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;
 desired=Multiply(ArmBodyToCamera(),Multiply(root,RpgKeyMat(kRpgLeftSupportFromFrame)));return true;
}
bool OtherFingers319(int side,float* bones){
 if(!LeftOther319())return false;const int role=1-side;const float* fingers=nullptr;float native[204];
 const auto& cache=g_otherSkin319;const auto h=fc3hands303::ReadPresentation314();const bool valid=cache.entity==h.entity&&cache.epoch==g_armPoseEpoch&&cache.profile==g_weaponMechanicProfile;
 if(PkmArmsNative()&&valid&&g_frame-cache.frame[role]<=1){Mat4 inv={};if(Invert(cache.nativeView[role][0],inv)){for(int b=3;b<=19;++b)StoreBone(native,b-3,Multiply(inv,cache.nativeView[role][b-2]));fingers=native;}}
 if(!fingers&&role==0){
  if(R700Model())fingers=R700RightOnBolt()?R700ClipPose().fingers:Bolt().rightIdle;
  else if(PkmModel())fingers=CurrentRifle().rightGrip;
  else if(RpgModel())fingers=RpgClipPlaying()?RpgClipPose().rightFingers:kRpgRightIdleFingers;
  else if(Gm94Model())fingers=kGm94RightIdleFingers;
  else if(FlamerModel())fingers=kFlamerRightIdleFingers;
  else if(valid&&cache.valid[0])fingers=cache.fingers[0];
 }else if(!fingers&&role==1){
  if(fc3hands303::OtherMagazine319(g_weaponMechanicProfile)&&(g_pistolReload.held||g_pistolReload.removalHeld))return false; //312 owns the carried magazine
  if(R700Model()&&(g_twoHand||R700Hold()||R700ClipPlaying()))fingers=Bolt().leftIdle;
  else if(PkmModel()&&g_twoHand)fingers=CurrentRifle().supportGrip;
  else if(RpgModel())fingers=RpgClipPlaying()?RpgClipPose().leftFingers:g_rpg.carrying?kRpgLeftRocketFingers:g_twoHand?kRpgLeftIdleFingers:nullptr;
  else if(Gm94Model())fingers=Gm94ClipPlaying()?Gm94ClipPose().fingers:(g_gm94.carrying||g_gm94.spentCarrying)?kGm94LeftShellCarryFingers:g_twoHand?kGm94LeftIdleFingers:nullptr;
  else if(FlamerModel())fingers=FlamerClipPlaying()?FlamerClipPose().fingers:g_flamer.leverGrab?kFlamerLeftLeverFingers:g_flamer.carrying?kFlamerLeftCanisterReturnFingers:g_twoHand?kFlamerLeftIdleFingers:nullptr;
  else if(RepairModel247()&&g_twoHand)fingers=kRepairKnobFingers250;
  else if(valid&&cache.valid[1])fingers=cache.fingers[1];
 }
 if(!fingers)return false;AkMirrorFingers316(bones,fingers);return true;
}
void RecordOtherHands319(ID3D11DeviceContext* context,int side,const float* bones){
 if(!LeftOther319())return;Mat4 view={},qi={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),qi))return;const auto semantic=Multiply(Multiply(qi,view),SidearmMirror315());
 if(side==1&&R700Model()){g_r700.rightHandBody=semantic;g_r700.rightHandFrame=g_frame;}
 if(side!=0)return;
 if(RpgModel()){g_rpg.leftWrist=semantic;g_rpg.leftFrame=g_frame;}
 if(Gm94Model()){g_gm94.leftWrist=semantic;g_gm94.leftFrame=g_frame;}
 if(FlamerModel()){g_flamer.leftWrist=semantic;g_flamer.leftFrame=g_frame;}
}
void PublishOtherTransfer319(){
 const auto h=fc3hands303::Read();const bool enabled=fc3hands303::Other319(g_weaponMechanicProfile)&&h.enabled&&h.engaged&&!h.empty&&h.nativeHoldingSide==0;bool allowed=false,contact=false;
 Mat4 root={},grip={},support={};if(enabled&&h.active&&g_frame-g_weaponMechanicFrame<=2&&h.hand>=0&&h.hand<2){
  allowed=g_physicalValid303[0]&&g_physicalValid303[1]&&!OtherBusy319()&&!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&OtherRoot319(root)&&OtherGrip319(grip,support);
  const int other=1-h.hand;const auto& a=g_freeHandAttachment[other];if(allowed&&a.profile==g_weaponMechanicProfile&&a.epoch==g_armPoseEpoch&&g_frame-a.frame<=2){
   Mat4 controller={},inv={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[other][i];const auto wrist=Multiply(controller,a.relative);const V3 palm=Apply(wrist,{other?-.6781:.6781,.0519,1.0387});
   if(Invert(root,inv)){const V3 local=Apply(inv,palm),rear=Apply(grip,{.6781,.0519,1.0387}),front=Apply(support,{-.6781,.0519,1.0387});const double d=Length(Sub(local,rear));contact=d<=.085*(std::max)(.1f,g_worldScale)&&d<Length(Sub(local,front));
    if(fc3hands303::OtherMagazine319(g_weaponMechanicProfile)){for(int b:{0,3})if(PistolGuideContact(root,b,palm)&&Length(Sub(local,b==0?V3{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]}:PistolRackGuide()))<=d)contact=false;}
   }
  }
 }
 fc3hands303::PublishTransfer307(enabled,contact,allowed,h.generation,h.entity,g_armPoseEpoch,true);
}

// Fit the opposite anatomical wrist at the actual native bow handle contact,
// rather than translating the whole canonical wrist by two palm widths.
Mat4 BowHoldingGrip350(){
 const auto hand=BowMat(kBowLeftHandFromRoot),mirror=SidearmMirror315();Mat4 plane=mirror;plane.m[3]=2*Apply(hand,{-.6781,.0519,1.0387}).x;
 return Multiply(plane,Multiply(hand,mirror));
}
bool RightBowRoot350(Mat4& root){
 if(!RightBow319(true)||!g_physicalValid303[0])return false;const auto& a=g_freeHandAttachment[0];
 if(a.epoch!=g_armPoseEpoch||a.profile!=10||g_frame-a.frame>2)return false;
 Mat4 controller={},inverse={};for(int k=0;k<16;++k)controller.m[k]=g_physicalBody303[0][k];if(!Invert(BowHoldingGrip350(),inverse))return false;
 root=Multiply(Multiply(controller,a.relative),inverse);return true;
}
// Bow keeps its native LEFT holding/right drawing roles; opposite ownership
// records the actual physical hands back into those semantic roles.
void RecordOtherBow319(ID3D11DeviceContext* context,int side,const float* bones){
 if(!RightBow319())return;Mat4 view={},qi={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),qi))return;Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[side][i];const auto& a=g_freeHandAttachment[side];if(!g_physicalValid303[side]||a.epoch!=g_armPoseEpoch||a.profile!=10||g_frame-a.frame>2)return;
 if(g_bow.epoch!=g_armPoseEpoch){g_bow={};g_bow.epoch=g_armPoseEpoch;}const auto actual=Multiply(controller,a.relative);
 if(side==0){Mat4 root={};if(!RightBowRoot350(root))return;g_bow.left=Multiply(root,BowMat(kBowLeftHandFromRoot));g_bow.handFrame[1]=g_frame;}else{g_bow.right=Multiply(actual,SidearmMirror315());g_bow.handFrame[0]=g_frame;}
}
bool OtherBowArm319(const PhysicalArmScope303& scope,int side,Mat4& desired){
 if(!scope.active||!RightBow319(true))return false;Mat4 root={},inv={};if(side==0){if(!BowRootBody(root))return false;desired=Multiply(ArmBodyToCamera(),Multiply(root,BowHoldingGrip350()));return true;}if(side!=1||!g_bow.nocked)return false;if(!BowRootBody(root)||!Invert(BowMat(kBowArrowFromRightWrist),inv))return false;const auto hand=Multiply(BowSeatedArrow(root,g_bow.draw),inv);Mat4 qi={},ri={};if(!Invert(root,ri))return false;desired=OtherMirrorHand319(root,Multiply(ri,hand),0,true);return true;
}
bool OtherBowFingers319(int side,float* bones){if(!RightBow319())return false;const int role=1-side;if(role==0&&!g_bow.held&&!g_bow.nocked)return false;AkMirrorFingers316(bones,role==1?kBowLeftGripFingers:kBowRightGripFingers);return true;}
void PublishBowTransfer319(){
 const auto h=fc3hands303::Read();const bool enabled=BowModel()&&h.enabled&&h.engaged&&!h.empty&&h.nativeHoldingSide==1;bool allowed=false,contact=false;Mat4 root={};
 if(enabled&&h.active&&h.hand>=0&&h.hand<2){allowed=g_physicalValid303[0]&&g_physicalValid303[1]&&!g_bow.held&&!g_bow.nocked&&!g_bow.shooting&&!g_bow.pending&&!g_bow.releasePending&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&BowHandsReady()&&BowRootBody(root);const int other=1-h.hand;const auto& a=g_freeHandAttachment[other];if(allowed&&a.profile==10&&a.epoch==g_armPoseEpoch&&g_frame-a.frame<=2){Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[other][i];const auto wrist=Multiply(controller,a.relative);const V3 palm=Apply(wrist,{other?-.6781:.6781,.0519,1.0387}),handle=Apply(Multiply(root,BowMat(kBowLeftHandFromRoot)),{-.6781,.0519,1.0387});const double d=Length(Sub(palm,handle));contact=d<=.085*(std::max)(.1f,g_worldScale)&&d<Length(Sub(palm,BowNockPoint(root)));}}
 fc3hands303::PublishTransfer307(enabled,contact,allowed,h.generation,h.entity,g_armPoseEpoch,true);
}
