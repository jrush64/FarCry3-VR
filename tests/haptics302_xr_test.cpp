#include "../src/xr_presenter.cpp"
#include <vector>
#include <cstdlib>

void Check302(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
struct Sent302{XrAction action;float amplitude;XrDuration duration;};
std::vector<Sent302> sent302;
int stops302=0,syncs302=0;
bool failApply302=false;
XrResult syncResult302=XR_SUCCESS;
XrResult XRAPI_CALL Apply302(XrSession,const XrHapticActionInfo* info,const XrHapticBaseHeader* base){
 Check302(syncs302>0,"haptics dispatched after actual XR action sync");
 const auto* pulse=reinterpret_cast<const XrHapticVibration*>(base);
 sent302.push_back({info->action,pulse->amplitude,pulse->duration});
 return failApply302?XR_ERROR_RUNTIME_FAILURE:XR_SUCCESS;
}
XrResult XRAPI_CALL Stop302(XrSession,const XrHapticActionInfo*){++stops302;return XR_SUCCESS;}
XrResult XRAPI_CALL Sync302(XrSession,const XrActionsSyncInfo*){++syncs302;return syncResult302;}
XrResult XRAPI_CALL Vector302(XrSession,const XrActionStateGetInfo*,XrActionStateVector2f* out){out->isActive=XR_FALSE;return XR_SUCCESS;}
XrResult XRAPI_CALL Float302(XrSession,const XrActionStateGetInfo*,XrActionStateFloat* out){out->isActive=XR_FALSE;return XR_SUCCESS;}
XrResult XRAPI_CALL Boolean302(XrSession,const XrActionStateGetInfo*,XrActionStateBoolean* out){out->isActive=XR_FALSE;return XR_SUCCESS;}

int main(){
 {
  fc3haptic302::Queue queue;
  queue.Request(1,100);Check302(!queue.Take(100),"requests before focused session cannot accumulate");
  queue.SetLive(100,true);queue.Request(1000000,100);
  int pulses=0;for(DWORD now=100;now<=900;now+=100){queue.SetLive(now,true);pulses+=queue.Take(now)?1:0;}
  Check302(pulses==4,"burst requests capped at four, spaced by elapsed time");
  queue.Request(-1,900);queue.Request(0,900);Check302(!queue.Take(900),"zero and negative notification counts stay silent");
  queue.Request(2,900);queue.SetLive(901,false);queue.SetLive(902,true);Check302(!queue.Take(902),"focus loss discards pending notifications");
  queue.Request(1,902);queue.SetLive(1200,true);Check302(!queue.Take(1200),"render gap discards stale notification instead of replaying it");
  queue.Request(1,1500);Check302(!queue.Take(1500),"producer cannot queue after a stale focused-frame receipt");
  queue.SetLive(0xfffffff0u,true);queue.Request(1,0xfffffff0u);queue.SetLive(0x10u,true);
  Check302(queue.Take(0x10u),"DWORD clock wrap preserves fresh notification");
  Check302(fc3haptic302::Amplitude(NAN,100)==0&&fc3haptic302::Amplitude(.8f,-1)==0&&fc3haptic302::Amplitude(.8f,101)==0,"invalid feedback amplitude/settings fail silent");
 }
 wchar_t folder[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"hap",0,path);
 WritePrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"preserved",path);
 LoadFeedback302(path);Check302(fc3xr::FeedbackStrength302()==100,"missing saved strength preserves existing100-percent default");
 Check302(fc3xr::ConfigureFeedback302(50,path),"feedback strength saves");
 wchar_t sentinel[32];GetPrivateProfileStringW(L"MotionControls",L"ReloadSentinel",L"",sentinel,32,path);
 Check302(wcscmp(sentinel,L"preserved")==0,"saving feedback cannot replace reload settings");
 LoadFeedback302(path);Check302(fc3xr::FeedbackStrength302()==50,"saved feedback strength reloads");
 Check302(!fc3xr::ConfigureFeedback302(101,path)&&fc3xr::FeedbackStrength302()==50,"invalid settings cannot replace live strength");
 WritePrivateProfileStringW(L"Haptics",L"Strength",L"nan",path);LoadFeedback302(path);
 Check302(fc3xr::FeedbackStrength302()==100,"malformed saved strength uses safe existing default");
 fc3xr::ConfigureFeedback302(50,path);
 g_session=static_cast<XrSession>(1);g_actionsReady=g_sessionRunning=true;g_selectAllowed290=false;
 for(int i=0;i<kInputCount;++i)g_actions[i]=static_cast<XrAction>(i+1);
 p_xrSyncActions=Sync302;p_xrGetActionStateVector2f=Vector302;p_xrGetActionStateFloat=Float302;p_xrGetActionStateBoolean=Boolean302;
 p_xrApplyHapticFeedback=Apply302;p_xrStopHapticFeedback=Stop302;
 SyncControllers();fc3xr::Buzz(1);SyncControllers();
 Check302(sent302.size()==2&&fabs(sent302[0].amplitude-.4f)<1e-6f&&sent302[0].duration==120000000,"shipping sync routes scaled two-hand notification with original duration");
 Check302(sent302[0].action==g_actions[kHapticRight]&&sent302[1].action==g_actions[kHapticLeft],"notification retains correct right/left action routing");
 sent302.clear();fc3xr::InteractionPulse285(1);SyncControllers();
 Check302(sent302.size()==1&&sent302[0].action==g_actions[kHapticLeft]&&fabs(sent302[0].amplitude-.11f)<1e-6f&&sent302[0].duration==25000000,"holster/recovery pulse preserves selected hand and duration while scaling strength");
 sent302.clear();fc3xr::MacheteHit();SyncControllers();
 Check302(sent302.size()==1&&sent302[0].action==g_actions[kHapticRight]&&fabs(sent302[0].amplitude-.275f)<1e-6f&&sent302[0].duration==45000000,"melee feedback keeps right-hand impact semantics at saved strength");
 sent302.clear();g_feedbackSpecificAt302=GetTickCount();fc3xr::Buzz(1);SyncControllers();
 Check302(sent302.empty(),"generic notification cannot overwrite an ongoing melee pulse");
 const DWORD waitStart302=GetTickCount();while(sent302.empty()&&GetTickCount()-waitStart302<350){Sleep(10);SyncControllers();}
 Check302(sent302.size()==2,"generic notification resumes after the specific-hand pulse duration");
 sent302.clear();fc3xr::Buzz(2);fc3xr::InteractionPulse285(0);fc3xr::MacheteHit();syncResult302=XR_SESSION_NOT_FOCUSED;SyncControllers();
 Check302(sent302.empty()&&stops302==2,"unfocused sync emits no feedback and stops both prior controller outputs");
 fc3xr::Buzz(1);fc3xr::InteractionPulse285(0);fc3xr::MacheteHit();syncResult302=XR_SUCCESS;SyncControllers();
 Check302(sent302.empty(),"focus return cannot replay generic/holster/melee notifications");
 fc3xr::ConfigureFeedback302(0,path);SyncControllers();fc3xr::Buzz(1);fc3xr::InteractionPulse285(0);fc3xr::MacheteHit();SyncControllers();
 Check302(sent302.empty(),"zero strength mutes every production feedback route");
 fc3xr::ConfigureFeedback302(100,path);SyncControllers();fc3xr::Buzz(1);SyncControllers();
 Check302(sent302.size()==2&&fabs(sent302[0].amplitude-.8f)<1e-6f,"100percent restores original notification amplitude");
 sent302.clear();fc3xr::Buzz(2);g_sessionRunning=false;SyncControllers();g_sessionRunning=true;SyncControllers();
 Check302(sent302.empty(),"session stop/resume discards pending feedback");
 failApply302=true;fc3xr::InteractionPulse285(1);SyncControllers();sent302.clear();failApply302=false;SyncControllers();
 Check302(sent302.empty(),"runtime haptic failure cannot build a replay backlog");
 DeleteFileW(path);puts("PASS:302 shipping XR feedback, saved strength, hand routing, focus/session safety and bounded scheduling; no headset/game launched");
 return 0;
}
