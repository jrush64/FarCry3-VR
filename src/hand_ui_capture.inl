// Replay original Scaleform draws only: no scene pixels and no invented game state.
fc3handui::Settings g_handUiSettings;
bool g_handUiControllerPlacement=false;int g_handUiPlacementPanel=0;
ID3D11Texture2D* g_handUiTexture=nullptr; ID3D11RenderTargetView* g_handUiRtv=nullptr;
ID3D11Resource* g_handUiScreenTarget=nullptr;
bool HandUiKeepVertexShadow(const D3D11_BUFFER_DESC& d){return d.ByteWidth==115200&&(d.BindFlags&D3D11_BIND_VERTEX_BUFFER)&&d.Usage==D3D11_USAGE_DYNAMIC&&(d.CPUAccessFlags&D3D11_CPU_ACCESS_WRITE);}
const GUID kHandUiOriginalVsGuid={0x7344e109,0x5cb1,0x48ef,{0xab,0x34,0xfc,0x30,0x34,0x11,0x20,0x01}};
bool HandUiShader(unsigned long long hash){switch(hash){case 0x89B04C1FA8AE3A7Bull:case 0x93A210224FC36841ull:case 0xA7CBD294B0F541E3ull:case 0xE5DAC12427065E53ull:case 0xE83151C971A9034Cull:case 0xF9CFF0600C999FE7ull:return true;default:return false;}}
struct HandUiOriginalShader {
 ID3D11VertexShader* shader=nullptr;
 ~HandUiOriginalShader(){SafeRelease(shader);}
 void From(ID3D11VertexShader* bound){if(!bound||!HandUiShader(ShaderHash(bound)))return;UINT n=sizeof(shader);if(SUCCEEDED(bound->GetPrivateData(kHandUiOriginalVsGuid,&n,&shader))&&shader)return;n=0;if(FAILED(bound->GetPrivateData(kEjectionBytecodeGuid,&n,nullptr))||!n||n>65536)return;std::vector<unsigned char> bytes(n);if(SUCCEEDED(bound->GetPrivateData(kEjectionBytecodeGuid,&n,bytes.data()))&&SUCCEEDED(g_oCreateVertexShader(g_dev,bytes.data(),n,nullptr,&shader)))bound->SetPrivateDataInterface(kHandUiOriginalVsGuid,shader);}
};
unsigned long long g_handUiReject[7]={},g_handUiCaptures=0,g_handUiCuts=0;
LONG g_handUiClearFrame=-1,g_handUiPoseFrame=-1; float g_handUiPose[16]={};
struct HandUiQuadBounds{float left=0,top=0,right=0,bottom=0;};
int HandUiNativeBounds(ID3D11DeviceContext* context,const DrawCall& call,HandUiQuadBounds& out)
{
 if(call.kind!=1||call.a!=4)return 1;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);UINT n=0;bool ok=cb&&SUCCEEDED(cb->GetPrivateData(kCbCopyGuid,&n,nullptr))&&n>=224&&n<=4096;std::vector<unsigned char> data(n<=4096?n:0);if(ok)ok=SUCCEEDED(cb->GetPrivateData(kCbCopyGuid,&n,data.data()));SafeRelease(cb);if(!ok)return 2;float v[56];std::memcpy(v,data.data(),sizeof(v));
 ID3D11Buffer* vb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);n=0;ok=vb&&stride>=8&&SUCCEEDED(vb->GetPrivateData(kCbCopyGuid,&n,nullptr))&&n<=1024*1024;std::vector<unsigned char> vertices(ok?n:0);if(ok)ok=SUCCEEDED(vb->GetPrivateData(kCbCopyGuid,&n,vertices.data()));SafeRelease(vb);if(!ok)return 3;
 out={1e9f,1e9f,-1e9f,-1e9f};for(UINT i=0;i<4;++i){size_t at=offset+size_t(call.b+i)*stride;if(at+8>n)return 4;float xy[2];std::memcpy(xy,vertices.data()+at,8);const float w=xy[0]*v[52]+xy[1]*v[53]+v[54]+v[55],sx=xy[0]*v[40]+xy[1]*v[41]+v[42]+v[43],sy=xy[0]*v[44]+xy[1]*v[45]+v[46]+v[47];if(!std::isfinite(w)||std::fabs(w)<1e-6f)return 5;const float x=((2*sx-1)/w+1)*.5f,y=((2*sy-1)/w+1)*.5f;if(!std::isfinite(x)||!std::isfinite(y))return 5;out.left=(std::min)(out.left,x);out.right=(std::max)(out.right,x);out.top=(std::min)(out.top,y);out.bottom=(std::max)(out.bottom,y);}return 0;
}
LONG g_handUiMapFrame=-1,g_handUiMaskedFrame=-1;HandUiQuadBounds g_handUiMapBounds;
void HandUiQuadDiagnostic(ID3D11DeviceContext* context,const DrawCall& call,int stage,const HandUiQuadBounds& bounds)
{
 static unsigned samples=0;if(samples++>=48)return;ID3D11Buffer* vb=nullptr,*cb=nullptr;UINT stride=0,offset=0;context->IAGetVertexBuffers(0,1,&vb,&stride,&offset);context->VSGetConstantBuffers(0,1,&cb);D3D11_BUFFER_DESC vd={},cd={};if(vb)vb->GetDesc(&vd);if(cb)cb->GetDesc(&cd);UINT vs=0,cs=0;if(vb)vb->GetPrivateData(kCbCopyGuid,&vs,nullptr);if(cb)cb->GetPrivateData(kCbCopyGuid,&cs,nullptr);SafeRelease(vb);SafeRelease(cb);Log("[HANDUI QUAD] frame=%ld stage=%d draw=%d/%u/start%u VB=%u/usage%u/cpu%u/stride%u/offset%u/shadow%u CB=%u/shadow%u bounds=%.6f/%.6f/%.6f/%.6f",g_frame,stage,call.kind,call.a,call.b,vd.ByteWidth,vd.Usage,vd.CPUAccessFlags,stride,offset,vs,cd.ByteWidth,cs,bounds.left,bounds.top,bounds.right,bounds.bottom);
}
void ObserveHandUiMap(ID3D11DeviceContext* context,const DrawCall& call,unsigned long long vs)
{
 if(vs!=0x93A210224FC36841ull)return;ID3D11PixelShader* ps=nullptr;context->PSGetShader(&ps,nullptr,nullptr);const auto hash=ShaderHash(ps);SafeRelease(ps);if(hash!=0x9F2E25CE646C91C3ull)return;HandUiQuadBounds b;const int stage=HandUiNativeBounds(context,call,b);const auto& p=g_handUiSettings.panels[0];if(!stage&&b.left>=p.left&&b.right<=p.right&&b.top>=p.top&&b.bottom<=p.bottom&&b.right-b.left>.01f&&b.bottom-b.top>.01f){g_handUiMapFrame=g_frame;g_handUiMapBounds=b;}HandUiQuadDiagnostic(context,call,stage,b);
}
#include "hand_ui_map_mask.inl"
bool CaptureHandUi(ID3D11DeviceContext* context,const DrawCall& call,bool running=fc3xr::Running())
{
 if(!g_handUiSettings.enabled){++g_handUiReject[0];return false;}if(!running){++g_handUiReject[1];return false;}if(fc3pad::NativeMenuOpen()){++g_handUiReject[2];return false;}if(context!=g_ctx){++g_handUiReject[3];return false;}
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto nativeVsHash=ShaderHash(vs);HandUiOriginalShader originalShader;originalShader.From(vs);SafeRelease(vs);if(!originalShader.shader){++g_handUiReject[4];return false;}
 ID3D11RenderTargetView* original[8]={};ID3D11DepthStencilView* depth=nullptr;context->OMGetRenderTargets(8,original,&depth);
 ID3D11Resource* target=nullptr;if(original[0])original[0]->GetResource(&target);D3D11_TEXTURE2D_DESC screenDesc={};const bool back=target&&HasFlag(target,kBackbufferGuid);if(back)static_cast<ID3D11Texture2D*>(target)->GetDesc(&screenDesc);if(back&&target!=g_handUiScreenTarget){SafeRelease(g_handUiScreenTarget);g_handUiScreenTarget=target;target->AddRef();}SafeRelease(target);
 if(!back){++g_handUiReject[5];for(auto& v:original)SafeRelease(v);SafeRelease(depth);return false;}
 const DXGI_FORMAT captureFormat=(screenDesc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||screenDesc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||screenDesc.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS)?DXGI_FORMAT_B8G8R8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;
 D3D11_TEXTURE2D_DESC td={};if(g_handUiTexture)g_handUiTexture->GetDesc(&td);
 if(!g_handUiTexture||td.Width!=g_bbW||td.Height!=g_bbH||td.Format!=captureFormat){SafeRelease(g_handUiRtv);SafeRelease(g_handUiTexture);td={};td.Width=g_bbW;td.Height=g_bbH;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=captureFormat;td.BindFlags=D3D11_BIND_RENDER_TARGET;
  if(!td.Width||!td.Height||FAILED(g_dev->CreateTexture2D(&td,nullptr,&g_handUiTexture))||FAILED(g_dev->CreateRenderTargetView(g_handUiTexture,nullptr,&g_handUiRtv))){++g_handUiReject[6];for(auto& v:original)SafeRelease(v);SafeRelease(depth);return false;}g_handUiClearFrame=-1;Log("[HANDUI] capture texture ready frame=%ld sourceFormat=%u captureFormat=%u size=%ux%u",g_frame,screenDesc.Format,captureFormat,g_bbW,g_bbH);}
 ID3D11DepthStencilState* ds=nullptr;UINT stencil=0;context->OMGetDepthStencilState(&ds,&stencil);
 ID3D11BlendState* blend=nullptr;float factors[4];UINT mask;context->OMGetBlendState(&blend,factors,&mask);
 D3D11_BLEND_DESC bd={};if(blend)blend->GetDesc(&bd);bd.RenderTarget[0].BlendEnable=TRUE;bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA;bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;bd.RenderTarget[0].RenderTargetWriteMask=15;
 if(blend){D3D11_BLEND_DESC native={};blend->GetDesc(&native);if(!native.RenderTarget[0].RenderTargetWriteMask){SafeRelease(blend);SafeRelease(ds);for(auto& v:original)SafeRelease(v);SafeRelease(depth);return false;}}
 ObserveHandUiMap(context,call,nativeVsHash);
 if(!blend){bd.RenderTarget[0].SrcBlend=D3D11_BLEND_ONE;bd.RenderTarget[0].DestBlend=D3D11_BLEND_INV_SRC_ALPHA;bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;}
 ID3D11BlendState* alpha=nullptr;g_dev->CreateBlendState(&bd,&alpha);
 const bool previous=g_own;g_own=true;
 if(g_handUiClearFrame!=g_frame){float clear[4]={};context->ClearRenderTargetView(g_handUiRtv,clear);g_handUiClearFrame=g_frame;}
 context->OMSetRenderTargets(1,&g_handUiRtv,nullptr);context->OMSetDepthStencilState(nullptr,0);if(alpha)context->OMSetBlendState(alpha,factors,mask);ID3D11VertexShader* gameVs=nullptr;context->VSGetShader(&gameVs,nullptr,nullptr);context->VSSetShader(originalShader.shader,nullptr,0);RawIssue(context,call);context->VSSetShader(gameVs,nullptr,0);SafeRelease(gameVs);
 context->OMSetRenderTargets(8,original,depth);context->OMSetDepthStencilState(ds,stencil);context->OMSetBlendState(blend,factors,mask);g_own=previous;
 SafeRelease(alpha);SafeRelease(blend);SafeRelease(ds);for(auto& v:original)SafeRelease(v);SafeRelease(depth);
 ++g_handUiCaptures;
 // Native screen rendering is retained until widget source regions are confirmed.
 return false;
}
void PublishHandUi(){static ULONGLONG lastTick=0;const ULONGLONG now=GetTickCount64();const float seconds=lastTick?float(now-lastTick)/1000.f:0;lastTick=now;if(fc3pad::MenuOpen()&&g_handUiControllerPlacement&&g_handUiPlacementPanel>=0&&g_handUiPlacementPanel<3){const auto c=fc3pad::Latest();if(c.active&&c.rightGrip>.7f)fc3handui::AdjustPanel(g_handUiSettings.panels[g_handUiPlacementPanel],c.rightX,c.rightY,c.leftX,c.leftY,c.rightTrigger>.7f,seconds);}MaskHandUiMap();fc3xr::SetHandUi(g_handUiClearFrame>=0?g_handUiTexture:nullptr,(g_handUiPoseFrame==g_frame||(fc3pad::MenuOpen()||fc3pad::NativeMenuOpen())&&g_handUiPoseFrame>=0)?g_handUiPose:nullptr,g_handUiSettings,fc3pad::WheelOwnsGrip());static LONG at=-600;static int lastState=-1;const int state=(g_handUiClearFrame==g_frame?1:0)|(g_handUiPoseFrame==g_frame?2:0)|(fc3xr::HandUiReady()?4:0);if(g_frame-at>=600||state!=lastState){static unsigned transitions=0;if(g_frame-at>=600||transitions++<32){at=g_frame;Log("[HANDUI] publish frame=%ld clearFrame=%ld poseFrame=%ld state=%d captures=%llu cuts=%llu rejects off/run/menu/context/shader/target/resource=%llu/%llu/%llu/%llu/%llu/%llu/%llu",g_frame,g_handUiClearFrame,g_handUiPoseFrame,state,g_handUiCaptures,g_handUiCuts,g_handUiReject[0],g_handUiReject[1],g_handUiReject[2],g_handUiReject[3],g_handUiReject[4],g_handUiReject[5],g_handUiReject[6]);}lastState=state;}}

