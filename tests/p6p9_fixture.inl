#include "../src/p6p9_data.inl"
#include "p6p9_live_data.inl"
// 6P9 = 1911 skeleton with a different mesh. Proves the reuse with captured PID56072 data and
// drives every mapped 6P9 draw through the real BuildWeaponWorld GPU path.
void TestP6p9(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const auto oldSlot=g_magazineStates[0];const LONG oldProfile=g_weaponMechanicProfile;
 auto closeMat=[](const Mat4& a,const Mat4& b,double pos,double rot){double r=0,t=0;for(int i=0;i<3;++i){t+=(a.m[i*4+3]-b.m[i*4+3])*(a.m[i*4+3]-b.m[i*4+3]);for(int j=0;j<3;++j)r=(std::max)(r,std::fabs(a.m[i*4+j]-b.m[i*4+j]));}return std::sqrt(t)<=pos&&r<=rot;};
 Check(closeMat(kP6p9ClipRest,kPistolClipRest,.0005,.0005)&&closeMat(kP6p9SlideRest,kPistolSlideRest,.0005,.0005),"captured 6P9 clip/slide rests equal accepted 1911 rests");
 Check(closeMat(kP6p9HandFromFrame,kPistolHandFromFrame,.01,.002),"captured 6P9 right hand-from-FRAME equals accepted 1911 within 1 cm");
 Check(closeMat(kP6p9MagazineFromHand,kPistolMagazineFromHand,.02,.08),"captured 6P9 magazine-in-hand within snapshot tolerance of 1911");
 g_weaponMechanicProfile=20;g_weaponMechanicFrame=++g_frame;
 Check(&CurrentPistolMesh()==&k1911Mesh&&PistolMeshFrame(294,4470)&&!PistolMeshFrame(4695,1263),"1911 keeps its exact part table");
 auto& slot=g_magazineStates[0];slot=PistolReloadState{};slot.enabled=true;TickPistolReload(false);slot.magazinePresent=false;
 g_weaponMechanicProfile=1;g_weaponMechanicFrame=++g_frame;
 Check(PistolModel()&&P6p9Model()&&&CurrentPistolMesh()==&k6p9Mesh,"6P9 is a recognized pistol with its own part table");
 TickPistolReload(false);Check(slot.magazinePresent&&slot.enabled,"1911 -> 6P9 switch starts the shared magazine slot fresh");
 std::puts("PASS: 6P9 captured rig equals the accepted 1911 (clip, slide, right hand, magazine hold); 1911 table unchanged; model switch resets shared slot.");

 g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
 auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=p.basis=true;p.magazinePresent=true;
 ID3D11Buffer* vb=Buffer(device,106448,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,20736,D3D11_BIND_INDEX_BUFFER);
 ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=240;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,kP6p9Camera,12);memcpy(cam.up,kP6p9Camera+3,12);memcpy(cam.forward,kP6p9Camera+6,12);memcpy(cam.position,kP6p9Camera+9,12);
 cam.xScale=kP6p9Camera[12];cam.yScale=kP6p9Camera[13];cam.zScale=kP6p9Camera[14];cam.zOffset=kP6p9Camera[15];cam.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
 ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 g_frameGunValid=true;g_frameGunMove=HandIdentity();
 // Live order: FRAME 4695 first, then bullet/slide/clip, then FRAME 3741/5958, hammer, tiny grip/slide.
 globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9FrameGlobals),kP6p9FrameGlobals);
 auto* output=BuildWeaponWorld(context,4695,1263);Check(output!=nullptr,"6P9 first FRAME draw GPU override");
 float frame[88]={};Read(device,context,output,frame);Mat4 root=HullWorld(frame),inverse={};Check(Invert(root,inverse)&&p.rootFrame==g_frame,"6P9 FRAME root learned");
 float part[88]={};
 globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9FrameGlobals),kP6p9FrameGlobals);output=BuildWeaponWorld(context,9828,540);Read(device,context,output,part);
 Check(part[7]==-10000.f,"6P9 loose bullet hidden like the accepted 1911");
 p.rackHeld=true;p.slidePull=.045;
 for(UINT start:{18u,2937u}){
  g_frameGunMove.m[3]+=.2;
  globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9SlideGlobals),kP6p9SlideGlobals);output=BuildWeaponWorld(context,start,start==18?2919:276);Read(device,context,output,part);
  const Mat4 rel=Multiply(inverse,HullWorld(part));
  for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-(kPistolSlideRest.m[i]-(i==7?.045:0)))<.001,"both 6P9 slide meshes ride the 1911 FRAME-local -Y rail");
 }
 p.rackHeld=false;p.slidePull=0;
 globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9ClipGlobals),kP6p9ClipGlobals);output=BuildWeaponWorld(context,3213,528);Read(device,context,output,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));for(int i=0;i<16;++i)Check(std::fabs(rel.m[i]-kPistolClipRest.m[i])<.001,"6P9 seated clip at 1911 clip rest on corrected FRAME");}
 for(UINT start:{3741u,5958u}){
  globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9FrameGlobals),kP6p9FrameGlobals);output=BuildWeaponWorld(context,start,start==3741?954:3468);Read(device,context,output,part);
  Compare(part,frame,12,"later 6P9 FRAME materials share the first root");
 }
 stride=20;offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
 globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9FrameGlobals),kP6p9FrameGlobals);output=BuildWeaponWorld(context,12,6);Read(device,context,output,part);
 Compare(part,frame,12,"6P9 tiny grip piece stays rigid on the corrected FRAME");
 p.rackHeld=true;p.slidePull=.03;
 globals->SetPrivateData(kCbCopyGuid,sizeof(kP6p9SlideGlobals),kP6p9SlideGlobals);output=BuildWeaponWorld(context,0,12);Read(device,context,output,part);
 {const Mat4 rel=Multiply(inverse,HullWorld(part));Check(std::fabs(rel.m[7]-(kPistolSlideRest.m[7]-.03))<.001&&std::fabs(rel.m[3]-kPistolSlideRest.m[3])<.001,"6P9 tiny slide piece follows the same rail");}
 p.rackHeld=false;p.slidePull=0;
 std::puts("PASS: 6P9 actual GPU FRAME/tiny grip share one corrected root; both slide meshes and tiny slide ride the 1911 rail; clip at 1911 rest; bullet hidden.");
 context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();
 g_magazineStates[0]=oldSlot;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;fc3pad::SetMagazineHandling(false);
}
