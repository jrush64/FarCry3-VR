// Diagnostic only: three automatic frames, each queued one frame ahead so the
// reflection producer and water consumer are observed within the same frame.
LONG g_waterCaptureFrame=-1;
int g_waterCaptureCount=0,g_waterProducerCount=0;
UINT g_waterCaptureSeen=0,g_waterDepthSeen=0;
ULONGLONG g_waterCaptureNext=0;
int WaterKind(unsigned long long h){
 if(h==0x5E139F4BB30305E5ull)return 0;
 if(h==0xA1F37111CC1926D5ull)return 1;
 if(h==0x919B2333D357EBB2ull)return 2;
 if(h==0xFFD1D36205DC7757ull)return 3;
 return -1;
}
void WaterBuffers(ID3D11DeviceContext* c,const wchar_t* label){
 for(int stage=0;stage<2;++stage)for(UINT slot: {0u,1u,2u,3u,4u,5u,13u}){
  ID3D11Buffer* cb=nullptr;if(stage)c->PSGetConstantBuffers(slot,1,&cb);else c->VSGetConstantBuffers(slot,1,&cb);
  std::vector<unsigned char> b;D3D11_BUFFER_DESC d={};if(cb)cb->GetDesc(&d);
  if(cb && d.ByteWidth<=65536 && ReadBytes(c,cb,b)){
   wchar_t name[160];_snwprintf_s(name,160,_TRUNCATE,L"fc3_water_f%ld_%s_%s_cb%u.bin",g_frame,label,stage?L"ps":L"vs",slot);WriteBlob(name,b);
  }SafeRelease(cb);
 }
}
void WaterReflection(ID3D11DeviceContext* c,ID3D11ShaderResourceView* srv,const wchar_t* label){
 if(!srv)return;D3D11_SHADER_RESOURCE_VIEW_DESC sd={};srv->GetDesc(&sd);
 if(sd.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D)return;
 ID3D11Resource* res=nullptr;srv->GetResource(&res);ID3D11Texture2D* tex=nullptr;
 if(res)res->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&tex);SafeRelease(res);if(!tex)return;
 D3D11_TEXTURE2D_DESC d={};tex->GetDesc(&d);
 if(d.Width>2048||d.Height>2048||d.ArraySize!=1||d.SampleDesc.Count!=1||BytesPerPixel(d.Format)>8){SafeRelease(tex);return;}
 const UINT sub=sd.Texture2D.MostDetailedMip;
 const UINT w=(std::max)(1u,d.Width>>sub),h=(std::max)(1u,d.Height>>sub),pitch=w*BytesPerPixel(d.Format);
 d.Width=w;d.Height=h;d.MipLevels=1;d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ID3D11Texture2D* copy=nullptr;HRESULT hr=g_dev->CreateTexture2D(&d,nullptr,&copy);
 if(SUCCEEDED(hr)){g_oCopySubresourceRegion(c,copy,0,0,0,0,tex,sub,nullptr);D3D11_MAPPED_SUBRESOURCE m={};hr=g_oMap(c,copy,0,D3D11_MAP_READ,0,&m);
  if(SUCCEEDED(hr)){
   UINT header[]={0x31525457u,w,h,(UINT)sd.Format,pitch,(UINT)g_frame,sub,0};
   std::vector<unsigned char> b(sizeof(header)+(size_t)pitch*h);memcpy(b.data(),header,sizeof(header));
   for(UINT y=0;y<h;++y)memcpy(b.data()+sizeof(header)+(size_t)y*pitch,(const unsigned char*)m.pData+(size_t)y*m.RowPitch,pitch);
   g_oUnmap(c,copy,0);wchar_t name[160];_snwprintf_s(name,160,_TRUNCATE,L"fc3_water_f%ld_%s_reflection.bin",g_frame,label);WriteBlob(name,b);
  }
 }Log("[WATERCAP14] reflection %ls hr=%08X %ux%u format=%u",label,hr,w,h,(UINT)sd.Format);SafeRelease(copy);SafeRelease(tex);
}
void WaterProducerTick(ID3D11DeviceContext* c){
 if(g_waterCaptureCount>=3 && g_frame!=g_waterCaptureFrame)return;
 ID3D11PixelShader* ps=nullptr;c->PSGetShader(&ps,nullptr,nullptr);auto ph=ShaderHash(ps);SafeRelease(ps);
 if(g_frame>g_waterCaptureFrame && g_waterCaptureCount<3 && g_gameplayFrames>600 && GetTickCount64()>=g_waterCaptureNext && WaterKind(ph)>=0){
  g_waterCaptureFrame=g_frame+1;g_waterCaptureNext=GetTickCount64()+6000;g_waterCaptureSeen=0;g_waterDepthSeen=0;g_waterProducerCount=0;++g_waterCaptureCount;
  Log("[WATERCAP14] queued frame %ld capture %d/3",g_waterCaptureFrame,g_waterCaptureCount);
 }
 if(g_frame!=g_waterCaptureFrame||g_waterProducerCount>=3)return;
 ID3D11RenderTargetView* rt=nullptr;c->OMGetRenderTargets(1,&rt,nullptr);ID3D11Resource* res=nullptr;if(rt)rt->GetResource(&res);SafeRelease(rt);
 ID3D11Texture2D* tex=nullptr;if(res)res->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&tex);SafeRelease(res);if(!tex)return;
 D3D11_TEXTURE2D_DESC d={};tex->GetDesc(&d);SafeRelease(tex);
 if(d.Width!=1024||d.Height!=1024||(d.Format!=DXGI_FORMAT_R16G16B16A16_TYPELESS&&d.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT))return;
 wchar_t label[32];_snwprintf_s(label,32,_TRUNCATE,L"producer%d",g_waterProducerCount++);WaterBuffers(c,label);
 ID3D11VertexShader* vs=nullptr;c->VSGetShader(&vs,nullptr,nullptr);Log("[WATERCAP14] %ls frame %ld vs=%016llX ps=%016llX",label,g_frame,ShaderHash(vs),ph);SafeRelease(vs);
}
void CaptureWaterDraw(ID3D11DeviceContext* c,int eye,int variant,bool worldOverride,bool armPalette){
 if(g_frame!=g_waterCaptureFrame)return;
 ID3D11PixelShader* ps=nullptr;c->PSGetShader(&ps,nullptr,nullptr);auto ph=ShaderHash(ps);SafeRelease(ps);int kind=WaterKind(ph);if(kind<0)return;
 UINT bit=1u<<(2*kind+eye);if(g_waterCaptureSeen&bit)return;g_waterCaptureSeen|=bit;
 wchar_t label[40];_snwprintf_s(label,40,_TRUNCATE,L"k%d_e%d",kind,eye);WaterBuffers(c,label);
 ID3D11VertexShader* vs=nullptr;c->VSGetShader(&vs,nullptr,nullptr);
 Log("[WATERCAP14] frame %ld %ls vs=%016llX ps=%016llX variant=%d worldOverride=%d armPalette=%d",g_frame,label,ShaderHash(vs),ph,variant,worldOverride,armPalette);SafeRelease(vs);
 ID3D11ShaderResourceView* reflection=nullptr;c->PSGetShaderResources(kind<2?1:2,1,&reflection);WaterReflection(c,reflection,label);SafeRelease(reflection);
 const UINT dbit=1u<<((kind<2?0:2)+eye);if(g_waterDepthSeen&dbit)return;g_waterDepthSeen|=dbit;
 ID3D11ShaderResourceView* depth=nullptr;c->PSGetShaderResources(kind<2?3:(kind==2?9:8),1,&depth);
 std::vector<float> pixels;UINT w=0,h=0;HRESULT hr=fc3shadow::ReadSample0(c,depth,pixels,w,h);SafeRelease(depth);
 if(SUCCEEDED(hr)){UINT header[]={0x31434853u,w,h,(UINT)g_frame,(UINT)kind,(UINT)eye,0,41};std::vector<unsigned char>b(sizeof(header)+pixels.size()*4);memcpy(b.data(),header,sizeof(header));memcpy(b.data()+sizeof(header),pixels.data(),pixels.size()*4);wchar_t name[160];_snwprintf_s(name,160,_TRUNCATE,L"fc3_water_f%ld_%s_depth.bin",g_frame,label);WriteBlob(name,b);}
 Log("[WATERCAP14] depth %ls hr=%08X %ux%u",label,hr,w,h);
}
