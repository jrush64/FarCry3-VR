#include "weapon_clip_clock223.inl"
// SPAS-12 (spas12.xbg, profile 34). Design: no pump after each shot, only after reloading.
// Manual mode differs from the M133: grab 1 shell and put it into the loading port under the gun, with the animation
// of the shell going in; after manually loading an EMPTY gun, the forend must be pulled back by hand before it can
// fire; a partial reload needs no pump. Settings also in the INSERT menu.
//  - Capture weapons3-spas-16848 (work/spas_clips.py -> spas_clips.inl): no bone palette; every part is a rigid draw with
//    its own world matrix (FRAME = main body draw 4908/7401). Parts: forend (PUMP) 12699/1494, slide (SLIDE) 12309/390,
//    loading gate (TONGUE) 14661/78 + 14739/270, loading shell 14193/108 + 14301/360. Native reload = one engine transfer
//    per shell (~0.81 s cycle); the native pump happens only in EMPTY reloads (forend 9 cm along FRAME -Y).
//  - Parts went missing under motion controls: its unskinned VS 3152 / 4C0D draws keep the world rows at
//    cb0 [4:16]; the hand move wrote [0:12] (stereo.cpp WeaponWorldOffset). Fixed there.
//  - Manual: LEFT grip + X (or the Waist / Chest zone) puts ONE shell in the left hand. Hand sphere into the PORT box under
//    the receiver plays Jason's own shell-insert animation on the gun (left hand, fingers, shell, loading gate from the
//    capture); at the native transfer moment the engine's own Begin/transfer/End add one shell (capacity checked, request
//    re-sent until the seam answers; refused = the shell stays in the hand). Loading an EMPTY gun locks the trigger until
//    the forend is pulled back and pushed forward (LEFT grip in the FOREND box). A partial reload fires straight away.
//    The game's X / automatic reload is refused.
//  - Simple: X runs the game's own reload (Jason's animation, his own pump on empty); the left arm follows it on the gun.
//  - Guides: PORT and FOREND boxes + the hand sphere, adjustable in INSERT > Spas12 like the 1911 / revolver.
//  - Revision 2 (grabbing the forend worked, but Jason's hand must actually grab it, as on the M133):
//    while the forend is held (and while it springs back) Jason's left hand is ON it: wrist = FRAME x his
//    captured forend hold (kSpasLeftSupportFromFrame) slid by the pump stroke, fingers = his forend grip, like the
//    1887 / rifle support holds. The stroke is read from the REAL controller wrist (the drawn hand is now pinned to the
//    forend, so it can no longer measure the pull).
//  - Revision 3 (the gun would not fire while held two-handed): holding the forend to aim, the hand drifts a few
//    mm along the rail, and the trigger locked as soon as the forend was 4.5 mm back. The stroke now has a 2 cm dead zone
//    (the forend stays fully forward until it is really pulled; the full stroke still takes ~9 cm of hand travel), and
//    every refused shot logs its reason.
//  - Revision 4 (the forend can be pulled back only after loading; an empty gun fires only after shells are in and the
//    forend has been pulled back; the free left hand moved erratically,
//    screenshot: flat splayed hand): the forend only slides while a pump is due (after loading an EMPTY gun); any other
//    time LEFT grip on the FOREND box is a plain two-handed hold - Jason's hand on the forend, it never moves, and the gun
//    fires. The free left hand no longer uses the once-captured neutral finger pose (the SPAS's native left hand is in its
//    reload / support animation when that capture happens, hence the splayed hand): in Manual it always shows Jason's
//    forend-hold fingers unless it is carrying a shell or inside his insert animation.
#include "spas_clips.inl"
const float kSpasShellIdleFromFrame[12]={1.f,0,0,.0000075f,0,1.f,0,-.1732254f,0,0,1.f,.0038377f}; // loading shell at rest in the tube (idle frames 10300-10500, work/spas_shell_idle.py)
const double kSpasForendDeadZone=.02; // m of hand travel along the rail before the forend moves
const int kSpasInsertFirstKey=9,kSpasInsertLastKey=20; // gate starts opening .. the shell is pushed into the tube
struct Spas
{
 bool simple=false,showGuides=true,ammoPreview=false;int pickup=2; // pickup: 0 waist, 1 chest, 2 LEFT grip + X
 // Two guide boxes (FRAME axes): 0 = PORT (loading port under the receiver), 1 = FOREND (pump).
 float boxMove[2][3]={},boxSize[2][3]={{.08f,.08f,.08f},{.07f,.12f,.07f}},boxRot[2][3]={};
 // Left hand sphere: offset from the palm in wrist axes + diameter (the 1911 values); shell in hand position / rotation.
 float hand[3]={-.0658f,.0481f,-.1174f},handDiameter=.10f,ammoPos[3]={},ammoRot[3]={};
 // state
 bool carrying=false,pending=false,refused=false,filled=false,needPump=false,grab=false,reachedBack=false,leftGripWas=false,xWas=false,xOwned=false;
 double pump=0,grabPump=0;V3 grabStart={},grabReal={}; // forend 0 forward .. 1 fully back
 Mat4 frameBody={};LONG frameFrame=-100000;Mat4 leftWrist={};LONG leftFrame=-100000;
 UINT ammoState=0,ammoWrapper=0;ULONGLONG deadline=0,pulseAt=0,lastTick=0;LONG loads=0,pumps=0,gripLogs=0;
 bool native=false;ULONGLONG nativeAt=0,nativeEndAt=0;
 int clip=-1;ULONGLONG clipAt=0,clipPauseAt223=0;bool seatFired=false;
 struct Pose{Mat4 hand,shell,gate;bool shellOn;float fingers[204];} pose={};LONG poseFrame=-100000;
} g_spas;
bool SpasModel(){return g_weaponMechanicProfile==34&&g_frame-g_weaponMechanicFrame<=2;}
bool SpasActive(){return SpasModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool SpasManual(){return SpasActive()&&!g_spas.simple;}
// INSERT values, n: 0..17 boxes (box*9 + move xyz, size xyz, rotate xyz), 18..20 hand offset, 21 hand diameter,
// 22..24 shell position, 25..27 shell rotation.
const wchar_t* kSpasKeys[28]={L"PortX",L"PortY",L"PortZ",L"PortWidth",L"PortLength",L"PortHeight",L"PortRotX",L"PortRotY",L"PortRotZ",
 L"ForendX",L"ForendY",L"ForendZ",L"ForendWidth",L"ForendLength",L"ForendHeight",L"ForendRotX",L"ForendRotY",L"ForendRotZ",
 L"HandX",L"HandY",L"HandZ",L"HandDiameter",L"AmmoPosX",L"AmmoPosY",L"AmmoPosZ",L"AmmoRotX",L"AmmoRotY",L"AmmoRotZ"};
float* SpasValue(Spas& S,int n)
{
 if(n<18){const int b=n/9,k=n%9;return k<3?&S.boxMove[b][k]:k<6?&S.boxSize[b][k-3]:&S.boxRot[b][k-6];}
 return n<21?&S.hand[n-18]:n==21?&S.handDiameter:n<25?&S.ammoPos[n-22]:&S.ammoRot[n-25];
}
float* SpasValue(int n){return SpasValue(g_spas,n);}
float SpasDefault(int n){static Spas d{};return *SpasValue(d,n);}
int SpasKind(int n){if(n<18){const int k=n%9;return k<3?0:k<6?1:2;}return n<21?0:n==21?3:n<25?0:2;} // 0 position, 1 size, 2 angle, 3 diameter
void LoadSpasSettings(const wchar_t* explicitPath)
{
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);const wchar_t* path=file;
 auto& S=g_spas;const wchar_t* s=L"Spas12";
 S.simple=GetPrivateProfileIntW(s,L"ReloadMode",0,path)==1;S.showGuides=GetPrivateProfileIntW(s,L"ShowGuides",1,path)!=0;
 S.pickup=(std::max)(0,(std::min)(2,static_cast<int>(GetPrivateProfileIntW(s,L"AmmoPickup",2,path))));
 for(int n=0;n<28;++n){wchar_t t[64]={};GetPrivateProfileStringW(s,kSpasKeys[n],L"",t,64,path);wchar_t* e=nullptr;const float f=static_cast<float>(wcstod(t,&e));
  const int k=SpasKind(n);if(e!=t&&!*e&&std::isfinite(f)&&(k==0?std::fabs(f)<=.5f:k==1?f>=.01f&&f<=.4f:k==3?f>=.01f&&f<=.2f:std::fabs(f)<=180))*SpasValue(n)=f;}
}
bool SaveSpasSettings(const wchar_t* explicitPath)
{
 // In the game INSERT passes no path: fall back to fc3_stereo.ini like every other weapon.
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);const wchar_t* path=file;
 const auto& S=g_spas;const wchar_t* s=L"Spas12";bool ok=true;
 ok=WritePrivateProfileStringW(s,L"ReloadMode",S.simple?L"1":L"0",path)!=FALSE&&ok;ok=WritePrivateProfileStringW(s,L"ShowGuides",S.showGuides?L"1":L"0",path)!=FALSE&&ok;
 {wchar_t t[16];_snwprintf_s(t,16,_TRUNCATE,L"%d",S.pickup);ok=WritePrivateProfileStringW(s,L"AmmoPickup",t,path)!=FALSE&&ok;}
 for(int n=0;n<28;++n){wchar_t t[64];_snwprintf_s(t,64,_TRUNCATE,L"%.6f",*SpasValue(n));ok=WritePrivateProfileStringW(s,kSpasKeys[n],t,path)!=FALSE&&ok;}
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);return ok;
}
Mat4 SpasKeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
// ---- Jason's shell-insert clip (keys 9..20 of the captured cycle: gate opens, shell seated, pushed into the tube)
int SpasSeatKey(){int seat=0;for(int i=0;i<kSpasClipShellInsertCount;++i)if(kSpasClipShellInsert[i].t<=kSpasShellInsertSeat)seat=i;return seat;}
double SpasClipStart(){return kSpasClipShellInsert[kSpasInsertFirstKey].t;}
double SpasClipEnd(){return kSpasClipShellInsert[kSpasInsertLastKey].t;}
void SpasSampleClip(double seconds,Spas::Pose& out)
{
 const SpasKey* k=kSpasClipShellInsert;const int count=kSpasClipShellInsertCount;
 int i=0;while(i+1<count&&k[i+1].t<=seconds)++i;
 const SpasKey& a=k[i];const SpasKey& b=k[(std::min)(i+1,count-1)];const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(seconds-a.t)/span)):0;
 out.hand=PistolRigidBlend(SpasKeyMat(a.leftHand),SpasKeyMat(b.leftHand),w);out.shell=PistolRigidBlend(SpasKeyMat(a.shell),SpasKeyMat(b.shell),w);
 out.gate=PistolRigidBlend(SpasKeyMat(a.gate),SpasKeyMat(b.gate),w);out.shellOn=(w<.5?a.shellOn:b.shellOn)>.5f;
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.leftFingers,bone),BoneMatrix(b.leftFingers,bone),w);for(int j=0;j<12;++j)out.fingers[bone*12+j]=static_cast<float>(m.m[j]);}
}
bool SpasClipPlaying(){return g_spas.clip>=0&&SpasModel()&&!g_spas.simple;}
double SpasClipSeconds(){return SpasClipStart()+WeaponClipElapsed223(g_spas.clipAt,g_spas.clipPauseAt223);}
const Spas::Pose& SpasClipPose()
{
 auto& S=g_spas;if(S.poseFrame!=g_frame){SpasSampleClip((std::min)(SpasClipSeconds(),SpasClipEnd()),S.pose);S.poseFrame=g_frame;}
 return S.pose;
}
void SpasStartClip()
{
 auto& S=g_spas;S.clip=0;S.clipAt=GetTickCount64();S.clipPauseAt223=0;S.seatFired=false;S.poseFrame=-100000;
 Log("[SPAS] Jason's shell-insert animation (keys %d..%d, transfer at %.3f s)",kSpasInsertFirstKey,kSpasInsertLastKey,kSpasShellInsertSeat);
}
// ---- geometry (FRAME = main body draw)
bool SpasFrameReady(){return g_frame-g_spas.frameFrame<=2;}
bool SpasLeftReady(){return g_frame-g_spas.leftFrame<=2;}
V3 SpasPalmOf(const Mat4& wrist,const float* hand){return Apply(wrist,{-.591+hand[0],.011+hand[1],1.149+hand[2]});}
// Box b (FRAME): 0 = PORT, centred 3 cm under the native shell seat; 1 = FOREND, at Jason's support palm, moving with the pump.
Mat4 SpasBoxFrame(int b)
{
 const auto& S=g_spas;V3 c={};
 if(b==0){const float* s=kSpasClipShellInsert[SpasSeatKey()].shell;c={s[3],s[7],s[11]-.03};}
 else{const float zero[3]={};c=SpasPalmOf(SpasKeyMat(kSpasLeftSupportFromFrame),zero);c=Add(c,Scale({kSpasForendAxis[0],kSpasForendAxis[1],kSpasForendAxis[2]},S.pump*kSpasForendTravel));}
 c=Add(c,{S.boxMove[b][0],S.boxMove[b][1],S.boxMove[b][2]});
 Mat4 basis=HandIdentity();
 for(int axis=0;axis<3;++axis){Mat4 rot=HandIdentity();const int p=(axis+1)%3,q=(axis+2)%3;const double th=S.boxRot[b][axis]*.0174532925199433;rot.m[p*4+p]=rot.m[q*4+q]=std::cos(th);rot.m[p*4+q]=std::sin(th);rot.m[q*4+p]=-std::sin(th);basis=Multiply(basis,rot);}
 basis.m[3]=c.x;basis.m[7]=c.y;basis.m[11]=c.z;if(b==1&&LeftShotgun318()){const auto mirror=SidearmMirror315();basis=Multiply(mirror,Multiply(basis,mirror));}return basis;
}
V3 SpasBoxCentre(int b){const Mat4 m=SpasBoxFrame(b);return {m.m[3],m.m[7],m.m[11]};}
double SpasBoxDistance(int b,const V3& pointFrame)
{
 const auto& S=g_spas;Mat4 inverse={};if(!Invert(SpasBoxFrame(b),inverse))return 1e9;
 const V3 l=Apply(inverse,pointFrame);const double v[3]={l.x,l.y,l.z};double d2=0;
 for(int a=0;a<3;++a){const double outside=(std::max)(0.0,std::fabs(v[a])-S.boxSize[b][a]*.5);d2+=outside*outside;}
 return std::sqrt(d2);
}
bool SpasPalmBody(V3& palm){const auto& S=g_spas;if(!SpasLeftReady())return false;palm=SpasPalmOf(S.leftWrist,S.hand);return true;}
bool SpasPalmFrame(V3& palm)
{
 const auto& S=g_spas;Mat4 inverse={};V3 body={};
 if(!SpasFrameReady()||!SpasPalmBody(body)||!Invert(S.frameBody,inverse))return false;
 palm=Apply(inverse,body);return true;
}
// Real left controller wrist in FRAME (the drawn hand sits on the forend while it is held).
bool SpasRealLeftFrame(V3& p)
{
 Mat4 inverse={};if(!SpasFrameReady()||!g_handLeftValid||!Invert(g_spas.frameBody,inverse))return false;
 p=Apply(inverse,ShotgunMotion318(1,ControllerWrist(g_handBody[1],1)));return true;
}
bool SpasForendHeld(){return SpasManual()&&(g_spas.grab||g_spas.pump>0)&&!SpasClipPlaying();}
// The specialized SPAS forend grab already owns the left hand. During an
// ordinary support hold it must also enable the shared two-controller gun
// turn; previously UpdateTwoHand unconditionally disabled that turn here.
// Pump measurement stays one-handed while a pump is due, so rotating the gun
// toward the pulling controller cannot consume the physical stroke itself.
bool SpasTwoHandAim56()
{
 const auto& S=g_spas;const auto c=fc3pad::Latest();
 return SpasManual()&&S.grab&&!S.needPump&&S.pump<=.001&&!S.carrying&&!S.pending&&!SpasClipPlaying()&&
  SpasFrameReady()&&c.active&&c.leftGrip>=.35f&&!fc3pad::GameplayInputBlocked()&&!fc3pad::WeaponSelectionRequested()&&
  MotionInteractionEnabled()&&g_eyeViewsSet&&g_handRightValid&&g_handLeftValid&&g_armJoints[0].valid&&g_armJoints[1].valid;
}
void SpasTwoHandAimDirection56(const ArmJoints& right,const ArmJoints& left)
{
 if(!g_twoHand||!SpasTwoHandAim56())return;
 Mat4 base=ControllerHandMove(0,{right.w[0],right.w[1],right.w[2]}),move={},inverse={};
 if(!GunMove(right,left,move)||!Invert(base,inverse))return;
 const Mat4 turn=Multiply(move,inverse);const V3 forward={g_handRight[2],g_handRight[6],g_handRight[10]};
 // Same barrel-direction conversion as the ordinary rifle support branch.
 const V3 turned={turn.m[0]*forward.x+turn.m[1]*forward.y+turn.m[2]*forward.z,
                  turn.m[4]*forward.x+turn.m[5]*forward.y+turn.m[6]*forward.z,
                  turn.m[8]*forward.x+turn.m[9]*forward.y+turn.m[10]*forward.z};
 g_barrelCam[0]=turned.x;g_barrelCam[1]=turned.y;g_barrelCam[2]=turned.z;
}
// Manual: the free left hand always has Jason's own fingers (never the captured neutral pose).
bool SpasOwnsLeftFingers(){return SpasManual();}
// Jason's forend hold, slid along the rail by the stroke (FRAME).
Mat4 SpasForendHand()
{
 const double d=g_spas.pump*kSpasForendTravel;Mat4 slide=HandIdentity();
 slide.m[3]=kSpasForendAxis[0]*d;slide.m[7]=kSpasForendAxis[1]*d;slide.m[11]=kSpasForendAxis[2]*d;
 return Multiply(slide,SpasKeyMat(kSpasLeftSupportFromFrame));
}
// Shell relative to the left wrist: Jason's own carry (clip key 0, shell in hand at the pouch) x INSERT position / rotation.
Mat4 SpasShellFromWrist()
{
 const auto& S=g_spas;const SpasKey& k=kSpasClipShellInsert[0];Mat4 wristInverse={};
 Mat4 base=SpasKeyMat(k.shell);if(Invert(SpasKeyMat(k.leftHand),wristInverse))base=Multiply(wristInverse,base);
 Mat4 correction=HandIdentity();
 for(int axis=0;axis<3;++axis){const double a=S.ammoRot[axis]*3.141592653589793/180;Mat4 r=HandIdentity();const int u=(axis+1)%3,v=(axis+2)%3;r.m[u*4+u]=r.m[v*4+v]=std::cos(a);r.m[u*4+v]=-std::sin(a);r.m[v*4+u]=std::sin(a);correction=Multiply(correction,r);}
 for(int i=0;i<3;++i)correction.m[i*4+3]=S.ammoPos[i];
 return Multiply(base,correction);
}
bool SpasAmmoPreview(){return g_spas.ammoPreview&&g_pistolMenu.open&&SpasModel()&&!g_spas.simple;}
// Rounds known from the HUD widget: 1 full (or no reserve), 0 room, -1 unknown (no request seen yet).
int SpasFull()
{
 const auto& S=g_spas;UINT loaded=0,capacity=0,reserve=0;
 if(!S.ammoWrapper||!ReloadSignalLocal(S.ammoState,S.ammoWrapper)||!Lever1887Ammo(S.ammoWrapper,loaded,capacity,reserve))return -1;
 const UINT weapon=ReloadRead(S.ammoWrapper,0x48);const UINT n=(std::max)(loaded,weapon?ReloadRead(weapon,0xcc):0u);
 return n>=capacity||!reserve?1:0;
}
// ---- final world of one SPAS part draw (after the hand move; rows = this draw's world rows). Every part is placed from its
// OWN draw: FRAME's moved world = this world x inverse(the part's fixed rest on FRAME), so draw order never matters.
void SpasFinalWorld(ID3D11DeviceContext* context,UINT start,UINT count,float* rows)
{
 if(!SpasModel()||!MotionInteractionEnabled())return;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=127600||d.format!=57)return;
 auto& S=g_spas;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera))return;
 const Mat4 view=HullView(camera);Mat4 viewInverse={},bodyInverse={};
 if(!Invert(view,viewInverse)||!Invert(ArmBodyToCamera(),bodyInverse))return;
 static Mat4 nativeFrame124={};static LONG nativeFrameAt124=-100000;static ID3D11DeviceContext* nativeContext124=nullptr;
 const Mat4 world=HullWorld(rows);
 Mat4 fixedWorld={},support124=SpasKeyMat(kSpasLeftSupportFromFrame);const bool fixed124=ControllerGripWorld124(context,SpasKeyMat(kSpasRightHandFromFrame),&support124,fixedWorld);
 const bool fixedBody=(start==4908&&count==7401)||(start==18&&count==3090)||(start==3108&&count==1800)||(start==0&&count==18);
 if(fixedBody){nativeFrame124=world;nativeFrameAt124=g_frame;nativeContext124=context;if(fixed124)WritePistolWorld(rows,fixedWorld);S.frameBody=Multiply(bodyInverse,Multiply(view,fixed124?fixedWorld:world));S.frameFrame=g_frame;return;}
 if(S.simple){
  Mat4 inverse124={};if(fixed124&&nativeFrameAt124==g_frame&&nativeContext124==context&&Invert(nativeFrame124,inverse124))WritePistolWorld(rows,Multiply(fixedWorld,Multiply(inverse124,world)));
  return; // retain native child mechanics on the controller-owned FRAME
 }
 const bool shell=(start==14193&&count==108)||(start==14301&&count==360),gate=(start==14661&&count==78)||(start==14739&&count==270);
 const bool forend=start==12699&&count==1494,slide=start==12309&&count==390;
 if(!shell&&!gate&&!forend&&!slide)return;
 Mat4 restInverse={};
 if(!Invert(SpasKeyMat(shell?kSpasShellIdleFromFrame:gate?kSpasGateRestFromFrame:forend?kSpasForendRestFromFrame:kSpasSlideRestFromFrame),restInverse))return;
 const Mat4 frameWorld=fixed124?fixedWorld:Multiply(world,restInverse);
 if(fixed124){Mat4 rest124={};if(Invert(restInverse,rest124))WritePistolWorld(rows,Multiply(frameWorld,rest124));}
 const bool clip=SpasClipPlaying();
 if(shell&&(S.carrying||SpasAmmoPreview())&&!clip&&SpasLeftReady())
 {WritePistolWorld(rows,Multiply(viewInverse,Multiply(ArmBodyToCamera(),ShotgunHeld318(S.leftWrist,SpasShellFromWrist(),1))));return;}
 if(clip&&(shell||gate)){const auto& p=SpasClipPose();WritePistolWorld(rows,Multiply(frameWorld,shell?p.shell:p.gate));return;}
 if((forend||slide)&&S.pump>1e-4)
 {
  const Mat4 rel=forend?PistolRigidBlend(SpasKeyMat(kSpasForendRestFromFrame),SpasKeyMat(kSpasForendBackFromFrame),S.pump):PistolRigidBlend(SpasKeyMat(kSpasSlideRestFromFrame),SpasKeyMat(kSpasSlideBackFromFrame),S.pump);
  WritePistolWorld(rows,Multiply(frameWorld,rel));
 }
}
void RecordSpasLeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!SpasModel())return;
 Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_spas.leftWrist=Multiply(inverse,view);g_spas.leftFrame=g_frame;
}
// ---- Simple: native reload window, the left arm follows Jason's animation on the gun (as the revolver)
void SpasNativeBegin(UINT state,UINT wrapper)
{
 if(!SpasModel()||!g_spas.simple||!ReloadSignalLocal(state,wrapper))return;
 auto& S=g_spas;S.native=true;S.nativeAt=GetTickCount64();S.nativeEndAt=0;Log("[SPAS] native reload: left arm follows Jason's animation on the gun");
}
void SpasNativeEnd(UINT state,UINT wrapper)
{
 auto& S=g_spas;if(!S.native||!ReloadSignalLocal(state,wrapper))return;
 S.nativeEndAt=GetTickCount64();Log("[SPAS] native reload end");
}
double SpasNativeLeftWeight()
{
 auto& S=g_spas;if(!S.native)return 0;
 const ULONGLONG now=GetTickCount64();
 // an 11-shell reload runs ~11 s
 if(!SpasModel()||now-S.nativeAt>14000||(S.nativeEndAt&&now-S.nativeEndAt>150)){S.native=false;return 0;}
 const double in=(std::min)(1.0,(now-S.nativeAt)/150.0),out=S.nativeEndAt?1-(std::min)(1.0,(now-S.nativeEndAt)/150.0):1;
 return (std::min)(in,out);
}
bool SpasNativeLeft(){SpasNativeLeftWeight();return g_spas.native;}
void SpasLeftNativeMove(Mat4& handMove)
{
 const double w=SpasNativeLeftWeight();if(w<=0)return;
 Mat4 gun={};if(!RightHandMove(gun))return;
 handMove=w>=1?gun:PistolRigidBlend(handMove,gun,w);
}
// Left fingers: holding the shell (Jason's carry), the clip's own fingers, or the forend grip.
bool SpasLeftGrip(float* bones)
{
 const auto& S=g_spas;
 if(!SpasManual()&&!SpasAmmoPreview())return false;
 const Mat4 wrist=BoneMatrix(bones,2);
 const float* fingers=SpasClipPlaying()?SpasClipPose().fingers:S.carrying||SpasAmmoPreview()?kSpasClipShellInsert[0].leftFingers:kSpasLeftIdleFingers;
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));
 return true;
}
// Left wrist during Jason's clip: FRAME x clip hand, blended in from the controller over 100 ms.
bool SpasLeftClipMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!SpasClipPlaying()||!SpasFrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 const Mat4 clipMove=Multiply(Multiply(ArmBodyToCamera(),Multiply(g_spas.frameBody,SpasClipPose().hand)),inverse);
 const double w=WeaponClipElapsed223(g_spas.clipAt,g_spas.clipPauseAt223)/.1;
 move=w>=1?clipMove:PistolRigidBlend(move,clipMove,w);return true;
}
// Left wrist while the forend is held: on Jason's forend hold, riding the stroke (AkSupportHandMove pattern).
bool SpasLeftSupportMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!SpasForendHeld()||!SpasFrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(g_spas.frameBody,SpasForendHand())),inverse);return true;
}
void UpdateSpasFire(const fc3pad::Controllers& c)
{
 const auto& S=g_spas;fc3pad::RequireFullFirePull(false);
 // semi-auto: never a pump between shots; locked only after an EMPTY reload until the forend is cycled, while Jason
 // pushes a shell in, or while the forend is pulled back
 const bool block=SpasManual()&&(S.needPump||S.clip>=0); // holding or even sliding the forend never blocks a shot
 fc3pad::BlockFire(block);
 static bool was=false;static LONG logs=0;const bool pulled=c.rightTrigger>.7f;
 if(pulled&&!was&&block&&InterlockedIncrement(&logs)<=40)Log("[SPAS] shot refused: %s",S.needPump?"pump due after an empty reload":"Jason is pushing a shell in");
 was=pulled;
}
// Reload request seam (game thread): one shell per seat, capacity checked (the engine transfer ignores capacity).
void SpasRequest(UINT state,UINT wrapper)
{
 auto& S=g_spas;
 if(!ReloadSignalLocal(state,wrapper))return;
 S.ammoState=state;S.ammoWrapper=wrapper;
 if(S.clip>=0&&(!SpasActive()||S.clipPauseAt223||g_pistolMenu.open||fc3pad::GameplayInputBlocked()))return;
 if(!S.pending||GetTickCount64()>S.deadline)return;
 S.pending=false;fc3pad::CancelReloadRequest();
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 UINT loaded=0,capacity=0,reserve=0;
 if(!Lever1887Ammo(wrapper,loaded,capacity,reserve))
 {S.refused=true;fc3xr::Buzz(0);Log("[SPAS] capacity unavailable: shell retained, no transfer");return;}
 if((std::max)(loaded,before)>=capacity||!reserve)
 {S.refused=true;fc3xr::Buzz(0);Log("[SPAS] shell refused: %u/%u, reserve %u (stays in the hand)",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after>before)
 {
  S.filled=true;++S.loads;fc3xr::Buzz(1);
  if(before==0&&!S.needPump){S.needPump=true;Log("[SPAS] loaded into an EMPTY gun: pull the forend back and forward before it fires");}
 }
 Log("[SPAS] shell seated: native Begin/transfer/End, loaded %u -> %u, engine ok=%d%s",before,after,ok,S.needPump?", pump needed":"");
}
void TickSpas(bool gameplay)
{
 auto& S=g_spas;const auto c=fc3pad::AmmoControls();
 const bool manual=gameplay&&SpasManual();
 // Manual: X is the shell pickup (with LEFT grip), never the game's reload. Only touched while this gun owns it, so the
 // revolver's own X ownership is never overwritten.
 if(SpasModel()||S.xOwned){fc3pad::SetXOwned(manual);S.xOwned=manual;}
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!S.leftGripWas,lgUp=c.leftGrip<.35f,xEdge=c.x&&!S.xWas;
 S.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&S.leftGripWas)):lg;S.xWas=c.x;
 const ULONGLONG now=GetTickCount64();const double dt=S.lastTick?(std::min)(.1,(now-S.lastTick)/1000.0):0;S.lastTick=now;
 // the pump requirement survives weapon switches
 const ULONGLONG resumed223=AdvanceWeaponClip223(S.clipAt,S.clipPauseAt223,S.clip>=0,gameplay&&SpasActive(),now);
 if(resumed223){if(S.pending)S.deadline+=resumed223;if(S.pulseAt)S.pulseAt+=resumed223;S.poseFrame=-100000;}
 if(!gameplay||!SpasActive()){S.grab=false;if(!SpasModel()){if(S.pending)fc3pad::CancelReloadRequest();S.pending=S.carrying=S.reachedBack=false;S.clip=-1;S.clipPauseAt223=0;S.pump=0;}return;}
 if(S.pending&&now>S.deadline){S.pending=false;fc3pad::CancelReloadRequest();}
 if(S.simple){S.carrying=S.grab=S.reachedBack=S.needPump=false;S.clip=-1;S.pump=0;return;}
 if(S.clip>=0)
 { // Jason's clip owns the left hand, the shell and the loading gate until it ends
  const double s=SpasClipSeconds();
  if(!S.seatFired&&s>=kSpasShellInsertSeat){S.seatFired=true;S.filled=false;S.pending=true;S.deadline=now+3000;S.pulseAt=now;fc3pad::RequestReload();Log("[SPAS] Jason seats the shell: engine transfer requested");}
  if(S.pending&&now-S.pulseAt>=250){S.pulseAt=now;fc3pad::RequestReload();} // re-send until the engine seam answers
  if(s>=SpasClipEnd())
  {
   if(S.pending)return; // hold the last frame until the transfer lands (or the 3 s deadline clears pending)
   if(S.filled){S.carrying=false;Log("[SPAS] shell in");}
   else{S.carrying=true;S.refused=true;fc3xr::Buzz(0);Log("[SPAS] the engine did not take the shell: it stays in the left hand (move out of the PORT box and back in)");}
   S.clip=-1;
  }
  return;
 }
 const double scale=(std::max)(.1f,g_worldScale),handR=S.handDiameter*.5*scale;
 V3 palm={},palmBody={};const bool palmOk=SpasPalmFrame(palm)&&SpasPalmBody(palmBody);
 const double dP=palmOk?SpasBoxDistance(0,palm):1e9,dF=palmOk?SpasBoxDistance(1,palm):1e9;
 const bool support342=ShotgunSupportAccepted342(c);
 const bool inPort=dP<=handR,inForend=dF<=handR||support342;
 double dZone=1e9;const bool zone=!support342&&palmOk&&RevolverPickupZone(S.pickup,palmBody,dZone);
 // LEFT grip + X (pickup "Grip + X"): one shell in the left hand
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&palmOk&&!inForend&&!inPort))&&!S.pending&&!S.carrying&&!S.grab&&(S.pickup==2||fc3pad::AmmoPickupOverride(c)||fc3pad::GripOnlyAmmoEnabled()))
 {
  if(SpasFull()==1){fc3xr::Buzz(0);Log("[SPAS] LEFT grip + X: nothing - the tube is full");}
  else{S.carrying=true;S.refused=false;fc3xr::Buzz(1);Log("[SPAS] LEFT grip + X: one shell in the left hand");}
 }
 if(lgEdge&&!S.carrying&&!S.grab&&!S.pending)
 {
  const char* action="nothing";
  if(zone&&SpasFull()!=1){S.carrying=true;S.refused=false;fc3xr::Buzz(1);action=S.pickup?"shell taken (chest)":"shell taken (waist)";}
  else if(zone){fc3xr::Buzz(0);action="no shell: the tube is full";}
  else if(inForend&&SpasRealLeftFrame(S.grabReal)){S.grab=true;S.grabStart=palm;S.grabPump=S.pump;fc3xr::Buzz(0);action=S.needPump?"forend grabbed: Jason's hand on it (pull back, push forward)":"forend held two-handed (no pump due: it will not slide)";}
  if(InterlockedIncrement(&S.gripLogs)<=60)Log("[SPAS] LEFT grip: %s | hand sphere %s (d %.3f); PORT box %s (%.3f), FOREND box %s (%.3f); pickup %s %.3f; pump %s",
   action,palmOk?"ok":"MISSING",S.handDiameter*scale,inPort?"IN":"out",dP,inForend?"IN":"out",dF,S.pickup==2?"grip+X":S.pickup?"chest":"waist",dZone,S.needPump?"needed":"not needed");
 }
 // Forend: follows the hand along its rail (FRAME rest -> back); released, it springs forward.
 if(S.grab)
 {
  V3 real={};
  if(lgUp||!SpasRealLeftFrame(real))S.grab=false;
  else if(!S.needPump)S.pump=0; // no pump due: a plain two-handed hold, the forend never moves
  else
  {
   // 2 cm dead zone: aiming with the hand on the forend never moves it
   const V3 axis={kSpasForendAxis[0],kSpasForendAxis[1],kSpasForendAxis[2]};const double d=Dot(Sub(real,S.grabReal),axis),dz=kSpasForendDeadZone;
   const double moved=d>dz?d-dz:d<-dz?d+dz:0;
   S.pump=(std::max)(0.0,(std::min)(1.0,S.grabPump+moved/(kSpasForendTravel-dz)));
  }
 }
 if(!S.grab&&S.pump>0)S.pump=(std::max)(0.0,S.pump-dt/.12);
 if(S.pump>=.85&&!S.reachedBack){S.reachedBack=true;fc3xr::Buzz(0);Log("[SPAS] forend back");}
 if(S.reachedBack&&S.pump<=.15)
 {
  S.reachedBack=false;++S.pumps;fc3xr::Buzz(1);
  if(S.needPump){S.needPump=false;Log("[SPAS] forend forward: chambered, fire live");}else Log("[SPAS] forend cycled (already chambered)");
 }
 // Shell into the PORT box: Jason pushes it in.
 if(!inPort)S.refused=false;
 if(S.carrying&&!S.pending&&!S.refused&&inPort&&S.pump<.05){SpasStartClip();return;}
 if(S.carrying&&lgUp&&!S.pending){S.carrying=false;Log("[SPAS] shell dropped, no ammo change");}
}
// Box + hand sphere guides (Manual play, and the INSERT preview), like the revolver.
int g_spasGuideTarget52=0;
void SpasGuideMarkers()
{
 const auto& S=g_spas;
 const bool preview=g_pistolMenu.open;
 const bool play=PhysicalReloadGuidesEnabled()&&!g_pistolMenu.open;
 if((!SpasModel()&&!(preview&&g_weaponMechanicProfile==34))||S.simple||(!SpasFrameReady()&&!(preview&&S.frameFrame>=0))||!PhysicalReloadGuidesEnabled()||(!play&&!preview)){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const float scale=(std::max)(.1f,g_worldScale);const double handR=S.handDiameter*.5*scale;
 V3 p={};const bool palmOk=SpasPalmFrame(p);
 const int b=preview?g_spasGuideTarget52:g_weaponTriggerDiagnostics?(g_weaponTriggerTarget==0?0:1):(S.needPump||S.grab||S.pump>0)&&!S.carrying?1:0; // menu selection is independent of pump gameplay state
 const int contact=(b==1&&S.grab)||(palmOk&&SpasBoxDistance(b,p)<=handR)?1:0;
 const Mat4 box=Multiply(S.frameBody,SpasBoxFrame(b));
 Mat4 basis=box;for(int c=0;c<3;++c){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+c]*basis.m[r*4+c];len=std::sqrt(len);if(len<1e-8)return;for(int r=0;r<3;++r)basis.m[r*4+c]/=len;}
 V3 hand={};bool showHand=SpasPalmBody(hand);if(!showHand&&preview&&S.leftFrame>=0){hand=Apply(S.leftWrist,{-.591+S.hand[0],.011+S.hand[1],1.149+S.hand[2]});showHand=true;}
 float a[3]={static_cast<float>(box.m[3]),static_cast<float>(box.m[7]),static_cast<float>(box.m[11])},h[3]={static_cast<float>(hand.x),static_cast<float>(hand.y),static_cast<float>(hand.z)};
 fc3xr::SetReloadMarkers(a,showHand?h:nullptr,S.boxSize[b][0]*.5f*scale,contact,S.handDiameter*scale); // point marker first: the box renderer needs it
 float transform[16],size[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(basis.m[i]);for(int i=0;i<3;++i)size[i]=S.boxSize[b][i]*scale;
 fc3xr::SetWeaponBox(transform,size,contact);
 if(showHand)fc3xr::SetHandSphere();
}
// INSERT > Spas12, laid out like the revolver page.
void SpasMenu(const wchar_t* savePath)
{
 auto& S=g_spas;static int result=0;
 {int mode=S.simple?1:0;const char* modes[]={"Manual (one shell at a time, pump after an empty reload)","Simple (X: game reload)"};ImGui::SetNextItemWidth(320*g_menuScale);
  if(ImGui::Combo("Reload mode",&mode,modes,2)){S.simple=mode==1;S.carrying=S.grab=S.reachedBack=S.needPump=false;S.pump=0;S.clip=-1;result=0;}}
 if(S.simple){ImGui::TextWrapped("Simple: X runs Jason's reload. Your left hand follows it.");
  if(ImGui::Button("Save All - Spas12"))result=SaveSpasSettings(savePath)?1:2;
  ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");return;}
 ImGui::TextWrapped("Shell: LEFT grip + X (or the Ammo pickup below). Load: put it into the PORT box under the gun, Jason pushes it in. LEFT grip the FOREND box: two-handed hold. Empty gun: after loading, pull the forend back and push it forward (it only slides then). Partial reload: fires straight away.");

 auto slider=[&](int n,const char* label){
  ImGui::PushID(4600+n);float* v=SpasValue(n);const int k=SpasKind(n);
  float shown=k==2?*v:*v*100.f;
  ImGui::SetNextItemWidth((std::max)(55.f*g_menuScale,(std::min)(250.f*g_menuScale,ImGui::GetContentRegionAvail().x-110.f*g_menuScale)));
  if(ImGui::SliderFloat(label,&shown,k==2?-180.f:k==1?1.f:k==3?1.f:-30.f,k==2?180.f:k==1?30.f:k==3?20.f:30.f,k==2?"%.1f deg":"%.1f cm")){*v=k==2?shown:shown*.01f;result=0;}
  ImGui::SameLine();if(ImGui::Button("Default")){*v=SpasDefault(n);result=0;}ImGui::PopID();};
 if(ImGui::Button("Save All - Spas12"))result=SaveSpasSettings(savePath)?1:2;
 ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");
 ImGui::Separator();
 if(ImGui::Checkbox("Position shell in hand",&S.ammoPreview))result=0;
 if(S.ammoPreview){ImGui::TextWrapped("Preview: the shell sits in your left hand while INSERT is open. Move and rotate it; Save All keeps it.");
  const char* an[]={"Ammo X","Ammo Y","Ammo Z","Ammo rotate X","Ammo rotate Y","Ammo rotate Z"};for(int i=0;i<6;++i)slider(22+i,an[i]);}
 const char* names[]={"X","Y","Z","Width (X)","Length (Y)","Height (Z)","Rotate X","Rotate Y","Rotate Z"};
 const char* targets[]={"PORT: loading shell","FOREND: holding and pump"};ImGui::Combo("Adjust guide",&g_spasGuideTarget52,targets,2);
 ImGui::Separator();ImGui::TextUnformatted("PORT box: shell in (weapon local axes)");
 if(g_spasGuideTarget52==0)for(int k=0;k<9;++k)slider(k,names[k]);
 ImGui::Separator();ImGui::TextUnformatted("FOREND box: grab, pull back, push forward (weapon local axes)");
 if(g_spasGuideTarget52==1)for(int k=0;k<9;++k)slider(9+k,names[k]);
 ImGui::Separator();ImGui::TextUnformatted("Left hand (offset from palm in wrist axes)");
 slider(18,"X");slider(19,"Y");slider(20,"Z");slider(21,"Diameter");
 ImGui::Separator();ImGui::TextUnformatted("Ammo pickup");
 {const char* pick[]={"Waist","Chest / upper stomach","Grip + X"};ImGui::SetNextItemWidth(220*g_menuScale);if(ImGui::Combo("Ammo pickup",&S.pickup,pick,3))result=0;}
 ImGui::TextWrapped("Grip at the selected body zone (or LEFT grip + X anywhere) takes one shell. Release grip to drop it.");
}
bool SpasNote()
{
 if(!SpasActive())return false;
 const auto& S=g_spas;
 if(S.simple){fc3xr::SetTestNote(S.native?L"SPAS-12 - RELOADING":L"SPAS-12 (Simple)\nX: reload.");return true;}
 fc3xr::SetTestNote(S.clip>=0||S.pending?L"SPAS-12 - LOADING":
  S.carrying?L"SPAS-12 - SHELL IN HAND\nPut it into the PORT box under the gun.\nLet go of LEFT grip to drop it.":
  S.needPump?L"SPAS-12 - PUMP\nLEFT grip the FOREND box,\npull it back and push it forward.":
  L"SPAS-12\nLEFT grip + X: one shell.\nLEFT grip the FOREND box: two-handed.");
 return true;
}
