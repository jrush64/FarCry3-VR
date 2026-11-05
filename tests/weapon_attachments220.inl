void TestWeaponSightPose51(ID3D11Device*,ID3D11DeviceContext*,bool);
void TestPistolAttachments220(ID3D11Device* device,ID3D11DeviceContext* context)
{
 context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_oCreateBuffer=PistolFixtureCreate;
 const auto oldProfile=g_weaponMechanicProfile;const auto oldAttachmentProfile=g_pistolAttachProfile220;
 PistolAttachment oldSlots[32];memcpy(oldSlots,g_pistolAttach,sizeof(oldSlots));PistolReloadState oldStates[20];memcpy(oldStates,g_magazineStates,sizeof(oldStates));
 auto* oldOutput=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;
 g_weaponMotionGuard=g_flatViewActive=g_twoHand=g_aimActive=false;g_machete.held=false;g_pistolMenu.open=false;g_reload={};g_r700={};
 fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);const Mat4 id=HandIdentity();++g_armPoseEpoch;g_freeRightEpoch=g_armPoseEpoch;g_freeRightFromController=id;
 g_frameGunValid=true;g_frameGunMove=id;
 for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);
 g_handBody[0][3]=.2f;g_handBody[0][7]=-.2f;g_handBody[0][11]=.4f;memcpy(g_handRight,g_handBody[0],64);memcpy(g_handLeft,g_handBody[1],64);
 auto* vb=Buffer(device,48520,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,7032,D3D11_BIND_INDEX_BUFFER);
 auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 auto* palette=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);auto* clip=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);
 const float alpha[4]={.1f,.2f,.3f,.4f};clip->SetPrivateData(kCbCopyGuid,sizeof(alpha),alpha);
 CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
 UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 ID3D11VertexShader* shaders[2][2]={};
 for(int prefix=0;prefix<2;++prefix)for(int s=0;s<2;++s){
  const std::string hlsl=std::string(prefix?"float4 AlphaValues;":"")+
   "row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;cbuffer BlendMatricesParameterProvider:register(b"+std::to_string(3+s)+
   "){float4 p[144];};float4 main(uint id:SV_VertexID):SV_Position{float3 q=mul(_WorldMatrix,float4(p[id].xyz,1))+_WorldPositionFractions;"+
   (prefix?"q+=AlphaValues.xyz*.00001;":"")+"return float4(q,1);}";
  auto* code=HullCompile(hlsl.c_str(),"vs_5_0");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[prefix][s])),"220 original pistol attachment shader");
  shaders[prefix][s]->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
 }
 auto close=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>5e-5)return false;return true;};
 Mat4 relative=id;relative.m[3]=.02;relative.m[7]=.15;relative.m[11]=.08;
 for(int prefix=0;prefix<2;++prefix)for(int s=0;s<2;++s){
  g_weaponMechanicProfile=20;g_pistolReload={};auto& p=g_pistolReload;p.enabled=p.magazinePresent=true;g_pistolAttachProfile220=-1;
  for(int pose=0;pose<5;++pose){
   g_weaponMechanicFrame=++g_frame;g_handBody[0][3]=.2f+pose*.03f;
   const Mat4 native=TurnAndMove({0,0,1},Unit({.1+pose*.15,.3-pose*.08,1}),{},{.05*pose,-.02*pose,.1});
   Mat4 target={},grip={};Check(ControllerCurrentRoot124(target,grip),"220 current pistol controller target");
   if(!pose){p.rootBody=target;p.rootNative=native;p.rootFrame=p.rootNativeFrame=g_frame;p.rootContext=context;p.rootEpoch=g_armPoseEpoch;}
   float rows[88]={};const int off=prefix*4;if(prefix)memcpy(rows,alpha,16);
   rows[off]=rows[off+5]=rows[off+10]=1;rows[off+3]=4;rows[off+7]=-6;rows[off+11]=6;rows[off+12]=.13f;rows[off+13]=.69f;rows[off+14]=.21f;
   const Mat4 sourceWorld=HullWorld(rows+off);Mat4 inverseWorld={};Check(Invert(sourceWorld,inverseWorld),"220 source sight world invertible");
   const Mat4 nativeAttachment=Multiply(native,relative),sourceBone=Multiply(inverseWorld,nativeAttachment);
   float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,sourceBone);
   globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);palette->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);context->UpdateSubresource(palette,0,nullptr,raw,0,0);
   ID3D11Buffer* cb[5]={globals,cameraBuffer,nullptr,s==0?palette:clip,s==1?palette:clip};context->VSSetConstantBuffers(0,5,cb);context->VSSetShader(shaders[prefix][s],nullptr,0);
   UINT outputSlot=99;auto* output=BuildWeaponPalette(context,168,3030,&outputSlot);Check(output&&outputSlot==UINT(3+s),"220 actual skinned pistol sight upload consumes reflected palette");
   float result[576]={};Read(device,context,output,result);const Mat4 displayed=Multiply(sourceWorld,BoneMatrix(result,0));
   Check(close(displayed,Multiply(target,relative)),"220 uploaded pistol sight remains mounted across bone animation, draw-before-FRAME, shifted globals and split position");
   auto* learned=PistolAttachSlot(48520,168,3030);Check(learned&&learned->valid&&close(learned->rel,relative),"220 cached pistol mounting offset includes the actual bone0 transform");
   float original[576]={};Read(device,context,palette,original);Compare(original,raw,576,"220 source sight palette preserved");
  }
 }
 g_weaponMechanicProfile=1;auto* other=PistolAttachSlot(48520,168,3030);Check(other&&!other->valid,"220 6P9 cannot inherit Kimber optic mounting offset");
 g_weaponMechanicProfile=20;for(UINT i=0;i<32;++i)Check(PistolAttachSlot(10560,i*3,3)!=nullptr,"220 attachment cache retains more than eight material/optic keys");
 Check(!PistolAttachSlot(10560,999,3),"220 full attachment cache remains bounded");
 memcpy(g_pistolAttach,oldSlots,sizeof(oldSlots));g_pistolAttachProfile220=oldAttachmentProfile;memcpy(g_magazineStates,oldStates,sizeof(oldStates));g_weaponMechanicProfile=oldProfile;
 context->ClearState();for(auto& pair:shaders)for(auto*& shader:pair)SafeRelease(shader);for(auto* b:{vb,ib,globals,cameraBuffer,palette,clip})b->Release();g_weaponPaletteCb->Release();g_weaponPaletteCb=oldOutput;MuzzleDiscardStage58();
 puts("PASS: attachments220 actual pistol optic GPU palettes follow animated native bone0 and current controller before FRAME, b3/b4 and shifted/split world, cache capacity and pistol-model isolation");
}
void TestEarlyRifleAttachment220(ID3D11Device* device,ID3D11DeviceContext* context)
{
 context->ClearState();g_dev=device;g_ctx=context;g_oUpdateSubresource=Update;g_oCreateBuffer=PistolFixtureCreate;
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=g_flatViewActive=g_twoHand=g_aimActive=false;
 g_machete.held=false;g_pistolMenu.open=false;g_r700={};g_frameGunValid=true;g_frameGunMove=HandIdentity();
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);const Mat4 id=HandIdentity();++g_armPoseEpoch;g_freeRightEpoch=g_armPoseEpoch;g_freeRightFromController=id;
 for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=g_handRef[side][i]=float(id.m[i]);
 g_handBody[0][3]=.2f;g_handBody[0][7]=-.2f;g_handBody[0][11]=.4f;memcpy(g_handRight,g_handBody[0],64);memcpy(g_handLeft,g_handBody[1],64);
 auto* vb=Buffer(device,10560,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,2160,D3D11_BIND_INDEX_BUFFER);
 auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* cameraBuffer=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cameraBuffer->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
 UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);ID3D11Buffer* cb[2]={globals,cameraBuffer};context->VSSetConstantBuffers(0,2,cb);
 for(int prefix=0;prefix<2;++prefix){
  const std::string hlsl=std::string(prefix?"float4 AlphaValues;":"")+"row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;float4 main(uint id:SV_VertexID):SV_Position{float3 p=mul(_WorldMatrix,float4(0,0,0,1))+_WorldPositionFractions;"+(prefix?"p+=AlphaValues.xyz*.00001;":"")+"return float4(p,1);}";
  auto* code=HullCompile(hlsl.c_str(),"vs_5_0");ID3D11VertexShader* shader=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)),"220 early rigid attachment shader");shader->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();context->VSSetShader(shader,nullptr,0);
  for(int profile:{7,8,9,14,17,22,31,35,36,41}){
   g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_pistolReload={};auto& p=g_pistolReload;p.enabled=p.magazinePresent=true;
   p.rootFrame=g_frame-1;p.parentValid=false;g_rifleAttachFrame=g_frame-1;g_rifleAttachEpoch=g_armPoseEpoch;g_rifleAttachMove=id;g_rifleAttachMove.m[3]=200;
   Mat4 relative=id;relative.m[7]=.26;relative.m[11]=.08;auto* learned=RifleAttachSlot(10560,0);Check(learned!=nullptr,"220 early rifle attachment cache slot");learned->rel=relative;learned->epoch=g_armPoseEpoch;learned->valid=true;
   const Mat4 native=TurnAndMove({0,0,1},Unit({.3,.2,1}),{},{.1,-.2,.3}),source=Multiply(native,relative);
   float rows[88]={};const int off=prefix*4;if(prefix){rows[0]=.1f;rows[1]=.2f;rows[2]=.3f;rows[3]=.4f;}
   for(int r=0;r<3;++r){for(int col=0;col<3;++col)rows[off+r*4+col]=float(source.m[r*4+col]);rows[off+r*4+3]=float(floor(source.m[r*4+3]));rows[off+12+r]=float(source.m[r*4+3]-rows[off+r*4+3]);}
   globals->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
   Mat4 target={},grip={};Check(ControllerCurrentRoot124(target,grip),"220 controller root exists before ordinary FRAME/parent learning");
   auto* uploaded=BuildWeaponWorld(context,0,1080);Check(uploaded!=nullptr,"220 actual early suppressor world uploaded");float out[88]={};Read(device,context,uploaded,out);
   const Mat4 displayed=HullWorld(out+off),wanted=Multiply(target,relative);
   for(int i=0;i<16;++i)Check(fabs(displayed.m[i]-wanted.m[i])<8e-5,"220 early rifle suppressor uses current controller root, complete split position and shifted layout, never last-frame movement");
   Check(!p.parentValid,"220 fixed early attachment does not learn a native animated holding offset");
   if(prefix)Compare(out,rows,4,"220 early attachment preserves AlphaValues");
  }
  SafeRelease(shader);
 }
 context->ClearState();for(auto* b:{vb,ib,globals,cameraBuffer})b->Release();
 puts("PASS: early attachments220 ten rigid rifle/SMG/LMG families place actual suppressor GPU worlds before FRAME/parent learning, both layouts and split position; stale cached move rejected");
}
void TestWeaponAttachments220(ID3D11Device* device,ID3D11DeviceContext* context)
{
 TestPistolAttachments220(device,context);TestRigidWeaponBindings218(device,context);
 for(int slot:{3,4})for(int profile:{28,21,25,43})TestMp5Root142(device,context,profile,slot,true);
 TestEarlyRifleAttachment220(device,context);
 TestWeaponSightPose51(device,context,false);TestWeaponSightPose51(device,context,true);
}
