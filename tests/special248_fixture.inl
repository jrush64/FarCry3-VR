void TestSpecial248(ID3D11Device* device,ID3D11DeviceContext* context){
 const auto copy249=g_oCopySubresourceRegion;const auto map249=g_oMap;const auto unmap249=g_oUnmap;g_oCopySubresourceRegion=EjectCopyRange;g_oMap=ProbeMap;g_oUnmap=ProbeUnmap;
 const auto profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame,frame=g_frame;
 const auto pistol=g_pistolReload;const auto anchor=g_1911FrameFinal;const auto support=g_repairSupport248;
 const auto controls=fc3pad::Latest();const bool motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand,game=g_inGameplay;
 const auto palmAttachment=g_freeHandAttachment[1];float hands[2][16];memcpy(hands,g_handBody,sizeof(hands));
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);g_inGameplay=g_motionGun=g_handLeftValid=g_handRightValid=true;g_weaponMotionGuard=false;
 const Mat4 id=HandIdentity();auto same=[](const Mat4& a,const Mat4& b,const char* label){for(int i=0;i<16;++i)Check(fabs(a.m[i]-b.m[i])<1e-5,label);};
 g_weaponMechanicProfile=40;g_weaponMechanicFrame=g_frame;g_twoHand=true;g_repairSupport248={};
 for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=float(id.m[i]);
 g_freeHandAttachment[1]={};g_freeHandAttachment[1].epoch=g_armPoseEpoch;g_freeHandAttachment[1].profile=40;g_freeHandAttachment[1].frame=g_frame;g_freeHandAttachment[1].relative=id;
 g_freeHandAttachment[1].relative.m[3]=.6781;g_freeHandAttachment[1].relative.m[7]=-.0519;g_freeHandAttachment[1].relative.m[11]=-1.0387;
 g_handBody[1][7]=.3f;g_handBody[1][11]=.2f;Mat4 root=id;
 Check(RepairSupportTurn248(root,kRepairRightHand247),"248 repair support handled");same(root,id,"248 support acquisition preserves tool angle at off-centre palm");
 root=id;RepairSupportTurn248(root,kRepairRightHand247);same(root,id,"248 repeated support query does not drift");
 g_handBody[1][3]=.15f;root=id;RepairSupportTurn248(root,kRepairRightHand247);
 Check(fabs(root.m[1])+fabs(root.m[2])+fabs(root.m[4])>1e-3,"248 subsequent palm movement steers tool");
 Mat4 fixedRoot249={},fixedGrip249={};Check(ControllerCurrentRoot124(fixedRoot249,fixedGrip249),"249 repair root available");
 float repairBones249[576];for(int b=0;b<48;++b)StoreBone(repairBones249,b,id);Mat4 bound249={};Check(RepairLeftMove249(id,repairBones249,bound249),"249 repair support wrist bound to tool");
 V3 contact249={},freePalm249={};Check(RepairSupportContact247(contact249,freePalm249),"249 repair guide available");Mat4 qi249={};Invert(ArmBodyToCamera(),qi249);
 same(Multiply(qi249,bound249),Multiply(fixedRoot249,kRepairKnobHand250),"250 repair hand retains the complete native knob pose, not the editable pickup point");
 Check(Length(Sub(Apply(kRepairKnobHand250,{-.6781,.0519,1.0387}),{-.0210273899,.1000712365,.0053844098}))<.036,"250 native palm wraps within36mm of authored pressure knob");
 const auto offset250=g_supportGrab214[40].offset[0];g_supportGrab214[40].offset[0]+=.2f;Mat4 edited250={};RepairLeftMove249(id,repairBones249,edited250);same(bound249,edited250,"250 editing pickup collider cannot drag the knob-gripping hand");g_supportGrab214[40].offset[0]=offset250;
 for(int row=0;row<3;++row)for(int col=0;col<3;++col)g_handBody[1][row*4+col]=float((row==0&&col==1)?-1:(row==1&&col==0)?1:(row==2&&col==2)?1:0);
 Mat4 rotated249={},rotatedGrip249={},handRotated249={};ControllerCurrentRoot124(rotated249,rotatedGrip249);same(fixedRoot249,rotated249,"249 rotating support controller alone cannot twist tool");RepairLeftMove249(id,repairBones249,handRotated249);same(bound249,handRotated249,"249 rotating controller cannot loosen displayed support wrist");
 V3 guideRotated249={};RepairSupportContact247(guideRotated249,freePalm249);Check(Length(Sub(contact249,guideRotated249))<1e-8,"249 support guide does not rotate away from tool contact");
 g_twoHand=false;RepairSupportReset248();Check(!g_repairSupport248.valid,"248 release clears support baseline");
 g_twoHand=true;root=id;RepairSupportTurn248(root,kRepairRightHand247);same(root,id,"248 regrip captures new palm without snapping");
 g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;g_twoHand=false;g_1911FrameFinal={};g_pistolReload={};
 auto& p=g_pistolReload;p.enabled=true;p.rackHeld=true;p.rootEpoch=g_armPoseEpoch;p.rootFrame=g_frame;p.rootBody=id;
 Check(RifleChargeHandHeld(),"248 Shadow routes held slide to bound hand");
 Mat4 start={},end={},low={},high={};float f0[204],f1[204],f2[204];
 Check(RifleChargeClipSample(0,start,f0)&&RifleChargeClipSample(1,end,f1),"248 Shadow slide animation available");
 RifleChargeClipSample(-10,low,f2);same(start,low,"248 slide hand cannot move past front stop");
 RifleChargeClipSample(10,high,f2);same(end,high,"248 slide hand cannot move past rear stop");
 Check(Length(Sub(Apply(start,{}),Apply(end,{})))>.02,"248 Shadow wrist follows slide stroke");
 float bones[576];for(int i=0;i<48;++i)StoreBone(bones,i,id);p.slidePull=.05f;Mat4 move={},overshoot={};
 Check(RifleChargeHandMove(id,bones,move),"248 Shadow arm transform generated");
 g_handBody[1][7]+=1.f;Check(RifleChargeHandMove(id,bones,overshoot),"248 Shadow overshoot still bound");same(move,overshoot,"248 raw controller overtravel cannot drag held wrist off slide");
 p.rackHeld=false;Check(!RifleChargeHandMove(id,bones,move),"248 release restores free tracked hand");
 p.rackHeld=true;p.rootFrame=g_frame-2;Check(!RifleChargeHandHeld(),"248 stale weapon root cannot bind hand");
 // Actual deferred replay, with distinct current VS and intentionally corrupt current cb0.
 const auto oldDev=g_dev;const auto oldCtx=g_ctx;const auto revolver=g_revolver;auto retained=std::move(g_revolverLoader);g_revolverLoader={};g_dev=device;g_ctx=context;context->ClearState();
 auto& L=g_revolverLoader;auto& part=L.part;
 auto* code=HullCompile("cbuffer Eye:register(b13){float4 eye[4];};float4 main(uint id:SV_VertexID):SV_Position{float2 p=id%3==0?float2(-.5,-.5):id%3==1?float2(0,.5):float2(.5,-.5);return float4(p+eye[0].xy,.5,1);}","vs_5_0");
 ID3D11VertexShader* currentVs=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&part.vs)),"248 cached loader VS");Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&currentVs)),"248 distinct receiver VS");SafeRelease(code);
 unsigned long long hash=123;part.vs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);L.vsHash=hash;hash=456;currentVs->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
 code=HullCompile("float4 main():SV_Target{return float4(0,1,0,1);}","ps_5_0");Check(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&part.ps)),"248 loader PS");SafeRelease(code);
 Check(RevolverLoaderOwnMesh(part),"248 real speedloader geometry created");L.stride=32;part.data.resize(88);part.data[70]=.375f;part.globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);part.mask=~0u;
 D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;Check(SUCCEEDED(device->CreateRasterizerState(&rd,&part.raster)),"248 loader raster");
 Check(SUCCEEDED(device->CreateDeferredContext(0,&L.deferred)),"248 loader deferred context");
 auto* vb=Buffer(device,18032,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,2688,D3D11_BIND_INDEX_BUFFER);auto* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER);auto* camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 float corrupt[88]={};context->UpdateSubresource(globals,0,nullptr,corrupt,0,0);globals->SetPrivateData(kCbCopyGuid,sizeof(corrupt),corrupt);CameraSummary cm={};cm.mainView=1;cm.right[0]=cm.up[1]=cm.forward[2]=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cm),&cm);
 ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);context->VSSetShader(currentVs,nullptr,0);UINT stride=32,offset=80;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 ID3D11Texture2D* color[2]={},*z[2]={};ID3D11RenderTargetView* rtv[2]={};ID3D11DepthStencilView* dsv[2]={};ID3D11Buffer* eyes[2]={g_eyeCb[1][0],g_eyeCb[1][1]};
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=64;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 for(int eye=0;eye<2;++eye){Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&color[eye]))&&SUCCEEDED(device->CreateRenderTargetView(color[eye],nullptr,&rtv[eye])),"248 loader target");auto dd=td;dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dd.BindFlags=D3D11_BIND_DEPTH_STENCIL;Check(SUCCEEDED(device->CreateTexture2D(&dd,nullptr,&z[eye]))&&SUCCEEDED(device->CreateDepthStencilView(z[eye],nullptr,&dsv[eye])),"248 loader depth");g_eyeCb[1][eye]=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);float e[16]={};e[0]=eye?.08f:-.08f;context->UpdateSubresource(g_eyeCb[1][eye],0,nullptr,e,0,0);float black[4]={};context->ClearRenderTargetView(rtv[eye],black);context->ClearDepthStencilView(dsv[eye],D3D11_CLEAR_DEPTH,1,0);}
 rtv[0]->SetPrivateDataInterface(kTwinGuid,rtv[1]);dsv[0]->SetPrivateDataInterface(kTwinGuid,dsv[1]);context->OMSetRenderTargets(1,&rtv[0],dsv[0]);D3D11_VIEWPORT vp={0,0,64,64,0,1};context->RSSetViewports(1,&vp);part.targets=1;part.formats[0]=td.Format;part.depthFormat=DXGI_FORMAT_D24_UNORM_S8_UINT;part.ready=true;
 g_weaponMechanicProfile=42;g_weaponMechanicFrame=g_frame;g_revolver={};g_revolver.carrying=true;g_revolver.leftFrame=g_frame;g_revolver.leftWrist=id;DrawCall call={};call.b=6;call.a=1338;
 L.vsHash=456;ReplayRevolverLoader(context,call);Check(L.frame==g_frame,"248 Cannon barrel triggers its cached material pass");
 float saved[88];Read(device,context,part.globals,saved);Check(saved[70]==.375f,"248 replay retains owned material data despite unrelated current cb0");
 ID3D11VertexShader* restored=nullptr;context->VSGetShader(&restored,nullptr,nullptr);Check(restored==currentVs,"248 loader replay restores current VS");SafeRelease(restored);
 ID3D11Buffer* restoredCb=nullptr;context->VSGetConstantBuffers(0,1,&restoredCb);Check(restoredCb==globals,"248 loader replay restores current globals");SafeRelease(restoredCb);
 double centers[2]={};for(int eye=0;eye<2;++eye){auto sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"248 loader readback");context->CopyResource(stage,color[eye]);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"248 loader map");int pixels=0;for(int y=0;y<64;++y)for(int x=0;x<64;++x){const auto* pixel=static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch+x*4;if(pixel[1]>200){++pixels;centers[eye]+=x;}}context->Unmap(stage,0);SafeRelease(stage);Check(pixels>100,"248 speedloader replay produces pixels in both eyes");centers[eye]/=pixels;}
 Check(centers[1]-centers[0]>3,"248 loader replay uses distinct eye constants");
 // A second native stage needs its own cache and extra VS/PS constants.
 RevolverLoaderVisual second249={};auto* clip249=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);auto* extra249=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);float extraData249[4]={1,0,0,1};context->UpdateSubresource(clip249,0,nullptr,extraData249,0,0);context->UpdateSubresource(extra249,0,nullptr,extraData249,0,0);
 auto* secondCode249=HullCompile("cbuffer Extra:register(b3){float4 val;};float4 main(uint id:SV_VertexID):SV_Position{float2 p=id%3==0?float2(-.5,-.5):id%3==1?float2(0,.5):float2(.5,-.5);return float4(p,.4,val.w);}","vs_5_0");ID3D11VertexShader* secondVs249=nullptr;Check(SUCCEEDED(device->CreateVertexShader(secondCode249->GetBufferPointer(),secondCode249->GetBufferSize(),nullptr,&secondVs249)),"249 second loader VS");D3D11_INPUT_ELEMENT_DESC element249={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};ID3D11InputLayout* layout249=nullptr;Check(SUCCEEDED(device->CreateInputLayout(&element249,1,secondCode249->GetBufferPointer(),secondCode249->GetBufferSize(),&layout249)),"249 loader input layout");SafeRelease(secondCode249);hash=789;secondVs249->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash);
 secondCode249=HullCompile("cbuffer MetalExtra:register(b4){float4 val;};float4 main():SV_Target{return val;}","ps_5_0");ID3D11PixelShader* secondPs249=nullptr;Check(SUCCEEDED(device->CreatePixelShader(secondCode249->GetBufferPointer(),secondCode249->GetBufferSize(),nullptr,&secondPs249)),"249 loader color shader");SafeRelease(secondCode249);
 context->IASetInputLayout(layout249);context->RSSetState(part.raster);context->VSSetShader(secondVs249,nullptr,0);context->PSSetShader(secondPs249,nullptr,0);context->VSSetConstantBuffers(2,1,&clip249);context->VSSetConstantBuffers(3,1,&extra249);context->PSSetConstantBuffers(0,1,&globals);context->PSSetConstantBuffers(4,1,&extra249);
 ObserveRevolverLoaderPass249(context,call,second249);Check(second249.part.ready&&second249.vertexExtra249[1]&&second249.part.psCb[4],"249 each material pass captures additional vertex and pixel constant buffers");
 second249.failed=true;ReplayRevolverLoaderPass249(context,call,second249);Check(second249.frame!=g_frame,"249 incomplete material cache cannot render");second249.failed=false;ReplayRevolverLoaderPass249(context,call,second249);Check(second249.frame==g_frame&&L.frame==g_frame,"249 second color stage replays in frame already used by first stage");
 auto stageDesc249=td;stageDesc249.Usage=D3D11_USAGE_STAGING;stageDesc249.BindFlags=0;stageDesc249.CPUAccessFlags=D3D11_CPU_ACCESS_READ;for(int eye=0;eye<2;++eye){ID3D11Texture2D* stage=nullptr;device->CreateTexture2D(&stageDesc249,nullptr,&stage);context->CopyResource(stage,color[eye]);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&m)),"249 second-stage pixels readback");const auto* pixel=static_cast<unsigned char*>(m.pData)+32*m.RowPitch+32*4;Check(pixel[0]>200&&pixel[1]<20,"249 actual second-stage color reaches both eye targets through extra PS constant slot");context->Unmap(stage,0);SafeRelease(stage);}
 ClearBowArrowPart(second249.part);SafeRelease(second249.deferred);for(auto*& b:second249.vertexExtra249)SafeRelease(b);SafeRelease(secondVs249);SafeRelease(secondPs249);SafeRelease(layout249);SafeRelease(clip249);SafeRelease(extra249);context->VSSetShader(currentVs,nullptr,0);
 ++g_frame;g_weaponMechanicFrame=g_frame;g_revolver.leftFrame=g_frame;cm.mainView=0;camera->SetPrivateData(kCameraSummaryGuid,sizeof(cm),&cm);ReplayRevolverLoader(context,call);Check(L.frame==g_frame-1,"248 non-main camera cannot replay loader");
 context->ClearState();ClearBowArrowPart(part);SafeRelease(L.deferred);g_revolverLoader=std::move(retained);for(int eye=0;eye<2;++eye){SafeRelease(g_eyeCb[1][eye]);g_eyeCb[1][eye]=eyes[eye];SafeRelease(rtv[eye]);SafeRelease(dsv[eye]);SafeRelease(color[eye]);SafeRelease(z[eye]);}SafeRelease(currentVs);SafeRelease(vb);SafeRelease(ib);SafeRelease(globals);SafeRelease(camera);
 g_oCopySubresourceRegion=copy249;g_oMap=map249;g_oUnmap=unmap249;g_dev=oldDev;g_ctx=oldCtx;g_revolver=revolver;g_pistolReload=pistol;g_1911FrameFinal=anchor;g_repairSupport248=support;g_freeHandAttachment[1]=palmAttachment;memcpy(g_handBody,hands,sizeof(hands));g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_frame=frame;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;g_inGameplay=game;fc3pad::Publish(controls);
 puts("PASS:248 repair support acquisition/regrip without snap, relative steering; Shadow bounded native pistol grasp and release; Cannon material-pass replay with owned material constants and stereo GPU pixels");
}
