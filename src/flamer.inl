// Flamethrower (flamethrower.xbg, profile 15). Design: to reload, push the lever, eject the clip by grabbing it
// (no auto drop), grab a new one, insert it, push the lever back up.
//  - Skinned, one world for all seven draws (captures/weapons7-29048, work/flamer_clips.py -> flamer_clips.inl). FRAME = the
//    AnchorBone (bone 11) slot; the CANISTER is bone 13 (draw 13590, slot 0); the lever = crank bone 8 (draw 3630 slot 2) with the
//    hydraulic bone 10 (draw 3630 slot 3) and the HANDLE bone 9 (draw 20838 slot 1). A slot is posed as
//    pal = FRAMEpal x pose(rel FRAME) x T(-bind), FRAMEpal = pal[anchor] x T(bind anchor).
//  - Captured motion: the handle unfolds 48.3 deg, the crank turns 103.3 deg about the canister axis (the handle folds back to 3
//    deg as it goes down); the canister then slides out along kFlamerCanisterOutDir.
//  - LEVER: LEFT grip on the LEVER box (the handle) and push down 15 cm: the lever goes down and stays down. Grip it again and push
//    it up: locked. The left hand sits on the handle (Jason's grip) while you hold it.
//  - CANISTER (lever down only): LEFT grip on the CANISTER box takes it out into the left hand. Let go away from the gun: it falls.
//    New one: LEFT grip + X (or grip at the waist / chest). Bring it to its seat (6 cm): it goes in and the engine's own
//    Begin/transfer/End fill the tank (capacity / reserve checked). The old one put back goes in with no ammo change.
//  - The game's own reload is refused; the trigger is dry while the lever is down or the canister is out.
#include "flamer_clips.inl"
struct Flamer
{
 double lever=0;bool leverDown=false,leverGrab=false,seated=true,carrying=false,carryingOld=false,pending=false,refused=false,falling=false;
 bool known=false,probing=false,leftGripWas=false,xWas=false,xOwned=false,nearBox=false,replaced=false,away=false;
 double leverStart=0,leverStartZ=0,leverGunY=0;V3 leverGrabAt={},leverGrabBody={};ULONGLONG removalBlockedUntil54=0,deadline=0,pulseAt=0,probeAt=0,fallAt=0;
 bool leverPoseKnown=false;Mat4 settledHandle={},settledCrank={},settledHydraulic={};
 Mat4 frameBody={},framePal={};LONG frameFrame=-100000,palFrame=-100000;UINT frameEpoch58=~0u;Mat4 leftWrist={};LONG leftFrame=-100000;Mat4 fallBody={};
 UINT ammoState=0,ammoWrapper=0;LONG gripLogs=0,fills=0;
} g_flamer;
#define kFlamerHand (g_w7Guides[3].hand)
#define kFlamerHandDiameter (g_w7Guides[3].diameter)

const float kFlamerBindAnchor[3]={0.f,.26105f,.08371f},kFlamerBindClip[3]={0.f,.20911f,-.01532f},kFlamerBindCrank[3]={0.f,.32075f,.03955f},kFlamerBindHandle[3]={0.f,.31698f,.059f},kFlamerBindHydraulic[3]={.00604f,.32082f,.03959f};
#define kFlamerLeverBoxSize (g_w7Guides[3].size[0])
#define kFlamerCanisterBoxSize (g_w7Guides[3].size[1])

