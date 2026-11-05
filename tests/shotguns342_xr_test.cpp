#define FC3_HANDLE333_ENTRY LegacyHandle333
#include "handle333_xr_test.cpp"
#undef FC3_HANDLE333_ENTRY
extern unsigned ExerciseShotgunSupport342(int,bool);
int main(){
 LegacyHandle333();
 for(int profile:{3,4,5,6,34})for(int owner:{0,1}){
  const int other=1-owner,receive=other?kLeftGrip:kRightGrip;
  ResetGrab333(owner);fc3pad::SetGameplayControls(true);fc3pad::SetDedicatedInteractions(true);fc3pad::SetGripOnlyAmmo(false);fc3pad::SetGripContext(true,false);auto b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);PollGrab333(b);
  input303[receive]=1;PollGrab333(b);auto c=fc3pad::Latest();const auto h=fc3hands303::Read();
  Check303(g_hands303.hand==owner&&g_hands303.gripAction333[other]==fc3grab333::Support,"342 real XR forend squeeze keeps persistent weapon owner");
  Check303(fc3pad::SupportHold342(c,profile,h.entity,h.generation,GetTickCount64())&&fc3pad::AmmoControls().leftGrip>.9f,"342 accepted support reaches normalized gameplay grip in both owners");
  Check303(!fc3pad::WheelOwnsGrip(),"342 accepted forend squeeze beats legacy free-space wheel context");
  Check303(!fc3pad::SupportHold342(c,profile,h.entity,h.generation+1,GetTickCount64())&&!fc3pad::SupportHold342(c,profile,h.entity,h.generation,c.shotgunSupportAt342+121),"342 hand-generation change and stale intent cannot force support");
  Check303(ExerciseShotgunSupport342(profile,false)==15,"342 shipping UpdateTwoHand/TickSpas accepts XR support despite stale or distant legacy contact;no ammo spawned");
  if(profile==34){PollGrab333(b);Check303(ExerciseShotgunSupport342(profile,true)==7,"342 SPAS pump-due grip holds forend without two-hand steering or ammo pickup");}
  tracked303[other]=false;PollGrab333(b);Check303(!fc3pad::Latest().shotgunSupport342,"342 lost receiving-hand tracking clears accepted support intent");tracked303[other]=true;
  input303[receive]=0;PollGrab333(b);Check303(!fc3pad::Latest().shotgunSupport342,"342 release clears support intent");
  ResetGrab333(owner);b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);pose303[other].position.x+=.19f;PollGrab333(b);input303[receive]=1;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::Support&&fc3pad::Latest().shotgunSupport342,"34220cm shotgun reach includes near-forend edge on either hand");
  ResetGrab333(owner);b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);
  const auto head0=g_views[0].pose,head1=g_views[1].pose;const auto palm=Palm303(pose303[other],other);
  g_views[0].pose.orientation=g_views[1].pose.orientation={0,0,0,1};g_views[0].pose.position=g_views[1].pose.position={palm.x,palm.y+.38f,palm.z+.12f};
  PollGrab333(b);input303[receive]=1;PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::Support,"342 body pouch overlapping forend cannot steal support grip");g_views[0].pose=head0;g_views[1].pose=head1;
  ResetGrab333(owner);b=Binding333(owner,profile);b.actionCount=1;for(int k=0;k<3;++k)b.actions[0].point[k]=b.foregrip[k];b.actions[0].point[0]-=.04f;
  PlaceGrab333(other,owner,b,true);pose303[other].position.x-=.04f;PollGrab333(b);input303[receive]=1;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::Reload&&!fc3pad::Latest().shotgunSupport342,"342 closer actual loading port retains reload priority over support");
  printf("PASS:342 shotgun profile%d owner%d shipping XR intent->real support mechanics;freshness,release,pump,20cm reach,pouch/port priority\n",profile,owner);
 }
 puts("PASS:342 both owners,five M133/SPAS profiles and legacy333 controller regressions;offline only");return 0;
}
