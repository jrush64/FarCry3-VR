// Read-only, user-triggered effect census. No shader, camera or input correction.
struct EnvironmentCapture252 {
 unsigned burst=0,samples=0,count[3]={};
 unsigned opaque[3]={},effects[3]={},offscreen[3]={},skipped[3]={};
 LONG frame=-1;ULONGLONG next=0;bool active=false;
 unsigned long long vs[3][144]={},ps[3][144]={},material[3][144]={};
 uintptr_t target[3][144]={};
 ULONGLONG session=0;HANDLE metadata=INVALID_HANDLE_VALUE;
 size_t bytes=0;
} g_environment252;
void EnvironmentLog255(const char* format,...){
 char line[2048];va_list args;va_start(args,format);vsnprintf_s(line,sizeof(line),_TRUNCATE,format,args);va_end(args);Log("%s",line);
 if(g_environment252.metadata!=INVALID_HANDLE_VALUE){DWORD written=0;WriteFile(g_environment252.metadata,line,static_cast<DWORD>(strlen(line)),&written,nullptr);WriteFile(g_environment252.metadata,"\r\n",2,&written,nullptr);}
}
fc3grid258::Cache g_gridCache258;
LONG g_gridAutoFrame258=-1,g_gridFrame258=-1;unsigned g_gridAutoCount258=0,g_gridFrames258=0,g_gridCount258=0;
unsigned long long g_gridHashes258[16]={};uintptr_t g_gridTargets258[16]={};
bool g_gridAfter258[16]={};
ULONGLONG g_gridSession258=0;
HANDLE g_gridLog258=INVALID_HANDLE_VALUE;
void CaptureSceneGrid258(ID3D11DeviceContext* c,int eye,bool before=false){
 if(eye!=0||(g_environment252.frame!=g_frame&&g_gridAutoFrame258!=g_frame))return;
 if(g_gridFrame258!=g_frame){if(g_gridFrames258>=12)return;g_gridFrame258=g_frame;++g_gridFrames258;g_gridCount258=0;memset(g_gridAfter258,0,sizeof(g_gridAfter258));}
 ID3D11PixelShader* ps=nullptr;c->PSGetShader(&ps,nullptr,nullptr);const auto hash=ShaderHash(ps);SafeRelease(ps);
 // Major scene boundaries, before the HUD. Avoid spending the budget on
 // per-material opaque draws; the final eye BMP remains the end-point image.
 bool selected=hash==0||WaterKind(hash)>=0;
 for(auto h:{0x2169ACC3A5D77635ull,0x3BAD3248C3991F75ull,0xA12636B84720D852ull,0x11C5DEB352337775ull,0xF8F31518956C98DBull,0x9C490CA65CF2CA77ull,0x3E11D1C4732A23E3ull,0xC19BFBC792F0F843ull,0xBA81B5F7771569B2ull,0x6771A60187C7067Full,0x057EE039AB2B7A3Eull,0xE20ABD31F16921EAull,0x83A6FCAF6854B4EDull,0xB42B55CCCF14B3CEull})selected|=hash==h;
 if(!selected)return;
 ID3D11RenderTargetView* rt=nullptr;c->OMGetRenderTargets(1,&rt,nullptr);if(!rt)return;
 ID3D11Resource* resource=nullptr;rt->GetResource(&resource);ID3D11Texture2D* tex=nullptr;resource->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&tex);
 const auto target=reinterpret_cast<uintptr_t>(resource);SafeRelease(resource);D3D11_TEXTURE2D_DESC desc={};if(tex)tex->GetDesc(&desc);SafeRelease(tex);
 if(desc.Width!=g_bbW||desc.Height!=g_bbH||(desc.Format!=DXGI_FORMAT_R16G16B16A16_TYPELESS&&desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT)){SafeRelease(rt);return;}
 unsigned draw=g_gridCount258;
 for(unsigned i=0;i<g_gridCount258;++i)if(g_gridHashes258[i]==hash&&g_gridTargets258[i]==target){draw=i;break;}
 if(draw<g_gridCount258){if(before||g_gridAfter258[draw]){SafeRelease(rt);return;}}
 else {if(!before||g_gridCount258>=16){SafeRelease(rt);return;}++g_gridCount258;g_gridHashes258[draw]=hash;g_gridTargets258[draw]=target;}
 if(!before)g_gridAfter258[draw]=true;
 std::vector<float> pixels;const HRESULT hr=fc3grid258::Read(g_gridCache258,c,rt,pixels,desc);SafeRelease(rt);
 if(!g_gridSession258){g_gridSession258=GetTickCount64();wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_grid258_p%u_t%llu.txt",fc3::DataFolder(),GetCurrentProcessId(),g_gridSession258);g_gridLog258=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);}
 if(SUCCEEDED(hr)){
  const UINT header[]={0x31524753,64,36,desc.Width,desc.Height,UINT(desc.Format),desc.SampleDesc.Count,UINT(g_frame)};
  std::vector<unsigned char> bytes(sizeof(header)+pixels.size()*4);memcpy(bytes.data(),header,sizeof(header));memcpy(bytes.data()+sizeof(header),pixels.data(),pixels.size()*4);
  wchar_t name[180];_snwprintf_s(name,180,_TRUNCATE,L"fc3_grid258_p%u_t%llu_f%ld_d%u_ps%016llX_%s.bin",GetCurrentProcessId(),g_gridSession258,g_frame,draw,hash,before?L"before":L"after");WriteBlob(name,bytes);
 }
 EnvironmentLog255("[GRID258] frame%ld index%u phase%s eye0 ps%016llX target%p format%u samples%u hr%08X camera %.5f/%.5f/%.5f; colour sample0 around first draw of this family, not a complete pass or compositor image",g_frame,draw,before?"before":"after",hash,reinterpret_cast<void*>(target),UINT(desc.Format),desc.SampleDesc.Count,hr,g_frameCamera.forward[0],g_frameCamera.forward[1],g_frameCamera.forward[2]);
 if(g_gridLog258!=INVALID_HANDLE_VALUE){char line[512];const int length=_snprintf_s(line,sizeof(line),_TRUNCATE,"frame%ld index%u phase%s eye0 ps%016llX target%p format%u samples%u hr%08X camera %.6f/%.6f/%.6f\r\n",g_frame,draw,before?"before":"after",hash,reinterpret_cast<void*>(target),UINT(desc.Format),desc.SampleDesc.Count,hr,g_frameCamera.forward[0],g_frameCamera.forward[1],g_frameCamera.forward[2]);DWORD written=0;if(length>0)WriteFile(g_gridLog258,line,DWORD(length),&written,nullptr);}
}
void BeginEnvironment252(){
 auto& s=g_environment252;
 if(s.active||s.frame>=g_frame){EnvironmentLog255("[ENV255] capture already active; press once per location");return;}
 if(s.burst>=4){Log("[ENV252] four capture windows completed; restart for more");return;}
 if(!s.session){s.session=GetTickCount64();wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_env255_p%u_t%llu.txt",fc3::DataFolder(),GetCurrentProcessId(),s.session);s.metadata=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);}
 ++s.burst;s.samples=0;s.next=GetTickCount64();s.active=true;
 EnvironmentLog255("[ENV255] window%u armed: four sampled frames over nine seconds; metadata=%s",s.burst,s.metadata==INVALID_HANDLE_VALUE?"FAILED":"open");
 fc3xr::Buzz(1);
}
void ScheduleEnvironment252(){
 auto& s=g_environment252;
 if(!s.active||!g_inGameplay||fc3pad::NativeMenuOpen()||GetTickCount64()<s.next)return;
 s.frame=g_frame+1;s.next=GetTickCount64()+3000;++s.samples;
 memset(s.count,0,sizeof(s.count));s.bytes=0;
 memset(s.opaque,0,sizeof(s.opaque));memset(s.effects,0,sizeof(s.effects));memset(s.offscreen,0,sizeof(s.offscreen));memset(s.skipped,0,sizeof(s.skipped));
 if(s.samples==4)s.active=false;
 EnvironmentLog255("[ENV255] window%u sample%u frame%ld queued",s.burst,s.samples,s.frame);
}
void CaptureEnvironmentDraw252(ID3D11DeviceContext* c,int eye,int variant){
 CaptureSceneGrid258(c,eye);
 auto& s=g_environment252;if(s.frame!=g_frame||eye< -1||eye>1)return;
 const int stage=eye+1;if(s.count[stage]>=144||s.bytes>=8u*1024*1024){++s.skipped[stage];return;}
 D3D11_VIEWPORT vp={};UINT n=1;c->RSGetViewports(&n,&vp);
 const bool main=n&&UINT(vp.Width+.5f)==g_bbW&&UINT(vp.Height+.5f)==g_bbH;
 ID3D11PixelShader* ps=nullptr;ID3D11VertexShader* vs=nullptr;
 c->PSGetShader(&ps,nullptr,nullptr);c->VSGetShader(&vs,nullptr,nullptr);
 const auto ph=ShaderHash(ps),vh=ShaderHash(vs);
 ID3D11BlendState* blend=nullptr;c->OMGetBlendState(&blend,nullptr,nullptr);
 D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);SafeRelease(blend);
 bool blended=false;for(const auto& target:bd.RenderTarget)blended|=target.BlendEnable!=FALSE;
 const bool water=WaterKind(ph)>=0||ph==0xB5850815B1E93395ull||ph==0xF9BD64B91671E5FEull;
 // Keep main scene capacity separate from reflection/shadow producers, and
 // include opaque unknown materials instead of guessing that water must blend.
 unsigned& bucket=!main?s.offscreen[stage]:blended||water?s.effects[stage]:s.opaque[stage];
 const unsigned limit=!main?16:64;if(bucket>=limit){++s.skipped[stage];SafeRelease(vs);SafeRelease(ps);return;}
 ID3D11RenderTargetView* rt=nullptr;ID3D11DepthStencilView* ds=nullptr;c->OMGetRenderTargets(1,&rt,&ds);
 ID3D11Resource* resource=nullptr;if(rt)rt->GetResource(&resource);const uintptr_t target=reinterpret_cast<uintptr_t>(resource);SafeRelease(resource);
 ID3D11Buffer* cb0=nullptr;c->PSGetConstantBuffers(0,1,&cb0);BYTE materialBytes[1024]={};const UINT copied=ReadCopy(cb0,materialBytes,sizeof(materialBytes));SafeRelease(cb0);const auto material=copied?Fnv64(materialBytes,copied):0;
 for(unsigned i=0;i<s.count[stage];++i)if(s.vs[stage][i]==vh&&s.ps[stage][i]==ph&&s.material[stage][i]==material&&s.target[stage][i]==target){SafeRelease(vs);SafeRelease(ps);SafeRelease(rt);SafeRelease(ds);return;}
 ++bucket;
 const unsigned draw=s.count[stage]++;s.vs[stage][draw]=vh;s.ps[stage][draw]=ph;
 s.material[stage][draw]=material;s.target[stage][draw]=target;
 DumpShaderBytecode(vs,"vs");DumpShaderBytecode(ps,"ps");SafeRelease(vs);SafeRelease(ps);
 char views[512]={};DescribeView(rt,views,sizeof(views));Append(views,sizeof(views)," depth:");DescribeView(ds,views,sizeof(views));SafeRelease(rt);SafeRelease(ds);
 EnvironmentLog255("[ENV255] window%u sample%u frame%ld eye%d draw%u variant%d vs%016llX ps%016llX blended%d water%d main%d target%p material%016llX vp%.0fx%.0f %s",s.burst,s.samples,g_frame,eye,draw,variant,vh,ph,blended,water,main,reinterpret_cast<void*>(target),material,vp.Width,vp.Height,views);
 for(UINT slot=0;slot<16;++slot){ID3D11ShaderResourceView* srv=nullptr;c->PSGetShaderResources(slot,1,&srv);if(srv){char text[256]={};DescribeView(srv,text,sizeof(text));ID3D11Resource* input=nullptr;srv->GetResource(&input);EnvironmentLog255("[ENV255] frame%ld eye%d draw%u t%u resource%p %s",g_frame,eye,draw,slot,input,text);SafeRelease(input);}SafeRelease(srv);}
 for(int shader=0;shader<2;++shader)for(UINT slot:{0u,1u,2u,3u,4u,5u,13u}){
  ID3D11Buffer* cb=nullptr;if(shader)c->PSGetConstantBuffers(slot,1,&cb);else c->VSGetConstantBuffers(slot,1,&cb);
  D3D11_BUFFER_DESC d={};if(cb)cb->GetDesc(&d);std::vector<unsigned char> bytes;
  if(cb&&d.ByteWidth<=16384&&s.bytes+d.ByteWidth<=8u*1024*1024&&ReadBytes(c,cb,bytes)){
   wchar_t name[180];_snwprintf_s(name,180,_TRUNCATE,L"fc3_env255_p%u_t%llu_w%u_s%u_f%ld_e%d_d%u_%s_cb%u.bin",GetCurrentProcessId(),s.session,s.burst,s.samples,g_frame,eye,draw,shader?L"ps":L"vs",slot);WriteBlob(name,bytes);s.bytes+=bytes.size();
  }SafeRelease(cb);
 }
}
