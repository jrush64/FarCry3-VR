#include "d50_live_data.inl"
// Desert Eagle D50: its own pistol rig on the 1911 path (captured PID102884 idle f11186) through the real BuildWeaponWorld
// GPU path; empty reload = Skorpion-style slide pull with Jason's captured hand path.
void TestD50(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const auto oldSlot=g_magazineStates[10];const LONG oldProfile=g_weaponMechanicProfile;
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 g_weaponMechanicProfile=12;g_weaponMechanicFrame=++g_frame;
 Check(PistolModel()&&D50Model()&&!P6p9Model()&&!AkModel()&&&CurrentPistolMesh()==&kD50Mesh,"D50 = a recognized pistol with its own part table");
 Check(MagazineFamilyOfProfile(12)==10&&!RifleFamily(10)&&&g_pistolReload==&g_magazineStates[10]&&wcscmp(MagazineSettingsSection(),L"D50Reload")==0,"D50 has its own magazine slot and settings, on the PISTOL page");
 Check(closeMat(MagazineHandFromFrame(),kD50HandFromFrameM,1e-9)&&closeMat(PistolMagazineFromHandCur(),kD50MagazineFromHandM,1e-9)&&PistolMagazineGripCur()==kD50MagazineGripRelative,"D50 right hand, magazine-in-hand and magazine grip are its own captured poses");
 Check(PistolMeshFrame(2004,8148)&&PistolMeshFrame(30,1104)&&!PistolMeshFrame(10995,369)&&PistolMeshSlide(11364,5607)&&PistolMeshSlide(16971,1194),"the three D50 FRAME draws (not 10995, which sits 14 cm off the origin) and both slide draws mapped");
 {Mat4 h0={},h1={};float f0[204],f1[204];Check(RifleChargeClipSample(0,h0,f0)&&RifleChargeClipSample(1,h1,f1)&&!closeMat(h0,h1,1e-3),"D50 slide pull: Jason's own hand path from the capture");}
 g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;
 ID3D11Buffer* vb=Buffer(device,202816,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,39126,D3D11_BIND_INDEX_BUFFER);
 ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=320;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,kD50Camera,12);memcpy(cam.up,kD50Camera+3,12);memcpy(cam.forward,kD50Camera+6,12);memcpy(cam.position,kD50Camera+9,12);cam.mainView=1;
 camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 g_frameGunValid=true;g_frameGunMove=HandIdentity();
 globals->SetPrivateData(kCbCopyGuid,88*4,kD50FrameGlobals);auto* output=BuildWeaponWorld(context,2004,8148);Check(output!=nullptr,"D50 FRAME draw GPU override");
 float frame[88]={};Read(device,context,output,frame);Mat4 inverse={};Check(Invert(HullWorld(frame),inverse)&&p.rootFrame==g_frame,"D50 FRAME root learned");
 float part[88]={};
 globals->SetPrivateData(kCbCopyGuid,88*4,kD50FrameGlobals);output=BuildWeaponWorld(context,10995,369);Read(device,context,output,part);
 {Mat4 fi={};Invert(HullWorld(frame),fi);const Mat4 rel=Multiply(fi,HullWorld(part));Check(std::fabs(rel.m[3])<1e-3&&std::fabs(rel.m[7])<1e-3&&std::fabs(rel.m[11])<1e-3,"10995 keeps its own native offset on the corrected root (here: drawn with the FRAME globals = no offset)");}
 globals->SetPrivateData(kCbCopyGuid,88*4,kD50ClipGlobals);output=BuildWeaponWorld(context,10152,843);Read(device,context,output,part);
 Check(closeMat(Multiply(inverse,HullWorld(part)),kD50ClipRestM,.001),"D50 seated magazine at its captured rest");
 p.rackHeld=true;p.slidePull=.05;
 globals->SetPrivateData(kCbCopyGuid,88*4,kD50Part0Globals);output=BuildWeaponWorld(context,11364,5607);Read(device,context,output,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));Check(std::fabs(rel.m[7]-(kD50SlideRestM.m[7]-kD50SlideTravel))<.001,"full pull: the D50 slide goes its full native 7 cm back");}
 p.rackHeld=false;p.slidePull=0;p.emptyLocked=true;
 output=BuildWeaponWorld(context,16971,1194);Read(device,context,output,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));Check(std::fabs(rel.m[7]-(kD50SlideRestM.m[7]-kD50LockPull))<.001,"empty: the second slide piece sits at the 5.6 cm slide lock");}
 p.emptyLocked=false;
 { // Reported: the gun showed up in 2 parts: the slide drawn BEFORE the FRAME in a new frame must land on this frame's corrected root
  Mat4 fixed124={};Check(p.parentValid&&ControllerMagazineRoot124(fixed124)&&closeMat(p.rootBody,fixed124,1e-6),"D50 uses fixed captured grip immediately instead of learning draw animation");
  for(int i=0;i<16;++i){++g_frame;g_weaponMechanicFrame=g_frame;globals->SetPrivateData(kCbCopyGuid,88*4,kD50FrameGlobals);BuildWeaponWorld(context,2004,8148);}
  Check(p.parentValid&&closeMat(p.rootBody,fixed124,1e-6),"D50 native idle frames cannot relearn the fixed grip");
  p.held=true; // pose-owned (as during a reload): the learned hold is in use
  ++g_frame;g_weaponMechanicFrame=g_frame;g_frameGunMove=HandIdentity();g_frameGunMove.m[3]=.3; // the plain hand move differs from the attachment
  globals->SetPrivateData(kCbCopyGuid,88*4,kD50Part0Globals);output=BuildWeaponWorld(context,11364,5607);Read(device,context,output,part);
  Check(p.rootFrame==g_frame,"no FRAME drawn yet this frame: the root is predicted from the controller attachment");
  const Mat4 earlyRoot=p.rootWorld;
  globals->SetPrivateData(kCbCopyGuid,88*4,kD50FrameGlobals);output=BuildWeaponWorld(context,2004,8148);float later[88]={};Read(device,context,output,later);
  Check(closeMat(HullWorld(later),earlyRoot,1e-4),"the FRAME drawn afterwards uses the very same root");
  Mat4 laterInverse={};Invert(HullWorld(later),laterInverse);
  Check(std::fabs(Multiply(laterInverse,HullWorld(part)).m[7]-kD50SlideRestM.m[7])<.002,"the early slide sits on that root at its rest (one gun, not two)");
  p.held=false;g_frameGunMove=HandIdentity();
  // Reported (one-handed holding had misplaced pieces): normal holding = the plain hand move for EVERY piece, no learned hold
  ++g_frame;g_weaponMechanicFrame=g_frame;g_frameGunMove.m[3]=.3;
  globals->SetPrivateData(kCbCopyGuid,88*4,kD50Part0Globals);output=BuildWeaponWorld(context,11364,5607);float s1[88]={};Read(device,context,output,s1);
  globals->SetPrivateData(kCbCopyGuid,88*4,kD50FrameGlobals);output=BuildWeaponWorld(context,2004,8148);float f1[88]={};Read(device,context,output,f1);
  {Mat4 fi={};Invert(HullWorld(f1),fi);Check(std::fabs(Multiply(fi,HullWorld(s1)).m[7]-kD50SlideRestM.m[7])<.002,"normal hold: slide drawn before the FRAME still sits on the same gun (both plain hand move)");
   Check(!PistolPoseOwned()&&D50PredictRoot(context,0),"normal hold: early pieces predict the shared controller-owned root");}
  { // Reported (capture 57124): the SKINNED holographic sight followed the bare hand during reloads (up to 7 cm off the gun)
   ID3D11Buffer* svb=Buffer(device,48520,D3D11_BIND_VERTEX_BUFFER),*sib=Buffer(device,7032,D3D11_BIND_INDEX_BUFFER);UINT sstride=40,soffset=1680;
   context->IASetVertexBuffers(0,1,&svb,&sstride,&soffset);context->IASetIndexBuffer(sib,DXGI_FORMAT_R16_UINT,96); // an index offset: NativeWeaponGeometry rejects it
   float sightBones[48*12]={};Mat4 bone0=HandIdentity();bone0.m[3]=.02;bone0.m[7]=.18;bone0.m[11]=.16;for(int b=0;b<48;++b)StoreBone(sightBones,b,bone0);
   Mat4 toView=HullView(cam);Mat4 mv={};
   Check(RifleSkinnedAttachmentMove(context,168,3030,sightBones,toView,mv),"plain hold: the sight follows the fixed gun and learns its native FRAME offset");
   Mat4 ni={};Invert(p.rootNative,ni);const Mat4 rel0=Multiply(ni,bone0); // the learned offset: native sight world vs native gun
   p.held=true;p.handFrame=-100000;Mat4 keepRoot=p.rootWorld;p.rootWorld.m[3]+=.25; // pose owned, the gun placed elsewhere
   Check(RifleSkinnedAttachmentMove(context,168,3030,sightBones,toView,mv),"reload: the sight takes the corrected-gun move");
   const Mat4 placed=Multiply(mv,Multiply(toView,bone0));Mat4 vi={};Invert(HullView(cam),vi);const Mat4 placedWorld=Multiply(vi,placed);
   Mat4 ri={};Invert(p.rootWorld,ri);const Mat4 rel=Multiply(ri,placedWorld);
   Check(std::fabs(rel.m[3]-rel0.m[3])<.002&&std::fabs(rel.m[7]-rel0.m[7])<.002&&std::fabs(rel.m[11]-rel0.m[11])<.002,"the sight sits on the moved gun at its learned offset (not on the hand)");
   p.held=false;p.rootWorld=keepRoot;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);svb->Release();sib->Release();}
  g_frameGunMove=HandIdentity();}
 // the state PistolNativeEnd leaves after an EMPTY D50 reload (that branch is the Skorpion's, profile 12): slide pull required
 p.chamberReady=false;p.needsRack=p.rackMode=true;p.emptyLocked=true;
 Check(PistolBusy(),"empty D50 reload: fire locked until the slide is pulled");
 p.rootBody=HandIdentity();p.rootEpoch=g_armPoseEpoch;p.rootFrame=p.palmFrame=g_frame;p.tip={g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]};
 float oldLeft[16];memcpy(oldLeft,g_handBody[1],sizeof(oldLeft));
 for(int i=0;i<16;++i)g_handBody[1][i]=i%5==0?1.f:0.f;
 c.leftGrip=0;AdvancePistolRack(c,1000);c.leftGrip=1;AdvancePistolRack(c,1020);Check(p.rackHeld,"LEFT grip in the slide box takes the slide");
 Check(RifleChargeHandHeld(),"while pulling, Jason's hand is on the slide (captured path)");
 g_handBody[1][7]-=.045f;AdvancePistolRack(c,1040);Check(p.rackRear,"full rear stroke read from the real controller");
 c.leftGrip=0;AdvancePistolRack(c,1060);AdvancePistolRack(c,1200);Check(!p.needsRack&&p.chamberReady,"slide released: chambered");
 memcpy(g_handBody[1],oldLeft,sizeof(oldLeft));
 std::puts("PASS: Desert Eagle D50: own pistol rig and settings (family 10, pistol page); actual GPU: 4 FRAME draws one root, magazine at its captured rest, slide 7 cm full stroke and 5.6 cm empty lock; EMPTY reload locks fire until the slide is pulled (Jason's captured hand on it, stroke from the real controller).");
 context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();
 g_magazineStates[10]=oldSlot;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;fc3pad::SetMagazineHandling(false);
}
