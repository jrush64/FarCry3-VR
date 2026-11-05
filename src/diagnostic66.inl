// Observation only. GPU copies are queued before Frame() publishes new tracking.
// Worker owns only copied bytes, never COM objects or gameplay state.
namespace probe66 {
struct Job { std::vector<unsigned char> bytes; wchar_t name[120]={}; };
struct Writer { SRWLOCK lock=SRWLOCK_INIT; HANDLE event=nullptr,thread=nullptr; std::vector<Job> jobs; size_t total=0; LONG drops=0,errors=0; };
Writer* writer=nullptr;
constexpr size_t kBudget=384u*1024*1024;
DWORD WINAPI WriteJobs(void*) {
 HANDLE trace=INVALID_HANDLE_VALUE;
 for(;;){WaitForSingleObject(writer->event,INFINITE);std::vector<Job> jobs;
  AcquireSRWLockExclusive(&writer->lock);jobs.swap(writer->jobs);ReleaseSRWLockExclusive(&writer->lock);
  for(auto& j:jobs){wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s%s",fc3::DataFolder(),j.name);
   HANDLE f=trace;if(j.name[0])f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
   else if(trace==INVALID_HANDLE_VALUE){_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_probe66_%lu.jsonl",fc3::DataFolder(),GetCurrentProcessId());f=trace=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);}
   DWORD n=0;if(f==INVALID_HANDLE_VALUE||!WriteFile(f,j.bytes.data(),DWORD(j.bytes.size()),&n,nullptr)||n!=j.bytes.size())InterlockedIncrement(&writer->errors);
   if(j.name[0]&&f!=INVALID_HANDLE_VALUE)CloseHandle(f);
  }
 }
}
bool Submit(Job&& j){
 if(!writer){writer=new(std::nothrow) Writer;if(!writer)return false;writer->event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(writer->event)writer->thread=CreateThread(nullptr,0,WriteJobs,nullptr,0,nullptr);
  if(!writer->thread)return false;Log("[PROBE66] automatic read-only recording; 384 MiB cap, nonblocking GPU readback and background disk writer");}
 if(!writer->thread)return false;
 AcquireSRWLockExclusive(&writer->lock);const bool ok=writer->jobs.size()<16&&j.bytes.size()<=kBudget-writer->total;
 if(ok){writer->total+=j.bytes.size();writer->jobs.push_back(std::move(j));}else ++writer->drops;
 ReleaseSRWLockExclusive(&writer->lock);if(ok)SetEvent(writer->event);return ok;
}
void Text(const std::string& text){Job j;j.bytes.assign(text.begin(),text.end());Submit(std::move(j));}
std::string Matrix(const float* p,UINT n){std::string s="[";for(UINT i=0;i<n;++i){char t[48];if(std::isfinite(p[i]))_snprintf_s(t,sizeof(t),_TRUNCATE,"%s%.9g",i?",":"",p[i]);else strcpy_s(t,i?",null":"null");s+=t;}return s+"]";}
std::string Matrix(const double* p,UINT n){float f[224]={};n=(std::min)(n,224u);for(UINT i=0;i<n;++i)f[i]=float(p[i]);return Matrix(f,n);}
bool expired=false;
LONG drawFrame=-1;UINT drawCount[3]={};std::string draws;UINT scopeFrames[5]={};
int Profile(){if(HoloProfile131())return g_scopeEyes60[0].spec==5?3:4;return g_weaponMechanicProfile==2?0:g_weaponMechanicProfile==23?1:g_weaponMechanicProfile==35?2:-1;}
bool ScopeWindow(){const int p=Profile();return !expired&&p>=0&&scopeFrames[p]<1200&&(p>=3?HoloAllowed131():fc3pad::Latest().leftTrigger>.2f);}
void Draw(ID3D11DeviceContext* c,const DrawCall& call,int phase,int eye,bool suppressed){
 if(!ScopeWindow()||(scopeFrames[Profile()]>120&&g_frame%4!=0)||c!=g_ctx||call.kind||eye<0||eye>1)return;
 if(drawFrame!=g_frame){drawFrame=g_frame;memset(drawCount,0,sizeof(drawCount));draws.clear();}
 const int bin=phase==0?0:eye+1;if(drawCount[bin]>=12)return;
 const auto geo=NativeWeaponGeometry(c,call);if(!geo.vertexBytes)return;++drawCount[bin];
 ID3D11Buffer* cb[2]={};c->VSGetConstantBuffers(0,2,cb);float w[256]={},cam[224]={},bones[576]={},eyeRows[dxbc::kEyeRows*4]={};
 const UINT nw=ReadCopy(cb[0],w,sizeof(w)),nc=ReadCopy(cb[1],cam,sizeof(cam));const int slot=MuzzlePaletteSlot58(c);
 ID3D11Buffer* palette=nullptr,*eyeCb=nullptr;UINT nb=0,ne=0;
 if(slot>=0){c->VSGetConstantBuffers(UINT(slot),1,&palette);nb=ReadCopy(palette,bones,sizeof(bones));}
 if(phase){c->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeCb);ne=ReadCopy(eyeCb,eyeRows,sizeof(eyeRows));}
 ID3D11VertexShader* vs=nullptr;c->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);
 ID3D11RenderTargetView* target=nullptr;ID3D11DepthStencilView* depth=nullptr;c->OMGetRenderTargets(1,&target,&depth);ID3D11Resource* resource=nullptr;if(target)target->GetResource(&resource);
 char h[800];_snprintf_s(h,sizeof(h),_TRUNCATE,"{\"type\":\"draw\",\"frame\":%ld,\"phase\":%d,\"eye\":%d,\"suppressed\":%d,\"profile\":%ld,\"epoch\":%u,\"geometry\":[%u,%u,%u,%u,%u,%u],\"vs\":\"%016llX\",\"context\":\"%p\",\"target\":\"%p\",\"depth\":\"%p\",\"cb0\":\"%p\",\"cb1\":\"%p\",\"slot\":%d,\"bytes\":[%u,%u,%u,%u],\"world\":",g_frame,phase,eye,suppressed,g_weaponMechanicProfile,g_armPoseEpoch,geo.vertexBytes,geo.indexBytes,geo.stride,geo.offset,geo.start,geo.count,hash,c,resource,depth,cb[0],cb[1],slot,nw,nc,nb,ne);
 draws+=h;draws+=Matrix(w,(std::min)(nw/4,256u))+",\"camera\":"+Matrix(cam,nc/4)+",\"palette\":"+Matrix(bones,(std::min)(nb/4,96u))+",\"eye_rows\":"+Matrix(eyeRows,ne/4)+"}\n";
 SafeRelease(cb[0]);SafeRelease(cb[1]);SafeRelease(palette);SafeRelease(eyeCb);SafeRelease(vs);SafeRelease(target);SafeRelease(depth);SafeRelease(resource);
}
// Fixed little-endian header followed by tightly packed original-format pixels.
// Mosaic1 is five patches (centre/TL/TR/BL/BR); 0 is a centre crop.
// Mosaic2 is the full eye, reduced by GPU mip generation to <=1024px wide.
// Original resource dimensions remain in sourceW/sourceH; no gameplay target is modified.
struct ImageHeader { UINT magic=0x36365046,version=1,frame=0,profile=0,epoch=0,eye=0,stage=0,format=0,sourceW=0,sourceH=0,width=0,height=0,pixelBytes=0,mosaic=0; };
struct Pending { ID3D11Texture2D* texture=nullptr;ID3D11Query* query=nullptr;ImageHeader h;UINT age=0; };
ID3D11Texture2D* pyramid=nullptr;ID3D11ShaderResourceView* pyramidView=nullptr;
Pending pending[8];UINT scopedImages[5]={},overviewImages=0,overviewTurns=0;ULONGLONG lastOverview=0,lastHeartbeat=0;float overviewForward[3]={};LONG overviewProfile=-999;
void ResetGpu(){SafeRelease(pyramidView);SafeRelease(pyramid);for(auto& p:pending){SafeRelease(p.texture);SafeRelease(p.query);p={};}}
void Poll(){for(auto& p:pending){if(!p.texture)continue;BOOL ready=FALSE;const HRESULT hr=g_ctx->GetData(p.query,&ready,sizeof(ready),D3D11_ASYNC_GETDATA_DONOTFLUSH);
  if(hr!=S_OK||!ready){if(FAILED(hr)||++p.age>240){char t[180];_snprintf_s(t,sizeof(t),_TRUNCATE,"{\"type\":\"image_error\",\"frame\":%u,\"stage\":%u,\"eye\":%u,\"hr\":%ld}\n",p.h.frame,p.h.stage,p.h.eye,hr);Text(t);SafeRelease(p.texture);SafeRelease(p.query);p={};}continue;}
  D3D11_MAPPED_SUBRESOURCE map={};const HRESULT mapped=g_oMap(g_ctx,p.texture,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&map);
  if(mapped==DXGI_ERROR_WAS_STILL_DRAWING)continue;
  if(SUCCEEDED(mapped)){Job j;const UINT row=p.h.width*p.h.pixelBytes;j.bytes.resize(sizeof(p.h)+size_t(row)*p.h.height);memcpy(j.bytes.data(),&p.h,sizeof(p.h));for(UINT y=0;y<p.h.height;++y)memcpy(j.bytes.data()+sizeof(p.h)+size_t(y)*row,static_cast<unsigned char*>(map.pData)+size_t(y)*map.RowPitch,row);g_oUnmap(g_ctx,p.texture,0);
   _snwprintf_s(j.name,120,_TRUNCATE,L"fc3_probe66_%lu_f%u_p%u_e%u_s%u.bin",GetCurrentProcessId(),p.h.frame,p.h.profile,p.h.eye,p.h.stage);Submit(std::move(j));
  }else {char t[180];_snprintf_s(t,sizeof(t),_TRUNCATE,"{\"type\":\"image_map_error\",\"frame\":%u,\"hr\":%ld}\n",p.h.frame,mapped);Text(t);}
  SafeRelease(p.texture);SafeRelease(p.query);p={};
 }}
