#include "smgs332_xr_fixture.inl"
int main(){
 OriginalHands326XR();
 for(int profile:{7,9,21,28,39})for(int owner:{0,1}){
  Check303(fc3hands303::Smg332(profile),"332 all five SMG profiles included");Reset321(owner);const int other=1-owner,hold=owner?kLeftGrip:kRightGrip,receive=other?kLeftGrip:kRightGrip;
  // Publisher policy is verified independently with real GPU hand contacts.
  // Exercise that policy through shipping XR/hand-state/input mapping here.
  auto poll=[&](bool busy=false){inventory303.at=GetTickCount64();SeedInventory303(inventory303,0);fc3xr::SetHandsContext303(true,!busy,1);fc3hands303::PublishTransfer307(true,!busy,!busy,g_hands303.generation,100,1,false,true);SyncControllers();};
  pose303[other].position.x+=other?-1.3f:1.3f;pose303[other].orientation={0,0,1,0};input303[hold]=1;poll();input303[receive]=1;poll();Check303(g_hands303.hand==owner&&g_hands303.receiver==other,"332 both grips arm SMG regardless of initial hand position or wrist roll");Check303(fc3pad::Latest().leftGrip>.9f&&fc3pad::AmmoControls().leftGrip<.1f,"332 XR support grip remains present while transfer owns ammo squeeze");
  pose303[owner].position.x+=owner?-.1f:.1f;poll();Check303(g_hands303.hand==owner,"332 moving owning hand alone cannot pass");pose303[other].position.x+=other?-.2f:.2f;poll();Check303(g_hands303.hand==other&&g_hands303.entity==100&&g_hands303.pendingSlot<0,"332 actual tracked receiving pull transfers same SMG without native cycling");poll();Check303(g_hands303.hand==other,"332 held grips cannot bounce SMG back");
  input303[hold]=input303[receive]=0;poll();input303[receive]=1;poll();input303[hold]=1;poll();pose303[owner].position.x+=owner?-.1f:.1f;poll();Check303(g_hands303.hand==owner,"332 neutral rearm and opposite pull returns SMG");
  Reset321(owner);input303[hold]=1;poll();input303[receive]=1;poll(true);Check303(g_hands303.hand==owner&&g_hands303.receiver<0&&fc3pad::AmmoControls().leftGrip>.9f,"332 active reload prevents new transfer and retains operating grip");
  Reset321(owner);input303[hold]=1;poll();input303[receive]=1;poll();input303[hold]=0;poll();Check303(g_hands303.hand==owner&&g_hands303.receiver<0,"332 releasing holding grip cancels optional pass");
  Reset321(owner);input303[hold]=1;poll();tracked303[other]=false;input303[receive]=1;poll();Check303(g_hands303.hand==owner&&g_hands303.receiver<0,"332 tracking loss cannot transfer SMG");tracked303[other]=true;
  Reset321(owner);input303[hold]=1;poll();input303[receive]=1;input303[owner?kButtonB:kButtonX]=1;poll();Check303(g_hands303.hand==owner&&g_hands303.receiver<0&&!fc3pad::Latest().ammoGripReserved326,"332 physical free-hand magazine button cannot become hand transfer");
  printf("PASS:332 profile%d owner%d shipping XR both-grip/pull,roll-independent entry,support preservation,no teleport/bounce,native identity,return,cancel,reload/button/tracking priority\n",profile,owner);
 }
 puts("PASS:332 all five SMGs in both directions through shipping XR and input mapping; offline only");
}
