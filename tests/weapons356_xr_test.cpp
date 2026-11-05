#define FC3_RELIABILITY345_ENTRY LegacyControls356
#include "reliability345_xr_test.cpp"
#undef FC3_RELIABILITY345_ENTRY
int main(){
 LegacyControls356();
 for(int profile:{3,4,5,6})for(int owner:{0,1}){
  const int other=1-owner,receive=other?kLeftGrip:kRightGrip;
  ResetGrab333(owner);fc3pad::SetGripOnlyAmmo(false);auto b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);PollGrab333(b);
  input303[receive]=1;PollGrab333(b);b.supportActive=true;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::Support&&fc3pad::Latest().shotgunSupport342,"356 XR M133 support is initially latched");
  b.enabled=false;b.support=b.supportActive=false;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::Support&&fc3pad::Latest().shotgunSupport342,"356 one missing post-shot binding must not drop M133 support");
  b.enabled=b.support=true;b.supportActive=true;PollGrab333(b);
  Check303(ExerciseShotgunSupport342(profile,false)==15,"356 continued XR squeeze reaches real M133 UpdateTwoHand without spawning ammo");
  b.enabled=b.support=b.supportActive=false;
  pose303[other].position.x+=.5f;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::WaitRelease&&!fc3pad::Latest().shotgunSupport342,"356 current tracked palm leaving cached foregrip still releases support");
 }
 puts("PASS:356 actual XR M133 binding-gap continuation with current physical distance and proper release; original controller regressions retained");
}
