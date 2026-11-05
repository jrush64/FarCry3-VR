//352: render-owned car wheel. World draw classification and camera remain native.
#include "car_wheel_assets352.inl"
fc3wheel352::Wheel g_car352;
LONG g_carFrame352=-100;
Mat4 g_carBodyToWheel353={};
ULONGLONG g_carSeen352=0,g_carBodyAt363=0;double g_carNativeAngle363=0;
ID3D11Buffer* g_carCb352[8]={};UINT g_carCbBytes352[8]={};
int CarModel352(const fc3weapon::Geometry& g){
    if(g.stride!=32||g.offset||g.format!=57)return -1;
    for(const auto& d:kCarDraws352)if(g.vertexBytes==d.vb&&g.indexBytes==d.ib&&g.start==d.start&&g.count==d.count)return int(d.model);
    return -1;
}
Mat4 CarPose352(const fc3aim::VehicleSample170& s){
    Mat4 m={};m.m[15]=1;
    for(int r=0;r<3;++r){for(int c=0;c<3;++c)m.m[r*4+c]=s.basis[c][r];m.m[r*4+3]=s.position[r];}
    return m;
}
Mat4 CarHeadView352(){
    // Both eyes share rotation. Averaging the eye positions removes IPD.
    Mat4 v={};for(int i=0;i<16;++i)v.m[i]=(double(g_eyeViewsBody[0].view[i])+g_eyeViewsBody[1].view[i])*.5;
    return v;
}
void CarReject352(int model,const char* why,const Mat4* native=nullptr){
    static unsigned rows[5]={};if(model<0||model>=5||rows[model]++>=8)return;
    Log("[CAR352] model=%s rejected=%s origin=%.5f/%.5f/%.5f axisY=%.5f/%.5f/%.5f; native steering retained",
        kCarAssets352[model].name,why,native?native->m[3]:0,native?native->m[7]:0,native?native->m[11]:0,
        native?native->m[1]:0,native?native->m[5]:0,native?native->m[9]:0);
}
// 364: returns nullptr when the driver context allows wheel grips, otherwise
// the reason. The left stick and the age of the physical pose are no longer
// part of the context: neither may throw the hands off the rim.
const char* CarDenied364(fc3aim::VehicleSample170& vehicle,fc3pad::Controllers& c){
    c=fc3pad::Latest();
    if(!fc3pad::CarWheelSteeringEnabled())return "wheel steering disabled";
    if(!g_enabled||!g_headTracking||!g_eyeViewsSet||!g_inGameplay||g_flatViewActive)return "no tracked gameplay view";
    if(g_pistolMenu.open||fc3pad::GameplayInputBlocked()||fc3pad::NativeMenuOpen())return "menu";
    if(!c.active||c.menu)return "controllers inactive";
    if(c.y)return "Y held";
    if(!fc3aim::GetVehicleDrawPose352(vehicle))return "vehicle draw pose missing";
    if(!vehicle.seatValid||vehicle.actualSeat!=1)return "not in the driver seat";
    if(vehicle.type!=0)return "not a car";
    return nullptr;
}
bool CarAllowed352(fc3aim::VehicleSample170& vehicle,fc3pad::Controllers& c){return CarDenied364(vehicle,c)==nullptr;}
void LogCarReleases364(const char* why){
    for(int side=0;side<2;++side)if(g_car352.released364[side]){
        Log("[CAR364 RELEASE] vehicle=%llu side=%d (%s hand) reason=%u %s%s%s angle=%.3f",g_car352.vehicle,side,side?"left":"right",
            g_car352.released364[side],fc3wheel352::ReleaseName364(g_car352.released364[side]),why?" ":"",why?why:"",g_car352.angle);
        g_car352.released364[side]=0; // 366: one line per release, not one per frame until the next Update
    }
}
void DropCar352(const char* why="context"){
    g_car352.Drop();LogCarReleases364(why);g_car352.waitRelease[0]=g_car352.waitRelease[1]=true;
    fc3pad::SetCarWheelInput352(0,0);
}
// 366: a vehicle sample that goes stale for a moment (the 365 log: a 390 ms capture stall
// made GetVehicleSample170 fail and both hands were dropped mid-turn) PAUSES the wheel:
// grips and angle are held and the pad lease is kept alive. Only a stale sample that
// persists for 1.5 s, or a real context change, drops the hands.
ULONGLONG g_carDeniedSince366=0;
bool CarContextGate366(fc3aim::VehicleSample170& vehicle,fc3pad::Controllers& c){
    const char* denied=CarDenied364(vehicle,c);
    if(!denied){if(g_carDeniedSince366)Log("[CAR366 RESUME] vehicle sample back after %llu ms, %d hand(s) still attached",GetTickCount64()-g_carDeniedSince366,int(g_car352.gripping[0])+int(g_car352.gripping[1]));g_carDeniedSince366=0;return false;}
    const auto now=GetTickCount64();
    if(strcmp(denied,"vehicle draw pose missing")==0&&g_car352.owner>=0){
        if(!g_carDeniedSince366){g_carDeniedSince366=now;Log("[CAR366 PAUSE] vehicle sample stale; holding %d hand(s) at angle=%.3f",int(g_car352.gripping[0])+int(g_car352.gripping[1]),g_car352.angle);}
        if(now>=g_carDeniedSince366&&now-g_carDeniedSince366<1500){
            fc3pad::SetCarWheelInput352(g_car352.vehicle,float(g_car352.Demand(fc3pad::CarWheelFullLockDegrees()*fc3wheel352::Pi/180)));return true;}
        DropCar352("vehicle sample stale for 1.5 s");g_carDeniedSince366=0;return true;
    }
    DropCar352(denied);g_carDeniedSince366=0;return true;
}
void TickCarWheel363(const CameraSummary& camera){
    if(g_car352.model<0||g_car352.model>=5||!g_car352.vehicle||!camera.mainView)return;
    if(g_nativeVehicleCamera279){float body[9]={};UINT serial=0;if(!fc3aim::VehicleCameraBasis279(camera.right,camera.up,camera.forward,body,serial))return;}
    fc3aim::VehicleSample170 vehicle={};fc3pad::Controllers c={};
    if(CarContextGate366(vehicle,c))return;
    if(vehicle.vehicleId!=g_car352.vehicle){DropCar352("vehicle changed");return;}
    const int model=g_car352.model;const auto& asset=kCarAssets352[model];Mat4 inverse={};if(!Invert(Multiply(CarPose352(vehicle),asset.rest),inverse)){DropCar352("vehicle pose not invertible");return;}
    Mat4 cameraWorld={};cameraWorld.m[15]=1;const float* axes[]={camera.right,camera.up,camera.forward};for(int r=0;r<3;++r){for(int k=0;k<3;++k)cameraWorld.m[r*4+k]=axes[k][r];cameraWorld.m[r*4+3]=camera.position[r];}
    const Mat4 head=CarHeadView352();const Mat4 bodyToWheel=Multiply(inverse,Multiply(cameraWorld,head));for(double v:bodyToWheel.m)if(!std::isfinite(v)){DropCar352("body-to-wheel not finite");return;}
    g_carBodyToWheel353=bodyToWheel;g_carBodyAt363=GetTickCount64();
    if(g_carFrame352!=g_frame){
        fc3wheel352::Input input;input.allowed=true;input.vehicle=vehicle.vehicleId;input.model=model;input.now=GetTickCount64();
        input.radius=asset.radius;input.smoothing363=fc3pad::VehicleSmoothing363(false)*.001;input.nativeAngle=g_carNativeAngle363;
        input.fullLock=fc3pad::CarWheelFullLockDegrees()*fc3wheel352::Pi/180;
        for(int hand=0;hand<2;++hand){
            // 364: a pose older than 250 ms (frame stall, tracking hiccup) pauses the hand instead of releasing it.
            auto& h=input.hand[hand];h.tracked=g_physicalValid303[hand]&&input.now>=g_physicalAt352&&input.now-g_physicalAt352<250;
            h.grip=c.weaponRoles303?(hand?c.physicalLeftGrip303:c.physicalRightGrip303):(hand?c.leftGrip:c.rightGrip);
            if(h.tracked){Mat4 pose={};for(int i=0;i<16;++i)pose.m[i]=g_physicalBody303[hand][i];
                const V3 contact=Sub(Apply(Multiply(bodyToWheel,pose),{0,-.015,.015}),asset.hub);
                h.x=contact.x;h.y=contact.y;h.z=contact.z;}
        }
        const int oldOwner=g_car352.owner;const int oldMask=(g_car352.gripping[0]?1:0)|(g_car352.gripping[1]?2:0);
        const bool held=g_car352.Update(input);
        LogCarReleases364(nullptr);
        for(int side=0;side<2;++side)if(g_car352.grabbed364[side]){const auto& h=input.hand[side];
            Log("[CAR364 GRIP] vehicle=%llu model=%s side=%d (%s hand) depth=%.3f r=%.3f rim=%.3f polar=%.3f angle=%.3f",
                vehicle.vehicleId,asset.name,side,side?"left":"right",h.y,std::hypot(h.x,h.z),asset.radius,std::atan2(h.z,h.x),g_car352.angle);}
        g_carFrame352=g_frame;g_carBodyAt363=input.now;g_carBodyView352=head;g_carBodyToWheel353=bodyToWheel;
        fc3pad::SetCarWheelInput352(held?vehicle.vehicleId:0,float(g_car352.Demand(input.fullLock)));
        if(oldOwner!=g_car352.owner||oldMask!=((g_car352.gripping[0]?1:0)|(g_car352.gripping[1]?2:0)))Log("[CAR353] vehicle=%llu model=%s gripOwner=%d angle=%.3f demand=%.3f release=%u/%u (1 tracking,2 grip,3 depth,4 radius,5 jump; right0 left1 released-1)",
            vehicle.vehicleId,asset.name,g_car352.owner,g_car352.angle,g_car352.Demand(input.fullLock),g_car352.release363[0],g_car352.release363[1]);
        static ULONGLONG next=0;static unsigned rows=0;
        if(input.now>=next&&rows<120){next=input.now+500;++rows;
            Log("[CAR352 CONTACT] vehicle=%llu model=%s radius=%.3f owner=%d R=%.3f/%.3f/%.3f grip=%.2f L=%.3f/%.3f/%.3f grip=%.2f",
                vehicle.vehicleId,asset.name,asset.radius,g_car352.owner,input.hand[0].x,input.hand[0].y,input.hand[0].z,input.hand[0].grip,
                input.hand[1].x,input.hand[1].y,input.hand[1].z,input.hand[1].grip);}
    }
}
ID3D11Buffer* BuildCarWheelWorld352(ID3D11DeviceContext* context,const DrawCall& call,int variant){
    if(!fc3pad::CarWheelSteeringEnabled()){if(g_car352.owner>=0)DropCar352("wheel steering disabled");return nullptr;}
    if(variant!=0)return nullptr;
    // Reject ordinary scene draws before querying D3D resources. Enabling
    // steering must not add VB/IB COM queries to every world draw.
    if(call.kind!=0||call.d!=0)return nullptr;
    bool candidate=false;for(const auto& d:kCarDraws352)if(call.b==d.start&&call.a==d.count){candidate=true;break;}
    if(!candidate)return nullptr;
    const int model=CarModel352(NativeWeaponGeometry(context,call));if(model<0)return nullptr;
    fc3aim::VehicleSample170 vehicle={};fc3pad::Controllers c={};
    if(CarContextGate366(vehicle,c))return nullptr;
    ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);
    D3D11_BUFFER_DESC desc={};if(cb)cb->GetDesc(&desc);
    float globals[kSmallCbMax/4]={};const UINT bytes=ReadCopy(cb,globals,sizeof(globals));SafeRelease(cb);
    bool fractions=false;const int off=WeaponWorldOffset(context,&fractions);
    if(bytes!=desc.ByteWidth||bytes<UINT((off+(fractions?15:12))*4)){CarReject352(model,"world buffer");return nullptr;}
    Mat4 native={};native.m[15]=1;for(int r=0;r<3;++r)for(int k=0;k<4;++k)native.m[r*4+k]=globals[off+r*4+k]+(k==3&&fractions?globals[off+12+r]:0);
    const auto& asset=kCarAssets352[model];
    const Mat4 base=Multiply(CarPose352(vehicle),asset.rest);Mat4 invBase={};
    if(!Invert(base,invBase))return nullptr;
    const Mat4 relative=Multiply(invBase,native);
    // Current native entity + exact wheel draw identify the instance. Another
    // car's identical wheel mesh must never acquire or receive this transform.
    if(Length({relative.m[3],relative.m[7],relative.m[11]})>.12||
       std::fabs(relative.m[1])>.04||std::fabs(relative.m[5]-1)>.04||std::fabs(relative.m[9])>.04||
       std::fabs(std::hypot(relative.m[0],relative.m[8])-1)>.04){CarReject352(model,"entity/rest mismatch",&native);
        static unsigned bindRows[5]={};if(bindRows[model]++<6)Log("[CAR352 BIND] vehicle=%llu model=%s expectedOrigin=%.5f/%.5f/%.5f relativeOrigin=%.5f/%.5f/%.5f relativeAxisY=%.5f/%.5f/%.5f nativeBasisX=%.5f/%.5f/%.5f",
            vehicle.vehicleId,asset.name,base.m[3],base.m[7],base.m[11],relative.m[3],relative.m[7],relative.m[11],relative.m[1],relative.m[5],relative.m[9],native.m[0],native.m[4],native.m[8]);
        return nullptr;}
    Mat4 toView={},fromView={};if(!DrawSpace(context,nullptr,toView,fromView,3,true)){CarReject352(model,"main view",&native);return nullptr;}
    if(g_car352.vehicle!=vehicle.vehicleId||g_car352.model!=model){g_car352={};g_car352.vehicle=vehicle.vehicleId;g_car352.model=model;g_car352.angle=g_car352.target363=std::atan2(relative.m[8],relative.m[0]);g_carFrame352=-100;}
    g_carNativeAngle363=std::atan2(relative.m[8],relative.m[0]);g_carSeen352=GetTickCount64();
    CameraSummary camera363={};ID3D11Buffer* cameraCb363=nullptr;context->VSGetConstantBuffers(1,1,&cameraCb363);UINT cameraBytes363=sizeof(camera363);const bool cameraValid363=cameraCb363&&SUCCEEDED(cameraCb363->GetPrivateData(kCameraSummaryGuid,&cameraBytes363,&camera363))&&cameraBytes363==sizeof(camera363);SafeRelease(cameraCb363);
    if(cameraValid363)TickCarWheel363(camera363);
    if(g_car352.vehicle!=vehicle.vehicleId||g_car352.model!=model||g_car352.owner<0)return nullptr;
    // The native animation is ignored while grabbed. Rotate a fixed rest node
    // about its actual axle, carried by the current vehicle pose each draw.
    Mat4 turn={};turn.m[0]=turn.m[10]=std::cos(g_car352.angle);turn.m[2]=-std::sin(g_car352.angle);turn.m[8]=std::sin(g_car352.angle);turn.m[5]=turn.m[15]=1;
    const V3 moved=Apply(turn,asset.hub);turn.m[3]=asset.hub.x-moved.x;turn.m[7]=asset.hub.y-moved.y;turn.m[11]=asset.hub.z-moved.z;
    const Mat4 controlled=Multiply(base,turn);
    for(int r=0;r<3;++r)for(int k=0;k<4;++k)globals[off+r*4+k]=float(controlled.m[r*4+k]-(k==3&&fractions?globals[off+12+r]:0));
    int slot=-1;for(int i=0;i<8;++i)if(g_carCbBytes352[i]==bytes){slot=i;break;}
    if(slot<0)for(int i=0;i<8;++i)if(!g_carCb352[i]){D3D11_BUFFER_DESC d={};d.ByteWidth=bytes;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(SUCCEEDED(g_oCreateBuffer(g_dev,&d,nullptr,&g_carCb352[i]))){slot=i;g_carCbBytes352[i]=bytes;}break;}
    if(slot<0){DropCar352("no constant buffer slot");return nullptr;}
    const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_carCb352[slot],0,nullptr,globals,0,0);g_own=own;
    return g_carCb352[slot];
}
#include "handlebar355.inl"
// Free car hands are independent of wheel visibility, wheel acquisition,
// steering enablement and analog input. Only the native driver context owns them.
bool CarHandsAllowed353(fc3aim::VehicleSample170& vehicle,fc3pad::Controllers& c){
    c=fc3pad::Latest();
    return g_enabled&&g_headTracking&&g_eyeViewsSet&&g_inGameplay&&!g_flatViewActive&&c.active&&
        !fc3pad::NativeMenuOpen()&&GetTickCount64()-g_physicalAt352<120&&
        fc3aim::GetVehicleSample170(vehicle)&&vehicle.identityValid&&vehicle.seatValid&&vehicle.actualSeat==1&&(vehicle.type==0||HandlebarDriver355(vehicle));
}
#include "car_hand_pose354.inl"
#include "car_grip367.inl"
ID3D11Buffer* BuildCarArm352(ID3D11DeviceContext* context,int side){
    fc3aim::VehicleSample170 vehicle={};fc3pad::Controllers controls={};
    if(side<0||side>1||!CarHandsAllowed353(vehicle,controls)||!g_physicalValid303[side]||!g_armPaletteCb[side])return nullptr;
    float bones[48*12]={},original[25*12]={},neutral367[25*12]={};Mat4 toView={},fromView={};
    if(!DrawSpace(context,bones,toView,fromView,3,true))return nullptr;
    CameraSummary armCamera363={};ID3D11Buffer* armCb363=nullptr;context->VSGetConstantBuffers(1,1,&armCb363);UINT armBytes363=sizeof(armCamera363);if(armCb363&&SUCCEEDED(armCb363->GetPrivateData(kCameraSummaryGuid,&armBytes363,&armCamera363))&&armBytes363==sizeof(armCamera363)){TickCarWheel363(armCamera363);TickHandlebars355(armCamera363);}SafeRelease(armCb363);
    memcpy(original,bones,sizeof(original));CaptureCarLeft354(vehicle.vehicleId,side,toView,bones);CaptureBarHand355(vehicle.vehicleId,side,toView,bones);const Mat4 head=CarHeadView352();
    ArmJoints joints={};joints.valid=true;
    for(int k=0;k<3;++k){const double f=side==1&&k==0?-1:1;joints.s[k]=f*kNeutralArmJoints[0][k];joints.e[k]=f*kNeutralArmJoints[0][3+k];joints.w[k]=f*kNeutralArmJoints[0][6+k];}
    joints=TransformArmJoints(joints,head);
    for(int b=0;b<25;++b){Mat4 n=BoneMatrix(kNeutralArmView[0],b);
        if(side==1)for(int r=0;r<3;++r)for(int k=0;k<4;++k)n.m[r*4+k]*=(r==0?-1:1)*(k==0?-1:1);
        StoreBone(bones,b,Multiply(fromView,Multiply(head,n)));}
    memcpy(neutral367,bones,sizeof(neutral367));
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
    // Final visible grip constraint AFTER native/fallback fingers are posed.
    // The earlier controller/wrist fit is not evidence of mesh contact.
    if(CarAttached354(vehicle.vehicleId,side)){
        Mat4 wheelToBody367={};if(!Invert(g_carBodyToWheel353,wheelToBody367))return nullptr;
        const auto& a=kCarAssets352[g_car352.model];const double polar=g_car352.angle+g_car352.gripOffset[side];
        const V3 target=Apply(head,Apply(wheelToBody367,Add(a.hub,{a.radius*std::cos(polar),0,a.radius*std::sin(polar)})));
        const V3 current=Apply(toView,CarFistCenter367(bones,side));const V3 shift=Sub(target,current);
        handMove.m[3]+=shift.x;handMove.m[7]+=shift.y;handMove.m[11]+=shift.z;
        Mat4 viewShift={};viewShift.m[0]=viewShift.m[5]=viewShift.m[10]=viewShift.m[15]=1;viewShift.m[3]=shift.x;viewShift.m[7]=shift.y;viewShift.m[11]=shift.z;
        // Move hand and its already-curled fingers together, never independently.
        for(int b=0;b<25;++b)if(ArmGroup(b)==2)StoreBone(bones,b,Multiply(fromView,Multiply(viewShift,Multiply(toView,BoneMatrix(bones,b)))));
        g_carBodyView352=head;g_carArmScope352=true;
        const bool solved367=SolveArm(joints,handMove,side,forearm,upper,true,true);g_carArmScope352=false;if(!solved367)return nullptr;
    }
    // Apply twist to the two skin bones; the old vehicle path left both rigid,
    // concentrating the entire hand roll at the wrist seam.
    Mat4 twist367[2]={forearm,forearm};if(vehicle.type==0)CarForearmTwist367(joints,handMove,forearm,side,twist367);
    for(int b=0;b<25;++b){
        if(ArmGroup(b)!=2){const Mat4& m=b==20?twist367[0]:b==21?twist367[1]:ArmGroup(b)==0?upper:forearm;
            StoreBone(bones,b,Multiply(fromView,Multiply(m,Multiply(toView,BoneMatrix(neutral367,b)))));}
        Mat4 inv={};if(Invert(Multiply(toView,BoneMatrix(original,b)),inv))g_armViewMoves[side][b]=Multiply(Multiply(toView,BoneMatrix(bones,b)),inv);
    }
    Mat4 inverseHead={};if(Invert(head,inverseHead)){
        for(int b=0;b<25;++b)g_armFinalBody[side][b]=Multiply(inverseHead,Multiply(toView,BoneMatrix(bones,b)));
        g_armFinalFrame[side]=g_frame;
    }
    if(CarAttached354(vehicle.vehicleId,side)){
        static ULONGLONG next367[2]={};const auto now=GetTickCount64();if(now>=next367[side]){next367[side]=now+1000;
            const V3 grip=Apply(g_carBodyToWheel353,Apply(inverseHead,Apply(toView,CarFistCenter367(bones,side))));
            const auto& a=kCarAssets352[g_car352.model];const double polar=g_car352.angle+g_car352.gripOffset[side];
            Log("[CAR367 GRIP] side=%d posedFingerCenterError=%.6fm radius=%.4f depth=%.4f twistBones=20/21",side,Length(Sub(grip,Add(a.hub,{a.radius*cos(polar),0,a.radius*sin(polar)}))),std::hypot(grip.x-a.hub.x,grip.z-a.hub.z),grip.y-a.hub.y);
        }
    }
    memcpy(g_armOriginal[side],original,sizeof(original));g_armMovesFrame[side]=g_frame;
    const bool own=g_own;g_own=true;g_oUpdateSubresource(context,g_armPaletteCb[side],0,nullptr,bones,0,0);g_own=own;
    return g_armPaletteCb[side];
}

