// Execute the full captured motion-blur shader, not a diagnostic substitute.
// Moving-camera negative control must blur a patterned image; patched output
// must match the original stationary-camera result, including alpha/masks.
static int MotionBlurTest(const char* folder) {
 char path[1024];sprintf_s(path,"%s\\fc3_ps_A577E6A5E9ECCB71.bin",folder);
 std::vector<unsigned char> original,patched;if(!ScreenReadFile(path,original))return Fail("motion shader file");
 bool tile=true;if(dxbc::PatchPixelShader(original.data(),original.size(),patched,&tile,true)!=dxbc::PixelResult::Patched||tile||!dxbc::VerifyChecksum(patched.data(),patched.size()))return Fail("motion patch/checksum");
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)))return Fail("motion WARP");
 ID3D11PixelShader *stock=nullptr,*fixed=nullptr;
 if(FAILED(dev->CreatePixelShader(original.data(),original.size(),nullptr,&stock))||FAILED(dev->CreatePixelShader(patched.data(),patched.size(),nullptr,&fixed)))return Fail("motion full PS creation");
 const char* source="struct O{float4 p:SV_Position;float3 a:TEXCOORD0;float3 b:TEXCOORD1;float3 c:TEXCOORD2;float3 ray:TEXCOORD3;float4 uv:TEXCOORD4;};O main(uint id:SV_VertexID){O o=(O)0;float2 uv=float2((id<<1)&2,id&2);o.p=float4(uv*float2(2,-2)+float2(-1,1),0,1);o.ray=float3(uv*float2(2,-2)+float2(-1,1),1);o.uv=uv.xyxy;return o;}";
 ID3DBlob *blob=nullptr,*err=nullptr;if(FAILED(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&blob,&err)))return Fail("motion VS compile");
 ID3D11VertexShader* vs=nullptr;if(FAILED(dev->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&vs)))return Fail("motion VS create");fc3shadow::Release(blob);fc3shadow::Release(err);
 const UINT W=64,H=8;std::vector<float> pixels(W*H*4);
 ID3D11Texture2D* tex[3]={};ID3D11ShaderResourceView* srv[3]={};
 D3D11_TEXTURE2D_DESC td={W,H,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE,0,0};
 for(int t=0;t<3;++t){for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x)for(int c=0;c<4;++c)pixels[(y*W+x)*4+c]=t==1?(.05f+.9f*float((x*17+y*7+c*11)%31)/30):1.f;
 D3D11_SUBRESOURCE_DATA sd={pixels.data(),W*16,0};if(FAILED(dev->CreateTexture2D(&td,&sd,&tex[t]))||FAILED(dev->CreateShaderResourceView(tex[t],nullptr,&srv[t])))return Fail("motion inputs");}
 ID3D11Texture2D *target=nullptr,*stage=nullptr;ID3D11RenderTargetView* rtv=nullptr;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 if(FAILED(dev->CreateTexture2D(&td,nullptr,&target))||FAILED(dev->CreateRenderTargetView(target,nullptr,&rtv)))return Fail("motion target");
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(dev->CreateTexture2D(&td,nullptr,&stage)))return Fail("motion staging");
 D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState* sampler=nullptr;if(FAILED(dev->CreateSamplerState(&sd,&sampler)))return Fail("motion sampler");
 ID3D11SamplerState* samplers[]={sampler,sampler,sampler};ctx->PSSetSamplers(0,3,samplers);ctx->PSSetShaderResources(0,3,srv);
 D3D11_VIEWPORT vp={0,0,(float)W,(float)H,0,1};ctx->RSSetViewports(1,&vp);ctx->OMSetRenderTargets(1,&rtv,nullptr);ctx->VSSetShader(vs,nullptr,0);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 float camera[40*4]={};camera[4*4]=camera[5*4+1]=camera[7*4+3]=1;camera[26*4]=1;camera[37*4]=W;camera[37*4+1]=H;camera[37*4+2]=1.f/W;camera[37*4+3]=1.f/H;
 ID3D11Buffer* cb1=MakeCb(dev,camera,sizeof(camera));ctx->PSSetConstantBuffers(1,1,&cb1);
 auto render=[&](ID3D11PixelShader* ps,float dx,float dy,float dt,std::vector<float>& out){
  float globals[20*4]={};globals[9*4]=globals[10*4+1]=globals[12*4+3]=1;globals[9*4+3]=dx;globals[10*4+3]=dy;globals[17*4]=1;globals[17*4+1]=1;globals[17*4+3]=dt;
  ID3D11Buffer* cb0=MakeCb(dev,globals,sizeof(globals));if(!cb0)return false;ctx->PSSetConstantBuffers(0,1,&cb0);ctx->PSSetShader(ps,nullptr,0);ctx->Draw(3,0);ctx->CopyResource(stage,target);
  D3D11_MAPPED_SUBRESOURCE m={};if(FAILED(ctx->Map(stage,0,D3D11_MAP_READ,0,&m)))return false;out.resize(W*H*4);for(UINT y=0;y<H;++y)memcpy(out.data()+y*W*4,(const char*)m.pData+y*m.RowPitch,W*16);ctx->Unmap(stage,0);fc3shadow::Release(cb0);return true;
 };
 unsigned checks=0,blurred=0;double worst=0;
 for(float mask:{0.f,.5f,1.f}){
  for(size_t i=0;i<pixels.size();++i)pixels[i]=mask;ctx->UpdateSubresource(tex[2],0,nullptr,pixels.data(),W*16,0);
  std::vector<float> baseline,moving,got;if(!render(stock,0,0,1.f/60,baseline))return Fail("motion stationary draw");
  for(int direction=0;direction<4;++direction)for(float dt:{0.f,1.f/120,1.f/30}){
   float dx=direction==0?.073f:direction==1?-.093f:0,dy=direction==2?.37f:direction==3?-.53f:0;
   if(!render(stock,dx,dy,dt,moving)||!render(fixed,dx,dy,dt,got))return Fail("motion moving draw");
   for(size_t i=0;i<got.size();++i){double e=fabs(got[i]-baseline[i]);worst=(std::max)(worst,e);if(!std::isfinite(got[i])||e>2e-5)return Fail("motion patched output differs from stationary colour/alpha");if(fabs(moving[i]-baseline[i])>.01)++blurred;++checks;}
  }
 }
 if(blurred<100)return Fail("motion negative control did not blur image");
 ctx->ClearState();for(int t=0;t<3;++t){fc3shadow::Release(srv[t]);fc3shadow::Release(tex[t]);}fc3shadow::Release(cb1);fc3shadow::Release(sampler);fc3shadow::Release(rtv);fc3shadow::Release(target);fc3shadow::Release(stage);fc3shadow::Release(vs);fc3shadow::Release(stock);fc3shadow::Release(fixed);fc3shadow::Release(ctx);fc3shadow::Release(dev);
 printf("PASS: full motion shader %u RGBA checks, max error %.8g; stock moving-camera negative control changed %u components\n",checks,worst,blurred);return 0;
}
