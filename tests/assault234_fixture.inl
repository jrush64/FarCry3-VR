namespace assault234 {
UINT triangles=0,hinted=0;ID3D11Buffer* originalMaterial=nullptr;ID3D11ShaderResourceView* black=nullptr;
void STDMETHODCALLTYPE Draw(ID3D11DeviceContext* c,UINT count,UINT start,INT base){
 ID3D11Buffer* material=nullptr;ID3D11ShaderResourceView* texture=nullptr;c->PSGetConstantBuffers(0,1,&material);c->PSGetShaderResources(2,1,&texture);
 const bool on=material==g_1911TintCb49;
 Check(!base&&(on||material==originalMaterial),"234 native draw/material retained");
 if(on){const auto& r=CurrentRifle();Check(g_weaponMechanicProfile==25?(start==11034&&count==984):(RiflePart(r.part[0],start,count)||(g_weaponMechanicProfile==23&&RiflePart(r.part[1],start,count))),"234 tint restricted to exact handle geometry");Check(texture==g_actionWhite231,"234 dark handle texture cannot conceal action colour");hinted+=count;}
 else Check(texture==black,"234 receiver retains its native diffuse texture");
 triangles+=count;SafeRelease(material);SafeRelease(texture);c->DrawIndexed(count,start,base);
}
void Test(ID3D11Device* device,ID3D11DeviceContext* context){
 const auto controls=fc3pad::Latest();const auto menu=g_pistolMenu;const LONG profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame,frame=g_frame;
 PistolReloadState states[20];memcpy(states,g_magazineStates,sizeof(states));SupportGrab214 settings[44];memcpy(settings,g_supportGrab214,sizeof(settings));
 const auto boltState=g_r700;const auto reload=g_reload;const auto manual=g_manualReload;const auto neutral=g_controllerNeutral124;const auto attachment=g_freeRightFromController;const UINT rightEpoch=g_freeRightEpoch;
 const bool motion=g_motionGun,guard=g_weaponMotionGuard,left=g_handLeftValid,right=g_handRightValid,two=g_twoHand,eyes=g_eyeViewsSet,guides=g_grabGuides,hint=g_reloadActionGuide227,loaded=g_1911TintLoaded49;
 const auto palmDelta=g_supportPalmDelta;const auto palmEpoch=g_supportPalmEpoch;const auto palmProfile=g_supportPalmProfile;const auto palmFrame=g_supportPalmFrame;
 const auto guideProfile=g_nativeWeaponProfile50;const UINT packets=g_nativeGuidePacketCount50;NativeGuideEye48 nativeEyes[2];memcpy(nativeEyes,g_nativeWeaponEye50,sizeof(nativeEyes));
 const auto joints0=g_armJoints[0],joints1=g_armJoints[1];float hands[2][16],viewHands[2][16];memcpy(hands,g_handBody,sizeof(hands));memcpy(viewHands[0],g_handRight,64);memcpy(viewHands[1],g_handLeft,64);
 const int colour=g_reloadActionColour227;const float strength=g_smgHintStrength230;auto* ctx=g_ctx;auto* dev=g_dev;
 g_ctx=context;g_dev=device;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=g_grabGuides=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;g_reload={};g_manualReload={};
 g_reloadActionGuide227=g_1911TintLoaded49=true;g_reloadActionColour227=0;g_smgHintStrength230=1;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 for(int side=0;side<2;++side){g_armJoints[side]=ArmJoints{{side?.2:-.2,-.15,-.2},{side?.35:-.35,-.3,.05},{side?.35:-.35,-.2,.4},true};}
 const Mat4 id=HandIdentity();
 for(int weapon:{8,41,14,31,25,17,7,28,9,21,35,36,22,43}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=true;
  auto& setting=g_supportGrab214[weapon];setting.offset[0]=.025f;setting.offset[1]=-.03f;setting.offset[2]=.04f;setting.sizeScale[0]=.4f;setting.sizeScale[1]=1.7f;setting.sizeScale[2]=2.4f;
  for(int step=0;step<6;++step){
   Mat4 body=id;const double turn=.13*step;body.m[0]=body.m[10]=cos(turn);body.m[2]=sin(turn);body.m[8]=-sin(turn);body.m[3]=.1+step*.01;body.m[7]=-.2;body.m[11]=.3;
   for(int side=0;side<2;++side){Mat4 hand=body;if(side)hand.m[3]-=.35;const Mat4 view=Multiply(ArmBodyToCamera(),hand);for(int k=0;k<16;++k){g_handBody[side][k]=float(hand.m[k]);(side?g_handLeft:g_handRight)[k]=float(view.m[k]);}}
   g_twoHand=step%2!=0;Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"234 assault root available");
   V3 expected=Apply(Multiply(root,*CurrentRifle().support),{-.6781,.0519,1.0387});SupportGrabAdjustPoint214(expected);
   g_supportPalmEpoch=g_armPoseEpoch;g_supportPalmProfile=weapon;g_supportPalmFrame=g_frame;g_supportPalmDelta={.3,-.4,.2};
   V3 normal={},preview={},palm={};Mat4 normalBox={},previewBox={};
   g_pistolMenu.open=false;Check(GrabGizmoPoints({8,9,10},normal,palm,&id)&&SupportGrabBox214(id,normal,normalBox),"234 gameplay support box");
   g_pistolMenu.open=true;g_supportPalmDelta={-.5,.2,-.3};Check(GrabGizmoPoints({-8,-9,-10},preview,palm,&body)&&SupportGrabBox214(body,preview,previewBox),"234 Insert support box");
   Check(Length(Sub(normal,preview))<1e-7&&Length(Sub(normal,expected))<1e-7,"234 all six assault rifles share calibrated gameplay/Insert point despite native palm/source changes");
   for(int k=0;k<16;++k)Check(fabs(normalBox.m[k]-previewBox.m[k])<1e-7,"234 OBB rotation/position identical in gameplay and Insert");
   Check(SupportGrabOutside214(weapon,normalBox,normal)<1e-8&&SupportGrabOutside214(weapon,previewBox,Add(normal,{10,10,10}))>1,"234 actual collision geometry agrees with preview");
   g_pistolMenu.open=false;
  }
  g_twoHand=false;g_nativeWeaponProfile50=weapon;
  for(int eye=0;eye<2;++eye){auto& e=g_nativeWeaponEye50[eye];e={};e.valid=e.anchor.valid=true;e.frame=e.anchor.frame=g_frame;e.anchor.epoch=g_armPoseEpoch;e.anchor.body=e.anchor.world=id;e.anchor.world.m[11]=2;for(int k=0;k<16;++k)e.eye[k]=e.camera[k]=(k%5==0)?1.f:0.f;e.eye[3]=eye?.03f:-.03f;}
  g_pistolMenu.open=false;g_nativeGuidePacketCount50=0;NativeHoldingGuides50();Check(g_nativeGuidePacketCount50==2&&g_nativeGuidePackets50[0].holding&&g_nativeGuidePackets50[0].count==24,"234 gameplay uses actual stereo holding-box renderer");
  const auto rendered=g_nativeGuidePackets50[0];g_pistolMenu.open=true;g_supportPalmDelta={5,6,7};g_nativeGuidePacketCount50=0;NativeHoldingGuides50();Check(g_nativeGuidePacketCount50==2,"234 Insert uses actual holding-box renderer");
  Check(!memcmp(rendered.clip,g_nativeGuidePackets50[0].clip,sizeof(rendered.clip)),"234 both-eye holding box projects identically before/after opening Insert");g_pistolMenu.open=false;
 }
 context->ClearState();
 auto* code=Native1911Compile48("float4 main(uint id:SV_VertexID):SV_Position{float2 p=id%3==0?float2(-.3,-.3):id%3==1?float2(0,.3):float2(.3,-.3);return float4(p+float2(id>=3?.45:-.45,0),.5,1);}","vs_5_0");ID3D11VertexShader* vs=nullptr;device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs);code->Release();
 code=Native1911Compile48("cbuffer Metal:register(b0){float3 _BaseDiffuseColor;float unused;};Texture2D BaseDiffuseTexture:register(t2);SamplerState S:register(s2);float4 main():SV_Target{return float4(_BaseDiffuseColor*BaseDiffuseTexture.Sample(S,float2(.5,.5)).rgb,1);}","ps_5_0");ID3D11PixelShader* ps=nullptr;device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&ps);ps->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());code->Release();
 originalMaterial=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);const float material[4]={.4f,.4f,.4f,.75f};originalMaterial->SetPrivateData(kCbCopyGuid,sizeof(material),material);context->UpdateSubresource(originalMaterial,0,nullptr,material,0,0);
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;UINT pixel=0xff000000;D3D11_SUBRESOURCE_DATA data={&pixel,4,0};ID3D11Texture2D* tex=nullptr;device->CreateTexture2D(&td,&data,&tex);device->CreateShaderResourceView(tex,nullptr,&black);
 D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState* sampler=nullptr;device->CreateSamplerState(&sd,&sampler);
 td.Width=td.Height=64;td.BindFlags=D3D11_BIND_RENDER_TARGET;ID3D11Texture2D* target=nullptr;ID3D11RenderTargetView* rtv=nullptr;device->CreateTexture2D(&td,nullptr,&target);device->CreateRenderTargetView(target,nullptr,&rtv);
 D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;ID3D11RasterizerState* raster=nullptr;device->CreateRasterizerState(&rd,&raster);
 context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->PSSetConstantBuffers(0,1,&originalMaterial);context->PSSetShaderResources(2,1,&black);context->PSSetSamplers(2,1,&sampler);context->RSSetState(raster);context->OMSetRenderTargets(1,&rtv,nullptr);D3D11_VIEWPORT vp={0,0,64,64,0,1};context->RSSetViewports(1,&vp);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 const auto draw=g_oDrawIndexed;g_oDrawIndexed=Draw;
 for(int weapon:{14,31,25,17,35,36,22,2,23}){
  g_weaponMechanicProfile=weapon;g_weaponMechanicFrame=++g_frame;g_pistolReload={};auto& p=g_pistolReload;p.enabled=p.magazinePresent=p.needsRack=true;p.chamberReady=false;
  g_r700=R700{};g_r700.chambered=false;const bool bolt=weapon==2||weapon==23;
  const auto& r=CurrentRifle();auto* vb=Buffer(device,r.vb,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,r.ib,D3D11_BIND_INDEX_BUFFER);UINT stride=r.stride,offset=r.offset;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
  DrawCall call={};call.b=weapon==25?11028:r.part[0][0];call.a=weapon==25?13881:r.part[0][1];std::vector<USHORT> indices(r.ib/2);for(UINT n=0;n<indices.size();++n)indices[n]=USHORT(n%3+((weapon==25?(n>=11034&&n<12018):(n>=call.b&&n<call.b+call.a))?3:0));context->UpdateSubresource(ib,0,nullptr,indices.data(),0,0);
  Check(ReloadActionRequired227()==(bolt?7:3),"234 required assault charging action publishes guide");
  for(int eye=0;eye<2;++eye){const float clear[4]={};context->ClearRenderTargetView(rtv,clear);triangles=hinted=0;auto* prior=Begin1911Tint49(context,call,1);Check(!prior&&IssueMp5Hint229(context,call),"234 exact charging draw reaches scoped cue");End1911Tint49(context,prior);Check(triangles==call.a&&hinted==(weapon==25?984:call.a),"234 both eyes draw every native triangle once, handle only");
   auto stageDesc=td;stageDesc.BindFlags=0;stageDesc.Usage=D3D11_USAGE_STAGING;stageDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;device->CreateTexture2D(&stageDesc,nullptr,&stage);context->CopyResource(stage,target);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"234 cue pixel readback");UINT found=0,wrong=0;for(int y=0;y<64;++y)for(int x=0;x<64;++x){const BYTE* v=static_cast<const BYTE*>(mapped.pData)+y*mapped.RowPitch+x*4;if(v[0]>100&&v[1]>80){++found;if(x<32)++wrong;}}context->Unmap(stage,0);stage->Release();Check(found>80&&!wrong,"234 selected handle produces visible yellow pixels; receiver stays black");}
  if(weapon==23){DrawCall body=call;body.b=r.part[1][0];body.a=r.part[1][1];triangles=hinted=0;auto* prior=Begin1911Tint49(context,body,1);Check(!prior&&IssueMp5Hint229(context,body),"236 Z93 bolt body receives same required-action cue as handle");End1911Tint49(context,prior);Check(triangles==body.a&&hinted==body.a,"236 Z93 bolt body drawn exactly once");}
  for(int state=0;state<7;++state){p.held=state==0;p.pending=state==1;p.cycle=state==2;p.rackReturning=state==3;p.needsRack=state!=4;g_pistolMenu.open=state==5;p.enabled=state!=6;g_r700.chambered=state==4;g_r700.native=state==6;Check(!ReloadActionRequired227(),"234 cue absent for magazine/reload/return/ready/Insert/Simple states");}
  p.held=p.pending=p.cycle=p.rackReturning=g_pistolMenu.open=false;p.enabled=p.needsRack=true;g_r700.chambered=g_r700.native=false;
  DrawCall receiver=call;receiver.b=r.frame[0][0];receiver.a=r.frame[0][1];Check(!Begin1911Tint49(context,receiver,1)&&!IssueMp5Hint229(context,receiver),"234 rigid/skin receiver excluded before cue");Check(!Begin1911Tint49(context,call,0)&&!IssueMp5Hint229(context,call),"234 world-camera draw excluded");
  UINT wrongStride=32,wrongOffset=0;auto* wrongVb=Buffer(device,4096,D3D11_BIND_VERTEX_BUFFER);context->IASetVertexBuffers(0,1,&wrongVb,&wrongStride,&wrongOffset);Check(!Begin1911Tint49(context,call,1)&&!IssueMp5Hint229(context,call),"234 coincident charging ranges with unrelated mesh rejected");wrongVb->Release();vb->Release();ib->Release();
 }
 g_oDrawIndexed=draw;context->ClearState();SafeRelease(raster);SafeRelease(rtv);SafeRelease(target);SafeRelease(sampler);SafeRelease(black);SafeRelease(tex);SafeRelease(originalMaterial);SafeRelease(vs);SafeRelease(ps);g_mp5HintContext229=nullptr;
 g_r700=boltState;memcpy(g_magazineStates,states,sizeof(states));memcpy(g_supportGrab214,settings,sizeof(settings));g_reload=reload;g_manualReload=manual;g_controllerNeutral124=neutral;g_freeRightFromController=attachment;g_freeRightEpoch=rightEpoch;
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_frame=frame;g_pistolMenu=menu;g_motionGun=motion;g_weaponMotionGuard=guard;g_handLeftValid=left;g_handRightValid=right;g_twoHand=two;g_eyeViewsSet=eyes;g_grabGuides=guides;g_reloadActionGuide227=hint;g_1911TintLoaded49=loaded;g_reloadActionColour227=colour;g_smgHintStrength230=strength;g_ctx=ctx;g_dev=dev;
 g_supportPalmDelta=palmDelta;g_supportPalmEpoch=palmEpoch;g_supportPalmProfile=palmProfile;g_supportPalmFrame=palmFrame;g_nativeWeaponProfile50=guideProfile;g_nativeGuidePacketCount50=packets;memcpy(g_nativeWeaponEye50,nativeEyes,sizeof(nativeEyes));memcpy(g_handBody,hands,sizeof(hands));memcpy(g_handRight,viewHands[0],64);memcpy(g_handLeft,viewHands[1],64);g_armJoints[0]=joints0;g_armJoints[1]=joints1;fc3pad::Publish(controls);
 puts("PASS:236 fourteen rifle/SMG/LMG gameplay/Insert contact+OBB+both-eye projections; nine required charging/bolt cues, exact MS16 handle triangles, visible both-eye pixels, ready/ownership/camera/geometry guards");
}
}
