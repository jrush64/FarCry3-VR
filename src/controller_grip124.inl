bool CalibratedSupportTurn350(Mat4&,const Mat4&,const Mat4&);
// Immutable neutral arm for every right-arm stream in this pose epoch.
// Seed before the first weapon draw too: early gun parts must not wait for a
// later arm draw, and switching vertex buffers cannot relearn an animated hold.
StableArmPose g_controllerNeutral124={};
void SeedControllerHand124()
{
    if(g_controllerNeutral124.epoch==g_armPoseEpoch&&g_freeRightEpoch==g_armPoseEpoch)return;
    if(!g_handRightValid)return;
    Mat4 qi={},controller={},ci={};if(!Invert(ArmBodyToCamera(),qi))return;
    for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];if(!Invert(controller,ci))return;
    const bool haveGrip=g_freeRightEpoch==g_armPoseEpoch;
    auto& pose=g_controllerNeutral124;pose={};pose.epoch=g_armPoseEpoch;
    ArmJoints fixed={};fixed.valid=true;for(int i=0;i<3;++i){fixed.s[i]=kNeutralArmJoints[0][i];fixed.e[i]=kNeutralArmJoints[0][3+i];fixed.w[i]=kNeutralArmJoints[0][6+i];}
    pose.joints=TransformArmJoints(fixed,qi);
    for(int b=0;b<25;++b)StoreBone(pose.bones,b,Multiply(qi,BoneMatrix(kNeutralArmView[0],b)));
    const Mat4 move=StableControllerHandMove(0,{fixed.w[0],fixed.w[1],fixed.w[2]});
    if(!haveGrip){g_freeRightFromController=Multiply(ci,Multiply(qi,Multiply(move,BoneMatrix(kNeutralArmView[0],2))));g_freeRightEpoch=g_armPoseEpoch;}
}
bool ControllerNeutralArm124(const Mat4& fromView,float* bones,ArmJoints& joints)
{
    SeedControllerHand124();if(g_controllerNeutral124.epoch!=g_armPoseEpoch)return false;
    const Mat4 q=ArmBodyToCamera(),toModel=Multiply(fromView,q);
    for(int b=0;b<25;++b)StoreBone(bones,b,Multiply(toModel,BoneMatrix(g_controllerNeutral124.bones,b)));
    joints=TransformArmJoints(g_controllerNeutral124.joints,q);return true;
}
// One controller-owned right wrist, shared by the arm and firearm roots.
// g_freeRightFromController is derived solely from the fixed neutral arm,
// never from a currently animated gun or a previous frame's native wrist.
bool ControllerGripRoot124(const Mat4& hand,const Mat4* support,Mat4& root)
{
    SeedControllerHand124();
    if(!g_handRightValid||g_freeRightEpoch!=g_armPoseEpoch)return false;
    Mat4 controller={},inverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    if(!Invert(hand,inverse))return false;
    root=Multiply(Multiply(controller,g_freeRightFromController),inverse);
    if(g_twoHand&&g_handLeftValid&&support&&!RepairSupportTurn248(root,hand)&&!CalibratedSupportTurn350(root,hand,*support)){
        const V3 pivot=Apply(Multiply(root,hand),{.591,.011,1.149});
        const V3 nominal=Apply(Multiply(root,*support),{-.591,.011,1.149});
        const V3 actual=ControllerWrist(g_handBody[1],1);
        if(Length(Sub(nominal,pivot))>.05&&Length(Sub(actual,pivot))>.05)
            root=Multiply(TurnAndMove(Sub(nominal,pivot),Sub(actual,pivot),pivot,pivot),root);
    }
    for(double value:root.m)if(!std::isfinite(value))return false;
    return true;
}
bool ControllerMagazineRoot124(Mat4& root)
{
    if(!PistolModel()||R700Hold()||MacheteHeld())return false;
    SeedControllerHand124();
    if(R700Model()&&R700ControllerRoot(root))return true;
    return ControllerGripRoot124(MagazineHandFromFrame(),AkModel()?CurrentRifle().support:nullptr,root);
}
bool ControllerCurrentRoot124(Mat4& root,Mat4& grip)
{
    if(MacheteHeld()||BowModel())return false;
    Mat4 support={};const Mat4* supporting=nullptr;
    if(PistolModel()&&!R700Hold()){grip=MagazineHandFromFrame();return ControllerMagazineRoot124(root);}
    else if(Gm94Model()){grip=Gm94KeyMat(kGm94RightHandFromFrame);support=Gm94KeyMat(kGm94LeftSupportFromFrame);supporting=&support;}
    else if(SpasModel()){grip=SpasKeyMat(kSpasRightHandFromFrame);support=SpasKeyMat(kSpasLeftSupportFromFrame);supporting=&support;}
    else if(RevolverModel())grip=kRevolverRightHandFromFrame;
    else if(RepairModel247()){grip=kRepairRightHand247;support=kRepairSupportHand247;supporting=&support;}
    else if(FlareModel())grip=FlareKeyMat(kFlareRightHandFromFrame);
    else if(RpgModel()){grip=RpgKeyMat(kRpgRightHandFromFrame);support=RpgKeyMat(kRpgLeftSupportFromFrame);supporting=&support;}
    else if(FlamerModel()){grip=FlamerKeyMat(kFlamerRightHandFromFrame);support=FlamerKeyMat(kFlamerLeftSupportFromFrame);supporting=&support;}
    else if(ShotgunHandContext()){
        grip=kShotgunHandFromGun;
        if(g_shotHand.leftValid){
            Mat4 basis={};if(HandBasis(grip,basis)){
                const V3 delta=g_shotHand.leftRel,p=Apply(grip,{.591,.011,1.149});
                const V3 left=Add(p,{basis.m[0]*delta.x+basis.m[1]*delta.y+basis.m[2]*delta.z,basis.m[4]*delta.x+basis.m[5]*delta.y+basis.m[6]*delta.z,basis.m[8]*delta.x+basis.m[9]*delta.y+basis.m[10]*delta.z});
                support=HandIdentity();support.m[3]=left.x+.591;support.m[7]=left.y-.011;support.m[11]=left.z-1.149;supporting=&support;
            }
        }
    }else return false;
    return ControllerGripRoot124(grip,supporting,root);
}
bool ControllerGripWorld124(ID3D11DeviceContext* context,const Mat4& grip,const Mat4* support,Mat4& world)
{
    Mat4 body={},inverse={};CameraSummary camera={};
    if(!ControllerGripRoot124(grip,support,body)||!PistolGuideCamera(context,camera)||!Invert(HullView(camera),inverse))return false;
    world=Multiply(inverse,Multiply(ArmBodyToCamera(),body));return true;
}
bool ControllerRightHand124(Mat4& desired)
{
    if(BowModel())return false; // the nocked arrow/string owns the drawing wrist
    if(RpgClipPlaying())return false; // captured insertion wrist owns this action
    if(Lever1887Active()&&g_lever.clip>=0)return false; // captured seating/cycle wrist owns this brief action
    if(!g_handRightValid||g_freeRightEpoch!=g_armPoseEpoch)return false;
    Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
    Mat4 hand=Multiply(controller,g_freeRightFromController),root={},grip={};
    if(ControllerCurrentRoot124(root,grip))hand=Multiply(root,grip);
    else if(Lever1887Active()&&!Lever1887LeftHold())hand=Multiply(Lever1887GunBody(),kL1887RightHandFromFrame);
    desired=Multiply(ArmBodyToCamera(),hand);
    static UINT seen[65]={};const int slot=g_weaponMechanicProfile>=0&&g_weaponMechanicProfile<64?g_weaponMechanicProfile+1:0;
    if(seen[slot]!=g_armPoseEpoch){seen[slot]=g_armPoseEpoch;Log("[HAND124] profile %ld epoch %u: final right wrist/controller grip owns pose; native locomotion wrist rejected",g_weaponMechanicProfile,g_armPoseEpoch);}
    return true;
}
bool PinWeaponPalette124(float* bones,const Mat4& toView,const Mat4& grip,int anchor)
{
    Mat4 root={},inverse={},fromView={};
    Mat4 currentGrip={};
    if(!ControllerCurrentRoot124(root,currentGrip)||!Invert(BoneMatrix(bones,anchor),inverse)||!Invert(toView,fromView))return false;
    // bones are model-space: target model root times inverse model root.
    const Mat4 move=Multiply(fromView,Multiply(Multiply(ArmBodyToCamera(),root),inverse));
    MoveBones(bones,0,47,move);return true;
}

// One-handed shotguns now display a fixed captured grip. The controller's +Z
// is therefore not the receiver's +Y barrel. Keep supported aiming unchanged.
bool OneHandShotgunAim125(V3& forward)
{
    if(g_twoHand||(!M133SupportProfile56()&&!SpasModel())||g_frame-g_weaponMechanicFrame>2||
       !MotionInteractionEnabled()||!g_eyeViewsSet||!g_handRightValid||!fc3pad::Latest().active||
       fc3pad::GameplayInputBlocked()||MacheteHeld())return false;
    Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;
    const V3 barrel={root.m[1],root.m[5],root.m[9]};const double length=Length(barrel);
    if(!std::isfinite(length)||length<1e-6)return false;
    forward=Scale(barrel,1/length);return true;
}
