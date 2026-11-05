void TestThrowCamera153(ID3D11Device* device,ID3D11DeviceContext* context)
{
 context->ClearState();g_dev=device;g_ctx=context;
 g_enabled=g_headTracking=g_inGameplay=g_motionGun=g_aimEnabled=g_eyeViewsSet=g_frameCameraValid=g_bodyValid=g_aimActive=true;
 g_flatViewActive=false;g_pistolMenu.open=false;g_weaponMotionGuard=true;g_visibleWeaponDrawsLastFrame=0;g_machete.held=false;
 fc3aim::SetNativeActionForTest128(0);fc3aim::SetVehicleStateForTest(0);fc3pad::SetGameplayControls(true);fc3pad::SetPistolCalibration(false);
 fc3pad::Controllers input={};input.active=true;fc3pad::Publish(input);
 g_bodyYaw=0;g_rightSign=1;g_visibilityCamera68=g_visibilityHandoff68=false;
 for(int eye=0;eye<2;++eye){auto& v=g_eyeViewsBody[eye];v={};const double h=.2;v.view[0]=v.view[10]=float(cos(h));v.view[2]=-float(sin(h));v.view[8]=float(sin(h));v.view[5]=v.view[15]=1;v.view[3]=eye?-.0315f:.0315f;}
 auto* vsCode=Native1911Compile48("cbuffer Camera:register(b0){row_major float4x4 transform;}float4 main(float3 p:POSITION):SV_Position{return mul(transform,float4(p,1));}","vs_5_0");
 auto* psCode=Native1911Compile48("float4 main():SV_Target{return float4(1,0,0,1);}","ps_5_0");Check(vsCode&&psCode,"throw camera shaders compile");
 ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;ID3D11InputLayout* layout=nullptr;
 Check(SUCCEEDED(device->CreateVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),nullptr,&vs))&&SUCCEEDED(device->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps)),"throw camera shaders");
 D3D11_INPUT_ELEMENT_DESC element={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
 Check(SUCCEEDED(device->CreateInputLayout(&element,1,vsCode->GetBufferPointer(),vsCode->GetBufferSize(),&layout)),"throw camera layout");
 float triangle[9]={2,.3f,-.3f,2,-.3f,-.3f,2,0,.3f};auto* vb=Buffer(device,sizeof(triangle),D3D11_BIND_VERTEX_BUFFER);context->UpdateSubresource(vb,0,nullptr,triangle,0,0);auto* cb=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=12,offset=0;context->IASetInputLayout(layout);context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->VSSetConstantBuffers(0,1,&cb);
 D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;ID3D11RasterizerState* raster=nullptr;Check(SUCCEEDED(device->CreateRasterizerState(&rd,&raster)),"throw camera raster");context->RSSetState(raster);D3D11_VIEWPORT viewport={0,0,64,64,0,1};context->RSSetViewports(1,&viewport);
 D3D11_TEXTURE2D_DESC desc={};desc.Width=desc.Height=64;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* target=nullptr,*stage=nullptr;ID3D11RenderTargetView* rtv=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target))&&SUCCEEDED(device->CreateRenderTargetView(target,nullptr,&rtv)),"throw camera target");desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&stage)),"throw camera readback");context->OMSetRenderTargets(1,&rtv,nullptr);
 Mat4 projection={};projection.m[0]=projection.m[5]=1.5;projection.m[10]=projection.m[14]=1;
 V3 baseline[2]={};
 for(int phase=0;phase<4;++phase){
  // Held, native release animation, no held-item state, and restored gun.
  g_throw144.owned=phase<2;g_throw144.kind=1;g_throw144.launched=phase>0;
  g_throwArm150={g_armPoseEpoch,g_frame,phase==1||phase==2?GetTickCount64()+2000:0};
  if(phase==3){g_throw144.owned=false;g_visibleWeaponDrawsLastFrame=3;}
  const double yaw[4]={.7,-.6,1.1,-.9},pitch[4]={.15,-.3,.4,-.2};double axes[3][3];VisibilityBasisMatrix68(yaw[phase],pitch[phase],.1*phase,1,axes);
  for(int i=0;i<3;++i){g_frameCamera.right[i]=float(axes[0][i]);g_frameCamera.up[i]=float(axes[1][i]);g_frameCamera.forward[i]=float(axes[2][i]);}
  QueryPerformanceCounter(&g_aimLast);AimFrame(true);
  Check(g_bodyValid&&g_aimActive&&fabs(g_bodyYaw)<1e-7&&!g_visibilityCamera68&&!g_visibilityHandoff68,"held/released/restored throw never reseeds body or enters hidden camera lane");
  Check(ThrowableCamera153(true)==(phase!=3),"throw camera ownership ends on restored firearm without replacing body basis");
  AimActualCamera(g_frameCamera);ApplyAimToPoses();
  Mat4 camera=HandIdentity();for(int i=0;i<3;++i)for(int j=0;j<3;++j)camera.m[i*4+j]=axes[i][j];
  for(int eye=0;eye<2;++eye){Mat4 view={};for(int i=0;i<16;++i)view.m[i]=g_eyeViews[eye].view[i];const Mat4 transform=Multiply(projection,Multiply(view,camera));float upload[16];for(int i=0;i<16;++i)upload[i]=float(transform.m[i]);context->UpdateSubresource(cb,0,nullptr,upload,0,0);
   const float black[4]={0,0,0,1};context->ClearRenderTargetView(rtv,black);context->Draw(3,0);context->CopyResource(stage,target);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"throw camera pixels");
   V3 center={};for(int y=0;y<64;++y)for(int x=0;x<64;++x)if(static_cast<const BYTE*>(mapped.pData)[y*mapped.RowPitch+x*4]>128){center.x+=x;center.y+=y;++center.z;}context->Unmap(stage,0);Check(center.z>20,"static world target remains visible in both rendered eyes");center.x/=center.z;center.y/=center.z;
   if(!phase)baseline[eye]=center;else Check(fabs(center.x-baseline[eye].x)<.1&&fabs(center.y-baseline[eye].y)<.1,"GPU world target does not snap when native throw/gun camera rotates");
  }
 }
 Check(fabs(baseline[0].x-baseline[1].x)>.5,"throw camera preserves stereo disparity");
 g_throwArm150={g_armPoseEpoch,g_frame,GetTickCount64()+1000};g_visibleWeaponDrawsLastFrame=0;
 fc3aim::SetVehicleStateForTest(1);Check(!ThrowableCamera153(true),"vehicle outranks throw camera");fc3aim::SetVehicleStateForTest(0);
 fc3aim::SetNativeActionForTest128(1);Check(!ThrowableCamera153(true),"traversal outranks throw camera");fc3aim::SetNativeActionForTest128(0);
 Check(!ThrowableCamera153(false),"XR loss releases throw camera ownership");g_throwArm150={};g_throw144={};
 context->ClearState();rtv->Release();target->Release();stage->Release();raster->Release();vb->Release();cb->Release();layout->Release();vs->Release();ps->Release();vsCode->Release();psCode->Release();fc3aim::SetNativeActionForTest128(-1);fc3aim::SetVehicleStateForTest(-1);
 std::puts("PASS:153 actual AimFrame and observed-draw compensation keep stereo world pixels stable across held throw, launch, hidden-item cleanup and gun restore; native action/vehicle/XR priority retained.");
}
