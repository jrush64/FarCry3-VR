// Execute the actual injected sample-conversion instructions from all four
// real particle pixel shaders, with the saved inverse eye matrices.
static int ParticleDepthTest254(const char* folder,const char* prefix,bool fog257=false){
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)))return Fail("particle depth WARP");
 int checks=0;char path[1024];
 for(const char* hash:{"C19BFBC792F0F843","BA81B5F7771569B2","057EE039AB2B7A3E","E20ABD31F16921EA","83A6FCAF6854B4ED","B42B55CCCF14B3CE","6771A60187C7067F"}){
  if(fog257!=(strcmp(hash,"6771A60187C7067F")==0))continue;
  const bool msaa=strcmp(hash,"B42B55CCCF14B3CE")==0;
  sprintf_s(path,"%s/fc3_ps_%s.bin",folder,hash);std::vector<unsigned char> original,patched,bytes;
  if(!ScreenReadFile(path,original)||dxbc::PatchPixelShader(original.data(),original.size(),patched)!=dxbc::PixelResult::Patched)return Fail("real particle depth PS patch");
  ID3D11PixelShader* full=nullptr;if(FAILED(dev->CreatePixelShader(patched.data(),patched.size(),nullptr,&full)))return Fail("full real particle depth PS invalid");fc3shadow::Release(full);
  auto read=[](const unsigned char* p){uint32_t v;memcpy(&v,p,4);return v;};
  uint32_t chunks=read(patched.data()+28);std::vector<unsigned char> diagnostic(patched.begin(),patched.begin()+32+chunks*4);
  for(uint32_t c=0;c<chunks;++c){uint32_t offset=read(patched.data()+32+c*4),tag=read(patched.data()+offset),size=read(patched.data()+offset+4),outOffset=(uint32_t)diagnostic.size();memcpy(diagnostic.data()+32+c*4,&outOffset,4);
   if(tag!=0x52444853&&tag!=0x58454853){diagnostic.insert(diagnostic.end(),patched.begin()+offset,patched.begin()+offset+8+size);continue;}
   std::vector<uint32_t> t(size/4);memcpy(t.data(),patched.data()+offset+8,size);size_t first=2;uint32_t temps=0;
   while(first<t.size()){auto op=t[first]&2047,len=(t[first]>>24)&127;if(!len)return Fail("particle depth token");if(op==104)temps=t[first+1];if(!(op>=88&&op<=106))break;first+=len;}
   size_t sample=first;while(sample<t.size()&&(t[sample]&2047)!=(msaa?46u:69u))sample+=(t[sample]>>24)&127;
   if(sample>=t.size())return Fail("particle sample missing");size_t dst=sample+1;uint32_t ext=t[sample];while(ext&0x80000000u)ext=t[dst++];
   uint32_t mask=(t[dst]>>4)&15,reg=t[dst+1],component=mask==1?0:1;
   size_t injected=sample+((t[sample]>>24)&127),end=injected;for(int i=0;i<9;++i)end+=(t[end]>>24)&127;
   std::vector<uint32_t> output(t.begin(),t.begin()+first);
   uint32_t cb[]={89u|(4u<<24),0x00208E46u,12,1};output.insert(output.end(),cb,cb+4);
   uint32_t seed[]={54u|(6u<<24),0x00100002u|(mask<<4),reg,0x00208AA6u,12,0};output.insert(output.end(),seed,seed+6);
   output.insert(output.end(),t.begin()+injected,t.begin()+end);
   uint32_t move[]={54u|(5u<<24),0x001020F2u,0,0x00100E46u,temps-1};output.insert(output.end(),move,move+5);
   uint32_t depth[]={54u|(5u<<24),0x00102042u,0,0x00100006u|(component*85u<<4),reg};output.insert(output.end(),depth,depth+5);
   output.push_back(62u|(1u<<24));output[1]=(uint32_t)output.size();uint32_t length=(uint32_t)output.size()*4;
   const auto append=[&](const void* p,size_t n){const auto* b=(const unsigned char*)p;diagnostic.insert(diagnostic.end(),b,b+n);};append(&tag,4);append(&length,4);append(output.data(),length);
  }
  uint32_t length=(uint32_t)diagnostic.size(),checksum[4];memcpy(diagnostic.data()+24,&length,4);dxbc::ComputeChecksum(diagnostic.data(),diagnostic.size(),checksum);memcpy(diagnostic.data()+4,checksum,16);
  ID3D11PixelShader* ps=nullptr;if(FAILED(dev->CreatePixelShader(diagnostic.data(),diagnostic.size(),nullptr,&ps)))return Fail("particle injected diagnostic PS");
  const char* source="cbuffer I:register(b0){float4 v;} struct O{float4 p:SV_Position;float clip:SV_ClipDistance0;float4 a:TEXCOORD0;float4 b:TEXCOORD1;float4 c:TEXCOORD2;float4 d:TEXCOORD3;float4 e:TEXCOORD4;float4 f:TEXCOORD5;float4 g:TEXCOORD6;float4 h:TEXCOORD7;};O main(uint id:SV_VertexID){O o=(O)0;float2 p=float2((id<<1)&2,id&2);o.p=float4(p*float2(2,-2)+float2(-1,1),0,1);o.clip=1;o.d=float4(v.xy,1,0);return o;}";
  ID3DBlob *code=nullptr,*error=nullptr;if(FAILED(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&code,&error)))return Fail("particle diagnostic VS compile");ID3D11VertexShader* vs=nullptr;dev->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&vs);fc3shadow::Release(code);fc3shadow::Release(error);
  D3D11_TEXTURE2D_DESC td={1,1,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_RENDER_TARGET,0,0};ID3D11Texture2D *target=nullptr,*stage=nullptr;ID3D11RenderTargetView* rtv=nullptr;dev->CreateTexture2D(&td,nullptr,&target);dev->CreateRenderTargetView(target,nullptr,&rtv);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;dev->CreateTexture2D(&td,nullptr,&stage);
  D3D11_VIEWPORT vp={0,0,1,1,0,1};ctx->RSSetViewports(1,&vp);ctx->OMSetRenderTargets(1,&rtv,nullptr);ctx->VSSetShader(vs,nullptr,0);ctx->PSSetShader(ps,nullptr,0);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  for(int eye=0;eye<2;++eye){sprintf_s(path,"%s_d%d_e%d_cb13.bin",prefix,eye+2,eye);if(!ScreenReadFile(path,bytes))return Fail("captured inverse eye for particle depth");float eyes[20]={};memcpy(eyes,bytes.data(),sizeof(eyes));ID3D11Buffer* cb13=MakeCb(dev,eyes,sizeof(eyes));ctx->PSSetConstantBuffers(13,1,&cb13);
   for(int y=0;y<3;++y)for(int x=0;x<3;++x)for(int z=0;z<3;++z){float in[]={.1f+.4f*x,.1f+.4f*y,.8f+.09f*z,1};auto* input=MakeCb(dev,in,sizeof(in));ctx->PSSetConstantBuffers(12,1,&input);ctx->VSSetConstantBuffers(0,1,&input);ctx->Draw(3,0);ctx->CopyResource(stage,target);D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(ctx->Map(stage,0,D3D11_MAP_READ,0,&map)))return Fail("particle result map");const float* got=(const float*)map.pData;
    double p[]={in[0]*2.-1,1-in[1]*2.,in[2],1},g[4]={};for(int r=0;r<4;++r)for(int c=0;c<4;++c)g[r]+=(eyes[c*4+r]+(r==c?1.:0.))*p[c];
    for(int r=0;r<4;++r){double want=r==2?g[2]/g[3]:g[r];if(!std::isfinite(got[r])||fabs(got[r]-want)/(1+fabs(want))>2e-5){printf("PS%s eye%d row%d got%.8g expected%.8g\n",hash,eye,r,got[r],want);return Fail("particle native-depth conversion disagrees with independent CPU");}++checks;}ctx->Unmap(stage,0);fc3shadow::Release(input);
   }fc3shadow::Release(cb13);
  }ctx->ClearState();fc3shadow::Release(rtv);fc3shadow::Release(target);fc3shadow::Release(stage);fc3shadow::Release(ps);fc3shadow::Release(vs);
 }fc3shadow::Release(ctx);fc3shadow::Release(dev);printf("PASS: %s validate and execute injected native-depth recovery, %d GPU comparisons across captured eyes\n",fog257?"captured atmospheric particle PS":"six real particle PS (including4x MSAA)",checks);return 0;
}
