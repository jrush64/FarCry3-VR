static int SceneGridTest258(){
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)))return Fail("grid WARP");
 const char* code="float4 vs(uint i:SV_VertexID):SV_Position{float2 uv=float2((i<<1)&2,i&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);}float4 ps(float4 p:SV_Position):SV_Target{return float4(p.x/96,p.y/54,.25,.75);}[numthreads(1,1,1)]void cs(uint3 p:SV_DispatchThreadID){}";
 ID3DBlob *blob=nullptr,*errors=nullptr;ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;ID3D11ComputeShader* cs=nullptr;
 auto compile=[&](const char* entry,const char* profile){HRESULT hr=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,entry,profile,0,0,&blob,&errors);fc3grid258::Release(errors);return SUCCEEDED(hr);};
 if(!compile("vs","vs_5_0")||FAILED(dev->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&vs)))return Fail("grid VS");fc3grid258::Release(blob);
 if(!compile("ps","ps_5_0")||FAILED(dev->CreatePixelShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&ps)))return Fail("grid PS");fc3grid258::Release(blob);
 if(!compile("cs","cs_5_0")||FAILED(dev->CreateComputeShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&cs)))return Fail("grid prior CS");fc3grid258::Release(blob);
 ctx->VSSetShader(vs,nullptr,0);ctx->PSSetShader(ps,nullptr,0);ctx->CSSetShader(cs,nullptr,0);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 D3D11_VIEWPORT vp={0,0,96,54,0,1};ctx->RSSetViewports(1,&vp);
 fc3grid258::Cache cache;int checks=0;
 for(UINT samples:{1u,4u}){
  D3D11_TEXTURE2D_DESC td={96,54,1,1,DXGI_FORMAT_R16G16B16A16_TYPELESS,{samples,0},D3D11_USAGE_DEFAULT,D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE,0,0};
  ID3D11Texture2D* tex=nullptr;ID3D11RenderTargetView* rt=nullptr;D3D11_RENDER_TARGET_VIEW_DESC rd={};rd.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;rd.ViewDimension=samples==1?D3D11_RTV_DIMENSION_TEXTURE2D:D3D11_RTV_DIMENSION_TEXTURE2DMS;
  if(FAILED(dev->CreateTexture2D(&td,nullptr,&tex))||FAILED(dev->CreateRenderTargetView(tex,&rd,&rt)))return Fail("grid HDR source");
  ctx->OMSetRenderTargets(1,&rt,nullptr);ctx->Draw(3,0);std::vector<float> got;D3D11_TEXTURE2D_DESC desc={};
  if(FAILED(fc3grid258::Read(cache,ctx,rt,got,desc))||got.size()!=64*36*4||desc.SampleDesc.Count!=samples)return Fail("grid real HDR sample read");
  for(int y=0;y<36;++y)for(int x=0;x<64;++x){const double expected[]={((int)((x+.5)*96/64)+.5)/96,((int)((y+.5)*54/36)+.5)/54,.25,.75};for(int c=0;c<4;++c){if(!std::isfinite(got[(y*64+x)*4+c])||fabs(got[(y*64+x)*4+c]-expected[c])>1e-3)return Fail("grid coordinate/sample-zero pixels");++checks;}}
  ID3D11RenderTargetView* restored=nullptr;ID3D11ComputeShader* restoredCs=nullptr;ID3D11PixelShader* restoredPs=nullptr;ctx->OMGetRenderTargets(1,&restored,nullptr);ctx->CSGetShader(&restoredCs,nullptr,nullptr);ctx->PSGetShader(&restoredPs,nullptr,nullptr);
  if(restored!=rt||restoredCs!=cs||restoredPs!=ps)return Fail("grid changed scene pipeline bindings");fc3grid258::Release(restored);fc3grid258::Release(restoredCs);fc3grid258::Release(restoredPs);
  ctx->OMSetRenderTargets(0,nullptr,nullptr);fc3grid258::Release(rt);fc3grid258::Release(tex);
 }
 cache.Reset();ctx->ClearState();fc3grid258::Release(cs);fc3grid258::Release(ps);fc3grid258::Release(vs);fc3grid258::Release(ctx);fc3grid258::Release(dev);
 printf("PASS:258 actual HDR and4x-MSAA scene grids, %d pixel checks, original colour target/compute/pixel bindings restored\n",checks);return 0;
}
