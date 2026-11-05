#include "rifles_live_data.inl"
#include "famas_live_data.inl"
#include "bz19_live_data.inl"
#include "p416_live_data.inl"
#include "ace_live_data.inl"
#include "svd_live_data.inl"
#include "vector_live_data.inl"
#include "r700_live_data.inl"
#include "u100_live_data.inl"
#include "mkg_live_data.inl"
#include "z93_live_data.inl"
// A2000 / STG-90 through the real BuildWeaponWorld GPU path with PID56072 idle data:
// every FRAME material shares one corrected root, the seated clip sits at its captured rest,
// the charging handle sits at its captured rest, and the A2000 knob rides its native -Y travel.
void TestRifleGpu(ID3D11Device* device,ID3D11DeviceContext* context,LONG profile,const float* frameGlobals,const float* clipGlobals,const float* partGlobals,const float* cameraData)
{
 const auto& rifle=RifleForProfile(profile);
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;
 g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 Check(AkModel()&&&CurrentRifle()==&rifle,"rifle profile selects its table");
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;
 float oldController[16];memcpy(oldController,g_handBody[0],sizeof(oldController));const Mat4 identity=HandIdentity();for(int i=0;i<16;++i)g_handBody[0][i]=static_cast<float>(identity.m[i]);
 ID3D11Buffer* vb=Buffer(device,rifle.vb,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,rifle.ib,D3D11_BIND_INDEX_BUFFER);
 ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=rifle.offset; // SVD / R700 bind the vertex buffer at an offset
 context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,cameraData,12);memcpy(cam.up,cameraData+3,12);memcpy(cam.forward,cameraData+6,12);memcpy(cam.position,cameraData+9,12);cam.mainView=1;
 camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 g_frameGunValid=true;g_frameGunMove=identity;
 int frames=0;float first[88]={},part[88]={};Mat4 inverse={};
 if(profile==2){g_r700.freeFromController=kR700HandFromFrame;g_r700.freeEpoch=g_armPoseEpoch;} // R700: root = controller x free hand x inv(grip) - here the free hand = the grip, so root = controller
 for(const auto& f:rifle.frame){if(!f[1])continue;
  globals->SetPrivateData(kCbCopyGuid,88*4,frameGlobals);auto* out=BuildWeaponWorld(context,f[0],f[1]);Check(out!=nullptr,"rifle FRAME GPU override");
  if(!frames){Read(device,context,out,first);Check(Invert(HullWorld(first),inverse)&&p.parentValid,"rifle FRAME root and controller attachment learned");
   if(profile==2){double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(p.rootBody.m[i]-HandIdentity().m[i]));Check(d<1e-6,"R700 root = the controller (never the game's hand)");}}
  else{Read(device,context,out,part);Compare(part,first,12,"every rifle FRAME material shares the first corrected root");}
  ++frames;}
 globals->SetPrivateData(kCbCopyGuid,88*4,clipGlobals);auto* out=BuildWeaponWorld(context,rifle.clip[0],rifle.clip[1]);Read(device,context,out,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-rifle.clipRest->m[i])<.001,"seated rifle clip at its captured rest on the corrected FRAME");}
 globals->SetPrivateData(kCbCopyGuid,88*4,partGlobals);out=BuildWeaponWorld(context,rifle.part[0][0],rifle.part[0][1]);Read(device,context,out,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-rifle.partRest[0]->m[i])<.001,"charging handle at its captured rest on the corrected FRAME");}
 if(rifle.emptyNeedsCharge){
  p.needsRack=p.rackMode=p.rackHeld=true;p.slidePull=.045;
  globals->SetPrivateData(kCbCopyGuid,88*4,partGlobals);out=BuildWeaponWorld(context,rifle.part[0][0],rifle.part[0][1]);Read(device,context,out,part);
  const Mat4 rel=Multiply(inverse,HullWorld(part));Mat4 expected=*rifle.partRest[0];expected.m[7]-=.045*(rifle.chargeTravel/.05);
  for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-expected.m[i])<.001,"A2000 knob moves only along FRAME-local -Y, scaled to its native 10 cm travel");
  float fingers[576];memcpy(fingers,profile==14?kFamasLeftBones:profile==9?kBz19LeftBones:profile==31?kP416LeftBones:profile==17?kAceLeftBones:profile==35?kSvdLeftBones:profile==2?kR700LeftBones:profile==36?kU100LeftBones:profile==22?kMkgLeftBones:profile==23?kZ93LeftBones:kA2000LeftBones,sizeof(fingers));const Mat4 wrist=BoneMatrix(fingers,2);
  Check(PistolGrip(fingers),"charging-handle pull selects the native charging finger pose");
  const float* grip=rifle.chargeHand?RifleChargeFingers():rifle.chargeGrip; // MP5 / F1: Jason's clip fingers at this pull
  for(int b=3;b<=19;++b){const Mat4 e=Multiply(wrist,BoneMatrix(grip,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-e.m[i])<.001,"charging fingers keep the tracked wrist");}
  p.needsRack=p.rackMode=p.rackHeld=false;p.slidePull=0;
 }
 if(profile==41){
  // Reproduce the live bug: holding a magazine (pose owned) while the native gun drifts away.
  // The reflex sight (separate attachment model, solid 0/168 stride 28) must stay on the corrected FRAME.
  p.held=true;p.handFrame=-100000;++g_frame;g_weaponMechanicFrame=g_frame;Check(PistolPoseOwned(),"holding ammo owns the rifle pose");
  Mat4 drifted=HullWorld(frameGlobals);drifted.m[3]+=.4;drifted.m[11]-=.2;float native[88];memcpy(native,frameGlobals,sizeof(native));WritePistolWorld(native,drifted);
  globals->SetPrivateData(kCbCopyGuid,sizeof(native),native);auto* o=BuildWeaponWorld(context,rifle.frame[0][0],rifle.frame[0][1]);float corrected[88]={};Read(device,context,o,corrected);
  Compare(corrected,first,12,"pose-owned rifle FRAME stays on the controller attachment despite native drift");
  Mat4 sightLocal=HandIdentity();sightLocal.m[3]=-.0028;sightLocal.m[7]=.1507;sightLocal.m[11]=.0414; // PID56072 reflex_a offset on the STG-90 FRAME
  float sight[88];memcpy(sight,frameGlobals,sizeof(sight));WritePistolWorld(sight,Multiply(drifted,sightLocal));
  ID3D11Buffer* svb=Buffer(device,48520,D3D11_BIND_VERTEX_BUFFER),*sib=Buffer(device,7032,D3D11_BIND_INDEX_BUFFER);UINT sstride=28,soffset=0;
  context->IASetVertexBuffers(0,1,&svb,&sstride,&soffset);context->IASetIndexBuffer(sib,DXGI_FORMAT_R16_UINT,0);
  Mat4 attach={};Check(RifleAttachmentMove(context,0,168,attach),"reflex sight recognized as a rifle attachment");
  globals->SetPrivateData(kCbCopyGuid,sizeof(sight),sight);o=BuildWeaponWorld(context,0,168);float placed[88]={};Read(device,context,o,placed);
  {const Mat4 rel=Multiply(inverse,HullWorld(placed));for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-sightLocal.m[i])<.001,"holographic sight stays mounted on the corrected STG-90 while a magazine is held");}
  ID3D11Buffer* ovb=Buffer(device,1234*32,D3D11_BIND_VERTEX_BUFFER);UINT ostride=32,ooffset=0;context->IASetVertexBuffers(0,1,&ovb,&ostride,&ooffset);
  Mat4 none={};Check(!RifleAttachmentMove(context,0,168,none),"non-attachment geometry keeps the ordinary hand move");
  context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);svb->Release();sib->Release();ovb->Release();p.held=false;
 }
 if(profile==2)
 { // Z93 scope / R700 silencer case: an attachment drawn BEFORE the gun is placed from the learned offset on THIS frame's gun
  ID3D11Buffer* avb=Buffer(device,7744,D3D11_BIND_VERTEX_BUFFER),*aib=Buffer(device,1680,D3D11_BIND_INDEX_BUFFER);UINT as=32,ao=0;
  Mat4 attLocal=HandIdentity();attLocal.m[7]=.25;attLocal.m[11]=.16;float att[88];memcpy(att,frameGlobals,sizeof(att));WritePistolWorld(att,Multiply(HullWorld(frameGlobals),attLocal));
  float placed[88]={},drawnGun[88]={};
  for(int k=0;k<3;++k)
  {
   ++g_frame;g_weaponMechanicFrame=g_frame;
   context->IASetVertexBuffers(0,1,&avb,&as,&ao);context->IASetIndexBuffer(aib,DXGI_FORMAT_R16_UINT,0);
   globals->SetPrivateData(kCbCopyGuid,sizeof(att),att);auto* ao2=BuildWeaponWorld(context,0,840);if(ao2)Read(device,context,ao2,placed);
   context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
   globals->SetPrivateData(kCbCopyGuid,88*4,frameGlobals);auto* fo=BuildWeaponWorld(context,rifle.frame[0][0],rifle.frame[0][1]);if(fo)Read(device,context,fo,drawnGun);
  }
  Mat4 gi={};Invert(HullWorld(drawnGun),gi);const Mat4 rel=Multiply(gi,HullWorld(placed));
  Check(std::fabs(rel.m[3]-attLocal.m[3])<.002&&std::fabs(rel.m[7]-attLocal.m[7])<.002&&std::fabs(rel.m[11]-attLocal.m[11])<.002,"attachment drawn before the gun sits on the drawn gun at its native offset (was off: view applied twice)");
  avb->Release();aib->Release();
 }
 V3 aim={};Check(PistolAimBody(aim),"rifle aim follows its corrected barrel");
 context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();memcpy(g_handBody[0],oldController,sizeof(oldController));
}
// MP5: skinned rifle. The palette placement is pure given the reload state: every slot conjugated onto the corrected FRAME,
// the magazine slot follows the reload (rest / hidden / in the hand), the handle + bolt slots ride the physical pull.
void TestMp5()
{
 const LONG oldProfile=g_weaponMechanicProfile;const auto s5=g_magazineStates[5];
 g_weaponMechanicProfile=28;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_pistolMenu.open=false;
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 Check(AkModel()&&CurrentRifle().skinned&&CurrentRifle().profile==28&&MagazineFamilyOfProfile(28)==5&&wcscmp(MagazineSettingsSection(),L"Mp5Reload")==0,"MP5 = skinned rifle, family 5, own settings");
 {fc3weapon::Geometry g={306360,55536,40,0,12234,15534,57};Check(fc3weapon::ProfileIndex(g)==28&&RifleMain(CurrentRifle(),g),"MP5 main draw = profile 28; stride-40 main stream accepted");}
 Mp5Slots s={};Check(Mp5DrawSlots(12234,15534,s)&&s.anchor==4&&s.clip==1&&s.handle==2&&Mp5DrawSlots(0,4149,s)&&s.anchor==5&&s.bolt==1&&s.handle==2&&s.clip<0&&!Mp5DrawSlots(5,5,s),"palette slot map per draw (ANCHOR, CLIP, handle, bolt)");
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;p.rootBody=HandIdentity();p.rootWorld=HandIdentity();p.rootFrame=g_frame;
 const Mat4 view=HandIdentity(),fromView=HandIdentity();
 Mat4 frame=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});
 auto fill=[&](float* bones){Mat4 rawFrame=HandIdentity();rawFrame.m[3]=.05;for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());StoreBone(bones,4,rawFrame);
  Mat4 clipRaw=rawFrame;clipRaw.m[11]+=.01;StoreBone(bones,1,clipRaw);Mat4 other=rawFrame;other.m[7]+=.02;StoreBone(bones,0,other);};
 float bones[48*12];fill(bones);Mp5DrawSlots(12234,15534,s);
 Mp5PlaceSlots(bones,s,frame,fromView,view);
 Check(closeMat(BoneMatrix(bones,4),frame,1e-6),"ANCHOR slot = the corrected FRAME");
 {Mat4 other=HandIdentity();other.m[7]=.02;Check(closeMat(BoneMatrix(bones,0),Multiply(frame,other),1e-6),"untouched slots keep their native pose relative to FRAME (conjugation)");}
 Check(closeMat(BoneMatrix(bones,1),Multiply(frame,Multiply(kMp5ClipRest,Mp5BindInverse(kMp5ClipRest))),1e-6)&&closeMat(BoneMatrix(bones,1),frame,1e-6)&&p.clipFrame==g_frame,"seated magazine: slot = FRAME (bind pose), clip world recorded");
 {Mat4 rawFrame=HandIdentity();rawFrame.m[3]=.05;Mat4 inv={};Invert(rawFrame,inv);Check(closeMat(BoneMatrix(bones,2),Multiply(frame,inv),1e-6),"handle untouched outside a reload (native pose, conjugated)");}
 p.magazinePresent=false;fill(bones);Mp5PlaceSlots(bones,s,frame,fromView,view);Check(bones[1*12+7]==-100,"magazine out: the CLIP slot is hidden");
 p.held=true;p.handFrame=g_frame;p.hand=TurnAndMove({1,0,0},{0,1,0},{0,0,0},{1,2,3});fill(bones);Mp5PlaceSlots(bones,s,frame,fromView,view);
 Check(closeMat(BoneMatrix(bones,1),Multiply(p.hand,Mp5BindInverse(kMp5ClipRest)),1e-6),"magazine held: the CLIP slot follows the left hand (bind removed)");
 p.held=false;p.magazinePresent=true;p.needsRack=p.rackMode=p.rackHeld=true;p.slidePull=.05;fill(bones);Mp5PlaceSlots(bones,s,frame,fromView,view);
 Check(closeMat(BoneMatrix(bones,2),Multiply(frame,Multiply(kMp5HandleBack,Mp5BindInverse(kMp5HandleRest))),1e-6),"full pull: the charging handle slot is at its captured back pose (6.6 cm back, swung out)");
 p.slidePull=.025;fill(bones);Mp5PlaceSlots(bones,s,frame,fromView,view);
 {const Mat4 h=BoneMatrix(bones,2),r=Multiply(frame,Multiply(kMp5HandleRest,Mp5BindInverse(kMp5HandleRest)));const double d=std::sqrt((h.m[3]-r.m[3])*(h.m[3]-r.m[3])+(h.m[7]-r.m[7])*(h.m[7]-r.m[7])+(h.m[11]-r.m[11])*(h.m[11]-r.m[11]));
  Check(d>.02&&d<.05,"half pull: the handle is part way along its native path");}
 Mp5DrawSlots(0,4149,s);fill(bones);StoreBone(bones,5,BoneMatrix(bones,4));Mp5PlaceSlots(bones,s,frame,fromView,view);
 Check(closeMat(BoneMatrix(bones,1),Multiply(frame,Multiply(PistolRigidBlend(kMp5BoltRest,kMp5BoltBack,.5),Mp5BindInverse(kMp5BoltRest))),1e-6),"draw 0: the bolt slot rides the same pull");
 { // Reported: handle pull "clipping and not the correct animation pose": the hand plays Jason's own captured path
  Check(CurrentRifle().emptyNeedsCharge,"229 MP5 empty reload requires the existing handle mechanism");
  Mat4 h0={},h1={};float f0[204],f1[204];Check(RifleChargeClipSample(0,h0,f0)&&RifleChargeClipSample(1,h1,f1),"MP5 charging hand sampled (unused: kept for a future rule)");
  Check(closeMat(h0,Multiply(kMp5HandleRest,kMp5HandFromHandle),1e-6),"pull 0: Jason's handle grip ON the MP5 handle at rest");
  Check(closeMat(h1,Multiply(kMp5HandleBack,kMp5HandFromHandle),1e-6)&&!closeMat(h0,h1,1e-3),"pull 1: the hand rides the handle to its back pose (6.6 cm back, swung out)");
  {int k=0;for(int i=0;i<kFamasClipEmptyReloadCount;++i)if(kFamasClipEmptyReload[i].t<=kFamasEmptySlideBackAt)k=i;
   const Mat4 fh=RifleMat12(kFamasClipEmptyReload[k].leftHand),fs=RifleMat12(kFamasClipEmptyReload[k].slide);
   const double f1=Length(Sub(Apply(fh,{-.591,.011,1.149}),{fs.m[3],fs.m[7],fs.m[11]})),mp5=Length(Sub(Apply(h1,{-.591,.011,1.149}),{kMp5HandleBack.m[3],kMp5HandleBack.m[7],kMp5HandleBack.m[11]}));
   Check(std::fabs(f1-mp5)<.005,"the hand sits on the MP5 handle exactly as Jason's sits on the F1 handle (same palm-to-handle distance)");}
  g_weaponMechanicProfile=14;g_weaponMechanicFrame=g_frame;Check(CurrentRifle().emptyNeedsCharge&&CurrentRifle().chargeHand,"F1: EMPTY reload needs the middle handle pulled");
  Check(RifleChargeClipSample(0,h0,f0)&&RifleChargeClipSample(1,h1,f1)&&!closeMat(h0,h1,1e-3),"F1: the hand plays Jason's own path from the pull start to the handle fully back");
  {int k=0;for(int i=0;i<kFamasClipEmptyReloadCount;++i)if(kFamasClipEmptyReload[i].t<=kFamasEmptySlideBackAt)k=i;Check(closeMat(h1,RifleMat12(kFamasClipEmptyReload[k].leftHand),1e-6),"F1 pull 1 = Jason's hand at the handle-back key");}
  {auto& q=g_pistolReload;const auto keep=q;q.enabled=true;q.needsRack=q.rackMode=q.rackHeld=true;q.slidePull=.05;q.rootFrame=g_frame;q.rootEpoch=g_armPoseEpoch;Check(RifleChargeHandHeld()&&RifleChargeFingers()!=CurrentRifle().chargeGrip,"F1: while pulling, the fingers come from the clip too");q=keep;}
  g_weaponMechanicProfile=28;}
 p=PistolReloadState{};p.enabled=true;g_magazineStates[5]=s5;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;
 std::puts("PASS: MP5 skinned rifle: profile 28 / family 5; palette slots conjugated onto the corrected FRAME; magazine slot at rest / hidden when out / in the left hand; charging handle and bolt slots ride the physical pull along the captured native path.");
}
// MK16: skinned like the MP5 but five draws; the BULLET draw has no ANCHOR (its own slot anchors, follows the clip).
void TestMk16()
{
 const LONG oldProfile=g_weaponMechanicProfile;const auto s9=g_magazineStates[9];
 g_weaponMechanicProfile=25;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_pistolMenu.open=false;
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 Check(AkModel()&&CurrentRifle().skinned&&CurrentRifle().profile==25&&MagazineFamilyOfProfile(25)==9&&CurrentRifle().emptyNeedsCharge,"MK16 = skinned rifle, family 9, charging handle after an EMPTY reload");
 Mp5Slots s={};
 Check(Mp5DrawSlots(0,5412,s)&&s.anchor==2&&s.clip==0&&Mp5DrawSlots(11028,13881,s)&&s.anchor==3&&s.handle==0&&s.bolt==1&&Mp5DrawSlots(10296,732,s)&&s.anchor==0&&s.clip==0&&Mp5DrawSlots(5412,2076,s)&&s.clip<0&&!Mp5DrawSlots(4149,8085,s),"MK16 slot map (CLIP, BOLTSLIDE, BOLT, BULLET with the clip, ANCHOR)");
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;p.rootBody=HandIdentity();p.rootWorld=HandIdentity();p.rootFrame=g_frame;
 const Mat4 frame=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});
 float bones[48*12];for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Mp5DrawSlots(11028,13881,s);
 p.needsRack=p.rackMode=p.rackHeld=true;p.slidePull=.05;Mp5PlaceSlots(bones,s,frame,HandIdentity(),HandIdentity());
 Check(closeMat(BoneMatrix(bones,0),Multiply(frame,Multiply(kMk16HandleBack,Mp5BindInverse(kMk16HandleRest))),1e-6),"full pull: BOLTSLIDE at its captured back pose (9.5 cm)");
 Check(closeMat(BoneMatrix(bones,1),Multiply(frame,Multiply(kMk16BoltBack,Mp5BindInverse(kMk16BoltRest))),1e-6),"the BOLT swings with it (captured back pose)");
 p.needsRack=p.rackMode=p.rackHeld=false;p.slidePull=0;p.magazinePresent=false;for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Mp5DrawSlots(10296,732,s);Mp5PlaceSlots(bones,s,frame,HandIdentity(),HandIdentity());
 Check(bones[7]==-100,"magazine out: the BULLET draw hides with it");
 {Mat4 h0={},h1={};float f0[204],f1[204];Check(RifleChargeClipSample(0,h0,f0)&&RifleChargeClipSample(1,h1,f1)&&!closeMat(h0,h1,1e-3),"MK16 pull: Jason's own hand path");}
 p=PistolReloadState{};p.enabled=true;g_magazineStates[9]=s9;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;
 std::puts("PASS: MK16 skinned rifle: profile 25 / family 9; five-draw slot map (bullet rides the clip); charging handle and bolt follow the pull along the captured path; Jason's captured hand pulls it.");
}
// PKM: box in by hand, then the game's own reload takes both hands; the lid / belt rounds stay with the box.
void TestPkm()
{
 const LONG oldProfile=g_weaponMechanicProfile;const auto s18=g_magazineStates[18];const auto oldK=g_pkm;
 g_weaponMechanicProfile=43;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_pistolMenu.open=false;
 Check(AkModel()&&PkmModel()&&CurrentRifle().skinned&&MagazineFamilyOfProfile(43)==18&&!CurrentRifle().emptyNeedsCharge&&wcscmp(MagazineSettingsSection(),L"PkmReload")==0,"PKM = skinned rifle, family 18, nothing to pull, own settings");
 Mp5Slots s={};Check(Mp5DrawSlots(14397,23700,s)&&s.anchor==0&&s.clip==7&&Mp5DrawSlots(10641,1020,s)&&s.clip==0&&Mp5DrawSlots(38397,3528,s)&&s.clip<0&&s.anchor<0&&Mp5DrawSlots(38097,300,s)&&s.anchor<0&&s.clip<0,"PKM slot map (+ the cover label 38097/300 rides the gun frame): box = slot 7 of the main draw and slot 0 of the box draws; the belt draw takes the gun frame of the anchored draws");
 auto& K=g_pkm;K=Pkm{};
 const Mat4 frame=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});float bones[48*12];
 // idle: box at rest -> the lid relation is learned
 auto boxRest=[&](){return Multiply(frame,Multiply(kPkmClipRest,Mp5BindInverse(kPkmClipRest)));};
 Mat4 lidIdle=HandIdentity();lidIdle.m[3]=.01;lidIdle.m[7]=.2;lidIdle.m[11]=.05;
 for(int b=0;b<48;++b)StoreBone(bones,b,frame);StoreBone(bones,7,boxRest());StoreBone(bones,8,Multiply(frame,lidIdle));
 PkmPostSlots(bones,14397,23700,frame);Check(K.lidValid&&!K.boxMoved,"idle: lid relation learned, box seated");
 // the game's reload, box phase: the native lid swings open, the mod keeps it shut on the seated box
 K.native=true;K.nativeAt=GetTickCount64()-500;Mat4 lidOpen=lidIdle;lidOpen.m[7]+=.1;
 StoreBone(bones,7,boxRest());StoreBone(bones,8,Multiply(frame,lidOpen));PkmPostSlots(bones,14397,23700,frame);
 {const Mat4 l=BoneMatrix(bones,8),e=Multiply(frame,lidIdle);double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(l.m[i]-e.m[i]));Check(d<1e-6,"game's reload, first 1.9 s: the lid stays shut on the box the player seated");}
 Check(PkmBoxPhase()&&!PkmArmsNative(),"first 1.9 s: the arms stay the player's");
 K.nativeAt=GetTickCount64()-2500;Check(!PkmBoxPhase()&&PkmArmsNative(),"after the box is settled: both arms follow the game's animation");
 StoreBone(bones,8,Multiply(frame,lidOpen));PkmPostSlots(bones,14397,23700,frame);{const Mat4 l=BoneMatrix(bones,8);Check(std::fabs(l.m[7]-Multiply(frame,lidOpen).m[7])<1e-5,"later in the reload the lid moves as the game animates it (belt, float palette)");}
 K=Pkm{};
 // box taken out by hand: the lid and the belt rounds ride it
 for(int b=0;b<48;++b)StoreBone(bones,b,frame);StoreBone(bones,7,boxRest());StoreBone(bones,8,Multiply(frame,lidIdle));PkmPostSlots(bones,14397,23700,frame);
 Mat4 lift=HandIdentity();lift.m[11]=-.3;const Mat4 boxOut=Multiply(lift,boxRest());StoreBone(bones,7,boxOut);PkmPostSlots(bones,14397,23700,frame);
 {const Mat4 l=BoneMatrix(bones,8),e=Multiply(lift,Multiply(frame,lidIdle));double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(l.m[i]-e.m[i]));Check(K.boxMoved&&d<1e-6,"box out by hand: the lid comes with it");}
 float belt[48*12];
 {K.boxMoved=false;K.boxHidden=false;K.boxDelta=HandIdentity();for(int b=0;b<48;++b)StoreBone(belt,b,frame);PkmPostSlots(belt,38397,3528,frame);Check(K.beltValid,"belt rounds' place on the gun learned at rest");
  StoreBone(bones,7,boxOut);PkmPostSlots(bones,14397,23700,frame);}
 for(int b=0;b<48;++b)StoreBone(belt,b,frame);PkmPostSlots(belt,38397,3528,frame);
 {const Mat4 r=BoneMatrix(belt,5),e=Multiply(lift,frame);double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(r.m[i]-e.m[i]));Check(d<1e-6,"box out by hand: the belt rounds come with it");}
 bones[7*12+7]=-100;PkmPostSlots(bones,14397,23700,frame);Check(bones[8*12+7]==-100,"box hidden: lid hidden too");
 for(int b=0;b<48;++b)StoreBone(belt,b,frame);PkmPostSlots(belt,38397,3528,frame);Check(belt[3*12+7]==-100,"box hidden: belt rounds hidden too");
 g_pkm=oldK;g_magazineStates[18]=s18;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;
 std::puts("PASS: PKM: skinned, family 18; box by hand then the game's own reload - first 1.9 s the box / lid stay seated and the arms stay the player's, then both arms follow the game; lid + belt rounds ride the box when it is out.");
}
void TestVector()
{
 const LONG oldProfile=g_weaponMechanicProfile;const auto s11=g_magazineStates[11];
 g_weaponMechanicProfile=21;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_pistolMenu.open=false;
 auto closeMat=[](const Mat4& a,const Mat4& b,double tol){for(int i=0;i<12;++i)if(std::fabs(a.m[i]-b.m[i])>tol)return false;return true;};
 Check(AkModel()&&CurrentRifle().skinned&&CurrentRifle().profile==21&&MagazineFamilyOfProfile(21)==11&&CurrentRifle().emptyNeedsCharge&&wcscmp(MagazineSettingsSection(),L"VectorReload")==0,"Vector = skinned rifle, family 11, charging handle after an EMPTY reload, own settings section");
 Mp5Slots s={};
 Check(Mp5DrawSlots(0,3615,s)&&s.anchor==0&&s.clip==1&&Mp5DrawSlots(3615,10623,s)&&s.anchor==0&&s.handle==4&&s.bolt==3&&Mp5DrawSlots(14238,3906,s)&&s.clip<0&&Mp5DrawSlots(18144,5091,s)&&!Mp5DrawSlots(11028,13881,s),"Vector slot map (CLIP, SLIDE_HANDLE, SLIDE, ANCHOR in every draw)");
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;p.rootBody=HandIdentity();p.rootWorld=HandIdentity();p.rootFrame=g_frame;
 const Mat4 frame=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});
 float bones[48*12];for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Mp5DrawSlots(3615,10623,s);
 p.needsRack=p.rackMode=p.rackHeld=true;p.slidePull=.05;Mp5PlaceSlots(bones,s,frame,HandIdentity(),HandIdentity());
 Check(closeMat(BoneMatrix(bones,4),Multiply(frame,Multiply(kVectorHandleBack,Mp5BindInverse(kVectorHandleRest))),1e-6),"full pull: SLIDE_HANDLE at its captured back pose (85 degree swing)");
 Check(closeMat(BoneMatrix(bones,3),Multiply(frame,Multiply(kVectorBoltBack,Mp5BindInverse(kVectorBoltRest))),1e-6),"the SLIDE (bolt) goes 4.3 cm back with it");
 p.needsRack=p.rackMode=p.rackHeld=false;p.slidePull=0;p.magazinePresent=false;for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Mp5DrawSlots(0,3615,s);Mp5PlaceSlots(bones,s,frame,HandIdentity(),HandIdentity());
 Check(bones[1*12+7]==-100,"magazine out: the CLIP slot hides");
 {Mat4 h0={},h1={};float f0[204],f1[204];Check(RifleChargeClipSample(0,h0,f0)&&RifleChargeClipSample(1,h1,f1)&&!closeMat(h0,h1,1e-3),"Vector pull: Jason's own hand path");}
 {
  const auto guides=g_pistolGuides;const bool mode=fc3pad::GripOnlyAmmoEnabled();
  const bool two=g_twoHand;fc3pad::SetGripOnlyAmmo(true);
  g_pistolGuides.box=true;g_pistolGuides.handDiameter=.01f;
  g_pistolGuides.port[0]=0;g_pistolGuides.port[1]=0;g_pistolGuides.port[2]=-.2f;
  g_pistolGuides.slide[0]=0;g_pistolGuides.slide[1]=0;g_pistolGuides.slide[2]=.2f;
  for(int a=0;a<3;++a){g_pistolGuides.size[0][a]=.02f;g_pistolGuides.size[1][a]=.5f;g_pistolGuides.angles[0][a]=15.f;g_pistolGuides.angles[1][a]=-40.f;}
  p=PistolReloadState{};p.enabled=p.basis=p.magazinePresent=true;
  p.rootBody=p.palmBody=HandIdentity();p.rootFrame=p.palmFrame=g_frame;p.rootEpoch=g_armPoseEpoch;
  fc3pad::Controllers control={};control.active=true;control.leftGrip=1;
  p.tip={0,0,.2};g_twoHand=true;TickAkRemoval(control);
  Check(!p.removalHeld&&p.magazinePresent&&g_twoHand,"KSV fresh support grip in old oversized removal box cannot steal seated magazine");
  Check(closeMat(PistolGuideBox(p.rootBody,0),PistolGuideBox(p.rootBody,1),1e-9),"KSV insertion and removal use identical centers and rotations");
  for(int x=-5;x<=5;++x)for(int y=-5;y<=5;++y)for(int z=-5;z<=5;++z){
   V3 at={x*.01,y*.01,-.2+z*.01};Check(PistolGuideContact(p.rootBody,0,at)==PistolGuideContact(p.rootBody,1,at),"KSV insertion/removal contact volumes identical including palm radius");
  }
  control.leftGrip=0;TickAkRemoval(control);p.tip={0,0,-.2};control.leftGrip=1;TickAkRemoval(control);
  Check(p.removalHeld&&p.magazinePresent&&!g_twoHand,"KSV grip at insertion target retains intentional manual magazine grab");
  p.tip.z-=.06;TickAkRemoval(control);Check(p.removalDetached&&!p.magazinePresent,"KSV existing pull threshold detaches magazine");
  control.leftGrip=0;TickAkRemoval(control);Check(!p.removalHeld&&p.dropActive,"KSV release still drops physically removed magazine");
  fc3pad::SetGripOnlyAmmo(mode);g_pistolGuides=guides;g_twoHand=two;
  puts("PASS: KSV support/removal conflict, exact calibrated insertion/removal geometry and manual pull/drop");
 }
 p=PistolReloadState{};p.enabled=true;g_magazineStates[11]=s11;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;
 std::puts("PASS: KSV Vector skinned rifle: profile 21 / family 11; four-draw slot map; SLIDE_HANDLE swing and SLIDE travel follow the pull along the captured path; Jason's captured hand pulls it.");
}
// R700 rules: a shot locks the trigger. PHYSICAL: LEFT grip = gun pinned to the left hand, RIGHT grip in the BOLT box, the
// real right palm drives the stroke (lift, back, home) = chambered. SIMPLE: RIGHT grip plays Jason's cycle, left hand on the forend.
// Recorded (Z93, 2026-10-09): the gun flipped upside down for ~6 frames at every hand-over (written world det -1 in 24 frames):
// the hand-over blend un-mirrored mirrored gun roots. A blend must keep its inputs' handedness.
void TestRigidBlendHandedness()
{
 Mat4 mir=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});for(int r=0;r<3;++r)mir.m[r*4+2]=-mir.m[r*4+2]; // mirrored (det -1)
 const Mat4 turn=TurnAndMove({0,1,0},{std::sin(.3),std::cos(.3),0},{0,0,0},{0,0,0});const Mat4 mir2=Multiply(turn,mir);
 auto det=[](const Mat4& m){return Dot(Cross({m.m[0],m.m[4],m.m[8]},{m.m[1],m.m[5],m.m[9]}),{m.m[2],m.m[6],m.m[10]});};
 Check(det(mir)<0&&det(mir2)<0,"fixture: both mirrored");
 for(double t:{.1,.5,.9}){const Mat4 b=PistolRigidBlend(mir,mir2,t);Check(det(b)<0,"blending two mirrored roots stays mirrored (no upside-down flip mid hand-over)");
  const V3 za={mir.m[2],mir.m[6],mir.m[10]},zb={b.m[2],b.m[6],b.m[10]};Check(Dot(za,zb)>.9,"the blended up axis stays near the inputs' (no 180 degree roll)");}
 {const Mat4 b=PistolRigidBlend(mir,mir,.5);double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(b.m[i]-mir.m[i]));Check(d<1e-9,"blending a mirrored root with itself returns it unchanged");}
 const Mat4 r1=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1}),r2=Multiply(turn,r1);Check(det(PistolRigidBlend(r1,r2,.5))>0,"normal roots stay normal");
 std::puts("PASS: rigid blend keeps handedness: mirrored gun roots (R700 / Z93 hand-over) never flip mid-blend; normal ones unchanged.");
}
void TestR700()
{
 auto& R=g_r700;R=R700{};g_weaponMechanicProfile=2;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_pistolMenu.open=false;g_twoHand=false;
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;p.rootBody=HandIdentity();p.rootWorld=HandIdentity();p.rootFrame=g_frame;
 float oldBody[2][16];memcpy(oldBody,g_handBody,sizeof(oldBody));const double oldScale=g_worldScale;g_worldScale=1;
 for(int s=0;s<2;++s){const Mat4 i=HandIdentity();for(int k=0;k<16;++k)g_handBody[s][k]=static_cast<float>(i.m[k]);}
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 Check(R700Model()&&R700Active()&&KnownNonPumpModel()&&R.chambered&&R.physical,"R700 equipped: chambered, physical bolt by default, its own rules (never the M133 pump)");
 { // two-hand grab point + guide: fixed on the forend, never the game's animated left wrist
  R.freeFromController=kR700HandFromFrame;R.freeEpoch=g_armPoseEpoch;
  V3 s0={},s1={};Check(R700SupportPosition(s0),"grab point available");const ArmJoints keepL=g_armJoints[1];g_armJoints[1].w[0]+=.3;g_armJoints[1].w[2]-=.2;
  Check(R700SupportPosition(s1)&&Length(Sub(s0,s1))<1e-9,"the game's left arm moving does not move the grab point");g_armJoints[1]=keepL;
  V3 gs={},gw={};bool in=false;Mat4 lw=HandIdentity();
  Check(R700TwoHandGuide(gs,gw,in),"guide shown one-handed");
  lw.m[3]=gs.x;lw.m[7]=gs.y;lw.m[11]=gs.z;for(int k=0;k<16;++k)g_handBody[1][k]=static_cast<float>(lw.m[k]);
  {const double* o=g_wristOffset[1];g_handBody[1][3]-=static_cast<float>(o[0]);g_handBody[1][7]-=static_cast<float>(o[1]);g_handBody[1][11]-=static_cast<float>(o[2]);}
  Check(R700TwoHandGuide(gs,gw,in)&&in,"left wrist on the grab point: the guide lights up");
  g_handBody[1][3]+=.3f;Check(R700TwoHandGuide(gs,gw,in)&&!in,"30 cm away: not lit");
  g_twoHand=true;Check(!R700TwoHandGuide(gs,gw,in),"guide hidden once two-handed");g_twoHand=false;
  for(int s=0;s<2;++s){const Mat4 i=HandIdentity();for(int k=0;k<16;++k)g_handBody[s][k]=static_cast<float>(i.m[k]);}}
 c.rightTrigger=1;fc3pad::Publish(c);UpdateR700Fire(c);Check(!R.chambered&&R.shots==1,"a shot locks the trigger");
 c.rightTrigger=0;fc3pad::Publish(c);UpdateR700Fire(c);c.rightTrigger=1;fc3pad::Publish(c);UpdateR700Fire(c);Check(R.shots==1&&R.refused==1,"second pull refused until the bolt is cycled");
 c.rightTrigger=0;fc3pad::Publish(c);UpdateR700Fire(c);
 // physical: two-handed + LEFT grip -> the gun is pinned to the left hand exactly where it is
 Mat4 gun=TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{.3,.2,.1});p.rootBody=gun;p.rootFrame=g_frame;g_twoHand=true;
 const Mat4 leftWrist=TurnAndMove({1,0,0},{.9,.1,.4},{0,0,0},{.1,.4,.2});for(int k=0;k<16;++k)g_handBody[1][k]=static_cast<float>(leftWrist.m[k]);
 c.leftGrip=1;fc3pad::Publish(c);TickR700(true);Check(R.hold&&R700LeftHold()&&R.holds==1,"two-handed while the bolt is due: gun into the left hand (level, not a grip edge)");
 {const Mat4 b=R700HoldBody();double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(b.m[i]-gun.m[i]));Check(d<1e-9,"pinned where it was: no jump at the switch");
  Check(std::fabs(AkParentBody().m[3]-gun.m[3])<1e-9,"every rigid draw re-roots from the left hand");}
 {Mat4 twisted=TurnAndMove({1,0,0},{0,1,0},{leftWrist.m[3],leftWrist.m[7],leftWrist.m[11]},{leftWrist.m[3],leftWrist.m[7],leftWrist.m[11]});twisted=Multiply(twisted,leftWrist);
  for(int k=0;k<16;++k)g_handBody[1][k]=static_cast<float>(twisted.m[k]);const Mat4 tb=R700HoldBody();double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(tb.m[i]-gun.m[i]));
  Check(d<1e-5,"twisting the left wrist 90 degrees does not swing the held gun (the aim stays)");}
 {Mat4 moved=leftWrist;moved.m[3]+=.2;for(int k=0;k<16;++k)g_handBody[1][k]=static_cast<float>(moved.m[k]);const Mat4 b=R700HoldBody();
  Check(std::fabs(b.m[3]-(gun.m[3]+.2))<1e-5,"the gun follows the left hand's position");p.rootBody=b;}
 Check(R700BoltBoxDue()&&PistolGameplayGuideTarget()==3,"the BOLT box is shown while the bolt is due");
 // RIGHT grip inside the BOLT box (palm at the handle rest), then the real palm along the captured path
 auto setPalm=[&](const V3& local){const V3 w=Apply(p.rootBody,local);Mat4 wrist=HandIdentity();const float* o=R.rightPalm;wrist.m[3]=w.x-.591-o[0];wrist.m[7]=w.y-.011-o[1];wrist.m[11]=w.z-1.149-o[2];R.rightHandBody=wrist;R.rightHandFrame=g_frame;}; // the DRAWN right wrist (body space)
 for(int i=0;i<3;++i){g_pistolGuides.charge[i]=PistolGuideDefault(30+i);g_pistolGuides.size[2][i]=PistolGuideDefault(33+i);} // the family-14 BOLT box defaults
 Check(PistolGuideDefault(33)>.1f&&std::fabs(g_pistolGuides.charge[1]-.2388f)<1e-4f,"R700 BOLT box defaults: where Jason's palm is on the handle");
 const auto& P=R700Path();
 const V3 boxCentre={g_pistolGuides.charge[0],g_pistolGuides.charge[1],g_pistolGuides.charge[2]};
 {V3 off=boxCentre;off.x+=.06;setPalm(off);c.rightGrip=1;fc3pad::Publish(c);TickR700(true);Check(R.grabbing&&R.grabs==1,"RIGHT grip 6 cm from the handle: grabbed (10 cm hand sphere)");c.rightGrip=0;fc3pad::Publish(c);TickR700(true);}
 setPalm(boxCentre);c.rightGrip=1;fc3pad::Publish(c);TickR700(true);Check(R.grabbing&&R.grabs==2,"RIGHT grip at the handle: grabbed");
 // pull 6 cm back from the grab = fully back (further changes nothing), push 5 cm forward from the furthest point = home
 auto moveController=[&](double dy){const V3 w=Apply(p.rootBody,{0,dy,0});const V3 o=Apply(p.rootBody,{0,0,0});Mat4 ctl=R.grabController;ctl.m[3]+=w.x-o.x;ctl.m[7]+=w.y-o.y;ctl.m[11]+=w.z-o.z;for(int k=0;k<16;++k)g_handBody[0][k]=static_cast<float>(ctl.m[k]);};
 moveController(-.03);TickR700(true);Check(std::fabs(R.s-.5)<.02,"3 cm back: half way");
 {Mat4 bolt={};Check(R700BoltPose(0,kR700PartRest0,bolt),"bolt pose follows the stroke");
  Mat4 mv={};float bones[48*12];for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Check(R700RightClipMove(HandIdentity(),bones,mv),"Jason's right hand rides the handle");}
 moveController(-.065);TickR700(true);Check(R.s>=1&&R.reachedBack,"6 cm back: fully back");
 {Mat4 bolt={};Check(R700BoltPose(0,kR700PartRest0,bolt)&&bolt.m[7]<kR700PartRest0.m[7]-.06,"bolt 7 cm back");}
 moveController(-.20);TickR700(true);Check(R.s>=1,"an over-long pull (20 cm, as logged) keeps it fully back");
 moveController(-.17);TickR700(true);Check(!R.chambered&&R.s<1&&R.s>0,"3 cm forward from the furthest point: closing");
 moveController(-.145);TickR700(true);Check(R.chambered&&R.cycles==1&&R.s==0&&!R.hold&&!R.grabbing,"5 cm forward from the furthest point: home, chambered, gun back in the right hand at once");
 Check(g_twoHand,"LEFT grip still on the forend: stays two-handed (no one-handed snap of the aim)");
 R.freeFromController=kR700HandFromFrame;R.freeEpoch=g_armPoseEpoch; // set by the arm draw in the game
 {Mat4 r0={};Check(R700ControllerRoot(r0),"root after the hold");double d=0;const Mat4 was=R.releaseFrom;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(r0.m[i]-was.m[i]));
  Check(d<1e-3,"the handover starts exactly where the gun was in the left hand (blended, no jump)");
  R.releaseAt=GetTickCount64()-1000;Mat4 r1={};R700ControllerRoot(r1);R.releaseAt=0;Mat4 r2={};R700ControllerRoot(r2);double e=0;for(int i=0;i<12;++i)e=(std::max)(e,std::fabs(r1.m[i]-r2.m[i]));
  Check(e<1e-9,"after 150 ms the root is the plain controller root");}
 {V3 rp={};Check(R700RightPalmBody(rp),"the right palm sphere is the drawn hand + the palm offset");}
 c.rightGrip=0;fc3pad::Publish(c);TickR700(true);
 // a re-grab at the back (log, shot 5: pulled, let go, grabbed again): pushing 5 cm forward from the new grab closes it
 R.chambered=false;R700BeginHold();R.s=1;R.reachedBack=true;
 setPalm(boxCentre);c.rightGrip=1;fc3pad::Publish(c);TickR700(true);Check(R.grabbing&&R.reachedBack,"re-grab with the bolt back");
 moveController(.055);TickR700(true);Check(R.chambered&&!R.hold,"re-grab + 5.5 cm forward: home, back in the right hand"); // exactly 5.00 cm lands at 4.998 in float
 c.rightGrip=0;fc3pad::Publish(c);TickR700(true);
 c.leftGrip=0;fc3pad::Publish(c);TickR700(true);Check(!R.hold,"LEFT grip released: gun in the right hand");
 {Mat4 bolt={};Mat4 moved=kR700PartRest0;moved.m[7]-=.07;Check(R700BoltPose(0,moved,bolt)&&std::fabs(bolt.m[7]-kR700PartRest0.m[7])<1e-9,"the game's automatic bolt motion after a shot is held at rest");}
 c.rightTrigger=1;fc3pad::Publish(c);UpdateR700Fire(c);Check(!R.chambered&&R.shots==2,"fires again after the cycle");c.rightTrigger=0;fc3pad::Publish(c);UpdateR700Fire(c);
 // Simple: RIGHT grip plays the cycle; the left hand snaps to the forend meanwhile
 R.physical=false;g_twoHand=false;R.chambered=false;
 c.rightGrip=1;fc3pad::Publish(c);g_weaponMechanicFrame=++g_frame;p.rootFrame=g_frame;TickR700(true);Check(R.clip==0&&R700ClipPlaying()&&!R.hold,"Simple: RIGHT grip starts Jason's bolt cycle, gun stays in the right hand");
 {Mat4 mv={};float bones[48*12];for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());Check(R700LeftSupportActive()&&R700LeftSupportMove(HandIdentity(),bones,mv)&&R700LeftGrip(bones),"Simple: the left hand is on the forend during the cycle");}
 c.rightGrip=0;fc3pad::Publish(c);R.clipAt-=static_cast<ULONGLONG>(kR700BoltCycleSeconds*1000)+100;g_weaponMechanicFrame=++g_frame;TickR700(true);Check(R.clip<0&&R.chambered&&R.cycles==3,"Simple: cycle done, chambered"); // 2 physical cycles above (incl. the re-grab) + this one
 // the game's own EMPTY reload: fire locked from Begin to End, bolt native
 R.native=true;R.nativeAt=GetTickCount64();R.chambered=false;
 {Mat4 bolt={};Mat4 moved=kR700PartRest0;moved.m[7]-=.07;Check(R700BoltPose(0,moved,bolt)&&std::fabs(bolt.m[7]-moved.m[7])<1e-9,"native reload: the bolt moves as the game animates it");}
 c.rightTrigger=1;fc3pad::Publish(c);UpdateR700Fire(c);Check(R.shots==2,"no shot while the game's reload plays");c.rightTrigger=0;fc3pad::Publish(c);UpdateR700Fire(c);
 R.nativeEndAt=GetTickCount64()-200;R.chambered=true;UpdateR700Fire(c);Check(!R.native&&R.chambered,"native reload end: chambered, fire free");
 R=R700{};p=PistolReloadState{};p.enabled=true;memcpy(g_handBody,oldBody,sizeof(oldBody));g_worldScale=oldScale;g_twoHand=false;
 std::puts("PASS: R700 bolt-action: a shot locks the trigger; PHYSICAL: LEFT grip pins the gun to the left hand with no jump, RIGHT grip in the BOLT box, the real palm drives the captured lift/back/home stroke (bolt + Jason's hand on the handle), home = chambered; SIMPLE: RIGHT grip plays the cycle with the left hand on the forend; the game's automatic cycle is held at rest; EMPTY reload = the game's own animation.");
}
void TestRifles(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const LONG oldProfile=g_weaponMechanicProfile;const auto s3=g_magazineStates[3],s4=g_magazineStates[4];
 TestRifleGpu(device,context,7,kA2000FrameGlobals,kA2000ClipGlobals,kA2000Part0Globals,kA2000Camera);
 std::puts("PASS: A2000 actual GPU: 3 FRAME materials share one root, clip and knob at captured rests, knob rides native -Y travel, native charging fingers, barrel aim.");
 TestRifleGpu(device,context,41,kStg90FrameGlobals,kStg90ClipGlobals,kStg90Part0Globals,kStg90Camera);
 std::puts("PASS: STG-90 actual GPU: 5 FRAME materials share one root, clip and handle at captured rests, barrel aim; holographic sight stays mounted on the corrected gun while a magazine is held.");
 {const auto s6=g_magazineStates[6];
  TestRifleGpu(device,context,14,kFamasFrameGlobals,kFamasClipGlobals,kFamasPart0Globals,kFamasCamera);
  g_weaponMechanicProfile=14;g_weaponMechanicFrame=++g_frame;
  Check(RifleClip(CurrentRifle(),15180,135)&&RifleClip(CurrentRifle(),15315,618)&&!RifleClip(CurrentRifle(),15933,582),"F1: both magazine draws are the magazine");
  Check(RiflePartIndex(15933,582)==0&&RiflePartIndex(18669,1020)==4&&CurrentRifle().emptyNeedsCharge&&std::fabs(CurrentRifle().chargeTravel-.1299)<.001,"F1: middle charging handle = part 0, 13 cm native travel, pull needed after an EMPTY reload");
  Check(CurrentRifle().clipRest->m[7]<-.1&&MagazineFamilyOfProfile(14)==6&&wcscmp(MagazineSettingsSection(),L"FamasReload")==0,"F1: bullpup magazine behind the grip; own settings section");
  g_magazineStates[6]=s6;}
 std::puts("PASS: FAMAS F1 actual GPU (PID99068 idle f8263): 4 FRAME materials share one root, magazine and middle charging handle at captured rests, handle rides FRAME -Y on the pull with the native charging fingers, barrel aim.");
 {const auto s7=g_magazineStates[7],s8=g_magazineStates[8];
  TestRifleGpu(device,context,9,kBz19FrameGlobals,kBz19ClipGlobals,kBz19Part0Globals,kBz19Camera);
  std::puts("PASS: BZ19 actual GPU (PID102884 idle f12147): 4 FRAME draws one root, magazine and left-side charging handle at captured rests, handle rides FRAME -Y 8.1 cm with Jason's charging fingers, barrel aim.");
  TestRifleGpu(device,context,31,kP416FrameGlobals,kP416ClipGlobals,kP416Part0Globals,kP416Camera);
  g_weaponMechanicProfile=31;g_weaponMechanicFrame=++g_frame;
  Check(RifleClip(CurrentRifle(),26529,1248)&&RifleClip(CurrentRifle(),27777,546)&&RiflePartIndex(28524,936)==0&&RiflePartIndex(31908,870)==4&&CurrentRifle().riderBack,"P416: two magazine draws, top handle part 0, latch rides it, three static parts");
  g_magazineStates[7]=s7;g_magazineStates[8]=s8;}
 std::puts("PASS: P416 actual GPU (PID102884 idle f14841): 5 FRAME draws one root, magazine and top charging handle at captured rests, handle rides FRAME -Y 5.7 cm with Jason's charging fingers, barrel aim.");
 {const auto s12=g_magazineStates[12],s13=g_magazineStates[13];
  TestRifleGpu(device,context,17,kAceFrameGlobals,kAceClipGlobals,kAcePart0Globals,kAceCamera);
  g_weaponMechanicProfile=17;g_weaponMechanicFrame=++g_frame;
  Check(RifleClip(CurrentRifle(),15219,672)&&RiflePartIndex(15891,906)==0&&RiflePartIndex(19137,276)==1&&RiflePartIndex(16797,2340)==2&&!CurrentRifle().riderBack&&std::fabs(CurrentRifle().chargeTravel-.066)<.002,"ACE: one magazine draw, top handle part 0 (6.6 cm), catch part 1, static part 2");
  std::puts("PASS: Galil ACE52 actual GPU (PID95632 idle): 3 FRAME draws one root, magazine and top charging handle at captured rests, handle rides FRAME -Y 6.6 cm with Jason's charging fingers, barrel aim.");
  TestRifleGpu(device,context,35,kSvdFrameGlobals,kSvdClipGlobals,kSvdPart0Globals,kSvdCamera);
  g_weaponMechanicProfile=35;g_weaponMechanicFrame=++g_frame;
  Check(CurrentRifle().offset==10160&&RifleClip(CurrentRifle(),25068,1794)&&RiflePartIndex(26862,243)==0&&RiflePartIndex(29031,732)==2&&std::fabs(CurrentRifle().chargeTravel-.13)<.002,"SVD: vertex offset 10160, one magazine draw, bolt handle part 0 (13 cm), two static parts");
  {fc3weapon::Geometry d={268464,59526,32,0,1968,978,57};Check(!RifleMain(CurrentRifle(),d),"SVD: an offset-0 draw of the same buffers is not the SVD");}
  std::puts("PASS: SVD actual GPU (PID95632 idle f5472): 5 FRAME draws (vertex offset 10160) one root, magazine and left bolt handle at captured rests, handle rides FRAME -Y 13 cm with Jason's charging fingers, barrel aim.");
  g_magazineStates[12]=s12;g_magazineStates[13]=s13;}
 TestRigidBlendHandedness();
 TestVector();
 {const auto s15=g_magazineStates[15],s16=g_magazineStates[16];
  TestRifleGpu(device,context,36,kU100FrameGlobals,kU100ClipGlobals,kU100Part0Globals,kU100Camera);
  g_weaponMechanicProfile=36;g_weaponMechanicFrame=++g_frame;
  Check(MagazineFamilyOfProfile(36)==15&&RifleClip(CurrentRifle(),14706,2658)&&RiflePartIndex(17364,372)==0&&RiflePartIndex(17736,972)==1&&RiflePartIndex(18708,396)==5&&std::fabs(CurrentRifle().chargeTravel-.1197)<.002&&CurrentRifle().emptyNeedsCharge,"U100: family 15, one magazine draw, top handle part 0 (12 cm), the two static draws in their own slots (part 1 / part 5)");
  std::puts("PASS: U100 actual GPU (PID115332 idle): 2 FRAME draws one root, magazine and top charging handle at captured rests, handle rides FRAME -Y 12 cm with Jason's charging fingers, barrel aim.");
  TestRifleGpu(device,context,22,kMkgFrameGlobals,kMkgClipGlobals,kMkgPart0Globals,kMkgCamera);
  g_weaponMechanicProfile=22;g_weaponMechanicFrame=++g_frame;
  Check(MagazineFamilyOfProfile(22)==16&&RifleClip(CurrentRifle(),16182,906)&&RifleClip(CurrentRifle(),17088,462)&&RifleClip(CurrentRifle(),17550,1200)&&RifleClip(CurrentRifle(),28518,1218)&&RiflePartIndex(13140,702)==0&&RiflePartIndex(27990,528)==1&&RiflePartIndex(13842,240)==5&&std::fabs(CurrentRifle().chargeTravel-.14)<.002,"MKG: family 16, the four box draws, handle part 0 (14 cm), cover part 1, sixth static part");
  std::puts("PASS: MKG actual GPU (PID115332 idle f8374): 6 FRAME draws one root, box (4 draws incl. the belt) and top charging handle at captured rests, handle rides FRAME -Y 14 cm with Jason's charging fingers, barrel aim.");
  {fc3weapon::Geometry g={636120,83850,40,0,14397,23700,57};Check(fc3weapon::ProfileIndex(g)==43&&strcmp(fc3weapon::kProfileNames[43],"pkm.xbg")==0,"PKM main draw = profile 43 (motion controls recognise it)");}
  TestPkm();
  g_magazineStates[15]=s15;g_magazineStates[16]=s16;}
 {const auto s17=g_magazineStates[17];const auto oldR=g_r700;g_r700=R700{};
  g_r700.freeFromController=kZ93HandFromFrame;g_r700.freeEpoch=g_armPoseEpoch;g_weaponMechanicProfile=23;
  TestRifleGpu(device,context,23,kZ93FrameGlobals,kZ93ClipGlobals,kZ93Part0Globals,kZ93Camera);
  g_weaponMechanicProfile=23;g_weaponMechanicFrame=++g_frame;
  Check(R700Model()&&Bolt().profile==23&&MagazineFamilyOfProfile(23)==17&&CurrentRifle().offset==10760&&RiflePartIndex(37260,3300)==0&&RiflePartIndex(40560,207)==5&&RifleClip(CurrentRifle(),26808,732),"Z93: bolt-action family, family 17, vertex offset 10760, bolt part 0, sixth static part, follower = clipD");
  Check(BoltAfterEveryReload(),"Z93: the bolt is due after EVERY reload");
  {Mat4 r={};Check(R700BoltPose(2,kZ93PartRest0,r)&&std::fabs(r.m[3]-kZ93PartRest2.m[3])<1e-9,"Z93: the swinging part is pinned at rest (no shot kick)");
   Mat4 b={},p1={};g_r700.physical=true;g_r700.s=.8;g_r700.poseFrame=-100000;Check(R700BoltPose(0,kZ93PartRest0,b)&&R700BoltPose(1,kZ93PartRest1,p1),"Z93: bolt + its second piece follow the stroke");
   Mat4 bi={};Invert(b,bi);const Mat4 rel=Multiply(bi,p1);Mat4 ri={};Invert(kZ93PartRest0,ri);const Mat4 relRest=Multiply(ri,kZ93PartRest1);double d=0;for(int i=0;i<12;++i)d=(std::max)(d,std::fabs(rel.m[i]-relRest.m[i]));
   Check(d<1e-6,"Z93: the second bolt piece stays rigid on the bolt");g_r700.s=0;}
  g_weaponMechanicProfile=2;Check(!BoltAfterEveryReload()&&Bolt().profile==2,"R700 keeps its own rule (bolt only after an empty reload)");
  g_r700=oldR;g_magazineStates[17]=s17;
  std::puts("PASS: Z93 actual GPU (PID96724 idle): 6 FRAME draws one root (vertex offset 10760), magazine and bolt at captured rests, barrel aim; R700 bolt family shared: bolt after every reload, two-piece bolt, swinging parts pinned.");}

 {const auto s14=g_magazineStates[14];const auto oldR=g_r700;g_r700=R700{};
  TestRifleGpu(device,context,2,kR700FrameGlobals,kR700ClipGlobals,kR700Part0Globals,kR700Camera);
  g_weaponMechanicProfile=2;g_weaponMechanicFrame=++g_frame;
  Check(CurrentRifle().offset==9560&&RifleClip(CurrentRifle(),1434,966)&&RiflePartIndex(28971,1386)==0&&RiflePartIndex(30363,732)==1&&!CurrentRifle().emptyNeedsCharge&&MagazineFamilyOfProfile(2)==14&&wcscmp(MagazineSettingsSection(),L"R700Reload")==0,"R700: vertex offset 9560, block = magazine, bolt part 0, slider part 1, nothing to pull on the left, own settings section");
  TestR700();
  g_r700=oldR;g_magazineStates[14]=s14;}
 TestMp5();
 TestMk16();
 g_weaponMechanicProfile=8;g_weaponMechanicFrame=++g_frame;Check(&CurrentRifle()==&kRifles[0]&&AkFrame(1056,17421)&&!AkFrame(10782,13122),"AK keeps its exact table");
 std::puts("PASS: AK table unchanged by the rifle generalization.");
 g_magazineStates[3]=s3;g_magazineStates[4]=s4;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;fc3pad::SetMagazineHandling(false);
}
