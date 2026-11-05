// Real D3D bindings, existing exact attachment-family classification, and
// production rigid/skinned attachment functions. This is coordinate evidence,
// not a claim that every game's optic was visually checked in a headset.
void TestWeaponSightPose51(ID3D11Device* device,ID3D11DeviceContext* context,bool shifted=false)
{
 PistolReloadState states[20];memcpy(states,g_magazineStates,sizeof(states));
 RifleAttachmentRel attachments[32];memcpy(attachments,g_rifleAttachRel,sizeof(attachments));
 const LONG oldProfile=g_weaponMechanicProfile,oldMechanic=g_weaponMechanicFrame,oldFrame=g_frame,oldAttachProfile=g_rifleAttachProfile,oldAttachFrame=g_rifleAttachFrame;
 const UINT oldAttachEpoch=g_rifleAttachEpoch;const Mat4 oldAttachMove=g_rifleAttachMove;
 const bool oldMotion=g_motionGun,oldGuard=g_weaponMotionGuard,oldRight=g_handRightValid,oldLeft=g_handLeftValid,oldTwo=g_twoHand,oldAim=g_aimActive;
 const auto controllers=fc3pad::Latest();double aim[3][3];memcpy(aim,g_aimQ,sizeof(aim));
 g_motionGun=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=false;g_twoHand=false;g_aimActive=true;
 fc3pad::Controllers active={};active.active=true;fc3pad::Publish(active);
 ID3D11Buffer* vb=Buffer(device,48520,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,7032,D3D11_BIND_INDEX_BUFFER),*world=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 ID3D11VertexShader* shader=nullptr;
 if(shifted){
  auto* code=HullCompile("float4 AlphaValues;row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;float4 main(uint id:SV_VertexID):SV_Position{return float4(mul(_WorldMatrix,float4(0,0,0,1))+_WorldPositionFractions+AlphaValues.xyz*.00001,1);}","vs_5_0");
  Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)),"sight220 shifted attachment world shader");shader->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
 }context->VSSetShader(shader,nullptr,0);
 UINT stride=28,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);ID3D11Buffer* cb[2]={world,camera};context->VSSetConstantBuffers(0,2,cb);
 auto matches=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>.0005)return false;return true;};
 // The production table owns these families: AK, F1, A2000, P416, ACE, SVD,
 // R700, U100, MKG, Z93 and STG90. No new model/draw ranges are invented.
 const LONG profiles[]={8,14,28,31,17,35,2,36,22,23,41};
 for(LONG profile:profiles){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_rifleAttachProfile=-1;
  auto& p=g_pistolReload;p=PistolReloadState{};p.enabled=true;p.magazinePresent=true;p.rootFrame=g_frame;p.rootNativeFrame=g_frame;p.rootEpoch=g_armPoseEpoch;p.rootContext=context;
  p.rootBody=HandIdentity();p.rootBody.m[3]=.23;p.rootBody.m[7]=-.38;p.rootBody.m[11]=.61;
  p.rootNative=HandIdentity();p.rootNative.m[3]=100;p.rootNative.m[7]=20;p.rootNative.m[11]=40;
  const Mat4 established=p.rootBody;Mat4 relative=HandIdentity();relative.m[3]=.02;relative.m[7]=.15;relative.m[11]=.08;
  const Mat4 attachment=Multiply(p.rootNative,relative);float globals[88]={};if(shifted){globals[0]=.1f;globals[1]=.2f;globals[2]=.3f;globals[3]=.4f;}WritePistolWorld(globals+(shifted?4:0),attachment);world->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);
  g_rifleAttachFrame=g_frame;g_rifleAttachEpoch=g_armPoseEpoch;g_rifleAttachMove=HandIdentity();
  CameraSummary cam={};cam.mainView=1;cam.right[0]=cam.up[1]=cam.forward[2]=1;cam.position[0]=100;cam.position[1]=20;cam.position[2]=40;
  for(int step=0;step<12;++step){
   const double angle=.035*step;for(int r=0;r<3;++r)for(int c=0;c<3;++c)g_aimQ[r][c]=r==c;
   g_aimQ[0][0]=g_aimQ[2][2]=cos(angle);g_aimQ[0][2]=sin(angle);g_aimQ[2][0]=-sin(angle);
   cam.position[1]=20-(step%2)*.55f;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
   Mat4 bodyInverse={};Check(Invert(ArmBodyToCamera(),bodyInverse),"sight51 current body basis invertible");
   Mat4 rigid={};Check(RifleAttachmentMove(context,0,168,rigid),"sight51 known rigid optic obtains corrected FRAME move");
   Check(matches(Multiply(bodyInverse,Multiply(rigid,Multiply(HullView(cam),attachment))),Multiply(established,relative)),"sight51 rigid glow/lens agrees with established rifle body after within-frame aim/crouch camera change");
   stride=40;offset=1680;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
   float bones[48*12]={};for(int bone=0;bone<48;++bone)StoreBone(bones,bone,HandIdentity());
   Mat4 skinned={};const Mat4 nativeView=Multiply(HullView(cam),attachment);
   Check(RifleSkinnedAttachmentMove(context,168,3030,bones,nativeView,skinned),"sight51 known skinned housing/lens obtains corrected FRAME move during ordinary hold");
   Check(matches(Multiply(bodyInverse,Multiply(skinned,nativeView)),Multiply(established,relative)),"sight51 skinned and rigid optic parts share body pose after camera reconciliation");
   if(profile==2){Mat4 fallback={};g_rifleAttachFrame=g_frame;g_rifleAttachEpoch=g_armPoseEpoch;Check(R700GunMove(context,fallback)&&matches(Multiply(bodyInverse,Multiply(fallback,Multiply(HullView(cam),p.rootNative))),established),"R700 scope fallback refreshes current draw camera instead of cached move");}
   Check(!memcmp(&established,&p.rootBody,sizeof(established)),"sight51 refresh does not relearn saved body attachment");
   stride=28;offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
  }
  const Mat4 before=p.rootWorld;p.rootFrame=g_frame-1;Check(!RefreshWeaponDrawRoot51(context)&&!memcmp(&before,&p.rootWorld,sizeof(before)),"sight51 stale rendered-frame root never refreshed");p.rootFrame=g_frame;
  p.rootEpoch=g_armPoseEpoch+1;Check(!RefreshWeaponDrawRoot51(context),"sight51 foreign tracking epoch rejected");p.rootEpoch=g_armPoseEpoch;
  p.rootContext=nullptr;Check(!RefreshWeaponDrawRoot51(context),"sight51 foreign context rejected");p.rootContext=context;
  cam.mainView=0;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);Check(!RefreshWeaponDrawRoot51(context),"sight51 reflection/non-main camera rejected");
 }
 memcpy(g_magazineStates,states,sizeof(states));memcpy(g_rifleAttachRel,attachments,sizeof(attachments));g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=oldMechanic;g_frame=oldFrame;g_rifleAttachProfile=oldAttachProfile;g_rifleAttachFrame=oldAttachFrame;g_rifleAttachEpoch=oldAttachEpoch;g_rifleAttachMove=oldAttachMove;
 g_motionGun=oldMotion;g_weaponMotionGuard=oldGuard;g_handRightValid=oldRight;g_handLeftValid=oldLeft;g_twoHand=oldTwo;g_aimActive=oldAim;memcpy(g_aimQ,aim,sizeof(aim));fc3pad::Publish(controllers);
 context->ClearState();SafeRelease(shader);vb->Release();ib->Release();world->Release();camera->Release();
 puts("PASS: sight51 existing rigid/skinned optic families agree with established FRAME across within-frame aim/crouch changes; stale frame/context/epoch/non-main rejection");
}
