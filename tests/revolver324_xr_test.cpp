#define main OriginalXR303
#include "hands303_xr_test.cpp"
#undef main
void Reset321(int owner){
 memset(input303,0,sizeof(input303));Neutral303();g_holsters297={};g_bodyPlaced297=true;g_rackBasisReady292=true;g_bodyRenderFailed297=false;inventory303.valid=true;inventory303.owner=123;inventory303.equipped=100;
 g_hands303={};g_hands303.initialized=true;g_hands303.player=123;g_hands303.entity=100;g_hands303.slot=0;g_hands303.hand=owner;g_hands303.reference=fc3xr::ReferenceGeneration286();g_hands303.epoch=1;g_hands303.generation=100;g_handGeneration303=~0u;g_handGripWait303[0]=g_handGripWait303[1]=false;tracked303[0]=tracked303[1]=true;
 for(int h=0;h<2;++h){pose303[h].orientation={0,0,0,1};const XrVector3f target={float(.1+h*.04),1.3f,-.5f};const auto offset=testnotes::Rotate(pose303[h].orientation,{h?-.0871f:.0871f,.0409f,-.1103f});pose303[h].position=testnotes::Sub(target,offset);}
 g_hands303.last=GetTickCount64();PublishHands303(true);fc3hands303::PublishTransfer307(true,false,true,g_hands303.generation,100,1,true,true);Neutral303();Neutral303();
}
void Poll321(bool busy=false){inventory303.at=GetTickCount64();SeedInventory303(inventory303,0);fc3xr::SetHandsContext303(true,!busy,1);fc3hands303::PublishTransfer307(true,!busy,!busy,g_hands303.generation,100,1,true,true);SyncControllers();}
int main(){OriginalXR303();Sleep(800);
 for(int owner:{0,1}){
  Reset321(owner);const int other=1-owner,hold=owner?kLeftGrip:kRightGrip,receive=other?kLeftGrip:kRightGrip;
  pose303[other].position.x+=1.5f;pose303[other].orientation={.7071068f,0,0,.7071068f};
  input303[hold]=1;Poll321();input303[receive]=1;Poll321();Check303(g_hands303.hand==owner&&g_hands303.receiver==other,"324 both grips arm without teleporting");pose303[other].position.x+=other?-.08f:.08f;Poll321();Check303(g_hands303.hand==other&&g_hands303.entity==100&&!g_hands303.empty,"321 owner modifier plus fresh opposite grip transfers only after actual lateral pull regardless of initial position/orientation");Check303(fc3pad::AmmoControls().leftGrip<.1f,"321 transfer chord never reaches cylinder/pickup grip");
  Tick303();Check303(g_hands303.hand==other&&g_hands303.modifierLatched321,"321 modifier latch survives the render-publication generation gap");Poll321();Check303(g_hands303.hand==other,"321 holding both grips cannot bounce back");input303[hold]=0;Poll321();input303[hold]=1;Poll321();Check303(g_hands303.hand==other,"321 old owner re-squeeze cannot reverse before both grips are released");input303[hold]=input303[receive]=0;Poll321();input303[receive]=1;Poll321();input303[hold]=1;Poll321();Check303(g_hands303.hand==other,"324 symmetric return arms without teleporting");pose303[owner].position.x+=owner?-.08f:.08f;Poll321();Check303(g_hands303.hand==owner,"324 neutral rearm plus receiving-side pull returns weapon");
  Reset321(owner);input303[hold]=1;Poll321();input303[receive]=1;Poll321();
  pose303[owner].position.x+=owner?-.1f:.1f;Poll321();Check303(g_hands303.hand==owner,"324 moving only owner cannot complete pass");
  pose303[other].position.x+=other?.12f:-.12f;Poll321();Check303(g_hands303.hand==owner,"324 wrong-way receiving motion cannot pass");
  input303[receive]=0;Poll321();pose303[other].position.x+=other?-.5f:.5f;Poll321();Check303(g_hands303.hand==owner,"324 released receiving grip cancels pass");
  Reset321(owner);input303[receive]=1;Poll321();Check303(g_hands303.hand==owner&&fc3pad::AmmoControls().leftGrip>.9f,"321 free-hand grip with gun grip released retains original reload controls");
  // Begin a reload first; adding the gun grip cannot steal an active mechanism.
  input303[hold]=1;Poll321(true);Check303(g_hands303.hand==owner&&!g_hands303.empty&&fc3pad::AmmoControls().leftGrip>.9f,"321 ongoing reload keeps ownership and receives its grip even when holding grip is added");input303[hold]=0;Poll321(true);Check303(g_hands303.hand==owner&&!g_hands303.empty,"321 releasing holding grip during reload does not drop/stow/transfer weapon");
  Reset321(owner);input303[hold]=1;Poll321();tracked303[other]=false;input303[receive]=1;Poll321();Check303(g_hands303.hand==owner,"321 missing receiving tracking prevents transfer");
  Reset321(owner);input303[hold]=1;Poll321();input303[receive]=1;Poll321(true);Check303(g_hands303.hand==owner,"321 reload lease prevents a new modifier transfer");
 }
 fc3hands303::State state;fc3hands303::Input in;in.context=in.inventory=in.sphereTransfer307=in.modifierTransfer321=true;in.player=1;in.entities[0]=in.equipped=100;in.tracked[0]=in.tracked[1]=true;in.now=2;state.initialized=true;state.player=1;state.entity=100;state.slot=0;state.hand=0;state.exited=true;state.last=1;state.down[0]=true;in.hover[0]=0;const auto e=state.Step(in);Check303(!e.stow&&!e.transfer&&!state.empty,"321 reload canChange lease blocks coincident holster release");
 puts("PASS:324 .44 both-grip plus pull handover actual XR inputs,both directions,no geometry,neutral rearm,reload grip priority and reload stow protection;no game launched");return 0;
}
