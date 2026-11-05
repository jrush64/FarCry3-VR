// Targeted control of the known shotgun hand poses. Source palettes remain untouched.
#include "captured_shell_grip.inl"
struct ShotHandControl
{
    Mat4 idle={},current={},previous={};
    LONG frame=-100000,shot=-100000;UINT epoch=0;
    int quiet=0;bool valid=false,active=false,departed=false,logged=false,reloadLogged=false;
    V3 leftRel={};bool leftValid=false,leftLogged=false; // native left wrist in the native right-hand frame, learned while idle
    LONG requests=0;
} g_shotHand;
// PID63244 (M133, candidate 105): the native sprint carry pose is steady enough to pass the
// quiet test, so the idle grip was relearned mid-sprint (render CSV 6.0-7.8 m/s; running is
// 3.4-4.3 m/s). The shot at frame 8373 fired mid-sprint then pinned the gun to that carry pose
// ("compensated 44.6 degrees" once the player stopped) while the camera, and with it the shot,
// kept following the controller; the release waited for the hand to come back within 4 degrees
// of the carry pose, which never happens on foot. Sprinting now blocks the idle relearn, and a
// correction whose native hand has settled at a different stance releases once chambered.
constexpr double kShotHandSprintSpeed107=5.0;     // m/s, horizontal camera speed
constexpr double kShotHandSprintHold107=.3;       // s the sprint flag outlives the last fast frame
constexpr LONG kShotHandSettleFrames107=45;       // 0.75 s settled away from the idle grip
// Candidate 111: run 45704 (candidate 110) never logged a sprint. 107 sampled the camera at the first
// shot-hand observation of a frame, which comes from the first-person draws before any world draw,
// while g_frameCameraValid is still false after the previous Present: every frame read "no camera" and
// the detector never held two samples. The camera is now sampled at Present (TrackShotHandMotion111).
// The idle grip learned in the last 0.75 s before a sprint starts is also discarded: the carry pose
// settles during the run-up (render CSV 4.0 -> 4.7 -> 8.2 m/s, frames 9032-9053) before 5 m/s.
constexpr double kShotHandRampRevert111=.75;     // s before sprint onset
struct ShotHandMotion107 {LONG frame=-100000,settleFrame=-100000,settled=0,quietFrame=-100000;double seconds=0,until=-1,speed=0,stretchSeconds=-100;float position[3]={};bool valid=false,sprinting=false,stretch=false,prevValid=false,prevLeftValid=false;Mat4 prevIdle={};V3 prevLeftRel={};} g_shotHandMotion107;
double g_shotHandClockForTest107=-1;
LONG g_shotHandSprintLogs107=0,g_shotHandSettleLogs107=0;
double ShotHandSeconds107()
{
    if(g_shotHandClockForTest107>=0)return g_shotHandClockForTest107;
    LARGE_INTEGER now={},frequency={};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart?double(now.QuadPart)/double(frequency.QuadPart):0;
}
LONG g_shotHandRevertLogs111=0;
double HandAngle(const Mat4&,const Mat4&);
void ShotHandSprintOnset111(double now)
{
    auto& m=g_shotHandMotion107;
    if(g_shotHand.active||!g_shotHand.valid||!m.stretch||now-m.stretchSeconds>kShotHandRampRevert111)return;
    if(g_shotHandRevertLogs111++<8)Log("[HAND CONTROL] sprint started %.2f s after the idle grip was learned: that pose (%.1f degrees from the one before) is discarded as the run-up carry pose",now-m.stretchSeconds,m.prevValid?HandAngle(m.prevIdle,g_shotHand.idle):-1.0);
    g_shotHand.idle=m.prevIdle;g_shotHand.valid=m.prevValid;g_shotHand.leftRel=m.prevLeftRel;g_shotHand.leftValid=m.prevLeftValid;g_shotHand.quiet=0;m.stretch=false;
}
// Once per frame at Present, where this frame's world camera is known.
void TrackShotHandMotion111()
{
    auto& m=g_shotHandMotion107;const double now=ShotHandSeconds107();const float* p=g_frameCamera.position;
    if(!g_frameCameraValid||!std::isfinite(p[0])||!std::isfinite(p[1])){m.valid=false;return;} // a gap: the next sample restarts
    if(m.valid)
    {
        const double dt=now-m.seconds;if(dt<1e-3)return;
        if(dt<.5){m.speed=std::hypot(double(p[0])-m.position[0],double(p[1])-m.position[1])/dt;if(m.speed>kShotHandSprintSpeed107){if(now>=m.until)ShotHandSprintOnset111(now);m.until=now+kShotHandSprintHold107;}}
    }
    memcpy(m.position,p,sizeof(m.position));m.seconds=now;m.valid=true;m.frame=g_frame;
}
bool ShotHandSprinting107(){auto& m=g_shotHandMotion107;m.sprinting=ShotHandSeconds107()<m.until;return m.sprinting;}
struct ReloadHandContact
{
    Mat4 view={},portParent={};V3 shellBody={},portBody={};
    LONG handFrame=-100000,portFrame=-100000;
    bool hand=false,port=false,logged=false,mirrored318=false;
} g_reloadContact;
Mat4 HandIdentity(){Mat4 m={};m.m[0]=m.m[5]=m.m[10]=m.m[15]=1;return m;}
bool HandBasis(const Mat4& source,Mat4& out)
{
    for(int r=0;r<3;++r)for(int c=0;c<3;++c)if(!std::isfinite(source.m[r*4+c]))return false;
    V3 x={source.m[0],source.m[4],source.m[8]},y={source.m[1],source.m[5],source.m[9]},z={source.m[2],source.m[6],source.m[10]};
    if(Length(x)<.1||Length(y)<.1||Length(z)<.1)return false;
    x=Unit(x);y=Sub(y,Scale(x,Dot(x,y)));if(Length(y)<.1)return false;y=Unit(y);
    V3 normal=Cross(x,y);normal=Scale(normal,Dot(normal,z)<0?-1.:1.);out=HandIdentity();
    for(int r=0;r<3;++r){out.m[r*4]=r==0?x.x:r==1?x.y:x.z;out.m[r*4+1]=r==0?y.x:r==1?y.y:y.z;out.m[r*4+2]=r==0?normal.x:r==1?normal.y:normal.z;}
    return true; // retain reflected camera bases; only their relative rotation is proper
}
double HandAngle(const Mat4& a,const Mat4& b)
{
    Mat4 inverse={};if(!Invert(a,inverse))return 180;
    const Mat4 delta=Multiply(inverse,b);return std::acos((std::max)(-1.,(std::min)(1.,(delta.m[0]+delta.m[5]+delta.m[10]-1)*.5)))*180/3.141592653589793;
}
bool ShotgunHandContext()
{
    return MotionInteractionEnabled()&&g_eyeViewsSet&&g_handRightValid&&fc3pad::Latest().active&&g_frame-g_reload.shotgunFrame<=2;
}
// Stable native attachment measured over 2620 same-frame gun/hand pairs.
const Mat4 kShotgunHandFromGun={{0.71591069204289,-0.0072893059582326,0.69816018479635,-1.1902666380758,0.65307041516379,0.3606497453857,-0.66590594253478,0.30517604790249,-0.24693843269692,0.93267259699893,0.26295209176542,-0.14775823709756,0,0,0,1}};
void ObserveNativeShotBasis(const Mat4& nativeBasis)
{
    const bool first=g_shotHand.frame<0;
    const double speed=first?180:HandAngle(g_shotHand.current,nativeBasis);
    g_shotHand.previous=g_shotHand.current;g_shotHand.current=nativeBasis;g_shotHand.frame=g_frame;
    if(g_lastShotFrame>=0&&g_lastShotFrame!=g_shotHand.shot)
    {g_shotHand.shot=g_lastShotFrame;g_shotHand.active=g_shotHand.valid;g_shotHand.departed=false;g_shotHand.logged=false;}
    const auto controls=fc3pad::AmmoControls();
    const bool nativeReload=ReloadPoseActive();
    if(!nativeReload)g_shotHand.reloadLogged=false;
    if(controls.x||nativeReload||g_reload.requests!=g_shotHand.requests){g_shotHand.active=false;g_shotHand.quiet=0;}
    g_shotHand.requests=g_reload.requests;
    if(g_shotHand.active&&HandAngle(g_shotHand.idle,nativeBasis)>8)g_shotHand.departed=true;
    if(g_shotHand.active&&g_shotHand.departed&&g_chambered&&HandAngle(g_shotHand.idle,nativeBasis)<4){g_shotHand.active=false;g_shotHand.quiet=0;}
    const bool sprinting=ShotHandSprinting107();
    auto& motion=g_shotHandMotion107;
    // A different native stance that has settled (not the shot or pump animation, which move)
    // with the chamber ready: keeping the correction would pin the gun to an old pose forever.
    if(g_shotHand.active&&g_shotHand.departed&&g_chambered&&!sprinting&&!first&&speed<1.5&&HandAngle(g_shotHand.idle,nativeBasis)>8)
    {
        if(motion.settleFrame!=g_frame){motion.settleFrame=g_frame;++motion.settled;}
        if(motion.settled>=kShotHandSettleFrames107)
        {
            if(g_shotHandSettleLogs107++<8)Log("[HAND CONTROL] native hand settled %.1f degrees from the idle grip for %ld frames with the chamber ready; shot correction released, idle relearns",HandAngle(g_shotHand.idle,nativeBasis),static_cast<long>(motion.settled));
            g_shotHand.active=false;g_shotHand.quiet=0;motion.settled=0;
        }
    }
    else if(motion.settleFrame!=g_frame){motion.settleFrame=g_frame;motion.settled=0;}
    BYTE pending=0;const bool nativePending=g_reload.ready&&AmmoRead(g_reload.state+8,&pending,1)&&pending!=0;
    if(!g_shotHand.active&&g_chambered&&!g_reload.mode&&!controls.x&&!nativePending&&!nativeReload&&sprinting)
    {
        if(speed<1.5&&g_shotHandSprintLogs107++<8)Log("[HAND CONTROL] sprinting %.1f m/s: idle grip kept, the sprint carry pose is not learned as the shot-hand idle",motion.speed);
        g_shotHand.quiet=0;
    }
    else if(!g_shotHand.active&&g_chambered&&!g_reload.mode&&!controls.x&&!nativePending&&!nativeReload)
    {
        // Quiet FRAMES, not observations: the arm draw and up to four weapon-root draws observe
        // every frame, so six observations could pass in two frames.
        if(speed>=1.5)g_shotHand.quiet=0;else if(motion.quietFrame!=g_frame){motion.quietFrame=g_frame;++g_shotHand.quiet;}
        if(g_shotHand.quiet>=6){
            if(!motion.stretch||!g_shotHand.valid||HandAngle(g_shotHand.idle,nativeBasis)>3)
            {motion.prevIdle=g_shotHand.idle;motion.prevValid=g_shotHand.valid;motion.prevLeftRel=g_shotHand.leftRel;motion.prevLeftValid=g_shotHand.leftValid;motion.stretch=true;motion.stretchSeconds=ShotHandSeconds107();}
            g_shotHand.idle=nativeBasis;g_shotHand.valid=true;
            const ArmJoints& r=g_armJoints[0];const ArmJoints& l=g_armJoints[1];
            if(r.valid&&l.valid){const V3 d={l.w[0]-r.w[0],l.w[1]-r.w[1],l.w[2]-r.w[2]};g_shotHand.leftRel={nativeBasis.m[0]*d.x+nativeBasis.m[4]*d.y+nativeBasis.m[8]*d.z,nativeBasis.m[1]*d.x+nativeBasis.m[5]*d.y+nativeBasis.m[9]*d.z,nativeBasis.m[2]*d.x+nativeBasis.m[6]*d.y+nativeBasis.m[10]*d.z};
             if(Length(g_shotHand.leftRel)>.1&&Length(g_shotHand.leftRel)<1.){if(!g_shotHand.leftLogged){Log("[HAND CONTROL] idle support-hand offset learned (%.3f m from the right wrist); two-handed gun aim and left hand ignore the native shot/pump animation",Length(g_shotHand.leftRel));g_shotHand.leftLogged=true;}g_shotHand.leftValid=true;}}}
    }
    else{g_shotHand.quiet=0;motion.stretch=false;}
}
void ObserveShotHand(const float* bones,const float* globals,const CameraSummary& camera,const ArmJoints&)
{
    if(!ShotgunHandContext()){g_shotHand=ShotHandControl{};return;}
    if(g_shotHand.epoch!=g_armPoseEpoch){g_shotHand=ShotHandControl{};g_shotHand.epoch=g_armPoseEpoch;}
    Mat4 nativeBasis={};
    const Mat4 model=Multiply(HullView(camera),HullWorld(globals));
    // Native animation lives in the first-person camera frame. Removing Q here
    // mixes controller-driven game aiming into the measured animation.
    if(!HandBasis(Multiply(model,BoneMatrix(bones,2)),nativeBasis))return;
    ObserveNativeShotBasis(nativeBasis);
    // Use the freshly measured right wrist for both arm and weapon draws.
    g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);
}
// Where the native LEFT wrist would be if the hands stayed in their idle grip:
// right wrist + current native right-hand basis x learned idle offset. PID57824
// (signature 870, two-handed): the native pump/shot animation swings the left
// wrist; aiming the barrel at it shook the gun and the left hand replayed it.
bool ShotgunSupportNative(const ArmJoints& right,V3& out)
{
    if(!ShotgunHandContext()||!g_shotHand.valid||!g_shotHand.leftValid||g_shotHand.epoch!=g_armPoseEpoch||g_frame-g_shotHand.frame>1||!right.valid)return false;
    const Mat4& b=g_shotHand.current;const V3& l=g_shotHand.leftRel;
    out={right.w[0]+b.m[0]*l.x+b.m[1]*l.y+b.m[2]*l.z,right.w[1]+b.m[4]*l.x+b.m[5]*l.y+b.m[6]*l.z,right.w[2]+b.m[8]*l.x+b.m[9]*l.y+b.m[10]*l.z};
    return true;
}
void ObserveShotgunRoot(const float* bones,const Mat4& toView,UINT start,UINT count)
{
    if(!ShotgunHandContext()||g_reload.shotgunFrame!=g_frame)return; // two-handed too: the native shot animation is the same
    if(!((start==486&&count==4071)||(start==4557&&count==5076)||(start==9633&&count==6795)||(start==16428&&count==7311)))return;
    if(g_shotHand.epoch!=g_armPoseEpoch){g_shotHand=ShotHandControl{};g_shotHand.epoch=g_armPoseEpoch;}
    const Mat4 hand=Multiply(toView,Multiply(BoneMatrix(bones,0),kShotgunHandFromGun));Mat4 nativeBasis={};
    if(!HandBasis(hand,nativeBasis))return;
    ObserveNativeShotBasis(nativeBasis);
    ArmJoints predicted=g_armJoints[0];const V3 wrist=Apply(hand,{.591,.011,1.149});
    for(int r=0;r<3;++r){const double point=r==0?wrist.x:r==1?wrist.y:wrist.z;predicted.e[r]+=point-predicted.w[r];predicted.w[r]=point;}
    g_frameGunValid=GunMove(predicted,g_armJoints[1],g_frameGunMove);
}
void CorrectShotHandMove(const ArmJoints& joints,Mat4& move)
{
    if(!ShotgunHandContext()||!g_shotHand.valid||g_frame-g_shotHand.frame>1)return; // two-handed: called before the support turn (GunMove)
    const bool manualReload=g_manualReloadEnabled&&ReloadPoseActive();
    // Same wrist-pivot rotation correction as firing. Preserve native child
    // animation and ammo events; configured X reload retains native posing.
    if(!manualReload&&(!g_shotHand.active||fc3pad::AmmoControls().x||ReloadPoseActive()))return;
    Mat4 inverse={};
    if(!Invert(g_shotHand.current,inverse))return;
    Mat4 correction=Multiply(g_shotHand.idle,inverse);
    const V3 wrist={joints.w[0],joints.w[1],joints.w[2]},turned=Apply(correction,wrist);
    correction.m[3]=wrist.x-turned.x;correction.m[7]=wrist.y-turned.y;correction.m[11]=wrist.z-turned.z;
    move=Multiply(move,correction); // orientation only, pivot stays at the existing wrist
    if(manualReload)
    {
        if(!g_shotHand.reloadLogged&&HandAngle(g_shotHand.idle,g_shotHand.current)>8)
        {Log("[HAND CONTROL] manual reload hand tilt compensated %.1f degrees; wrist and weapon child animation retained",HandAngle(g_shotHand.idle,g_shotHand.current));g_shotHand.reloadLogged=true;}
    }
    else if(!g_shotHand.logged&&HandAngle(g_shotHand.idle,g_shotHand.current)>8)
    {Log("[HAND CONTROL] one-hand shot rotation compensated %.1f degrees; wrist translation retained",HandAngle(g_shotHand.idle,g_shotHand.current));g_shotHand.logged=true;}
}
bool CapturedReloadGrip(ID3D11DeviceContext* context,float* bones)
{
    if(!g_reload.held||!g_reload.mode||g_twoHand||g_frame-g_reload.shotgunFrame>2)return false;
    ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);
    D3D11_BUFFER_DESC desc={};if(vb)vb->GetDesc(&desc);SafeRelease(vb);
    if(desc.ByteWidth!=316240||stride!=40||offset!=0)return false;
    const Mat4 hand=BoneMatrix(bones,2);
    for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(hand,BoneMatrix(kCapturedGripRelative,b-3)));
    return true; // no CurlFingers pass over the captured held-shell pose
}
bool HandDrawView(ID3D11DeviceContext* context,const float* bones,int bone,Mat4& matrix)
{
    ID3D11Buffer* cb[2]={};context->VSGetConstantBuffers(0,2,cb);float globals[88]={};CameraSummary camera={};UINT size=sizeof(camera);
    const bool ok=ReadCopy(cb[0],globals,sizeof(globals))>=80&&cb[1]&&SUCCEEDED(cb[1]->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)&&camera.mainView;
    for(auto*& b:cb)SafeRelease(b);if(!ok)return false;
    matrix=Multiply(HullView(camera),Multiply(HullWorld(globals),BoneMatrix(bones,bone)));return true;
}
void RecordReloadHand(ID3D11DeviceContext* context,const float* bones)
{
    if(!g_reload.held||!g_reload.mode){g_reloadContact.hand=false;return;}
    Mat4 view={},bodyInverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),bodyInverse))return;
    const V3 centre=Apply(Multiply(view,kCapturedShellToHand),kCapturedShellCentre);
    g_reloadContact.mirrored318=false;g_reloadContact.view=view;g_reloadContact.shellBody=Apply(bodyInverse,centre);g_reloadContact.handFrame=g_frame;g_reloadContact.hand=true;
    if(g_manualReloadEnabled)ManualMarker();
    if(!g_reloadContact.logged){Log("[HAND CONTROL] captured native shell grip and shell-to-hand attachment active (frame 7042)");g_reloadContact.logged=true;}
}
void RecordReloadPort(ID3D11DeviceContext* context,const float* bones,UINT start,UINT count)
{
    if(start!=16428||count!=7311||g_reload.shotgunFrame!=g_frame)return;
    Mat4 view={},bodyInverse={};if(!HandDrawView(context,bones,0,view)||!Invert(ArmBodyToCamera(),bodyInverse))return;
    const V3 insert=g_manualReloadEnabled?V3{g_manualSettings.port[0],g_manualSettings.port[1],g_manualSettings.port[2]}:kCapturedInsertPoint;
    g_reloadContact.portParent=Multiply(bodyInverse,view);g_reloadContact.portBody=Apply(g_reloadContact.portParent,insert);g_reloadContact.portFrame=g_frame;g_reloadContact.port=true;
    if(g_manualReloadEnabled)ManualMarker();
}
bool ReloadContactDistance(double& distance)
{
    if(!g_reloadContact.hand||!g_reloadContact.port||g_frame-g_reloadContact.handFrame>2||g_frame-g_reloadContact.portFrame>2)return false;
    distance=Length(Sub(g_reloadContact.shellBody,g_reloadContact.portBody));return std::isfinite(distance);
}
bool CapturedShellTarget(Mat4& target)
{
    if(!g_reload.held||!g_reloadContact.hand||g_frame-g_reloadContact.handFrame>2)return false;
    target=Multiply(g_reloadContact.view,kCapturedShellToHand);if(g_reloadContact.mirrored318){Mat4 m=HandIdentity();m.m[0]=-1;target=Multiply(target,m);}return true;
}
