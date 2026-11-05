#include "weapon_clip_clock223.inl"
// .44 Magnum revolver (stealthhunter.xbg, profile 42). Design: both a Simple mode (X or grip) and a Manual mode, with
// guides; in Manual, X opens and closes, and a left-thumb push also opens and closes;
// simple = "Whole reload".
//  - ONE skinned mesh (capture PID3384, work/analyze_revolver_final.py): FRAME = palette slot0. Draw 12/9432 slots 2,3 =
//    crane, cylinder; draw 9444/2625 slot 3 = cylinder; draw 12069/1800 every slot = crane (the cartridges); 13869 static.
//    The crane swings about -Y through (0,*,0.0433); the native reload is ONE engine transfer that fills all six.
//  - Manual: X, or a LEFT thumb push on the cylinder's side (LEFT grip there, push left), opens it. LEFT grip at the rear of
//    the open cylinder pulls the spent rounds out (they ride in the left hand; let go = dropped). LEFT grip at the left hip
//    (or chest) takes a speedloader: bring it to the open cylinder and the engine's own Begin/transfer/End fill it (capacity
//    checked). X, or push from the outer side, closes. The game's X / automatic reload is refused; trigger locked while open.
//    First headset test (X opened the cylinder but nothing could be grabbed): the hip pickup compared the raw
//    controller with a mirrored offset. It now uses the DRAWN left palm, like the pistol magazine pickup, and every LEFT grip
//    press logs the distances it saw.
//  - Simple: X or LEFT grip runs the game's own reload; during it the left arm keeps its native animation, moved with the
//    gun, so hand, speedloader and cylinder are exactly Jason's.
//  - Revision 4 (the left-hand sphere was tiny and sat on the wrist; the loader should open only by physically gripping
//    the box and pulling it, and close the same way - no one-button open in physical reloading; the ammo pickup should
//    not be limited to chest and waist, the left hand too): copied the 1911/AK model 1:1 - left-hand sphere offset
//    from the palm + diameter in INSERT (defaults = the tuned 1911 values, 10 cm), ammo pickup Waist / Chest / Grip + X
//    (default Grip + X like every other gun), speedloader position/rotation in hand with a live preview, open/close ONLY by
//    gripping the CYLINDER box and pulling out / pushing in (X alone does nothing). Insert = hand sphere touches the
//    ROUNDS box (as the 1911 port). Rounds out = LEFT grip + X in the ROUNDS box.
//  - Revision 5 (closing the cylinder eventually stopped working): with the cylinder open the ROUNDS and
//    CYLINDER boxes are a few cm apart, so the 10 cm sphere sat in BOTH (log: 0.000 / 0.000) and the ROUNDS box won (it
//    needs X), so grip never grabbed. Now, open: LEFT grip in EITHER box grabs the cylinder; X while gripping in the
//    ROUNDS box (grab not yet moved) unloads instead. A full-cylinder refusal no longer repeats until the hand leaves.
//  - Revision 6 (native screenshots: shells drop when Jason tips the gun; Jason holds the speedloader with the
//    six rounds; Jason's left hand should hold the loaded speedloader only once the cylinder is empty):
//    cylinder open + rounds in: tip the muzzle up and they fall out (gravity), or X makes them vanish. The speedloader
//    comes only when the cylinder is open AND empty. Its body (stealthhunter_fastloader, vb 15360 / ib 2724) exists only
//    while the game draws its own reload, so it is snapshotted from that draw once per session and replayed in the left
//    hand with the rounds (the shotgun hull replay method).
//  - Revision 7 (an emptied cylinder came back full after closing and reopening; a shake of the right hand to the
//    right should swing the cylinder back; saved box values were not persisting):
//    an emptied cylinder stays empty through close / reopen / weapon switch until a
//    speedloader goes in, and the trigger is dry while it is empty (no ammo written: the engine count stays, the fill
//    later only debits what is missing). A flick of the right hand to the right (gun +X, > 1.1 m/s) closes the open
//    cylinder, which swings back at the native ~0.25 s pace. Save All wrote to a NULL path in the game (INSERT passes
//    none; the 1911 save falls back to fc3_stereo.ini) - it now does the same.
//  - Revision 8 (loading an empty cylinder was hard; use the game's own animation of Jason inserting the rounds;
//    give each shell its own physics; the right flick should play the close animation with the cylinder swinging
//    back): the skeleton (work/revolver_bones.py) gives every casing its own
//    bone (draw 12069 slots 0-5 = CASING_03,02,01,04,05,06; slot 6 = BULLET_TIPS), so dumped casings fall and tumble
//    one by one. Jason's own Insert and Close animations (capture R1 12798-12860 / 12855-12886, work/revolver_clips.py ->
//    revolver_clips.inl) play on the gun: the speedloader reaching the cylinder plays Insert from where the loader is at
//    the cylinder (left hand, fingers, loader, rounds, crane all from the clip; the fill fires at the native transfer
//    moment); a right-hand flick plays Close. A dumped cylinder whose engine count is still full just takes the rounds
//    back (no ammo change).
//  - Revision 9 (after firing 3 of 6, the opened cylinder still showed 6 rounds instead of
//    3): the cylinder shows exactly the engine's loaded count (weapon +0xCC through the request-seam wrapper; one
//    harmless request probe learns the wrapper, the Manual reload signal stays refused). Each casing has its own bone,
//    so fired chambers are simply not drawn; the shared bullet-tip bone shows while any round is in.
//  - Revision 10 (the speedloader moved independently of its 6 rounds;
//    pressing X made all ammo in the cylinder disappear): the speedloader was drawn with the
//    WORLD eye projection (copied from the ejected-shell replay); first-person weapon pieces use the WEAPON projection
//    (eye variant 1), like the rounds. Fixed. X no longer empties the cylinder: only tipping the gun does.
//  - Revision 12 (grip + X did not bring the shells out): log showed grip + X with the cylinder
//    open and rounds in did NOTHING (no unload path since revision 6, speedloader silently refused). Now, cylinder open with
//    rounds in: LEFT grip + X (anywhere) -> the rounds fall out one by one (same casing physics as tipping). With it
//    empty, LEFT grip + X -> speedloader. Every grip + X is logged with what it did.
//  - Revision 13 (with 3 of 6 loaded, the left hand should get only the 3 missing rounds;
//    the insert sometimes stopped with the cylinder empty and only worked the second time; the
//    speedloader body was not visible): (1) LEFT grip + X with the cylinder open and rounds in gives a speedloader holding only the
//    MISSING rounds (the loaded chambers stay in the cylinder); tipping still dumps. (2) Log: on the failed inserts the
//    fill request never reached the engine seam within its one 200 ms pulse; it is now re-sent every 250 ms for 3 s,
//    the clip holds at its end until the fill lands, and if it never does the speedloader stays in the hand (no more
//    empty cylinder + lost loader). (3) The speedloader body was replayed at the first draw with the rigid weapon
//    shader, which can be a world object's draw; it is now drawn only on the long barrel's own draw (start 6 / 1338),
//    with that draw's camera and that frame's world fractions (the 1887 part convention, WritePistolWorld).
//  - Guides are boxes + the hand sphere, like the AK/pistol guides. Second headset
//    test (no guides visible, nothing could be grabbed): guides were opt-in (now always on in Manual play, like the
//    AK), the box renderer needs a point marker with a radius first (ReloadMarkerBox skips without it), and the log showed
//    the hand reaching 6-8 cm from the spots against 5-6 cm radii (now 8 cm; hip zone 22 cm).
#include "revolver_data.inl"
#include "revolver_clips.inl"
struct Revolver
{
 bool simple=false,showGuides=true,ammoPreview=false;int pickup=2; // pickup: 0 waist, 1 chest, 2 LEFT grip + X (the default on every gun)
 // Two guide boxes, adjustable in INSERT like the AK/pistol boxes: 0 = rounds (rear of the cylinder), 1 = cylinder grab.
 float boxMove[2][3]={},boxSize[2][3]={{.07f,.07f,.07f},{.07f,.07f,.07f}},boxRot[2][3]={};
 // Left hand sphere: offset from the palm in wrist axes + diameter (1911 HandX/Y/Z/HandDiameter); speedloader in hand.
 float hand[3]={-.0658f,.0481f,-.1174f},handDiameter=.10f,ammoPos[3]={},ammoRot[3]={};
 // state
 bool open=false,carrying=false,spent=false,empty=false,grab=false,pending=false,refused=false,leftGripWas=false,xWas=false;double angle=0;V3 grabStart={};
 bool upSwipe323=false;V3 upStart323={};double upGunY323=0;LONG upCloses323=0;
 Mat4 frameBody={};LONG frameFrame=-100000;Mat4 leftWrist={};LONG leftFrame=-100000;
 UINT ammoState=0,ammoWrapper=0;ULONGLONG deadline=0,lastTick=0;LONG loads=0,opens=0,gripLogs=0;
 bool native=false;ULONGLONG nativeAt=0,nativeEndAt=0;
 // spent rounds falling out (tip the muzzle up)
 bool dropping=false;Mat4 dropBody={};ULONGLONG dropAt=0,tiltSince=0;
 V3 rightPrev={};ULONGLONG rightPrevAt=0;
 // Jason's clip on the gun: -1 none, 0 Insert, 1 Close
 int clip=-1;int clipFirst=0;ULONGLONG clipAt=0,clipPauseAt223=0;bool seatFired=false;
 bool probing=false;ULONGLONG probeAt=0;int dropCount=6;
 int carryCount=6;bool filled=false;ULONGLONG pulseAt=0,seatWaitAt324=0; // rounds in the speedloader (the missing ones), fill confirmed
 struct Pose{Mat4 hand,crane,cart,loader;bool loaderOn;float fingers[204];} pose={};LONG poseFrame=-100000;bool poseLeft325=false;
} g_revolver;
const double kRevolverOpenDeg=100;
const float kRevolverLoaderSeat[3]={.0001f,.0711f,.0716f}; // speedloader centre at the native seat (crane local, PID3384 f12826)
const float kRevolverCylinderMid[3]={.0003f,.085f,.0699f}; // cylinder centre on its axis (crane local)
bool RevolverModel(){return g_weaponMechanicProfile==42&&g_frame-g_weaponMechanicFrame<=2;}
bool RevolverActive(){return RevolverModel()&&MotionInteractionEnabled()&&fc3pad::Latest().active&&g_handLeftValid&&g_handRightValid;}
bool RevolverManual(){return RevolverActive()&&!g_revolver.simple;}
// Reflect the reload arrangement in the gun's lateral axis, not the weapon
// body or its textures. Conjugation keeps rigid poses right-handed (det +1).
bool RevolverLeftReload325(){
 const auto s=fc3hands303::ReadPresentation314();
 return g_weaponMechanicProfile==42&&!g_revolver.simple&&s.enabled&&s.engaged&&!s.empty&&s.entity&&s.hand==1&&s.nativeHoldingSide==0;
}
Mat4 RevolverReloadTransform325(const Mat4& native){
 if(!RevolverLeftReload325())return native;
 Mat4 m=native;for(int r=0;r<4;++r)for(int c=0;c<4;++c)if((r==0)!=(c==0))m.m[r*4+c]=-m.m[r*4+c];return m;
}
// INSERT values, n: 0..17 boxes (box*9 + move xyz, size xyz, rotate xyz), 18..20 hand offset, 21 hand diameter,
// 22..24 speedloader position, 25..27 speedloader rotation.
const wchar_t* kRevolverKeys[28]={L"RoundsX",L"RoundsY",L"RoundsZ",L"RoundsWidth",L"RoundsLength",L"RoundsHeight",L"RoundsRotX",L"RoundsRotY",L"RoundsRotZ",
 L"CylinderX",L"CylinderY",L"CylinderZ",L"CylinderWidth",L"CylinderLength",L"CylinderHeight",L"CylinderRotX",L"CylinderRotY",L"CylinderRotZ",
 L"HandX",L"HandY",L"HandZ",L"HandDiameter",L"AmmoPosX",L"AmmoPosY",L"AmmoPosZ",L"AmmoRotX",L"AmmoRotY",L"AmmoRotZ"};