bool Capture(ID3D11Texture2D* source,UINT eye,UINT stage,bool mosaic){
 if(!source||!g_dev||!g_ctx)return false;Pending* p=nullptr;for(auto& s:pending)if(!s.texture){p=&s;break;}if(!p)return false;
 D3D11_TEXTURE2D_DESC td={};source->GetDesc(&td);const UINT bytes=fc3night::PixelBytes(td.Format);
 if(!bytes||td.SampleDesc.Count!=1||td.ArraySize!=1||!td.Width||!td.Height)return false;
 p->h={};p->h.frame=UINT(g_frame);p->h.profile=UINT(g_weaponMechanicProfile);p->h.epoch=g_armPoseEpoch;p->h.eye=eye;p->h.stage=stage;p->h.format=td.Format;p->h.sourceW=td.Width;p->h.sourceH=td.Height;p->h.pixelBytes=bytes;p->h.mosaic=mosaic;
 UINT mip=0;ID3D11Texture2D* copySource=source;
 if(!mosaic&&td.Width>1024&&(td.Format==DXGI_FORMAT_R8G8B8A8_UNORM||td.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS||td.Format==DXGI_FORMAT_B8G8R8A8_UNORM||td.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS||td.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)&&g_oGenerateMips){
  while((td.Width>>mip)>1024)++mip;const DXGI_FORMAT pyramidFormat=(td.Format==DXGI_FORMAT_B8G8R8A8_UNORM||td.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS||td.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;D3D11_TEXTURE2D_DESC old={};if(pyramid)pyramid->GetDesc(&old);
  if(!pyramid||old.Width!=td.Width||old.Height!=td.Height||old.Format!=pyramidFormat){SafeRelease(pyramidView);SafeRelease(pyramid);auto pd=td;pd.Format=pyramidFormat;pd.MipLevels=mip+1;pd.Usage=D3D11_USAGE_DEFAULT;pd.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;pd.CPUAccessFlags=0;pd.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
   if(FAILED(g_dev->CreateTexture2D(&pd,nullptr,&pyramid))||FAILED(g_dev->CreateShaderResourceView(pyramid,nullptr,&pyramidView))){SafeRelease(pyramidView);SafeRelease(pyramid);return false;}}
  g_oCopySubresourceRegion(g_ctx,pyramid,0,0,0,0,source,0,nullptr);g_oGenerateMips(g_ctx,pyramidView);copySource=pyramid;p->h.mosaic=2;
 }
 const UINT side=(std::min)(mosaic?128u:512u,(std::min)(td.Width,td.Height));p->h.width=mosaic?side*5:side;p->h.height=side;
 if(mip){p->h.width=td.Width>>mip;p->h.height=td.Height>>mip;}
 td.Width=p->h.width;td.Height=p->h.height;td.MipLevels=td.ArraySize=1;td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 D3D11_QUERY_DESC q={D3D11_QUERY_EVENT,0};if(FAILED(g_dev->CreateTexture2D(&td,nullptr,&p->texture))||FAILED(g_dev->CreateQuery(&q,&p->query))){SafeRelease(p->texture);SafeRelease(p->query);return false;}
 if(mip){g_oCopySubresourceRegion(g_ctx,p->texture,0,0,0,0,copySource,mip,nullptr);g_ctx->End(p->query);return true;}
 for(UINT i=0;i<(mosaic?5u:1u);++i){const UINT x=i==0?(p->h.sourceW-side)/2:(i==2||i==4?p->h.sourceW-side:0),y=i==0?(p->h.sourceH-side)/2:(i>=3?p->h.sourceH-side:0);D3D11_BOX b={x,y,0,x+side,y+side,1};g_oCopySubresourceRegion(g_ctx,p->texture,0,i*side,0,0,source,0,&b);}g_ctx->End(p->query);return true;
}
bool captureScope=false,captureOverview=false;ULONGLONG started=0;UINT frames=0;
void Before(ID3D11Texture2D* left,ID3D11Texture2D* right){
#ifdef FC3_AMMO_TEST
 if(!GetEnvironmentVariableW(L"FC3_PROBE66_ONLY",nullptr,0))return;
#endif
 Poll();captureScope=captureOverview=false;if(!g_inGameplay||!g_eyeViewsSet||!right)return;
 const ULONGLONG now=GetTickCount64();if(!started){started=now;Text("{\"type\":\"session\",\"build\":135,\"stage0\":\"scene before scope compositing\",\"stage1\":\"eye source before XR copy and next tracking\",\"stage2\":\"current clean scope source\",\"phase0\":\"original constants before VR overrides\",\"phase1\":\"consumed final draw constants; suppressed flag distinguishes skipped native lens\",\"palette_floats_max\":96,\"note\":\"render-camera evidence does not establish native visibility culling camera; eye source excludes separate XR quad layers\"}\n");}if(now-started>20*60*1000||++frames>100000||(writer&&writer->total>=kBudget)){expired=true;return;}
 const int profile=Profile();const bool scoped=ScopeWindow();if(scoped)++scopeFrames[profile];
 captureScope=scoped&&scopeFrames[profile]%120==1&&scopedImages[profile]<8;
 const float* view=g_eyeViews[0].view;const double alignment=view[8]*overviewForward[0]+view[9]*overviewForward[1]+view[10]*overviewForward[2];const bool turned=now-lastOverview>=500&&(alignment<.85||overviewProfile!=g_weaponMechanicProfile);const bool heartbeat=now-lastHeartbeat>=10000;
 captureOverview=!captureScope&&overviewImages<128&&(heartbeat||(overviewTurns<64&&turned));
 if(captureScope)++scopedImages[profile];if(captureOverview){++overviewImages;lastOverview=now;overviewProfile=g_weaponMechanicProfile;for(int i=0;i<3;++i)overviewForward[i]=view[8+i];if(heartbeat)lastHeartbeat=now;else ++overviewTurns;}
 if(captureScope){Capture(left,0,0,false);Capture(right,1,0,false);}
 if(!scoped&&g_frame%6!=0&&!captureOverview)return;
 char h[1200];const auto c=fc3pad::Latest();_snprintf_s(h,sizeof(h),_TRUNCATE,"{\"type\":\"frame\",\"frame\":%ld,\"tick\":%llu,\"profile\":%ld,\"epoch\":%u,\"gameplay\":%d,\"insert\":%d,\"native_menu\":%d,\"two_hand\":%d,\"tracking\":[%d,%d,%d],\"grips_triggers\":[%.6f,%.6f,%.6f,%.6f],\"scope_held\":%d,\"scope_rendered_previous\":%d,\"lens_frames\":[%ld,%ld],\"projection_frames\":[%ld,%ld],\"clean_frame\":%ld,\"clean_draws\":%ld,\"clean_failed\":%d,\"bolt_hold\":%d,\"bolt_chambered\":%d,\"visible_weapon_draws\":%ld,\"aim_active\":%d,\"camera_valid\":%d,\"body_yaw\":%.9g,\"image_scope\":%d,\"image_overview\":%d,\"writer_drops\":%ld,\"writer_errors\":%ld,\"right_hand\":",g_frame,now,g_weaponMechanicProfile,g_armPoseEpoch,g_inGameplay,g_pistolMenu.open,fc3pad::NativeMenuOpen(),g_twoHand,g_eyeViewsSet,g_handRightValid,g_handLeftValid,c.leftGrip,c.rightGrip,c.leftTrigger,c.rightTrigger,g_scopeHeld60,g_scopeRendered60,g_scopeEyes60[0].frame,g_scopeEyes60[1].frame,g_scopeProjectionFrame62[0],g_scopeProjectionFrame62[1],g_scopeCleanFrame61,g_scopeCleanDraws61,g_scopeCleanFailed61,g_r700.hold,g_r700.chambered,g_visibleWeaponDrawsThisFrame,g_aimActive,g_frameCameraValid,g_bodyYaw,captureScope,captureOverview,writer?writer->drops:0,writer?writer->errors:0);
 std::string text=h;text+=Matrix(g_handRight,16)+",\"left_hand\":"+Matrix(g_handLeft,16)+",\"eye_view_left\":"+Matrix(g_eyeViews[0].view,16)+",\"eye_view_right\":"+Matrix(g_eyeViews[1].view,16)+",\"camera_forward\":"+Matrix(g_frameCamera.forward,3)+",\"camera_right\":"+Matrix(g_frameCamera.right,3)+",\"camera_position\":"+Matrix(g_frameCamera.position,3)+",\"camera_up\":"+Matrix(g_frameCamera.up,3)+",\"camera_projection\":"+Matrix(&g_frameCamera.xScale,4)+"}\n";
 if(drawFrame==g_frame)text+=draws;draws.clear();Text(text);
}
void After(ID3D11Texture2D* left,ID3D11Texture2D* right){if(ScopeWindow()){char result[220];_snprintf_s(result,sizeof(result),_TRUNCATE,"{\"type\":\"scope_result\",\"frame\":%ld,\"profile\":%ld,\"held\":%d,\"rendered\":%d}\n",g_frame,g_weaponMechanicProfile,g_scopeHeld60,g_scopeRendered60);Text(result);}if(!captureScope&&!captureOverview)return;
 for(int eye=0;eye<2;++eye){const auto& s=g_scopeEyes60[eye];char h[400];_snprintf_s(h,sizeof(h),_TRUNCATE,"{\"type\":\"lens\",\"frame\":%ld,\"eye\":%d,\"packet_frame\":%ld,\"spec\":%d,\"rendered\":%d,\"scene_axis\":[%.9g,%.9g],\"zoom\":%.9g,\"world\":",g_frame,eye,s.frame,s.spec,g_scopeRendered60,g_scopeSceneAxis62[eye][0],g_scopeSceneAxis62[eye][1],ScopeZoom63());std::string t=h;t+=Matrix(s.draw.anchor.world.m,16)+",\"eye_matrix\":"+Matrix(s.draw.eye,16)+",\"base_eye\":"+Matrix(s.baseEye,16)+",\"presentation\":"+Matrix(g_scopePresentation62[eye],16)+",\"camera\":"+Matrix(s.draw.camera,224);char status72[320];_snprintf_s(status72,sizeof(status72),_TRUNCATE,",\"presentation_frame72\":%ld,\"body_valid72\":%d,\"body_side72\":%d,\"aim_active72\":%d,\"aim_strength72\":%.6f,\"aim_frame72\":%ld,\"aim_raw72\":[%.9g,%.9g,%.9g],\"aim_filtered72\":[%.9g,%.9g,%.9g]",g_scopePresentationFrame72,s.bodyValid72,s.bodySide72,g_scopeAim72.active,(g_scopeAim72.scoped129?g_scopeAimSteady72:(HoloProfile131()?g_holoSteady131:g_weaponSteady129)),g_scopeAim72.frame,g_scopeAim72.raw.x,g_scopeAim72.raw.y,g_scopeAim72.raw.z,g_scopeAim72.filtered.x,g_scopeAim72.filtered.y,g_scopeAim72.filtered.z);t+=status72;char optic131[260];ImVec2 dot131={};const bool dotValid131=HoloProfile131()?HoloDot131(eye,dot131):OpticShotDot131(eye,dot131);_snprintf_s(optic131,sizeof(optic131),_TRUNCATE,",\"scope_eye131\":%d,\"holo_rendered131\":%d,\"dot_valid131\":%d,\"dot_ndc131\":[%.9g,%.9g]",g_scopeEyeState131.selected,g_holoRenderedFrame131==g_frame,dotValid131,dot131.x,dot131.y);t+=optic131;t+="}\n";Text(t);}
 Capture(left,0,1,captureOverview);Capture(right,1,1,captureOverview);
 if(captureScope&&Profile()<3)for(int eye=0;eye<1;++eye){auto* clean=ScopeCleanImage61(eye?right:left);Capture(clean,UINT(eye),2,false);SafeRelease(clean);}
 captureScope=captureOverview=false;
}
} // probe66
