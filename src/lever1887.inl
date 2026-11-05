#include "weapon_clip_clock223.inl"
int g_leverGuideTarget=0;
// 1887 hand switch + lever. History:
//  1 (ACCEPTED 3169BAE6) LEFT grip moves the gun into the left hand, right hand free, release gives it back.
//  2 (ACCEPTED FE9B1622) lever: every shot locks the trigger; with the gun in the left hand, RIGHT grip at the
//    lever loop + ~4 cm pull + release cycles it; lever/hammer drawn from the hand.
//  3/4 removed the grab flip and the native post-shot animation (always-locked right hand, stable right arm).
//  5 (reported: wood parts still animating by themselves on the right hand; the left-hand grip sometimes correct,
//    sometimes floating above the hand; INSERT controls requested; the gun turning upside down on the left-to-right
//    hand-over):
//    - every part uses a FIXED rest on the gun body (PID56072 quiet frame 10600: those parts never move against
//      the body in the native cycle), lever/hammer from the hand. Nothing depends on draw order or native motion.
//    - left hold = drawn left wrist * normal native forend hold (frame 10600) * user adjustment. Same pose every
//      time, set in INSERT > 1887 and saved.
//    - right hold = right controller * learned attachment * user adjustment; stable right arm on the gun.
//    - (superseded) hand-offs were slerped over 150 ms.
//  6 (the transition must snap immediately; the normal two-handed hold was lost - restore it, and while two-handed
//    let the left hand pull the weapon away):
//    - hand-offs SNAP: no blend either way.
//    - LEFT grip on the forend = normal two-handed hold again: the locked gun turns about the right hand toward the
//      real left wrist (AK support pattern) and the left hand sits on the forend (native support pose, frame 10600).
//    - while two-handed, pulling the left hand away from the forend by more than PullAway (default 12 cm, INSERT)
//      moves the gun into the left hand. Releasing LEFT grip gives it back to the right hand.
//  7 (taking it from the right hand: press X while still holding the grip button):
//    the switch is X pressed while two-handed (LEFT grip held). Pull-away removed.
//  8 (remove the X reload so reloading is fully manual; add manual shell insertion):
//    the game's X / automatic reload is refused for the 1887; X never reaches the game while the left hand is on it.
//    With the gun in the left hand the free RIGHT grip takes a shell (weapon parts have priority), holds
//    it in the native carry pose (PID56072 frames 11639-11651) and pushes it into the native seat point (frame 11662):
//    one round at once through the engine's own reload routines (AK instant seam; one shell per call on the 1887).
//    Loading an empty gun leaves the chamber empty (lever before firing); a top-up keeps the chambered round.
//  9 (switching from right to left only with an empty chamber, via grip + RT on the right controller; reloading
//    needs the lever down and the top compartment open):
//    - switch: two-handed (LEFT grip on the forend) + empty chamber + RIGHT grip and RIGHT trigger together.
//      Back to the right hand: release LEFT grip. (X no longer switches.)
//    - the lever has a state. Grab it at the loop and pull along the native lever path (PID56072 palm 10518 -> 10528):
//      it opens and STAYS open when released. Grab again and push back: it closes and chambers a round.
//      One continuous pull-and-push also cycles it. Trigger locked while the lever is open.
//    - shells only go in while the lever is open.
//  10 (grip + RT for the left hold worked poorly: use LEFT grip + left stick click, keeping grip + RT as an INSERT
//    bool option; the right hand vanished in the normal two-handed hold; the one-handed aim was
//    wrong):
//    - default switch: two-handed + LEFT stick click (any chamber state). INSERT bool `SwitchWithRightTrigger` restores
//      RIGHT grip + trigger (empty chamber only). The left-stick click never reaches the game while the left hand is on it.
//    - right hand vanishing: stereo.cpp chained the LEFT forend placement onto the RIGHT arm (if/else), fixed there.
//    - aim follows the displayed 1887 barrel (FRAME +Y), one- and two-handed (Lever1887AimBody).
//  11 (two-handed aiming put the crosshair out of view; also allow X while
//    holding the grip): the two-handed turn read the left wrist in CAMERA space (g_handLeft, re-expressed with the aim
//    rotation) and converted back with the CURRENT aim rotation; the aim then moved the camera, which moved that reading,
//    which moved the aim: a feedback loop. The left wrist is now read in BODY space (g_handBody[1]), the same space as
//    the right controller, so the camera cannot feed back. Switch: LEFT grip + left stick click OR X; INSERT option
//    adds RIGHT grip + trigger (empty chamber only).
//  12 (a Simple mode, chosen from an INSERT dropdown: after a shot, RIGHT grip or a shake of the right controller works
//    the lever; to reload, X pulls the lever down, each grip press adds a shell, X closes; the regular animations play
//    step by step):
//    INSERT "Reload mode": Manual (all of the above) or Simple. Simple keeps the gun in the right hand and plays the game's
//    own 1887 animation, captured as clips (PID56072, work/prepare_1887_clips.py), one step per input:
//      after a shot: RIGHT grip or a shake -> lever cycle clip, chambers;  X -> reload-start clip (lever down, stays down);
//      RIGHT grip while open -> one shell clip, +1 round at the native seat moment;  X again -> close clip, chambers.
//    The engine cannot step its own reload one shell at a time (it runs to full), so the clips are replayed by the mod.
//  13 (reloading continued even with a full tube; the lever should only work while
//    holding with 2 hands): log showed loaded 37 -> 38: the engine transfer called directly does NOT stop at capacity.
//    Every insert (Manual and Simple) now reads the weapon's HUD ammo widget (loaded/capacity/reserve, hud+0x134) and is
//    refused when full or out of reserve. Simple: before a shell clip or opening the lever the tube is checked (a
//    request probe learns the weapon the first time); full = no animation, short buzz. Every Simple lever / shell step
//    needs the two-handed hold (LEFT grip on the forend).
#include "lever1887_data.inl"
#include "lever1887_clips.inl"
struct Lever1887
{
 bool enabled=true,hold=false,menuPreview=false;ULONGLONG holdAt=0,releaseAt=0;
 Mat4 lastHoldBody={},parentRel={},candidate={},releaseRel={};bool parentValid=false;UINT parentEpoch=0;int stableFrames=0;
 Mat4 displayBody={};LONG displayFrame=-100000;
 Mat4 leftHandBody={};LONG leftHandFrame=-100000;Mat4 rightHandBody={};LONG rightHandFrame=-100000;
 bool chambered=true,passing=false,waitRelease=false;LONG shotFrame=-100000;float lastTrigger=0;
 bool grabbing=false,full=false,rightGripWas=false,xWas=false,rtWas=false;
 bool leverOpen=false,grabFromOpen=false,reachedOpen=false,switchRightTrigger=false,l3Was=false;double frac=0;
 bool shell=false,pending=false;ULONGLONG deadline=0;int contactFrames=0,pickup=2;LONG inserts=0;
 float gate[3]={kL1887GateDefault[0],kL1887GateDefault[1],kL1887GateDefault[2]},gateRadius=.07f;V3 grabStart={};double stroke=0;ULONGLONG returnAt=0;double returnFrom=0;
 // User adjustments (INSERT > 1887): gun-local metres and degrees.
 float left[6]={},right[6]={};
 bool guideBox[2]={false,false};float guideSize[2][3]={},guideRot[2][3]={}; // lever, loading gate; legacy sphere until edited
 bool supportEdited56=false;float supportPosition56[3]={},supportSize56[3]={.12f,.18f,.12f},supportRotation56[3]={};
 float palmOffset56[2][3]={},palmDiameter56[2]={.10f,.10f}; // right, left; actual palm-local calibration
 V3 grabMotionStart56={}; // raw controller stroke, separate from visible/contact palm
 float lever[3]={kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]},grabRadius=.15f,pull=.04f,pullAway=.12f;
 LONG holds=0,cycles=0;
 // Simple mode (game animation clips).
 bool simple=false,insertFired=false,poseValid=false;int clip=-1,shellsSinceOpen=0;ULONGLONG clipAt=0,clipPauseAt223=0,clipPulseAt223=0;
 UINT ammoState=0,ammoWrapper=0;int probeClip=-1;bool probing=false;ULONGLONG probeDeadline=0;
 struct Pose{Mat4 hand,lever,hammer,shell,cradle;float fingers[204];} pose={},blendFrom={};LONG poseFrame=-100000;
 V3 shakePos[16]={};ULONGLONG shakeAt[16]={};int shakeHead=0;
} g_lever;
struct L1887ClipRef{const L1887Key* keys;int count;float insert;};
const L1887ClipRef kL1887Clips[5]={
 {kL1887ClipCycle,static_cast<int>(sizeof(kL1887ClipCycle)/sizeof(*kL1887ClipCycle)),kL1887ClipCycleInsert},
 {kL1887ClipOpen,static_cast<int>(sizeof(kL1887ClipOpen)/sizeof(*kL1887ClipOpen)),kL1887ClipOpenInsert},
 {kL1887ClipShellFirst,static_cast<int>(sizeof(kL1887ClipShellFirst)/sizeof(*kL1887ClipShellFirst)),kL1887ClipShellFirstInsert},
 {kL1887ClipShellLoop,static_cast<int>(sizeof(kL1887ClipShellLoop)/sizeof(*kL1887ClipShellLoop)),kL1887ClipShellLoopInsert},
 {kL1887ClipClose,static_cast<int>(sizeof(kL1887ClipClose)/sizeof(*kL1887ClipClose)),kL1887ClipCloseInsert}};