bool CutHandUi(ID3D11DeviceContext* context,const DrawCall& call,bool ready=fc3xr::HandUiReady())
{
 if(!ready||g_handUiClearFrame!=g_frame||g_handUiPoseFrame!=g_frame||!g_handUiSettings.enabled||fc3pad::GameplayInputBlocked()||call.kind!=1||call.a!=4)return false;
 ID3D11VertexShader* vs=nullptr;context->VSGetShader(&vs,nullptr,nullptr);const auto hash=ShaderHash(vs);SafeRelease(vs);if(!HandUiShader(hash))return false;
 ID3D11RenderTargetView* output=nullptr;context->OMGetRenderTargets(1,&output,nullptr);ID3D11Resource* resource=nullptr;if(output)output->GetResource(&resource);SafeRelease(output);ID3D11Resource* twin=TwinResource(g_handUiScreenTarget,false);const bool screen=resource&&(resource==g_handUiScreenTarget||resource==twin);SafeRelease(resource);SafeRelease(twin);if(!screen)return false;
 HandUiQuadBounds b;const int stage=HandUiNativeBounds(context,call,b);if(stage){HandUiQuadDiagnostic(context,call,stage,b);return false;}const float minx=b.left,maxx=b.right,miny=b.top,maxy=b.bottom;
 const bool on[3]={g_handUiSettings.health,g_handUiSettings.ammo,g_handUiSettings.wheel&&fc3pad::WheelOwnsGrip()};
 for(int i=0;i<3;++i){const auto& p=g_handUiSettings.panels[i];if(on[i]&&fc3handui::Valid(p)&&minx>=p.left&&maxx<=p.right&&miny>=p.top&&maxy<=p.bottom){++g_handUiCuts;return true;}}return false;
}
void LoadHandUiSettings(const wchar_t* path)
{
 g_handUiSettings.levelText345=GetPrivateProfileIntW(L"HandUI",L"LevelText",0,path)!=0;
 g_handUiSettings.glanceOnly285=GetPrivateProfileIntW(L"HandUI",L"GlanceOnly",0,path)!=0;
 g_handUiSettings.enabled=GetPrivateProfileIntW(L"HandUI",L"Enabled",1,path)!=0;g_handUiSettings.health=GetPrivateProfileIntW(L"HandUI",L"Health",1,path)!=0;g_handUiSettings.ammo=GetPrivateProfileIntW(L"HandUI",L"Ammo",1,path)!=0;g_handUiSettings.wheel=GetPrivateProfileIntW(L"HandUI",L"Wheel",1,path)!=0;
 const wchar_t* names[]={L"Health",L"Ammo",L"Wheel"};const wchar_t* fields[]={L"X",L"Y",L"Z",L"Pitch",L"Yaw",L"Roll",L"Width",L"Left",L"Top",L"Right",L"Bottom"};
 for(int i=0;i<3;++i){auto original=g_handUiSettings.panels[i];for(int j=0;j<11;++j){wchar_t key[64],text[64];swprintf_s(key,L"%s%s",names[i],fields[j]);GetPrivateProfileStringW(L"HandUI",key,L"",text,64,path);if(text[0])fc3handui::Field(g_handUiSettings.panels[i],j)=float(wcstod(text,nullptr));}if(!fc3handui::Valid(g_handUiSettings.panels[i]))g_handUiSettings.panels[i]=original;}
}

