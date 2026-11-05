// GM-94 pump grenade launcher (gm94.xbg, profile 18). Design: after every shot, push the barrel
// forward and tilt the weapon so the shell falls out with simple physics (no tilt, no fall). Full reload: open the top
// compartment with the left hand, grab a shell from one of the 3 pickup areas, put it in, then close the top (it will not
// fire while open). A partial reload works the same way. One shell per grab; after reloading the barrel must be cycled too
// or it will not fire.
//  - Rigid (captures/weapons7-29048, work/gm94_clips.py -> gm94_clips.inl), four groups with one matrix each: FRAME (2544/7356,
//    48/870, 918/1626), BARREL (9900/2952, 12852/456, 13308/1512; slides 17 cm along FRAME +Y), COVER (14820/1638, 16458/3354 and a
//    stride-20 draw 0/48; hinges 142.85 deg about FRAME +X through its origin), SHELL (19812/36, 19848/216, 20064/324; parked inside
//    the gun at idle). The game's own reload and its post-shot animation are not used: every group is placed here from FRAME.
//  - PUMP: the two-handed hold takes the barrel (fixed grab point on it; AUTO-HOLD test: no button, the left hand reaching it holds,
//    25 cm away lets go - UpdateTwoHand); pushing the left hand forward slides the barrel with it
//    (17 cm). After a shot the spent shell sits in the chamber: with the barrel forward, TILT the gun (roll it 30 degrees, or point
//    the muzzle up/down 35 degrees) and it falls out. Barrel back with the chamber clear chambers the next shell.
//  - RELOAD (empty or partial): LEFT grip the COVER box (top) and pull it up/forward: it opens. One shell per grab: LEFT grip + X,
//    or LEFT grip at the waist or chest. Into the PORT box: the engine's own Begin/transfer/End add that one shell (capacity and
//    reserve checked). Grip the open cover and push it back: closed. Then pump (that chambers). The trigger is dry while the cover is
//    open, the barrel is forward, the chamber is spent or empty, or a pump is due.
#include "gm94_clips.inl"
struct Gm94
{
 bool chambered=true,spent=false,spentCarrying=false,needsPump=false,coverOpen=false,coverGrab=false,carrying=false,pending=false,refused=false,falling=false;
 bool coverGripSpent=false,recheckShot222=false;NativeShot219 shot;
 bool known=false,probing=false,leftGripWas=false,xWas=false,xOwned=false,triggerWas=false,pumping=false,reachedForward=false,nearBox=false;
 double offset=0,pumpStartOffset=0,coverDeg=0;V3 pumpStart={},coverStart={};ULONGLONG lastTick=0,deadline=0,pulseAt=0,probeAt=0,fallAt=0,tiltSince=0;
 bool coverPoseKnown=false;Mat4 settledCover={},settledCoverHand={};
 Mat4 frameBody={};LONG frameFrame=-100000;Mat4 leftWrist={};LONG leftFrame=-100000;Mat4 fallBody={};V3 fallVelocity={},spentLastPosition={},spentVelocity={};ULONGLONG spentSampleAt=0;
 UINT ammoState=0,ammoWrapper=0;LONG inserts=0,gripLogs=0;
} g_gm94;
#define kGm94Hand (g_w7Guides[2].hand)
#define kGm94HandDiameter (g_w7Guides[2].diameter)

const float kGm94CoverBoxLocal[3]={0.f,-.05f,.05f};
#define kGm94CoverBoxSize (g_w7Guides[2].size[0])
 // cover-local: the top of the closed cover
const float kGm94PortBox[3]={-.072f,.112f,.191f};
#define kGm94PortBoxSize (g_w7Guides[2].size[1])
 // FRAME: the left palm at Jason's inserts
