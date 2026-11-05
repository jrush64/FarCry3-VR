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
namespace fc3stereo {void CopyTexture(ID3D11Texture2D* a,ID3D11Texture2D* b){ID3D11Device* d=nullptr;a->GetDevice(&d);ID3D11DeviceContext* c=nullptr;d->GetImmediateContext(&c);c->CopyResource(a,b);c->Release();d->Release();}}
namespace fc3pad {bool fixtureMenu=false,active290=false;bool MenuOpen(){return fixtureMenu;}bool GameplayInputBlocked(){return fixtureMenu;}void SetSelector290(bool on){active290=on;}}
namespace fc3xr {void InteractionPulse285(int){} }
namespace fc3select290 {Inventory fixtureInventory;int fixtureResult=0,fixtureGrab=-1,fixtureCalls=0;bool lease=false;void KeepAlive(bool on){lease=on;}Inventory Read(){auto i=fixtureInventory;i.at=GetTickCount64();return i;}bool Request(const Inventory& shown,int slot){if(!shown.valid||!shown.weapons[slot].entity)return false;fixtureGrab=slot;++fixtureCalls;fixtureResult=0;return true;}int Result(){return fixtureResult;}}
XrView g_views[2]={};bool g_viewsTracked=true,g_referenceValid=true;
XrSession g_session=static_cast<XrSession>(1);XrSpace g_space=static_cast<XrSpace>(2),g_handSpaces[2]={static_cast<XrSpace>(3),static_cast<XrSpace>(4)};XrTime g_displayTime=1;
ID3D11Device* device=nullptr;ID3D11Texture2D* images[2]={};XrPosef hand={{0,0,0,1},{0,0,0}};bool tracked=true;UINT acquired=0,released=0;
XrResult XRAPI_CALL Create(XrSession,const XrSwapchainCreateInfo* info,XrSwapchain* out){Check(info->width==1024&&info->height==640&&info->arraySize==1,"four-card atlas XR dimensions");D3D11_TEXTURE2D_DESC d={};d.Width=1024;d.Height=640;d.ArraySize=d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;for(auto& image:images)Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&image)),"WARP compositor image");*out=static_cast<XrSwapchain>(5);return XR_SUCCESS;}
XrResult XRAPI_CALL Destroy(XrSwapchain){for(auto& i:images){if(i)i->Release();i=nullptr;}return XR_SUCCESS;}
XrResult XRAPI_CALL Enumerate(XrSwapchain,uint32_t capacity,uint32_t* count,XrSwapchainImageBaseHeader* out){*count=2;if(capacity){auto* d=reinterpret_cast<XrSwapchainImageD3D11KHR*>(out);for(UINT i=0;i<2;++i)d[i].texture=images[i];}return XR_SUCCESS;}
XrResult XRAPI_CALL Acquire(XrSwapchain,const XrSwapchainImageAcquireInfo*,uint32_t* index){*index=acquired++%2;return XR_SUCCESS;}
XrResult XRAPI_CALL Wait(XrSwapchain,const XrSwapchainImageWaitInfo*){return XR_SUCCESS;}
XrResult XRAPI_CALL Release(XrSwapchain,const XrSwapchainImageReleaseInfo*){++released;return XR_SUCCESS;}
XrResult XRAPI_CALL Locate(XrSpace,XrSpace,XrTime,XrSpaceLocation* out){out->pose=hand;out->locationFlags=tracked?(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT|XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT):0;return XR_SUCCESS;}
auto p_xrCreateSwapchain=Create;auto p_xrDestroySwapchain=Destroy;auto p_xrEnumerateSwapchainImages=Enumerate;auto p_xrAcquireSwapchainImage=Acquire;auto p_xrWaitSwapchainImage=Wait;auto p_xrReleaseSwapchainImage=Release;auto p_xrLocateSpace=Locate;
#include "../src/xr_selector290.inl"
void Controls(fc3pad::Controllers c){g_selectAllowed290=true;g_selectAllowedAt290=GetTickCount64();SelectorControls290(c);}
int main(){
 auto& inv=fc3select290::fixtureInventory;inv.valid=true;inv.owner=37;const char* names[]={"AK47","DesertEagle","R700","BOW"};
 for(int i=0;i<4;++i){inv.weapons[i].id=100+i;inv.weapons[i].entity=200+i;strcpy_s(inv.weapons[i].name,names[i]);}
 for(int i=0;i<2;++i){g_views[i].pose.orientation.w=1;g_views[i].pose.position={i?.032f:-.032f,1.6f,0};}
 Check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,nullptr)),"software D3D device; no runtime/headset");
 for(int slot=0;slot<4;++slot){fc3pad::Controllers c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);Check(g_selectGesture290.open&&!g_selectGesture290.pending,"shipping XR controller chord opens without selecting");
  Controls(c);Check(g_selectShown290.valid,"shipping UI receives actual inventory snapshot contract");
  XrCompositionLayerQuad q[4]={};Check(SelectorLayers290(q,images[0]?images[0]:([](){D3D11_TEXTURE2D_DESC d={};d.Width=1024;d.Height=640;d.MipLevels=d.ArraySize=1;d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;ID3D11Texture2D* t=nullptr;device->CreateTexture2D(&d,nullptr,&t);return t;})()),"shipping atlas uploads and submits all four preview cards");
  for(int i=0;i<4;++i){const auto centre=fc3select290::Centre(i);const auto relative=testnotes::Rotate(testnotes::Inverse(g_selectPose290.orientation),testnotes::Sub(q[i].pose.position,g_selectPose290.position));Check(fc3select290::Touch({relative.x,relative.y,relative.z})==i&&q[i].subImage.imageRect.offset.x==(i%2)*512&&q[i].subImage.imageRect.offset.y==(i/2)*320&&q[i].eyeVisibility==XR_EYE_VISIBILITY_BOTH,"rendered XR card, atlas crop and reached slot agree");Check(g_selectPreview290[i]!=nullptr,"exact native-name game-model preview loaded");}
  const auto centre=fc3select290::Centre(slot);hand.position=testnotes::Add(g_selectPose290.position,testnotes::Rotate(g_selectPose290.orientation,{centre.x,centre.y,centre.z}));c.rightGrip=0;Controls(c);Check(g_selectGesture290.hover==slot,"tracked right hand highlights the reached card");c.rightGrip=1;Controls(c);
  Check(fc3select290::fixtureGrab==slot&&g_selectGesture290.pending&&g_selectGesture290.open,"grab queues matching native slot and keeps display pending");fc3select290::fixtureResult=1;Controls(c);Check(!g_selectGesture290.open&&!fc3pad::active290,"exact equip confirmation closes all cards");
 }
 fc3pad::Controllers c={};c.active=true;Controls(c);c.a=true;c.rightGrip=1;Controls(c);tracked=false;Controls(c);Check(!g_selectGesture290.open&&!fc3select290::lease,"tracking loss cancels UI and native lease");tracked=true;
 Check(acquired==released,"all XR image acquisitions released");
 std::vector<unsigned> pixels;g_selectShown290=inv;g_selectGesture290.ready=true;g_selectGesture290.hover=1;Check(SelectorPixels290(pixels),"actual shipping four-weapon atlas rasterized for review");
 BITMAPFILEHEADER file={};file.bfType=0x4d42;file.bfOffBits=54;file.bfSize=54+DWORD(pixels.size()*4);BITMAPINFOHEADER bitmap={};bitmap.biSize=40;bitmap.biWidth=1024;bitmap.biHeight=-640;bitmap.biPlanes=1;bitmap.biBitCount=32;FILE* output=nullptr;fopen_s(&output,"evidence/selector-atlas290.bmp","wb");if(output){fwrite(&file,14,1,output);fwrite(&bitmap,40,1,output);fwrite(pixels.data(),4,pixels.size(),output);fclose(output);}
 return 0;
}
