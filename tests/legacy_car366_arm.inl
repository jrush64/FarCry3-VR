// Unmodified366 witness; only function name changed.
ID3D11Buffer* LegacyBuildCarArm366(ID3D11DeviceContext* context,int side){
    fc3aim::VehicleSample170 vehicle={};fc3pad::Controllers controls={};
    if(side<0||side>1||!CarHandsAllowed353(vehicle,controls)||!g_physicalValid303[side]||!g_armPaletteCb[side])return nullptr;
    float bones[48*12]={},original[25*12]={};Mat4 toView={},fromView={};
    if(!DrawSpace(context,bones,toView,fromView,3,true))return nullptr;
    CameraSummary armCamera363={};ID3D11Buffer* armCb363=nullptr;context->VSGetConstantBuffers(1,1,&armCb363);UINT armBytes363=sizeof(armCamera363);if(armCb363&&SUCCEEDED(armCb363->GetPrivateData(kCameraSummaryGuid,&armBytes363,&armCamera363))&&armBytes363==sizeof(armCamera363)){TickCarWheel363(armCamera363);TickHandlebars355(armCamera363);}SafeRelease(armCb363);
    memcpy(original,bones,sizeof(original));CaptureCarLeft354(vehicle.vehicleId,side,toView,bones);CaptureBarHand355(vehicle.vehicleId,side,toView,bones);const Mat4 head=CarHeadView352();
    ArmJoints joints={};joints.valid=true;
    for(int k=0;k<3;++k){const double f=side==1&&k==0?-1:1;joints.s[k]=f*kNeutralArmJoints[0][k];joints.e[k]=f*kNeutralArmJoints[0][3+k];joints.w[k]=f*kNeutralArmJoints[0][6+k];}
    joints=TransformArmJoints(joints,head);
    for(int b=0;b<25;++b){Mat4 n=BoneMatrix(kNeutralArmView[0],b);
        if(side==1)for(int r=0;r<3;++r)for(int k=0;k<4;++k)n.m[r*4+k]*=(r==0?-1:1)*(k==0?-1:1);
        StoreBone(bones,b,Multiply(fromView,Multiply(head,n)));}
    Mat4 pose={},ref={};for(int i=0;i<16;++i){pose.m[i]=g_physicalBody303[side][i];ref.m[i]=g_handRef[side][i];}
    // A gripping palm is attached to its captured point on the controlled
    // rim. Raw controller positions still drive steering; no feedback from IK.
    if(g_car352.vehicle==vehicle.vehicleId&&g_car352.model>=0&&g_car352.gripping[side]&&GetTickCount64()-g_carBodyAt363<120){
        Mat4 wheelToBody={};if(Invert(g_carBodyToWheel353,wheelToBody)){
            const auto& asset=kCarAssets352[g_car352.model];const double polar=g_car352.angle+g_car352.gripOffset[side];
            const V3 target=Apply(wheelToBody,Add(asset.hub,{asset.radius*std::cos(polar),0,asset.radius*std::sin(polar)}));
            const V3 shift=Sub(target,Apply(pose,{0,-.015,.015}));pose.m[3]+=shift.x;pose.m[7]+=shift.y;pose.m[11]+=shift.z;
        }
    }
    BarPlaceController355(vehicle.vehicleId,side,pose);
    Mat4 handMove=HandMove(Multiply(head,pose),Multiply(head,ref),{joints.w[0],joints.w[1],joints.w[2]},side),forearm={},upper={};
    const bool fittedCar363=CarGripMove354(vehicle.vehicleId,side,Multiply(toView,BoneMatrix(bones,2)),handMove);
    BarGripMove355(vehicle.vehicleId,side,Multiply(toView,BoneMatrix(bones,2)),handMove);
    // Also constrain the generic controller fallback while the native grip
    // is still being sampled. Only translation changes; raw tracking steers.
    if(CarAttached354(vehicle.vehicleId,side)||BarAttached355(vehicle.vehicleId,side)){
        Mat4 inverse={},inverseHead={};if(Invert(head,inverseHead)){
            const V3 viewPalm=Apply(Multiply(handMove,Multiply(toView,BoneMatrix(bones,2))),{side?-.6781:.6781,.0519,1.0387});V3 bodyTarget={};bool targetReady=false;
            if(BarAttached355(vehicle.vehicleId,side)&&Invert(g_bodyToBar355,inverse)){bodyTarget=Apply(inverse,Apply(BarTurn355(g_bars355.angle),kBarAssets355[g_barModel355].grip[side]));targetReady=true;}
            else if(CarAttached354(vehicle.vehicleId,side)&&Invert(g_carBodyToWheel353,inverse)){
                const auto& a=kCarAssets352[g_car352.model];const double polar=g_car352.angle+g_car352.gripOffset[side];const auto localPalm=Apply(g_carBodyToWheel353,Apply(inverseHead,viewPalm));const double depth=fittedCar363?(std::max)(-.025,(std::min)(.025,localPalm.y-a.hub.y)):0;
                bodyTarget=Apply(inverse,Add(Add(a.hub,{a.radius*std::cos(polar),depth,a.radius*std::sin(polar)}),HandRimOffset365(polar,side)));targetReady=true;
            }
            if(targetReady){const V3 shift=Sub(Apply(head,bodyTarget),viewPalm);handMove.m[3]+=shift.x;handMove.m[7]+=shift.y;handMove.m[11]+=shift.z;}
        }
    }
    g_carBodyView352=head;g_carArmScope352=true;
    const bool solved=SolveArm(joints,handMove,side,forearm,upper,true,true);g_carArmScope352=false;
    if(!solved)return nullptr;
    const Mat4 moves[3]={upper,forearm,handMove};
    for(int b=0;b<25;++b){const Mat4 final=Multiply(moves[ArmGroup(b)],Multiply(toView,BoneMatrix(bones,b)));
        StoreBone(bones,b,Multiply(fromView,final));Mat4 inv={};if(Invert(Multiply(toView,BoneMatrix(original,b)),inv))g_armViewMoves[side][b]=Multiply(final,inv);}
    const float grip=controls.weaponRoles303?(side?controls.physicalLeftGrip303:controls.physicalRightGrip303):(side?controls.leftGrip:controls.rightGrip);
    // Use physical controls directly; weapon roles and native driving animation
    // cannot choose a grip or close the wrong hand here.
    const double mirror=side==1?-1.:1.;
    if(!BarGripFingers355(vehicle.vehicleId,side,bones)&&!CarGripFingers354(vehicle.vehicleId,side,bones))for(int f=0;f<5;++f){const FingerChain& chain=kFingers[f];
        Mat4 oldParent=BoneMatrix(bones,chain.parent),newParent=oldParent;
        for(int k=0;k<3;++k){const Mat4 oldChild=BoneMatrix(bones,chain.bones[k]);Mat4 inverse={};if(!Invert(oldParent,inverse))break;
            const double joint[3]={chain.joints[k][0]*mirror,chain.joints[k][1],chain.joints[k][2]};
            const Mat4 child=Multiply(newParent,PartialTurn(Multiply(inverse,oldChild),joint,grip));
            StoreBone(bones,chain.bones[k],child);oldParent=oldChild;newParent=child;}}
    Mat4 inverseHead={};if(Invert(head,inverseHead)){
        for(int b=0;b<25;++b)g_armFinalBody[side][b]=Multiply(inverseHead,Multiply(toView,BoneMatrix(bones,b)));
        g_armFinalFrame[side]=g_frame;
    }
    if(CarAttached354(vehicle.vehicleId,side)||BarAttached355(vehicle.vehicleId,side)){
        static ULONGLONG next363[2]={};static unsigned rows363[2]={};const auto now363=GetTickCount64();if(now363>=next363[side]&&rows363[side]<48){++rows363[side];next363[side]=now363+1000;
            const V3 palm363=Apply(g_armFinalBody[side][2],{side?-.6781:.6781,.0519,1.0387});V3 actual363={},target363={};
            if(BarAttached355(vehicle.vehicleId,side)){actual363=Apply(g_bodyToBar355,palm363);target363=Apply(BarTurn355(g_bars355.angle),kBarAssets355[g_barModel355].grip[side]);}
            else{actual363=Apply(g_carBodyToWheel353,palm363);const auto& a=kCarAssets352[g_car352.model];const double polar=g_car352.angle+g_car352.gripOffset[side];target363=Add(a.hub,{a.radius*std::cos(polar),actual363.y-a.hub.y,a.radius*std::sin(polar)});}
            Log("[VEHICLE363 PALM] vehicle=%llu side=%d handlebars=%d gripContactError=%.6fm current-frame world/fractions",vehicle.vehicleId,side,BarAttached355(vehicle.vehicleId,side),Length(Sub(actual363,target363)));
        }
    }
    memcpy(g_armOriginal[side],original,sizeof(original));g_armMovesFrame[side]=g_frame;
    const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_armPaletteCb[side],0,nullptr,bones,0,0);g_own=own;
    return g_armPaletteCb[side];
}

