#include "native_ui_ray298.h"
fc3ui::RayState298 g_nativeRayState298;
bool g_nativeRayHover298=false,g_nativeRayPressed298=false;
ULONGLONG g_nativeUiPanelAt298[2]={};
fc3ui::PointerState g_nativePointerState;
bool g_nativeCursorVisible=false;
float g_nativeCursorX=.5f,g_nativeCursorY=.5f;
fc3ui::PanelTransform g_nativeUiPanel[2];
void ObserveNativeUiPanel(int eye,float sx,float sy,float ox,float oy)
{
 if(eye>=0&&eye<2){g_nativeUiPanel[eye]={sx,sy,ox,oy};g_nativeUiPanelAt298[eye]=GetTickCount64();}
}
bool SendNativePointer(DWORD flags,LONG x=0,LONG y=0){INPUT input={};input.type=INPUT_MOUSE;input.mi.dwFlags=flags;input.mi.dx=x;input.mi.dy=y;return SendInput(1,&input,sizeof(input))==1;}
void TickNativeUiPointer(HWND window)
{
 static ULONGLONG prior=0;const auto now=GetTickCount64();const float dt=prior?float(now-prior)*.001f:0;prior=now;
 const auto controls=fc3pad::NativeUiControls();RECT client={};POINT point={};
 const bool menu=g_nativeUiMouseEnabled&&window&&fc3xr::Running()&&fc3pad::NativeMenuConfirmed()&&
  GetClientRect(window,&client)&&client.right>1&&client.bottom>1;
 const bool ready=menu&&controls.active&&GetForegroundWindow()==window&&GetCursorPos(&point)&&ScreenToClient(window,&point);
 // Seed from the actual native cursor on entry; afterwards retain subpixel
 // motion and don't let the game's per-frame mouse recenter erase stick travel.
 static bool haveNativePosition=false;
 if(!menu)haveNativePosition=false;
 const bool entering=ready&&!haveNativePosition;
 static float nativeX=0,nativeY=0;
 if(entering){nativeX=float(point.x);nativeY=float(point.y);haveNativePosition=true;}
 float screen298[4]={},targetU298=0,targetV298=0;
 const bool fresh298=g_nativeUiPanelAt298[0]&&g_nativeUiPanelAt298[1]&&now-g_nativeUiPanelAt298[0]<=120&&now-g_nativeUiPanelAt298[1]<=120;
 const bool hit298=ready&&g_nativeUiRayEnabled298&&fresh298&&g_bbW>1&&g_bbH>1&&fc3xr::NativeMenuRay298(g_uiDepth/(std::max)(.1f,g_worldScale),float(g_bbH)/float(g_bbW),screen298)&&fc3ui::NativeRayHit298(g_nativeUiPanel,screen298,targetU298,targetV298);
 const auto ray298=g_nativeRayState298.Step(ready&&g_nativeUiRayEnabled298,hit298,targetU298,targetV298,controls.rightTrigger,controls.rightGrip,controls.rightX,controls.rightY,dt);
 auto step=g_nativePointerState.Step(ready,controls.rightStickClick||ray298.click,ray298.point?0:controls.rightX,ray298.point?0:controls.rightY,dt,ray298.point?targetU298*client.right:nativeX,ray298.point?targetV298*client.bottom:nativeY,float(client.right),float(client.bottom));
 if(ray298.point){step.x=(std::max)(0.f,(std::min)(float(client.right-1),ray298.u*client.right));step.y=(std::max)(0.f,(std::min)(float(client.bottom-1),ray298.v*client.bottom));step.move=std::lround(step.x)!=std::lround(nativeX)||std::lround(step.y)!=std::lround(nativeY);}
 g_nativeRayHover298=ray298.point;g_nativeRayPressed298=ray298.click;
 fc3pad::SetNativePointerTrigger298(menu&&g_nativeUiRayEnabled298&&ray298.reserveTrigger);
 if(ready){nativeX=step.x;nativeY=step.y;}
 bool sent=true;
 // Idle mouse recenter injection changes the game's input mode every frame,
 // competing with native A confirmation. Submit movement only for pointer use.
 if(ready&&(step.move||entering||step.down)){
  POINT at={LONG(std::lround(step.x)),LONG(std::lround(step.y))};ClientToScreen(window,&at);
  const int vx=GetSystemMetrics(SM_XVIRTUALSCREEN),vy=GetSystemMetrics(SM_YVIRTUALSCREEN),vw=GetSystemMetrics(SM_CXVIRTUALSCREEN),vh=GetSystemMetrics(SM_CYVIRTUALSCREEN);
  if(vw>1&&vh>1)sent=SendNativePointer(MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK,LONG((double(at.x-vx)*65535)/(vw-1)),LONG((double(at.y-vy)*65535)/(vh-1)));
 }
 if(ready&&step.down)sent=SendNativePointer(MOUSEEVENTF_LEFTDOWN)&&sent;
 if(step.up)sent=SendNativePointer(MOUSEEVENTF_LEFTUP)&&sent; // release only a click this bridge owns, even on focus loss
 static bool failed=false;
 if(ready&&!sent&&!failed)Log("[UI34] native mouse injection failed; error=%lu; preserving ordinary gamepad menu input",GetLastError());
 failed=ready&&!sent;
 fc3pad::SetNativeMousePointer(ready&&sent);
 // A brief tracking/focus loss stops injection and releases the owned click,
 // but does not hide or reseed an already established native selection cursor.
 g_nativeCursorVisible=menu&&haveNativePosition&&(!ready||sent);
 // SendInput queues movement and click in order. Show the precise rounded
 // destination submitted to native input, rather than a possibly stale OS read.
 if(g_nativeCursorVisible){g_nativeCursorX=float(std::lround(nativeX))/(std::max)(1L,client.right);g_nativeCursorY=float(std::lround(nativeY))/(std::max)(1L,client.bottom);}
 static bool was=false;if(ready!=was){was=ready;Log("[MENU40] native mouse bridge %s: nativeUI=%d controller=%d XR=%d focus=%d; controller points + trigger clicks; stick fallback, stick click selects, native A confirms",ready?"active":"inactive",fc3pad::NativeMenuConfirmed()?1:0,controls.active?1:0,fc3xr::Running()?1:0,GetForegroundWindow()==window?1:0);}
}
ImGuiContext* g_nativeCursorContext=nullptr;ID3D11Device* g_nativeCursorDevice=nullptr;
void RenderNativeUiCursor(ID3D11Texture2D* left,ID3D11Texture2D* right,bool nativeOwns)
{
 if(!g_nativeCursorVisible||!nativeOwns||g_pistolMenu.open||!g_enabled||!left||!right||!g_dev||!g_ctx||
  !std::isfinite(g_nativeCursorX)||!std::isfinite(g_nativeCursorY)||g_nativeCursorX<0||g_nativeCursorX>1||g_nativeCursorY<0||g_nativeCursorY>1)return;
 const bool own=g_own;g_own=true;ImGuiContext* previous=ImGui::GetCurrentContext();
 if(g_nativeCursorContext&&g_nativeCursorDevice!=g_dev){ImGui::SetCurrentContext(g_nativeCursorContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_nativeCursorContext);g_nativeCursorContext=nullptr;}
 if(!g_nativeCursorContext){
  g_nativeCursorContext=ImGui::CreateContext();ImGui::SetCurrentContext(g_nativeCursorContext);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
  if(!ImGui_ImplDX11_Init(g_dev,g_ctx)){ImGui::DestroyContext(g_nativeCursorContext);g_nativeCursorContext=nullptr;ImGui::SetCurrentContext(previous);g_own=own;return;}
  g_nativeCursorDevice=g_dev;
 }
 ImGui::SetCurrentContext(g_nativeCursorContext);ID3D11RenderTargetView* saved[8]={};ID3D11DepthStencilView* depth=nullptr;g_ctx->OMGetRenderTargets(8,saved,&depth);
 int eye=0;
 for(auto* texture:{left,right}){
  float u=0,v=0;const auto panel=g_nativeUiPanel[eye++];
  if(!panel.Map(g_nativeCursorX,g_nativeCursorY,u,v))continue;
  D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);ID3D11RenderTargetView* rtv=nullptr;if(FAILED(g_dev->CreateRenderTargetView(texture,nullptr,&rtv)))continue;
  auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(float(td.Width),float(td.Height));io.DeltaTime=1.f/60;ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
  const float x=u*td.Width,y=v*td.Height,r=(std::max)(16.f,td.Height*.014f);
  auto* draw=ImGui::GetForegroundDrawList();const ImVec2 a(x,y),b(x+r*.35f,y+r),c(x+r*.7f,y+r*.6f);
  if(g_nativeRayHover298){draw->AddCircleFilled(a,r*.35f,IM_COL32(0,0,0,230));draw->AddCircle(a,r*.32f,g_nativeRayPressed298?IM_COL32(80,255,130,255):IM_COL32(70,220,255,255),20,2.f);draw->AddCircleFilled(a,2.f,IM_COL32(255,255,255,255));}else{draw->AddTriangleFilled(a,b,c,IM_COL32(255,255,255,255));draw->AddTriangle(a,b,c,IM_COL32(0,0,0,255),1.5f);}
  ImGui::Render();g_ctx->OMSetRenderTargets(1,&rtv,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());rtv->Release();
 }
 g_ctx->OMSetRenderTargets(8,saved,depth);for(auto* view:saved)if(view)view->Release();if(depth)depth->Release();ImGui::SetCurrentContext(previous);g_own=own;
}