enum {kL1887Cycle=0,kL1887Open=1,kL1887ShellFirst=2,kL1887ShellLoop=3,kL1887Close=4};
bool Lever1887Model(){return g_weaponMechanicProfile==0&&g_frame-g_weaponMechanicFrame<=2;}
bool Lever1887Active(){return g_lever.enabled&&Lever1887Model()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool Lever1887Holding(){return g_lever.hold||(g_pistolMenu.open&&g_lever.menuPreview);}
bool Lever1887LeftHold(){return Lever1887Active()&&Lever1887Holding();}
const wchar_t* kLever1887Keys[]={L"LeftX",L"LeftY",L"LeftZ",L"LeftRotX",L"LeftRotY",L"LeftRotZ",L"RightX",L"RightY",L"RightZ",L"RightRotX",L"RightRotY",L"RightRotZ",L"LeverX",L"LeverY",L"LeverZ",L"LeverGrabRadius",L"LeverPull",L"PullAway",L"GateX",L"GateY",L"GateZ",L"GateRadius"};
float* Lever1887Value(int n){auto& L=g_lever;return n<6?&L.left[n]:n<12?&L.right[n-6]:n<15?&L.lever[n-12]:n==15?&L.grabRadius:n==16?&L.pull:n==17?&L.pullAway:n<21?&L.gate[n-18]:&L.gateRadius;}
float Lever1887Default(int n){return n<12?0.f:n<15?kL1887LeverPoint[n-12]:n==15?.15f:n==16?.04f:n==17?.12f:n<21?kL1887GateDefault[n-18]:.07f;}
void LoadLever1887Settings(const wchar_t* path)
{
 auto& L=g_lever;const wchar_t* s=L"Lever1887";
 L.enabled=GetPrivateProfileIntW(s,L"HandSwitch",1,path)!=0;
 L.switchRightTrigger=GetPrivateProfileIntW(s,L"SwitchWithRightTrigger",0,path)!=0;
 L.simple=GetPrivateProfileIntW(s,L"ReloadMode",0,path)==1;
 L.pickup=GetPrivateProfileIntW(s,L"ShellPickup",GetPrivateProfileIntW(L"AkReload",L"AmmoPickup",2,path),path);if(L.pickup<0||L.pickup>2)L.pickup=2;
 for(int n=0;n<22;++n){wchar_t t[64]={};GetPrivateProfileStringW(s,kLever1887Keys[n],L"",t,64,path);wchar_t* e=nullptr;const float v=static_cast<float>(wcstod(t,&e));
  *Lever1887Value(n)=e==t||*e||!std::isfinite(v)?Lever1887Default(n):v;}
 for(int b=0;b<2;++b){const wchar_t* name=b?L"Gate":L"Lever";wchar_t key[64],text[64];swprintf_s(key,L"%sBoxEnabled",name);L.guideBox[b]=GetPrivateProfileIntW(s,key,0,path)!=0;
  for(int axis=0;axis<3;++axis)for(int rot=0;rot<2;++rot){swprintf_s(key,L"%sBox%s%c",name,rot?L"Rotate":L"Size",L'X'+axis);GetPrivateProfileStringW(s,key,L"",text,64,path);wchar_t* end=nullptr;float value=float(wcstod(text,&end));const float fallback=rot?0:2*(b?L.gateRadius:L.grabRadius);if(end==text||*end||!std::isfinite(value)||(rot?fabs(value)>180:value<.01f||value>.6f))value=fallback;(rot?L.guideRot[b][axis]:L.guideSize[b][axis])=value;}}
 L.supportEdited56=GetPrivateProfileIntW(s,L"SupportEdited",0,path)!=0;for(int n=0;n<9;++n){wchar_t key[64],text[64];swprintf_s(key,L"Support%s%c",n<3?L"Position":n<6?L"Size":L"Rotation",L'X'+n%3);GetPrivateProfileStringW(s,key,L"",text,64,path);wchar_t* end=nullptr;float value=float(wcstod(text,&end));if(end!=text&&!*end&&std::isfinite(value)&&(n<3?fabs(value)<=2:n<6?value>=.01f&&value<=.6f:fabs(value)<=180))*(n<3?&L.supportPosition56[n]:n<6?&L.supportSize56[n-3]:&L.supportRotation56[n-6])=value;}
 for(int side=0;side<2;++side)for(int n=0;n<4;++n){wchar_t key[64],text[64];if(n<3)swprintf_s(key,L"%sPalm%c",side?L"Left":L"Right",L'X'+n);else swprintf_s(key,L"%sPalmDiameter",side?L"Left":L"Right");GetPrivateProfileStringW(s,key,L"",text,64,path);wchar_t* end=nullptr;float value=float(wcstod(text,&end));if(end==text||*end||!std::isfinite(value)||(n<3?fabs(value)>.5:value<.01f||value>.3f))value=n<3?0:.10f;*(n<3?&L.palmOffset56[side][n]:&L.palmDiameter56[side])=value;}
 Log("[1887 HANDS] switch %s; left adj %.3f %.3f %.3f / %.1f %.1f %.1f; right adj %.3f %.3f %.3f / %.1f %.1f %.1f; lever %.3f %.3f %.3f",L.enabled?"on":"off",
     L.left[0],L.left[1],L.left[2],L.left[3],L.left[4],L.left[5],L.right[0],L.right[1],L.right[2],L.right[3],L.right[4],L.right[5],L.lever[0],L.lever[1],L.lever[2]);
}
bool SaveLever1887Settings(const wchar_t* explicitPath=nullptr)
{
 auto& L=g_lever;
 wchar_t path[MAX_PATH];ManualSettingsPath(path);if(explicitPath)wcscpy_s(path,explicitPath);bool ok=true;
 for(int n=0;n<22;++n){wchar_t t[64];_snwprintf_s(t,64,_TRUNCATE,L"%.6f",*Lever1887Value(n));ok=WritePrivateProfileStringW(L"Lever1887",kLever1887Keys[n],t,path)!=FALSE&&ok;}
 {wchar_t t[16];_snwprintf_s(t,16,_TRUNCATE,L"%d",L.pickup);ok=WritePrivateProfileStringW(L"Lever1887",L"ShellPickup",t,path)!=FALSE&&ok;}
 ok=WritePrivateProfileStringW(L"Lever1887",L"SwitchWithRightTrigger",L.switchRightTrigger?L"1":L"0",path)!=FALSE&&ok;
 ok=WritePrivateProfileStringW(L"Lever1887",L"ReloadMode",L.simple?L"1":L"0",path)!=FALSE&&ok;
 for(int b=0;b<2;++b){const wchar_t* name=b?L"Gate":L"Lever";wchar_t key[64],text[64];swprintf_s(key,L"%sBoxEnabled",name);ok=WritePrivateProfileStringW(L"Lever1887",key,L.guideBox[b]?L"1":L"0",path)!=FALSE&&ok;for(int axis=0;axis<3;++axis)for(int rot=0;rot<2;++rot){swprintf_s(key,L"%sBox%s%c",name,rot?L"Rotate":L"Size",L'X'+axis);float value=rot?L.guideRot[b][axis]:(L.guideSize[b][axis]>0?L.guideSize[b][axis]:2*(b?L.gateRadius:L.grabRadius));swprintf_s(text,L"%.8g",double(value));ok=WritePrivateProfileStringW(L"Lever1887",key,text,path)!=FALSE&&ok;}}
 ok=WritePrivateProfileStringW(L"Lever1887",L"SupportEdited",L.supportEdited56?L"1":L"0",path)!=FALSE&&ok;for(int n=0;n<9;++n){wchar_t key[64],text[64];swprintf_s(key,L"Support%s%c",n<3?L"Position":n<6?L"Size":L"Rotation",L'X'+n%3);float value=*(n<3?&L.supportPosition56[n]:n<6?&L.supportSize56[n-3]:&L.supportRotation56[n-6]);swprintf_s(text,L"%.8g",double(value));ok=WritePrivateProfileStringW(L"Lever1887",key,text,path)!=FALSE&&ok;}
 for(int side=0;side<2;++side)for(int n=0;n<4;++n){wchar_t key[64],text[64];if(n<3)swprintf_s(key,L"%sPalm%c",side?L"Left":L"Right",L'X'+n);else swprintf_s(key,L"%sPalmDiameter",side?L"Left":L"Right");swprintf_s(text,L"%.8g",double(n<3?L.palmOffset56[side][n]:L.palmDiameter56[side]));ok=WritePrivateProfileStringW(L"Lever1887",key,text,path)!=FALSE&&ok;}
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);Log("[1887 HANDS] settings saved: %d",ok);return ok;
}
Mat4 Lever1887Adjust(const float* a)
{
 Mat4 m=HandIdentity();
 for(int axis=0;axis<3;++axis){const double r=a[3+axis]*3.141592653589793/180;Mat4 x=HandIdentity();const int u=(axis+1)%3,v=(axis+2)%3;x.m[u*4+u]=x.m[v*4+v]=std::cos(r);x.m[u*4+v]=-std::sin(r);x.m[v*4+u]=std::sin(r);m=Multiply(m,x);}
 m.m[3]=a[0];m.m[7]=a[1];m.m[11]=a[2];return m;
}
Mat4 Lever1887GuideLocal52(int box,bool open=false,bool mirror=false){
 const auto& L=g_lever;float adjustment[6]={};const float* point=box?L.gate:L.lever;for(int i=0;i<3;++i){adjustment[i]=point[i];adjustment[3+i]=L.guideBox[box]?L.guideRot[box][i]:0;}
 if(!box&&open)for(int i=0;i<3;++i)adjustment[i]+=kL1887LeverOpenPoint[i]-kL1887LeverPoint[i];if(mirror)adjustment[0]=-adjustment[0];return Lever1887Adjust(adjustment);
}
float Lever1887GuideSize52(int box,int axis){const auto& L=g_lever;return L.guideBox[box]&&L.guideSize[box][axis]>0?L.guideSize[box][axis]:2*(box?L.gateRadius:L.grabRadius);}
bool Lever1887GuideContact52(int box,V3 point,bool open=false,bool mirror=false,double handRadius=0){
 const auto& L=g_lever;const double scale=(std::max)(.1f,g_worldScale);const Mat4 local=Lever1887GuideLocal52(box,open,mirror);
 if(!L.guideBox[box])return Length(Sub(point,{local.m[3],local.m[7],local.m[11]}))<=(box?L.gateRadius:L.grabRadius)*scale+handRadius;
 Mat4 inverse={};if(!Invert(local,inverse))return false;const V3 p=Apply(inverse,point);const double v[]={p.x,p.y,p.z};double d2=0;for(int a=0;a<3;++a){const double d=(std::max)(0.0,fabs(v[a])-Lever1887GuideSize52(box,a)*scale*.5);d2+=d*d;}return d2<=handRadius*handRadius;
}
// Shortest-path rigid blend: quaternion slerp for rotation, linear translation.
void Lever1887Quat(const Mat4& m,double q[4])
{
 const double tr=m.m[0]+m.m[5]+m.m[10];
 if(tr>0){const double s=std::sqrt(tr+1)*2;q[0]=.25*s;q[1]=(m.m[9]-m.m[6])/s;q[2]=(m.m[2]-m.m[8])/s;q[3]=(m.m[4]-m.m[1])/s;}
 else if(m.m[0]>m.m[5]&&m.m[0]>m.m[10]){const double s=std::sqrt(1+m.m[0]-m.m[5]-m.m[10])*2;q[0]=(m.m[9]-m.m[6])/s;q[1]=.25*s;q[2]=(m.m[1]+m.m[4])/s;q[3]=(m.m[2]+m.m[8])/s;}
 else if(m.m[5]>m.m[10]){const double s=std::sqrt(1+m.m[5]-m.m[0]-m.m[10])*2;q[0]=(m.m[2]-m.m[8])/s;q[1]=(m.m[1]+m.m[4])/s;q[2]=.25*s;q[3]=(m.m[6]+m.m[9])/s;}
 else{const double s=std::sqrt(1+m.m[10]-m.m[0]-m.m[5])*2;q[0]=(m.m[4]-m.m[1])/s;q[1]=(m.m[2]+m.m[8])/s;q[2]=(m.m[6]+m.m[9])/s;q[3]=.25*s;}
}
Mat4 Lever1887Slerp(const Mat4& a,const Mat4& b,double t)
{
 t=(std::max)(0.0,(std::min)(1.0,t));if(t<=0)return a;if(t>=1)return b;
 double p[4],q[4];Lever1887Quat(a,p);Lever1887Quat(b,q);double d=p[0]*q[0]+p[1]*q[1]+p[2]*q[2]+p[3]*q[3];
 if(d<0){for(double& v:q)v=-v;d=-d;}
 double r[4];
 if(d>.9995){for(int i=0;i<4;++i)r[i]=p[i]+(q[i]-p[i])*t;}
 else{const double th=std::acos(d),s=std::sin(th),wa=std::sin((1-t)*th)/s,wb=std::sin(t*th)/s;for(int i=0;i<4;++i)r[i]=wa*p[i]+wb*q[i];}
 const double n=std::sqrt(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]+r[3]*r[3]);for(double& v:r)v/=n;
 const double w=r[0],x=r[1],y=r[2],z=r[3];Mat4 m=HandIdentity();
 m.m[0]=1-2*(y*y+z*z);m.m[1]=2*(x*y-z*w);m.m[2]=2*(x*z+y*w);m.m[4]=2*(x*y+z*w);m.m[5]=1-2*(x*x+z*z);m.m[6]=2*(y*z-x*w);m.m[8]=2*(x*z-y*w);m.m[9]=2*(y*z+x*w);m.m[10]=1-2*(x*x+y*y);
 for(int i=0;i<3;++i)m.m[i*4+3]=a.m[i*4+3]+(b.m[i*4+3]-a.m[i*4+3])*t;return m;
}
double Lever1887Fraction()
{
 const auto& L=g_lever;
 return L.grabbing?L.frac:(L.leverOpen?1.0:0.0); // lever angle: open/closed state, or follows the hand while grabbed
}
// Fire gate, from UpdatePump while the 1887 is equipped (same allowance as the accepted M133 pump).
void UpdateLever1887Fire(const fc3pad::Controllers& c)
{
 auto& L=g_lever;
 if(!Lever1887Active()){L.passing=false;L.lastTrigger=c.rightTrigger;fc3pad::RequireFullFirePull(false);fc3pad::BlockFire(false);return;}
 const bool held=Lever1887Holding(),hardBlock=held||L.leverOpen||L.grabbing||L.clip>=0;
 if(hardBlock)L.passing=false;
 if(L.chambered&&!hardBlock&&!fc3pad::MacheteFireBlocked()&&c.rightTrigger>.7f&&L.lastTrigger<=.7f)
 {L.chambered=false;L.shotFrame=g_frame;L.passing=true;L.waitRelease=true;static LONG shots=0;if(InterlockedIncrement(&shots)<=5)Log("[1887 LEVER] shot; trigger locked until the lever is cycled");}
 L.lastTrigger=c.rightTrigger;
 if(c.rightTrigger<.3f||g_frame-L.shotFrame>=6)L.passing=false;
 if(!L.chambered)L.waitRelease=true;
 if(L.chambered&&c.rightTrigger<.3f)L.waitRelease=false;
 fc3pad::RequireFullFirePull(true);
 fc3pad::BlockFire(hardBlock||(!L.passing&&(!L.chambered||L.waitRelease)));
}
double Lever1887PalmRadius56(int side){return side>=0&&side<2?g_lever.palmDiameter56[side]*.5*(std::max)(.1f,g_worldScale):0;}
bool Lever1887PalmBody56(int side,V3& palm)
{
 if(side<0||side>1||!(side?g_handLeftValid:g_handRightValid))return false;
 if(ShotgunPalm318(side,g_lever.palmOffset56[side],palm))return true;
 const auto& attachment=g_freeHandAttachment[side];Mat4 hand={};
 if(attachment.epoch==g_armPoseEpoch&&attachment.profile==g_weaponMechanicProfile&&g_frame-attachment.frame<=2){
  Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[side][i];hand=Multiply(controller,attachment.relative);
 }else{
  const LONG frame=side?g_lever.leftHandFrame:g_lever.rightHandFrame;if(frame<0||(!g_pistolMenu.open&&g_frame-frame>1))return false;
  hand=side?g_lever.leftHandBody:g_lever.rightHandBody;
 }
 const auto& offset=g_lever.palmOffset56[side];const double scale=(std::max)(.1f,g_worldScale);
 palm=Apply(hand,{(side?-.6781:.6781)+offset[0]*scale,.0519+offset[1]*scale,1.0387+offset[2]*scale});return true;
}
bool Lever1887RawRightLocal56(V3& local)
{
 Mat4 inverse={};if(!g_handRightValid||g_frame-g_lever.displayFrame>1||!Invert(g_lever.displayBody,inverse))return false;
 local=Apply(inverse,ShotgunMotion318(0,ControllerWrist(g_handBody[0],0)));return true;
}
bool Lever1887RightPalmLocal(V3& local)
{
 const auto& L=g_lever;Mat4 inverse={};V3 palm={};
 if(g_frame-L.displayFrame>1||!Invert(L.displayBody,inverse)||!Lever1887PalmBody56(0,palm))return false;
 local=Apply(inverse,palm);return true;
}
bool Lever1887PickupZone(int mode,const V3& hand)
{
 Mat4 view={},inverse={};for(int i=0;i<16;++i)view.m[i]=g_eyeViewsBody[0].view[i];
 if(!Invert(view,inverse))return false;
 const double scale=(std::max)(.1f,g_worldScale);const V3 head=Apply(inverse,{0,0,0});
 const V3 offset=mode==0?V3{LeftShotgun318()?-.18:.18,-.65,.03}:V3{0,-.38,.12}; // mirror of the left-hand pistol waist / chest zones
 return Length(Sub(hand,Add(head,Scale(offset,scale))))<=.18*scale;
}
bool Lever1887ParentReady(){
 auto& L=g_lever;
 if((!L.parentValid||L.parentEpoch!=g_armPoseEpoch)&&Lever1887Active()){
  Mat4 root={},controller={},inverse={};for(int i=0;i<16;++i)controller.m[i]=g_handBody[0][i];
  if(ControllerGripRoot124(kL1887RightHandFromFrame,nullptr,root)&&Invert(controller,inverse)){
   L.parentRel=Multiply(inverse,root);L.parentValid=true;L.parentEpoch=g_armPoseEpoch;
  }
 }
 return L.parentValid&&L.parentEpoch==g_armPoseEpoch;
}
Mat4 Lever1887RightBody(){Mat4 r={};for(int i=0;i<16;++i)r.m[i]=g_handBody[0][i];return Multiply(Multiply(r,g_lever.parentRel),Lever1887Adjust(g_lever.right));}
V3 Lever1887SupportPoint(const Mat4& gun){V3 point=Apply(kL1887LeftSupportFromFrame,{-.591,.011,1.149});if(LeftShotgun318())point.x=-point.x;return Apply(gun,point);}
Mat4 Lever1887GunBody();
Mat4 Lever1887SupportBox56(){
 const auto& L=g_lever;const V3 fallback=Apply(kL1887LeftSupportFromFrame,{-.6781,.0519,1.0387});float a[6];for(int i=0;i<3;++i){a[i]=L.supportEdited56?L.supportPosition56[i]:float(i==0?fallback.x:i==1?fallback.y:fallback.z);a[3+i]=L.supportRotation56[i];}Mat4 local=Lever1887Adjust(a);if(LeftShotgun318()){const auto mirror=SidearmMirror315();local=Multiply(mirror,Multiply(local,mirror));}const Mat4 placed=Multiply(Lever1887ParentReady()?Lever1887GunBody():L.displayBody,local);Mat4 box={};if(!HandBasis(placed,box))return {};box.m[3]=placed.m[3];box.m[7]=placed.m[7];box.m[11]=placed.m[11];return box;
}
double Lever1887SupportDistance56(){
 if(!Lever1887Model()||!g_handLeftValid||g_lever.displayFrame<0||(!g_pistolMenu.open&&g_frame-g_lever.displayFrame>1))return 1e9;Mat4 inverse={};if(!Invert(Lever1887SupportBox56(),inverse))return 1e9;V3 palm={};if(!Lever1887PalmBody56(1,palm))return 1e9;V3 local=Apply(inverse,palm);double d2=0;const double v[]={local.x,local.y,local.z};for(int i=0;i<3;++i){const double d=(std::max)(0.0,fabs(v[i])-g_lever.supportSize56[i]*(std::max)(.1f,g_worldScale)*.5);d2+=d*d;}return sqrt(d2);
}
V3 Lever1887RealLeftWrist(){return ShotgunMotion318(1,ControllerWrist(g_handBody[1],1));} // body space: independent of the aim-driven camera
// The gun in the right hand: locked attachment, turned about the right hand toward the real left wrist while two-handed.
Mat4 Lever1887GunBody()
{
 Mat4 root=Lever1887RightBody();
 if(g_twoHand&&!g_lever.hold)
 {
  const V3 pivot=Apply(Multiply(root,kL1887RightHandFromFrame),{.591,.011,1.149}),support=Lever1887SupportPoint(root),real=Lever1887RealLeftWrist();
  if(Length(Sub(support,pivot))>.05&&Length(Sub(real,pivot))>.05)root=Multiply(TurnAndMove(Sub(support,pivot),Sub(real,pivot),pivot,pivot),root);
 }
 return root;
}
// How far the real left wrist is from the forend of the two-handed gun (pull-away takes it into the left hand).
double Lever1887PullAway(){return Length(Sub(Lever1887RealLeftWrist(),Lever1887SupportPoint(Lever1887ParentReady()?Lever1887GunBody():g_lever.displayBody)));}
// Aim along the displayed 1887 barrel (FRAME +Y), like the pistols/rifles.
bool Lever1887AimBody(V3& forward)
{
 if(!Lever1887Active()||!Lever1887ParentReady()||Lever1887Holding())return false;
 const Mat4 gun=Lever1887GunBody();const V3 barrel={gun.m[1],gun.m[5],gun.m[9]};const double length=Length(barrel);
 if(!std::isfinite(length)||length<1e-6)return false;
 forward=Scale(barrel,1/length);return true;
}
bool Lever1887LeftBody(Mat4& body)
{
 const auto& L=g_lever;Mat4 inverse={};
 if(!Invert(kL1887LeftSupportFromFrame,inverse))return false;
 Mat4 wrist=L.leftHandBody;
 if(LeftShotgun318(true)){
  // The gun and both reload hands must use this frame's same physical wrist.
  // A record written after drawing an arm otherwise moves the parent midway
  // through the frame, leaving the first hand beside the newly drawn gun.
  Mat4 actual={};if(!ShotgunPhysicalWrist318(1,actual))return false;
  // The right support grasp is the native left grasp reflected across the
  // forend centre. Anchor the gun to that authored wrist, not a translated
  // left-hand wrist (which leaves fingers on the wrong side of the wood).
  const auto mirror=SidearmMirror315();const auto support=Multiply(mirror,Multiply(kL1887LeftSupportFromFrame,mirror));
  Mat4 supportInverse={};if(!Invert(support,supportInverse))return false;
  body=Multiply(Multiply(actual,supportInverse),Lever1887Adjust(L.left));return true;
 }else if(g_frame-L.leftHandFrame>2)return false;
 body=Multiply(Multiply(wrist,inverse),Lever1887Adjust(L.left));return true;
}
Mat4 L1887KeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
void L1887BlendPose(const Lever1887::Pose& a,const Lever1887::Pose& b,double t,Lever1887::Pose& out)
{
 out.hand=PistolRigidBlend(a.hand,b.hand,t);out.lever=PistolRigidBlend(a.lever,b.lever,t);out.hammer=PistolRigidBlend(a.hammer,b.hammer,t);
 out.shell=PistolRigidBlend(a.shell,b.shell,t);out.cradle=PistolRigidBlend(a.cradle,b.cradle,t);
 for(int b3=0;b3<17;++b3){const Mat4 m=PistolRigidBlend(BoneMatrix(a.fingers,b3),BoneMatrix(b.fingers,b3),t);for(int i=0;i<12;++i)out.fingers[b3*12+i]=static_cast<float>(m.m[i]);}
}
void L1887KeyPose(const L1887Key& k,Lever1887::Pose& out)
{out.hand=L1887KeyMat(k.hand);out.lever=L1887KeyMat(k.lever);out.hammer=L1887KeyMat(k.hammer);out.shell=L1887KeyMat(k.shell);out.cradle=L1887KeyMat(k.cradle);memcpy(out.fingers,k.fingers,sizeof(out.fingers));}
void L1887SampleClip(int clip,double t,Lever1887::Pose& out)
{
 const auto& c=kL1887Clips[clip];int i=0;while(i+1<c.count&&c.keys[i+1].t<=t)++i;
 Lever1887::Pose a={},b={};L1887KeyPose(c.keys[i],a);
 if(i+1>=c.count){out=a;return;}
 L1887KeyPose(c.keys[i+1],b);const double span=c.keys[i+1].t-c.keys[i].t;L1887BlendPose(a,b,span>1e-6?(t-c.keys[i].t)/span:1,out);
}
void L1887QuietPose(Lever1887::Pose& out)
{out.hand=kL1887RightHandFromFrame;out.lever=kL1887LeverRest;out.hammer=g_lever.chambered?kL1887HammerCocked:kL1887HammerFired;out.shell=kL1887ShellBRest;out.cradle=kL1887CradleRest;memcpy(out.fingers,kL1887RightQuietGripRelative,sizeof(out.fingers));}
bool Lever1887SimplePoseActive(){return g_lever.simple?(g_lever.clip>=0||g_lever.leverOpen):g_lever.clip>=0;}
// The pose shown this frame in Simple mode: the playing clip (blended in over 80 ms) or the held open pose.
const Lever1887::Pose& Lever1887SimplePose()
{
 auto& L=g_lever;
 if(L.poseFrame==g_frame&&L.poseValid)return L.pose;
 if(L.clip>=0){
  const double t=WeaponClipElapsed223(L.clipAt,L.clipPauseAt223);Lever1887::Pose s={};L1887SampleClip(L.clip,t,s);
  if(t<.08)L1887BlendPose(L.blendFrom,s,t/.08,L.pose);else L.pose=s;L.poseValid=true;
 }
 else if(!L.poseValid){L1887SampleClip(kL1887Open,1e9,L.pose);L.poseValid=true;}
 L.poseFrame=g_frame;return L.pose;
}
void Lever1887StartClip(int clip)
{
 auto& L=g_lever;
 if(Lever1887SimplePoseActive()&&L.poseValid)L.blendFrom=Lever1887SimplePose();else L1887QuietPose(L.blendFrom);
 L.clip=clip;L.clipAt=GetTickCount64();L.clipPauseAt223=0;L.insertFired=false;L.poseFrame=-100000;
 static const char* names[]={"lever cycle","reload start (lever down)","first shell","next shell","close lever"};
 Log("[1887 SIMPLE] %s",names[clip]);
}
bool Lever1887Shake()
{
 auto& L=g_lever;const ULONGLONG now=GetTickCount64();const V3 pos={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
 L.shakePos[L.shakeHead]=pos;L.shakeAt[L.shakeHead]=now;L.shakeHead=(L.shakeHead+1)%16;
 const double need=.12*(std::max)(.1f,g_worldScale);
 for(int i=0;i<16;++i)if(L.shakeAt[i]&&now-L.shakeAt[i]<=150&&Length(Sub(pos,L.shakePos[i]))>=need){for(auto& a:L.shakeAt)a=0;return true;}
 return false;
}
// The weapon's HUD ammo widget: loaded / capacity / reserve. False = not seen.
bool AmmoRead(UINT,void*,SIZE_T);
bool Lever1887Ammo(UINT wrapper,UINT& loaded,UINT& capacity,UINT& reserve)
{
 if(!wrapper)return false;
 for(const auto& widget:g_reload.widgets)
 {
  if(static_cast<UINT>(widget.wrapper)!=wrapper||!widget.hud)continue;
  UINT w[5]={};if(!AmmoRead(static_cast<UINT>(widget.hud)+0x134,w,sizeof(w)))continue;
  if(w[1]<1||w[1]>128||w[3]>100000)continue;
  loaded=w[0];capacity=w[1];reserve=w[3];return true;
 }
 return false;
}
// 1 = full (or no reserve), 0 = room, -1 = unknown (weapon not learned yet).
int Lever1887Full()
{
 const auto& L=g_lever;UINT loaded=0,capacity=0,reserve=0;
 if(!L.ammoWrapper||!ReloadSignalLocal(L.ammoState,L.ammoWrapper)||!Lever1887Ammo(L.ammoWrapper,loaded,capacity,reserve))return -1;
 const UINT weapon=ReloadRead(L.ammoWrapper,0x48),count=weapon?ReloadRead(weapon,0xcc):loaded;
 return ((std::max)(loaded,count)>=capacity||!reserve)?1:0;
}
// Shell / open step: refused when full; unknown weapon -> one request probe (no load) learns it first.
void Lever1887StartLoadStep(int clip)
{
 auto& L=g_lever;const int full=Lever1887Full();
 if(full==1){fc3xr::Buzz(0);static LONG n=0;if(InterlockedIncrement(&n)<=10)Log("[1887 SIMPLE] tube full (or no reserve): %s refused",clip==kL1887Open?"lever down":"shell");return;}
 if(full==0){Lever1887StartClip(clip);return;}
 L.probeClip=clip;L.probing=true;L.probeDeadline=GetTickCount64()+400;L.pending=true;L.deadline=L.probeDeadline;fc3pad::RequestReload();
}
void TickLever1887Simple(bool rgEdge,bool xEdge)
{
 auto& L=g_lever;const ULONGLONG now=GetTickCount64();
 L.hold=L.grabbing=L.shell=false;
 const bool shake=Lever1887Shake();
 if(L.clip>=0)
 {
  const auto& c=kL1887Clips[L.clip];const double t=WeaponClipElapsed223(L.clipAt,L.clipPauseAt223);
  if(c.insert>=0&&!L.insertFired&&t>=c.insert){L.insertFired=true;L.pending=true;L.deadline=now+1500;L.clipPulseAt223=now;fc3pad::RequestReload();}
  if(L.pending&&now-L.clipPulseAt223>=250){L.clipPulseAt223=now;fc3pad::RequestReload();}
  if(t>=c.keys[c.count-1].t)
  {
   if(L.pending)return; // retain the final seat pose until transfer or bounded timeout
   L.poseValid=false;Lever1887SimplePose();const int done=L.clip;L.clip=-1;
   if(done==kL1887Cycle||done==kL1887Close){L.leverOpen=false;L.chambered=true;++L.cycles;fc3xr::Buzz(1);}
   else if(done==kL1887Open){L.leverOpen=true;L.chambered=false;L.shellsSinceOpen=0;fc3xr::Buzz(0);}
   else ++L.shellsSinceOpen;
  }
  return; // one step at a time
 }
 if(L.probeClip>=0)
 { // waiting for the request probe to name the weapon
  if(!L.probing){const int clip=L.probeClip;L.probeClip=-1;if(Lever1887Full()==1){fc3xr::Buzz(0);Log("[1887 SIMPLE] tube full (or no reserve): refused");}else Lever1887StartClip(clip);}
  else if(now>L.probeDeadline){L.probing=L.pending=false;fc3pad::CancelReloadRequest();const int clip=L.probeClip;L.probeClip=-1;Log("[1887 SIMPLE] ammo probe unanswered; step allowed, insert still checks capacity");Lever1887StartClip(clip);}
  return;
 }
 if(!g_twoHand)return; // lever and shells only with both hands on the gun
 if(xEdge&&fc3pad::AmmoPickupOverride(fc3pad::AmmoControls())){Lever1887StartLoadStep(L.leverOpen?(L.shellsSinceOpen?kL1887ShellLoop:kL1887ShellFirst):kL1887Open);return;}
 if(xEdge){if(L.leverOpen)Lever1887StartClip(kL1887Close);else Lever1887StartLoadStep(kL1887Open);return;}
 if(rgEdge){if(L.leverOpen)Lever1887StartLoadStep(L.shellsSinceOpen?kL1887ShellLoop:kL1887ShellFirst);else if(!L.chambered)Lever1887StartClip(kL1887Cycle);return;}
 if(shake&&!L.leverOpen&&!L.chambered)Lever1887StartClip(kL1887Cycle);
}
void TickLever1887(bool gameplay)
{
 auto& L=g_lever;const auto c=fc3pad::AmmoControls();
 fc3pad::SetLeverHandling(gameplay&&Lever1887Active()&&(L.hold||g_twoHand)); // left hand on the 1887: RB/LB (and A with right grip) stay with the mod
 fc3pad::SetLeverXOwned(gameplay&&Lever1887Active()&&L.simple); // Simple: X (lever) and RIGHT grip (lever / shells) never reach the game
 const bool rg=c.rightGrip>.7f,rgEdge=rg&&!L.rightGripWas,rgUp=c.rightGrip<.35f,rt=c.rightTrigger>.7f,rtEdge=rt&&!L.rtWas,xEdge=c.x&&!L.xWas;
 const bool l3=c.leftStickClick&&!c.rightStickClick,l3Edge=l3&&!L.l3Was;
 L.rightGripWas=rg;L.xWas=c.x;L.rtWas=rt;L.l3Was=l3;
 const ULONGLONG now223=GetTickCount64();
 const ULONGLONG resumed223=AdvanceWeaponClip223(L.clipAt,L.clipPauseAt223,L.clip>=0,gameplay&&Lever1887Active(),now223);
 if(resumed223){if(L.clipPulseAt223)L.clipPulseAt223+=resumed223;if(L.pending){L.deadline+=resumed223;fc3pad::RequestReload();}L.poseFrame=-100000;}
 if(!gameplay||!Lever1887Active())
 {if(!Lever1887Model()){if(L.pending)fc3pad::CancelReloadRequest();L.hold=L.grabbing=L.shell=L.pending=false;L.releaseAt=L.returnAt=0;L.parentValid=false;L.stableFrames=0;L.clip=L.probeClip=-1;L.probing=false;L.clipPauseAt223=0;}return;} // tracking loss keeps hand, lever and chamber
 const ULONGLONG now=GetTickCount64();
 if(L.pending&&now>L.deadline&&!L.probing){
  L.pending=false;fc3pad::CancelReloadRequest();
  if(!L.simple){L.shell=true;L.contactFrames=0;Log("[1887 SHELL] request timed out; shell stays in the hand");}
 }
 if(L.simple){TickLever1887Simple(rgEdge,xEdge);return;}
 if(L.clip>=0){
  const auto& clip=kL1887Clips[L.clip];const double elapsed=WeaponClipElapsed223(L.clipAt,L.clipPauseAt223);
  if(!L.insertFired&&elapsed>=clip.insert){L.insertFired=true;L.pending=true;L.deadline=now+1500;L.clipPulseAt223=now;fc3pad::RequestReload();}
  if(L.pending&&now-L.clipPulseAt223>=250){L.clipPulseAt223=now;fc3pad::RequestReload();}
  if(elapsed>=clip.keys[clip.count-1].t&&!L.pending){L.clip=-1;L.poseValid=false;++L.shellsSinceOpen;}
  return; // preserve gun/hand ownership during the short native seating clip
 }
 auto releaseLever=[&](){
  L.grabbing=false;const bool open=L.frac>=.5,wasOpen=L.grabFromOpen||L.reachedOpen;
  if(open&&!L.leverOpen){L.leverOpen=true;L.chambered=false;fc3xr::Buzz(0);Log("[1887 LEVER] lever left open; loading gate open, trigger locked");}
  else if(!open&&wasOpen){L.leverOpen=false;L.chambered=true;++L.cycles;fc3xr::Buzz(1);Log("[1887 LEVER] lever closed (%ld); chambered",L.cycles);}
  else L.leverOpen=open;};
 // When the lever is due, the support grip becomes the holding hand. The
 // same nearby foregrip used for ordinary support qualifies a fresh grab.
 const bool leverDue=!L.chambered||L.leverOpen;
 const bool supportFresh=L.displayFrame==g_frame&&L.leftHandFrame==g_frame;
 const bool nearSupport=supportFresh&&Lever1887SupportDistance56()<=Lever1887PalmRadius56(1);
 if(!L.hold&&leverDue&&c.leftGrip>.7f&&(g_twoHand||nearSupport))
 {L.hold=true;L.holdAt=now;L.releaseAt=0;g_twoHand=false;++L.holds;fc3xr::Buzz(1);Log("[1887 HANDS] lever due: LEFT support grip moved gun to left hand (switch %ld)",L.holds);}
 else if(L.hold&&c.leftGrip<.35f)
 {
  L.hold=false;L.releaseAt=0;if(L.pending)fc3pad::CancelReloadRequest();L.shell=L.pending=false;if(L.grabbing)releaseLever();
  fc3xr::Buzz(0);Log("[1887 HANDS] left hand let go; gun back in the right hand (lever %s)",L.leverOpen?"open":"closed");
 }
 if(!L.hold)return;
 const double scale=(std::max)(.1f,g_worldScale);
 const V3 hand={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
 // Lever: grab at the loop, move along the native lever path; position follows the hand, release keeps open/closed.
 V3 palm={},motion={};const bool palmOk=Lever1887RightPalmLocal(palm),motionOk=Lever1887RawRightLocal56(motion);const double palmRadius=Lever1887PalmRadius56(0);
 const V3 path=Unit(Sub(V3{kL1887LeverOpenPoint[0],kL1887LeverOpenPoint[1],kL1887LeverOpenPoint[2]},V3{kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]}));
 const V3 loop=L.leverOpen?Add(V3{L.lever[0],L.lever[1],L.lever[2]},Sub(V3{kL1887LeverOpenPoint[0],kL1887LeverOpenPoint[1],kL1887LeverOpenPoint[2]},V3{kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]})):V3{L.lever[0],L.lever[1],L.lever[2]};
 // Either side of the gun (pull from the right or the left side): the same loop point mirrored across the
 // gun's centre plane (FRAME X = 0); pulling still follows the native lever path.
 const V3 loopLeft={-loop.x,loop.y,loop.z};const bool leftSide=palmOk&&Length(Sub(palm,loopLeft))<Length(Sub(palm,loop));
 if(!L.grabbing&&!L.shell&&!L.pending&&rgEdge&&palmOk)
 {static LONG tries=0;if(InterlockedIncrement(&tries)<=40)Log("[1887 LEVER] grip at gun-local %.3f %.3f %.3f: right loop %.3f, left loop %.3f (radius %.3f)",palm.x,palm.y,palm.z,Length(Sub(palm,loop)),Length(Sub(palm,loopLeft)),L.grabRadius*scale);}
 const bool leverContact=palmOk&&(Lever1887GuideContact52(0,palm,L.leverOpen,false,palmRadius)||Lever1887GuideContact52(0,palm,L.leverOpen,true,palmRadius));
 const bool gateContact=palmOk&&Lever1887GuideContact52(1,palm,false,false,palmRadius);
 if(!L.grabbing&&!L.shell&&!L.pending&&rgEdge&&leverContact&&motionOk)
 {L.grabbing=true;L.grabFromOpen=L.leverOpen;L.reachedOpen=L.leverOpen;L.grabStart=palm;L.grabMotionStart56=motion;L.frac=L.leverOpen?1:0;fc3xr::Buzz(0);Log("[1887 LEVER] right hand grabbed the lever from the %s side (%s)",leftSide?"left":"right",L.leverOpen?"open":"closed");}
 if(L.grabbing)
 {
  if(motionOk){const double need=(std::max)(.005,static_cast<double>(L.pull)*scale);L.frac=(std::max)(0.0,(std::min)(1.0,(L.grabFromOpen?1.0:0.0)+Dot(Sub(motion,L.grabMotionStart56),path)/need));}
  if(L.frac>=1)L.reachedOpen=true;
  if(rgUp)releaseLever();
 }
 // Physical: the free RIGHT grip takes a shell. Lever/gate contacts win over
 // implicit ammo pickup. Both pickup paths require the lever open so a shell
 // cannot occupy the hand needed to open it.
 if(L.leverOpen&&!L.grabbing&&!L.shell&&!L.pending)
 {
  const bool take=(xEdge&&fc3pad::AmmoPickupOverride(c))||
   (rgEdge&&palmOk&&!leverContact&&!gateContact&&(fc3pad::GripOnlyAmmoEnabled()||L.pickup==2||Lever1887PickupZone(L.pickup,hand)));
  if(take){L.shell=true;L.contactFrames=0;fc3xr::Buzz(0);Log("[1887 SHELL] right hand took a shell (pickup %d)",L.pickup);}
 }
 if(L.shell&&rgUp){L.shell=false;L.contactFrames=0;Log("[1887 SHELL] shell let go away from the gate; dropped, no ammo change");}
 if(L.shell&&L.leverOpen&&!L.pending&&g_frame-L.rightHandFrame<=1&&g_frame-L.displayFrame<=1)
 {
  Mat4 inverse={};
  if(Invert(L.displayBody,inverse))
  {
   const Mat4 shellBody=ShotgunHeld318(L.rightHandBody,kL1887ShellFromHand,0,false);
   const V3 local=Apply(inverse,{shellBody.m[3],shellBody.m[7],shellBody.m[11]});
   const double distance=Length(Sub(local,{L.gate[0],L.gate[1],L.gate[2]}));
   if(Lever1887GuideContact52(1,local)){if(++L.contactFrames>=2){
    Lever1887StartClip(L.shellsSinceOpen?kL1887ShellLoop:kL1887ShellFirst);
    const double lead=(std::max)(0.0,double(kL1887Clips[L.clip].insert)-.18);
    L.clipAt=now-ULONGLONG(lead*1000);L.shell=false;L.contactFrames=0;
    Log("[1887 SHELL231] gate contact starts native shell seating; engine request at captured seat time (distance %.3f)",distance);
   }}
   else L.contactFrames=0;
  }
 }
}
// Engine request seam (game thread, pre-predicate): one shell per insertion, no animation.
void Lever1887Request(UINT state,UINT wrapper)
{
 auto& L=g_lever;
 if(!Lever1887Active()||!ReloadSignalLocal(state,wrapper))return;
 L.ammoState=state;L.ammoWrapper=wrapper; // the 1887's own request: remember the weapon for capacity checks
 if(L.clip>=0&&(L.clipPauseAt223||g_pistolMenu.open||fc3pad::GameplayInputBlocked()))return;
 if(!L.pending||GetTickCount64()>L.deadline)return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 L.pending=false;fc3pad::CancelReloadRequest();
 if(L.probing){L.probing=false;return;} // probe only: no load
 // Recheck the physical gate at the game-thread seat, not just when the
 // render-side contact requested it. No transfer is allowed without capacity.
 if(!L.simple&&(!L.hold||!L.leverOpen)){
  L.shell=true;L.contactFrames=0;Log("[1887 INSERT] gate closed before seat; shell retained");return;
 }
 {UINT loaded=0,capacity=0,reserve=0;
  if(!Lever1887Ammo(wrapper,loaded,capacity,reserve)){
   if(!L.simple)L.shell=true;L.contactFrames=0;fc3xr::Buzz(0);
   Log("[1887 INSERT] capacity unavailable; no transfer, shell retained");return;
  }
  if((std::max)(loaded,before)>=capacity||!reserve)
  {if(!L.simple)L.shell=true;fc3xr::Buzz(0);Log("[1887 INSERT] refused: tube %u/%u, reserve %u (engine transfer does not stop at capacity)",(std::max)(loaded,before),capacity,reserve);return;}}
 const bool ok=AkInstantNativeTriple(state,wrapper);
 const UINT after=ReloadRead(weapon,0xcc);
 if(after>before){++L.inserts;fc3xr::Buzz(1);} // the lever is open: closing it chambers the round
 else if(!L.simple){L.shell=true;L.contactFrames=0;} // native refusal never consumes the physical shell
 Log("[1887 INSERT] native Begin/transfer/End synchronous, loaded %u -> %u, engine ok=%d, chamber %s",before,after,ok,L.chambered?"loaded":"empty (cycle the lever)");
}
// Fixed rest of every 1887 draw on the gun body (null = unknown draw: ordinary path).
const Mat4* Lever1887Rest(bool main,UINT start,UINT count)
{
 static const Mat4 identity=HandIdentity();
 if(!main)return start==0&&count==42?&kL1887TinyRest:nullptr;
 if(start==42&&count==10899)return &identity;
 if(start==10941&&count==2685)return &kL1887AmmoRest;
 if(start==20796&&count==108)return &kL1887ShellARest;
 if(start==20904&&count==360)return &kL1887ShellBRest;
 if(start==23220&&count==378)return &kL1887CradleRest;
 return nullptr;
}
bool Lever1887FinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,const Mat4& nativeWorld,float* globals)
{
 if(!Lever1887Active())return false;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=217840||d.indexBytes!=47196||d.format!=57)return false;
 auto& L=g_lever;(void)nativeWorld;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return false;
 const Mat4 view=HullView(camera);
 const bool main=d.stride==32&&d.offset==400;
 const bool root=main&&start==42&&count==10899,leverDraw=main&&start==13626&&count==7170,hammerDraw=main&&start==21264&&count==1956;
 Mat4 viewInverse={},bodyInverse={};
 if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return false;
 const ULONGLONG now=GetTickCount64();
 const bool holding=Lever1887Holding(),parent=Lever1887ParentReady();(void)now;
 if(!holding&&!parent)
 {
  // Learn the right-hand attachment once the native gun has sat still in the hand (never during the equip animation).
  if(root)
  {
   const Mat4 rightBody=Multiply(bodyInverse,Multiply(view,HullWorld(globals))); // ordinary right-hand path
   Mat4 r={},ri={};for(int i=0;i<16;++i)r.m[i]=g_handBody[0][i];
   if(fc3pad::Latest().rightTrigger<.3f&&g_frame-L.shotFrame>100&&Invert(r,ri))
   {
    const Mat4 candidate=Multiply(ri,rightBody);double change=0;
    for(int i=0;i<12;++i)change=(std::max)(change,std::fabs(candidate.m[i]-L.candidate.m[i]));
    L.stableFrames=change<.004?L.stableFrames+1:0;L.candidate=candidate;
    if(L.stableFrames>=15){L.parentRel=candidate;L.parentValid=true;L.parentEpoch=g_armPoseEpoch;Log("[1887 HANDS] right-hand attachment learned from a still native hold");}
   }
   L.displayBody=rightBody;L.displayFrame=g_frame;
  }
  return false; // ordinary right-hand path until the attachment is learned
 }
 Mat4 body=parent?Lever1887GunBody():L.displayBody;
 if(holding){Mat4 leftBody={};if(Lever1887LeftBody(leftBody)){body=leftBody;L.lastHoldBody=leftBody;}else body=L.lastHoldBody;} // snap, no blend
 if(root){L.displayBody=body;L.displayFrame=g_frame;}
 if(main&&((start==20796&&count==108)||(start==20904&&count==360))&&L.shell&&g_frame-L.rightHandFrame<=1)
 {WritePistolWorld(globals,Multiply(viewInverse,Multiply(ArmBodyToCamera(),ShotgunHeld318(L.rightHandBody,kL1887ShellFromHand,0,false))));return true;}
 if(Lever1887SimplePoseActive()&&main)
 {
  const auto& s=Lever1887SimplePose();const Mat4* sp=leverDraw?&s.lever:hammerDraw?&s.hammer:((start==20796&&count==108)||(start==20904&&count==360))?&s.shell:(start==23220&&count==378)?&s.cradle:nullptr;
  if(sp){WritePistolWorld(globals,Multiply(Multiply(viewInverse,Multiply(ArmBodyToCamera(),body)),*sp));return true;}
 }
 const Mat4* rest=Lever1887Rest(main,start,count);
 Mat4 relative=rest?*rest:HandIdentity();
 if(leverDraw)relative=PistolRigidBlend(kL1887LeverRest,kL1887LeverOpen,Lever1887Fraction());
 else if(hammerDraw)relative=(L.chambered&&!L.grabbing)||L.leverOpen||L.reachedOpen?kL1887HammerCocked:PistolRigidBlend(kL1887HammerFired,kL1887HammerCocked,L.grabbing?(std::min)(1.0,Lever1887Fraction()/.66):0);
 else if(!rest)return false; // unknown draw: leave it on the ordinary path
 WritePistolWorld(globals,Multiply(Multiply(viewInverse,Multiply(ArmBodyToCamera(),body)),relative));
 return true;
}
// Right arm: the stable arm (no native animation) once the attachment is learned; with the gun in the right
// hand its wrist sits on the gun in the native quiet pose.
bool Lever1887RightStable(){return Lever1887Active()&&Lever1887ParentReady();}
bool Lever1887RightHoldingMove(const Mat4& toView,const float* bones,Mat4& move)
{
 const auto& L=g_lever;
 if(!Lever1887RightStable()||(Lever1887Holding()&&L.clip<0))return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 Mat4 root=Lever1887GunBody();if(Lever1887Holding()){if(!Lever1887LeftBody(root))root=L.lastHoldBody;}
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(root,Lever1887SimplePoseActive()?Lever1887SimplePose().hand:kL1887RightHandFromFrame)),inverse);return true;
}
// Two-handed: the left hand sits on the forend of the locked gun (native support pose).
bool Lever1887LeftSupportMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!Lever1887RightStable()||Lever1887Holding()||!g_twoHand)return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(Lever1887GunBody(),kL1887LeftSupportFromFrame)),inverse);return true;
}
bool Lever1887SupportPosition(V3& support)
{
 if(!Lever1887RightStable()||Lever1887Holding())return false;
 support=Apply(Multiply(ArmBodyToCamera(),Multiply(Lever1887GunBody(),kL1887LeftSupportFromFrame)),{-.591,.011,1.149});return true;
}
bool Lever1887RightGrip(float* bones)
{
 // Lever grab: native lever fingers. Gun in the right hand: quiet holding grip.
 if(!Lever1887Active())return false;
 const float* pose=Lever1887SimplePoseActive()?Lever1887SimplePose().fingers:Lever1887Holding()?(g_lever.shell?kL1887ShellGripRelative:g_lever.grabbing?kL1887RightLeverGripRelative:nullptr):kL1887RightQuietGripRelative;
 if(!pose)return false;
 const Mat4 wrist=BoneMatrix(bones,2);
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(pose,b-3)));
 return true;
}
void Record1887Hand(ID3D11DeviceContext* context,const float* bones,Mat4& body,LONG& frame)
{
 if(!Lever1887Active())return;
 Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 body=Multiply(inverse,view);frame=g_frame;
}
void Record1887RightHand(ID3D11DeviceContext* context,const float* bones){Record1887Hand(context,bones,g_lever.rightHandBody,g_lever.rightHandFrame);}
void Record1887LeftHand(ID3D11DeviceContext* context,const float* bones){Record1887Hand(context,bones,g_lever.leftHandBody,g_lever.leftHandFrame);}
bool Lever1887LeftGrip(float* bones)
{
 if(!Lever1887LeftHold()&&!(Lever1887RightStable()&&g_twoHand))return false;
 const Mat4 wrist=BoneMatrix(bones,2);
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(kL1887LeftQuietGripRelative,b-3))); // forend grip
 return true;
}
// INSERT > 1887.
extern float g_menuScale;
void Lever1887Menu(const wchar_t* savePath)
{
 auto& L=g_lever;static int result=0;
 ImGui::TextWrapped("Set where the 1887 sits in each hand. Changes apply live. Save All stores them.");
 {int mode=L.simple?1:0;const char* modes[]={"Manual (left hand, lever, shells by hand)","Simple (buttons + game animations)"};ImGui::SetNextItemWidth(320*g_menuScale);
  if(ImGui::Combo("Reload mode",&mode,modes,2)){L.simple=mode==1;L.clip=-1;L.hold=L.grabbing=L.shell=false;result=0;}}
 if(L.simple)ImGui::TextWrapped("Simple (both hands on the gun): after a shot, RIGHT grip or shake = lever. X = lever down, RIGHT grip = one shell each, X = close.");
 ImGui::Checkbox("Show the gun in the LEFT hand while adjusting",&L.menuPreview);
 ImGui::TextWrapped("Manual: LEFT grip at the forend holds the gun while the lever is due. RIGHT grip at the loop works the lever; with the lever open, away from weapon parts it takes a shell. Release LEFT grip to return the gun. Both grips + X overrides ammo pickup.");
 auto slider=[&](int n,const char* label){
  ImGui::PushID(1887+n);float* v=Lever1887Value(n);const bool angle=(n%6)>=3&&n<12;const bool size=n==15||n==16||n==17||n==21;
  float shown=angle?*v:*v*100.f;
  ImGui::SetNextItemWidth((std::max)(55.f*g_menuScale,(std::min)(250.f*g_menuScale,ImGui::GetContentRegionAvail().x-110.f*g_menuScale)));
  if(ImGui::SliderFloat(label,&shown,angle?-180.f:size?1.f:-30.f,angle?180.f:30.f,angle?"%.1f deg":"%.1f cm")){*v=angle?shown:shown*.01f;result=0;}
  ImGui::SameLine();if(ImGui::Button("Default")){*v=Lever1887Default(n);result=0;}ImGui::PopID();};
 ImGui::Separator();const char* targets[]={"Loading gate (blue)","Lever grab (blue)","Two-handed holding (yellow)"};ImGui::Combo("Adjust guide",&g_leverGuideTarget,targets,3);
 const bool support=g_leverGuideTarget==2;const int b=g_leverGuideTarget==0?1:0;
 const char* labels[]={"Position X","Position Y","Position Z","Size X","Size Y","Size Z","Rotation X","Rotation Y","Rotation Z"};
 const V3 nativeSupport=Apply(kL1887LeftSupportFromFrame,{-.6781,.0519,1.0387});
 for(int n=0;n<9;++n){ImGui::PushID(5600+n);const int axis=n%3;
  float* value=support?(n<3?&L.supportPosition56[axis]:n<6?&L.supportSize56[axis]:&L.supportRotation56[axis]):n<3?(b?&L.gate[axis]:&L.lever[axis]):n<6?&L.guideSize[b][axis]:&L.guideRot[b][axis];
  float raw=*value;if(support&&n<3&&!L.supportEdited56)raw=float(axis==0?nativeSupport.x:axis==1?nativeSupport.y:nativeSupport.z);if(!support&&n>=3&&n<6)raw=Lever1887GuideSize52(b,axis);
  float shown=raw*(n>=6?1.f:100.f);if(ImGui::SliderFloat(labels[n],&shown,n>=6?-180.f:n>=3?1.f:-200.f,n>=6?180.f:n>=3?60.f:200.f,n>=6?"%.1f deg":"%.2f cm")){
   if(support&&!L.supportEdited56){L.supportPosition56[0]=float(nativeSupport.x);L.supportPosition56[1]=float(nativeSupport.y);L.supportPosition56[2]=float(nativeSupport.z);}
   if(!support&&n>=3&&!L.guideBox[b])for(int i=0;i<3;++i)L.guideSize[b][i]=2*(b?L.gateRadius:L.grabRadius);
   *value=shown*(n>=6?1.f:.01f);if(support)L.supportEdited56=true;else if(n>=3)L.guideBox[b]=true;result=0;
  }ImGui::PopID();
 }
 ImGui::Separator();ImGui::TextUnformatted("Hand contact spheres (same points used for grabbing)");
 for(int side=0;side<2;++side){ImGui::PushID(5700+side);ImGui::TextUnformatted(side?"LEFT palm: support / holding":"RIGHT palm: lever / loading");
  const char* names[]={"Palm X","Palm Y","Palm Z","Palm diameter"};for(int n=0;n<4;++n){float* value=n<3?&L.palmOffset56[side][n]:&L.palmDiameter56[side];float shown=*value*100.f;ImGui::SetNextItemWidth(230*g_menuScale);if(ImGui::SliderFloat(names[n],&shown,n<3?-50.f:1.f,n<3?50.f:30.f,"%.2f cm")){*value=shown*.01f;result=0;}}ImGui::PopID();}
 if(ImGui::CollapsingHeader("Whole weapon placement (separate from guides)")){const char* names[]={"Move X","Move Y","Move Z","Rotate X","Rotate Y","Rotate Z"};ImGui::TextUnformatted("Left holding hand");for(int n=0;n<6;++n)slider(n,names[n]);ImGui::TextUnformatted("Right holding hand");for(int n=6;n<12;++n)slider(n,names[n-6]);}
 if(ImGui::CollapsingHeader("Reload stroke and legacy pickup zones")){slider(16,"Lever pull distance");const char* pick[]={"Right waist grab","Chest grab","Free RIGHT grip"};if(ImGui::Combo("Shell pickup",&L.pickup,pick,3))result=0;ImGui::TextDisabled("Global grip-only ammo ignores pickup zones; weapon part contacts have priority.");}
 ImGui::Separator();
 if(ImGui::Button("Save All - 1887"))result=SaveLever1887Settings(savePath)?1:2;
 ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");
}
bool Lever1887Note()
{
 if(!Lever1887Active())return false;
 const auto& L=g_lever;
 if(L.simple){fc3xr::SetTestNote(L.clip>=0?L"1887":L.leverOpen?L"1887 - LEVER DOWN\nRIGHT grip: one shell. X: close the lever.":!g_twoHand?L"1887 (Simple)\nHold it with both hands (LEFT grip)\nto work the lever or reload.":!L.chambered?L"1887 - CYCLE\nRIGHT grip or shake the gun.":L"1887 (Simple)\nRIGHT grip / shake: lever. X: reload.");return true;}
 fc3xr::SetTestNote(L.pending?L"1887 - LOADING":
  L.shell?(L.leverOpen?L"1887 - SHELL IN RIGHT HAND\nPush it into the open gate on top.":L"1887 - SHELL IN RIGHT HAND\nOpen the lever first (grip the loop, pull down).\nLet go of RIGHT grip to drop the shell."):
  L.grabbing?L"1887 - LEVER\nPull down to open, push back up to close.":
  L.hold?(L.leverOpen?L"1887 - LEVER OPEN\nRIGHT grip away from weapon parts: shell.\nPush it into the gate. Grip the lever to close.\nRelease LEFT grip to return the gun.":
               L"1887 - IN LEFT HAND\nGrip the lever loop, pull down to open.\nRelease LEFT grip to take it back."):
  L.leverOpen?L"1887 - LEVER OPEN\nLEFT grip at the forend holds the gun.\nWork the lever; release LEFT grip to return.":
  !L.chambered?L"1887 - CHAMBER EMPTY\nLEFT grip at the forend holds the gun.\nWork the lever, then release LEFT grip.":
  L"1887\nAfter firing, LEFT grip at the forend\nholds the gun for the physical lever.");
 return true;
}

