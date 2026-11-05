#include "../src/stereo.cpp"
#include <cstdlib>
// Exercise frozen old publishers only for unchanged pose/reload regression coverage.
#define Publish1911Transfer307 PublishLegacyTransfer332
void Check(bool ok,const char* what) { if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);} }
void STDMETHODCALLTYPE Update(ID3D11DeviceContext* c,ID3D11Resource* r,UINT s,const D3D11_BOX* box,const void* p,UINT rp,UINT dp)
{ c->UpdateSubresource(r,s,box,p,rp,dp); }
ID3D11Buffer* Buffer(ID3D11Device* d,UINT size,UINT bind)
{
    D3D11_BUFFER_DESC desc={};desc.ByteWidth=size;desc.BindFlags=bind;ID3D11Buffer* b=nullptr;
    Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&b)),"buffer creation");return b;
}
void Read(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Buffer* source,float* out)
{
    D3D11_BUFFER_DESC desc={};source->GetDesc(&desc);desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* stage=nullptr;Check(SUCCEEDED(d->CreateBuffer(&desc,nullptr,&stage)),"staging buffer");c->CopyResource(stage,source);
    D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(c->Map(stage,0,D3D11_MAP_READ,0,&m)),"read upload");std::memcpy(out,m.pData,desc.ByteWidth);c->Unmap(stage,0);stage->Release();
}
void Compare(const float* a,const float* b,int n,const char* what)
{for(int i=0;i<n;++i)Check(std::isfinite(a[i]) && std::fabs(a[i]-b[i])<2e-5f,what);}
extern void SetRenderHands303(int hand,bool empty,unsigned epoch);
extern void SetRenderReceiver309(int receiver);
extern void PublishReloadControls312(float,float,bool=false,bool=false);
#include "sidearms315_render_fixture.inl"
#include "ak316_render_fixture.inl"
#include "shotguns318_render_fixture.inl"
#include "others319_render_fixture.inl"
#include "revolver321_render_fixture.inl"
#include "revolver323_render_fixture.inl"
#include "revolver325_render_fixture.inl"
#include "hands326_render_fixture.inl"
#include "hands327_render_fixture.inl"
#include "locker331_render_fixture.inl"
#include "smgs332_render_fixture.inl"
#include "handle333_render_fixture.inl"
#include "a2000full337_render_fixture.inl"
#include "sidemirror338_render_fixture.inl"
#include "reliability345_render_fixture.inl"
#include "support349_render_fixture.inl"
#include "grab350_render_fixture.inl"
#include "car352_fixture.inl"
#include "car353_fixture.inl"
#include "car354_fixture.inl"
#include "handlebar355_fixture.inl"
#include "weapons356_fixture.inl"
#include "car_door357_fixture.inl"
#include "throwable_pouch361_fixture.inl"
#include "vehicle363_fixture.inl"
int main(){
 wchar_t path[MAX_PATH];GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d11.dll");auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(path),"D3D11CreateDevice"));
 ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP");
 g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_aimActive=false;g_neutralQuietGate=true;g_twoHand=false;g_machete.held=false;g_weaponMechanicProfile=20;g_frame=100;g_weaponMechanicFrame=g_frame;
 fc3aim::SetVehicleStateForTest(0);fc3aim::SetNativeActionForTest128(0);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 float physical[2][16]={};for(int h=0;h<2;++h){physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.3f:.3f;physical[h][7]=-.2f;physical[h][11]=.45f;memcpy(g_handRef[h],physical[h],64);g_handRef[h][3]=g_handRef[h][7]=g_handRef[h][11]=0;}
 physical[1][0]=physical[1][10]=0;physical[1][2]=1;physical[1][8]=-1;
 SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
 Check(g_handBody[0][3]<0&&g_handBody[0][2]==1&&g_handBody[1][3]>0,"left owner routes primary weapon pose and aim forward from physical left controller");
 const V3 primary=ControllerWrist(g_handRight,0);Check(primary.x<0,"left weapon wrist comes from left physical location");
 {
  PhysicalArmScope303 physicalScope;Check(physicalScope.active&&g_handBody[0][3]>.2&&g_handBody[1][3]<-.2,"native right/left meshes retain correct anatomical controllers");
 }
 Check(g_handBody[0][3]<-.2&&g_handBody[1][3]>.2,"arm draw restores weapon roles after anatomical palette work");
 auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
 ID3D11Buffer* cb[4]={Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER),nullptr,Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER)};
 float rows[88]={};rows[0]=rows[5]=rows[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);CameraSummary cam={};cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.mainView=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);context->VSSetConstantBuffers(0,4,cb);
 float original[576]={};for(int b=0;b<48;++b)StoreBone(original,b,HandIdentity());memcpy(original,kNeutralArmView[0],sizeof(kNeutralArmView[0]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
 for(int h=0;h<2;++h){g_armPaletteCb[h]=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);g_armJoints[h]={{h?-.2:.2,-.15,-.2},{h?-.35:.35,-.3,.05},{h?-.35:.35,-.2,.4},true};}
 if(GetEnvironmentVariableW(L"FC3_VEHICLE363_ONLY",nullptr,0)){TestSteeringMath363();TestVehicleContact363(device,context,cb);TestDoor363(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_DUAL362_ONLY",nullptr,0)){extern unsigned ExerciseDualAudit362();const unsigned proof=ExerciseDualAudit362();printf("362 native audit proof=%u\n",proof);Check(proof==127,"362 distinct native instance/loaded field observation, alias rejection, freed pointer rejection and no native memory writes");puts("PASS:362 read-only native dual-instance audit; no dual firing or fake second weapon enabled");return 0;}
 if(GetEnvironmentVariableW(L"FC3_POUCH361_ONLY",nullptr,0)){TestPouch361();return 0;}
 if(GetEnvironmentVariableW(L"FC3_DOOR357_ONLY",nullptr,0)){TestDoor357(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_CACHE356_ONLY",nullptr,0)){TestSupportCache356();return 0;}
 if(GetEnvironmentVariableW(L"FC3_M133356_ONLY",nullptr,0)){TestM133Shot356(device,context,cb,physical);return 0;}
 if(GetEnvironmentVariableW(L"FC3_RPG356_ONLY",nullptr,0)){TestRpgFit356(device,context,cb,physical);return 0;}
 if(GetEnvironmentVariableW(L"FC3_BARS355_ONLY",nullptr,0)){TestBarGpu355(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_CAR354_ONLY",nullptr,0)){TestNativeCarPose354(device,context,cb);TestDualWheel353();TestCarHandsDraw353(device,context,cb);TestCarGpu352(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_CAR353_ONLY",nullptr,0)){TestDualWheel353();TestCarHandsDraw353(device,context,cb);TestCarGpu352(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_CAR352_ONLY",nullptr,0)){TestCarGpu352(device,context,cb);return 0;}
 if(GetEnvironmentVariableW(L"FC3_RPG351_ONLY",nullptr,0)){g_oCreateBuffer=MirrorCreate337;g_frameGunValid=true;g_frameGunMove=HandIdentity();TestGrab350(device,context,cb,physical);return 0;}
 V3 actualLeftPalm303={};float fitted305[576]={};
 for(bool empty:{false,true}){
  SetRenderHands303(1,empty,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
  for(int side=0;side<2;++side){
   Check(BuildArmPalette(context,side)!=nullptr,"actual anatomical GPU arm palette builds with left ownership/empty hands");float output[576];Read(device,context,g_armPaletteCb[side],output);
   const V3 wrist=Apply(BoneMatrix(output,2),{side?-.591:.591,.011,1.149});const V3 expected=ControllerWrist(physical[side],side);if(side==1)actualLeftPalm303=Apply(BoneMatrix(output,2),{-.6781,.0519,1.0387});
   if(!empty&&side==1){memcpy(fitted305,output,sizeof(fitted305));Check(Length(Sub(wrist,expected))<.11,"305 holding wrist remains near the physical left controller while fitting the grip");}
   else Check(Length(Sub(wrist,expected))<.002,"uploaded free native hand stays at its own physical controller");

  }
 }
 fc3visual292::SetPreparing330(true);SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
 Check(fc3visual292::Hidden()&&fc3hands303::Read().empty&&!fc3hands303::ReadPresentation314().active,"330 preparation hides firearms without active or grace-held gun presentation");
 for(int side=0;side<2;++side){Check(BuildArmPalette(context,side)!=nullptr,"330 preparation builds physical empty-hand GPU palette");float output[576];Read(device,context,g_armPaletteCb[side],output);const auto wrist=Apply(BoneMatrix(output,2),{side?-.591:.591,.011,1.149});Check(Length(Sub(wrist,ControllerWrist(physical[side],side)))<.002,"330 preparation keeps each anatomical hand at its own controller");}
 fc3visual292::SetPreparing330(false);SetRenderHands303(1,false,g_armPoseEpoch);Check(!fc3visual292::Hidden()&&!fc3hands303::Read().empty,"330 ending preparation restores original left-owned presentation");
 puts("PASS:330 preparation publication has no gun/grace fallback; actual empty-hand GPU palettes remain anatomical; original ownership returns");
 SetRenderHands303(0,true,g_armPoseEpoch);g_weaponMechanicProfile=3;g_reload.shotgunFrame=g_frame;g_chambered=false;g_pumpStage=1;g_twoHand=true;UpdateTwoHand();Check(!g_chambered&&g_pumpStage==1&&!g_twoHand,"empty hands cannot advance pumping/chamber state of a stowed weapon");g_weaponMechanicProfile=20;
 SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_shotHand={};g_pistolReload={};Mat4 move={};Check(GunMove(g_armJoints[0],g_armJoints[1],move),"actual production gun move accepts primary left pose");
 const V3 moved=Apply(move,{g_armJoints[0].w[0],g_armJoints[0].w[1],g_armJoints[0].w[2]});Check(Length(Sub(moved,ControllerWrist(g_handBody[0],0)))<.002,"displayed weapon grip follows left controller through actual GunMove");
 Check(BuildArmPalette(context,1)!=nullptr,"305 held palette after controller-local binding publication");Read(device,context,g_armPaletteCb[1],fitted305);
 Mat4 gripRoot303={},grip303={};Check(ControllerCurrentRoot124(gripRoot303,grip303),"left-held magazine weapon has a real native grip frame");const V3 gunPalm303=Apply(Multiply(gripRoot303,grip303),{.6781,.0519,1.0387});const V3 leftPalm306=Apply(BoneMatrix(fitted305,2),{-.6781,.0519,1.0387});Check(Length(Sub(leftPalm306,actualLeftPalm303))<.002,"306 fitted left1911 palm stays on the physical tracked left palm after opposite-face compensation");
 // Validate the actual uploaded wrist axes and every captured finger against
 // a spatial reflection of the canonical fitted right hand around the gun's
 // native grip plane, not just the coincident palm point accepted in303.
 const Mat4 right305=Multiply(gripRoot303,grip303);const V3 centre305=Apply(gripRoot303,{0,0,0});const V3 axis305=Unit({gripRoot303.m[0],gripRoot303.m[4],gripRoot303.m[8]});
 for(int bone=2;bone<=19;++bone){const Mat4 canonical305=bone==2?right305:Multiply(right305,BoneMatrix(kPistolRightGripRelative,bone-3));const Mat4 actual305=BoneMatrix(fitted305,bone);
 for(const V3 point:{V3{.6781,.0519,1.0387},V3{.691,.011,1.149},V3{.591,.111,1.149},V3{.591,.011,1.249}}){const V3 r=Apply(canonical305,point);const V3 mirrored=Sub(r,Scale(axis305,2*Dot(Sub(r,centre305),axis305)));const V3 l=Apply(actual305,{-point.x,point.y,point.z});if(Length(Sub(l,mirrored))>=.002)fprintf(stderr,"bone%d distance %.6f actual %.4f %.4f %.4f expected %.4f %.4f %.4f\n",bone,Length(Sub(l,mirrored)),l.x,l.y,l.z,mirrored.x,mirrored.y,mirrored.z);Check(Length(Sub(l,mirrored))<.002,"306 GPU wrist axes and captured fingers match Shadow native weapon centre plane");}}
 // Exercise actual GPU skinning with action publication expired after a shot stall.
 {extern void StallRender314();StallRender314();
 Check(!fc3hands303::Read().active,"314 stalled action publication remains inactive");
 fc3stereo::SetHandPoses(physical[0],physical[1]);Check(g_handBody[0][3]<0,"314 stale publication cannot move primary weapon back to physical right");
 Check(BuildArmPalette(context,1)!=nullptr,"314 stalled left holding skin still builds");float stalled314[576];Read(device,context,g_armPaletteCb[1],stalled314);
 for(int bone=2;bone<=19;++bone){const auto expected=BoneMatrix(fitted305,bone),actual=BoneMatrix(stalled314,bone);for(int n=0;n<16;++n)Check(fabs(expected.m[n]-actual.m[n])<2e-4,"314 GPU wrist and fingers retain accepted left grip during publication gap");}
 SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);
 }
 // Keep the physical palm and the native left grip together through wrist
 // rotation and when the free right controller is temporarily untracked.
 const float turns306[3][9]={{1,0,0,0,1,0,0,0,1},{0,-1,0,1,0,0,0,0,1},{1,0,0,0,0,-1,0,1,0}};
 for(int turn=0;turn<3;++turn){for(int row=0;row<3;++row)for(int col=0;col<3;++col)physical[1][row*4+col]=turns306[turn][row*3+col];
 SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(turn==2?nullptr:physical[0],physical[1]);Check(BuildArmPalette(context,1)!=nullptr,"306 rotated/missing-free-controller left1911 palette builds");float output306[576];Read(device,context,g_armPaletteCb[1],output306);
 Mat4 controller306={};for(int i=0;i<16;++i)controller306.m[i]=physical[1][i];const V3 trackedPalm306=Apply(Multiply(controller306,g_freeHandAttachment[1].relative),{-.6781,.0519,1.0387});const V3 heldPalm306=Apply(BoneMatrix(output306,2),{-.6781,.0519,1.0387});Check(Length(Sub(trackedPalm306,heldPalm306))<.002,"306 fit remains attached to physical left palm through yaw/roll/pitch and right tracking loss");}
 SetRenderHands303(0,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);Check(g_handBody[0][3]>.2&&g_handBody[1][3]<-.2,"right-hand ownership restores accepted primary/support pose ordering");
 Compare(g_handBody[0],physical[0],16,"306 right-held1911 controller attachment remains byte-for-byte unchanged");
 SetRenderHands303(1,false,g_armPoseEpoch);g_weaponMechanicProfile=38;float untouched306[16];memcpy(untouched306,physical[1],64);fc3hands303::AdjustRolePose(0,untouched306);fc3stereo::SetHandPoses(physical[0],physical[1]);Compare(g_handBody[0],untouched306,16,"306 other left weapons retain305 controller attachment");g_weaponMechanicProfile=20;
 //307 uses the exact calibrated reload sphere, including mirrored receiving side.
 const Mat4 identity307=HandIdentity();g_pistolReload.palmBody=identity307;Check(Length(Sub(TransferSphere307(identity307,1),PistolHandGuide()))<1e-8,"307 left receiver sphere exactly matches existing reload contact centre");const V3 sphereL307=TransferSphere307(identity307,1),sphereR307=TransferSphere307(identity307,0);Check(fabs(sphereL307.x+sphereR307.x)<1e-8&&fabs(sphereL307.y-sphereR307.y)<1e-8&&fabs(sphereL307.z-sphereR307.z)<1e-8,"307 right receiver uses anatomical mirror of same calibrated reload sphere");
 Check(TransferHandleContact307(identity307,{0,-.013,-.015}),"307 calibrated hand sphere contacts actual1911 handle box");Check(!TransferHandleContact307(identity307,{g_pistolGuides.port[0],g_pistolGuides.port[1],g_pistolGuides.port[2]}),"307 magazine well remains a reload target rather than a hand transfer");Check(!TransferHandleContact307(identity307,{g_pistolGuides.slide[0],g_pistolGuides.slide[1],g_pistolGuides.slide[2]}),"307 slide remains a reload target rather than a hand transfer");Check(!TransferHandleContact307(identity307,{.5,.5,.5}),"307 remote contact cannot take weapon");
 for(int owner307=0;owner307<2;++owner307){SetRenderHands303(owner307,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_pistolReload={};g_pistolReload.magazinePresent=true;g_reload.held=g_reload.pending=g_reloadAlignment=false;g_twoHand=true;
 Mat4 root307={},grip307={};Check(ControllerCurrentRoot124(root307,grip307),"307 actual1911 root available");const int free307=1-owner307;const V3 target307=Apply(root307,{0,-.013,-.015});Mat4 freeController307={};for(int i=0;i<16;++i)freeController307.m[i]=g_physicalBody303[free307][i];const V3 current307=TransferSphere307(Multiply(freeController307,g_freeHandAttachment[free307].relative),free307);const V3 shift307=Sub(target307,current307);g_physicalBody303[free307][3]+=float(shift307.x);g_physicalBody303[free307][7]+=float(shift307.y);g_physicalBody303[free307][11]+=float(shift307.z);
 Publish1911Transfer307();auto contact307=fc3hands303::TransferContact307();Check(contact307.enabled&&contact307.allowed&&contact307.contact,"309 shipping publisher starts from native support in either hand");g_pistolReload.held=true;Publish1911Transfer307();contact307=fc3hands303::TransferContact307();Check(contact307.enabled&&!contact307.allowed&&!contact307.contact,"307 carrying a reload magazine preempts actual transfer publication");g_pistolReload.held=false;
 for(int target308=0;target308<2;++target308){const auto box308=PistolGuideBox(root307,target308);const V3 reloadPoint308={box308.m[3],box308.m[7],box308.m[11]};Check(ReloadContact308(root307,reloadPoint308),"308 actual calibrated magazine/slide region retains priority in either ownership");}
 g_weaponMechanicProfile=38;Publish1911Transfer307();Check(!fc3hands303::TransferContact307().enabled,"307 other weapons retain original transfer policy");g_weaponMechanicProfile=20;}
 //309 native weapon-local support, with no308 controller-distance replacement.
 for(int owner309=0;owner309<2;++owner309){
 SetRenderHands303(owner309,false,g_armPoseEpoch);SetRenderReceiver309(-1);g_pistolReload={};g_pistolReload.magazinePresent=true;g_reload={};g_reloadAlignment=false;g_twoHand=false;g_autoPistolSupport=true;
 fc3pad::Controllers neutral309={};neutral309.active=true;fc3pad::Publish(neutral309);fc3pad::SetGameplayControls(true);
 auto& native309=g_supportLocal236[20];native309.valid=true;native309.epoch=g_armPoseEpoch;native309.point={.02,-.013,-.015};
 const int free309=1-owner309;fc3stereo::SetHandPoses(physical[0],physical[1]);
 for(int align309=0;align309<2;++align309){Mat4 root309={},grip309={};Check(ControllerCurrentRoot124(root309,grip309),"309 native root exists");V3 local309=native309.point;if(owner309)local309.x=-local309.x;V3 target309=Apply(root309,local309);SupportGrabAdjustPoint214(target309);V3 palm309={};Check(FreeHandPalm(1,palm309),"309 support role resolves current physical free palm");const V3 shift309=Sub(target309,palm309);physical[free309][3]+=float(shift309.x);physical[free309][7]+=float(shift309.y);physical[free309][11]+=float(shift309.z);fc3stereo::SetHandPoses(physical[0],physical[1]);}
 UpdateTwoHand();Check(g_twoHand,"309 restored native support target acquires without fresh squeeze in either hand");
 SetRenderReceiver309(free309);UpdateTwoHand();Check(g_twoHand,"309 arming transfer does not release the native two-hand hold");
 Publish1911Transfer307();Check(fc3hands303::TransferContact307().contact,"309 actual publisher acknowledges intact native support");
 physical[free309][3]+=.5f;fc3stereo::SetHandPoses(physical[0],physical[1]);UpdateTwoHand();Check(!g_twoHand,"309 moving away releases native support normally");
 Publish1911Transfer307();Check(!fc3hands303::TransferContact307().modifier321&&!fc3hands303::TransferContact307().contact,"327 without owning grip original support contact remains required");SetRenderReceiver309(-1);
 }
 //310 real native support palette, then its physical-right mirror for left ownership.
 g_nativeSupport310={};g_pistolReload={};g_pistolReload.magazinePresent=true;g_reload={};g_reloadAlignment=false;
 SetRenderHands303(0,false,g_armPoseEpoch);SetRenderReceiver309(-1);fc3stereo::SetHandPoses(physical[0],physical[1]);g_twoHand=true;g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);Check(g_frameGunValid,"310 native support gun move ready");
 Mat4 root310={},grip310={},ri310={},qi310={};Check(ControllerCurrentRoot124(root310,grip310)&&Invert(root310,ri310)&&Invert(ArmBodyToCamera(),qi310),"310 current native gun reference ready");
 memcpy(original,kNeutralArmView[1],sizeof(kNeutralArmView[1]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
 const Mat4 rawLocal310=Multiply(ri310,Multiply(qi310,Multiply(g_frameGunMove,BoneMatrix(original,2))));
 g_supportLocal236[20].point=Add(Apply(rawLocal310,{-.6781,.0519,1.0387}),{.08,-.06,.04});g_supportLocal236[20].valid=true;g_supportLocal236[20].epoch=g_armPoseEpoch;
 Check(BuildArmPalette(context,1)!=nullptr,"310 actual right-owned native support arm builds");Check(g_nativeSupport310.valid&&g_nativeSupport310.confirmed,"310 captures actual native support wrist and fingers in gun space");
 float nativeSupport310[576];Read(device,context,g_armPaletteCb[1],nativeSupport310);
 for(int bone=2;bone<=19;++bone){const Mat4 expected=Multiply(ri310,Multiply(qi310,BoneMatrix(nativeSupport310,bone)));for(int n=0;n<16;++n)Check(fabs(expected.m[n]-g_nativeSupport310.local[bone-2].m[n])<2e-4,"311 source must equal final displayed support even when trigger centre differs");}
 SetRenderHands303(1,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);g_twoHand=true;g_frameGunValid=GunMove(g_armJoints[0],g_armJoints[1],g_frameGunMove);Check(ControllerCurrentRoot124(root310,grip310),"310 left-owned root ready");
 memcpy(original,kNeutralArmView[0],sizeof(kNeutralArmView[0]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);
 Check(BuildArmPalette(context,0)!=nullptr,"310 physical right support arm builds for left-owned1911");float supported310[576];Read(device,context,g_armPaletteCb[0],supported310);Mat4 mirror310=HandIdentity();mirror310.m[0]=-1;
 for(int bone=2;bone<=19;++bone){const Mat4 expected=Multiply(ArmBodyToCamera(),Multiply(root310,Multiply(mirror310,Multiply(g_nativeSupport310.local[bone-2],mirror310))));const Mat4 actual=BoneMatrix(supported310,bone);for(int n=0;n<16;++n)Check(fabs(actual.m[n]-expected.m[n])<2e-4,"310 GPU right support wrist and all fingers mirror native left support on current gun");}
 V3 contact311={},freePalm311={};Check(LeftSupportContact311(contact311,freePalm311),"311 left support target uses actual cached support palm");Check(Length(Sub(contact311,Apply(BoneMatrix(supported310,2),{.6781,.0519,1.0387})))<.002,"311 left support trigger and displayed right palm agree instead of shifted neutral centre");
 SetRenderReceiver309(0);Check(BuildArmPalette(context,0)!=nullptr,"310 arming reverse transfer retains support palette");float armed310[576];Read(device,context,g_armPaletteCb[0],armed310);for(int bone=2;bone<=19;++bone)Compare(armed310+bone*12,supported310+bone*12,12,"310 arming handover cannot release or change support grip");SetRenderReceiver309(-1);
 //The accepted holding hand must remain fitted while right support is added.
 Check(BuildArmPalette(context,1)!=nullptr,"310 left holding palette remains available while supported");float held310[576];Read(device,context,g_armPaletteCb[1],held310);const Mat4 capturedRight310=Multiply(root310,grip310);
 for(int bone=2;bone<=19;++bone){const Mat4 canonical=bone==2?capturedRight310:Multiply(capturedRight310,BoneMatrix(kPistolRightGripRelative,bone-3));const Mat4 expected=Multiply(mirror310,Multiply(canonical,mirror310));(void)expected;
 const V3 axis=Unit({root310.m[0],root310.m[4],root310.m[8]}),centre=Apply(root310,{0,0,0});for(const V3 point:{V3{.6781,.0519,1.0387},V3{.591,.111,1.149}}){const V3 right=Apply(canonical,point);const V3 reflected=Sub(right,Scale(axis,2*Dot(Sub(right,centre),axis)));const V3 actual=Apply(BoneMatrix(held310,bone),{-point.x,point.y,point.z});Check(Length(Sub(actual,reflected))<.002,"310 GPU306 holding hand remains unchanged while supporting");}}
 g_twoHand=false;Check(BuildArmPalette(context,0)!=nullptr,"310 leaving support restores free physical right arm");float free310[576];Read(device,context,g_armPaletteCb[0],free310);Check(Length(Sub(Apply(BoneMatrix(free310,2),{.591,.011,1.149}),ControllerWrist(physical[0],0)))<.002,"310 support release follows right controller instead of sticking to gun");
 g_twoHand=true;g_pistolReload.held=true;Check(BuildArmPalette(context,0)!=nullptr,"310 reload lease releases support pose");Read(device,context,g_armPaletteCb[0],free310);Check(Length(Sub(Apply(BoneMatrix(free310,2),{.591,.011,1.149}),ControllerWrist(physical[0],0)))<.002,"310 magazine hand cannot remain pinned by support");g_pistolReload.held=false;
 {PhysicalArmScope303 scope310;Mat4 desired310={};++g_nativeSupport310.epoch;Check(!Right1911Support310(scope310,0,desired310),"310 stale arm epoch cannot replay support skin");--g_nativeSupport310.epoch;++g_nativeSupport310.entity;Check(!Right1911Support310(scope310,0,desired310),"310 different weapon identity cannot reuse support skin");--g_nativeSupport310.entity;g_weaponMechanicProfile=3;Check(!Right1911Support310(scope310,0,desired310),"310 other weapon profiles do not use1911 support skin");g_weaponMechanicProfile=20;}
 g_nativeSupport310={};g_twoHand=false;memcpy(original,kNeutralArmView[1],sizeof(kNeutralArmView[1]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);Check(BuildArmPalette(context,1)!=nullptr&&g_nativeSupport310.valid&&!g_nativeSupport310.confirmed,"310 direct-left draw can seed native support without a registration gesture");
 //312: actual physical-right reload contact, role controls and carried clip.
 SetRenderHands303(1,false,g_armPoseEpoch);SetRenderReceiver309(-1);g_pistolReload={};g_pistolReload.enabled=true;g_pistolReload.magazinePresent=false;g_pistolReload.basis=true;g_reload={};g_reloadAlignment=false;g_twoHand=false;
 physical[0][3]+=.7f;fc3stereo::SetHandPoses(physical[0],physical[1]);Mat4 reloadRoot312={},reloadGrip312={};Check(ControllerCurrentRoot124(reloadRoot312,reloadGrip312),"312 left-held reload root exists");g_pistolReload.rootBody=reloadRoot312;g_pistolReload.rootFrame=g_pistolReload.portFrame=g_frame;g_pistolReload.rootEpoch=g_armPoseEpoch;
 memcpy(original,kNeutralArmView[0],sizeof(kNeutralArmView[0]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(original),original);Check(BuildArmPalette(context,0)!=nullptr,"312 real right reload arm palette builds");float rightFree312[576];Read(device,context,g_armPaletteCb[0],rightFree312);
 const V3 actualGuide312=Apply(BoneMatrix(rightFree312,2),{.591-g_pistolGuides.hand[0],.011+g_pistolGuides.hand[1],1.149+g_pistolGuides.hand[2]});Check(Length(Sub(PistolHandGuide(),actualGuide312))<.002,"312 reload sphere follows displayed physical RIGHT hand");
 Check(BuildArmPalette(context,1)!=nullptr,"312 fitted holding LEFT hand still builds");Check(Length(Sub(PistolHandGuide(),actualGuide312))<.002,"312 later holding-left draw cannot overwrite free-right contact");
 Publish1911Transfer307();PublishReloadControls312(0,0);PublishReloadControls312(1,0,true,false);Check(fc3pad::AmmoControls().leftGrip>.9f&&fc3pad::AmmoAction(fc3pad::AmmoControls())==fc3pad::AmmoButtonAction::Release,"312 physical right grip B reaches unchanged canonical reload action while left owns");PublishReloadControls312(0,0);
 fc3pad::SetGripOnlyAmmo(true);PublishReloadControls312(1,0);TickPistolLoose(fc3pad::AmmoControls(),GetTickCount64());Check(g_pistolReload.held&&g_pistolReload.looseId==1&&!g_pistolReload.pending&&!g_pistolReload.cycle,"312 physical RIGHT grip obtains one magazine without native reload request");
 Check(BuildArmPalette(context,0)!=nullptr,"312 right magazine hand builds");float rightClip312[576];Read(device,context,g_armPaletteCb[0],rightClip312);const Mat4 actualRight312=BoneMatrix(rightClip312,2);Mat4 f312=HandIdentity();f312.m[0]=-1;
 const Mat4 expectedMagazine312=Multiply(actualRight312,Multiply(f312,Multiply(MagazineHeldFromHand(),f312)));for(int n=0;n<16;++n)Check(fabs(g_pistolReload.hand.m[n]-expectedMagazine312.m[n])<2e-4,"312 carried magazine attaches to displayed right wrist with mirrored native attachment");
 for(int bone=3;bone<=19;++bone){const Mat4 expected=Multiply(actualRight312,Multiply(f312,Multiply(BoneMatrix(PistolMagazineGripCur(),bone-3),f312)));const Mat4 actual=BoneMatrix(rightClip312,bone);for(int n=0;n<16;++n)Check(fabs(expected.m[n]-actual.m[n])<2e-4,"312 GPU right fingers mirror working magazine grip");}
 const Mat4 carried312=g_pistolReload.hand;Check(BuildArmPalette(context,1)!=nullptr,"312 left holding palette preserved during magazine carry");for(int n=0;n<16;++n)Check(fabs(carried312.m[n]-g_pistolReload.hand.m[n])<2e-4,"312 holding hand cannot steal magazine attachment");
 //Bring the physical right hand's contact to the actual native insertion target.
 Mat4 port312=PistolGuideBox(g_pistolReload.rootBody,0);const V3 portPoint312={port312.m[3],port312.m[7],port312.m[11]},toPort312=Sub(portPoint312,PistolHandGuide());physical[0][3]+=float(toPort312.x);physical[0][7]+=float(toPort312.y);physical[0][11]+=float(toPort312.z);fc3stereo::SetHandPoses(physical[0],physical[1]);Check(BuildArmPalette(context,0)!=nullptr,"312 physical right insertion arm builds");Check(PistolGuideContact(g_pistolReload.rootBody,0,g_pistolReload.tip),"312 right-hand magazine contact reaches existing native insertion gate");
 //Slide stroke reads the real physical right controller, never the pinned skin.
 g_pistolReload.held=false;g_pistolReload.magazinePresent=true;g_pistolReload.rackMode=g_pistolReload.rackHeld=true;g_pistolReload.slidePull=.025;g_pistolReload.rootEpoch=g_armPoseEpoch;g_pistolReload.rootFrame=g_frame;
 Check(BuildArmPalette(context,0)!=nullptr,"312 mirrored right slide hand builds");float rightSlide312[576];Read(device,context,g_armPaletteCb[0],rightSlide312);Mat4 nativeSlide312={};float slideFingers312[204];Check(RifleChargeClipSample(.5,nativeSlide312,slideFingers312),"312 existing native slide clip sampled");const Mat4 expectedSlide312=Multiply(g_pistolReload.rootBody,Multiply(f312,Multiply(nativeSlide312,f312)));const Mat4 actualSlide312=BoneMatrix(rightSlide312,2);for(int n=0;n<16;++n)Check(fabs(actualSlide312.m[n]-expectedSlide312.m[n])<2e-4,"312 GPU right slide wrist mirrors native reload pose on left-held gun");
 Check(Length(Sub(PhysicalReloadWrist312({99,99,99}),ControllerWrist(physical[0],0)))<.002,"312 slide motion reads physical right wrist rather than pinned animation");
 for(int bone=3;bone<=19;++bone){const Mat4 expected=Multiply(actualSlide312,Multiply(f312,Multiply(BoneMatrix(slideFingers312,bone-3),f312)));const Mat4 actual=BoneMatrix(rightSlide312,bone);for(int n=0;n<16;++n)Check(fabs(actual.m[n]-expected.m[n])<2e-4,"312 GPU right slide fingers mirror native clip");}
 g_pistolReload.rackHeld=g_pistolReload.rackMode=false;SetRenderHands303(0,false,g_armPoseEpoch);fc3stereo::SetHandPoses(physical[0],physical[1]);const V3 sentinel312={1,2,3};Check(Length(Sub(PhysicalReloadWrist312(sentinel312),sentinel312))<1e-8,"312 right-owned canonical reload wrist path remains untouched");
 puts("PASS:312 actual right reload contact/controls/pickup/magazine/insertion/slide skin;311 support/306 holding preserved; no headset/game launched");
 TestSidearms315(device,context,cb,physical);
 for(int profile:{8,7,41,28,14,9,31,25,21,17,35,36,22})TestAk316(device,context,cb,physical,profile);
 for(int excluded:{0,2,3,4,5,6,10,15,18,23,32,34,38,43}){g_weaponMechanicProfile=excluded;g_weaponMechanicFrame=g_frame;Check(!fc3hands303::MagazineRifle317(excluded)&&!LeftAk316(),"317 excluded shotgun/bolt/special profiles cannot enter magazine hand adapters");Publish1911Transfer307();Check(fc3hands303::Shotgun318(excluded)||fc3hands303::Other319(excluded)||excluded==10||!fc3hands303::TransferContact307().enabled,"317 non-shotgun excluded profiles retain their previous transfer policy");const auto before=Multiply(HandIdentity(),HandIdentity()),after=AkCarried316(HandIdentity(),HandIdentity());for(int i=0;i<16;++i)Check(fc3hands303::OtherMagazine319(excluded)||before.m[i]==after.m[i],"317 excluded ammo transforms remain canonical");}
 puts("PASS:317 thirteen magazine families in both owners,charging paths,shotgun/bolt/PKM/special exclusions");
 TestShotguns318(device,context,cb,physical);
 TestOthers319(device,context,cb,physical);
 TestBow319(device,context,cb,physical);
 TestRevolver321(device,context,cb,physical);
 TestRevolverSwipe323(device,context,cb,physical);
 TestRevolverMirror325(device,context,cb,physical);
 TestHands326(device,context,cb,physical);
 TestBoth1911Methods327(device,context,cb,physical);
 TestSmgs332(device,context,cb,physical);
 TestHandleBinding333(device,context,cb,physical);
 TestA2000Full337(device,context,cb);
 TestSideMirror338(device,context,cb);
 TestCapture331(device,context);
 TestAim345(device,context,cb,physical,vb);
 TestSupport349(device,context,cb,physical);
 TestFlamerLever349(device,context,cb,physical);
 TestGrab350(device,context,cb,physical);
 context->ClearState();for(auto* buffer:cb)if(buffer)buffer->Release();for(auto*& buffer:g_armPaletteCb)SafeRelease(buffer);vb->Release();context->Release();device->Release();
}




