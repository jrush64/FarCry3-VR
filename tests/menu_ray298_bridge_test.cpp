#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_dx11.h"
#include "../src/vr_pad.h"
#include "../src/native_ui_ray298.h"
ULONGLONG tick298=1000;bool focus298=true,menu298=true,hit298=true,inject298=true;std::vector<INPUT> events298;float targetU298=.75f,targetV298=.25f;
HWND window298=(HWND)1;POINT cursor298={200,300};
ULONGLONG Clock298(){return tick298;}
HWND Foreground298(){return focus298?window298:(HWND)2;}
BOOL Client298(HWND,LPRECT r){*r={0,0,1920,1080};return TRUE;}
BOOL Cursor298(LPPOINT p){*p=cursor298;return TRUE;}
BOOL Convert298(HWND,LPPOINT){return TRUE;}
int Metrics298(int which){return which==SM_CXVIRTUALSCREEN?1920:which==SM_CYVIRTUALSCREEN?1080:0;}
UINT Input298(UINT n,LPINPUT inputs,int){if(!inject298)return 0;for(UINT i=0;i<n;++i)events298.push_back(inputs[i]);return n;}
namespace fc3pad {Controllers raw298;bool mouse298=false,trigger298=false;Controllers NativeUiControls(){return raw298;}bool NativeMenuConfirmed(){return menu298;}void SetNativeMousePointer(bool on){mouse298=on;}void SetNativePointerTrigger298(bool on){trigger298=on;}}
namespace fc3xr {bool Running(){return true;}bool NativeMenuRay298(float,float,float* uv){if(!hit298)return false;for(int i=0;i<2;++i){uv[2*i]=targetU298;uv[2*i+1]=targetV298;}return true;}}
void Log(const char*,...){}
bool g_nativeUiMouseEnabled=true,g_nativeUiRayEnabled298=true,g_enabled=true,g_own=false;float g_uiDepth=2,g_worldScale=1;UINT g_bbW=1920,g_bbH=1080;struct {bool open=false;}g_pistolMenu;ID3D11Device* g_dev=nullptr;ID3D11DeviceContext* g_ctx=nullptr;
#define GetTickCount64 Clock298
#define GetForegroundWindow Foreground298
#define GetClientRect Client298
#define GetCursorPos Cursor298
#define ScreenToClient Convert298
#define ClientToScreen Convert298
#define GetSystemMetrics Metrics298
#define SendInput Input298
#include "../src/native_ui_pointer.inl"
void Check298(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
int Count298(DWORD kind){int n=0;for(const auto& e:events298)if(e.mi.dwFlags&kind)++n;return n;}
void Step298(){tick298+=16;events298.clear();ObserveNativeUiPanel(0,1,1,0,0);ObserveNativeUiPanel(1,1,1,0,0);TickNativeUiPointer(window298);}
void TestCursorPixels298(){
 ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;Check298(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP device for shipping native pointer overlay");g_dev=device;g_ctx=context;
 D3D11_TEXTURE2D_DESC desc={};desc.Width=640;desc.Height=360;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* images[2]={};ID3D11RenderTargetView* rtvs[2]={};for(int i=0;i<2;++i){Check298(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&images[i]))&&SUCCEEDED(device->CreateRenderTargetView(images[i],nullptr,&rtvs[i])),"native stereo cursor target");}
 g_nativeCursorVisible=true;g_nativeCursorX=.75f;g_nativeCursorY=.25f;g_nativeRayHover298=true;g_nativeUiPanel[0]={.6f,.6f,.04f,0};g_nativeUiPanel[1]={.6f,.6f,-.04f,0};
 for(int pressed=0;pressed<2;++pressed){g_nativeRayPressed298=pressed!=0;const float black[4]={0,0,0,0};for(auto* view:rtvs)context->ClearRenderTargetView(view,black);RenderNativeUiCursor(images[0],images[1],true);
  for(int eye=0;eye<2;++eye){auto stagingDesc=desc;stagingDesc.BindFlags=0;stagingDesc.Usage=D3D11_USAGE_STAGING;stagingDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;Check298(SUCCEEDED(device->CreateTexture2D(&stagingDesc,nullptr,&staging)),"cursor readback texture");context->CopyResource(staging,images[eye]);D3D11_MAPPED_SUBRESOURCE mapped={};Check298(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped)),"read actual cursor pixels");float x,y;g_nativeUiPanel[eye].Map(.75f,.25f,x,y);const int cx=int(x*640),cy=int(y*360);int whites=0,colours=0;for(int dy=-8;dy<=8;++dy)for(int dx=-8;dx<=8;++dx){const BYTE* pixel=static_cast<const BYTE*>(mapped.pData)+(cy+dy)*mapped.RowPitch+(cx+dx)*4;if(std::abs(dx)<=2&&std::abs(dy)<=2&&pixel[0]>180&&pixel[1]>180&&pixel[2]>180)++whites;if(pixel[0]<150&&pixel[1]>180&&(pressed?pixel[2]<170:pixel[2]>180))++colours;}
   Check298(whites>0&&colours>0,pressed?"both eyes render green pressed ring at the native menu coordinate":"both eyes render cyan aim ring at the actual per-eye menu coordinate");context->Unmap(staging,0);staging->Release();
  }
 }
 ImGui::SetCurrentContext(g_nativeCursorContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_nativeCursorContext);g_nativeCursorContext=nullptr;g_nativeCursorDevice=nullptr;for(int i=0;i<2;++i){rtvs[i]->Release();images[i]->Release();}context->ClearState();context->Release();device->Release();g_dev=nullptr;g_ctx=nullptr;
}
int main(){auto& c=fc3pad::raw298;c.active=true;c.rightTrigger=1;Step298();Check298(g_nativeRayHover298&&!Count298(MOUSEEVENTF_LEFTDOWN)&&fc3pad::trigger298,"held trigger on menu entry cannot click, and cannot also zoom map");
 c.rightTrigger=0;Step298();c.rightTrigger=1;Step298();Check298(Count298(MOUSEEVENTF_LEFTDOWN)==1&&fc3pad::trigger298&&g_nativeRayPressed298,"fresh trigger owns one actual mouse click while pointing");
 for(int n=0;n<12;++n){Step298();Check298(!Count298(MOUSEEVENTF_LEFTDOWN),"holding trigger does not repeat clicks");}
 targetU298=.3f;Step298();Check298(Count298(MOUSEEVENTF_MOVE)==1&&!Count298(MOUSEEVENTF_LEFTDOWN),"held trigger moves native cursor for dragging without a second click");
 hit298=false;Step298();Check298(Count298(MOUSEEVENTF_LEFTUP)==1&&fc3pad::trigger298,"leaving panel releases owned mouse button and retains held-trigger zoom guard");
 c.rightTrigger=0;Step298();Check298(!fc3pad::trigger298,"neutral trigger restores native map zoom while pointing away");
 hit298=true;targetU298=.75f;Step298();c.rightTrigger=1;Step298();focus298=false;Step298();Check298(Count298(MOUSEEVENTF_LEFTUP)==1&&!Count298(MOUSEEVENTF_MOVE)&&!fc3pad::mouse298&&fc3pad::trigger298,"focus loss releases owned click without moving desktop cursor");
 focus298=true;Step298();Check298(!Count298(MOUSEEVENTF_LEFTDOWN),"restoring focus while holding trigger cannot click");c.rightTrigger=0;Step298();
 c.a=true;Step298();Check298(events298.empty(),"stationary pointing injects no competing click or movement when native A confirms");c.a=false;
 c.rightX=.8f;Step298();Check298(!g_nativeRayHover298&&Count298(MOUSEEVENTF_MOVE)==1,"right stick immediately takes over real native cursor");c.rightX=0;Step298();Check298(!g_nativeRayHover298,"neutral stick retains manual cursor instead of snapping back");targetU298=.9f;Step298();Check298(g_nativeRayHover298,"deliberate controller re-aim resumes pointing after stick takeover");
 c.rightGrip=1;Step298();c.rightTrigger=1;Step298();Check298(!g_nativeRayHover298&&!fc3pad::trigger298&&!Count298(MOUSEEVENTF_LEFTDOWN),"right-grip override retains native trigger map zoom without clicking");c.rightTrigger=0;c.rightGrip=0;Step298();
 c.rightTrigger=1;Step298();menu298=false;Step298();Check298(Count298(MOUSEEVENTF_LEFTUP)==1&&!fc3pad::trigger298&&!fc3pad::mouse298,"menu exit releases pointer and stops all menu input leases");
 menu298=true;g_nativeUiRayEnabled298=false;c.rightTrigger=0;c.rightX=.8f;Step298();Check298(!g_nativeRayHover298&&!fc3pad::trigger298&&Count298(MOUSEEVENTF_MOVE)==1,"disabling ray restores existing stick cursor and native trigger routing");
 g_nativeUiRayEnabled298=true;c={};c.active=true;Step298();c.rightTrigger=1;Step298();tick298+=150;Step298();Check298(Count298(MOUSEEVENTF_LEFTUP)==1&&fc3pad::trigger298,"frame hitch cancels owned mouse click without turning held trigger into zoom");Step298();Check298(!Count298(MOUSEEVENTF_LEFTDOWN),"held trigger after hitch cannot click on recovery");c.rightTrigger=0;Step298();
 c.rightTrigger=NAN;Step298();Check298(!Count298(MOUSEEVENTF_LEFTDOWN)&&!g_nativeRayHover298,"invalid trigger cannot click or retain ray interaction");c.rightTrigger=0;Step298();
 c.rightTrigger=1;Step298();tick298+=121;TickNativeUiPointer(window298);Check298(!g_nativeRayHover298,"stale native panel observation disables pointing");c.rightTrigger=0;Step298();
 TestCursorPixels298();
 puts("PASS:298 actual native UI bridge event ordering, trigger arming/release, drag, focus, A, stick fallback, zoom override and exit guards; OS input mocked");
}