const double kFlamerLeverStroke=.08,kFlamerSeatReach=.08; // keep the movement simple: an 8 cm push snaps it
bool FlamerModel(){return g_weaponMechanicProfile==15&&g_frame-g_weaponMechanicFrame<=2;}
bool FlamerActive(){return FlamerModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool FlamerFrameReady(){return g_frame-g_flamer.frameFrame<=2;}
// Holding uses the same final receiver FRAME as the accepted reload contacts.
// The generic support path measured an animated native wrist and then used the
// unsupported GunMove basis to draw its cube. Those are two different parents.
struct FlamerSupportAttachment58 {V3 local={};UINT epoch=~0u;bool learned=false;}g_flamerSupport58;
bool FlamerClipPlaying();
bool FlamerSupportFrame58(){return FlamerModel()&&g_flamer.frameEpoch58==g_armPoseEpoch&&g_flamer.frameFrame<=g_frame&&FlamerFrameReady();}
bool FlamerSupportPoints58(const V3& supportCamera,const Mat4& unsupportedMove,V3& point,V3& palm){
 if(!FlamerSupportFrame58()||!g_handLeftValid)return false;
 auto& attachment=g_flamerSupport58;const auto& f=g_flamer;
 if(attachment.epoch!=g_armPoseEpoch){attachment={};attachment.epoch=g_armPoseEpoch;}
 // Captured neutral foregrip, never a current equip/fire/reload wrist.
 attachment.local=Apply(FlareKeyMat(kFlamerLeftSupportFromFrame),{-.6781,.0519,1.0387});attachment.learned=true;
 point=Apply(f.frameBody,OtherSupportPoint344(attachment.local));SupportGrabAdjustPoint214(point);
 if(!FreeHandPalm(1,palm))palm=OtherMotion319(1,ControllerWrist(g_handBody[1],1));
 return true;
}
bool FlamerSupportBox58(Mat4& box){
 if(!FlamerSupportFrame58())return false;
 g_flamerSupport58.local=Apply(FlareKeyMat(kFlamerLeftSupportFromFrame),{-.6781,.0519,1.0387});g_flamerSupport58.epoch=g_armPoseEpoch;g_flamerSupport58.learned=true;
 if(!HandBasis(g_flamer.frameBody,box))return false;V3 p=Apply(g_flamer.frameBody,OtherSupportPoint344(g_flamerSupport58.local));SupportGrabAdjustPoint214(p);box.m[3]=p.x;box.m[7]=p.y;box.m[11]=p.z;return true;
}
float FlamerSupportDiameter58(){const float scale=(std::max)(.1f,g_worldScale);return SupportGrabBoxDiameter214(15)*scale;}
double FlamerSupportDistance58(){
 Mat4 box={};if(!g_handLeftValid||!FlamerSupportBox58(box))return 1e9;V3 palm={};if(!FreeHandPalm(1,palm))palm=OtherMotion319(1,ControllerWrist(g_handBody[1],1));return SupportGrabOutside214(15,box,palm);
}
bool FlamerLeftReady(){return g_frame-g_flamer.leftFrame<=2;}
Mat4 FlamerKeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
Mat4 FlamerTranslate(const float* t,double sign){Mat4 m=HandIdentity();m.m[3]=sign*t[0];m.m[7]=sign*t[1];m.m[11]=sign*t[2];return m;}
// A simple motion still plays the proper lever open/close animation (no instant snap open):
// grip the lever and move the hand 4 cm (any way): Jason's own lever-down (or lever-up) plays from the capture - handle,
// crank, hydraulic, his left wrist and fingers on the handle - and the lever stays there.
struct FlamerLeverClip{int kind=0;double scrub=-1,startScrub=0;ULONGLONG at=0;LONG poseFrame=-100000;Mat4 hand={},handle={},crank={},hydraulic={};float fingers[204]={};} g_flamerClip; // 1 down, 2 up
const double kFlamerDownFrom=kFlamerHandleUnfoldStartAt-.15,kFlamerDownTo=kFlamerLeverDownAt+.08,kFlamerUpFrom=kFlamerLeverCloseStartAt-.05,kFlamerUpTo=kFlamerLeverUpAt;
bool FlamerClipPlaying(){return g_flamerClip.kind!=0&&FlamerModel();}
double FlamerClipSeconds(){const auto& C=g_flamerClip;const double from=C.kind==1?kFlamerDownFrom:kFlamerUpFrom,to=C.kind==1?kFlamerDownTo:kFlamerUpTo;return from+(std::max)(0.0,C.scrub)*(to-from);} // scrubbed by the hand
double FlamerClipEnd(){return g_flamerClip.kind==1?kFlamerDownTo:kFlamerUpTo;}
const FlamerLeverClip& FlamerClipPose()
{
 auto& C=g_flamerClip;if(C.poseFrame==g_frame)return C;C.poseFrame=g_frame;
 const double s=(std::min)(FlamerClipSeconds(),FlamerClipEnd());
 int i=0;while(i+1<kFlamerClipReloadCount&&kFlamerClipReload[i+1].t<=s)++i;const int j=(std::min)(i+1,kFlamerClipReloadCount-1);
 const auto& a=kFlamerClipReload[i];const auto& b=kFlamerClipReload[j];const auto& ea=kFlamerClipReloadExtra[i];const auto& eb=kFlamerClipReloadExtra[j];
 const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(s-a.t)/span)):0;
 auto K=[](const float* v){Mat4 m=HandIdentity();for(int k=0;k<12;++k)m.m[k]=v[k];return m;};
 C.hand=PistolRigidBlend(K(a.leftHand),K(b.leftHand),w);C.handle=PistolRigidBlend(K(a.lever),K(b.lever),w);
 C.crank=PistolRigidBlend(K(ea.crank),K(eb.crank),w);C.hydraulic=PistolRigidBlend(K(ea.hydraulic),K(eb.hydraulic),w);
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.leftFingers,bone),BoneMatrix(b.leftFingers,bone),w);for(int k=0;k<12;++k)C.fingers[bone*12+k]=static_cast<float>(m.m[k]);}
 return C;
}
void ReleaseFlamerLever219()
{
 auto& F=g_flamer;F.leverGrab=false;
 if(!g_flamerClip.kind)return;
 const auto pose=FlamerClipPose();F.settledHandle=pose.handle;F.settledCrank=pose.crank;F.settledHydraulic=pose.hydraulic;F.leverPoseKnown=true;
 F.lever=g_flamerClip.kind==1?g_flamerClip.scrub:1-g_flamerClip.scrub;
 g_flamerClip.kind=0;g_flamerClip.scrub=-1;
}
// The lever tip (FRAME) at clip time t (Jason's handle pose x the farthest handle vertex).
V3 FlamerTipAtClip(double t)
{
 int i=0;while(i+1<kFlamerClipReloadCount&&kFlamerClipReload[i+1].t<=t)++i;
 Mat4 h=HandIdentity();for(int k=0;k<12;++k)h.m[k]=kFlamerClipReload[i].lever[k];
 return Apply(h,{kFlamerHandleTipBind[0]-kFlamerBindHandle[0],kFlamerHandleTipBind[1]-kFlamerBindHandle[1],kFlamerHandleTipBind[2]-kFlamerBindHandle[2]});
}
Mat4 FlamerCrank(){return FlareHinge(kFlamerCrankAxis,kFlamerCrankPivot,kFlamerCrankDownDegrees*g_flamer.lever);}
double FlamerFoldDeg(){const auto& F=g_flamer;return (F.leverGrab||F.lever>.001)?kFlamerHandleFoldDegrees*(1-F.lever)+3.0*F.lever:0;}
// Lever parts rel FRAME (captured: handle = crank(theta) x fold(phi) x handle rest; crank / hydraulic = crank(theta) x rest).
Mat4 FlamerHandle(){if(FlamerClipPlaying())return FlamerClipPose().handle;if(g_flamer.leverPoseKnown)return g_flamer.settledHandle;return Multiply(FlamerCrank(),Multiply(FlareHinge(kFlamerHandleFoldAxis,kFlamerHandlePivot,FlamerFoldDeg()),FlamerKeyMat(kFlamerLeverUpFromFrame)));}
Mat4 FlamerCanisterRest(){return FlamerKeyMat(kFlamerCanisterRestFromFrame);}
Mat4 FlamerCarriedBody(){return OtherCarried319(W7VisualWrist54(3,g_flamer.leftWrist),W7Ammo(3,FlamerKeyMat(kFlamerCanisterFromLeftWristReturn)));}
// Insertion box is the palm position that seats the held tank, not the tank's centre.
// This makes the displayed yellow sphere/box contact exactly match the insertion rule.
Mat4 FlamerSeatBox()
{
 Mat4 box=FlamerCanisterRest(),inverse={};
 if(!Invert(W7Ammo(3,FlamerKeyMat(kFlamerCanisterFromLeftWristReturn)),inverse))return box;
 const Mat4 wrist=Multiply(box,inverse);
 const V3 palm=Apply(wrist,{-.591+kFlamerHand[0],.011+kFlamerHand[1],1.149+kFlamerHand[2]});
 box.m[3]=palm.x;box.m[7]=palm.y;box.m[11]=palm.z;return FlamerInsertAdjust55(box);
}

