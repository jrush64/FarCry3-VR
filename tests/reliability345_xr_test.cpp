#define FC3_HANDLE333_ENTRY LegacyHandle333
#include "handle333_xr_test.cpp"
#undef FC3_HANDLE333_ENTRY
extern unsigned ExerciseShotgunSupport342(int,bool);
int Baseline342(){
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

extern unsigned BusyPolicy345(int,bool,bool,bool);
#ifndef FC3_RELIABILITY345_ENTRY
#define FC3_RELIABILITY345_ENTRY main
#endif
int FC3_RELIABILITY345_ENTRY(){
 if(Baseline342())return 1;
 for(int profile:{0,1,2,3,4,5,6,7,8,9,12,14,15,17,18,20,21,22,23,25,28,31,32,34,35,36,40,41,43})for(int owner:{0,1}){
  const int other=1-owner,grip=other?kLeftGrip:kRightGrip,trigger=owner?kLeftTrigger:kRightTrigger;ResetGrab333(owner);auto b=Binding333(owner,profile);PlaceGrab333(other,owner,b,true);PollGrab333(b);input303[grip]=1;PollGrab333(b);
  Check303(g_hands303.gripAction333[other]==fc3grab333::Support,"345 fresh authored foregrip enters support for either owner");
  for(int shot=0;shot<4;++shot){input303[trigger]=1;PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::Support&&g_hands303.hand==owner,"345 firing squeeze cannot change support intent or weapon owner");input303[trigger]=0;PollGrab333(b);}
  b.supportActive=true;pose303[other].position.x+=.6f;PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::WaitRelease&&fc3pad::Latest().leftGrip<.1f,"345 moving far away releases support and reserves squeeze from reload");
  input303[grip]=0;PollGrab333(b);input303[grip]=1;PollGrab333(b);Check303(g_hands303.gripAction333[other]!=fc3grab333::Support,"345 stale render supportActive cannot grant a distant new grab");
  input303[grip]=0;PollGrab333(b);PlaceGrab333(other,owner,b,true);PollGrab333(b);input303[grip]=1;PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::Support,"345 actual release and fresh near-forend squeeze reacquires");
  tracked303[other]=false;PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::WaitRelease,"345 tracking loss cannot preserve a support permission");tracked303[other]=true;PollGrab333(b);input303[grip]=0;PollGrab333(b);
 }
 Check303(BusyPolicy345(34,true,false,false)==1,"345 ordinary SPAS support blocks legacy gestures but not explicit holster stow");
 Check303((BusyPolicy345(34,true,true,false)&14)==14,"345 physical SPAS pump still blocks stow and transfer");
 Check303((BusyPolicy345(34,false,false,true)&14)==14,"345 pending SPAS shell reload still blocks stow and transfer");
 for(int owner:{0,1}){
  ResetGrab333(owner);auto b=Binding333(owner,28);const int other=1-owner,grip=other?kLeftGrip:kRightGrip,hold=owner?kLeftGrip:kRightGrip;
  PlaceGrab333(other,owner,b,true);PollGrab333(b);input303[grip]=input303[hold]=1;PollGrab333(b);g_hands303.exited=true;Move303(owner,g_hands303.slot);const auto axis345=testnotes::Rotate(RackHandle292(g_hands303.slot).orientation,{0,fc3grab295::HalfLength+.11f,0});pose303[owner].position=testnotes::Add(pose303[owner].position,axis345);PlaceGrab333(other,owner,b,true);PollGrab333(b);Check303(g_hands303.gripAction333[other]==fc3grab333::Support,"345 stow approach retains actual support intent inside displayed capsule");input303[hold]=0;PollGrab333(b);Check303(g_hands303.empty,"345 occupied-slot release can stow while support hand is held");
 }
 for(int owner:{0,1}){ResetGrab333(owner);auto b=Binding333(owner,28);const int other=1-owner,grip=other?kLeftGrip:kRightGrip;PlaceGrab333(other,owner,b,true);PollGrab333(b);input303[grip]=1;PollGrab333(b);b=fc3grab333::Read();b.generation++;fc3grab333::Publish(b);SyncControllers();Check303(fc3pad::Latest().leftGrip<.1f&&fc3pad::AmmoControls().leftGrip<.1f,"345 wrong-generation binding cannot sustain support or become ammo input");}
 // Placement uses the same anatomical palm and current head-level reference
 // as displayed capsules, for both hands and every custom slot.
 for(int hand:{0,1})for(int slot=0;slot<4;++slot){ResetGrab333(0);g_holsters297={};g_holsters297.height=.1f;pose303[hand].orientation={0,0,.3f,std::sqrt(.91f)};pose303[hand].position={hand?-.25f:.25f,1.1f,-.4f};auto settings=g_holsters297;
  Check303(fc3xr::CaptureHolsterPosition345(settings,slot,hand),"345 tracked anatomical palm captures custom seated holster");g_holsters297=settings;const auto actual=Palm292(RackHandle292(slot)),wanted=Palm303(pose303[hand],hand);const auto d=testnotes::Sub(actual,wanted);Check303(d.x*d.x+d.y*d.y+d.z*d.z<1e-8f,"345 rendered capsule exactly matches captured palm including height offset");
  tracked303[hand]=false;Check303(!fc3xr::CaptureHolsterPosition345(settings,slot,hand),"345 untracked placement cannot change saved holster");tracked303[hand]=true;
 }

 // A reach freezes the complete body anchor; after leaving, it follows again.
 ResetGrab333(0);g_holsters297={};g_selectPose290=g_views[0].pose;g_singleHeld297=false;g_pick292.Cancel();Move303(0,0);XrSpaceLocation reach345={XR_TYPE_SPACE_LOCATION};reach345.pose=pose303[0];const auto wrist345=Wrist303(reach345,0);const auto centre345=Palm292(RackHandle292(0));g_views[0].pose.position.x+=.2f;g_views[1].pose.position.x+=.2f;UpdateAnchor297(wrist345,true,GetTickCount64());const auto anchored345=Palm292(RackHandle292(0)),d345=testnotes::Sub(anchored345,centre345);Check303(d345.x*d345.x+d345.y*d345.y+d345.z*d345.z<1e-8f,"345 head lean cannot move a holster away during a reach");reach345.pose.position={10,10,10};UpdateAnchor297(Wrist303(reach345,0),true,GetTickCount64());Check303(std::fabs(g_selectPose290.position.x-g_views[0].pose.position.x)<1e-6f,"345 body anchor follows head again when hands leave holsters");
 auto clear345=fc3handui::Settings{};const auto wheel345=clear345.panels[2];fc3handui::ClearWrist345(clear345);Check303(clear345.levelText345&&clear345.panels[0].x+clear345.panels[0].width*.5f<-.06f&&clear345.panels[1].x-clear345.panels[1].width*.5f>.06f&&clear345.panels[2].x==wheel345.x,"345 watch-clearance preset leaves wrist centre open and preserves wheel placement");
 // Levelling preserves attachment position and plane normal.
 for(float angle:{-.9f,-.4f,.3f,1.1f}){const XrQuaternionf q={0,0,std::sin(angle*.5f),std::cos(angle*.5f)},view={0,0,0,1};const auto level=LevelHandUi345(q,view);const auto up=testnotes::Rotate(level,{0,1,0}),normal=testnotes::Rotate(level,{0,0,1});Check303(std::fabs(up.x)<1e-5&&up.y>.999f&&std::fabs(normal.z-1)<1e-5,"345 wrist text level removes in-plane slant while retaining surface normal");}
 const auto original=fc3handui::Settings{};g_nativeHandSettings=original;g_nativeHandPose={{0,0,.3f,std::sqrt(.91f)},{.2f,1.2f,-.4f}};g_viewsValid=true;g_views[0].pose.orientation={0,0,0,1};const auto before=HandUiPanelPose(0);g_nativeHandSettings.levelText345=true;const auto after=HandUiPanelPose(0);const auto delta=testnotes::Sub(before.position,after.position);Check303(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z<1e-10f,"345 levelling never moves wrist HUD position");
 for(int model=0;model<2;++model)for(int known=0;known<2;++known)for(int different=0;different<2;++different)Check303(fc3hands303::FirstPersonCamera345(different,model,known)==(model&&(different||known)),"345 exact known weapon retains FP camera through equal-FOV draw;unknown scenery cannot acquire it");
 puts("PASS:345 controller ownership/support/reload/stow,custom seated placement and wrist UI regressions;headset pending");return 0;
}
