#include "../src/xr_presenter.cpp"
#include <cstdio>
#include <cstdlib>
void Check298(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
XrPosef aim298={{0,0,0,1},{0,1.6f,0}};bool handTracked298=true,locateOk298=true;
XrResult XRAPI_CALL Locate298(XrSpace,XrSpace,XrTime,XrSpaceLocation* out){if(!locateOk298)return XR_ERROR_RUNTIME_FAILURE;out->pose=aim298;out->locationFlags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;if(handTracked298)out->locationFlags|=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;return XR_SUCCESS;}
int main(){
 g_sessionRunning=g_viewsValid=g_viewsTracked=g_actionsReady=true;g_handSpaces[0]=(XrSpace)1;p_xrLocateSpace=Locate298;g_viewsLocatedAt298=GetTickCount64();g_frameTracked=true;
 for(int i=0;i<2;++i){g_views[i].pose={{0,0,0,1},{i?.032f:-.032f,1.6f,0}};g_views[i].fov={-std::atan(1.f),std::atan(1.f),std::atan(1.f),-std::atan(1.f)};}
 float screen[4];Check298(fc3xr::NativeMenuRay298(2,9.f/16,screen),"shipping XR API locates tracked native aim action at menu display time");
 Check298(std::fabs(screen[0]-.508f)<1e-5f&&std::fabs(screen[2]-.492f)<1e-5f&&screen[1]==.5f&&screen[3]==.5f,"each eye projects the aim ray with its own IPD origin");
 fc3ui::PanelTransform panels[2]={{.5773503f,.56f,.016f,0},{.5773503f,.56f,-.016f,0}};float u,v;Check298(fc3ui::NativeRayHit298(panels,screen,u,v)&&std::fabs(u-.5f)<1e-5f&&std::fabs(v-.5f)<1e-5f,"inverse of actual per-eye native UI placement meets at one native cursor coordinate");
 for(float targetU:{.1f,.5f,.9f})for(float targetV:{.1f,.5f,.9f}){float screenUv[4];for(int i=0;i<2;++i)panels[i].Map(targetU,targetV,screenUv[i*2],screenUv[i*2+1]);Check298(fc3ui::NativeRayHit298(panels,screenUv,u,v)&&std::fabs(u-targetU)<1e-5f&&std::fabs(v-targetV)<1e-5f,"native panel transform round trip retains actual cursor placement");}
 // Reproduce native shader panel transforms with asymmetric left/right FOV.
 for(int i=0;i<2;++i){const float l=i?-.8f:-1.1f,r=i?1.1f:.8f,b=-.7f,t=.9f;g_views[i].fov={std::atan(l),std::atan(r),std::atan(t),std::atan(b)};panels[i]={1.2f/(r-l),1.2f*(9.f/16)/(t-b),-(r+l)/(r-l)+(i?-1:1)*(.032f/(r-l)),-(t+b)/(t-b)};}
 aim298.position={.36f,1.72f,.4f};Check298(fc3xr::NativeMenuRay298(2,9.f/16,screen)&&fc3ui::NativeRayHit298(panels,screen,u,v)&&std::fabs(u-.65f)<1e-5f&&std::fabs(v-(.5f-.12f/(2.4f*9.f/16)))<1e-5f,"asymmetric submitted eye FOV and off-centre controller meet the actual shader-scaled native panel");
 for(int i=0;i<2;++i)g_views[i].fov={-std::atan(1.f),std::atan(1.f),std::atan(1.f),-std::atan(1.f)};
 auto yaw=XrQuaternionf{0,std::sin(.6f),0,std::cos(.6f)};auto head=XrVector3f{7,2,-3};for(int i=0;i<2;++i){g_views[i].pose.orientation=yaw;g_views[i].pose.position=testnotes::Add(head,testnotes::Rotate(yaw,{i?.032f:-.032f,0,0}));}aim298={yaw,head};
 Check298(fc3xr::NativeMenuRay298(2,9.f/16,screen)&&std::fabs(screen[0]-.508f)<1e-5f&&std::fabs(screen[2]-.492f)<1e-5f,"translated and rotated headset/controller preserve native UI ray coordinates without recentering");
 g_frameTracked=false;const float xs=fc3stereo::MainXScale();Check298(xs>0&&fc3xr::NativeMenuRay298(2,9.f/16,screen)&&std::fabs(screen[0]-(.5f+.008f*xs))<1e-5f,"head-locked menu uses game projection actually submitted rather than headset FOV");
 handTracked298=false;Check298(!fc3xr::NativeMenuRay298(2,9.f/16,screen),"untracked placeholder aim pose cannot point at menus");handTracked298=true;
 g_viewsTracked=false;Check298(!fc3xr::NativeMenuRay298(2,9.f/16,screen),"lost HMD tracking stops pointer");g_viewsTracked=true;
 g_viewsLocatedAt298=GetTickCount64()-121;Check298(!fc3xr::NativeMenuRay298(2,9.f/16,screen),"stale frame prediction cannot drive menu pointer");g_viewsLocatedAt298=GetTickCount64();
 locateOk298=false;Check298(!fc3xr::NativeMenuRay298(2,9.f/16,screen),"failed XR locate cannot drive menu pointer");locateOk298=true;
 aim298.orientation=testnotes::Multiply(yaw,{0,1,0,0});Check298(!fc3xr::NativeMenuRay298(2,9.f/16,screen),"controller pointing away cannot hit the menu plane");
 Check298(!fc3xr::NativeMenuRay298(NAN,9.f/16,screen)&&!fc3xr::NativeMenuRay298(2,NAN,screen),"invalid depth/aspect cannot produce a pointer");
 {float mismatched[4]={.25f,.5f,.75f,.5f};Check298(!fc3ui::NativeRayHit298(panels,mismatched,u,v),"inconsistent stereo panel coordinates cannot click");float invisible[4]={-1.f,.5f,-1.f,.5f};Check298(!fc3ui::NativeRayHit298(panels,invisible,u,v),"offscreen ray cannot select invisible UI");}
 panels[0].scaleX=NAN;Check298(!fc3ui::NativeRayHit298(panels,screen,u,v),"invalid native panel refused");
 puts("PASS:298 shipping XR native menu ray: stereo placement, submitted FOV, reference independence and tracking guards; no headset/game launched");
}
