#include "../src/support_continuation356.h"
void InitWeapon356(int profile,int owner,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload={};g_reloadPose={};g_manualReload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;
 g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=g_pistolReload.chamberReady=true;
 g_rpg={};g_shotgunSkin318={};g_otherSkin319={};g_machete.held=false;g_autoLongGunSupport=false;
 SetRenderNative319(0);SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);PublishSidearmControls315(0,0);
 for(int h=0;h<2;++h){for(int i=0;i<16;++i)physical[h][i]=i%5==0?1.f:0.f;physical[h][3]=h?-.35f:.35f;physical[h][7]=-.2f;physical[h][11]=.5f;}
 context->VSSetConstantBuffers(0,4,cb);fc3stereo::SetHandPoses(physical[0],physical[1]);
 for(int side=0;side<2;++side){float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,HandIdentity());memcpy(raw,kNeutralArmView[side],sizeof(kNeutralArmView[side]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);Check(BuildArmPalette(context,side)!=nullptr,"356 initial anatomical arm draw");}
}
void TestM133Shot356(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
 for(int profile:{3,4,5,6})for(int owner:{0,1}){
  InitWeapon356(profile,owner,context,cb,physical);g_manualReloadEnabled=true;g_chambered=true;g_pumpEnabled=true;g_pumpStage=0;g_pumpBaseSet=false;g_lastTrigger=0;
  g_armJoints[0].valid=g_armJoints[1].valid=true;g_m133SupportLoaded56=true;g_m133Support56={};g_m133Support56.positioned=true;g_m133Support56.position[0]=50;
  g_reloadContact.port=true;g_reloadContact.portFrame=g_frame;g_reloadContact.portParent=HandIdentity();
  auto c=fc3pad::Controllers{};c.active=true;c.leftGrip=1;c.shotgunSupport342=true;c.shotgunSupportProfile342=profile;
  const auto held=fc3hands303::Read();c.shotgunSupportEntity342=held.entity;c.shotgunSupportGeneration342=held.generation;c.shotgunSupportAt342=GetTickCount64();fc3pad::Publish(c);
  UpdateTwoHand();Check(g_twoHand,"356 actual accepted M133 forend acquires before firing");
  c.rightTrigger=.85f;fc3pad::Publish(c);UpdateTwoHand();Check(!g_chambered&&g_twoHand,"356 firing spends the chamber without releasing support");
  BYTE* page=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"356 native reload-pose memory");
  const UINT actor=UINT(page),component=actor+4096,state=component+0x718,wrapper=actor+8192,weapon=actor+12288,hud=actor+16384;
  *reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;*reinterpret_cast<UINT*>(wrapper+0x48)=weapon;
  *reinterpret_cast<UINT*>(weapon+0xcc)=2;const UINT values[5]={2,7,0,10,12};memcpy(reinterpret_cast<void*>(hud+0x134),values,sizeof(values));
  g_reload.widgets[0].hud=LONG(hud);g_reload.widgets[0].wrapper=LONG(wrapper);g_reload.shotgunFrame=g_frame;
  g_reloadPose.state=state;g_reloadPose.wrapper=wrapper;g_reloadPose.weapon=weapon;g_reloadPose.active=1;
  Check(ReloadPoseActive()&&ManualReloadOwnsLeftHand(),"356 real validated native pose lease overlaps a physical support squeeze");
  for(int step=0;step<3;++step){++g_frame;g_weaponMechanicFrame=g_reload.shotgunFrame=g_frame;c.shotgunSupportAt342=GetTickCount64();fc3pad::Publish(c);UpdateTwoHand();
   Check(g_twoHand,"356 M133 native post-shot pose must not release accepted physical support");Check(ReloadPoseActive(),"356 support cannot cancel native engine pose lifecycle");}
  // An actual physical shell transaction still owns the hand and blocks support.
  g_manualReload.pending=1;c.shotgunSupportAt342=GetTickCount64();fc3pad::Publish(c);UpdateTwoHand();Check(!g_twoHand,"356 physical shell insertion retains reload priority");
  g_manualReload={};g_reloadPose.active=0;g_reload={};c.leftGrip=0;c.rightTrigger=0;fc3pad::Publish(c);UpdateTwoHand();Check(!g_twoHand,"356 released grip does not leave phantom support");
  VirtualFree(page,0,MEM_RELEASE);
 }
 vb->Release();puts("PASS:356 actual M133/signature firing and native pose overlap in both owners, retained shell priority, native lifecycle and physical release");
}
void TestRpgFit356(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
 for(int owner:{0,1}){
  InitWeapon356(32,owner,context,cb,physical);const auto saved=g_supportGrab214[32];g_supportGrab214[32].offset[0]=g_supportGrab214[32].offset[1]=g_supportGrab214[32].offset[2]=0;
  g_twoHand=true;fc3stereo::SetHandPoses(physical[0],physical[1]);float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,HandIdentity());memcpy(raw,kNeutralArmView[1-owner],sizeof(kNeutralArmView[1-owner]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);
  Check(BuildArmPalette(context,1-owner)!=nullptr,"356 RPG support actual GPU palette");float skin[576]={};Read(device,context,g_armPaletteCb[1-owner],skin);
  Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"356 RPG controlled tube root");
  const Mat4 authored=RpgKeyMat(kRpgLeftSupportFromFrame);const V3 native=Apply(root,Apply(authored,{-.6781,.0519,1.0387}));
  const V3 actual=Apply(g_armFinalBody[1-owner][2],{owner?.6781:-.6781,.0519,1.0387});
  Check(Length(Sub(native,actual))<.003,"356 opposite anatomical support hand must contact the same unmirrored RPG grasp");
  Mat4 expected=Multiply(ArmBodyToCamera(),Multiply(root,authored));
  if(owner){Mat4 plane=SidearmMirror315();plane.m[3]=2*Apply(authored,{-.6781,.0519,1.0387}).x;expected=Multiply(ArmBodyToCamera(),Multiply(root,Multiply(plane,Multiply(authored,SidearmMirror315()))));}
  for(int i=0;i<12;++i)Check(std::fabs(BoneMatrix(skin,2).m[i]-expected.m[i])<.003,"356 RPG GPU wrist fits native contact with proper anatomical orientation");
  V3 guide={},palm={};Check(GrabGizmoPoints({},guide,palm)&&Length(Sub(guide,native))<.003,"356 RPG guide describes actual unmirrored tube contact");
  g_supportGrab214[32]=saved;g_twoHand=false;
 }
 vb->Release();puts("PASS:356 RPG native physical grasp, opposite anatomical wrist, actual GPU palm/guide consistency in both owners");
}

