static bool ScreenReadFile(const char* path,std::vector<unsigned char>& bytes) {
 FILE* f=nullptr;if(fopen_s(&f,path,"rb")||!f)return false;
 fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);bytes.resize(n);
 bool ok=fread(bytes.data(),1,bytes.size(),f)==bytes.size();fclose(f);return ok;
}
static int ScreenOutputsTest(const char* folder,const char* prefix) {
 ID3D11Device* dev=nullptr;ID3D11DeviceContext* ctx=nullptr;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx)))return Fail("screen WARP");
 const char* hashes[]={"2287E89CC64747ED","E01EBD2966163B82","F23C874C31A8D0A0","97ADE3799CCB9D8D","19F9BF97F64AD98D"};
 int total=0;double worst=0;char path[1024];
 for(int shader=0;shader<5;++shader)for(int eye=0;eye<2;++eye) {
  sprintf_s(path,"%s\\fc3_vs_%s.bin",folder,hashes[shader]);std::vector<unsigned char> original,patched,bytes;
  if(!ScreenReadFile(path,original))return Fail("read real screen shader");
  if(dxbc::PatchVertexShader(original.data(),original.size(),patched)!=dxbc::Result::Patched)return Fail("patch screen shader");
  ID3D11VertexShader* vs=nullptr;if(FAILED(dev->CreateVertexShader(patched.data(),patched.size(),nullptr,&vs)))return Fail("create patched screen VS");
  const D3D11_SO_DECLARATION_ENTRY entriesInterior[]={{0,"SV_Position",0,0,4,0},{0,"TEXCOORD",1,0,3,0},{0,"TEXCOORD",0,3,1,0}};
  const D3D11_SO_DECLARATION_ENTRY entriesWater[]={{0,"SV_Position",0,0,4,0},{0,"TEXCOORD",3,0,3,0},{0,"TEXCOORD",5,0,3,0},{0,"TEXCOORD",4,0,3,0}};
  const bool river=shader>=3;
  const D3D11_SO_DECLARATION_ENTRY entriesRiver[]={{0,"SV_Position",0,0,4,0},{0,"TEXCOORD",shader==4?5u:6u,0,3,0}};
  UINT floats=river?7:(shader?13:8),stride=floats*4;ID3D11GeometryShader* gs=nullptr;
  HRESULT hr=dev->CreateGeometryShaderWithStreamOutput(patched.data(),patched.size(),river?entriesRiver:(shader?entriesWater:entriesInterior),river?2:(shader?4:3),&stride,1,D3D11_SO_NO_RASTERIZED_STREAM,nullptr,&gs);
  if(FAILED(hr)){printf("SO HRESULT %08X\n",hr);return Fail("screen stream output");}
  float constants[6][1024];for(int b=0;b<6;++b)for(int i=0;i<1024;++i)constants[b][i]=.1f+.003f*(i%7);
  sprintf_s(path,"%s_d%d_e%d_cb1.bin",prefix,eye+2,eye);if(!ScreenReadFile(path,bytes))return Fail("screen captured cb1");
  memcpy(constants[river?0:1],bytes.data(),bytes.size());
  if(river) {
   // Keep the captured viewport, use a translated identity spline transform.
   for(int r=0;r<3;++r)for(int c=0;c<4;++c)constants[2][20+r*4+c]=c==3?constants[0][152+r]:(r==c?1.f:0.f);
   constants[2][8]=0;constants[2][9]=1;constants[3][28]=0;
  }
  // Controlled interior volume; real game viewport/camera constants remain intact.
  if(!shader) {
   constants[0][40]=0;constants[0][41]=1;
   for(int r=0;r<3;++r)for(int c=0;c<4;++c)constants[0][108+r*4+c]=c==3?constants[1][152+r]:(r==c?1.f:0.f);
  }
  sprintf_s(path,"%s_d%d_e%d_cb13.bin",prefix,eye+2,eye);if(!ScreenReadFile(path,bytes))return Fail("screen captured eye cb");
  float invcols[20];memcpy(invcols,bytes.data(),80);M4 inverse={},forward={};
  for(int r=0;r<4;++r)for(int c=0;c<4;++c)inverse.m[r*4+c]=invcols[c*4+r]+(r==c?1.:0.);
  if(!Inverse(inverse,forward))return Fail("eye matrix inverse");
  float eyeData[64]={};for(int i=0;i<16;++i)eyeData[i]=(float)(forward.m[i]-(i%5==0?1.:0.));
  ID3D11Buffer* cb[6]={};for(int b=0;b<6;++b){D3D11_BUFFER_DESC bd={sizeof(constants[b]),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA init={constants[b],0,0};if(FAILED(dev->CreateBuffer(&bd,&init,&cb[b])))return Fail("screen cb");}
  ID3D11Buffer* eyeCb=nullptr;D3D11_BUFFER_DESC ed={sizeof(eyeData),D3D11_USAGE_DEFAULT,D3D11_BIND_CONSTANT_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA ei={eyeData,0,0};if(FAILED(dev->CreateBuffer(&ed,&ei,&eyeCb)))return Fail("screen eye cb");
  const int n=32;float vertices[n][4];for(int i=0;i<n;++i)for(int c=0;c<4;++c){float v=(float)(2+(i*(c+1))%19);if(shader && !river)vertices[i][c]=v;else {int iv=(int)v;memcpy(&vertices[i][c],&iv,4);}}
  D3D11_INPUT_ELEMENT_DESC elements[]={
   {shader?"position":"POSITION",0,(shader&&!river)?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R32G32B32A32_SINT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
   {"texcoord",0,DXGI_FORMAT_R32G32_SINT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
   {"normal",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
   {"color",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
   {"fog",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
   {"psize",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}};
  ID3D11InputLayout* layout=nullptr;if(FAILED(dev->CreateInputLayout(elements,river?6:1,original.data(),original.size(),&layout)))return Fail("screen layout");
  D3D11_BUFFER_DESC vd={sizeof(vertices),D3D11_USAGE_DEFAULT,D3D11_BIND_VERTEX_BUFFER,0,0,0};D3D11_SUBRESOURCE_DATA vi={vertices,0,0};ID3D11Buffer* vb=nullptr;if(FAILED(dev->CreateBuffer(&vd,&vi,&vb)))return Fail("screen VB");
  D3D11_BUFFER_DESC od={n*stride,D3D11_USAGE_DEFAULT,D3D11_BIND_STREAM_OUTPUT,0,0,0};ID3D11Buffer* so=nullptr;ID3D11Buffer* stage=nullptr;
  if(FAILED(dev->CreateBuffer(&od,nullptr,&so)))return Fail("screen SO buffer");od.Usage=D3D11_USAGE_STAGING;od.BindFlags=0;od.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(dev->CreateBuffer(&od,nullptr,&stage)))return Fail("screen staging");
  float texel[4]={.25f,.3f,.4f,.5f};D3D11_TEXTURE2D_DESC td={1,1,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,{1,0},D3D11_USAGE_DEFAULT,D3D11_BIND_SHADER_RESOURCE,0,0};D3D11_SUBRESOURCE_DATA ti={texel,16,0};ID3D11Texture2D* tex=nullptr;ID3D11ShaderResourceView* srv=nullptr;
  if(FAILED(dev->CreateTexture2D(&td,&ti,&tex))||FAILED(dev->CreateShaderResourceView(tex,nullptr,&srv)))return Fail("screen VS texture");
  D3D11_SAMPLER_DESC samp={};samp.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;samp.AddressU=samp.AddressV=samp.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;samp.MaxLOD=D3D11_FLOAT32_MAX;ID3D11SamplerState* sampler=nullptr;if(FAILED(dev->CreateSamplerState(&samp,&sampler)))return Fail("screen sampler");
  ID3D11ShaderResourceView* srvs[]={srv,srv};ID3D11SamplerState* samplers[]={sampler,sampler};ctx->VSSetShaderResources(0,2,srvs);ctx->VSSetSamplers(0,2,samplers);
  UINT offset=0,vstride=16;ctx->IASetInputLayout(layout);ctx->IASetVertexBuffers(0,1,&vb,&vstride,&offset);ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
  ctx->VSSetConstantBuffers(0,6,cb);ctx->VSSetConstantBuffers(13,1,&eyeCb);ctx->VSSetShader(vs,nullptr,0);ctx->GSSetShader(gs,nullptr,0);ctx->SOSetTargets(1,&so,&offset);ctx->Draw(n,0);ctx->SOSetTargets(0,nullptr,nullptr);ctx->CopyResource(stage,so);
  D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(ctx->Map(stage,0,D3D11_MAP_READ,0,&map)))return Fail("screen SO map");
  const float* values=(const float*)map.pData;
  for(int i=0;i<n;++i){const float* p=values+i*floats;if(!std::isfinite(p[3])||fabs(p[3])<1e-6)return Fail("invalid fixture clip");
   const float* viewport=constants[river?0:1];
   double expected[]={p[0]*viewport[96]+p[3]*viewport[100],p[1]*viewport[97]+p[3]*viewport[101],p[3],shader?(p[0]+p[3])*.5:p[3]*viewport[147],(p[3]-p[1])*.5,p[3]};
   for(int c=0;c<(river?3:(shader?6:4));++c){double err=fabs(p[c+4]-expected[c])/(1+fabs(expected[c]));worst=(std::max)(worst,err);if(!std::isfinite(p[c+4])||err>2e-5){printf("shader %s eye %d vertex %d component %d got %.8g expected %.8g\n",hashes[shader],eye,i,c,p[c+4],expected[c]);return Fail("screen outputs disagree with eye clip");}++total;}
  }
  ctx->Unmap(stage,0);ctx->ClearState();for(auto& b:cb)fc3shadow::Release(b);
  fc3shadow::Release(vs);fc3shadow::Release(gs);fc3shadow::Release(layout);fc3shadow::Release(vb);fc3shadow::Release(so);fc3shadow::Release(stage);fc3shadow::Release(eyeCb);fc3shadow::Release(srv);fc3shadow::Release(tex);fc3shadow::Release(sampler);
 }
 fc3shadow::Release(ctx);fc3shadow::Release(dev);printf("PASS: 5 captured VS, both captured eye transforms, %d screen-coordinate/depth checks, worst relative error %.8g\n",total,worst);return 0;
}