double Gm94PumpDistance54();
bool Gm94Model(){return g_weaponMechanicProfile==18&&g_frame-g_weaponMechanicFrame<=2;}
bool Gm94Active(){return Gm94Model()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool Gm94FrameReady(){return g_frame-g_gm94.frameFrame<=2;}
bool Gm94LeftReady(){return g_frame-g_gm94.leftFrame<=2;}
Mat4 Gm94KeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
Mat4 Gm94Barrel(){Mat4 m=Gm94KeyMat(kGm94BarrelRestFromFrame);for(int r=0;r<3;++r)m.m[r*4+3]+=kGm94BarrelAxis[r]*g_gm94.offset;return m;}
// Opening the compartment plays the proper pose and animation: gripping the cover plays Jason's
// own open (or close) from the empty-reload capture - his left wrist + fingers and the cover, on the gun - then the cover stays.
struct Gm94CoverClip{int kind=0;double scrub=-1,startScrub=0;V3 grabBody={},grabLocal={},liftLocal={};double grabGunY=0;ULONGLONG at=0;LONG poseFrame=-100000;Mat4 hand={},cover={};float fingers[204]={};} g_gm94Clip; // kind 1 open, 2 close
bool Gm94BeginCoverMotion(const Mat4& gun,const V3& hand){
 Mat4 inv={};if(!Invert(gun,inv))return false;
 auto& C=g_gm94Clip;C.grabLocal=Apply(inv,hand);
 C.liftLocal=Sub(Apply(inv,Add(hand,{0,1,0})),C.grabLocal);return true;
}
double Gm94CoverTravel(const Mat4& gun,const V3& hand){
 Mat4 inv={};if(!Invert(gun,inv))return 0;
 const double n=Dot(g_gm94Clip.liftLocal,g_gm94Clip.liftLocal);return n>1e-12?Dot(Sub(Apply(inv,hand),g_gm94Clip.grabLocal),g_gm94Clip.liftLocal)/n:0;
}
const double kGm94OpenClipFrom=kGm94EmptyCoverOpenStartAt,kGm94OpenClipTo=kGm94EmptyCoverOpenAt,kGm94CloseClipFrom=kGm94EmptyCoverCloseStartAt,kGm94CloseClipTo=kGm94EmptyCoverClosedAt;
bool Gm94ClipPlaying(){return g_gm94Clip.kind!=0&&Gm94Model();}
double Gm94ClipSeconds(){const auto& C=g_gm94Clip;const double from=C.kind==1?kGm94OpenClipFrom:kGm94CloseClipFrom,to=C.kind==1?kGm94OpenClipTo:kGm94CloseClipTo;return from+(std::max)(0.0,C.scrub)*(to-from);} // driven by the hand
double Gm94ClipEnd(){return g_gm94Clip.kind==1?kGm94OpenClipTo:kGm94CloseClipTo;}
const Gm94CoverClip& Gm94ClipPose()
{
 auto& C=g_gm94Clip;if(C.poseFrame==g_frame)return C;C.poseFrame=g_frame;
 const double s=(std::min)(Gm94ClipSeconds(),Gm94ClipEnd());
 int i=0;while(i+1<kGm94ClipEmptyReloadCount&&kGm94ClipEmptyReload[i+1].t<=s)++i;
 const auto& a=kGm94ClipEmptyReload[i];const auto& b=kGm94ClipEmptyReload[(std::min)(i+1,kGm94ClipEmptyReloadCount-1)];
 const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(s-a.t)/span)):0;
 auto K=[](const float* v){Mat4 m=HandIdentity();for(int k=0;k<12;++k)m.m[k]=v[k];return m;};
 C.hand=PistolRigidBlend(K(a.leftHand),K(b.leftHand),w);C.cover=PistolRigidBlend(K(a.cover),K(b.cover),w);
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.leftFingers,bone),BoneMatrix(b.leftFingers,bone),w);for(int j=0;j<12;++j)C.fingers[bone*12+j]=static_cast<float>(m.m[j]);}
 return C;
}
Mat4 Gm94Cover()
{
 if(Gm94ClipPlaying())return Gm94ClipPose().cover;
 if(g_gm94.coverPoseKnown)return g_gm94.settledCover;
 return Multiply(FlareHinge(kGm94CoverHingeAxis,kGm94CoverHingePivot,-g_gm94.coverDeg),Gm94KeyMat(kGm94CoverClosedFromFrame));
}
void ReleaseGm94Cover219()
{
 auto& G=g_gm94;G.coverGrab=false;
 if(!g_gm94Clip.kind)return;
 const auto pose=Gm94ClipPose();G.settledCover=pose.cover;G.settledCoverHand=pose.hand;G.coverPoseKnown=true;
 G.coverDeg=kGm94CoverOpenAngleDeg*(g_gm94Clip.kind==1?g_gm94Clip.scrub:1-g_gm94Clip.scrub);
 g_gm94Clip.kind=0;g_gm94Clip.scrub=-1;
}
// Support (left wrist) on the barrel, FRAME space: Jason's idle hold, slid with the barrel.
Mat4 Gm94PumpBox54(){auto& s=g_w7Guides[2];Mat4 m=Gm94Barrel();const V3 at=Apply(Gm94KeyMat(kGm94LeftSupportFromFrame),{-.591,.011,1.149});for(int a=0;a<3;++a)m.m[a*4+3]=(&at.x)[a]+kGm94BarrelAxis[a]*g_gm94.offset;return OtherSupportFrame344(W7Transform(m,s.pumpMove54,s.pumpRotation54));}
Mat4 Gm94Support(){Mat4 m=Gm94KeyMat(kGm94LeftSupportFromFrame);for(int r=0;r<3;++r)m.m[r*4+3]+=kGm94BarrelAxis[r]*g_gm94.offset;return m;}
bool Gm94SupportPosition(V3& support) // camera space, as AkSupportPosition
{
 if(!Gm94Active()||!Gm94FrameReady())return false;
 const auto m=Multiply(ArmBodyToCamera(),Multiply(g_gm94.frameBody,Gm94PumpBox54()));support={m.m[3],m.m[7],m.m[11]};return true;
}
bool Gm94PalmBody(V3& palm){if(!Gm94LeftReady())return false;palm=Apply(g_gm94.leftWrist,{-.591+kGm94Hand[0],.011+kGm94Hand[1],1.149+kGm94Hand[2]});return true;}
bool Gm94ToFrame(const V3& body,V3& out){Mat4 inverse={};if(!Gm94FrameReady()||!Invert(g_gm94.frameBody,inverse))return false;out=Apply(inverse,body);return true;}
// Contact follows Jason's captured palm on the latch, including its different open/closed grip.
Mat4 Gm94CoverBox(){Mat4 m=Gm94Cover();const Mat4 hand=Gm94ClipPlaying()?Gm94ClipPose().hand:(g_gm94.coverDeg>1&&g_gm94.coverDeg<kGm94CoverOpenAngleDeg-1&&g_gm94.coverPoseKnown)?g_gm94.settledCoverHand:Gm94KeyMat(g_gm94.coverOpen?kGm94LeftOnCoverCloseFromFrame:kGm94LeftOnCoverOpenFromFrame);const V3 c=Apply(hand,{-.591+kGm94Hand[0],.011+kGm94Hand[1],1.149+kGm94Hand[2]});m.m[3]=c.x;m.m[7]=c.y;m.m[11]=c.z;return W7Adjust(2,0,m);}
double Gm94CoverReach(double scale){return kGm94HandDiameter*.5*scale+(g_gm94.coverOpen?.08*scale:0);}
Mat4 Gm94PortBoxMat(){Mat4 m=HandIdentity();m.m[3]=kGm94PortBox[0];m.m[7]=kGm94PortBox[1];m.m[11]=kGm94PortBox[2];return W7Adjust(2,1,m);}
double Gm94BoxDistance(const Mat4& box,const float* size,const V3& p)
{
 Mat4 inverse={};if(!Invert(box,inverse))return 1e9;const V3 l=Apply(inverse,p);const double v[3]={l.x,l.y,l.z};double d2=0;
 for(int a=0;a<3;++a){const double o=(std::max)(0.0,std::fabs(v[a])-size[a]*.5);d2+=o*o;}return std::sqrt(d2);
}
int Gm94Loaded(){const auto& G=g_gm94;if(!G.ammoWrapper||!ReloadSignalLocal(G.ammoState,G.ammoWrapper))return -1;const UINT w=ReloadRead(G.ammoWrapper,0x48);if(!w)return -1;return static_cast<int>(ReloadRead(w,0xcc));}
Mat4 Gm94FallBody(double s)
{
 const auto& G=g_gm94;const double scale=(std::max)(.1f,g_worldScale);
 const V3 q0={G.fallBody.m[3],G.fallBody.m[7],G.fallBody.m[11]};V3 q=Add(q0,Scale(G.fallVelocity,s));q.y-=.5*9.81*s*s*scale;
 const double a=9*s,c=std::cos(a),sn=std::sin(a);Mat4 spin=HandIdentity();spin.m[0]=c;spin.m[1]=-sn;spin.m[4]=sn;spin.m[5]=c;
 Mat4 m=Multiply(G.fallBody,spin);m.m[3]=q.x;m.m[7]=q.y;m.m[11]=q.z;return m;
}
// Draw order (capture): the shell, barrel and cover groups are usually drawn BEFORE the body. Each group therefore finds this
// frame's FRAME from its own moved world and its native offset from FRAME (learned every frame from the game's own draws;
// rest pose until learned), so nothing trails the gun by a frame.
// Post-shot animations must not move the gun: fixed arms were not enough - the GUN still followed the
// game's own post-shot pump/recoil, because every GM-94 draw was placed from the game's animated world. Now (the 1887 / R700 rule) the
// gun is LOCKED to the right controller: its offset from the controller is learned once from a still native hold (trigger up, 1.5 s
// after a shot, barrel home, 15 frames within 4 mm), then every GM-94 draw = controller x that offset x its own part pose. The game's
// animation never moves the gun or the hands again. Two-handed, the gun turns about the right hand toward the real left wrist.
struct Gm94Lock{Mat4 rel={},candidate={},body={};bool valid=false;int stable=0;UINT epoch=0;LONG bodyFrame=-100000;ULONGLONG shotAt=0;} g_gm94Lock;
// The gun could face the wrong way after the draw-out: the learned offset could be taken during the draw-out animation
// and then stayed wrong. No learning now (the R700 rule): gun = right controller x the fixed right hand drawn there x inverse of
// Jason's captured idle grip (kGm94RightHandFromFrame). The gun always sits in the drawn hand as in the game's own idle hold.
extern Mat4 g_freeRightFromController;extern UINT g_freeRightEpoch;
bool Gm94LockedBody(Mat4& body)
{
 auto& L=g_gm94Lock;
 if(!g_handRightValid||g_freeRightEpoch!=g_armPoseEpoch)return false;
 // Tracking/support ownership can change inside the same Present. Recompute
 // from the current inputs rather than reuse a frame-number-only cached pose.
 Mat4 b={},support=Gm94KeyMat(kGm94LeftSupportFromFrame);
 if(!ControllerGripRoot124(Gm94KeyMat(kGm94RightHandFromFrame),&support,b))return false;
 L.body=b;L.bodyFrame=g_frame;body=b;return true;
}
// Learn the controller -> gun offset from a still native hold (ordinary placement of the body draw).
void Gm94LearnLock(const Mat4& ordinaryBody)
{
 auto& L=g_gm94Lock;if(L.valid&&L.epoch==g_armPoseEpoch)return;
 if(L.epoch!=g_armPoseEpoch){L.valid=false;L.stable=0;L.epoch=g_armPoseEpoch;}
 const auto& G=g_gm94;
 if(!g_handRightValid||fc3pad::Latest().rightTrigger>.3f||W7Now()-L.shotAt<1500||G.offset>.001||g_twoHand){L.stable=0;return;}
 Mat4 controller={},ci={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];if(!Invert(controller,ci))return;
 const Mat4 c=Multiply(ci,ordinaryBody);double change=0;for(int i=0;i<12;++i)change=(std::max)(change,std::fabs(c.m[i]-L.candidate.m[i]));
 L.stable=change<.004?L.stable+1:0;L.candidate=c;
 if(L.stable>=15){L.rel=c;L.valid=true;Log("[GM94] gun locked to the right controller (learned from a still hold): the game's animation no longer moves it");}
}
double Gm94PumpDistance54(){V3 palm={},local={};if(!FreeHandPalm(1,palm)){if(!g_handLeftValid)return 1e9;palm=OtherMotion319(1,ControllerWrist(g_handBody[1],1));}if(!Gm94ToFrame(palm,local))return 1e9;float dims[3];const float scale=(std::max)(.1f,g_worldScale);for(int a=0;a<3;++a)dims[a]=g_w7Guides[2].pumpSize54[a]*scale;return Gm94BoxDistance(Gm94PumpBox54(),dims,local);}
struct Gm94Native{Mat4 frame={};LONG frameFrame=-100000;Mat4 group[3]={};LONG groupFrame[3]={-100000,-100000,-100000};Mat4 rel[3]={};bool relValid[3]={};} g_gm94Native;
bool Gm94FinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
 if(!Gm94Model()||!MotionInteractionEnabled())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=243072||d.indexBytes!=40776||d.format!=57)return false;
 const bool main=d.stride==32&&d.offset==640;
 const bool frameDraw=main&&((start==2544&&count==7356)||(start==48&&count==870)||(start==918&&count==1626));
 const bool barrelDraw=main&&((start==9900&&count==2952)||(start==12852&&count==456)||(start==13308&&count==1512));
 // The auxiliary stride-20 cover piece has the SAME recorded world rows as the rigid cover.
 // Pose it with the cover, but never treat its draw as the launcher root.
 const bool coverDraw=(main&&((start==14820&&count==1638)||(start==16458&&count==3354)))||(d.stride==20&&d.offset==0&&start==0&&count==48);
 const bool shellDraw=main&&((start==19812&&count==36)||(start==19848&&count==216)||(start==20064&&count==324));
 if(!frameDraw&&!barrelDraw&&!coverDraw&&!shellDraw)return false;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);Mat4 viewInverse={},bodyInverse={};if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
 auto& G=g_gm94;auto& N=g_gm94Native;
 const int group=barrelDraw?0:coverDraw?1:shellDraw?2:-1;Mat4 nativeInverse={};
 if(frameDraw)
 {
  N.frame=nativeWorld;N.frameFrame=g_frame;
  if(Invert(nativeWorld,nativeInverse))for(int g=0;g<3;++g)if(N.groupFrame[g]==g_frame){N.rel[g]=Multiply(nativeInverse,N.group[g]);N.relValid[g]=true;}
  G.frameBody=Multiply(bodyInverse,Multiply(view,HullWorld(globals)));G.frameFrame=g_frame;
  Mat4 locked={};if(Gm94LockedBody(locked)){G.frameBody=locked;WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),locked)));}
  return true;
 }
 N.group[group]=nativeWorld;N.groupFrame[group]=g_frame;
 if(N.frameFrame==g_frame&&Invert(N.frame,nativeInverse)){N.rel[group]=Multiply(nativeInverse,nativeWorld);N.relValid[group]=true;}
 const Mat4 rest=Gm94KeyMat(group==0?kGm94BarrelRestFromFrame:group==1?kGm94CoverClosedFromFrame:kGm94ShellParkFromFrame);
 Mat4 relInverse={};if(!Invert(N.relValid[group]?N.rel[group]:rest,relInverse))return false;
 G.frameBody=Multiply(bodyInverse,Multiply(view,Multiply(HullWorld(globals),relInverse)));G.frameFrame=g_frame; // this frame's FRAME, from this draw
 {Mat4 locked={};if(Gm94LockedBody(locked))G.frameBody=locked;} // locked to the controller: no game animation
 Mat4 body=Multiply(G.frameBody,Gm94KeyMat(kGm94ShellParkFromFrame));
 // The parked shells stayed visible: the game PARKS this shell outside the gun (off-screen in the flat game, in view in
 // VR); an earlier build left it there. It is only shown in the hand, falling, or spent in the open chamber.
 if(shellDraw&&!W7PreviewWristReady54(2)&&!G.carrying&&!G.spentCarrying&&!(G.falling&&W7Now()-G.fallAt<1200)&&!(G.spent&&G.offset>.01))
 {Mat4 hidden=HullWorld(globals);for(int r=0;r<3;++r)for(int col=0;col<3;++col)hidden.m[r*4+col]*=1e-4;WritePistolWorld(globals,hidden);return true;}
 if(barrelDraw)body=Multiply(G.frameBody,Gm94Barrel());
 else if(coverDraw)body=Multiply(G.frameBody,Gm94Cover());
 else if(((G.carrying||G.spentCarrying)&&Gm94LeftReady())||W7PreviewWristReady54(2))body=OtherCarried319(W7VisualWrist54(2,G.leftWrist),W7Ammo(2,Gm94KeyMat(kGm94ShellCarryFromLeftWrist)));
 else if(G.falling&&W7Now()-G.fallAt<1200)body=Gm94FallBody((W7Now()-G.fallAt)/1000.0);
 WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),body)));return true;
}
void RecordGm94LeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!Gm94Model())return;Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_gm94.leftWrist=Multiply(inverse,view);g_gm94.leftFrame=g_frame;
}
bool Gm94LeftGrip(float* bones)
{
 const auto& G=g_gm94;if(!Gm94Active())return false;
 const float* fingers=Gm94ClipPlaying()?Gm94ClipPose().fingers:(G.carrying||G.spentCarrying)?kGm94LeftShellCarryFingers:g_twoHand?kGm94LeftIdleFingers:nullptr;if(!fingers)return false;
 const Mat4 wrist=BoneMatrix(bones,2);for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));return true;
}
// Two-handed: the left wrist sits on the barrel (Jason's hold) and slides with it.
bool Gm94LeftSupportMove(const Mat4& toView,const float* bones,Mat4& move)
{
 const bool clip=Gm94ClipPlaying();if(!Gm94Active()||!(g_twoHand||clip)||!Gm94FrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(g_gm94.frameBody,clip?Gm94ClipPose().hand:Gm94Support())),inverse);return true; // Jason's hand on the cover while it plays
}
// (Log: one cover grip in the whole test.) the auto-hold took the gun as the hand came to the top, and every grip after that was
// ignored. The hand in the COVER box (or the PORT box with the cover open) is now never taken by the two-handed hold.
// A stray grenade beside the GM-94 moved with the launcher: the recorder keeps 24 draws a frame
// and missed the GM-94's other render pass; scaling the shell inside the world write did not reach every pass. While the shell is
// not in use (in the hand, falling, or spent in the open chamber) its three draws are not issued at all, in every pass.
bool Gm94ShellVisible(){const auto& G=g_gm94;return W7PreviewWristReady54(2)||G.carrying||G.spentCarrying||(G.falling&&W7Now()-G.fallAt<1200)||(G.spent&&G.offset>.01);}
bool Gm94SkipDraw(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(call.kind!=0||!Gm94Model()||!MotionInteractionEnabled())return false;
 const auto d=NativeWeaponGeometry(context,call);
 // The "rogue grenade" ([ROGUE] probe, both passes): a separate grenade model, vb 5824 / ib 1008, pieces 0/36, 36/144, 180/324
 if(d.vertexBytes==5824&&d.indexBytes==1008&&((call.b==0&&call.a==36)||(call.b==36&&call.a==144)||(call.b==180&&call.a==324)))return true;
 if(Gm94ShellVisible())return false;
 if(!((call.b==19812&&call.a==36)||(call.b==19848&&call.a==216)||(call.b==20064&&call.a==324)))return false;
 return d.vertexBytes==243072&&d.indexBytes==40776;
}
bool Gm94BusyLeft(){const auto& G=g_gm94;return Gm94Active()&&(G.carrying||G.spentCarrying||G.coverGrab||G.coverOpen||G.pending||G.nearBox||g_gm94Clip.kind!=0);}
bool Gm94Ready(){const auto& G=g_gm94;return G.chambered&&!G.spent&&!G.needsPump&&!G.coverOpen&&G.coverDeg<1&&G.offset<.01&&!G.carrying&&!G.spentCarrying;}
void UpdateGm94Fire(const fc3pad::Controllers& c)
{
 auto& G=g_gm94;fc3pad::RequireFullFirePull(false);
 const bool active=Gm94Active(),pull=c.rightTrigger>.7f;const auto now=W7Now();const int loaded=Gm94Loaded();
 if(active&&loaded<0&&G.known){G.known=false;G.probeAt=0;}
 const bool ready=Gm94Ready(),wasPending=G.shot.pending;
 const bool confirmed=PollNativeShot219(G.shot,active,ready,G.ammoState,G.ammoWrapper,loaded,now);
 if(confirmed){
  G.chambered=false;G.spent=true;g_gm94Lock.shotAt=now;
  Log("[GM94] native shot confirmed %d -> %d: spent shell in chamber; pump + tilt",G.shot.before,loaded);
 }
 if(wasPending&&!G.shot.pending&&!confirmed){
  // Keep the expired baseline until the existing probe reconciles a delayed
  // shot. A new owner, lost tracking or changed mechanism cannot confirm it.
  G.recheckShot222=active&&ready&&loaded>=0&&G.shot.state==G.ammoState&&G.shot.wrapper==G.ammoWrapper&&
   G.shot.weapon==ReloadRead(G.ammoWrapper,0x48)&&G.shot.epoch==g_armPoseEpoch;
  G.known=false;G.probeAt=0;
 }
 if(c.rightTrigger<.3f&&!G.shot.pending)G.shot.waitRelease=false;
 if(active&&ready&&loaded>0&&pull&&!G.triggerWas&&!G.shot.waitRelease&&!G.shot.pending&&!G.recheckShot222){
  BeginNativeShot219(G.shot,G.ammoState,G.ammoWrapper,loaded,now);
  Log("[RELOAD26] GM94 trigger window opened; native count %d, awaiting actual shot",loaded);
 }
 fc3pad::BlockFire(active&&(!ready||loaded<=0||G.shot.waitRelease||G.recheckShot222)&&!G.shot.pending);
 static LONG denied=0;if(active&&pull&&!G.triggerWas&&(!Gm94Ready()||loaded<=0)&&denied<60){++denied;
  Log("[TRIGGER41] GM94 fire denied: loaded=%d chambered=%d spent=%d pumpDue=%d coverOpen=%d coverDeg=%.1f barrel=%.3f carrying=%d spentCarrying=%d frameAge=%ld handAge=%ld",loaded,G.chambered,G.spent,G.needsPump,G.coverOpen,G.coverDeg,G.offset,G.carrying,G.spentCarrying,long(g_frame-G.frameFrame),long(g_frame-G.leftFrame));
 }
 G.triggerWas=pull;
}
void Gm94Request(UINT state,UINT wrapper)
{
 if(g_pistolMenu.open)return;
 auto& G=g_gm94;if(!ReloadSignalLocal(state,wrapper))return;
 G.ammoState=state;G.ammoWrapper=wrapper;
 if(G.probing){
  G.probing=false;fc3pad::CancelReloadRequest();const int n=Gm94Loaded();
  if(G.recheckShot222){
   auto sample=G.shot;sample.pending=true;sample.until=W7Now()+1;
   if(PollNativeShot219(sample,Gm94Active(),Gm94Ready(),state,wrapper,n,W7Now())){
    G.chambered=false;G.spent=true;g_gm94Lock.shotAt=W7Now();
    Log("[GM94] delayed native shot confirmed %d -> %d",sample.before,n);
   }
   G.recheckShot222=false;
  }
  if(!G.known&&n>=0){G.known=true;if(!G.spent&&!G.needsPump)G.chambered=n>0;}
  Log("[GM94] weapon known: %d shell(s) loaded",n);
 }
 if(!G.pending||W7Now()>G.deadline)return;
 G.pending=false;fc3pad::CancelReloadRequest();
 UINT loaded=0,capacity=0,reserve=0;const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 if(!Lever1887Ammo(wrapper,loaded,capacity,reserve))
 {G.refused=true;fc3xr::Buzz(0);Log("[GM94] capacity unavailable: no native transfer");return;}
 if((std::max)(loaded,before)>=capacity||!reserve){G.refused=true;fc3xr::Buzz(0);Log("[GM94] shell refused: %u/%u, reserve %u",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after>before){G.carrying=false;++G.inserts;G.needsPump=true;fc3xr::Buzz(1);}
 Log("[GM94] shell in: native Begin/transfer/End, loaded %u -> %u, engine ok=%d",before,after,ok);
}
bool RevolverPickupZone(int,const V3&,double&);
void TickGm94(bool gameplay)
{
 if(g_pistolMenu.open)return;
 auto& G=g_gm94;const auto c=fc3pad::AmmoControls();
 const bool active=gameplay&&Gm94Active();
 if(Gm94Model()||G.xOwned){fc3pad::SetXOwned(active);G.xOwned=active;}
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!G.leftGripWas,lgUp=c.leftGrip<.35f,xEdge=c.x&&!G.xWas;G.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&G.leftGripWas)):lg;G.xWas=c.x;
 const ULONGLONG now=W7Now();G.lastTick=now;if(lgUp)G.coverGripSpent=false;
 if(!active){
  if(G.shot.pending){PollNativeShot219(G.shot,false,false,G.ammoState,G.ammoWrapper,-1,now);G.known=false;G.probeAt=0;}
  G.recheckShot222=false;
  if(G.coverGrab||g_gm94Clip.kind){ReleaseGm94Cover219();G.coverGripSpent=!lgUp;}
  G.pumping=false;G.nearBox=false;G.tiltSince=0;
  if(!Gm94Model()){if(G.pending)fc3pad::CancelReloadRequest();G.pending=G.carrying=G.spentCarrying=G.falling=false;}
  return;
 }
 const auto& attachment=g_freeHandAttachment[1];
 if(attachment.epoch==g_armPoseEpoch&&attachment.profile==18&&g_frame-attachment.frame<=2){
  Mat4 locked={},controller={};if(Gm94LockedBody(locked)){G.frameBody=locked;G.frameFrame=g_frame;}
  for(int i=0;i<16;++i)controller.m[i]=g_handBody[1][i];
  Mat4 actual319={};if(LeftOther319()&&OtherWrist319(1,actual319))G.leftWrist=Multiply(actual319,SidearmMirror315());else G.leftWrist=Multiply(controller,attachment.relative);G.leftFrame=g_frame;
 }
 if(G.pending&&now>G.deadline){G.pending=false;fc3pad::CancelReloadRequest();Log("[GM94] the engine never took the shell: it stays in the left hand");}
 if(G.pending&&now-G.pulseAt>=250){G.pulseAt=now;fc3pad::RequestReload();}
 if(G.probing&&now-G.probeAt>400){G.probing=false;fc3pad::CancelReloadRequest();}
 if(!G.known&&!G.probing&&!G.pending&&now-G.probeAt>2000){G.probing=true;G.probeAt=now;fc3pad::RequestReload();Log("[GM94] probing the weapon once to read its loaded shells (the reload itself stays refused)");}
 if(G.falling&&now-G.fallAt>1200)G.falling=false;
 const double scale=(std::max)(.1f,g_worldScale),handR=kGm94HandDiameter*.5*scale;
 // cover swings at the native pace (open 0.13 s, close 0.15 s)
 // Cover position is retained; only controller motion operates it.
 const bool coverSettled=std::fabs(G.coverDeg-(G.coverOpen?kGm94CoverOpenAngleDeg:0))<1;
 // ---- pump: the two-handed hold slides the barrel along FRAME +Y with the real left hand
 V3 real={};const bool realOk=Gm94ToFrame(OtherMotion319(1,ControllerWrist(g_handBody[1],1)),real);
 if(g_twoHand&&realOk&&!G.carrying&&!G.spentCarrying)
 {
  if(!G.pumping){G.pumping=true;G.pumpStart=real;G.pumpStartOffset=G.offset;}
  const double along=Dot(Sub(real,G.pumpStart),{kGm94BarrelAxis[0],kGm94BarrelAxis[1],kGm94BarrelAxis[2]});
  G.offset=(std::max)(0.0,(std::min)(static_cast<double>(kGm94BarrelTravel),G.pumpStartOffset+along*kGm94BarrelTravel/(g_w7Guides[2].pumpStroke54*scale)));
 }
 else{G.pumping=false;} // released pump stays where the player left it
 if(G.offset>=kGm94BarrelTravel*.8&&!G.reachedForward){G.reachedForward=true;fc3xr::Buzz(0);}
 // spent shell drops when the barrel is forward and the gun is tilted
 {
  const double xu=G.frameBody.m[4],yu=G.frameBody.m[5];const double xl=std::sqrt(G.frameBody.m[0]*G.frameBody.m[0]+xu*xu+G.frameBody.m[8]*G.frameBody.m[8]),yl=std::sqrt(G.frameBody.m[1]*G.frameBody.m[1]+yu*yu+G.frameBody.m[9]*G.frameBody.m[9]);
  // The spent shell needed several pumps and a full sideways turn; it should fall as soon as the chamber is open and the
  // launcher is tilted: roll 20 deg or muzzle 25 deg up/down is enough, and it falls as soon as the barrel is forward.
  const bool tilted=Gm94FrameReady()&&xl>1e-6&&yl>1e-6&&(std::fabs(xu/xl)>.34||std::fabs(yu/yl)>.42);
  // Pulling the barrel back with the weapon tilted drops the shell; otherwise part of it stays visible in the barrel:
  // it drops as the barrel comes back from a full stroke with the gun tilted.
  const bool canDrop=G.spent&&G.offset>=kGm94BarrelTravel*.6&&tilted;
  if(canDrop){G.spent=false;G.falling=true;G.fallAt=now;G.fallBody=Multiply(G.frameBody,Gm94KeyMat(kGm94ShellParkFromFrame));G.fallVelocity={};fc3xr::Buzz(0);Log("[GM94] chamber open and the gun tilted: the spent shell falls out");}
 }
 // barrel back after a full stroke: chambers (if the chamber is clear and there is a shell)
 if(G.reachedForward&&G.offset<=kGm94BarrelTravel*.15)
 {
  const int n=Gm94Loaded();
  if(G.spent)Log("[GM94] pumped but the spent shell is still in: tilt the gun as you pull the barrel back");
  else if(G.coverOpen)Log("[GM94] pumped with the cover open: close it first");
  else{G.reachedForward=false;G.needsPump=false;G.chambered=n!=0;fc3xr::Buzz(1);Log("[GM94] pumped: %s",G.chambered?"chambered, fire live":"no shell to chamber");}
 }
 // ---- cover, shells
 V3 palm={},palmBody={};const bool palmOk=Gm94PalmBody(palmBody)&&Gm94ToFrame(palmBody,palm);
 const double dCover=palmOk?Gm94BoxDistance(Gm94CoverBox(),kGm94CoverBoxSize,palm):1e9,dPort=palmOk?Gm94BoxDistance(Gm94PortBoxMat(),kGm94PortBoxSize,palm):1e9;
 const Mat4 spentBox=W7Adjust(2,2,Gm94KeyMat(kGm94ShellParkFromFrame));
 const bool inSpent=G.spent&&G.offset>=kGm94BarrelTravel*.6&&palmOk&&Gm94BoxDistance(spentBox,g_w7Guides[2].size[2],palm)<=handR;
 const bool inCover=dCover<=Gm94CoverReach(scale),inPort=G.coverOpen&&coverSettled&&dPort<=handR;
 static LONG attempts=0;if(lgEdge&&attempts<80){++attempts;
  Log("[TRIGGER41] GM94 grip: palmFresh=%d frameAge=%ld handAge=%ld coverDistance=%.3f portDistance=%.3f coverHit=%d portHit=%d spentHit=%d twoHand=%d carrying=%d pending=%d coverGrab=%d clip=%d gripSpent=%d",palmOk,long(g_frame-G.frameFrame),long(g_frame-G.leftFrame),dCover,dPort,inCover,inPort,inSpent,g_twoHand,G.carrying,G.pending,G.coverGrab,g_gm94Clip.kind,G.coverGripSpent);
 }
 if(lgEdge&&inSpent&&!G.carrying&&!G.pending&&!G.coverGrab){G.spent=false;G.spentCarrying=true;G.spentSampleAt=0;G.spentVelocity={};g_twoHand=false;G.pumping=false;fc3xr::Buzz(0);Log("[GM94] spent shell grabbed: release LEFT grip to throw it away");}
 if(G.spentCarrying)
 {
  const Mat4 held=OtherCarried319(G.leftWrist,W7Ammo(2,Gm94KeyMat(kGm94ShellCarryFromLeftWrist)));const V3 at=OtherMotion319(1,ControllerWrist(g_handBody[1],1));
  if(G.spentSampleAt&&now>G.spentSampleAt){const double seconds=(now-G.spentSampleAt)/1000.0;if(seconds<=.15){const V3 v=Scale(Sub(at,G.spentLastPosition),1/seconds);G.spentVelocity=Length(v)<=12*scale?v:V3{};}else G.spentVelocity={};}
  G.spentLastPosition=at;G.spentSampleAt=now;
  if(lgUp){G.fallBody=held;G.fallVelocity=G.spentVelocity;G.falling=true;G.fallAt=now;G.spentCarrying=false;Log("[GM94] spent shell thrown: chamber clear, no ammo change");}
  return;
 }
 G.nearBox=palmOk&&!G.pumping&&(inCover||inPort||inSpent);
 double dZone=1e9,dChest=1e9;const bool zone=palmOk&&!g_twoHand&&(RevolverPickupZone(0,palmBody,dZone)||RevolverPickupZone(1,palmBody,dChest));
 const bool canTake=G.coverOpen&&coverSettled&&!G.carrying&&!G.pending;
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&palmOk&&!inCover&&!inPort&&!inSpent&&!G.pumping&&canTake))&&!G.coverGrab&&!g_twoHand)
 {
  if(canTake){G.carrying=true;G.refused=false;fc3xr::Buzz(1);Log("[GM94] LEFT grip + X: one shell in the left hand");}
  else if(!G.carrying){fc3xr::Buzz(0);Log("[GM94] LEFT grip + X: nothing - open the top cover first");}
 }
 else if((lgEdge||(lg&&inCover&&!G.coverGripSpent))&&!G.carrying&&!G.coverGrab&&!G.pending&&(!g_twoHand||inCover))
 {
  const char* action="nothing";
  if(inCover&&!g_gm94Clip.kind&&!G.coverGripSpent){g_twoHand=false;G.coverGripSpent=true;G.coverGrab=true;g_gm94Clip.grabBody=OtherMotion319(1,ControllerWrist(g_handBody[1],1));g_gm94Clip.grabGunY=G.frameBody.m[7];Gm94BeginCoverMotion(G.frameBody,g_gm94Clip.grabBody);g_gm94Clip.kind=G.coverOpen?2:1;g_gm94Clip.startScrub=G.coverOpen?1-G.coverDeg/kGm94CoverOpenAngleDeg:G.coverDeg/kGm94CoverOpenAngleDeg;g_gm94Clip.scrub=g_gm94Clip.startScrub;g_gm94Clip.poseFrame=-100000;fc3xr::Buzz(0);action=G.coverOpen?"cover grabbed (push the hand down to close)":"cover grabbed (lift the hand to open)";}
  else if(zone&&canTake){G.carrying=true;G.refused=false;fc3xr::Buzz(1);action="shell taken (body)";}
  if(InterlockedIncrement(&G.gripLogs)<=60)Log("[GM94] LEFT grip: %s | COVER box %.3f, PORT box %.3f (r %.3f); zones %.3f / %.3f; cover %s",action,dCover,dPort,handR,dZone,dChest,G.coverOpen?"open":"closed");
 }
 // The top compartment opens with motion controls and the animation pose, not automatically on a grip press:
 // gripping the cover does nothing by itself. Lift the hand (the REAL controller - the
 // drawn hand is pinned to the cover) and the cover opens with it through Jason's own open (his hand pose, fingers, the cover); push
 // the hand down and it closes the same way. 8 cm = all the way; small reversals do not undo progress. Release retains the partial pose.
 if(G.coverGrab)
 {
  if(lgUp||!g_handLeftValid){ReleaseGm94Cover219();Log("[GM94] cover released: keeps its current position");}
  else
  {
   const double dy=Gm94CoverTravel(G.frameBody,OtherMotion319(1,ControllerWrist(g_handBody[1],1)));
   const double s=(std::max)(0.0,(std::min)(1.0,g_gm94Clip.startScrub+(g_gm94Clip.kind==2?-dy:dy)/(.08*scale)));
   if(s>.02){if(!g_gm94Clip.kind){g_gm94Clip.kind=G.coverOpen?2:1;Log(G.coverOpen?"[GM94] cover closing with the hand":"[GM94] cover opening with the hand");}g_gm94Clip.scrub=s>=.97?1.0:(std::max)(g_gm94Clip.scrub,s);g_gm94Clip.poseFrame=-100000;}
   else if(g_gm94Clip.kind){g_gm94Clip.scrub=(std::max)(0.0,g_gm94Clip.scrub);g_gm94Clip.poseFrame=-100000;}
  }
 }
 if(g_gm94Clip.kind&&g_gm94Clip.scrub>=1)
 {
  const auto completed=Gm94ClipPose();G.settledCover=completed.cover;G.settledCoverHand=completed.hand;G.coverPoseKnown=true;
  G.coverGrab=false;g_gm94Clip.scrub=-1;
  const bool opened=g_gm94Clip.kind==1;g_gm94Clip.kind=0;G.coverOpen=opened;G.coverDeg=opened?kGm94CoverOpenAngleDeg:0;fc3xr::Buzz(1);if(opened)G.chambered=false;
  Log(opened?"[GM94] Jason's cover open done: one shell per grab into the port":"[GM94] Jason's cover close done (pump to chamber)");
 }
 if(G.carrying&&!G.pending&&!G.refused&&inPort){G.pending=true;G.deadline=now+3000;G.pulseAt=now;fc3pad::RequestReload();Log("[GM94] shell at the port: engine fill requested");}
 if(!inPort)G.refused=false;
 if(G.carrying&&lgUp&&!G.pending){G.carrying=false;Log("[GM94] shell dropped, no ammo change");}
}
void Gm94GuideMarkers()
{
 const auto& G=g_gm94;
 if(!Gm94Active()||!Gm94FrameReady()||!PhysicalReloadGuidesEnabled()||g_pistolMenu.open){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 // (Log: the hand never came within 27 cm of the cover; no guide was shown with a full gun): the cover guide shows whenever the
 // gun is not full OR the left palm is within 30 cm of the cover.
 V3 nb={},np={};const bool nearTop=Gm94PalmBody(nb)&&Gm94ToFrame(nb,np)&&Gm94BoxDistance(Gm94CoverBox(),kGm94CoverBoxSize,np)<.30*(std::max)(.1f,g_worldScale);
 const int n=Gm94Loaded();const bool spent=g_weaponTriggerDiagnostics?g_weaponTriggerTarget==2:G.spent&&G.offset>=kGm94BarrelTravel*.6;const bool port=g_weaponTriggerDiagnostics?g_weaponTriggerTarget==0:G.carrying&&G.coverOpen;const bool cover=g_weaponTriggerDiagnostics?g_weaponTriggerTarget==1:!port&&(G.coverOpen||G.coverGrab||(n>=0&&n<4)||nearTop||PhysicalReloadGuidesEnabled());
 const bool pump=!g_weaponTriggerDiagnostics&&!port&&!spent&&!G.coverOpen&&!G.coverGrab&&(G.needsPump||G.spent||!G.chambered||G.pumping);
 if(!port&&!cover&&!spent&&!pump){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const Mat4 local=pump?Gm94PumpBox54():spent?W7Adjust(2,2,Gm94KeyMat(kGm94ShellParkFromFrame)):port?Gm94PortBoxMat():Gm94CoverBox();const float* size=pump?g_w7Guides[2].pumpSize54:spent?g_w7Guides[2].size[2]:port?kGm94PortBoxSize:kGm94CoverBoxSize;const float scale=(std::max)(.1f,g_worldScale);
 V3 pb={},p={};const bool palmOk=Gm94PalmBody(pb)&&Gm94ToFrame(pb,p);const int contact=G.coverGrab||(palmOk&&Gm94BoxDistance(local,size,p)<=(cover?Gm94CoverReach(scale):kGm94HandDiameter*.5*scale))?1:0;
 const Mat4 box=Multiply(G.frameBody,local);Mat4 basis=box;
 for(int col=0;col<3;++col){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+col]*basis.m[r*4+col];len=std::sqrt(len);if(len<1e-8)return;for(int r=0;r<3;++r)basis.m[r*4+col]/=len;}
 float a[3]={static_cast<float>(box.m[3]),static_cast<float>(box.m[7]),static_cast<float>(box.m[11])},h[3]={static_cast<float>(pb.x),static_cast<float>(pb.y),static_cast<float>(pb.z)};
 fc3xr::SetReloadMarkers(a,palmOk?h:nullptr,size[0]*.5f*scale,contact,kGm94HandDiameter*scale);
 float transform[16],dims[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(basis.m[i]);for(int i=0;i<3;++i)dims[i]=size[i]*scale;
 fc3xr::SetWeaponBox(transform,dims,contact);if(palmOk)fc3xr::SetHandSphere();
}
bool Gm94Note()
{
 if(!Gm94Active())return false;const auto& G=g_gm94;
 fc3xr::SetTestNote(G.spentCarrying?L"GM-94 - SPENT SHELL\nRelease LEFT grip to throw it away.":G.pending?L"GM-94 - LOADING":G.carrying?L"GM-94 - SHELL IN HAND\nPut it in the PORT box.":G.coverOpen?L"GM-94 - TOP OPEN\nOne shell per grab: LEFT grip + X (or grip at waist / chest).\nGrip the cover and push it back to close.":
  G.spent?(G.offset>=kGm94BarrelTravel*.6?L"GM-94 - BARREL FORWARD\nTilt to drop the shell, OR grip the shell to remove it.\nThen pull the pump back.":L"GM-94 - FIRED\nHold it with both hands, push the barrel forward.")
  :G.needsPump||!G.chambered?L"GM-94 - PUMP\nHold it with both hands: push the barrel forward and back.":L"GM-94");
 return true;
}