// Exact immutable Jason geometry selects the anatomical side. Measuring the
// animated wrist/elbow pose is neither needed nor reliable in a car.
int CarArmSide353(const fc3weapon::Geometry& g){
    if(g.stride!=40||g.offset||g.format!=57)return -1;
    if(g.vertexBytes==316240&&g.indexBytes==78330&&g.count==10443){
        if(g.start==6861)return 1;if(g.start==17304)return 0;
    }
    if(g.vertexBytes==320760&&g.indexBytes==79254){
        if(g.start==19635&&g.count==9549)return 1;
        if(g.start==29184&&g.count==10443)return 0;
    }
    return -1;
}
ID3D11Buffer* BuildCarArmDraw353(ID3D11DeviceContext* context,const DrawCall& call,bool& handled){
    handled=false;
    if(call.kind!=0||call.d||!((call.b==6861&&call.a==10443)||(call.b==17304&&call.a==10443)||
        (call.b==19635&&call.a==9549)||(call.b==29184&&call.a==10443)))return nullptr;
    const int side=CarArmSide353(NativeWeaponGeometry(context,call));if(side<0)return nullptr;
    fc3aim::VehicleSample170 vehicle={};fc3pad::Controllers c={};if(!CarHandsAllowed353(vehicle,c))return nullptr;
    handled=true;
    ID3D11Buffer* palette=BuildCarArm352(context,side);
    static unsigned rows[2]={};if(rows[side]++<12){
        ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);const auto hash=ShaderHash(shader);SafeRelease(shader);
        Log("[CAR353 ARMS] actual draw vehicle=%llu side=%d start=%u count=%u shader=%016llX tracked=%d palette=%d independentOfWheel=1",
            vehicle.vehicleId,side,call.b,call.a,hash,g_physicalValid303[side],palette!=nullptr);
    }
    return palette;
}