void TestSupportCache356(){
 fc3support356::Cache cache;fc3grab333::Binding b;b.mode=b.enabled=b.support=b.supportActive=true;b.profile=3;b.entity=17;b.generation=4;b.epoch=2;b.reference=8;b.at=100;
 auto choose=[&](const fc3grab333::Binding& v,bool latched,bool tracked,float grip,ULONGLONG at){return cache.Choose(v,17,4,2,8,latched,tracked,grip,at);};
 choose(b,true,true,1,100);auto missing=b;missing.enabled=missing.support=missing.supportActive=false;missing.at=110;
 Check(choose(missing,true,true,1,110).supportCarry356,"356 cache bridges missing binding for an existing physical support grip");
 Check(!choose(missing,false,true,1,110).supportCarry356,"356 cache cannot acquire a new support grip");
 Check(!choose(missing,true,false,1,110).supportCarry356,"356 cache cannot bridge tracking loss");
 Check(!choose(missing,true,true,0,110).supportCarry356,"356 cache cannot bridge physical grip release");
 missing.physicalReload356=true;Check(!choose(missing,true,true,1,110).supportCarry356,"356 explicit physical reload vetoes continuation");missing.physicalReload356=false;
 missing.at=220;Check(choose(missing,true,true,1,220).supportCarry356,"356 continuation accepts bounded120ms old source");
 missing.at=221;Check(!choose(missing,true,true,1,221).supportCarry356,"356 missing publications cannot renew source lifetime");
 auto changed=missing;changed.generation=5;Check(!choose(changed,true,true,1,110).supportCarry356,"356 generation change rejects cached weapon support");
 choose(b,true,true,1,100);changed=missing;changed.entity=18;Check(!choose(changed,true,true,1,110).supportCarry356,"356 weapon entity change rejects cached support");
 choose(b,true,true,1,100);changed=missing;changed.reference=9;Check(!choose(changed,true,true,1,110).supportCarry356,"356 XR reference change rejects cached support");
 choose(b,true,true,1,100);changed=missing;changed.epoch=3;Check(!choose(changed,true,true,1,110).supportCarry356,"356 pose epoch change rejects cached support");
 choose(b,true,true,1,100);changed=missing;changed.profile=34;Check(!choose(changed,true,true,1,110).supportCarry356,"356 M133 cache cannot modify SPAS or other weapons");
 puts("PASS:356 bounded M133 continuation, current physical grip/tracking, transaction veto and identity/reference/epoch/generation isolation");
}
