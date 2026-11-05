#define WIN32_LEAN_AND_MEAN
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include "../src/vr_pad.h"
#include "../src/xr_hand_tracking.h"
#include "../src/test_notes_ui.h"
#include "../src/selector290.h"
#include "../src/selector_policy290.h"
void Check(bool b,const char* s){if(!b){fprintf(stderr,"FAIL: %s\n",s);exit(1);}printf("PASS: %s\n",s);}
void Log(const char*,...){}
namespace fc3 {const wchar_t* DataFolder(){static wchar_t p[1024]={};if(!p[0]){DWORD n=GetEnvironmentVariableW(L"FC3VR_TEST_DATA",p,1000);if(n==0||n>=1000)lstrcpyW(p,L".\\");else if(p[n-1]!=L'\\')lstrcatW(p,L"\\");}return p;}} // set FC3VR_TEST_DATA to the folder holding the captured test assets
namespace fc3stereo {bool RunOverlay291(bool(*render)(void*),void* data){return render(data);}void CopyTexture(ID3D11Texture2D* a,ID3D11Texture2D* b){ID3D11Device* d=nullptr;a->GetDevice(&d);ID3D11DeviceContext* c=nullptr;d->GetImmediateContext(&c);c->CopyResource(a,b);c->Release();d->Release();}}
namespace fc3pad {bool fixtureMenu=false,active290=false;bool MenuOpen(){return fixtureMenu;}bool GameplayInputBlocked(){return fixtureMenu;}void SetSelector290(bool on){active290=on;}}
namespace fc3xr {void InteractionPulse285(int){} }
namespace fc3select290 {Inventory fixtureInventory;int fixtureResult=0,fixtureGrab=-1,fixtureCalls=0;bool lease=false;void KeepAlive(bool on){lease=on;}Inventory Read(){auto i=fixtureInventory;i.at=GetTickCount64();return i;}bool Request(const Inventory& shown,int slot){if(!shown.valid||!shown.weapons[slot].entity)return false;fixtureGrab=slot;++fixtureCalls;fixtureResult=0;return true;}int Result(){return fixtureResult;}}
XrView g_views[2]={};bool g_viewsTracked=true,g_referenceValid=true;
XrSession g_session=static_cast<XrSession>(1);XrSpace g_space=static_cast<XrSpace>(2),g_handSpaces[2]={static_cast<XrSpace>(3),static_cast<XrSpace>(4)};XrTime g_displayTime=1;
UINT g_width=640,g_height=640;DXGI_FORMAT g_format=DXGI_FORMAT_R8G8B8A8_UNORM;
ID3D11Device* device=nullptr;ID3D11Texture2D* images[2]={};XrPosef hand={{0,0,0,1},{0,0,0}};bool tracked=true;UINT acquired=0,released=0;
XrResult XRAPI_CALL Create(XrSession,const XrSwapchainCreateInfo* info,XrSwapchain* out){Check(info->width==640&&info->height==640&&info->arraySize==2,"actual3D projection has two independent eye slices");D3D11_TEXTURE2D_DESC d={};d.Width=640;d.Height=640;d.ArraySize=2;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_RENDER_TARGET;for(auto& image:images)Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&image)),"WARP stereo compositor images");*out=static_cast<XrSwapchain>(5);return XR_SUCCESS;}
XrResult XRAPI_CALL Destroy(XrSwapchain){for(auto& i:images){if(i)i->Release();i=nullptr;}return XR_SUCCESS;}
XrResult XRAPI_CALL Enumerate(XrSwapchain,uint32_t capacity,uint32_t* count,XrSwapchainImageBaseHeader* out){*count=2;if(capacity){auto* d=reinterpret_cast<XrSwapchainImageD3D11KHR*>(out);for(UINT i=0;i<2;++i)d[i].texture=images[i];}return XR_SUCCESS;}
XrResult XRAPI_CALL Acquire(XrSwapchain,const XrSwapchainImageAcquireInfo*,uint32_t* index){*index=acquired++%2;return XR_SUCCESS;}
XrResult XRAPI_CALL Wait(XrSwapchain,const XrSwapchainImageWaitInfo*){return XR_SUCCESS;}
XrResult XRAPI_CALL Release(XrSwapchain,const XrSwapchainImageReleaseInfo*){++released;return XR_SUCCESS;}
XrResult XRAPI_CALL Locate(XrSpace,XrSpace,XrTime,XrSpaceLocation* out){out->pose=hand;out->locationFlags=tracked?(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT):0;return XR_SUCCESS;}
auto p_xrCreateSwapchain=Create;auto p_xrDestroySwapchain=Destroy;auto p_xrEnumerateSwapchainImages=Enumerate;auto p_xrAcquireSwapchainImage=Acquire;auto p_xrWaitSwapchainImage=Wait;auto p_xrReleaseSwapchainImage=Release;auto p_xrLocateSpace=Locate;
#include "../src/xr_selector290.inl"
void Controls(fc3pad::Controllers c){g_selectAllowed290=true;g_selectAllowedAt290=GetTickCount64();SelectorControls290(c);}
std::vector<unsigned> Pixels291(ID3D11Texture2D* texture,int eye){D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);td.ArraySize=1;td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&staging)),"pixel readback staging");ID3D11DeviceContext* c=nullptr;device->GetImmediateContext(&c);c->CopySubresourceRegion(staging,0,0,0,0,texture,eye,nullptr);D3D11_MAPPED_SUBRESOURCE map={};Check(SUCCEEDED(c->Map(staging,0,D3D11_MAP_READ,0,&map)),"read actual rendered eye pixels");std::vector<unsigned> p(640*640);for(int y=0;y<640;++y)memcpy(p.data()+y*640,static_cast<char*>(map.pData)+y*map.RowPitch,640*4);c->Unmap(staging,0);c->Release();staging->Release();return p;}
void Save291(const std::vector<unsigned>& p){BITMAPFILEHEADER h={};h.bfType=0x4d42;h.bfOffBits=54;h.bfSize=54+DWORD(p.size()*4);BITMAPINFOHEADER b={};b.biSize=40;b.biWidth=640;b.biHeight=-640;b.biPlanes=1;b.biBitCount=32;std::vector<unsigned> bgra=p;for(auto& v:bgra)v=(v&0xff00ff00)|((v&0xff)<<16)|((v>>16)&0xff);FILE* f=nullptr;fopen_s(&f,"evidence/floating-weapons291-left.bmp","wb");if(f){fwrite(&h,14,1,f);fwrite(&b,40,1,f);fwrite(bgra.data(),4,bgra.size(),f);fclose(f);}}
int main(){
 auto& inv=fc3select290::fixtureInventory;inv.valid=true;inv.owner=37;const char* names[]={"M133.Bull","HUNTER.Cannon","LPO-50","KW1911.Shadow"};
 for(int i=0;i<4;++i){inv.weapons[i].id=100+i;inv.weapons[i].entity=200+i;strcpy_s(inv.weapons[i].name,names[i]);}
 for(int i=0;i<2;++i){g_views[i].pose.orientation.w=1;g_views[i].pose.position={i?.032f:-.032f,1.6f,0};g_views[i].fov={-.8f,.8f,.8f,-.8f};}
 ID3D11DeviceContext* immediate=nullptr;Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&immediate)),"software3D device; no headset/runtime");
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=640;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;ID3D11Texture2D* source=nullptr;ID3D11RenderTargetView* original=nullptr;device->CreateTexture2D(&td,nullptr,&source);device->CreateRenderTargetView(source,nullptr,&original);immediate->OMSetRenderTargets(1,&original,nullptr);
 for(int slot=0;slot<4;++slot){fc3pad::Controllers c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);Check(g_selectGesture290.open&&!g_selectGesture290.pending,"grip+A opens without an accidental grab");
  c.a=false;c.rightGrip=0;Controls(c);Controls(c);Check(g_selectGesture290.open&&g_selectGesture290.ready,"release A and grip; floating weapons stay open");
  for(int i=0;i<4;++i)Check(g_selectMesh291[i].valid&&g_selectMesh291[i].vertices.size()>100,"exact signature weapon name loads actual triangle geometry");
  XrCompositionLayerProjection layer={};XrCompositionLayerProjectionView views[2]={};Check(SelectorLayer291(layer,views,source),"actual meshes render into stereo projection, no cards or quads");
  Check(layer.viewCount==2&&views[0].subImage.imageArrayIndex==0&&views[1].subImage.imageArrayIndex==1&&views[0].pose.position.x==g_views[0].pose.position.x&&views[1].fov.angleRight==g_views[1].fov.angleRight,"submitted eyes carry exact rendered pose and FOV");
  ID3D11RenderTargetView* restored=nullptr;immediate->OMGetRenderTargets(1,&restored,nullptr);Check(restored==original,"game immediate-context render target restored after mesh command list");if(restored)restored->Release();
  if(slot==0){auto left=Pixels291(images[(acquired-1)%2],0),right=Pixels291(images[(acquired-1)%2],1);size_t opaque=0,different=0;for(size_t i=0;i<left.size();++i){if(left[i]>>24)++opaque;if(left[i]!=right[i])++different;}
   Check(opaque>1000&&opaque<left.size()/3&&different>500,"actual3D silhouettes with transparent surroundings and distinct stereo eye pixels");
   for(int i=0;i<4;++i){float matrix[20];SelectorMatrix291(i,0,matrix);const int x=int((matrix[3]/matrix[15]+1)*320),y=int((1-matrix[7]/matrix[15])*320);int pixels=0;for(int yy=(std::max)(0,y-75);yy<(std::min)(640,y+75);++yy)for(int xx=(std::max)(0,x-100);xx<(std::min)(640,x+100);++xx)if(left[yy*640+xx]>>24)++pixels;Check(pixels>60,"all four floating weapon locations contain rendered model pixels");}
   Save291(left);
  }
  const auto vertex=g_selectMesh291[slot].vertices[g_selectMesh291[slot].vertices.size()/2].p;const auto centre=fc3select290::Centre(slot);const auto point=fc3select291::Add(centre,{vertex[0],vertex[1],vertex[2]});hand.position=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,{point.x,point.y,point.z}));Controls(c);Check(g_selectGesture290.hover==slot,"tracked controller touches actual chosen mesh surface");
  c.rightGrip=1;Controls(c);Check(fc3select290::fixtureGrab==slot&&g_selectGesture290.pending&&g_selectGesture290.open,"one grip on model queues exact slot without holding A");fc3select290::fixtureResult=1;Controls(c);Check(!g_selectGesture290.open&&!fc3pad::active290,"exact equipped identity closes models");Check(!SelectorLayer291(layer,views,source),"closed selector submits no models");
 }
 fc3pad::Controllers c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);c.a=false;Controls(c);tracked=false;Controls(c);Check(!g_selectGesture290.open&&!fc3select290::lease,"tracking loss cancels model and native leases");
 Check(acquired==released,"every3D XR image acquisition released");SelectorGpuRelease291();immediate->OMSetRenderTargets(0,nullptr,nullptr);original->Release();source->Release();immediate->Release();device->Release();return 0;
}
