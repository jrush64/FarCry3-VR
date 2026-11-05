// Execute the matrix prologue from the ACTUAL patched river PS. Retain its
// declarations, replace only the material body with four dp4 diagnostic outputs.
// Compare homogeneous game-view reconstruction to independent CPU matrices.
static int RiverDepthTest(const char* folder,const char* prefix,bool reflection=false,const char* hash="5E139F4BB30305E5") {
 const bool ocean=strcmp(hash,"919B2333D357EBB2")==0||strcmp(hash,"FFD1D36205DC7757")==0;
 const bool edge=strcmp(hash,"6CEEF5F8E48054A5")==0,waterSurface=ocean||edge||strcmp(hash,"3E11D1C4732A23E3")==0;
 const bool waterMatrixOnly=strcmp(hash,"22ECA6952F53DBD7")==0;
 const bool diffuse=strcmp(hash,"B5850815B1E93395")==0 || strcmp(hash,"F9BD64B91671E5FE")==0;
 char path[1024];std::vector<unsigned char> original,patched,bytes;
 sprintf_s(path,"%s\\fc3_ps_%s.bin",folder,hash);
 if(!ScreenReadFile(path,original))return Fail("river shader file");
 bool tile=false;
 if(dxbc::PatchPixelShader(original.data(),original.size(),patched,&tile,true)!=dxbc::PixelResult::Patched || tile)return Fail("river patch missing or tile test added");
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)))return Fail("river WARP");
 ID3D11PixelShader* full=nullptr;
 if(FAILED(dev->CreatePixelShader(patched.data(),patched.size(),nullptr,&full)))return Fail("full patched river PS invalid");
 fc3shadow::Release(full);
 auto u32=[](const unsigned char* p){uint32_t v;memcpy(&v,p,4);return v;};
 uint32_t count=u32(patched.data()+28);std::vector<unsigned char> diagnostic(patched.begin(),patched.begin()+32+count*4);
 for(uint32_t c=0;c<count;++c){
  uint32_t off=u32(patched.data()+32+c*4),tag=u32(patched.data()+off),size=u32(patched.data()+off+4);
  uint32_t newOff=(uint32_t)diagnostic.size();memcpy(diagnostic.data()+32+c*4,&newOff,4);
  if(tag!=0x52444853u && tag!=0x58454853u){diagnostic.insert(diagnostic.end(),patched.begin()+off,patched.begin()+off+8+size);continue;}
  std::vector<uint32_t> t(size/4);memcpy(t.data(),patched.data()+off+8,size);
  uint32_t base=0;
  size_t first=2;while(first<t.size()){uint32_t op=t[first]&0x7ff,len=(t[first]>>24)&127;if(!len)return Fail("river declaration length");if(op==104)base=t[first+1]-(waterSurface||(!diffuse&&!waterMatrixOnly)?6:4);if(!(op>=88&&op<=106))break;first+=len;}
  size_t end=first;for(int i=0;i<32;++i){if(end>=t.size())return Fail("river prologue bounds");end+=(t[end]>>24)&127;}
  // Four rows, each four dp4s, add, mad, add, mul. Avoid silently testing
  // a different section if the patcher changes its instruction arrangement.
  if(end-first!=280)return Fail("river prologue changed");
  if(reflection)for(int i=0;i<(waterSurface?11:13);++i)end+=(t[end]>>24)&127;
  t.resize(end);
  const uint32_t inputDecl[]={89u|(4u<<24),0x00208E46u,12,1};
  t.insert(t.begin()+first,inputDecl,inputDecl+4);
  if(reflection){uint32_t op[]={54u|(5u<<24),0x001020F2u,0,0x00100E46u,base+5};t.insert(t.end(),op,op+5);}
  else for(uint32_t row=0;row<4;++row){uint32_t op[]={17u|(8u<<24),0x00102002u|((1u<<row)<<4),0,0x00208E46u,12,0,0x00100E46u,base+row};t.insert(t.end(),op,op+8);}
  t.push_back(62u|(1u<<24));t[1]=(uint32_t)t.size();uint32_t newSize=(uint32_t)t.size()*4;
  auto append=[&](const void* p,size_t n){const unsigned char* b=(const unsigned char*)p;diagnostic.insert(diagnostic.end(),b,b+n);};
  append(&tag,4);append(&newSize,4);append(t.data(),newSize);
 }
 uint32_t totalSize=(uint32_t)diagnostic.size();memcpy(diagnostic.data()+24,&totalSize,4);uint32_t sum[4];dxbc::ComputeChecksum(diagnostic.data(),diagnostic.size(),sum);memcpy(diagnostic.data()+4,sum,16);
 ID3D11PixelShader* ps=nullptr;if(FAILED(dev->CreatePixelShader(diagnostic.data(),diagnostic.size(),nullptr,&ps)))return Fail("river diagnostic PS");
 const char* source="cbuffer Input:register(b0){float4 sampleInput;} struct O{float4 p:SV_Position;float4 a:TEXCOORD0;float4 b:TEXCOORD1;float3 c:TEXCOORD2;float3 d:TEXCOORD3;float3 e:TEXCOORD4;float3 f:TEXCOORD5;float3 g:TEXCOORD6;float3 h:TEXCOORD7;float3 i:TEXCOORD8;};O main(uint id:SV_VertexID){O o=(O)0;float2 uv=float2((id<<1)&2,id&2);o.p=float4(uv*float2(2,-2)+float2(-1,1),0,1);o.g=float3(sampleInput.xy*sampleInput.w,sampleInput.w);o.a=float4(o.g,0);o.b=float4(o.g,0);o.c=o.d=o.e=o.f=o.h=o.i=o.g;return o;}";
 ID3DBlob *blob=nullptr,*err=nullptr;if(FAILED(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&blob,&err)))return Fail("river diagnostic VS compile");
 ID3D11VertexShader* vs=nullptr;if(FAILED(dev->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&vs)))return Fail("river diagnostic VS");
 fc3shadow::Release(blob);fc3shadow::Release(err);
 D3D11_TEXTURE2D_DESC td={1,1,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_RENDER_TARGET,0,0};
 ID3D11Texture2D *target=nullptr,*stage=nullptr;ID3D11RenderTargetView* rtv=nullptr;
 if(FAILED(dev->CreateTexture2D(&td,nullptr,&target))||FAILED(dev->CreateRenderTargetView(target,nullptr,&rtv)))return Fail("river output target");
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(dev->CreateTexture2D(&td,nullptr,&stage)))return Fail("river staging");
 D3D11_VIEWPORT vp={0,0,1,1,0,1};ctx->RSSetViewports(1,&vp);ctx->OMSetRenderTargets(1,&rtv,nullptr);ctx->VSSetShader(vs,nullptr,0);ctx->PSSetShader(ps,nullptr,0);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 int checks=0;double worst=0;
 for(int eye=0;eye<2;++eye){
  sprintf_s(path,"%s_d%d_e%d_cb1.bin",prefix,eye+2,eye);if(!ScreenReadFile(path,bytes))return Fail("river viewport capture");
  float camera[1024]={};if(bytes.size()>sizeof(camera))return Fail("river viewport size");memcpy(camera,bytes.data(),bytes.size());
  ID3D11Buffer* cb1=MakeCb(dev,camera,sizeof(camera));ctx->PSSetConstantBuffers(diffuse||edge?0:1,1,&cb1);
  sprintf_s(path,"%s_d%d_e%d_cb13.bin",prefix,eye+2,eye);if(!ScreenReadFile(path,bytes))return Fail("river eye capture");
  float eyes[20];memcpy(eyes,bytes.data(),sizeof(eyes));ID3D11Buffer* cb13=MakeCb(dev,eyes,sizeof(eyes));ctx->PSSetConstantBuffers(13,1,&cb13);
  M4 inverse={},projectionInverse={};for(int r=0;r<4;++r)for(int c=0;c<4;++c){inverse.m[r*4+c]=eyes[c*4+r]+(r==c?1.:0.);projectionInverse.m[r*4+c]=camera[64+r*4+c];}
  M4 expected=Mul(projectionInverse,inverse);
  for(int y=0;y<3;++y)for(int x=0;x<3;++x)for(int z=0;z<3;++z){
   float input[]={.1f+.4f*x,.1f+.4f*y,.8f+.09f*z,1};ID3D11Buffer* cb0=MakeCb(dev,input,sizeof(input));ctx->PSSetConstantBuffers(12,1,&cb0);
   float surface[]={input[0],input[1],input[2],1.f/(camera[78]*input[2]+camera[79])};
   ID3D11Buffer* vsCb=MakeCb(dev,surface,sizeof(surface));ctx->VSSetConstantBuffers(0,1,&vsCb);
   const float clear[]={999,999,999,999};ctx->ClearRenderTargetView(rtv,clear);ctx->Draw(3,0);ctx->CopyResource(stage,target);
   D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(ctx->Map(stage,0,D3D11_MAP_READ,0,&map)))return Fail("river map");const float* got=(const float*)map.pData;
   double point[]={input[0]*2.-1,1-input[1]*2.,input[2],1};
   double game[4]={};for(int r=0;r<4;++r)for(int c=0;c<4;++c)game[r]+=inverse.m[r*4+c]*point[c];
   for(int r=0;r<(reflection&&!waterSurface?2:4);++r){double want=0;
    if(reflection&&waterSurface)want=r==3?game[3]*surface[3]:game[r];
    else if(reflection)want=(.5+(r?-.5:.5)*game[r]/game[3])*camera[148+r];
    else for(int c=0;c<4;++c)want+=expected.m[r*4+c]*point[c];double e=fabs(got[r]-want)/(1+fabs(want));worst=(std::max)(worst,e);if(!std::isfinite(got[r])||e>2e-5){printf("eye %d xyz %d %d %d row %d: got %.9g expected %.9g\n",eye,x,y,z,r,got[r],want);return Fail("river game-view depth reconstruction");}++checks;}
   ctx->Unmap(stage,0);fc3shadow::Release(cb0);fc3shadow::Release(vsCb);
  }
  fc3shadow::Release(cb1);fc3shadow::Release(cb13);
 }
 ctx->ClearState();fc3shadow::Release(rtv);fc3shadow::Release(target);fc3shadow::Release(stage);fc3shadow::Release(vs);fc3shadow::Release(ps);fc3shadow::Release(ctx);fc3shadow::Release(dev);
 printf("PASS: river %s %s GPU prologue agrees with CPU, %d checks, worst %.8g\n",hash,reflection?"reflection":"depth",checks,worst);return 0;
}
