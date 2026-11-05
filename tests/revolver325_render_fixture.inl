void TestRevolverMirror325(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb,float physical[2][16]){
 auto equal=[](const Mat4& a,const Mat4& b){for(int i=0;i<16;++i)if(fabs(a.m[i]-b.m[i])>3e-4)return false;return true;};
 const Mat4 mirror=SidearmMirror315();
 auto reflect=[&](const Mat4& m){return Multiply(mirror,Multiply(m,mirror));};
 float raw[2][576]={};for(int s=0;s<2;++s){for(int b=0;b<48;++b)StoreBone(raw[s],b,HandIdentity());memcpy(raw[s],kNeutralArmView[s],sizeof(kNeutralArmView[s]));}
 ID3D11VertexShader* previous=nullptr;context->VSGetShader(&previous,nullptr,nullptr);
 ID3D11Buffer* oldCb[5]={};context->VSGetConstantBuffers(0,5,oldCb);
 ID3D11Buffer* oldVb=nullptr;UINT oldStride=0,oldOffset=0;context->IAGetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);
 ID3D11Buffer* oldIb=nullptr;DXGI_FORMAT oldFormat={};UINT oldIbOffset=0;context->IAGetIndexBuffer(&oldIb,&oldFormat,&oldIbOffset);
 auto* gunVb=Buffer(device,186104,D3D11_BIND_VERTEX_BUFFER);auto* gunIb=Buffer(device,31002,D3D11_BIND_INDEX_BUFFER);
 auto* source=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);auto* previousOutput=g_weaponPaletteCb;g_weaponPaletteCb=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
 ID3D11VertexShader* shaders[2]={};for(int pass=0;pass<2;++pass){char hlsl[240];sprintf_s(hlsl,"cbuffer BlendMatricesParameterProvider:register(b%d){float4 p[144];};float4 main(uint id:SV_VertexID):SV_Position{return float4(p[id].w,p[id+1].w,0,1);}",pass+3);ID3DBlob* code=nullptr;Check(SUCCEEDED(D3DCompile(hlsl,strlen(hlsl),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&code,nullptr)),"325 compile palette shader");Check(code&&SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[pass])),"325 native palette shader fixture");shaders[pass]->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();}
 for(int owner:{0,1}){
  g_weaponMechanicProfile=42;g_weaponMechanicFrame=++g_frame;g_revolver={};g_pistolReload={};g_reload={};g_reloadAlignment=false;g_pistolMenu.open=false;g_twoHand=false;g_revolver.probing=true;g_revolver.probeAt=GetTickCount64();SetRenderNative319(0);
  fc3pad::SetGameplayControls(true);fc3pad::SetGripOnlyAmmo(true);fc3pad::SetGripContext(true,true);fc3pad::SetSwapBX(false);
  for(int h=0;h<2;++h){memset(physical[h],0,64);physical[h][0]=physical[h][5]=physical[h][10]=physical[h][15]=1;physical[h][3]=h?-.3f:.3f;physical[h][7]=-.2f;physical[h][11]=.5f;}
  SetRenderHands303(owner,false,g_armPoseEpoch);SetRenderReceiver309(-1);PublishSidearmControls315(0,0);
  auto build=[&](int side){fc3stereo::SetHandPoses(physical[0],physical[1]);cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw[side]),raw[side]);Check(BuildArmPalette(context,side)!=nullptr,"325 anatomical hand palette");};build(0);build(1);
  Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"325 held revolver root");auto& r=g_revolver;r.frameBody=root;r.frameFrame=g_frame;r.angle=100;
  const Mat4 nativeCrane=RevolverCanonicalCrane325(100),crane=RevolverCrane(100);
  Check(equal(crane,owner?reflect(nativeCrane):nativeCrane),"325 crane mirrors only for left owner");
  const auto cylinder=Apply(crane,{kRevolverCylinderAxis[0],.05,kRevolverCylinderAxis[2]});Check(owner?cylinder.x>.02:cylinder.x<-.02,"325 open cylinder lies on free-hand side of gun");
  // Compare both guide transforms against independent reflection of canonical boxes.
  for(int b=0;b<2;++b){SetRenderHands303(0,false,g_armPoseEpoch);const auto canonical=RevolverBoxFrame(b);SetRenderHands303(owner,false,g_armPoseEpoch);Check(equal(RevolverBoxFrame(b),owner?reflect(canonical):canonical),"325 guide rotations and offsets mirror together");}
  fc3stereo::SetHandPoses(physical[0],physical[1]);
  // Actual shader-selected b3 and b4 weapon uploads: base/hammer remain pinned;
  // only crane/cylinder bones articulate on the opposite side.
  for(int pass=0;pass<2;++pass){UINT stride=40,offset=224;context->IASetVertexBuffers(0,1,&gunVb,&stride,&offset);context->IASetIndexBuffer(gunIb,DXGI_FORMAT_R16_UINT,0);float native[576]={};for(int b=0;b<48;++b)StoreBone(native,b,HandIdentity());source->SetPrivateData(kCbCopyGuid,sizeof(native),native);context->UpdateSubresource(source,0,nullptr,native,0,0);ID3D11Buffer* buffers[5]={cb[0],cb[1],nullptr,pass?nullptr:source,pass?source:nullptr};context->VSSetConstantBuffers(0,5,buffers);context->VSSetShader(shaders[pass],nullptr,0);const BOOL staleMirror338=TRUE;g_weaponPaletteCb->SetPrivateData(kA2000Mirror337,sizeof(staleMirror338),&staleMirror338);UINT slot=99;auto* output=BuildRevolverPalette217(context,12,9432,slot);Check(output&&slot==UINT(pass+3),"325 actual weapon palette routes b3/b4");BOOL winding338=TRUE;UINT windingSize338=sizeof(winding338);Check(SUCCEEDED(output->GetPrivateData(kA2000Mirror337,&windingSize338,&winding338))&&!winding338,"338 real .44 palette clears previous mirrored-rifle winding tag");float uploaded[576];Read(device,context,output,uploaded);const auto target=Multiply(ArmBodyToCamera(),root);Check(equal(BoneMatrix(uploaded,0),target)&&equal(BoneMatrix(uploaded,1),target),"325 receiver and hammer remain unchanged by reload reflection");Check(equal(BoneMatrix(uploaded,2),Multiply(target,crane))&&equal(BoneMatrix(uploaded,3),Multiply(target,crane)),"325 actual GPU crane and cylinder upload share mirrored articulation");float untouched[576];Read(device,context,source,untouched);Compare(untouched,native,576,"325 native source palette preserved");}
  context->IASetVertexBuffers(0,1,&oldVb,&oldStride,&oldOffset);context->IASetIndexBuffer(oldIb,oldFormat,oldIbOffset);context->VSSetConstantBuffers(0,5,oldCb);context->VSSetShader(previous,nullptr,0);
  r.open=true;r.empty=r.carrying=true;r.angle=100;r.frameBody=root;r.frameFrame=g_frame;
  for(double seconds:{.73,.95,1.19,1.34}){
   r.clip=0;r.clipFirst=0;r.clipAt=GetTickCount64()-ULONGLONG(seconds*1000);r.poseFrame=-100000;r.seatFired=r.pending=false;Revolver::Pose canonical={};RevolverSampleClip(0,RevolverClipSeconds(),canonical);const auto& pose=RevolverClipPose();
   Check(equal(pose.hand,owner?reflect(canonical.hand):canonical.hand)&&equal(pose.crane,owner?reflect(canonical.crane):canonical.crane)&&equal(pose.cart,owner?reflect(canonical.cart):canonical.cart)&&equal(pose.loader,owner?reflect(canonical.loader):canonical.loader),"325 authored hand,cylinder,cartridges and loader share one reflection");
   Check(pose.loaderOn&&equal(RevolverLoaderBody(),Multiply(root,pose.loader)),"325 loader remains visible and attached through seating/withdrawal");
   Check(fabs(RevolverCraneDegrees(pose.crane)-(-atan2(canonical.crane.m[2],canonical.crane.m[0])*180/3.14159265358979))<.001,"325 reflected clip preserves positive cylinder angle");
   build(1-owner);float skin[576];Read(device,context,g_armPaletteCb[1-owner],skin);Check(equal(BoneMatrix(skin,2),Multiply(ArmBodyToCamera(),Multiply(root,pose.hand))),"325 operating wrist GPU pose follows cylinder on its new side");
  }
  // A same-frame ownership change must invalidate the pose reflection cache.
  // Freeze animation time while checking only same-frame ownership invalidation.
  r.clipPauseAt223=GetTickCount64();r.clipAt=r.clipPauseAt223-1340;r.poseFrame=-100000;
  const auto leftOrRight=RevolverClipPose().loader;SetRenderHands303(1-owner,false,g_armPoseEpoch);Check(equal(RevolverClipPose().loader,reflect(leftOrRight)),"325 pose cache tracks ownership even within one frame");SetRenderHands303(owner,false,g_armPoseEpoch);
  r.clipPauseAt223=0;r.clip=-1;r.carrying=false;r.open=false;r.angle=0;r.grab=false;build(0);build(1);r.frameBody=root;r.frameFrame=g_frame;
  const int free=1-owner;V3 palm={};Check(RevolverPalmBody(palm),"325 physical operating contact available");const auto move=Sub(Apply(root,RevolverBoxCentre(1)),palm);physical[free][3]+=float(move.x);physical[free][7]+=float(move.y);physical[free][11]+=float(move.z);build(free);Publish1911Transfer307();PublishSidearmControls315(0,0);TickRevolver(true);PublishSidearmControls315(0,0,free==0?1.f:0.f,free==1?1.f:0.f);TickRevolver(true);Check(r.grab&&!r.open,"325 free hand acquires closed-cylinder guide");for(int row=0;row<3;++row)physical[free][row*4+3]+=float(root.m[row*4]*.035*(owner?1:-1));build(free);TickRevolver(true);Check(r.open,"325 outward pull opens toward free hand in either owner");PublishSidearmControls315(0,0);
  // Optional gun flick closes inward, with the free grip released. Its sign
  // follows the cylinder side; the tracked UP-close is covered by323's suite.
  r.grab=false;r.open=true;r.angle=100;PublishSidearmControls315(0,0);r.rightPrevAt=GetTickCount64()-20;r.rightPrev={g_handBody[0][3],g_handBody[0][7],g_handBody[0][11]};
  for(int row=0;row<3;++row)physical[owner][row*4+3]+=float(root.m[row*4]*.05*(owner?-1:1));fc3stereo::SetHandPoses(physical[0],physical[1]);TickRevolver(true);Check(r.clip==1,"325 free-grip-released gun flick closes inward in either owner");
  printf("PASS:325 owner%d free-side cylinder,guides,actual b3/b4 weapon and operating-hand GPU palettes,clip alignment,loader visibility,outward opening,inward flick\n",owner);
 }
 for(auto* shader:shaders)shader->Release();source->Release();gunVb->Release();gunIb->Release();SafeRelease(g_weaponPaletteCb);g_weaponPaletteCb=previousOutput;for(auto* buffer:oldCb)SafeRelease(buffer);SafeRelease(oldVb);SafeRelease(oldIb);SafeRelease(previous);
}