float* RevolverValue(int n)
{
 auto& R=g_revolver;
 if(n<18){const int b=n/9,k=n%9;return k<3?&R.boxMove[b][k]:k<6?&R.boxSize[b][k-3]:&R.boxRot[b][k-6];}
 return n<21?&R.hand[n-18]:n==21?&R.handDiameter:n<25?&R.ammoPos[n-22]:&R.ammoRot[n-25];
}
float RevolverDefault(int n){static const Revolver d{};Revolver copy=d;auto* keep=&g_revolver;(void)keep;
 if(n<18){const int b=n/9,k=n%9;return k<3?d.boxMove[b][k]:k<6?d.boxSize[b][k-3]:d.boxRot[b][k-6];}
 return n<21?d.hand[n-18]:n==21?d.handDiameter:n<25?d.ammoPos[n-22]:d.ammoRot[n-25];}
// kinds: 0 position (m), 1 size (m), 2 angle (deg), 3 diameter (m)
int RevolverKind(int n){if(n<18){const int k=n%9;return k<3?0:k<6?1:2;}return n<21?0:n==21?3:n<25?0:2;}
void LoadRevolverSettings(const wchar_t* explicitPath)
{
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);const wchar_t* path=file;
 auto& R=g_revolver;const wchar_t* s=L"Revolver";
 R.simple=GetPrivateProfileIntW(s,L"ReloadMode",0,path)==1;R.showGuides=GetPrivateProfileIntW(s,L"ShowGuides",1,path)!=0;
 R.pickup=(std::max)(0,(std::min)(2,static_cast<int>(GetPrivateProfileIntW(s,L"AmmoPickup",2,path))));
 for(int n=0;n<28;++n){wchar_t t[64]={};GetPrivateProfileStringW(s,kRevolverKeys[n],L"",t,64,path);wchar_t* e=nullptr;const float f=static_cast<float>(wcstod(t,&e));
  const int k=RevolverKind(n);if(e!=t&&!*e&&std::isfinite(f)&&(k==0?std::fabs(f)<=.5f:k==1?f>=.01f&&f<=.4f:k==3?f>=.01f&&f<=.2f:std::fabs(f)<=180))*RevolverValue(n)=f;}
}
bool SaveRevolverSettings(const wchar_t* explicitPath)
{
 // In the game INSERT passes no path: fall back to fc3_stereo.ini like the 1911 / 1887 saves.
 wchar_t file[MAX_PATH];ManualSettingsPath(file);if(explicitPath)wcscpy_s(file,explicitPath);const wchar_t* path=file;
 const auto& R=g_revolver;const wchar_t* s=L"Revolver";bool ok=true;
 ok=WritePrivateProfileStringW(s,L"ReloadMode",R.simple?L"1":L"0",path)!=FALSE&&ok;ok=WritePrivateProfileStringW(s,L"ShowGuides",R.showGuides?L"1":L"0",path)!=FALSE&&ok;
 {wchar_t t[16];_snwprintf_s(t,16,_TRUNCATE,L"%d",R.pickup);ok=WritePrivateProfileStringW(s,L"AmmoPickup",t,path)!=FALSE&&ok;}
 for(int n=0;n<28;++n){wchar_t t[64];_snwprintf_s(t,64,_TRUNCATE,L"%.6f",*RevolverValue(n));ok=WritePrivateProfileStringW(s,kRevolverKeys[n],t,path)!=FALSE&&ok;}
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);return ok;
}
// Crane rotation in FRAME: about -Y through the captured pivot (open = cylinder swings to -X).
Mat4 RevolverCanonicalCrane325(double degrees)
{
 const double a=-degrees*3.14159265358979/180,c=std::cos(a),s=std::sin(a);Mat4 m=HandIdentity();
 m.m[0]=c;m.m[2]=s;m.m[8]=-s;m.m[10]=c;
 const double px=kRevolverCranePivot[0],pz=kRevolverCranePivot[2];
 m.m[3]=px-(c*px+s*pz);m.m[11]=pz-(-s*px+c*pz);return m;
}
Mat4 RevolverCrane(double degrees){return RevolverReloadTransform325(RevolverCanonicalCrane325(degrees));}
// ---- Jason's clips
const RevolverKey* RevolverClipKeys(int clip,int& count){count=clip?kRevolverClipCloseCount:kRevolverClipInsertCount;return clip?kRevolverClipClose:kRevolverClipInsert;}
Mat4 RevolverKeyMat(const float* v){Mat4 m=HandIdentity();for(int i=0;i<12;++i)m.m[i]=v[i];return m;}
// Insert starts where the loader is already at the cylinder (the player brought it there): first key within 3 cm of the seat pose.
int RevolverInsertFirstKey()
{
 static int first=-1;if(first>=0)return first;
 int seat=0;for(int i=0;i<kRevolverClipInsertCount;++i)if(kRevolverClipInsert[i].t<=kRevolverInsertSeat)seat=i;
 const float* s=kRevolverClipInsert[seat].loader;first=seat;
 for(int i=0;i<=seat;++i){const float* l=kRevolverClipInsert[i].loader;const double d=std::sqrt((l[3]-s[3])*(l[3]-s[3])+(l[7]-s[7])*(l[7]-s[7])+(l[11]-s[11])*(l[11]-s[11]));
  if(kRevolverClipInsert[i].loaderOn>.5f&&d<.03){first=i;break;}}
 return first;
}
void RevolverSampleClip(int clip,double seconds,Revolver::Pose& out)
{
 int count=0;const RevolverKey* k=RevolverClipKeys(clip,count);
 int i=0;while(i+1<count&&k[i+1].t<=seconds)++i;
 const RevolverKey& a=k[i];const RevolverKey& b=k[(std::min)(i+1,count-1)];const double span=b.t-a.t,w=span>1e-6?(std::max)(0.0,(std::min)(1.0,(seconds-a.t)/span)):0;
 out.hand=PistolRigidBlend(RevolverKeyMat(a.hand),RevolverKeyMat(b.hand),w);out.crane=PistolRigidBlend(RevolverKeyMat(a.crane),RevolverKeyMat(b.crane),w);
 out.cart=PistolRigidBlend(RevolverKeyMat(a.cart),RevolverKeyMat(b.cart),w);out.loader=PistolRigidBlend(RevolverKeyMat(a.loader),RevolverKeyMat(b.loader),w);
 out.loaderOn=(w<.5?a.loaderOn:b.loaderOn)>.5f;
 for(int bone=0;bone<17;++bone){const Mat4 m=PistolRigidBlend(BoneMatrix(a.fingers,bone),BoneMatrix(b.fingers,bone),w);for(int j=0;j<12;++j)out.fingers[bone*12+j]=static_cast<float>(m.m[j]);}
}
double RevolverClipSeconds(){const auto& R=g_revolver;int count=0;const RevolverKey* k=RevolverClipKeys(R.clip,count);const double seconds=k[R.clipFirst].t+WeaponClipElapsed223(R.clipAt,R.clipPauseAt223);return R.clip==0&&R.seatFired&&R.pending&&!R.filled?(std::min)(seconds,double(kRevolverInsertSeat)):seconds;}
double RevolverClipLength(int clip){int count=0;const RevolverKey* k=RevolverClipKeys(clip,count);return k[count-1].t;}
bool RevolverClipPlaying(){return g_revolver.clip>=0&&RevolverModel()&&!g_revolver.simple;}
const Revolver::Pose& RevolverClipPose()
{
 auto& R=g_revolver;const bool left=RevolverLeftReload325();if(R.poseFrame!=g_frame||R.poseLeft325!=left){RevolverSampleClip(R.clip,(std::min)(RevolverClipSeconds(),RevolverClipLength(R.clip)),R.pose);if(left){R.pose.hand=RevolverReloadTransform325(R.pose.hand);R.pose.crane=RevolverReloadTransform325(R.pose.crane);R.pose.cart=RevolverReloadTransform325(R.pose.cart);R.pose.loader=RevolverReloadTransform325(R.pose.loader);}R.poseFrame=g_frame;R.poseLeft325=left;}
 return R.pose;
}
double RevolverCraneDegrees(const Mat4& crane){const auto native=RevolverReloadTransform325(crane);return -std::atan2(native.m[2],native.m[0])*180/3.14159265358979;}
void RevolverStartClip(int clip)
{
 auto& R=g_revolver;R.clip=clip;R.clipFirst=clip?0:RevolverInsertFirstKey();R.clipAt=GetTickCount64();R.clipPauseAt223=0;R.seatWaitAt324=0;R.seatFired=false;R.poseFrame=-100000;R.grab=false;
 Log("[REVOLVER] Jason's %s animation (from key %d)",clip?"close":"speedloader insert",R.clipFirst);
}
// Casings falling out one by one: casing b's bind centroid (FRAME, work/revolver_bones.py), per-casing velocity and spin.
const float kRevolverCasingCentroid[6][3]={{.0119f,.0651f,.0635f},{.0119f,.0650f,.0768f},{-.0107f,.0648f,.0630f},{.0006f,.0651f,.0568f},{.0002f,.0652f,.0831f},{-.0112f,.0648f,.0764f}};
Mat4 RevolverCasingBody(int b,double s)
{
 const auto& R=g_revolver;const double scale=(std::max)(.1f,g_worldScale);
 const float* c=kRevolverCasingCentroid[b];const V3 q0=Apply(R.dropBody,{c[0],c[1],c[2]});
 // out of the chamber along the drum axis (FRAME -Y, toward the rear), a little sideways, then gravity
 V3 axis={-R.dropBody.m[1],-R.dropBody.m[5],-R.dropBody.m[9]};const double al=Length(axis);if(al>1e-6)axis=Scale(axis,1/al);
 const double side[6]={.25,-.18,.12,-.3,.2,-.08},fwd[6]={.35,.5,.42,.3,.55,.45},spinRate[6]={9,-13,11,-7,15,-10};
 V3 right={R.dropBody.m[0],R.dropBody.m[4],R.dropBody.m[8]};const double rl=Length(right);if(rl>1e-6)right=Scale(right,1/rl);
 V3 v=Add(Scale(axis,fwd[b]*scale),Scale(right,side[b]*scale));
 V3 q=Add(q0,Scale(v,s));q.y-=.5*9.81*s*s*scale;
 const double a=spinRate[b]*s;V3 spinAxis=Add(right,{0,.3*(b%2?1:-1),0});const double sl=Length(spinAxis);spinAxis=Scale(spinAxis,1/sl);
 const double cs=std::cos(a),sn=std::sin(a),k=1-cs;Mat4 spin=HandIdentity();const double u[3]={spinAxis.x,spinAxis.y,spinAxis.z};
 for(int r=0;r<3;++r)for(int col=0;col<3;++col)spin.m[r*4+col]=(r==col?cs:0)+k*u[r]*u[col];
 spin.m[1]-=sn*u[2];spin.m[2]+=sn*u[1];spin.m[4]+=sn*u[2];spin.m[6]-=sn*u[0];spin.m[8]-=sn*u[1];spin.m[9]+=sn*u[0];
 Mat4 toOrigin=HandIdentity();toOrigin.m[3]=-q0.x;toOrigin.m[7]=-q0.y;toOrigin.m[11]=-q0.z;
 Mat4 toNow=HandIdentity();toNow.m[3]=q.x;toNow.m[7]=q.y;toNow.m[11]=q.z;
 return Multiply(toNow,Multiply(spin,Multiply(toOrigin,R.dropBody)));
}
// Rounds actually in the gun (engine count), -1 = not known yet.
int RevolverLoadedRounds()
{
 const auto& R=g_revolver;if(!R.ammoWrapper||!ReloadSignalLocal(R.ammoState,R.ammoWrapper))return -1;
 const UINT weapon=ReloadRead(R.ammoWrapper,0x48);if(!weapon)return -1;
 const UINT n=ReloadRead(weapon,0xcc);return n>6?6:static_cast<int>(n);
}
// Casing slots around the cylinder in ring order (top, +X upper, +X lower, bottom, -X lower, -X upper; work/revolver_bones.py).
const int kRevolverRing[6]={4,1,0,3,2,5};
int RevolverVisibleRounds(){const auto& R=g_revolver;if(R.empty)return 0;const int n=RevolverLoadedRounds();return n<0?6:n;}
// Hide the casings beyond `visible` (and the shared bullet tips when none is left).
void RevolverHideCasings(float* bones,int visible)
{
 for(int i=visible;i<6;++i)bones[kRevolverRing[i]*12+7]=-100;
 if(visible<=0)bones[6*12+7]=-100;
}
// Box b in FRAME space (centre + rotated basis), at the current crane angle (or fully open for the carried-rounds target).
// 0 = rounds: the speedloader seat at the rear of the cylinder. 1 = push: the cylinder's side (right side closed, outer
// side open). Both follow the cylinder as it swings.
Mat4 RevolverBoxFrame(int b,bool fullyOpen=false)
{
 const auto& R=g_revolver;const Mat4 crane=RevolverCanonicalCrane325(fullyOpen?kRevolverOpenDeg:R.angle);
 // rounds box: 3 cm behind the seat (the hand comes from the rear); push box: 4 cm beside the cylinder.
 V3 local=b?V3{kRevolverCylinderMid[0],kRevolverCylinderMid[1],kRevolverCylinderMid[2]}:V3{kRevolverLoaderSeat[0],kRevolverLoaderSeat[1]-.03f,kRevolverLoaderSeat[2]};
 local=Add(local,{R.boxMove[b][0],R.boxMove[b][1],R.boxMove[b][2]});
 Mat4 basis=crane;V3 c=Apply(crane,local);
 if(b)c.x+=R.open?-.04:.04; // the side shift is in FRAME X (after the swing): right side closed, outer side open
 for(int axis=0;axis<3;++axis){Mat4 rot=HandIdentity();const int p=(axis+1)%3,q=(axis+2)%3;const double th=R.boxRot[b][axis]*.0174532925199433;rot.m[p*4+p]=rot.m[q*4+q]=std::cos(th);rot.m[p*4+q]=std::sin(th);rot.m[q*4+p]=-std::sin(th);basis=Multiply(basis,rot);}
 basis.m[3]=c.x;basis.m[7]=c.y;basis.m[11]=c.z;return RevolverReloadTransform325(basis);
}
V3 RevolverBoxCentre(int b,bool fullyOpen=false){const Mat4 m=RevolverBoxFrame(b,fullyOpen);return {m.m[3],m.m[7],m.m[11]};}
// Distance from a point to the box surface (0 inside), including edges and corners (PistolGuideContact).
double RevolverBoxDistance(int b,const V3& pointFrame,bool fullyOpen=false)
{
 const auto& R=g_revolver;Mat4 inverse={};if(!Invert(RevolverBoxFrame(b,fullyOpen),inverse))return 1e9;
 const V3 l=Apply(inverse,pointFrame);const double v[3]={l.x,l.y,l.z};double d2=0;
 for(int a=0;a<3;++a){const double outside=(std::max)(0.0,std::fabs(v[a])-R.boxSize[b][a]*.5);d2+=outside*outside;}
 return std::sqrt(d2);
}
// Sphere (hand) versus the box.
bool RevolverBoxContact(int b,const V3& pointFrame,double sphereRadius,bool fullyOpen=false){return RevolverBoxDistance(b,pointFrame,fullyOpen)<=sphereRadius;}
bool RevolverFrameReady(){return g_frame-g_revolver.frameFrame<=2;}
bool RevolverLeftReady(){return g_frame-g_revolver.leftFrame<=2;}
// Drawn left palm, body space (the pistol magazine pickup uses the same point).
// Left hand sphere centre (body space): the 1911 formula, palm + INSERT offset in wrist axes.
bool RevolverPalmBody(V3& palm){const auto& R=g_revolver;if(!RevolverLeftReady())return false;palm=Apply(R.leftWrist,{-.591+R.hand[0],.011+R.hand[1],1.149+R.hand[2]});return true;}
// Speedloader rounds relative to the wrist: native carry pose x INSERT position / rotation (MagazineHeldFromHand).
Mat4 RevolverRoundsFromWrist()
{
 const auto& R=g_revolver;Mat4 correction=HandIdentity();
 for(int axis=0;axis<3;++axis){const double a=R.ammoRot[axis]*3.141592653589793/180;Mat4 r=HandIdentity();const int u=(axis+1)%3,v=(axis+2)%3;r.m[u*4+u]=r.m[v*4+v]=std::cos(a);r.m[u*4+v]=-std::sin(a);r.m[v*4+u]=std::sin(a);correction=Multiply(correction,r);}
 for(int i=0;i<3;++i)correction.m[i*4+3]=R.ammoPos[i];
 return Multiply(kRevolverCartridgesFromLeftWrist,correction);
}
bool RevolverAmmoPreview(){return g_revolver.ammoPreview&&g_pistolMenu.open&&RevolverModel()&&!g_revolver.simple;}
bool RevolverPalmFrame(V3& palm)
{
 const auto& R=g_revolver;Mat4 inverse={};V3 body={};
 if(!RevolverFrameReady()||!RevolverPalmBody(body)||!Invert(R.frameBody,inverse))return false;
 palm=Apply(inverse,body);return true;
}
// Carried rounds, relative to FRAME.
bool RevolverCarriedRel(Mat4& rel)
{
 const auto& R=g_revolver;Mat4 inverse={};
 if(!RevolverFrameReady()||!RevolverLeftReady()||!Invert(R.frameBody,inverse))return false;
 rel=Multiply(inverse,SidearmCarried315(R.leftWrist,RevolverRoundsFromWrist()));return true;
}
V3 RevolverCarriedRoundsFrame(const Mat4& rel){return Apply(rel,{kRevolverLoaderSeat[0],kRevolverLoaderSeat[1],kRevolverLoaderSeat[2]});}
// Bone override for one revolver draw (final palette, after the hand move). Pure: tests drive it directly.
void RevolverAdjustBones(float* bones,UINT start,UINT count,const Mat4& toView)
{
 auto& R=g_revolver;
 const Mat4 frame=BoneMatrix(bones,0);
 if(start==12&&count==9432){Mat4 inv={};if(Invert(ArmBodyToCamera(),inv)){R.frameBody=Multiply(inv,Multiply(toView,frame));R.frameFrame=g_frame;}}
 if(R.simple)return; // Simple: the game's own animation owns every bone
 const bool cartridges=start==12069&&count==1800;
 // Carried: straight from the left wrist (view = toView x bone), no dependence on draw order or a cached FRAME.
 Mat4 fromView={};
 if(cartridges&&(R.carrying||RevolverAmmoPreview())&&!RevolverClipPlaying()&&RevolverLeftReady()&&Invert(toView,fromView))
 {
  const Mat4 m=Multiply(fromView,Multiply(ArmBodyToCamera(),SidearmCarried315(R.leftWrist,RevolverRoundsFromWrist())));
  const int inHand=RevolverAmmoPreview()&&!R.carrying?6:R.carryCount,stay=6-inHand;Mat4 frameInverse={};
  const Mat4 cyl=Invert(frame,frameInverse)?Multiply(frame,Multiply(RevolverCrane(R.angle),Multiply(frameInverse,frame))):frame;
  for(int i=0;i<6;++i)StoreBone(bones,kRevolverRing[i],i<stay?cyl:m); // loaded chambers stay, the missing rounds ride in the hand
  StoreBone(bones,6,stay>0?cyl:m);return;
 }
 if(cartridges&&R.dropping&&Invert(toView,fromView))
 { // falling out of the tipped cylinder one by one (each casing has its own bone); the bullet tips go with them hidden
  const double s=static_cast<double>(GetTickCount64()-R.dropAt)/1000;
  for(int b=0;b<6;++b)StoreBone(bones,b,Multiply(fromView,Multiply(ArmBodyToCamera(),RevolverCasingBody(b,s))));
  for(int i=R.dropCount;i<6;++i)bones[kRevolverRing[i]*12+7]=-100; // only the rounds that were in fall out
  bones[6*12+7]=-100;return;
 }
 if(RevolverClipPlaying())
 { // Jason's clip: rounds and crane from the clip, relative to this draw's FRAME
  const auto& c=RevolverClipPose();
  if(cartridges)
  {
   if(R.clip==1){if(R.empty){for(int b=0;b<7;++b)bones[b*12+7]=-100;return;}const Mat4 cyl=Multiply(frame,c.crane);for(int b=0;b<7;++b)StoreBone(bones,b,cyl);RevolverHideCasings(bones,RevolverVisibleRounds());return;}
   const int stay=6-R.carryCount;const Mat4 cyl=Multiply(frame,c.crane),moving=Multiply(frame,c.cart);
   for(int i=0;i<6;++i)StoreBone(bones,kRevolverRing[i],i<stay?cyl:moving);StoreBone(bones,6,stay>0?cyl:moving);return;
  }
  Mat4 frameInverse={};if(!Invert(frame,frameInverse))return;
  const Mat4 conj=Multiply(frame,Multiply(c.crane,frameInverse));int first=0,last=-1;
  if(start==12&&count==9432){first=2;last=3;}else if(start==9444&&count==2625){first=last=3;}
  for(int b=first;b<=last;++b)StoreBone(bones,b,Multiply(conj,BoneMatrix(bones,b)));
  return;
 }
 if(cartridges&&R.empty){for(int b=0;b<7;++b)bones[b*12+7]=-100;return;} // rounds out: the cylinder is empty
 if(cartridges&&!R.carrying)RevolverHideCasings(bones,RevolverVisibleRounds()); // fired chambers are not drawn (moves below keep the hides)
 if(R.angle<.01)return;
 Mat4 frameInverse={};if(!Invert(frame,frameInverse))return;
 const Mat4 conj=Multiply(frame,Multiply(RevolverCrane(R.angle),frameInverse));
 int first=0,last=-1;
 if(start==12&&count==9432){first=2;last=3;}else if(start==9444&&count==2625){first=last=3;}else if(cartridges){first=0;last=6;}
 for(int b=first;b<=last;++b)StoreBone(bones,b,Multiply(conj,BoneMatrix(bones,b)));
 if(cartridges)RevolverHideCasings(bones,RevolverVisibleRounds()); // after the swing, so the hidden ones stay hidden
}
void RevolverPalette(ID3D11DeviceContext* context,float* bones,const Mat4& toView,UINT start,UINT count)
{
 if(!RevolverModel())return;
 DrawCall call={};call.a=count;call.b=start;const auto d=NativeWeaponGeometry(context,call);
 if(d.vertexBytes!=186104||d.indexBytes!=31002||d.stride!=40||d.offset!=224||d.format!=57)return;
 RevolverAdjustBones(bones,start,count,toView);
}
void RecordRevolverLeftHand(ID3D11DeviceContext* context,const float* bones)
{
 if(!RevolverModel())return;
 Mat4 view={},inverse={};if(!HandDrawView(context,bones,2,view)||!Invert(ArmBodyToCamera(),inverse))return;
 g_revolver.leftWrist=Multiply(inverse,view);g_revolver.leftFrame=g_frame;
}
// Speedloader pickup zones: the 1911 Waist / Chest zones (head-relative, 18 cm) tested with the hand sphere centre.
bool RevolverPickupZone(int mode,const V3& palmBody,double& distance)
{
 distance=1e9;if(mode==2)return false;
 Mat4 view={},inverse={};for(int i=0;i<16;++i)view.m[i]=g_eyeViewsBody[0].view[i];
 if(!Invert(view,inverse))return false;
 const double scale=(std::max)(.1f,g_worldScale);const V3 head=Apply(inverse,{0,0,0});
 const V3 offset=mode==0?V3{(LeftSidearm315()||LeftShotgun318()||LeftOther319())?.18:-.18,-.65,.03}:V3{0,-.38,.12};
 distance=Length(Sub(palmBody,Add(head,Scale(offset,scale))));return distance<=.18*scale;
}
void RevolverSetOpen(bool open,const char* how)
{
 auto& R=g_revolver;if(R.open==open)return;
 R.open=open;if(open)++R.opens;fc3xr::Buzz(open?0:1); // an emptied cylinder stays empty until a speedloader goes inLog("[REVOLVER] cylinder %s (%s)",open?"open":"closed",how);
}
// Native reload window (Simple): left arm follows the gun with its native animation.
void RevolverNativeBegin(UINT state,UINT wrapper)
{
 if(!RevolverModel()||!g_revolver.simple||!ReloadSignalLocal(state,wrapper))return;
 auto& R=g_revolver;R.native=true;R.nativeAt=GetTickCount64();R.nativeEndAt=0;Log("[REVOLVER] native reload: left arm follows Jason's animation on the gun");
}
void RevolverNativeEnd(UINT state,UINT wrapper)
{
 auto& R=g_revolver;if(!R.native||!ReloadSignalLocal(state,wrapper))return;
 R.nativeEndAt=GetTickCount64();Log("[REVOLVER] native reload end");
}
// 0..1 weight of the gun-attached native left arm (150 ms blend in and out).
double RevolverNativeLeftWeight()
{
 auto& R=g_revolver;if(!R.native)return 0;
 const ULONGLONG now=GetTickCount64();
 if(!RevolverModel()||now-R.nativeAt>6000||(R.nativeEndAt&&now-R.nativeEndAt>150)){R.native=false;return 0;}
 const double in=(std::min)(1.0,(now-R.nativeAt)/150.0),out=R.nativeEndAt?1-(std::min)(1.0,(now-R.nativeEndAt)/150.0):1;
 return (std::min)(in,out);
}
bool RevolverNativeLeft(){RevolverNativeLeftWeight();return g_revolver.native;} // the whole window (weight may still be 0 on its first frame)
// The left hand move during the native reload: blend from the controller move to the gun's move.
void RevolverLeftNativeMove(Mat4& handMove)
{
 const double w=RevolverNativeLeftWeight();if(w<=0)return;
 Mat4 gun={};if(!RightHandMove(gun))return;
 handMove=w>=1?gun:PistolRigidBlend(handMove,gun,w);
}
bool RevolverLeftGrip(float* bones)
{
 if(!RevolverManual()||!(g_revolver.carrying||RevolverAmmoPreview()||RevolverClipPlaying()))return false;
 const Mat4 wrist=BoneMatrix(bones,2);const float* fingers=RevolverClipPlaying()?RevolverClipPose().fingers:kRevolverLeftSeatFingers;
 for(int b=3;b<=19;++b)StoreBone(bones,b,Multiply(wrist,BoneMatrix(fingers,b-3)));
 return true;
}
// Left wrist during Jason's clip: FRAME x clip hand (the 1887 right-hand placement formula).
bool RevolverLeftClipMove(const Mat4& toView,const float* bones,Mat4& move)
{
 if(!RevolverClipPlaying()||!RevolverFrameReady())return false;
 Mat4 inverse={};if(!Invert(Multiply(toView,BoneMatrix(bones,2)),inverse))return false;
 move=Multiply(Multiply(ArmBodyToCamera(),Multiply(g_revolver.frameBody,RevolverClipPose().hand)),inverse);return true;
}
void UpdateRevolverFire(const fc3pad::Controllers&)
{
 const auto& R=g_revolver;fc3pad::RequireFullFirePull(false);
 fc3pad::BlockFire(RevolverManual()&&(R.open||R.angle>.5||R.empty||R.clip>=0)); // cylinder out, emptied (dry), or Jason's animation
}
// Accept only a live six-round HUD associated with this exact weapon. Some
// requests use a different wrapper from the HUD; matching the weapon handles
// that alias without borrowing another gun's reserve or inventing capacity.
bool RevolverAmmo324(UINT wrapper,UINT& loaded,UINT& capacity,UINT& reserve){
 const UINT weapon=ReloadRead(wrapper,0x48);if(!weapon)return false;const UINT count=ReloadRead(weapon,0xcc);if(count>6)return false;
 for(const auto& widget:g_reload.widgets){const UINT observed=UINT(widget.wrapper);if(!observed||!widget.hud||ReloadRead(observed,0x48)!=weapon)continue;
  UINT w[5]={};if(!AmmoRead(UINT(widget.hud)+0x134,w,sizeof(w))||w[1]!=6||w[0]!=count||w[3]>100000)continue;
  loaded=w[0];capacity=w[1];reserve=w[3];return true;
 }return false;
}
// Reload request seam (game thread): Manual fill, capacity checked (the engine transfer ignores capacity).
void RevolverRequest(UINT state,UINT wrapper)
{
 auto& R=g_revolver;
 if(!ReloadSignalLocal(state,wrapper))return;
 R.ammoState=state;R.ammoWrapper=wrapper;if(R.probing){R.probing=false;fc3pad::CancelReloadRequest();Log("[REVOLVER] weapon known: %d rounds loaded",RevolverLoadedRounds());}
 if(R.clip>=0&&(!RevolverActive()||R.clipPauseAt223||g_pistolMenu.open||fc3pad::GameplayInputBlocked()))return;
 if(!R.pending||GetTickCount64()>R.deadline)return;
 const UINT weapon=ReloadRead(wrapper,0x48),before=ReloadRead(weapon,0xcc);
 UINT loaded=0,capacity=0,reserve=0;
 if(!RevolverAmmo324(wrapper,loaded,capacity,reserve))
 {static LONG reports=0;if(InterlockedIncrement(&reports)<=12)Log("[REVOLVER324] awaiting matching ammo HUD: state %08X wrapper %08X weapon %08X count %u; loader retained",state,wrapper,weapon,before);return;}
 R.pending=false;fc3pad::CancelReloadRequest();
 if(R.seatWaitAt324){R.clipAt+=GetTickCount64()-R.seatWaitAt324;R.seatWaitAt324=0;R.poseFrame=-100000;}
 if((std::max)(loaded,before)>=capacity&&R.empty)
 {R.carrying=R.empty=false;R.filled=true;fc3xr::Buzz(1);Log("[REVOLVER] rounds back in: the engine count was still full (%u/%u), no ammo change",(std::max)(loaded,before),capacity);return;}
 if((std::max)(loaded,before)>=capacity||!reserve)
 {R.refused=true;fc3xr::Buzz(0);Log("[REVOLVER] speedloader refused: %u/%u, reserve %u (not retried until the hand leaves the box)",(std::max)(loaded,before),capacity,reserve);return;}
 const bool ok=AkInstantNativeTriple(state,wrapper);const UINT after=ReloadRead(weapon,0xcc);
 if(after>before){R.carrying=R.spent=R.empty=false;R.filled=true;++R.loads;fc3xr::Buzz(1);}
 Log("[REVOLVER] speedloader seated: native Begin/transfer/End, loaded %u -> %u, engine ok=%d",before,after,ok);
}
// The free physical palm, independent of any just-finished loader animation.
bool RevolverSwipePalm323(V3& body,V3& local){
 const auto h=fc3hands303::Read();
 if(h.enabled){
  if(!h.active||!h.engaged||h.empty||h.nativeHoldingSide!=0)return false;const int side=fc3hands303::PhysicalHand(h,1);const auto& a=g_freeHandAttachment[side];
  if(!g_physicalValid303[side]||a.profile!=42||a.epoch!=g_armPoseEpoch||g_frame-a.frame>2)return false;
  Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[side][i];const auto wrist=Multiply(controller,a.relative);const auto& hand=g_revolver.hand;
  body=Apply(wrist,{side?-.591+hand[0]:.591-hand[0],.011+hand[1],1.149+hand[2]});
 }else if(!RevolverPalmBody(body))return false;
 Mat4 inverse={};if(!RevolverFrameReady()||!Invert(g_revolver.frameBody,inverse))return false;local=Apply(inverse,body);return true;
}
void RevolverUpSwipe323(bool grip){
 auto& r=g_revolver;V3 body={},local={};const double scale=(std::max)(.1f,g_worldScale);
 const bool eligible=grip&&r.open&&std::fabs(r.angle-kRevolverOpenDeg)<.5&&!r.carrying&&!r.pending&&!RevolverClipPlaying()&&RevolverSwipePalm323(body,local);
 if(!eligible){r.upSwipe323=false;return;}
 const double reach=r.handDiameter*.5*scale+.04*scale;
 const bool withinCylinder323=RevolverBoxDistance(1,local)<=reach||RevolverBoxDistance(0,local)<=reach;
 const auto cylinder=Apply(r.frameBody,RevolverBoxCentre(1));
 if(!r.upSwipe323){if(withinCylinder323){r.upSwipe323=true;r.upStart323=body;r.upGunY323=cylinder.y;}return;}
 const auto travel=Sub(body,r.upStart323);const double rise=travel.y-(cylinder.y-r.upGunY323);
 if(travel.x*travel.x+travel.z*travel.z>.20*.20*scale*scale){r.upSwipe323=false;return;}
 if(travel.y>=.035*scale&&rise>=.025*scale){
  r.upSwipe323=false;r.grab=false;++r.upCloses323;RevolverSetOpen(false,"free-hand upward swipe");
  Log("[REVOLVER SWIPE323] free-hand rise %.4f relative rise %.4f; physical close, hand stays tracked",travel.y,rise);
 }
}
void TickRevolver(bool gameplay)
{
 auto& R=g_revolver;const auto c=fc3pad::AmmoControls();
 const bool manual=gameplay&&RevolverManual();
 fc3pad::SetXOwned(manual); // Manual: X opens / closes the cylinder, never the game's reload
 const bool lg=c.leftGrip>.7f,lgEdge=lg&&!R.leftGripWas,lgUp=c.leftGrip<.35f,xEdge=c.x&&!R.xWas;
 R.leftGripWas=fc3pad::GripOnlyAmmoEnabled()?(lg||(c.leftGrip>=.35f&&R.leftGripWas)):lg;R.xWas=c.x;
 const ULONGLONG now=GetTickCount64();const double dt=R.lastTick?(std::min)(.1,(now-R.lastTick)/1000.0):0;R.lastTick=now;
 const ULONGLONG resumed223=AdvanceWeaponClip223(R.clipAt,R.clipPauseAt223,R.clip>=0,gameplay&&RevolverActive(),now);
 if(resumed223){if(R.pending)R.deadline+=resumed223;if(R.pulseAt)R.pulseAt+=resumed223;if(R.seatWaitAt324)R.seatWaitAt324+=resumed223;R.poseFrame=-100000;}
 if(!gameplay||!RevolverActive()){R.upSwipe323=false;R.grab=false;R.tiltSince=R.rightPrevAt=0;if(!RevolverModel()){if(R.pending)fc3pad::CancelReloadRequest();R.pending=R.carrying=R.spent=R.grab=R.dropping=false;R.clip=-1;R.clipPauseAt223=0;}return;} // keeps open/empty across weapon switches
 if(R.pending&&now>R.deadline){R.pending=false;fc3pad::CancelReloadRequest();if(R.clip==0&&!R.filled){R.clip=-1;R.carrying=true;R.refused=true;R.grab=false;Log("[REVOLVER324] fill timed out; loader returned to tracked hand, no ammo added");}}
 if(R.probing&&(RevolverLoadedRounds()>=0||now-R.probeAt>400)){R.probing=false;fc3pad::CancelReloadRequest();}
 if(!R.simple&&!R.pending&&!R.probing&&RevolverLoadedRounds()<0&&now-R.probeAt>2000)
 {R.probing=true;R.probeAt=now;fc3pad::RequestReload();Log("[REVOLVER] probing the weapon once to read its loaded rounds (the reload itself stays refused)");}
 if(R.simple)
 {
  R.upSwipe323=false;R.open=R.carrying=R.spent=R.empty=R.grab=false;R.angle=0;R.clip=-1;
  if(lgEdge&&!R.native){fc3pad::RequestReload();Log("[REVOLVER] LEFT grip: game reload");}
  return;
 }
 const double scale=(std::max)(.1f,g_worldScale),handR=R.handDiameter*.5*scale;
 // cylinder angle follows open/closed (fast swing, like the native 3-frame swing)
 const double target=R.open?kRevolverOpenDeg:0;const double step=(R.open?1200:400)*dt; // opens in ~3 frames, closes in ~0.25 s like Jason's push
 R.angle=R.angle<target?(std::min)(target,R.angle+step):(std::max)(target,R.angle-step);
 bool settled=std::fabs(R.angle-target)<.5;
 if(R.clip>=0)
 { R.upSwipe323=false; // Jason's clip owns the left hand, the crane and the rounds until it ends
  const double s=RevolverClipSeconds();R.angle=RevolverCraneDegrees(RevolverClipPose().crane);
  if(R.clip==0&&!R.seatFired&&s>=kRevolverInsertSeat){R.seatFired=true;R.filled=false;R.pending=true;R.deadline=now+3000;R.pulseAt=R.seatWaitAt324=now;R.clipAt=now-ULONGLONG((kRevolverInsertSeat-kRevolverClipInsert[R.clipFirst].t)*1000);R.poseFrame=-100000;fc3pad::RequestReload();Log("[REVOLVER] Jason seats the speedloader: engine fill requested");}
  if(R.clip==0&&R.pending&&now-R.pulseAt>=250){R.pulseAt=now;fc3pad::RequestReload();} // re-send until the engine seam answers
  if(s>=RevolverClipLength(R.clip))
  {
   if(R.clip==0&&R.pending)return; // hold the last frame until the fill lands (or the 3 s deadline clears pending)
   if(R.clip==1){R.open=false;R.angle=0;Log("[REVOLVER] Jason's close animation done");}
   else if(R.filled){R.carrying=false;Log("[REVOLVER] Jason's insert animation done");}
   else{R.carrying=true;R.refused=true;fc3xr::Buzz(0);Log("[REVOLVER] the engine never took the fill: the speedloader stays in the left hand (move it out and back in)");}
   R.clip=-1;
  }
  return;
 }
 V3 palm={},palmBody={};const bool palmOk=RevolverPalmFrame(palm)&&RevolverPalmBody(palmBody);
 const double dR=palmOk?RevolverBoxDistance(0,palm):1e9,dC=palmOk?RevolverBoxDistance(1,palm):1e9;
 const bool inRounds=dR<=handR,inCylinder=dC<=handR,cylinderNearer=dC<dR; // overlapping boxes: the one the hand is deeper in wins
 double dZone=1e9;const bool zone=palmOk&&RevolverPickupZone(R.pickup,palmBody,dZone);
 // X does nothing on its own. LEFT grip + X: rounds out (hand in the ROUNDS box, cylinder open) or, with "Grip + X" pickup
 // and an empty hand anywhere else, a speedloader.
 // Cylinder open with rounds in: tip the muzzle up and they fall out (the only way to empty it).
 const bool loadedOpen=R.open&&settled&&RevolverVisibleRounds()>0&&!R.carrying&&!R.dropping; // rounds actually in (not emptied, not fired dry)
 // X alone never empties it (pressing X used to make all ammo in the cylinder disappear).
 {
  const double bx=R.frameBody.m[1],by=R.frameBody.m[5],bz=R.frameBody.m[9],len=std::sqrt(bx*bx+by*by+bz*bz);
  const bool up=RevolverFrameReady()&&len>1e-6&&by/len>.64; // barrel at least ~40 degrees above level
  if(!loadedOpen||!up)R.tiltSince=0;else if(!R.tiltSince)R.tiltSince=now;
  // Tipping the muzzle up dumps the rounds (LEFT grip + X now gives a speedloader with just the missing rounds instead).
  const bool gripX=false;
  if((R.tiltSince&&now-R.tiltSince>=100)||gripX){R.dropCount=RevolverVisibleRounds();R.dropBody=Multiply(R.frameBody,RevolverCrane(R.angle));R.dropping=true;R.dropAt=now;R.empty=true;R.tiltSince=0;fc3xr::Buzz(0);
   Log(gripX?"[REVOLVER] LEFT grip + X: rounds out of the open cylinder":"[REVOLVER] muzzle up: rounds fall out of the open cylinder");}
  if(R.dropping&&now-R.dropAt>=1200)R.dropping=false;
 }
 // The speedloader only comes once the open cylinder is empty (as Jason does it).
 const int missing=6-RevolverVisibleRounds();
 const bool canLoad=R.open&&settled&&missing>0; // the speedloader carries only the missing rounds
 if(((xEdge&&lg)||(fc3pad::GripOnlyAmmoEnabled()&&lgEdge&&palmOk&&!inCylinder&&!inRounds))&&!R.pending&&!R.carrying&&!R.grab&&(R.pickup==2||fc3pad::AmmoPickupOverride(c)||fc3pad::GripOnlyAmmoEnabled()))
 {
  if(canLoad){R.carrying=true;R.spent=false;R.carryCount=missing;fc3xr::Buzz(1);Log("[REVOLVER] LEFT grip + X: speedloader with %d round(s) in the left hand",missing);}
  else{fc3xr::Buzz(0);Log(R.open?"[REVOLVER] LEFT grip + X: nothing - the cylinder is full":"[REVOLVER] LEFT grip + X: nothing - open the cylinder first (grip it, pull it out)");}
 }
 if(lgEdge&&!R.carrying&&!R.grab&&!R.pending&&settled)
 {
  const char* action="nothing";
  if(zone&&canLoad){R.carrying=true;R.spent=false;R.carryCount=missing;fc3xr::Buzz(1);action=R.pickup?"speedloader taken (chest)":"speedloader taken (waist)";}
  else if(zone){fc3xr::Buzz(0);action="no speedloader: open the cylinder and empty it first";}
  // open: the cylinder and its rear are one part; either box grabs it
  else if(inCylinder||(R.open&&inRounds)){R.grab=true;R.grabStart=palm;fc3xr::Buzz(0);action=R.open?"cylinder grabbed (push it in to close)":"cylinder grabbed (pull it out to open)";}
  else if(inRounds)action=R.open?"in the ROUNDS box":"in the ROUNDS box, cylinder closed";
  if(InterlockedIncrement(&R.gripLogs)<=60)Log("[REVOLVER] LEFT grip: %s | hand sphere %s (d %.3f); ROUNDS box %s (%.3f from surface), CYLINDER box %s (%.3f); pickup %s %.3f; cylinder %s",
   action,palmOk?"ok":"MISSING",R.handDiameter*scale,inRounds?"IN":"out",dR,inCylinder?"IN":"out",dC,R.pickup==2?"grip+X":R.pickup?"chest":"waist",dZone,R.open?"open":"closed");
 }
 // Right-hand flick to the right closes the open cylinder (gun +X velocity of the right controller, body space).
 {
  const V3 rp={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
  if(R.rightPrevAt&&now>R.rightPrevAt&&now-R.rightPrevAt<100&&RevolverFrameReady())
  {
   const double fdt=(now-R.rightPrevAt)/1000.0;const V3 v=Scale(Sub(rp,R.rightPrev),1/fdt);
   V3 gx={R.frameBody.m[0],R.frameBody.m[4],R.frameBody.m[8]};const double gl=Length(gx);
   if(gl>1e-6&&lgUp&&R.open&&settled&&!R.grab&&!R.carrying&&!R.pending&&(RevolverLeftReload325()?-1:1)*Dot(v,Scale(gx,1/gl))>1.1*scale){Log("[REVOLVER] right-hand flick");RevolverStartClip(1);R.rightPrev={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};R.rightPrevAt=now;return;}
  }
  R.rightPrev=rp;R.rightPrevAt=now;
 }
 // Physical opening keeps the original gun-local pull. Closing is the
 // grip-held physical UP swipe below; reaching inward is not a close command.
 if(R.grab)
 {
  if(lgUp||!palmOk)R.grab=false;
   else{const double d=(RevolverLeftReload325()?-1:1)*(palm.x-R.grabStart.x),need=.025*scale;
   if(!R.open&&d<=-need){RevolverSetOpen(true,"pulled out by hand");R.grab=false;}
}
 }
 RevolverUpSwipe323(!lgUp&&(lg||R.leftGripWas||R.upSwipe323));
 // Carried rounds. Fresh speedloader: the hand sphere touching the ROUNDS box of the open cylinder fills it (as the 1911
 // magazine port). Let go: spent rounds back at the box go back in, anywhere else they are dropped; a fresh speedloader is
 // dropped (no ammo change).
 const bool handAtRounds=R.open&&settled&&inRounds;
 if(!handAtRounds)R.refused=false;
 if(R.carrying&&!R.spent&&!R.pending&&!R.refused&&handAtRounds){RevolverStartClip(0);return;} // Jason pushes it in
 if(R.carrying&&lgUp&&!R.pending)
 {
  if(R.spent&&handAtRounds){R.empty=false;Log("[REVOLVER] spent rounds back in");}
  else Log(R.spent?"[REVOLVER] spent rounds dropped":"[REVOLVER] speedloader dropped, no ammo change");
  R.carrying=R.spent=false;
 }
}
// Box + hand sphere guides (Manual play, and the INSERT preview), like the AK/pistol boxes.
void RevolverGuideMarkers()
{
 const auto& R=g_revolver;
 const bool preview=g_pistolMenu.open;
 const bool play=PhysicalReloadGuidesEnabled()&&!g_pistolMenu.open;
 if((!RevolverModel()&&!(preview&&g_weaponMechanicProfile==42))||R.simple||(!RevolverFrameReady()&&!(preview&&R.frameFrame>=0))||!PhysicalReloadGuidesEnabled()||(!play&&!preview)){fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);return;}
 const float scale=(std::max)(.1f,g_worldScale);const double handR=R.handDiameter*.5*scale;
 V3 p={};const bool palmOk=RevolverPalmFrame(p);
 const int b=g_weaponTriggerDiagnostics?(g_weaponTriggerTarget==0?0:1):(R.carrying&&!R.spent)||(R.open&&!R.empty&&!R.grab)?0:1; // rounds box while loading / unloading, else the cylinder box
 const int contact=R.grab||(palmOk&&RevolverBoxDistance(b,p)<=handR)?1:0;
 const Mat4 box=Multiply(R.frameBody,RevolverBoxFrame(b));
 Mat4 basis=box;for(int c=0;c<3;++c){double len=0;for(int r=0;r<3;++r)len+=basis.m[r*4+c]*basis.m[r*4+c];len=std::sqrt(len);if(len<1e-8)return;for(int r=0;r<3;++r)basis.m[r*4+c]/=len;}
 V3 hand={};bool showHand=RevolverPalmBody(hand);if(!showHand&&preview&&R.leftFrame>=0){hand=Apply(R.leftWrist,{-.591+R.hand[0],.011+R.hand[1],1.149+R.hand[2]});showHand=true;}
 float a[3]={static_cast<float>(box.m[3]),static_cast<float>(box.m[7]),static_cast<float>(box.m[11])},h[3]={static_cast<float>(hand.x),static_cast<float>(hand.y),static_cast<float>(hand.z)};
 fc3xr::SetReloadMarkers(a,showHand?h:nullptr,R.boxSize[b][0]*.5f*scale,contact,R.handDiameter*scale); // point marker first: the box renderer needs it
 float transform[16],size[3];for(int i=0;i<16;++i)transform[i]=static_cast<float>(basis.m[i]);for(int i=0;i<3;++i)size[i]=R.boxSize[b][i]*scale;
 fc3xr::SetWeaponBox(transform,size,contact);
 if(showHand)fc3xr::SetHandSphere();
}
// INSERT > HUNTER (.44 Magnum), laid out like the 1911 / AK page.
void RevolverMenu(const wchar_t* savePath)
{
 auto& R=g_revolver;static int result=0;
 {int mode=R.simple?1:0;const char* modes[]={"Manual (physical cylinder and speedloader)","Simple (X or LEFT grip: game reload)"};ImGui::SetNextItemWidth(320*g_menuScale);
  if(ImGui::Combo("Reload mode",&mode,modes,2)){R.simple=mode==1;R.open=R.carrying=R.spent=R.empty=R.grab=false;R.angle=0;result=0;}}
 if(R.simple){ImGui::TextWrapped("Simple: X or LEFT grip runs Jason's reload. Your left hand follows it.");}
 else
 {
  ImGui::TextWrapped("Open: LEFT grip the CYLINDER box and pull it out. Reload: LEFT grip + X gives a speedloader with just the missing rounds. Tip the muzzle up to dump the rounds. Speedloader (only once the cylinder is empty): the Ammo pickup below. Load: hand into the ROUNDS box. Close: hold LEFT grip near the open cylinder and swipe UP. Your hand stays tracked.");

  auto slider=[&](int n,const char* label){
   ImGui::PushID(4400+n);float* v=RevolverValue(n);const int k=RevolverKind(n);
   float shown=k==2?*v:*v*100.f;
   ImGui::SetNextItemWidth((std::max)(55.f*g_menuScale,(std::min)(250.f*g_menuScale,ImGui::GetContentRegionAvail().x-110.f*g_menuScale)));
   if(ImGui::SliderFloat(label,&shown,k==2?-180.f:k==1?1.f:k==3?1.f:-30.f,k==2?180.f:k==1?30.f:k==3?20.f:30.f,k==2?"%.1f deg":"%.1f cm")){*v=k==2?shown:shown*.01f;result=0;}
   ImGui::SameLine();if(ImGui::Button("Default")){*v=RevolverDefault(n);result=0;}ImGui::PopID();};
  if(ImGui::Button("Save All - Revolver"))result=SaveRevolverSettings(savePath)?1:2;
  ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");
  ImGui::Separator();
  if(ImGui::Checkbox("Position speedloader in hand",&R.ammoPreview))result=0;
  if(R.ammoPreview){ImGui::TextWrapped("Preview: the rounds sit in your left hand while INSERT is open. Move and rotate them; Save All keeps it.");
   const char* an[]={"Ammo X","Ammo Y","Ammo Z","Ammo rotate X","Ammo rotate Y","Ammo rotate Z"};for(int i=0;i<6;++i)slider(22+i,an[i]);}
  const char* names[]={"X","Y","Z","Width (X)","Length (Y)","Height (Z)","Rotate X","Rotate Y","Rotate Z"};
  ImGui::Separator();ImGui::TextUnformatted("ROUNDS box: unload + load (weapon local axes)");
  for(int k=0;k<9;++k)slider(k,names[k]);
  ImGui::Separator();ImGui::TextUnformatted("CYLINDER box: grab, pull out / push in (weapon local axes)");
  for(int k=0;k<9;++k)slider(9+k,names[k]);
  ImGui::Separator();ImGui::TextUnformatted("Left hand (offset from palm in wrist axes)");
  slider(18,"X");slider(19,"Y");slider(20,"Z");slider(21,"Diameter");
  ImGui::Separator();ImGui::TextUnformatted("Ammo pickup");
  {const char* pick[]={"Waist","Chest / upper stomach","Grip + X"};ImGui::SetNextItemWidth(220*g_menuScale);if(ImGui::Combo("Ammo pickup",&R.pickup,pick,3))result=0;}
  ImGui::TextWrapped("Grip at the selected body zone (or LEFT grip + X anywhere) takes a speedloader. Release grip to drop it.");
 }
 if(R.simple){if(ImGui::Button("Save All - Revolver"))result=SaveRevolverSettings(savePath)?1:2;
  ImGui::SameLine();ImGui::TextUnformatted(result==1?"Saved":result==2?"SAVE FAILED":"Unsaved edits stay live until restart");}
}
bool RevolverNote()
{
 if(!RevolverActive())return false;
 const auto& R=g_revolver;
 if(R.simple){fc3xr::SetTestNote(R.native?L".44 MAGNUM - RELOADING":L".44 MAGNUM (Simple)\nX or LEFT grip: reload.");return true;}
 fc3xr::SetTestNote(R.clip>=0?L".44 MAGNUM":R.pending?L".44 MAGNUM - LOADING":
  R.carrying?L".44 MAGNUM - SPEEDLOADER\nPut your hand in the ROUNDS box.\nLet go of LEFT grip to drop it.":
  R.open?(R.empty?L".44 MAGNUM - CYLINDER EMPTY\nTake a speedloader (LEFT grip + X).\nHold LEFT grip and swipe UP: close.":L".44 MAGNUM - CYLINDER OPEN\nLEFT grip + X: speedloader (missing rounds).\nHold LEFT grip and swipe UP: close."):
  R.empty?L".44 MAGNUM - EMPTY\nLEFT grip the CYLINDER box, pull it out,\nthen load a speedloader.":L".44 MAGNUM\nLEFT grip the CYLINDER box,\npull it out: open.");
 return true;
}

