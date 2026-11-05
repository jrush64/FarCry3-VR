#include "../../native282/src/xr_presenter.cpp"
#include <cstdlib>
void Check283(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}}
XrResult XRAPI_CALL Wait283(XrSession,const XrFrameWaitInfo*,XrFrameState* s){s->predictedDisplayTime=1;s->predictedDisplayPeriod=0;s->shouldRender=XR_TRUE;return XR_SUCCESS;}
XrResult XRAPI_CALL Begin283(XrSession,const XrFrameBeginInfo*){return XR_SUCCESS;}
bool tracked283=true;
XrResult XRAPI_CALL Locate283(XrSession,const XrViewLocateInfo*,XrViewState* s,uint32_t,uint32_t* n,XrView* v){
 *n=2;s->viewStateFlags=XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT;
 if(tracked283)s->viewStateFlags|=XR_VIEW_STATE_POSITION_TRACKED_BIT|XR_VIEW_STATE_ORIENTATION_TRACKED_BIT;
 for(int i=0;i<2;++i){v[i].pose.orientation.w=1;v[i].pose.position={i?.032f:-.032f,1.6f,0};v[i].fov={-.8f,.8f,.8f,-.8f};}return XR_SUCCESS;
}
int main(){
 p_xrWaitFrame=Wait283;p_xrBeginFrame=Begin283;p_xrLocateViews=Locate283;g_actionsReady=false;
 // Production InWorldView is false before any game scene; use the actual
 // shipping frame function to test a world/menu gap, without an XR runtime.
 Check283(!fc3stereo::InWorldView(),"fixture begins outside a world view");
 g_referenceValid=true;g_referenceYaw=.71f;g_referencePosition={.2f,1.7f,-.4f};g_frameTracked=true;
 StartFrame();Check283(!g_frameTracked&&g_referenceValid&&g_referenceYaw==.71f&&g_referencePosition.y==1.7f,"283 world/menu pause stops tracked output without changing the established reference");
 tracked283=false;StartFrame();Check283(g_referenceValid&&g_referenceYaw==.71f,"283 temporary headset tracking loss is not an implicit recenter");
 tracked283=true;StartFrame();Check283(g_referenceValid&&g_referenceYaw==.71f,"283 recovery retains the original reference");
 fc3xr::Recenter();Check283(!g_referenceValid,"283 explicit user recenter still releases the reference");
 puts("PASS:283 shipping XR StartFrame preserves reference across world/menu/tracking gaps; explicit Recenter remains authoritative; no runtime/headset created");
}