Mat4 FlamerLeverBox(){const Mat4 h=FlamerHandle();Mat4 m=h;const V3 tip=Apply(h,{kFlamerHandleTipBind[0]-kFlamerBindHandle[0],kFlamerHandleTipBind[1]-kFlamerBindHandle[1],kFlamerHandleTipBind[2]-kFlamerBindHandle[2]});m.m[3]=tip.x;m.m[7]=tip.y;m.m[11]=tip.z;m=W7Adjust(3,0,m);if(LeftOther319(true)){const auto mirror=SidearmMirror315();m=Multiply(mirror,Multiply(m,mirror));}return m;}
double FlamerBoxDistance(const Mat4& box,const float* size,const V3& p)
{
 Mat4 inverse={};if(!Invert(box,inverse))return 1e9;const V3 l=Apply(inverse,p);const double v[3]={l.x,l.y,l.z};double d2=0;
 for(int a=0;a<3;++a){const double o=(std::max)(0.0,std::fabs(v[a])-size[a]*.5);d2+=o*o;}return std::sqrt(d2);
}
bool FlamerPalmBody(V3& palm){if(!FlamerLeftReady())return false;palm=Apply(g_flamer.leftWrist,{-.591+kFlamerHand[0],.011+kFlamerHand[1],1.149+kFlamerHand[2]});return true;}
bool FlamerToFrame(const V3& body,V3& out){Mat4 inverse={};if(!FlamerFrameReady()||!Invert(g_flamer.frameBody,inverse))return false;out=Apply(inverse,body);return true;}
int FlamerLoaded(){const auto& F=g_flamer;if(!F.ammoWrapper||!ReloadSignalLocal(F.ammoState,F.ammoWrapper))return -1;const UINT w=ReloadRead(F.ammoWrapper,0x48);if(!w)return -1;return static_cast<int>(ReloadRead(w,0xcc));}
Mat4 FlamerFallBody(double s)
{
 const auto& F=g_flamer;const double scale=(std::max)(.1f,g_worldScale);
 const V3 q0={F.fallBody.m[3],F.fallBody.m[7],F.fallBody.m[11]};V3 q=q0;q.y-=.5*9.81*s*s*scale;
 const double a=3*s,c=std::cos(a),sn=std::sin(a);Mat4 spin=HandIdentity();spin.m[0]=c;spin.m[2]=sn;spin.m[8]=-sn;spin.m[10]=c;
 Mat4 m=Multiply(F.fallBody,spin);m.m[3]=q.x;m.m[7]=q.y;m.m[11]=q.z;return m;
}
// Slot pose in the draw's palette space (framePal = pal[anchor]).
void FlamerStore(float* bones,int slot,const Mat4& framePal,const Mat4& poseFromFrame,const float* bind){StoreBone(bones,slot,Multiply(framePal,Multiply(poseFromFrame,FlamerTranslate(bind,-1))));}
#include "flamer_static239.inl"
void FlamerPublishFrame58(const Mat4& toView,const float* bones,int anchor){
 auto& f=g_flamer;Mat4 inverse={};if(anchor<0||!Invert(ArmBodyToCamera(),inverse))return;
 f.framePal=BoneMatrix(bones,anchor);f.palFrame=g_frame;f.frameBody=Multiply(inverse,Multiply(toView,f.framePal));f.frameFrame=g_frame;f.frameEpoch58=g_armPoseEpoch;
}
void FlamerPalette(ID3D11DeviceContext* context,float* bones,const Mat4& toView,UINT start,UINT count)
{
 if(!FlamerModel()||!MotionInteractionEnabled())return;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=349160||d.indexBytes!=68436||d.stride!=40||d.offset!=0||d.format!=57)return;
 auto& F=g_flamer;
 const int anchor=start==0&&count==1548?0:start==1548&&count==2082?2:start==3630&&count==8004?4:start==13338&&count==252?0:start==20838&&count==13380?2:-1;
 Mat4 fixedRoot={},fixedGrip={};const bool fixed=ControllerCurrentRoot124(fixedRoot,fixedGrip);
 if(fixed){F.frameBody=fixedRoot;F.frameFrame=F.palFrame=g_frame;F.frameEpoch58=g_armPoseEpoch;}
 else if(anchor>=0)PinWeaponPalette124(bones,toView,FlamerKeyMat(kFlamerRightHandFromFrame),anchor);
 if(anchor>=0&&F.palFrame!=g_frame)
 {
  // The flame tank sat in front of the gun and could not be moved: FRAME = W x pal[anchor] (flamer_clips.inl header);
  // An earlier build multiplied in the anchor's bind position as well, so FRAME, the tank, the lever and every box sat 26 cm forward.
  FlamerPublishFrame58(toView,bones,anchor);
 }
 if(F.palFrame!=g_frame)return;
 // The first anchor owns a body-space FRAME, not the palette-space matrix
 // of all subsequent draws. Native camera/world rows can change within the
 // rendered frame; re-express that same FRAME in this draw's palette space.
 Mat4 drawInverse={};if(!Invert(toView,drawInverse))return;
 const Mat4 fp=Multiply(drawInverse,Multiply(ArmBodyToCamera(),F.frameBody));
 if(fixed&&anchor>=0)StoreBone(bones,anchor,fp);
 FlamerStatic239(bones,fp,start,count);
 if(start==3630&&count==8004)
 {
  if(FlamerClipPlaying()){const auto& c=FlamerClipPose();FlamerStore(bones,2,fp,c.crank,kFlamerBindCrank);FlamerStore(bones,3,fp,c.hydraulic,kFlamerBindHydraulic);return;}
  if(F.leverPoseKnown){FlamerStore(bones,2,fp,F.settledCrank,kFlamerBindCrank);FlamerStore(bones,3,fp,F.settledHydraulic,kFlamerBindHydraulic);return;}
  const Mat4 crank=FlamerCrank();
  FlamerStore(bones,2,fp,Multiply(crank,FlamerKeyMat(kFlamerCrankUpFromFrame)),kFlamerBindCrank);
  FlamerStore(bones,3,fp,Multiply(crank,FlamerKeyMat(kFlamerHydraulicUpFromFrame)),kFlamerBindHydraulic);
 }
 else if(start==20838&&count==13380)FlamerStore(bones,1,fp,FlamerHandle(),kFlamerBindHandle);
 else if(start==13590&&count==7248)
 {
  Mat4 fromView={};
  if(((F.carrying&&FlamerLeftReady())||W7PreviewWristReady54(3))&&Invert(toView,fromView)){StoreBone(bones,0,Multiply(fromView,Multiply(ArmBodyToCamera(),Multiply(FlamerCarriedBody(),FlamerTranslate(kFlamerBindClip,-1)))));return;}
  if(F.falling&&W7Now()-F.fallAt<1500&&Invert(toView,fromView)){StoreBone(bones,0,Multiply(fromView,Multiply(ArmBodyToCamera(),Multiply(FlamerFallBody((W7Now()-F.fallAt)/1000.0),FlamerTranslate(kFlamerBindClip,-1)))));return;}
  if(!F.seated){bones[7]=-100;return;} // no canister in the gun
  FlamerStore(bones,0,fp,FlamerCanisterRest(),kFlamerBindClip);
 }
}
// Reflect only the articulated lever bones at the weapon centre. Receiver,
// dial, tank and nozzle bones stay native, including detached canister ownership.
bool FlamerLeverMirror349(ID3D11DeviceContext* context,float* bones,const Mat4& toView,UINT start,UINT count){
 if(!FlamerModel()||!MotionInteractionEnabled()||!LeftOther319(true)||!((start==3630&&count==8004)||(start==20838&&count==13380)))return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=349160||d.indexBytes!=68436||d.stride!=40||d.offset!=0||d.format!=57)return false;
 Mat4 invView={},inverse={};if(!FlamerFrameReady()||!Invert(toView,invView))return false;
 const auto frame=Multiply(invView,Multiply(ArmBodyToCamera(),g_flamer.frameBody));if(!Invert(frame,inverse))return false;
 const auto reflection=Multiply(frame,Multiply(SidearmMirror315(),inverse));
 if(start==3630){for(int b:{2,3})StoreBone(bones,b,Multiply(reflection,BoneMatrix(bones,b)));}
 else StoreBone(bones,1,Multiply(reflection,BoneMatrix(bones,1)));return true;
}
void RecordFlamerLeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!FlamerModel())return;Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_flamer.leftWrist=Multiply(inverse,view);g_flamer.leftFrame=g_frame;
}
bool FlamerLeftGrip(float* bones)
{
 const auto& F=g_flamer;if(!FlamerActive())return false;
 const float* fingers=FlamerClipPlaying()?FlamerClipPose().fingers:F.leverGrab?kFlamerLeftLeverFingers:F.carrying?kFlamerLeftCanisterReturnFingers:nullptr;if(!fingers)return false;
 const Mat4 wrist=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));return true;
}
// Holding the lever: the left wrist sits on the handle (Jason's grip) and follows it.
bool FlamerLeftLeverMove(const Mat4& toView,const float* bones,Mat4& move)
{
 const bool clip=FlamerClipPlaying();if(!FlamerActive()||!(g_flamer.leverGrab||clip)||!FlamerFrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 const Mat4 wrist=clip?FlamerClipPose().hand:Multiply(FlamerHandle(),FlamerKeyMat(kFlamerLeftWristFromLever)); // Jason's wrist while it plays
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(g_flamer.frameBody,wrist)),inverse);return true;
}
bool FlamerBusyLeft(){const auto& F=g_flamer;return FlamerActive()&&(g_flamerClip.kind!=0||F.leverGrab||F.carrying||F.pending||F.leverDown||F.nearBox);}
bool FlamerReady(){const auto& F=g_flamer;return !g_flamerClip.kind&&!F.leverDown&&F.lever<.02&&F.seated&&!F.carrying&&!F.leverGrab;}
void UpdateFlamerFire(const fc3pad::Controllers&){fc3pad::RequireFullFirePull(false);fc3pad::BlockFire(FlamerActive()&&!FlamerReady());}
void FlamerRequest(UINT state,UINT wrapper)
{
 if(g_pistolMenu.open)return;
 auto& F=g_flamer;if(!ReloadSignalLocal(state,wrapper))return;
 F.ammoState=state;F.ammoWrapper=wrapper;
 if(F.probing){F.probing=false;fc3pad::CancelReloadRequest();F.known=true;Log("[FLAMER] weapon known: fuel %d",FlamerLoaded());}
 if(!F.pending||W7Now()>F.deadline)return;
 F.pending=false;fc3pad::CancelReloadRequest();
 UINT loaded=0,capacity=0,reserve=0;const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 if(!Lever1887Ammo(wrapper,loaded,capacity,reserve))
 {fc3xr::Buzz(0);Log("[FLAMER] capacity unavailable: no native transfer");return;}
 if((std::max)(loaded,before)>=capacity||!reserve)
 {F.removalBlockedUntil54=W7Now()+2000;F.carrying=false;F.seated=true;F.replaced=true;fc3xr::Buzz(1);Log("[FLAMER] canister in; tank already full or no reserve (%u/%u, reserve %u): no ammo change",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after<=before){fc3xr::Buzz(0);Log("[FLAMER] native transfer refused; canister retained, fuel %u -> %u, engine ok=%d",before,after,ok);return;}
 F.removalBlockedUntil54=W7Now()+2000;F.carrying=false;F.seated=true;F.replaced=true;++F.fills;fc3xr::Buzz(1);
 Log("[FLAMER] new canister seated: native Begin/transfer/End, fuel %u -> %u, engine ok=%d",before,after,ok);
}
bool RevolverPickupZone(int,const V3&,double&);
void TickFlamer(bool gameplay)
{
 if(g_pistolMenu.open)return;
 auto& F=g_flamer;const auto c=fc3pad::AmmoControls();
 const bool active=gameplay&&FlamerActive();
 if(FlamerModel()||F.xOwned){fc3pad::SetXOwned(active);F.xOwned=active;}
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!F.leftGripWas,lgUp=c.leftGrip<.35f,xEdge=c.x&&!F.xWas;F.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&F.leftGripWas)):lg;F.xWas=c.x;
 const ULONGLONG now=W7Now();
 if(!active){ReleaseFlamerLever219();if(!FlamerModel()){if(F.pending)fc3pad::CancelReloadRequest();F.pending=F.falling=false;}F.nearBox=false;return;} // retain lever position, release stale motion origin
 if(F.pending&&now>F.deadline){F.pending=false;fc3pad::CancelReloadRequest();Log("[FLAMER] the engine never answered: the canister stays in the left hand");}
 if(F.pending&&now-F.pulseAt>=250){F.pulseAt=now;fc3pad::RequestReload();}
 if(F.probing&&now-F.probeAt>400){F.probing=false;fc3pad::CancelReloadRequest();}
 if(!F.known&&!F.probing&&!F.pending&&now-F.probeAt>2000){F.probing=true;F.probeAt=now;fc3pad::RequestReload();Log("[FLAMER] probing the weapon once (the reload itself stays refused)");}
 if(F.falling&&now-F.fallAt>1500)F.falling=false;
 const double scale=(std::max)(.1f,g_worldScale),handR=kFlamerHandDiameter*.5*scale;
 V3 palm={},palmBody={};const bool palmOk=FlamerPalmBody(palmBody)&&FlamerToFrame(palmBody,palm);
 const double dLever=palmOk?FlamerBoxDistance(FlamerLeverBox(),kFlamerLeverBoxSize,palm):1e9;
 Mat4 canBox=W7Adjust(3,1,FlamerCanisterRest());
 const double dCan=palmOk?FlamerBoxDistance(canBox,kFlamerCanisterBoxSize,palm):1e9;
 const bool inLever=dLever<=handR,inCan=F.leverDown&&F.lever>.98&&F.seated&&now>=F.removalBlockedUntil54&&dCan<=handR;
 F.nearBox=inLever||inCan; // the two-handed grab stays off while the hand is on the lever / canister
 double dZone=1e9,dChest=1e9;const bool zone=palmOk&&(RevolverPickupZone(0,palmBody,dZone)||RevolverPickupZone(1,palmBody,dChest));
 const bool canTake=F.leverDown&&F.lever>.98&&!F.seated&&!F.carrying&&!F.pending;
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&palmOk&&!inLever&&!inCan&&canTake&&!g_twoHand))&&!F.leverGrab)
 {
  if(canTake){F.carrying=true;F.carryingOld=false;F.refused=false;F.away=false;fc3xr::Buzz(1);Log("[FLAMER] LEFT grip + X: new canister in the left hand");}
  else if(!F.carrying){fc3xr::Buzz(0);Log(F.leverDown?"[FLAMER] LEFT grip + X: nothing - take the old canister out first":"[FLAMER] LEFT grip + X: nothing - push the lever down first");}
 }
 else if(lgEdge&&!F.carrying&&!F.leverGrab&&!F.pending)
 {
  const char* action="nothing";
  if(inLever&&!(inCan&&dCan<dLever)){F.leverGrab=true;F.leverStart=F.lever;g_flamerClip.startScrub=F.leverDown?1-F.lever:F.lever;F.leverStartZ=palm.z;F.leverGrabAt=palm;F.leverGrabBody=OtherMotion319(1,ControllerWrist(g_handBody[1],1));F.leverGunY=F.frameBody.m[7];g_flamerClip.kind=F.leverDown?2:1;g_flamerClip.scrub=g_flamerClip.startScrub;g_flamerClip.poseFrame=-100000;fc3xr::Buzz(0);action=F.leverDown?"lever grabbed (pull the hand up: lever up)":"lever grabbed (push the hand down: lever down)";}
  else if(inCan){F.carrying=true;F.carryingOld=true;F.seated=false;F.replaced=false;F.away=false;fc3xr::Buzz(1);action="canister taken out";}
  else if(zone&&canTake){F.carrying=true;F.carryingOld=false;F.refused=false;F.away=false;fc3xr::Buzz(1);action="new canister taken (body)";}
  if(InterlockedIncrement(&F.gripLogs)<=60)Log("[FLAMER] LEFT grip: %s | LEVER box %.3f, CANISTER box %.3f (r %.3f); lever %s, canister %s",action,dLever,dCan,handR,F.leverDown?"down":"up",F.seated?"in":"out");
 }
 // A grip press alone must not close the lever; hand motion (forward, back, up, down) drives it:
 // the lever follows the hand. Gripping it does nothing yet; moving the hand (forward, back, up, down - any way) scrubs Jason's own
 // lever animation in step with the hand; 10 cm = all the way and it locks there. Let go before that and it goes back.
 if(F.leverGrab)
 {
  if(lgUp||!palmOk)
  {
   ReleaseFlamerLever219();Log("[FLAMER] lever released: stays at its current position");
  }
  else
  {
   // The lever is pulled and pushed, not snapped: the lever tip follows the hand ALONG ITS OWN PATH (the tip's
   // travel in Jason's lever animation): push the hand the way the tip goes and it goes that far; 10 cm of any arm movement no
   // longer finishes it.
   // Hold it and push down or forward to close, up or back to open, in the player's own
   // frame. Closed (locked) -> OPEN: the hand moves UP or BACK (toward you). Open -> CLOSE: the hand moves DOWN or FORWARD (toward the
   // muzzle). Whichever of the two is larger drives Jason's lever animation; 8 cm = all the way.
   // The REAL controller, not the drawn palm: while the lever is held the drawn hand is pinned to the handle, so measuring it never
   // moved the lever.
   const V3 d=Sub(OtherMotion319(1,ControllerWrist(g_handBody[1],1)),F.leverGrabBody);V3 fwd={F.frameBody.m[1],F.frameBody.m[5],F.frameBody.m[9]};fwd.y=0;const double fl=Length(fwd);if(fl>1e-6)fwd=Scale(fwd,1/fl);
   // Final rule: pulling up raises the lever, pulling down lowers it: only the
   // hand's height. Lever up (locked) -> hand DOWN brings it down; lever down -> hand UP brings it up.
   (void)fwd;const double up=d.y-(F.frameBody.m[7]-F.leverGunY);
   const double amount=F.leverDown?up:-up;
   const double s=(std::max)(0.0,(std::min)(1.0,g_flamerClip.startScrub+amount/(.08*scale)));
   if(s>.02){if(!g_flamerClip.kind){g_flamerClip.kind=F.leverDown?2:1;Log(g_flamerClip.kind==1?"[FLAMER] lever going down with the hand":"[FLAMER] lever going up with the hand");}g_flamerClip.scrub=s;g_flamerClip.poseFrame=-100000;}
   else if(g_flamerClip.kind){g_flamerClip.scrub=0;g_flamerClip.poseFrame=-100000;}
   if(s>=.97&&g_flamerClip.kind)
   {
    const auto completed=FlamerClipPose();F.settledHandle=completed.handle;F.settledCrank=completed.crank;F.settledHydraulic=completed.hydraulic;F.leverPoseKnown=true;
    const bool down=g_flamerClip.kind==1;g_flamerClip.kind=0;g_flamerClip.scrub=-1;F.leverDown=down;F.lever=down?1:0;F.leverGrab=false;if(!down)F.replaced=false;fc3xr::Buzz(1);
    Log(down?"[FLAMER] lever all the way down: the canister can come out":(F.seated?"[FLAMER] lever all the way up: locked, fire live":"[FLAMER] lever all the way up (no canister)"));
   }
  }
  return; // the lever owns the left hand while it is held
 }
 // carried canister: back at its seat it goes in; let go elsewhere and it falls
 if(F.carrying&&!F.pending&&FlamerLeftReady()&&FlamerFrameReady())
 {
  V3 o={};const Mat4 carried=FlamerCarriedBody();FlamerToFrame({carried.m[3],carried.m[7],carried.m[11]},o);
  const double dist=palmOk?FlamerBoxDistance(FlamerSeatBox(),FlamerInsertSize55(),palm):1e9;
  if(dist>kFlamerSeatReach*1.5*scale)F.away=true; // it has to leave the seat before it can go back in
  if(F.away&&dist<=kFlamerSeatReach*scale&&!F.refused&&F.leverDown)
  {
   if(F.carryingOld){F.removalBlockedUntil54=W7Now()+2000;F.carrying=false;F.seated=true;F.replaced=true;fc3xr::Buzz(1);Log("[FLAMER] old canister put back (no ammo change)");}
   else{F.pending=true;F.deadline=now+3000;F.pulseAt=now;fc3pad::RequestReload();Log("[FLAMER] new canister at its seat: engine fill requested");}
  }
  else if(dist>kFlamerSeatReach*2*scale&&!F.carryingOld)F.refused=false;
  if(F.carrying&&lgUp&&!F.pending&&dist>kFlamerSeatReach*scale){F.fallBody=carried;F.falling=true;F.fallAt=now;F.carrying=false;Log(F.carryingOld?"[FLAMER] old canister dropped":"[FLAMER] new canister dropped, no ammo change");}
 }
}
void FlamerGuideMarkers()
{
 const auto& F=g_flamer;
 if(!FlamerActive()||!FlamerFrameReady()||!PhysicalReloadGuidesEnabled()||g_pistolMenu.open){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 // carrying: the seat; holding the lever: the lever; lever down + canister in: take it out, or (just replaced) push the lever up;
 // lever up: the lever when the tank is empty or the hand is near it.
 const int selected=g_weaponTriggerDiagnostics?(g_weaponTriggerTarget==0?1:0):F.carrying?1:(F.leverGrab||(F.lever>.02&&F.lever<.98))?0:F.leverDown?(F.seated?(F.replaced?0:1):-1):(FlamerLoaded()==0||F.nearBox)?0:-1;
 const int which=selected<0?0:selected;
 if(which<0){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const Mat4 local=which?(F.carrying?FlamerSeatBox():W7Adjust(3,1,FlamerCanisterRest())):FlamerLeverBox();const float* size=which?(F.carrying?FlamerInsertSize55():kFlamerCanisterBoxSize):kFlamerLeverBoxSize;const float scale=(std::max)(.1f,g_worldScale);
 V3 pb={},p={};const bool palmOk=FlamerPalmBody(pb)&&FlamerToFrame(pb,p);const int contact=F.leverGrab||(palmOk&&FlamerBoxDistance(local,size,p)<=kFlamerHandDiameter*.5*scale)?1:0;
 const Mat4 box=Multiply(F.frameBody,local);Mat4 basis=box;
 for(int col=0;col<3;++col){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+col]*basis.m[r*4+col];len=std::sqrt(len);if(len<1e-8)return;for(int r=0;r<3;++r)basis.m[r*4+col]/=len;}
 float a[3]={static_cast<float>(box.m[3]),static_cast<float>(box.m[7]),static_cast<float>(box.m[11])},h[3]={static_cast<float>(pb.x),static_cast<float>(pb.y),static_cast<float>(pb.z)};
 fc3xr::SetReloadMarkers(a,palmOk?h:nullptr,size[0]*.5f*scale,contact,kFlamerHandDiameter*scale);
 float transform[16],dims[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(basis.m[i]);for(int i=0;i<3;++i)dims[i]=size[i]*scale;
 fc3xr::SetWeaponBox(transform,dims,contact);if(palmOk)fc3xr::SetHandSphere();
}
bool FlamerNote()
{
 if(!FlamerActive())return false;const auto& F=g_flamer;
 fc3xr::SetTestNote(F.pending?L"FLAMETHROWER - FILLING":F.carrying?(F.carryingOld?L"FLAMETHROWER - OLD CANISTER\nLet go away from the gun to drop it.":L"FLAMETHROWER - NEW CANISTER\nPut it into its seat under the gun."):
  F.leverGrab?(F.leverDown?L"FLAMETHROWER\nPush the lever up.":L"FLAMETHROWER\nPush the lever down."):
  F.leverDown?(F.seated?L"FLAMETHROWER - LEVER DOWN\nLEFT grip the CANISTER: take it out.\nOr push the lever back up.":L"FLAMETHROWER - NO CANISTER\nLEFT grip + X: new canister."):
  F.seated?L"FLAMETHROWER\nReload: LEFT grip the LEVER, push it down.":L"FLAMETHROWER - NO CANISTER");
 return true;
}