// ---- Speedloader body (stealthhunter_fastloader.xbg). The speedloader did not appear: the native-draw snapshot
// only existed after a game reload in the same session. Now it is built every session from the asset itself: the mesh
// (480 verts stride 32 / 1362 indices, work/revolver_loader_extract.py -> revolver_loader_mesh.inl) drawn with the
// material of the long barrel's stride-32 draw (start 6 / 1338), which is the SAME material (StealthHunter_Metal,
// JESSIEMARTEL-M-2011062055413018), the same vertex format (flag 3018) and the same PMCP position scale.
// world = inv(view) x ArmBodyToCamera x (wrist x RoundsFromWrist x LoaderFromRounds).
#include "revolver_loader_mesh.inl"
static_assert(sizeof(kRevolverLoaderVB)==15360&&sizeof(kRevolverLoaderIB)==2724,"speedloader mesh = the game's 15360 / 2724-byte buffers");
struct RevolverLoaderVisual{HullPart part;UINT stride=0,offset=0;unsigned long long vsHash=0;ID3D11DeviceContext* deferred=nullptr;LONG frame=-1;bool logged=false,failed=false,fromAsset=false;ID3D11Buffer* vertexExtra249[6]={};}g_revolverLoader;
RevolverLoaderVisual g_revolverLoaderExtra249[5];
RevolverLoaderVisual& RevolverLoaderPass249(int index){return index?g_revolverLoaderExtra249[index-1]:g_revolverLoader;}
// The mod's own copy of the speedloader mesh, replacing the borrowed draw's buffers.
bool RevolverLoaderOwnMesh(HullPart& p)
{
 D3D11_BUFFER_DESC vd={sizeof(kRevolverLoaderVB),D3D11_USAGE_IMMUTABLE,D3D11_BIND_VERTEX_BUFFER,0,0,0},id={sizeof(kRevolverLoaderIB),D3D11_USAGE_IMMUTABLE,D3D11_BIND_INDEX_BUFFER,0,0,0};
 D3D11_SUBRESOURCE_DATA vdata={kRevolverLoaderVB,0,0},idata={kRevolverLoaderIB,0,0};ID3D11Buffer* vb=nullptr,*ib=nullptr;
 if(!g_dev||FAILED(g_dev->CreateBuffer(&vd,&vdata,&vb))||FAILED(g_dev->CreateBuffer(&id,&idata,&ib))){SafeRelease(vb);SafeRelease(ib);return false;}
 SafeRelease(p.vb);SafeRelease(p.ib);p.vb=vb;p.ib=ib;p.indexOffset=0;p.draw.a=kRevolverLoaderDraw[1];p.draw.b=kRevolverLoaderDraw[0];p.draw.d=0;return true;
}
bool RevolverLoaderReady(){return g_revolverLoader.part.ready&&g_revolverLoader.deferred;}
bool RevolverBarrelMaterial217(const fc3weapon::Geometry& d,UINT start,UINT count){
 return d.format==57&&d.stride==32&&d.offset==80&&start==6&&
    ((d.vertexBytes==18032&&d.indexBytes==2688&&count==1338)||
     (d.vertexBytes==14768&&d.indexBytes==2172&&count==1080));
}
void ObserveRevolverLoaderPass249(ID3D11DeviceContext* context,const DrawCall& call,RevolverLoaderVisual& L)
{
 if(L.part.ready||L.failed||call.kind!=0||call.d!=0)return;
 const auto d=NativeWeaponGeometry(context,call);
 const bool native=d.vertexBytes==15360&&d.indexBytes==2724&&d.format==57;
 const bool barrel=RevolverBarrelMaterial217(d,call.b,call.a);
 if(!native&&!barrel)return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);std::vector<float> data(kSmallCbMax/4);const UINT bytes=ReadCopy(cb,data.data(),kSmallCbMax);SafeRelease(cb);
 if(bytes<80)return;data.resize(bytes/4);
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);L.vsHash=ShaderHash(vs);SafeRelease(vs);
 ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&L.stride,&L.offset);SafeRelease(vb);
 bool ok=SnapshotVisualPart(context,call,L.part,data);
 if(ok&&barrel){ok=RevolverLoaderOwnMesh(L.part);L.stride=32;L.offset=0;L.fromAsset=true;}
 // Native color shaders use GlobalShaderParameterProvider and clip planes at
 // different slots than depth shaders. Retain every bound material input.
 if(ok){
  ID3D11Buffer* cb[8]={};context->VSGetConstantBuffers(0,8,cb);
  for(int i=2;i<8;++i)if(cb[i])ok=HullClone(context,cb[i],D3D11_BIND_CONSTANT_BUFFER,&L.vertexExtra249[i-2])&&ok;
  for(auto*& b:cb)SafeRelease(b);context->PSGetConstantBuffers(0,8,cb);
  for(int i=1;i<8;++i)if(cb[i])ok=HullClone(context,cb[i],D3D11_BIND_CONSTANT_BUFFER,&L.part.psCb[i])&&ok;
  for(auto*& b:cb)SafeRelease(b);
 }
 if(ok&&!L.deferred)ok=SUCCEEDED(g_dev->CreateDeferredContext(0,&L.deferred));
 if(!ok){L.failed=true;Log("[REVOLVER] speedloader body could not be built; rounds only");}
 else Log("[LOADER249] pass copied VS%016llX targets%u firstFormat%d depth%d; asset%d, not yet drawn",L.vsHash,L.part.targets,int(L.part.formats[0]),int(L.part.depthFormat),L.fromAsset);
}
void ObserveRevolverLoaderDraw(ID3D11DeviceContext* context,const DrawCall& call){
 if(call.kind!=0||call.d!=0||!RevolverModel()||!MotionInteractionEnabled())return;
 const auto d=NativeWeaponGeometry(context,call);if(!RevolverBarrelMaterial217(d,call.b,call.a))return;
 CameraSummary camera={};if(!PistolGuideCamera(context,camera)||!camera.mainView)return;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);SafeRelease(vs);
 ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* depth=nullptr;context->OMGetRenderTargets(8,rt,&depth);
 DXGI_FORMAT formats[8]={};bool target=false;for(int i=0;i<8;++i){formats[i]=HullFormat(rt[i]);target=target||rt[i]!=nullptr;SafeRelease(rt[i]);}const auto df=HullFormat(depth);const bool valid=target&&depth;SafeRelease(depth);if(!valid)return;
 RevolverLoaderVisual* empty=nullptr;
 for(int i=0;i<6;++i){auto& L=RevolverLoaderPass249(i);if(!L.part.ready&&!L.failed){if(!empty)empty=&L;continue;}bool same=L.vsHash==hash&&L.part.depthFormat==df;for(int k=0;k<8;++k)same=same&&L.part.formats[k]==formats[k];if(same)return;}
 if(empty)ObserveRevolverLoaderPass249(context,call,*empty);
}
Mat4 RevolverLoaderBody()
{
 if(RevolverClipPlaying())return Multiply(g_revolver.frameBody,RevolverClipPose().loader);
 return SidearmCarried315(g_revolver.leftWrist,Multiply(RevolverRoundsFromWrist(),kRevolverLoaderFromRounds));
}
void ReplayRevolverLoaderPass249(ID3D11DeviceContext* context,const DrawCall& call,RevolverLoaderVisual& L)
{
 const auto& R=g_revolver;
 const bool clipLoader=RevolverClipPlaying()&&R.clip==0&&RevolverClipPose().loaderOn;
 if(L.failed||!L.part.ready||!L.deferred||L.frame==g_frame||!((R.carrying&&!RevolverClipPlaying())||RevolverAmmoPreview()||clipLoader)||!RevolverLeftReady()||!RevolverModel())return;
 // Replay each native barrel material/depth stage once, with that stage's
 // own inputs. A depth-stage replay cannot replace the later color stage.
 if(call.kind!=0||call.d!=0||!RevolverBarrelMaterial217(NativeWeaponGeometry(context,call),call.b,call.a))return;
 ID3D11VertexShader* source=nullptr;context->VSGetShader(&source,nullptr,nullptr);const auto sourceHash=ShaderHash(source);SafeRelease(source);if(sourceHash!=L.vsHash)return;
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};UINT cbytes=sizeof(camera);
 bool ok=cameraBuffer&&SUCCEEDED(cameraBuffer->GetPrivateData(kCameraSummaryGuid,&cbytes,&camera))&&cbytes==sizeof(camera)&&camera.mainView;
 ID3D11RenderTargetView* targets[8]={},*twins[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;
 context->OMGetRenderTargets(8,targets,&depth);const auto& p=L.part;
 for(UINT i=0;i<8;++i)ok=ok&&HullFormat(targets[i])==p.formats[i];
 ok=ok&&depth&&HullFormat(depth)==p.depthFormat;
 if(ok){for(UINT i=0;i<p.targets;++i)if(targets[i]){twins[i]=static_cast<ID3D11RenderTargetView*>(TwinView(targets[i],ViewKind::RenderTarget,false));ok=ok&&twins[i];}
  twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth&&g_eyeCb[1][0]&&g_eyeCb[1][1];}
 Mat4 viewInverse={};ok=ok&&Invert(HullView(camera),viewInverse);
 if(ok)
 {
  const Mat4 world=Multiply(viewInverse,Multiply(ArmBodyToCamera(),RevolverLoaderBody()));
  auto data=p.data;WritePistolWorld(data.data(),world); // owned rigid material layout, not Cannon's potentially skinned cb0
  D3D11_VIEWPORT viewport[16]={};UINT nvp=16;context->RSGetViewports(&nvp,viewport);D3D11_RECT scissor[16]={};UINT nsc=16;context->RSGetScissorRects(&nsc,scissor);
  const bool own=g_own;g_own=true;auto* dc=L.deferred;dc->ClearState();dc->RSSetViewports(nvp,viewport);dc->RSSetScissorRects(nsc,scissor);
  context->UpdateSubresource(p.globals,0,nullptr,data.data(),0,0);
  UINT stride=L.stride,offset=L.offset;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);
  dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);dc->IASetInputLayout(p.layout);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
  ID3D11Buffer* vertexCb[8]={p.globals,cameraBuffer,L.vertexExtra249[0]?L.vertexExtra249[0]:p.clip,L.vertexExtra249[1],L.vertexExtra249[2],L.vertexExtra249[3],L.vertexExtra249[4],L.vertexExtra249[5]};dc->VSSetConstantBuffers(0,8,vertexCb);
  dc->PSSetConstantBuffers(0,8,const_cast<ID3D11Buffer**>(p.psCb));dc->PSSetShaderResources(0,16,const_cast<ID3D11ShaderResourceView**>(p.textures));dc->PSSetSamplers(0,16,const_cast<ID3D11SamplerState**>(p.samplers));
  dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
  for(int eye=0;eye<2;++eye)
  {
   dc->OMSetRenderTargets(p.targets,eye?twins:targets,eye?twinDepth:depth);
   dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]); // weapon projection, as the moved weapon pieces
   if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
   dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
  }
  ID3D11CommandList* list=nullptr;
  if(SUCCEEDED(dc->FinishCommandList(FALSE,&list))){context->ExecuteCommandList(list,TRUE);list->Release();L.frame=g_frame;
   if(!L.logged){Log("[LOADER249] speedloader pass VS%016llX drawn in both eyes targets%u format%d",L.vsHash,p.targets,int(p.formats[0]));L.logged=true;}}
  g_own=own;
 }
 for(UINT i=0;i<8;++i){SafeRelease(targets[i]);SafeRelease(twins[i]);}
 SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}

void ReplayRevolverLoader(ID3D11DeviceContext* context,const DrawCall& call){for(int i=0;i<6;++i)ReplayRevolverLoaderPass249(context,call,RevolverLoaderPass249(i));}
void ClearBowArrowPart(HullPart&);
void ReleaseRevolverLoader249(){for(int i=0;i<6;++i){auto& L=RevolverLoaderPass249(i);ClearBowArrowPart(L.part);for(auto*& b:L.vertexExtra249)SafeRelease(b);SafeRelease(L.deferred);L=RevolverLoaderVisual{};}}

