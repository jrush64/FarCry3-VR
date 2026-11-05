#include "weapon_clip_clock223.inl"
// Remington R700 (700sniperrifle.xbg, profile 2, rifle family 14). Design:
//  PHYSICAL (default, like the 1887 Manual): after a shot the trigger locks. LEFT grip on the forend moves the gun into the LEFT
//  hand (pinned where it was at that moment, so nothing jumps). RIGHT grip inside the BOLT box grabs the handle; the handle, the
//  bolt and Jason's right hand then follow the real right palm along the captured native path (lift 65 deg, 7 cm back, forward,
//  down) by a single stroke fraction; fully back then home = chambered. Let go of LEFT grip: gun back in the right hand.
//  SIMPLE (INSERT "Bolt mode"): RIGHT grip plays the captured cycle on the gun in the right hand; the LEFT hand snaps to the forend
//  for the cycle so the right hand never works the bolt in the air.
//  EMPTY reload (both modes): block in by hand, then the game's own reload animation (left arm follows it, fire locked to End).
//  Partial reload: instant, fires at once. The game's automatic post-shot bolt wiggle is always held at rest.
#include "r700_bolt_path.inl"
#include "z93_bolt_path.inl"
const Mat4 kR700RightHandOnBolt=RifleMat12(kR700RightHandFromBolt); // right wrist rel the bolt while on the handle
const Mat4 kZ93RightHandOnBolt=RifleMat12(kZ93RightHandFromBolt);   // Z93: a median only - Jason's hand is not rigid on its handle
// The bolt-action family (the Z93 works exactly like the R700): every gun-specific number comes from this table.
static_assert(sizeof(Z93Key)==sizeof(R700Key),"Z93 keys share the R700 key layout");
struct BoltGun
{
 LONG profile;const R700Key* bolt;int boltCount;double cycleSeconds;
 const Mat4 *hand,*support,*partRest0,*partRest1,*rightOnBolt;const float *leftIdle,*rightIdle;
 int strokeFirst,strokeCount,strokeHandKey;const float* strokeS;const float* tipLocal;
 bool handRigid;           // R700: Jason's hand rigid on the handle (1 mm); Z93: not (35/86/33 mm) - the keys' own hand is used
 bool boltAfterEveryReload;// Z93 needs the bolt after EVERY reload, full or partial; R700 only after an empty one
 bool partFollowsBolt;     // Z93: part 1 (36630) is rigid with the bolt (37260)
 bool pinSwingParts;       // Z93: parts 2/3 kick about 30 deg at every shot (the game's animation): drawn at their settled rest
};
const BoltGun kBoltGuns[2]={
 {2,kR700ClipBolt,kR700ClipBoltCount,kR700BoltCycleSeconds,&kR700HandFromFrame,&kR700SupportHandFromFrame,&kR700PartRest0,&kR700PartRest1,&kR700RightHandOnBolt,
  kR700LeftIdleFingers,kR700RightIdleFingers,kR700StrokeFirstKey,kR700StrokeCount,kR700StrokeHandKey,kR700StrokeS,kR700BoltTipLocal,true,false,false,false},
 {23,reinterpret_cast<const R700Key*>(kZ93ClipBolt),kZ93ClipBoltCount,kZ93BoltCycleSeconds,&kZ93HandFromFrame,&kZ93SupportHandFromFrame,&kZ93PartRest0,&kZ93PartRest1,&kZ93RightHandOnBolt,
  kZ93LeftIdleFingers,kZ93RightIdleFingers,kZ93StrokeFirstKey,kZ93StrokeCount,kZ93StrokeHandKey,kZ93StrokeS,kZ93BoltTipLocal,false,true,true,true},
};
const BoltGun& Bolt(){return g_weaponMechanicProfile==23?kBoltGuns[1]:kBoltGuns[0];}
struct R700
{
 bool physical=true,showGuides=true;
 float rightPalm[3]={.0658f,.0481f,-.1174f},rightDiameter=.12f; // RIGHT hand sphere: offset from the wrist point (bind space; the left hand default mirrored) + diameter
 bool chambered=true;int clip=-1;ULONGLONG clipAt=0,clipPauseAt223=0;bool rgWas=false,triggerWas=false;
 bool native=false;ULONGLONG nativeAt=0,nativeEndAt=0;
 bool hold=false,grabbing=false,reachedBack=false;double s=0,grabS=0,grabY=0,backY=0;V3 grabStart={};Mat4 holdRel={},holdBody0={};V3 holdLeft0={},holdGrip0={};bool awaitGrip=false;ULONGLONG holdAt=0;bool leftGripWas=false;
 Mat4 grabController={};ULONGLONG strokeLogAt=0; // the REAL right controller at the grab: the stroke is its movement since, gun-local
 Mat4 rightHandBody={};LONG rightHandFrame=-100000;ULONGLONG shotAt=0;
 Mat4 freeFromController={};UINT freeEpoch=0xffffffffu;
 Mat4 releaseFrom={};ULONGLONG releaseAt=0; // the gun where the left-hand hold ended: blended into the right-hand root (no snap) // the fixed right hand relative to the controller (constant per pose epoch) // the DRAWN right wrist (body space): the palm offset is in the arm skin's frame, not the controller's
 struct Pose{Mat4 hand,bolt;float fingers[204];} pose={};LONG poseFrame=-100000;double poseS=-1;
 LONG shots=0,cycles=0,refused=0,holds=0,grabs=0;
} g_r700;
bool R700Model(){return (g_weaponMechanicProfile==2||g_weaponMechanicProfile==23)&&g_frame-g_weaponMechanicFrame<=2;} // the bolt-action family: R700, Z93
bool R700Active(){return R700Model()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handRightValid;}
bool R700ClipPlaying(){return g_r700.clip>=0&&R700Model();}
bool R700Hold(){return g_r700.hold&&R700Model();}
bool R700LeftHold(){return R700Active()&&R700Hold();}
bool R700Grabbing(){return R700Hold()&&g_r700.grabbing;}
// Revision 6 (every shot played the bolt animation on both the hand and the R700): the rifle path places the
// gun as native gun x (native right hand -> controller). The game's native right hand LEAVES the grip for 0.8 s after every shot
// (its own bolt cycle), so that move dragged the whole gun around the controller. The R700's root is the controller x the grip
// learned at a QUIET moment (chambered, 1.5 s after a shot, no reload); it never follows the native hand again.
bool R700GripQuiet(){const auto& R=g_r700;return R.chambered&&!R.native&&R.clip<0&&GetTickCount64()-R.shotAt>1500;}
bool R700ParentedRoot(){return R700Model()&&!g_r700.hold;}
bool R700BoltBoxDue(){return R700Hold()&&g_r700.physical&&!g_r700.chambered;} // the BOLT box is shown while the bolt is due
Mat4 R700KeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
void R700KeyPose(const R700Key& k,R700::Pose& out){out.hand=R700KeyMat(k.rightHand);out.bolt=R700KeyMat(k.bolt);memcpy(out.fingers,k.rightFingers,sizeof(out.fingers));}
void R700BlendPose(const R700::Pose& a,const R700::Pose& b,double w,R700::Pose& out)
{
 out.hand=PistolRigidBlend(a.hand,b.hand,w);out.bolt=PistolRigidBlend(a.bolt,b.bolt,w);
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.fingers,bone),BoneMatrix(b.fingers,bone),w);for(int j=0;j<12;++j)out.fingers[bone*12+j]=static_cast<float>(m.m[j]);}
}
// Simple mode: the cycle by time.
void R700SampleClip(double seconds,R700::Pose& out)
{
 const R700Key* k=Bolt().bolt;const int count=Bolt().boltCount;
 int i=0;while(i+1<count&&k[i+1].t<=seconds)++i;
 R700::Pose a={},b={};R700KeyPose(k[i],a);R700KeyPose(k[(std::min)(i+1,count-1)],b);
 const double span=k[(std::min)(i+1,count-1)].t-k[i].t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(seconds-k[i].t)/span)):0;
 R700BlendPose(a,b,w,out);
}
// Physical: the stroke by fraction s (0 rest .. 1 fully back) along the captured opening keys 8..22 (position, not time).
// The bolt comes from the keys; the right hand rides the handle rigidly (kR700RightHandFromBolt, constant to 0.3 mm over the
// stroke), so it is on the handle at every s, including the approach keys where Jason's own hand was still arriving.
void R700SampleStroke(double s,R700::Pose& out)
{
 s=(std::max)(0.0,(std::min)(1.0,s));
 int i=0;while(i+1<Bolt().strokeCount&&Bolt().strokeS[i+1]<=s)++i;
 const int j=(std::min)(i+1,Bolt().strokeCount-1);
 const R700Key& a=Bolt().bolt[Bolt().strokeFirst+i];const R700Key& b=Bolt().bolt[Bolt().strokeFirst+j];
 const double span=Bolt().strokeS[j]-Bolt().strokeS[i],w=span>1e-6?(s-Bolt().strokeS[i])/span:0;
 out.bolt=PistolRigidBlend(R700KeyMat(a.bolt),R700KeyMat(b.bolt),w);
 out.hand=Bolt().handRigid?Multiply(out.bolt,(*Bolt().rightOnBolt)):PistolRigidBlend(R700KeyMat(a.rightHand),R700KeyMat(b.rightHand),w);
 const R700Key& f=Bolt().bolt[(std::max)(Bolt().strokeHandKey,Bolt().strokeFirst+i)];memcpy(out.fingers,f.rightFingers,sizeof(out.fingers));
}
// Grasp point (gun-local) at each stroke key: bolt pose x bolt-local grasp point.
struct R700StrokePath{V3 tip[24];bool ready=false;LONG profile=-1;} g_r700Path;
const R700StrokePath& R700Path()
{
 auto& P=g_r700Path;if(P.ready&&P.profile==Bolt().profile)return P;P.profile=Bolt().profile;
 for(int i=0;i<Bolt().strokeCount;++i)P.tip[i]=Apply(R700KeyMat(Bolt().bolt[Bolt().strokeFirst+i].bolt),{Bolt().tipLocal[0],Bolt().tipLocal[1],Bolt().tipLocal[2]});
 P.ready=true;return P;
}
double R700ClipSeconds(){return WeaponClipElapsed223(g_r700.clipAt,g_r700.clipPauseAt223);}
const R700::Pose& R700ClipPose()
{
 auto& R=g_r700;if(R.poseFrame!=g_frame||R.poseS!=R.s){if(R.clip>=0)R700SampleClip(R700ClipSeconds(),R.pose);else R700SampleStroke(R.s,R.pose);R.poseFrame=g_frame;R.poseS=R.s;}return R.pose;
}
// ---- the gun in the LEFT hand (physical): pinned where it was when LEFT grip took it
Mat4 R700LeftWristBody(){Mat4 w={};for(int i=0;i<16;++i)w.m[i]=g_handBody[1][i];return OtherController319(1,w);}
// Revision 12 (the aim changed after hand switches): the held gun followed the left wrist's TWIST, so working the bolt swung the
// barrel and the aim jumped back when the gun returned to the right hand. Now it keeps the orientation it had when it went into the
// left hand and only follows the left hand's position: the aim stays where it was while the bolt is worked.
// (An earlier attempt pivoted the held gun about its grip toward the left hand; that was worse - the gun snapped low and
// immediately back - so the earlier behaviour below was restored.)
Mat4 R700HoldBody()
{
 const auto& R=g_r700;Mat4 body=R.holdBody0;const Mat4 lw=R700LeftWristBody();
 body.m[3]+=lw.m[3]-R.holdLeft0.x;body.m[7]+=lw.m[7]-R.holdLeft0.y;body.m[11]+=lw.m[11]-R.holdLeft0.z;return body;
}
void R700BeginHold()
{
 auto& R=g_r700;const Mat4 lw=R700LeftWristBody();
 R.holdBody0=g_pistolReload.rootBody;R.holdLeft0={lw.m[3],lw.m[7],lw.m[11]};R.holdGrip0=Apply(Multiply(R.holdBody0,*Bolt().hand),{.591,.011,1.149});R.awaitGrip=false;R.hold=true;R.holdAt=GetTickCount64();R.grabbing=R.reachedBack=false;R.releaseAt=0;
}
// The drawn right wrist, recorded at the right arm draw (as the 1887 does): the hand on the controller, body space.
void RecordR700RightHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!R700Model())return;
 Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_r700.rightHandBody=Multiply(inverse,view);g_r700.rightHandFrame=g_frame;
}
// Right palm, body space and gun-local. Revision 3 fix: the palm offset (.591,.011,1.149) is in the ARM SKIN's wrist frame; applied
// to the raw controller pose it landed a metre off the gun (log: "grip at gun-local 1.18 0.66 0.31"), so no grab could ever succeed.
bool R700RightPalmBody(V3& palm)
{
 if(g_frame-g_r700.rightHandFrame>1)return false;
 const float* o=g_r700.rightPalm; // (.591,.011,1.149) is the WRIST point; the palm is that plus the INSERT offset
 palm=Apply(g_r700.rightHandBody,{.591+o[0],.011+o[1],1.149+o[2]});return true;
}
bool R700RightPalmLocal(V3& local)
{
 const auto& p=g_pistolReload;if(g_frame-p.rootFrame>1)return false;
 Mat4 inverse={};V3 palm={};if(!R700RightPalmBody(palm)||!Invert(p.rootBody,inverse))return false;
 local=Apply(inverse,palm);return true;
}
// The gun's move for draws that are not in the rigid world layout (the R700 scope passes: other shader / stride) while the gun is
// in the left hand: the FRAME's own native->corrected move of this frame (g_rifleAttachMove), never the right-hand move.
bool R700GunMove(ID3D11DeviceContext* context,Mat4& move)
{
 if(!R700Model()||g_frame-g_rifleAttachFrame>1||g_rifleAttachEpoch!=g_armPoseEpoch)return false;
 return RefreshRifleAttachmentMove51(context,move); // current draw camera, established body FRAME
}
// Revision 9 (the forearm animated one-handed, and the right hand/wrist in the left-hand hold): the right arm
// used the GAME's animated arm bones (only the hand was pinned), and the game's arm works the bolt after every shot. Now the R700's
// right arm is always the fixed neutral arm (stable path, like the 1887) and nothing of the game's arm or hand is read:
//  free hand  = controller x F, F constant per epoch (fixed neutral wrist, fixed wrist offset, g_handRef) - recorded at the arm draw;
//  gun root   = controller x F x inv(Jason's captured hand-on-grip), turned toward the left hand when two-handed;
//  hand drawn = root x hand-on-grip (= the free hand one-handed); in the left-hand hold the right hand is the free hand.
// The fixed right hand relative to the right controller, for any gun drawn with the fixed arm (GM-94 uses it too).
Mat4 g_freeRightFromController={};UINT g_freeRightEpoch=0xffffffffu;
void RecordR700FreeHand(const Mat4& freeWristView)
{
 (void)freeWristView;SeedControllerHand124();
 if(!R700Model()||!g_handRightValid||g_freeRightEpoch!=g_armPoseEpoch)return;
 auto& R=g_r700;const bool first=R.freeEpoch!=g_armPoseEpoch;
 R.freeFromController=g_freeRightFromController;R.freeEpoch=g_armPoseEpoch;
 if(first)Log("[R700 GRIP] shared controller-owned right grip; native animation cannot relearn it (epoch %u)",g_armPoseEpoch);
}
bool R700ControllerRoot(Mat4& root)
{
 const auto& R=g_r700;if(!g_handRightValid||R.freeEpoch!=g_armPoseEpoch)return false;
 Mat4 controller={},gripInverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
 if(!Invert((*Bolt().hand),gripInverse))return false;
 root=Multiply(Multiply(controller,R.freeFromController),gripInverse);
 if(g_twoHand&&g_handLeftValid)
 {
  const V3 pivot=Apply(Multiply(root,(*Bolt().hand)),{.591,.011,1.149}),support=Apply(Multiply(root,(*Bolt().support)),{-.591,.011,1.149}),real=OtherMotion319(1,ControllerWrist(g_handBody[1],1));
  if(Length(Sub(support,pivot))>.05&&Length(Sub(real,pivot))>.05)root=Multiply(TurnAndMove(Sub(support,pivot),Sub(real,pivot),pivot,pivot),root);
 }
 if(R.releaseAt){const double t=(GetTickCount64()-R.releaseAt)/150.0;if(t<1)root=PistolRigidBlend(R.releaseFrom,root,t);} // 150 ms handover, no snap
 return true;
}
// Two-handed grab point (shown brightly, like a guide). It was the
// GAME's animated left wrist moved with the gun (UpdateTwoHand), so it wandered with the game's arm animation (log: "LEFT grip held
// but the left hand is 0.33 m from the forend" with the hand on the gun). Now a fixed point: the gun x Jason's captured forend hold.
bool R700SupportPosition(V3& support) // camera space, as AkSupportPosition
{
 if(!R700Model()||g_r700.hold)return false;Mat4 root={};if(!R700ControllerRoot(root))return false;
 support=Apply(Multiply(ArmBodyToCamera(),root),A2000ChargePoint336(Apply(*Bolt().support,{-.591,.011,1.149})));return true;
}
// R700 already has a captured fixed foregrip. Applying the generic native-arm
// wrist-to-palm delta to that point a second time makes its grab zone wander.
bool R700FixedGrabPoints53(V3& point,V3& palm){
 if(!R700Model()||R700Hold()||!g_handLeftValid)return false;
 Mat4 root={};if(!R700ControllerRoot(root))return false;
 const Mat4 support=Multiply(root,(*Bolt().support));
 point=Apply(root,A2000ChargePoint336(Apply(*Bolt().support,{-.6781,.0519,1.0387})));
 if(!FreeHandPalm(1,palm))palm=ControllerWrist(g_handBody[1],1);
 return true;
}
// The guide (body space): the grab point, the real left wrist, and whether it is close enough to grab (UpdateTwoHand: 15 cm).
bool R700TwoHandGuide(V3& support,V3& wrist,bool& inRange)
{
 const auto& R=g_r700;if(!R700Model()||g_twoHand||R.hold||R.native||!g_handLeftValid||g_pistolReload.held||g_pistolReload.removalHeld)return false;
 Mat4 root={};if(!R700ControllerRoot(root))return false;
 support=Apply(root,A2000ChargePoint336(Apply(*Bolt().support,{-.591,.011,1.149})));wrist=OtherMotion319(1,ControllerWrist(g_handBody[1],1));
 inRange=Length(Sub(wrist,support))<.15;return true;
}
// Where the right controller would be if it held the gun as it is now: the inverse of R700ControllerRoot.
bool R700GripReturned(double radius)
{
 const auto& R=g_r700;if(R.freeEpoch!=g_armPoseEpoch||!g_handRightValid)return true; // no fixed hand yet: hand over at once
 Mat4 freeInverse={};if(!Invert(R.freeFromController,freeInverse))return true;
 const Mat4 expected=Multiply(Multiply(R700HoldBody(),*Bolt().hand),freeInverse);
 const V3 at={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
 return Length(Sub(at,{expected.m[3],expected.m[7],expected.m[11]}))<=radius;
}
// Ending the left-hand hold: remember where the gun was so the right-hand root takes over smoothly; two-handed stays two-handed.
void R700EndHold(bool keepTwoHand)
{
 auto& R=g_r700;if(R.hold){R.releaseFrom=R700HoldBody();R.releaseAt=GetTickCount64();}
 R.hold=R.grabbing=R.awaitGrip=false;g_twoHand=keepTwoHand&&g_handLeftValid;
}
// Nearest point on the captured handle-tip path: arc fraction s and distance (metres, gun-local).
double R700PathFraction(const V3& input,double& distance)
{
 const V3 local=A2000ChargePoint336(input);
 double bestS=0;distance=1e9;
 const auto& P=R700Path();
 for(int i=0;i+1<Bolt().strokeCount;++i)
 {
  const V3 a=P.tip[i],b=P.tip[i+1];
  const V3 ab=Sub(b,a);const double len2=Dot(ab,ab);const double t=len2>1e-12?(std::max)(0.0,(std::min)(1.0,Dot(Sub(local,a),ab)/len2)):0;
  const double d=Length(Sub(local,Add(a,Scale(ab,t))));
  if(d<distance){distance=d;bestS=Bolt().strokeS[i]+t*(Bolt().strokeS[i+1]-Bolt().strokeS[i]);}
 }
 return bestS;
}
// Part 0 (the bolt) rel the corrected FRAME.
bool R700BoltPose(int index,const Mat4& nativeRelative,Mat4& relative)
{
 if(!R700Model())return false;
 const auto& B=Bolt();
 if(B.pinSwingParts&&(index==2||index==3)){relative=*CurrentRifle().partRest[index];return true;} // never the game's shot kick
 if(index==1&&B.partFollowsBolt)
 {Mat4 bolt={},inverse={};if(!R700BoltPose(0,nativeRelative,bolt)||!Invert(*B.partRest0,inverse))return false;relative=Multiply(bolt,Multiply(inverse,*B.partRest1));return true;}
 if(index!=0)return false;
 if(g_r700.native){relative=nativeRelative;return true;}
 if(R700ClipPlaying()||(g_r700.physical&&g_r700.s>.001)){relative=R700ClipPose().bolt;return true;}
 relative=*B.partRest0;return true;
}
bool R700RightOnBolt(){return R700ClipPlaying()||R700Grabbing();}
// Right wrist on the bolt: Jason's captured hand at this point of the stroke (physical) or of the cycle (Simple), on the gun.
// Revision 7 (the wrist played the post-shot animation): the right hand's orientation came from the arm path (the game's
// own wrist rotation under the controller move), and the game's post-shot bolt cycle rotates that wrist. The R700's right hand
// is now RIGID ON THE GRIP: root x captured hand-from-FRAME, exactly like the 1887 (Lever1887RightHoldingMove). Only while the
// gun is in the left hand and the right hand is free does it follow the controller on its own.
bool R700RightClipMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!R700Model())return false;const auto& p=g_pistolReload;if(g_frame-p.rootFrame>1)return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 if(R700RightOnBolt())
 {
  const Mat4 clipMove=Multiply(Multiply(ArmBodyToCamera(),Multiply(p.rootBody,R700ClipPose().hand)),inverse);
  const double w=R700ClipPlaying()?R700ClipSeconds()/.1:1;
  move=w>=1?clipMove:PistolRigidBlend(move,clipMove,w);return true;
 }
 if(R700Hold()||p.held||PistolPoseOwned())return false; // right hand free (reaching for the bolt / handling a magazine)
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(p.rootBody,(*Bolt().hand))),inverse);return true; // on the grip, rigid
}
// Right fingers: Jason's on the bolt, otherwise ALWAYS the captured idle grip (the right hand played the handle-pull
// animation after each shot: the rifle path keeps the game's fingers while not pose-owned, and the game's own
// post-shot bolt animation runs through them).
bool R700RightGrip(float* bones)
{
 if(!R700Model())return false;
 const Mat4 wrist=BoneMatrix(bones,2);const float* fingers=R700RightOnBolt()?R700ClipPose().fingers:Bolt().rightIdle;
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));
 return true;
}
// Left wrist on the forend: Simple (during the cycle) and physical hold (the gun is pinned to this wrist anyway).
// Revision 12 (after the first empty reload the left hand still played the animation; in Simple the left arm played the
// reload animation after each shot): the log shows no game reload ran - the game still animates its arms after a shot and on an empty
// gun, and the mod built the LEFT arm from those game bones whenever the gun was held two-handed. The R700's left arm is now the
// fixed arm (stereo.cpp: side-1 stable for the R700), placed on the forend from the gun while two-handed, in the left-hand hold and
// during the Simple clip; free otherwise (and while it handles a magazine).
bool R700LeftSupportActive(){const auto& p=g_pistolReload;return R700Model()&&(R700ClipPlaying()||R700Hold()||g_twoHand)&&!p.held&&!p.removalHeld&&!PistolOwnsLeft()&&!PistolAmmoPreviewActive();}
bool R700LeftSupportMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!R700LeftSupportActive())return false;const auto& p=g_pistolReload;if(g_frame-p.rootFrame>1)return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(p.rootBody,(*Bolt().support))),inverse);return true;
}
bool R700LeftGrip(float* bones)
{
 if(!R700LeftSupportActive())return false;
 const Mat4 wrist=BoneMatrix(bones,2);
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(Bolt().leftIdle,b-3)));
 return true;
}
// ---- the game's own EMPTY reload (unchanged from the first build)
double R700NativeWeight()
{
 auto& R=g_r700;if(!R.native)return 0;
 const ULONGLONG now=GetTickCount64();
 if(!R700Model()||now-R.nativeAt>9000||(R.nativeEndAt&&now-R.nativeEndAt>150)){R.native=false;R.chambered=true;return 0;}
 const double in=(std::min)(1.0,(now-R.nativeAt)/150.0),out=R.nativeEndAt?1-(std::min)(1.0,(now-R.nativeEndAt)/150.0):1;
 return (std::min)(in,out);
}
bool R700NativeLeft(){R700NativeWeight();return g_r700.native;}
void R700LeftNativeMove(Mat4& handMove)
{
 const double w=R700NativeWeight();if(w<=0)return;
 Mat4 gun={};if(!RightHandMove(gun))return;
 handMove=w>=1?gun:PistolRigidBlend(handMove,gun,w);
}
void R700NativeBegin(UINT state,UINT wrapper)
{
 if(!R700Model()||!ReloadSignalLocal(state,wrapper))return;
 auto& R=g_r700;R.native=true;R.nativeAt=GetTickCount64();R.nativeEndAt=0;R.chambered=false;R.clip=-1;R.s=0;R.grabbing=R.reachedBack=false;
 Log("[R700] native reload: the game's own animation (bolt included); left arm follows it on the gun; fire locked until it ends");
}
void R700NativeEnd(UINT state,UINT wrapper)
{
 auto& R=g_r700;if(!R.native||!ReloadSignalLocal(state,wrapper))return;
 R.nativeEndAt=GetTickCount64();R.chambered=true;Log("[R700] native reload end: chambered");
}
bool AkInstantNativeTriple(UINT,UINT);
bool BoltAfterEveryReload(){return R700Model()&&Bolt().boltAfterEveryReload;}
// Revision 11 (on an empty reload the left hand / forearm / arm played the reload animation): the EMPTY reload no longer
// lets the game play its reload (the left arm followed it). Like every rifle: the block is seated instantly at the request seam,
// then the bolt is due - the same physical bolt as after a shot (or the Simple clip) chambers the first round. The arms stay the player's.
void R700EmptyInsert(UINT state,UINT wrapper)
{
 auto& p=g_pistolReload;
 p.pending=p.entryAccepted=p.cycle=false;fc3pad::CancelReloadRequest();g_reloadSignal.pistolPermissionUntil=0;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 p.committed=after>before;if(p.committed)++p.commits;
 ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=false;
 p.needsRack=p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=p.emptyLocked=false;p.slidePull=0;
 p.chamberReady=true;p.awaitingNativeClose=false;p.reloadChamberTracked=false;p.akRackComplete=true;
 p.fireResumeFrame=-100000;p.poseHoldUntil=0;p.waitTriggerRelease=false;p.result=1;p.contactFrames=0;
 auto& R=g_r700;R.native=false;R.clip=-1;R.chambered=false;R.s=0;R.reachedBack=R.grabbing=false;
 fc3xr::Buzz(1);
 Log("[R700] block seated into an EMPTY rifle: instant (no game animation), loaded %u -> %u, engine ok=%d; work the bolt to chamber",before,after,ok);
}
void R700NativeInsert(UINT state,UINT wrapper)
{
 auto& p=g_pistolReload;
 p.pending=p.entryAccepted=p.cycle=false;fc3pad::CancelReloadRequest();
 ProtectSeatedMagazine54();p.magazinePresent=true;p.held=p.looseFlying=false;
 p.needsRack=p.rackMode=p.rackArmed=p.rackHeld=p.rackReturning=p.rackRear=p.emptyLocked=false;p.slidePull=0;
 p.chamberReady=true;p.awaitingNativeClose=false;p.reloadChamberTracked=false;p.akRackComplete=true;
 p.fireResumeFrame=-100000;p.poseHoldUntil=0;p.waitTriggerRelease=false;p.result=1;p.contactFrames=0;
 g_r700.chambered=false;
 const bool granted=GrantPistolReloadPermission(state,wrapper);
 if(!granted){AkInstantNativeTriple(state,wrapper);g_r700.chambered=true;}
 fc3xr::Buzz(1);
 Log("[R700] magazine seated into an EMPTY rifle at the request seam: %s",granted?"native reload permitted, the game's own animation plays":"permission refused, seated instantly");
}
void UpdateR700Fire(const fc3pad::Controllers& c)
{
 auto& R=g_r700;fc3pad::RequireFullFirePull(false);
 const bool active=R700Active();R700NativeWeight();
 const bool block=active&&(!R.chambered||R.clip>=0||R.native||R.hold);
 fc3pad::BlockFire(block);
 const bool pulled=c.rightTrigger>.7f;
 if(active&&pulled&&!R.triggerWas)
 {
  if(!block&&!fc3pad::MacheteFireBlocked()){R.chambered=false;R.shotAt=GetTickCount64();++R.shots;if(R.shots<=20)Log("[R700] shot %ld: trigger locked until the bolt is cycled (%s)",R.shots,R.physical?"LEFT grip the forend, RIGHT grip the bolt":"RIGHT grip");}
  else if(InterlockedIncrement(&R.refused)<=40)Log("[R700] shot refused: %s",R.native?"the game's reload is playing":R.hold?"the gun is in the left hand":R.clip>=0?"Jason is working the bolt":"bolt not cycled");
 }
 R.triggerWas=pulled;
}
void TickR700(bool gameplay)
{
 auto& R=g_r700;const auto c=fc3pad::Latest();const ULONGLONG now=GetTickCount64();auto& p=g_pistolReload;
 const bool rg=c.rightGrip>.7f,rgEdge=rg&&!R.rgWas,rgUp=c.rightGrip<.35f,lg=c.leftGrip>.7f,lgEdge=lg&&!R.leftGripWas;R.rgWas=rg;R.leftGripWas=lg;
 const bool clipActive223=gameplay&&R700Active()&&(R.physical||g_handLeftValid);
 if(AdvanceWeaponClip223(R.clipAt,R.clipPauseAt223,R.clip>=0,clipActive223,now))R.poseFrame=-100000;
 if(!clipActive223||(R.physical&&R.hold&&!g_handLeftValid)){R.grabbing=false;if(!R700Model()){R.clip=-1;R.clipPauseAt223=0;R.native=false;R.hold=R.reachedBack=false;R.s=0;}return;}
 if(R.native){R.hold=R.grabbing=false;return;}
 if(!R.physical)
 { // Simple: RIGHT grip plays the cycle on the gun in the right hand (left hand snaps to the forend meanwhile)
  R.hold=R.grabbing=false;
  if(R.clip>=0){if(R700ClipSeconds()>=Bolt().cycleSeconds){R.clip=-1;R.chambered=true;++R.cycles;fc3xr::Buzz(1);Log("[R700] bolt cycled (%ld): chambered",R.cycles);}return;}
  if(rgEdge&&!R.chambered&&!PistolPoseOwned()){R.clip=0;R.clipAt=now;R.clipPauseAt223=0;R.poseFrame=-100000;Log("[R700] RIGHT grip: Jason works the bolt");}
  return;
 }
 R.clip=-1;
 // hand switch (after every shot the weapon moves into the left-hand grip, like the regular two-handed grip):
 // while the bolt is due and the left hand holds the forend (g_twoHand, set by UpdateTwoHand when LEFT grip is within 15 cm of
 // the support), the gun is pinned to the left hand exactly where it is. A LEVEL condition: revision 1 wanted a LEFT-grip EDGE
 // while two-handed, but two-handed is only set after that very edge, so it never fired (log: shot locked, no [R700 HANDS]).
 if(!R.hold&&g_twoHand&&!R.chambered&&!PistolPoseOwned()&&g_frame-p.rootFrame<=1)
 {
  {R700BeginHold();(void)now;++R.holds;fc3xr::Buzz(1);Log("[R700 HANDS] two-handed with the bolt due: gun moved to the left hand (switch %ld); RIGHT grip the bolt handle",R.holds);}
 }
 else if(!R.hold&&!R.chambered&&lg&&!g_twoHand&&g_frame-p.rootFrame<=1)
 { // LEFT grip held but not on the forend: say how far (rate limited), so the next log explains a missing switch
  static ULONGLONG lastLog=0;if(now-lastLog>1500){lastLog=now;const V3 support=Apply(Multiply(p.rootBody,(*Bolt().support)),{-.591,.011,1.149}),real=OtherMotion319(1,ControllerWrist(g_handBody[1],1));
   Log("[R700 HANDS] LEFT grip held but the left hand is %.2f m from the forend (needs < 0.15 for the two-handed hold): gun stays in the right hand",Length(Sub(real,support)));}
 }
 else if(R.hold&&c.leftGrip<.35f)
 {R700EndHold(false);fc3xr::Buzz(0);Log("[R700 HANDS] left hand let go; gun back in the right hand (bolt at %.2f, %s)",R.s,R.chambered?"chambered":"NOT chambered");}
 if(!R.hold)return;
 const double scale=(std::max)(.1f,g_worldScale);

 V3 palm={};const bool palmOk=R700RightPalmLocal(palm);
 if(!R.grabbing&&rgEdge&&palmOk)
 {
  V3 palmBody={};R700RightPalmBody(palmBody);const float keepDiameter=g_pistolGuides.handDiameter;g_pistolGuides.handDiameter=R.rightDiameter;
  const bool inBox=PistolGuideContact(p.rootBody,3,palmBody);g_pistolGuides.handDiameter=keepDiameter; // the RIGHT hand's own sphere
  static LONG tries=0;if(InterlockedIncrement(&tries)<=40)Log("[R700 BOLT] grip at gun-local %.3f %.3f %.3f: %s the BOLT box",palm.x,palm.y,palm.z,inBox?"inside":"outside");
  if(inBox)
  {
   R.grabbing=true;R.grabStart=palm;R.grabS=R.s;for(int i=0;i<16;++i)R.grabController.m[i]=g_handBody[0][i];R.grabController=OtherController319(0,R.grabController);++R.grabs;fc3xr::Buzz(0);
   Mat4 rootInverse={};if(Invert(p.rootBody,rootInverse)){R.grabY=Apply(rootInverse,{R.grabController.m[3],R.grabController.m[7],R.grabController.m[11]}).y;
    // Preserve an already partly closed bolt on re-grab. The remaining push
    // starts at this controller position, not a reset to the fully rear pose.
    R.backY=R.grabY-(R.reachedBack?(1-R.s)*.05*scale:0);}
   Log("[R700 BOLT] right hand on the bolt handle (stroke %.2f%s)",R.s,R.reachedBack?", already back: push it forward":"");
  }
 }
 if(R.grabbing)
 { // Revision 10 (the bolt worked only some of the time; log: pulls of 14-22 cm, then home only counted
   // once the hand came all the way back to the grab point, so a long pull or a re-grab at the back left the bolt open).
   // Two phases along the barrel (gun -Y), each measured from where it starts:
   //  1. pull: 6 cm back from the grab point = fully back (buzz). Pulling further changes nothing.
   //  2. push: 5 cm forward from the FURTHEST-back point reached (or from a re-grab at the back) = home, chambered, buzz, and the
   //     gun goes straight back to the right hand ready to fire.
  Mat4 rootInverse={};if(Invert(p.rootBody,rootInverse))
  {
   Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];controller=OtherController319(0,controller);
   const double y=Apply(rootInverse,{controller.m[3],controller.m[7],controller.m[11]}).y,pull=.06*scale,push=.05*scale;
   if(!R.reachedBack)
   {
    R.s=(std::max)(0.0,(std::min)(1.0,R.grabS+(R.grabY-y)/pull));
    if(R.s>=1){R.reachedBack=true;R.backY=y;fc3xr::Buzz(0);Log("[R700 BOLT] fully back: push it forward");}
   }
   else
   {
    R.backY=(std::min)(R.backY,y); // further back moves the push start with the hand
    const double forward=y-R.backY;R.s=(std::max)(0.0,(std::min)(1.0,1-forward/push));
    if(forward>=push)
    {
     R.s=0;R.reachedBack=false;R.chambered=true;++R.cycles;R700EndHold(fc3pad::Latest().leftGrip>.6f);fc3xr::Buzz(1);
     Log("[R700 BOLT] bolt home (%ld): chambered; gun back in the right hand, ready to fire",R.cycles);return;
    }
   }
   if(now-R.strokeLogAt>400){R.strokeLogAt=now;static LONG logs=0;if(InterlockedIncrement(&logs)<=60)Log("[R700 BOLT] stroke %.2f (%s)",R.s,R.reachedBack?"pushing forward":"pulling back");}
  }
  if(rgUp){R.grabbing=false;Log("[R700 BOLT] let go at %.2f%s",R.s,R.reachedBack?" (fully back: grab it again and push forward)":"");}
 }
}
// INSERT > R700 (settings: [R700] BoltMode 0 physical / 1 simple, ShowGuides)
void LoadR700Settings(const wchar_t* explicitPath)
{
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);
 g_r700.physical=GetPrivateProfileIntW(L"R700",L"BoltMode",0,file)!=1;g_r700.showGuides=GetPrivateProfileIntW(L"R700",L"ShowGuides",1,file)!=0;
 const wchar_t* keys[4]={L"RightHandX",L"RightHandY",L"RightHandZ",L"RightHandDiameter"};float* values[4]={&g_r700.rightPalm[0],&g_r700.rightPalm[1],&g_r700.rightPalm[2],&g_r700.rightDiameter};
 for(int i=0;i<4;++i){wchar_t t[64]={};GetPrivateProfileStringW(L"R700",keys[i],L"",t,64,file);wchar_t* e=nullptr;const float f=static_cast<float>(wcstod(t,&e));
  if(e!=t&&!*e&&std::isfinite(f)&&(i<3?std::fabs(f)<=.5f:f>=.01f&&f<=.20f))*values[i]=f;}
}
bool SaveR700Settings(const wchar_t* explicitPath=nullptr)
{
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);
 bool ok=WritePrivateProfileStringW(L"R700",L"BoltMode",g_r700.physical?L"0":L"1",file)!=FALSE;
 ok=WritePrivateProfileStringW(L"R700",L"ShowGuides",g_r700.showGuides?L"1":L"0",file)!=FALSE&&ok;
 {const wchar_t* keys[4]={L"RightHandX",L"RightHandY",L"RightHandZ",L"RightHandDiameter"};const float values[4]={g_r700.rightPalm[0],g_r700.rightPalm[1],g_r700.rightPalm[2],g_r700.rightDiameter};
  for(int i=0;i<4;++i){wchar_t t[64];_snwprintf_s(t,64,_TRUNCATE,L"%.6f",values[i]);ok=WritePrivateProfileStringW(L"R700",keys[i],t,file)!=FALSE&&ok;}}
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,file);return ok;
}
#ifdef FC3_AMMO_TEST
ImGuiID g_boltSaveId53=0;
#endif
void R700Menu(const wchar_t* savePath)
{
 auto& R=g_r700;static int result=0;
 {int mode=R.physical?0:1;const char* modes[]={"Physical (LEFT grip = gun in the left hand, RIGHT grip works the bolt)","Simple (RIGHT grip = Jason's bolt cycle)"};ImGui::SetNextItemWidth(320*g_menuScale);
  if(ImGui::Combo("Bolt mode",&mode,modes,2)){R.physical=mode==0;R.clip=-1;R.hold=R.grabbing=R.reachedBack=false;R.s=0;result=0;}}
 ImGui::TextWrapped(R.physical?"After a shot: LEFT grip the forend (gun stays where it is, in the left hand), RIGHT grip inside the BOLT box, lift and pull the handle back, push it home. Let go of LEFT grip for the right hand.":"After a shot: RIGHT grip plays the game's bolt cycle; the left hand holds the forend meanwhile.");
 if(R.physical)
 { // the RIGHT hand sphere that grabs the bolt (shown in the headset while the bolt is due)
  ImGui::TextUnformatted("Right hand sphere (grabs the bolt)");
  const char* labels[4]={"Right hand X","Right hand Y","Right hand Z","Right hand diameter"};float* values[4]={&R.rightPalm[0],&R.rightPalm[1],&R.rightPalm[2],&R.rightDiameter};
  const float defaults[4]={.0658f,.0481f,-.1174f,.12f};
  for(int i=0;i<4;++i){ImGui::PushID(4800+i);float cm=*values[i]*100.f;ImGui::SetNextItemWidth(220*g_menuScale);
   if(ImGui::SliderFloat(labels[i],&cm,i<3?-30.f:1.f,i<3?30.f:20.f,"%.2f cm")){*values[i]=cm*.01f;result=0;}
   ImGui::SameLine();if(ImGui::Button("Default")){*values[i]=defaults[i];result=0;}ImGui::PopID();}
 }
 if(ImGui::Button("Save bolt settings##R700BoltSettings"))result=SaveR700Settings(savePath)?1:2;
#ifdef FC3_AMMO_TEST
 g_boltSaveId53=ImGui::GetItemID();
#endif
 ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");
}
