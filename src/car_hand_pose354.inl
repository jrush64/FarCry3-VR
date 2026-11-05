// Native LEFT driving grip, captured before the mod's car-arm palette replaces it.
// Used only while attached to the wheel; free tracking/steering stay in353.
struct CarHandPose354 {
    unsigned long long vehicle=0;unsigned epoch=0;int model=-1;
    bool fingers=false,hand=false;
    Mat4 finger[17]={},candidate[17]={},canonicalLeft={};
    LONG frame=-100;unsigned quiet=0;
} g_carHand354;
Mat4 CarRimTurn354(double angle){
    Mat4 m={};m.m[0]=m.m[10]=std::cos(angle);m.m[2]=-std::sin(angle);m.m[8]=std::sin(angle);m.m[5]=m.m[15]=1;return m;
}
bool CarAttached354(unsigned long long vehicle,int side){
    return side>=0&&side<2&&g_car352.vehicle==vehicle&&g_car352.model>=0&&g_car352.gripping[side]&&GetTickCount64()-(g_carBodyAt363?g_carBodyAt363:g_carSeen352)<120;
}
void CaptureCarLeft354(unsigned long long vehicle,int side,const Mat4& toView,const float* raw){
    if(side!=1)return; // never sample the right hand resting on the thigh
    auto& p=g_carHand354;
    if(p.vehicle!=vehicle||p.epoch!=g_armPoseEpoch){p={};p.vehicle=vehicle;p.epoch=g_armPoseEpoch;}
    if(p.hand||g_car352.vehicle!=vehicle||g_car352.model<0||GetTickCount64()-g_carSeen352>=120)return;
    Mat4 inverseWrist={};if(!BoneUsable(raw+24)||!Invert(BoneMatrix(raw,2),inverseWrist))return;
    Mat4 relative[17]={};
    for(int b=3;b<=19;++b){if(!BoneUsable(raw+b*12))return;
        relative[b-3]=Multiply(inverseWrist,BoneMatrix(raw,b));for(double x:relative[b-3].m)if(!std::isfinite(x))return;}
    Mat4 inverseHead={};if(!Invert(CarHeadView352(),inverseHead))return;
    const Mat4 native=Multiply(g_carBodyToWheel353,Multiply(inverseHead,Multiply(toView,BoneMatrix(raw,2))));
    const auto& asset=kCarAssets352[g_car352.model];
    const V3 palm=Sub(Apply(native,{-.6781,.0519,1.0387}),asset.hub);
    const double radius=std::hypot(palm.x,palm.z);
    if(!std::isfinite(radius)||std::fabs(radius-asset.radius)>.13||std::fabs(palm.y)>.15)return;
    Mat4 centered=native;centered.m[3]-=asset.hub.x;centered.m[7]-=asset.hub.y;centered.m[11]-=asset.hub.z;
    const Mat4 canonical=Multiply(CarRimTurn354(-std::atan2(palm.z,palm.x)),centered);
    for(double x:canonical.m)if(!std::isfinite(x))return;
    if(p.frame==g_frame)return; // multiple passes/eyes are not independent samples
    bool quiet=p.quiet&&g_frame-p.frame<=2;
    for(int b=0;b<17&&quiet;++b)for(int i=0;i<12;++i)if(std::fabs(relative[b].m[i]-p.candidate[b].m[i])>.012){quiet=false;break;}
    p.frame=g_frame;p.quiet=quiet?p.quiet+1:1;memcpy(p.candidate,relative,sizeof(relative));
    if(p.quiet<6)return; // do not freeze the transitional car-entry/open-hand pose
    memcpy(p.finger,relative,sizeof(relative));p.fingers=true;
    p.canonicalLeft=canonical;p.model=g_car352.model;p.hand=true;
    Log("[CAR354 POSE] native left-wheel wrist fit captured vehicle=%llu model=%d palmRadius=%.4f depth=%.4f",vehicle,p.model,radius,palm.y);
}
// 365: wrist offset from the rim centreline at a given rim angle (wheel space).
V3 HandRimOffset365(double polar,int side){
    const int h=side==1?1:0;const double out=g_handRimOutCm366[h]*.01,pull=g_handRimPullCm366[h]*.01;
    return {out*std::cos(polar),-pull,out*std::sin(polar)};
}
bool CarGripMove354(unsigned long long vehicle,int side,const Mat4& neutralView,Mat4& move){
    const auto& p=g_carHand354;
    if(!CarAttached354(vehicle,side)||!p.hand||p.vehicle!=vehicle||p.epoch!=g_armPoseEpoch||p.model!=g_car352.model)return false;
    Mat4 wheelToBody={},inverseNeutral={};if(!Invert(g_carBodyToWheel353,wheelToBody)||!Invert(neutralView,inverseNeutral))return false;
    Mat4 canonical=p.canonicalLeft;
    if(side==0){ // reflect wheel X and bind-pose X, then return to the same rim-angle reference
        Mat4 mirror={};mirror.m[0]=-1;mirror.m[5]=mirror.m[10]=mirror.m[15]=1;
        canonical=Multiply(CarRimTurn354(-fc3wheel352::Pi),Multiply(mirror,Multiply(canonical,mirror)));
    }
    const double polar=g_car352.angle+g_car352.gripOffset[side];
    Mat4 desired=Multiply(CarRimTurn354(polar),canonical);
    const auto& hub=kCarAssets352[p.model].hub;desired.m[3]+=hub.x;desired.m[7]+=hub.y;desired.m[11]+=hub.z;
    // Native wrist orientation/fingers are retained, but the accepted grasp
    // is on the actual controlled rim, not on an offset native animation.
    const V3 contact=Apply(desired,{side?-.6781:.6781,.0519,1.0387});
    const double depth=(std::max)(-.025,(std::min)(.025,contact.y-hub.y));
    const V3 target=Add(Add(hub,{kCarAssets352[p.model].radius*std::cos(polar),depth,kCarAssets352[p.model].radius*std::sin(polar)}),HandRimOffset365(polar,side));
    const V3 correction=Sub(target,contact);desired.m[3]+=correction.x;desired.m[7]+=correction.y;desired.m[11]+=correction.z;
    const Mat4 view=Multiply(CarHeadView352(),Multiply(wheelToBody,desired));
    move=Multiply(view,inverseNeutral);return true;
}
bool CarGripFingers354(unsigned long long vehicle,int side,float* bones){
    const auto& p=g_carHand354;
    if(!CarAttached354(vehicle,side)||!p.fingers||p.vehicle!=vehicle||p.epoch!=g_armPoseEpoch)return false;
    const Mat4 wrist=BoneMatrix(bones,2);
    Mat4 mirror={};mirror.m[0]=-1;mirror.m[5]=mirror.m[10]=mirror.m[15]=1;
    for(int b=3;b<=19;++b){const Mat4 relative=side==1?p.finger[b-3]:Multiply(mirror,Multiply(p.finger[b-3],mirror));
        StoreBone(bones,b,Multiply(wrist,relative));}
    return true;
}
