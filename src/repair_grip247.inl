#include "repair_pose250.inl"
// Native PID67768 paired tool ANCHOR / right wrist,232 same-frame samples.
// Dominant rigid grip has median translation deviation1.1e-7m; discard equip
// outliers instead of learning their animated wrist on every draw.
const Mat4 kRepairRightHand247={{
 .9338378734,.0190692155,.3571878934,-.9617884941,
 -.0006285977,.9986638953,-.0516723243,.0055793856,
 -.3576960037,.0480290459,.9326021551,-.7701908134,0,0,0,1}};
// Authored IK_LeftHand target; only its contact point owns support steering.
const Mat4 kRepairSupportHand247={{1,0,0,.591,0,1,0,.1247393712,0,0,1,-1.1422839600,0,0,0,1}};
bool RepairModel247(){return g_weaponMechanicProfile==40&&g_frame-g_weaponMechanicFrame<=2;}
struct RepairSupport248{V3 local={};UINT epoch=0;bool valid=false;}g_repairSupport248;
void RepairSupportReset248(){if(!g_twoHand||!RepairModel247()||!g_handLeftValid||!g_handRightValid)g_repairSupport248={};}
bool RepairSupportTurn248(Mat4& root,const Mat4& hand){
 if(!RepairModel247())return false;
 const V3 actual={g_handBody[1][3],g_handBody[1][7],g_handBody[1][11]};
 Mat4 inverse={};if(!Invert(root,inverse))return true;
 auto& s=g_repairSupport248;
 // Grip anywhere inside the calibrated volume without snapping the tool onto
 // a different wrist/palm point. Only subsequent relative hand motion steers.
 if(!s.valid||s.epoch!=g_armPoseEpoch){s.local=Apply(inverse,actual);s.epoch=g_armPoseEpoch;s.valid=true;return true;}
 const V3 pivot=Apply(Multiply(root,hand),{.591,.011,1.149}),nominal=Apply(root,s.local);
 if(Length(Sub(nominal,pivot))>.05&&Length(Sub(actual,pivot))>.05)
  root=Multiply(TurnAndMove(Sub(nominal,pivot),Sub(actual,pivot),pivot,pivot),root);
 return true;
}
bool RepairPalette247(ID3D11DeviceContext* context,float* bones,const Mat4& toView,UINT start,UINT count){
 if(!MotionInteractionEnabled()||MacheteHeld())return false;
 DrawCall call={};call.b=start;call.a=count;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=213320||d.indexBytes!=34200||d.stride!=40||d.offset||d.format!=57)return false;
 const int anchor=start==4824&&count==12276?1:((start==864&&count==888)||(start==1752&&count==984)||(start==2736&&count==2088))?0:-1;
 const bool wires=start==0&&count==864;if(anchor<0&&!wires)return false;
 g_weaponMechanicProfile=40;g_weaponMechanicFrame=g_frame;
 Mat4 root={},grip={},fromView={};if(!ControllerCurrentRoot124(root,grip)||!Invert(toView,fromView))return false;
 const Mat4 fp=Multiply(fromView,Multiply(ArmBodyToCamera(),root));
 if(!wires){Mat4 inverse={};if(!Invert(BoneMatrix(bones,anchor),inverse))return false;MoveBones(bones,0,47,Multiply(fp,inverse));}
 // Wire-only sections have no ANCHOR. Their bind-pose skin matrices are
 // identity: keep all wire sections on that same fixed root, never treat an
 // animated wire/pressure knob as the receiver. Retain the pressure knob's
 // native rotation on the metal section (slot0), independently of the grip.
 if(wires){StoreBone(bones,0,fp);StoreBone(bones,1,fp);}
 else if(anchor==1)StoreBone(bones,2,fp);
 else if(start!=2736){for(int i=1;i<=3;++i)StoreBone(bones,i,fp);}
 static UINT logs=0;if(logs++<5)Log("[REPAIR247] part%u/%u pinned to captured controller grip, ANCHOR%d; native wrist/knob cannot move tool root",start,count,anchor);
 return true;
}
bool RepairTransport247(const Mat4& view,Mat4& transport){
 if(!RepairModel247()||!MotionInteractionEnabled()||g_frame-g_repairTool.rootFrame>2||g_repairTool.rootFrame>g_frame)return false;
 Mat4 root={},grip={},vi={},ni={};if(!ControllerCurrentRoot124(root,grip)||!Invert(view,vi)||!Invert(g_repairTool.nativeRoot,ni))return false;
 // Authored FX_FIRE is10.82mm beyond the actual nozzle rim along local -Z.
 // Keep selection/native repair behavior authored; seat the visible pilot on
 // the rim measured in welding_torch.xbg (centre X=.00003052,Y=.3634924).
 Mat4 nozzle=HandIdentity();nozzle.m[3]=.0000305194;nozzle.m[11]=.0108199065;
 transport=Multiply(vi,Multiply(ArmBodyToCamera(),Multiply(root,Multiply(nozzle,ni))));return true;
}
bool RepairSupportContact247(V3& point,V3& palm){
 if(!RepairModel247()||!MotionInteractionEnabled()||!g_handLeftValid)return false;
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;
 point=Apply(root,{0,.1357393712,.0067160400});SupportGrabAdjustPoint214(point);
 if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);return true;
}

bool RepairLeftMove249(const Mat4& toView,const float* bones,Mat4& move){
 if(!RepairModel247()||!g_twoHand||!MotionInteractionEnabled()||!g_handLeftValid||!g_handRightValid)return false;
 Mat4 root={},grip={},inverse={};if(!ControllerCurrentRoot124(root,grip)||!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 // The grab box calibrates acquisition, not Jason's native knob grip.
 const Mat4 hand=kRepairKnobHand250;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(root,hand)),inverse);return true;
}
bool RepairLeftGrip249(float* bones){
 if(!RepairModel247()||!g_twoHand||!MotionInteractionEnabled())return false;
 const Mat4 hand=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(hand,BoneMatrix(kRepairKnobFingers250,b-3)));return true;
}
