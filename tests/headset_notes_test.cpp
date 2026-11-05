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
#include "../src/test_notes_ui.h"
void Check(bool b,const char* s){if(!b){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
namespace fc3 {const wchar_t* DataFolder(){static wchar_t p[1024]={};if(!p[0]){DWORD n=GetEnvironmentVariableW(L"FC3VR_TEST_DATA",p,1000);if(n==0||n>=1000)lstrcpyW(p,L".\\");else if(p[n-1]!=L'\\')lstrcatW(p,L"\\");}return p;}} // set FC3VR_TEST_DATA to the folder holding the captured test assets
namespace fc3stereo {float WorldScale(){return 1.25f;} void CopyTexture(ID3D11Texture2D* a,ID3D11Texture2D* b){ID3D11Device* d=nullptr;a->GetDevice(&d);ID3D11DeviceContext* c=nullptr;d->GetImmediateContext(&c);c->CopyResource(a,b);c->Release();d->Release();}}
void Log(const char*,...){}
bool Ok(XrResult r,const char*){return XR_SUCCEEDED(r);}
DXGI_FORMAT FormatFamily(DXGI_FORMAT f){return f==DXGI_FORMAT_R8G8B8A8_UNORM?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_TYPELESS;}
XrView g_views[2]={};bool g_viewsValid=true;
bool g_referenceValid=true;float g_referenceYaw=.7f;XrVector3f g_referencePosition={.5f,1.4f,-.2f};
XrSession g_session=static_cast<XrSession>(1);XrSpace g_space=static_cast<XrSpace>(2),g_handSpaces[2]={static_cast<XrSpace>(3),static_cast<XrSpace>(4)};XrTime g_displayTime=1;
DXGI_FORMAT g_format=DXGI_FORMAT_R8G8B8A8_UNORM;
ID3D11Device* device=nullptr;ID3D11Texture2D* images[2]={};XrPosef left={{0,0,0,1},{0,0,0}};bool tracked=true;UINT acquired=0,released=0;
XrResult XRAPI_CALL Create(XrSession,const XrSwapchainCreateInfo* info,XrSwapchain* out)
{
    Check(((info->width==1024&&info->height==640)||(info->width==128&&info->height==64))&&info->arraySize==1,"complete native quad swapchain dimensions");
    D3D11_TEXTURE2D_DESC d={};d.Width=info->width;d.Height=info->height;d.ArraySize=1;d.MipLevels=1;d.Format=static_cast<DXGI_FORMAT>(info->format);d.SampleDesc.Count=1;
    for(auto& i:images)Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&i)),"WARP compositor image");*out=static_cast<XrSwapchain>(5);return XR_SUCCESS;
}
XrResult XRAPI_CALL Destroy(XrSwapchain){for(auto& i:images){if(i)i->Release();i=nullptr;}return XR_SUCCESS;}
XrResult XRAPI_CALL Enumerate(XrSwapchain,uint32_t capacity,uint32_t* count,XrSwapchainImageBaseHeader* out)
{*count=2;if(capacity){auto* d=reinterpret_cast<XrSwapchainImageD3D11KHR*>(out);for(UINT i=0;i<2;++i)d[i].texture=images[i];}return XR_SUCCESS;}
XrResult XRAPI_CALL Acquire(XrSwapchain,const XrSwapchainImageAcquireInfo*,uint32_t* index){*index=acquired++%2;return XR_SUCCESS;}
XrResult XRAPI_CALL Wait(XrSwapchain,const XrSwapchainImageWaitInfo*){return XR_SUCCESS;}
XrResult XRAPI_CALL Release(XrSwapchain,const XrSwapchainImageReleaseInfo*){++released;return XR_SUCCESS;}
XrResult XRAPI_CALL Locate(XrSpace,XrSpace,XrTime,XrSpaceLocation* out){out->pose=left;out->locationFlags=tracked?(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT):0;return XR_SUCCESS;}
auto p_xrCreateSwapchain=Create;auto p_xrDestroySwapchain=Destroy;auto p_xrEnumerateSwapchainImages=Enumerate;auto p_xrAcquireSwapchainImage=Acquire;auto p_xrWaitSwapchainImage=Wait;auto p_xrReleaseSwapchainImage=Release;auto p_xrLocateSpace=Locate;
namespace fc3pad {bool fixtureMenu=false;bool MenuOpen(){return fixtureMenu;}}
#include "../src/xr_options_menu.inl"
#include "../src/xr_test_notes.inl"
#include "../src/xr_reload_markers.inl"
int main()
{
 fc3pad::Controllers c={};c.active=true;c.leftGrip=1;c.leftTrigger=1;c.x=c.y=true;
 g_noteVisible=true;g_noteOptions=true;g_noteDragging=true;
 NotesControls(c);
 Check(c.leftGrip==1&&c.leftTrigger==1&&c.x&&c.y,"disabled notes never consume wheel or reload controls");
 Check(!g_noteVisible&&!g_noteDragging,"disabled notes cannot remain visible or drag");
 XrCompositionLayerQuad quad={};g_noteVisible=true;
 Check(!NotesLayer(quad,nullptr),"even programmatic show cannot submit instruction overlay");
 // Exercise the actual restored options submission, including uploaded pixels.
 D3D_FEATURE_LEVEL level={};ID3D11DeviceContext* context=nullptr;
 Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)),"WARP options device");
 D3D11_TEXTURE2D_DESC td={};td.Width=128;td.Height=64;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;
 std::vector<unsigned> pixels(128*64,0xff3050a0);D3D11_SUBRESOURCE_DATA data={pixels.data(),128*4,0};
 Check(SUCCEEDED(device->CreateTexture2D(&td,&data,&g_optionsTexture)),"options source texture");
 fc3pad::fixtureMenu=true;
 Check(NotesLayer(quad,g_optionsTexture)&&quad.eyeVisibility==XR_EYE_VISIBILITY_BOTH&&released==1,"Insert options still submit stereo quad with instructions disabled");
 D3D11_TEXTURE2D_DESC readDesc=td;readDesc.Usage=D3D11_USAGE_STAGING;readDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ID3D11Texture2D* readback=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&readDesc,nullptr,&readback)),"options readback");
 context->CopyResource(readback,g_optionsImages[0]);D3D11_MAPPED_SUBRESOURCE mapped={};
 Check(SUCCEEDED(context->Map(readback,0,D3D11_MAP_READ,0,&mapped))&&*static_cast<unsigned*>(mapped.pData)==pixels[0],"Insert options pixels reach compositor image");context->Unmap(readback,0);
 fc3pad::fixtureMenu=false;Check(!NotesLayer(quad,g_optionsTexture),"closing Insert does not enable old instruction popup");
 readback->Release();g_optionsTexture->Release();g_optionsTexture=nullptr;Destroy(g_optionsChain);context->Release();device->Release();
 puts("PASS: instructions disabled and controls preserved; Insert options stereo quad and pixels restored");
 return 0;
}
