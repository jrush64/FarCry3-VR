// The laser uses the confirmed core weapon draw's shader constants and a snapshot of its depth buffer.
// Missing or incompatible depth fails closed.
// Candidate 206:
//  - the beam is re-anchored to the weapon drawn THIS frame: gun-local muzzle (from the game-thread sample)
//    and the published barrel ray turned by the current drawn frame; only the scene DISTANCE comes from the
//    older game-thread ray. Earlier candidates projected a 1-4 frame old endpoint ("the laser moves late").
//  - the game's 4x MSAA weapon depth is resolved (nearest sample) into a single-sample test surface instead
//    of being rejected; the test runs with the weapon pass's own viewport depth range.
//  - only a depth pass with a real comparison AND depth writes is accepted; the live 205 log showed the good
//    LESS_EQUAL capture overwritten by a later ALWAYS pass, which can never occlude anything.
// Candidate 207: the depth is not copied AT the core draw any more (the suppressor, optics and magazine are
//  drawn after it and were missing: "the laser leaks through silencers"). The core draw only ARMS a pending
//  capture; the copy/resolve runs when that depth target is switched away, cleared, or at the latest before the
//  eye images are finished, i.e. after every attachment of the pass has been drawn.
struct LaserEye205 {
 ID3D11Texture2D* depthTexture=nullptr;ID3D11DepthStencilView* depthView=nullptr; // single-sample test target
 ID3D11Texture2D* sourceCopy=nullptr;ID3D11ShaderResourceView* sourceSrv=nullptr;ID3D11Texture2D* sourceSeen=nullptr; // MSAA resolve inputs
 ID3D11DepthStencilState* depthState=nullptr;ID3D11Device* device=nullptr;
 D3D11_TEXTURE2D_DESC textureDesc={};D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc={};
 D3D11_DEPTH_STENCIL_DESC stateDesc={};
 float camera[224]={},eye[16]={},minDepth=0,maxDepth=1;LONG frame=-1;UINT epoch=0,profile=~0u;bool valid=false;
};
LaserEye205 g_laserEye205[2];
// Armed at the core draw, flushed at the end of the weapon depth pass.
struct LaserPending206 {
 ID3D11Texture2D* source=nullptr;ID3D11Device* device=nullptr;
 D3D11_TEXTURE2D_DESC td={};D3D11_DEPTH_STENCIL_VIEW_DESC vd={};D3D11_DEPTH_STENCIL_DESC ds={};
 DXGI_FORMAT typeless=DXGI_FORMAT_UNKNOWN,srvFormat=DXGI_FORMAT_UNKNOWN;
 float camera[224]={},eye[16]={},minDepth=0,maxDepth=1;LONG frame=-1;UINT epoch=0,profile=~0u;bool reversed=false,armed=false;
};
LaserPending206 g_laserPending206[2];
LONG g_laserFlushes206[3]={}; // flushed by: depth target switch, clear, end of frame (diagnostic)
ID3D11VertexShader* g_laserResolveVs206=nullptr;ID3D11PixelShader* g_laserResolvePs206[2]={};ID3D11DepthStencilState* g_laserResolveDs206=nullptr;ID3D11Device* g_laserResolveDevice206=nullptr;
void ClearLaserResolve206(){SafeRelease(g_laserResolveVs206);for(auto*& p:g_laserResolvePs206)SafeRelease(p);SafeRelease(g_laserResolveDs206);g_laserResolveDevice206=nullptr;}
void ClearLaserEye205(LaserEye205& e){SafeRelease(e.depthView);SafeRelease(e.depthTexture);SafeRelease(e.sourceSrv);SafeRelease(e.sourceCopy);SafeRelease(e.sourceSeen);SafeRelease(e.depthState);SafeRelease(e.device);e={};}
void ClearLaserPending206(LaserPending206& p){SafeRelease(p.source);SafeRelease(p.device);p={};}
bool LaserClip205(const LaserEye205& e,const float* world,float* clip){
 if(!world||!clip)return false;
 double p[4]={double(world[0])-e.camera[152]-e.camera[108],double(world[1])-e.camera[153]-e.camera[109],double(world[2])-e.camera[154]-e.camera[110],1};
 const double len=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);if(!std::isfinite(len)||len<1e-8)return false;
 for(int i=0;i<3;++i)p[i]+=p[i]/len*e.camera[111];
 double native[4]={};for(int r=0;r<4;++r)for(int c=0;c<4;++c)native[r]+=e.camera[r*4+c]*p[c];
 for(int r=0;r<4;++r){double value=0;for(int c=0;c<4;++c)value+=e.eye[r*4+c]*native[c];clip[r]=float(value);if(!std::isfinite(clip[r]))return false;}
 return clip[3]>.025f&&clip[2]/clip[3]>=0&&clip[2]/clip[3]<=1;
}
// Depth formats the resolve can read: the typeless texture format and the shader-readable view format.
bool LaserDepthFormats206(DXGI_FORMAT texture,DXGI_FORMAT& typeless,DXGI_FORMAT& srv){
 switch(texture){
  case DXGI_FORMAT_R24G8_TYPELESS:case DXGI_FORMAT_D24_UNORM_S8_UINT:typeless=DXGI_FORMAT_R24G8_TYPELESS;srv=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;return true;
  case DXGI_FORMAT_R32_TYPELESS:case DXGI_FORMAT_D32_FLOAT:typeless=DXGI_FORMAT_R32_TYPELESS;srv=DXGI_FORMAT_R32_FLOAT;return true;
  case DXGI_FORMAT_R32G8X24_TYPELESS:case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:typeless=DXGI_FORMAT_R32G8X24_TYPELESS;srv=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;return true;
  case DXGI_FORMAT_R16_TYPELESS:case DXGI_FORMAT_D16_UNORM:typeless=DXGI_FORMAT_R16_TYPELESS;srv=DXGI_FORMAT_R16_UNORM;return true;
  default:return false;
 }
}
bool LaserResolveResources206(ID3D11Device* device){
 if(!device)return false;if(g_laserResolveDevice206!=device)ClearLaserResolve206();if(g_laserResolveVs206)return true;
 auto* vs=Native1911Compile48("float4 main(uint id:SV_VertexID):SV_Position{float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);}","vs_5_0");
 auto* nearest=Native1911Compile48("Texture2DMS<float4> depth:register(t0);float main(float4 pos:SV_Position):SV_Depth{uint w,h,n;depth.GetDimensions(w,h,n);float d=1;[loop]for(uint i=0;i<n;++i)d=min(d,depth.Load(int2(pos.xy),i).r);return d;}","ps_5_0");
 auto* farthest=Native1911Compile48("Texture2DMS<float4> depth:register(t0);float main(float4 pos:SV_Position):SV_Depth{uint w,h,n;depth.GetDimensions(w,h,n);float d=0;[loop]for(uint i=0;i<n;++i)d=max(d,depth.Load(int2(pos.xy),i).r);return d;}","ps_5_0");
 D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=true;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_ALWAYS;
 const bool ok=vs&&nearest&&farthest&&SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&g_laserResolveVs206))&&
  SUCCEEDED(device->CreatePixelShader(nearest->GetBufferPointer(),nearest->GetBufferSize(),nullptr,&g_laserResolvePs206[0]))&&
  SUCCEEDED(device->CreatePixelShader(farthest->GetBufferPointer(),farthest->GetBufferSize(),nullptr,&g_laserResolvePs206[1]))&&
  SUCCEEDED(device->CreateDepthStencilState(&ds,&g_laserResolveDs206));
 SafeRelease(vs);SafeRelease(nearest);SafeRelease(farthest);if(!ok){ClearLaserResolve206();return false;}
 g_laserResolveDevice206=device;return true;
}
// Full-screen resolve of the MSAA weapon depth into the single-sample test texture. All touched state is restored.
bool LaserResolve206(ID3D11DeviceContext* c,LaserEye205& e,bool reversed){
 if(!c||!e.sourceSrv||!e.depthView||!LaserResolveResources206(e.device)||!Native1911Resources48(e.device))return false;
 const bool own=g_own;g_own=true;
 ID3D11RenderTargetView* rt[8]={};ID3D11DepthStencilView* depth=nullptr;c->OMGetRenderTargets(8,rt,&depth);
 ID3D11DepthStencilState* ds=nullptr;UINT stencil=0;c->OMGetDepthStencilState(&ds,&stencil);ID3D11RasterizerState* rs=nullptr;c->RSGetState(&rs);D3D11_VIEWPORT viewport[16];UINT count=16;c->RSGetViewports(&count,viewport);
 ID3D11InputLayout* layout=nullptr;ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;D3D11_PRIMITIVE_TOPOLOGY topology;c->IAGetInputLayout(&layout);c->IAGetVertexBuffers(0,1,&vb,&stride,&offset);c->IAGetPrimitiveTopology(&topology);
 ID3D11ShaderResourceView* srv0=nullptr;c->PSGetShaderResources(0,1,&srv0);
 #define LASER_SAVE_SHADER206(stage,type) type* saved##stage=nullptr;ID3D11ClassInstance* instances##stage[256]={};UINT n##stage=256;c->stage##GetShader(&saved##stage,instances##stage,&n##stage)
 LASER_SAVE_SHADER206(VS,ID3D11VertexShader);LASER_SAVE_SHADER206(PS,ID3D11PixelShader);LASER_SAVE_SHADER206(GS,ID3D11GeometryShader);LASER_SAVE_SHADER206(HS,ID3D11HullShader);LASER_SAVE_SHADER206(DS,ID3D11DomainShader);
 c->OMSetRenderTargets(0,nullptr,e.depthView);c->OMSetDepthStencilState(g_laserResolveDs206,0);c->RSSetState(g_nativeGuideRs48);
 D3D11_VIEWPORT vp={0,0,float(e.textureDesc.Width),float(e.textureDesc.Height),0,1};c->RSSetViewports(1,&vp);
 ID3D11Buffer* noVb=nullptr;UINT zero=0;c->IASetInputLayout(nullptr);c->IASetVertexBuffers(0,1,&noVb,&zero,&zero);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 c->VSSetShader(g_laserResolveVs206,nullptr,0);c->PSSetShader(g_laserResolvePs206[reversed?1:0],nullptr,0);c->GSSetShader(nullptr,nullptr,0);c->HSSetShader(nullptr,nullptr,0);c->DSSetShader(nullptr,nullptr,0);
 c->PSSetShaderResources(0,1,&e.sourceSrv);
 if(g_oDraw)g_oDraw(c,3,0);else c->Draw(3,0);
 ID3D11ShaderResourceView* noSrv=nullptr;c->PSSetShaderResources(0,1,&noSrv);
 #define LASER_RESTORE_SHADER206(stage) c->stage##SetShader(saved##stage,instances##stage,n##stage);SafeRelease(saved##stage);for(UINT i=0;i<n##stage;++i)SafeRelease(instances##stage[i])
 LASER_RESTORE_SHADER206(VS);LASER_RESTORE_SHADER206(PS);LASER_RESTORE_SHADER206(GS);LASER_RESTORE_SHADER206(HS);LASER_RESTORE_SHADER206(DS);
 #undef LASER_SAVE_SHADER206
 #undef LASER_RESTORE_SHADER206
 c->IASetInputLayout(layout);c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetPrimitiveTopology(topology);c->RSSetViewports(count,viewport);c->RSSetState(rs);c->OMSetDepthStencilState(ds,stencil);c->OMSetRenderTargets(8,rt,depth);c->PSSetShaderResources(0,1,&srv0);
 SafeRelease(layout);SafeRelease(vb);SafeRelease(rs);SafeRelease(ds);SafeRelease(depth);for(auto*& r:rt)SafeRelease(r);SafeRelease(srv0);g_own=own;return true;
}
// The pass is over: copy/resolve the armed depth (now including every attachment drawn after the core mesh).
void FlushLaserPending206(ID3D11DeviceContext* context,int reason){
 for(int eye=0;eye<2;++eye){auto& p=g_laserPending206[eye];if(!p.armed)continue;
  const bool current=p.frame==g_frame;auto& e=g_laserEye205[eye];
  if(!current||!context){ClearLaserPending206(p);continue;} // a stale arm never produces a depth for a different frame
  if(e.device!=p.device||!e.depthTexture||memcmp(&e.textureDesc,&p.td,sizeof(p.td))||memcmp(&e.viewDesc,&p.vd,sizeof(p.vd)))ClearLaserEye205(e);
  if(!e.depthTexture){
   D3D11_TEXTURE2D_DESC copy=p.td;copy.BindFlags=D3D11_BIND_DEPTH_STENCIL;copy.CPUAccessFlags=0;copy.Usage=D3D11_USAGE_DEFAULT;copy.MiscFlags=0;copy.SampleDesc.Count=1;copy.SampleDesc.Quality=0;
   D3D11_DEPTH_STENCIL_VIEW_DESC view=p.vd;if(p.td.SampleDesc.Count>1){view.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;view.Flags=0;view.Texture2D.MipSlice=0;}
   if(FAILED(p.device->CreateTexture2D(&copy,nullptr,&e.depthTexture))||FAILED(p.device->CreateDepthStencilView(e.depthTexture,&view,&e.depthView))){ClearLaserEye205(e);ClearLaserPending206(p);continue;}
   if(p.td.SampleDesc.Count>1&&!(p.td.BindFlags&D3D11_BIND_SHADER_RESOURCE)){ // the game's surface cannot be sampled: resolve from a sampleable copy
    D3D11_TEXTURE2D_DESC ms=p.td;ms.Format=p.typeless;ms.BindFlags=D3D11_BIND_SHADER_RESOURCE;ms.CPUAccessFlags=0;ms.Usage=D3D11_USAGE_DEFAULT;ms.MiscFlags=0;
    if(FAILED(p.device->CreateTexture2D(&ms,nullptr,&e.sourceCopy))){ClearLaserEye205(e);ClearLaserPending206(p);continue;}
   }
   e.textureDesc=p.td;e.viewDesc=p.vd;e.device=p.device;p.device->AddRef();
  }
  D3D11_DEPTH_STENCIL_DESC ds=p.ds;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;ds.StencilEnable=false;
  if(!e.depthState||memcmp(&e.stateDesc,&ds,sizeof(ds))){SafeRelease(e.depthState);if(FAILED(p.device->CreateDepthStencilState(&ds,&e.depthState))){e.valid=false;ClearLaserPending206(p);continue;}e.stateDesc=ds;}
  bool resolved=false;const char* how="copied";
  if(p.td.SampleDesc.Count==1){const bool own=g_own;g_own=true;context->CopyResource(e.depthTexture,p.source);g_own=own;resolved=true;}
  else{
   ID3D11Texture2D* sampled=e.sourceCopy?e.sourceCopy:p.source;
   if(e.sourceCopy){const bool own=g_own;g_own=true;context->CopyResource(e.sourceCopy,p.source);g_own=own;}
   if(e.sourceSeen!=sampled){SafeRelease(e.sourceSrv);SafeRelease(e.sourceSeen);D3D11_SHADER_RESOURCE_VIEW_DESC sd={};sd.Format=p.srvFormat;sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DMS;
    if(SUCCEEDED(p.device->CreateShaderResourceView(sampled,&sd,&e.sourceSrv))){e.sourceSeen=sampled;sampled->AddRef();}}
   resolved=e.sourceSrv&&LaserResolve206(context,e,p.reversed);how=e.sourceCopy?"copied+resolved":"resolved";
   if(!resolved){static LONG failures=0;if(InterlockedIncrement(&failures)<=6)Log("[LASER206] MSAA depth resolve failed eye%d frame%ld samples%u format%u srv%d bind%u",eye,g_frame,p.td.SampleDesc.Count,UINT(p.td.Format),e.sourceSrv?1:0,p.td.BindFlags);}
  }
  if(resolved){memcpy(e.camera,p.camera,sizeof(e.camera));memcpy(e.eye,p.eye,sizeof(e.eye));e.minDepth=p.minDepth;e.maxDepth=p.maxDepth;e.frame=p.frame;e.epoch=p.epoch;e.profile=p.profile;e.valid=true;
   if(reason>=0&&reason<3)++g_laserFlushes206[reason];
   static LONG captures=0;if(InterlockedIncrement(&captures)<=8)Log("[LASER207] weapon depth %s eye%d frame%ld profile%u %ux%u samples%u format%u compare%u viewportDepth %.3f-%.3f at %s",how,eye,g_frame,e.profile,p.td.Width,p.td.Height,p.td.SampleDesc.Count,UINT(p.vd.Format),UINT(p.ds.DepthFunc),p.minDepth,p.maxDepth,reason==0?"depth target switch":reason==1?"depth clear":"end of frame");}
  else e.valid=false;
  ClearLaserPending206(p);
 }
}
// Every draw: when the game's depth target is no longer the armed one, the weapon pass has ended.
void LaserDrawObserve206(ID3D11DeviceContext* context){
 const auto& p=g_laserPending206[0];if(!p.armed||!context)return;
 if(p.frame!=g_frame){FlushLaserPending206(nullptr,-1);return;}
 ID3D11DepthStencilView* view=nullptr;context->OMGetRenderTargets(0,nullptr,&view);ID3D11Resource* resource=nullptr;if(view)view->GetResource(&resource);
 const bool same=resource&&static_cast<ID3D11Resource*>(p.source)==resource;SafeRelease(resource);SafeRelease(view);
 if(!same)FlushLaserPending206(context,0);
}
// Before a clear of the armed depth: the attachments are drawn, the content is about to go.
void LaserClearObserve206(ID3D11DeviceContext* context,ID3D11DepthStencilView* view){
 if(!view||!context)return;bool armed=false;for(const auto& p:g_laserPending206)armed=armed||p.armed;if(!armed)return;
 ID3D11Resource* resource=nullptr;view->GetResource(&resource);bool hit=false;
 for(const auto& p:g_laserPending206)if(p.armed&&resource&&static_cast<ID3D11Resource*>(p.source)==resource)hit=true;
 SafeRelease(resource);if(hit)FlushLaserPending206(context,1);
}
void CaptureLaserEye205(ID3D11DeviceContext* context,const DrawCall& call,ID3D11Buffer* buffer,int variant,int eye){
 if(!g_laserMode194||eye<0||eye>1||!context||!buffer||!g_muzzleLaserFrame203.valid||g_muzzleLaserFrame203.frame!=g_frame)return;
 if(g_laserEye205[eye].valid&&g_laserEye205[eye].frame==g_frame)return; // the first accepted weapon depth pass of the frame owns the test
 if(g_laserPending206[eye].armed&&g_laserPending206[eye].frame==g_frame)return;
 Mat4 move={};V3 root={};if(!MuzzleDrawTransport58(context,call,buffer,variant,move,root))return;
 if(BarrelFamily206(int(g_muzzleRender58.profile))!=BarrelFamily206(int(g_muzzleLaserFrame203.profile))||g_muzzleRender58.epoch!=g_muzzleLaserFrame203.epoch)return;
 ID3D11DepthStencilView* sourceView=nullptr;context->OMGetRenderTargets(0,nullptr,&sourceView);if(!sourceView)return;
 ID3D11Resource* resource=nullptr;sourceView->GetResource(&resource);ID3D11Texture2D* source=nullptr;
 if(resource)resource->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&source));SafeRelease(resource);
 ID3D11Device* device=nullptr;context->GetDevice(&device);D3D11_TEXTURE2D_DESC td={};D3D11_DEPTH_STENCIL_VIEW_DESC vd={};sourceView->GetDesc(&vd);if(source)source->GetDesc(&td);
 ID3D11DepthStencilState* bound=nullptr;UINT stencilRef=0;context->OMGetDepthStencilState(&bound,&stencilRef);D3D11_DEPTH_STENCIL_DESC ds={};if(bound)bound->GetDesc(&ds);SafeRelease(bound);
 D3D11_VIEWPORT viewports[16]={};UINT viewportCount=16;context->RSGetViewports(&viewportCount,viewports);const float minDepth=viewportCount?viewports[0].MinDepth:0.f,maxDepth=viewportCount?viewports[0].MaxDepth:1.f;
 ID3D11Buffer* cameraCb=nullptr,*eyeCb=nullptr;context->VSGetConstantBuffers(1,1,&cameraCb);context->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeCb);
 float camera[224]={},rows[dxbc::kEyeRows*4]={};const UINT cameraBytes=ReadCopy(cameraCb,camera,sizeof(camera)),eyeBytes=ReadCopy(eyeCb,rows,sizeof(rows));SafeRelease(cameraCb);SafeRelease(eyeCb);
 const bool compareOk=ds.DepthEnable&&ds.DepthWriteMask==D3D11_DEPTH_WRITE_MASK_ALL&&(ds.DepthFunc==D3D11_COMPARISON_LESS||ds.DepthFunc==D3D11_COMPARISON_LESS_EQUAL||ds.DepthFunc==D3D11_COMPARISON_GREATER||ds.DepthFunc==D3D11_COMPARISON_GREATER_EQUAL);
 const bool reversed=ds.DepthFunc==D3D11_COMPARISON_GREATER||ds.DepthFunc==D3D11_COMPARISON_GREATER_EQUAL;
 DXGI_FORMAT typeless=DXGI_FORMAT_UNKNOWN,srvFormat=DXGI_FORMAT_UNKNOWN;const bool formatOk=LaserDepthFormats206(td.Format,typeless,srvFormat);
 const bool usable=source&&device&&td.Width==g_bbW&&td.Height==g_bbH&&td.ArraySize==1&&td.MipLevels==1&&compareOk&&cameraBytes==sizeof(camera)&&eyeBytes==sizeof(rows)&&(td.SampleDesc.Count==1||formatOk)&&maxDepth>minDepth;
 if(!usable){static LONG rejects=0;const LONG n=InterlockedIncrement(&rejects);if(n<=12||n%600==0)Log("[LASER206] depth pass skipped eye%d frame%ld source%d size%ux%u vs %ux%u samples%u format%u depth%d compare%u writes%u camera%u eyeCb%u viewportDepth %.3f-%.3f (needs LESS/GREATER family with depth writes)",eye,g_frame,source?1:0,td.Width,td.Height,g_bbW,g_bbH,td.SampleDesc.Count,UINT(td.Format),ds.DepthEnable?1:0,UINT(ds.DepthFunc),UINT(ds.DepthWriteMask),cameraBytes,eyeBytes,minDepth,maxDepth);SafeRelease(source);SafeRelease(sourceView);SafeRelease(device);return;}
 // Arm: the copy waits for the end of this depth pass so later attachment draws (suppressor, optics) are included.
 auto& p=g_laserPending206[eye];ClearLaserPending206(p);
 p.source=source;p.device=device;p.td=td;p.vd=vd;p.ds=ds;p.typeless=typeless;p.srvFormat=srvFormat;memcpy(p.camera,camera,sizeof(p.camera));for(int i=0;i<16;++i)p.eye[i]=rows[i]+(i%5==0?1.f:0.f);
 p.minDepth=minDepth;p.maxDepth=maxDepth;p.frame=g_frame;p.epoch=g_armPoseEpoch;p.profile=g_muzzleRender58.profile;p.reversed=reversed;p.armed=true;
 SafeRelease(sourceView); // source and device references are owned by the pending record until the flush
}
void LaserGateLog206(const char* gate){
 static ULONGLONG next=0;static LONG lines=0;const ULONGLONG now=GetTickCount64();
 if(lines>=80||now<next)return;next=now+2000;++lines;
 fc3aim::LaserSample194 s={};const bool have=fc3aim::ReadLaser194(s);const auto& f=g_muzzleLaserFrame203;
 Log("[LASER206] not drawn: %s (frame%ld, mode%d, aim %d, sample %d age%llu hit %d dist %.2f local %d profile%u epoch%u; confirmed frame%ld/%ld epoch%u/%u profile%u barrel %d; eyes valid %d/%d frames %ld/%ld; flushes switch%ld clear%ld frameEnd%ld)",
  gate,g_frame,g_laserMode194,g_aimActive?1:0,have?1:0,have?GetTickCount64()-s.ms:0ULL,s.hit?1:0,s.distance,s.localValid?1:0,s.profile,s.epoch,f.frame,g_frame,f.epoch,g_armPoseEpoch,f.profile,f.barrelValid?1:0,
  g_laserEye205[0].valid?1:0,g_laserEye205[1].valid?1:0,g_laserEye205[0].frame,g_laserEye205[1].frame,g_laserFlushes206[0],g_laserFlushes206[1],g_laserFlushes206[2]);
}
void RenderLaserDepth205(ID3D11Texture2D* left,ID3D11Texture2D* right){
 FlushLaserPending206(g_ctx,2); // no switch or clear seen: the pass content is still intact at the end of the frame
 if(!g_laserMode194||!g_enabled||!g_inGameplay||g_flatViewActive||!g_eyeViewsSet||g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::WheelOwnsGrip()||ThrowableOwns144()||!left||!right||!g_ctx)return;
 if(!g_aimActive){LaserGateLog206("aim inactive");return;}
 fc3aim::LaserSample194 sample={};bool have=fc3aim::ReadLaser194(sample);
#ifdef FC3_AMMO_TEST
 if(g_laserFixture194){sample=*g_laserFixture194;have=true;}
#endif
 if(!have){LaserGateLog206("no fresh scene sample");return;}
 const auto& f=g_muzzleLaserFrame203;
 if(!f.valid||f.frame!=g_frame||f.epoch!=g_armPoseEpoch){LaserGateLog206("no confirmed core draw this frame");return;}
 if(BarrelFamily206(int(sample.profile))!=BarrelFamily206(int(f.profile))||sample.epoch!=g_armPoseEpoch){LaserGateLog206("sample belongs to another weapon/epoch");return;}
 // Current drawn weapon frame: the muzzle and the barrel ray are rigid with it.
 const Mat4 drawn=Multiply(f.move,f.native);
 const V3 start=sample.localValid?Apply(drawn,{sample.muzzleLocal[0],sample.muzzleLocal[1],sample.muzzleLocal[2]}):Apply(f.move,{sample.nativeMuzzle[0],sample.nativeMuzzle[1],sample.nativeMuzzle[2]});
 V3 direction=f.barrelValid?f.barrelWorld:Unit(Sub({sample.point[0],sample.point[1],sample.point[2]},start));
 if(!std::isfinite(start.x)||!std::isfinite(start.y)||!std::isfinite(start.z)||Length(direction)<.5){LaserGateLog206("no barrel direction");return;}
 direction=Unit(direction);const double range=std::isfinite(sample.distance)&&sample.distance>.05?sample.distance:25.;
 const V3 end=Add(start,Scale(direction,range)),ring=Add(start,Scale(direction,(std::max)(.05,range-.02))); // ring 2 cm short of the surface: no z-fight
 const float startF[3]={float(start.x),float(start.y),float(start.z)},endF[3]={float(end.x),float(end.y),float(end.z)},ringF[3]={float(ring.x),float(ring.y),float(ring.z)};
 ID3D11Texture2D* images[2]={left,right};int drawnEyes=0;const char* gate="eye depth not captured this frame";
 for(int eye=0;eye<2;++eye){auto& e=g_laserEye205[eye];if(!e.valid||e.frame!=g_frame||e.epoch!=g_armPoseEpoch||BarrelFamily206(int(e.profile))!=BarrelFamily206(int(sample.profile))||!e.depthView||!e.depthState)continue;
  D3D11_TEXTURE2D_DESC image={};images[eye]->GetDesc(&image);if(image.Width!=e.textureDesc.Width||image.Height!=e.textureDesc.Height||image.SampleDesc.Count!=1){gate="eye image size differs from the weapon depth";continue;}
  float a[4]={},b[4]={},r[4]={};if(!LaserClip205(e,startF,a)||!LaserClip205(e,endF,b)||!LaserClip205(e,ringF,r)){gate="beam outside the weapon clip volume";continue;}
  float lines[18][4]={};UINT count=0;const auto append=[&](const float* v){memcpy(lines[count++],v,4*sizeof(float));};
  if(g_laserMode194==2||g_laserMode194==3){append(a);append(b);}
  if(sample.hit&&(g_laserMode194==1||g_laserMode194==3)){
   // 207: a round ring on the headset, not a pixel circle. The eye image's pixels are not square in angle:
   // horizontal and vertical tangent spans differ from the pixel aspect (same correction as ReticleRadii).
   const auto& ev=g_eyeViews[eye];const double sx=ev.tanRight-ev.tanLeft,sy=ev.tanUp-ev.tanDown;
   const float ry=7.f*2/image.Height,rx=(sx>0&&sy>0&&std::isfinite(sx)&&std::isfinite(sy))?float(ry*(sy/sx)):7.f*2/image.Width;
   for(int i=0;i<8;++i){const float t0=float(i*6.28318530718/8),t1=float((i+1)*6.28318530718/8);float v0[4],v1[4];memcpy(v0,r,sizeof(v0));memcpy(v1,r,sizeof(v1));v0[0]+=std::cos(t0)*rx*r[3];v0[1]+=std::sin(t0)*ry*r[3];v1[0]+=std::cos(t1)*rx*r[3];v1[1]+=std::sin(t1)*ry*r[3];append(v0);append(v1);}
  }
  if(count){DrawNative1911Guide48(g_ctx,images[eye],&lines[0][0],count,3+ReticlePresetIndex52(g_redDotColour52),e.depthView,e.depthState,e.minDepth,e.maxDepth);++drawnEyes;
   static LONG draws=0;if(eye==0&&InterlockedIncrement(&draws)<=12)Log("[LASER206] drawn eye%d frame%ld profile%u mode%d muzzle %.2f/%.2f/%.2f range %.2f beam %.3f/%.3f to %.3f/%.3f depth %.4f/%.4f (viewport %.3f-%.3f)",eye,g_frame,e.profile,g_laserMode194,start.x,start.y,start.z,range,a[0]/a[3],a[1]/a[3],b[0]/b[3],b[1]/b[3],a[2]/a[3],b[2]/b[3],e.minDepth,e.maxDepth);}
 }
 if(!drawnEyes)LaserGateLog206(gate);
}
