#define main OriginalXR303
#include "hands303_xr_test.cpp"
#undef main
void Reset320(int owner){
 memset(input303,0,sizeof(input303));Neutral303();g_holsters297={};g_bodyPlaced297=true;g_rackBasisReady292=true;g_bodyRenderFailed297=false;inventory303.valid=true;inventory303.owner=123;inventory303.equipped=100;
 g_hands303={};g_hands303.initialized=true;g_hands303.player=123;g_hands303.entity=100;g_hands303.slot=0;g_hands303.hand=owner;g_hands303.reference=fc3xr::ReferenceGeneration286();g_hands303.epoch=1;g_hands303.generation=100;g_handGeneration303=~0u;g_handGripWait303[0]=g_handGripWait303[1]=false;tracked303[0]=tracked303[1]=true;
 for(int h=0;h<2;++h){pose303[h].orientation={0,0,0,1};const XrVector3f target={float(.1+h*.04),1.3f,-.5f};const auto offset=testnotes::Rotate(pose303[h].orientation,{h?-.0871f:.0871f,.0409f,-.1103f});pose303[h].position=testnotes::Sub(target,offset);}
 g_hands303.last=GetTickCount64();PublishHands303(true);fc3hands303::PublishTransfer307(true,false,true,g_hands303.generation,100,1,true,true);Neutral303();Neutral303();
}
int main(){OriginalXR303();Sleep(800);
 for(int owner:{0,1}){
  Reset320(owner);const int other=1-owner,hold=owner?kLeftGrip:kRightGrip,receive=other?kLeftGrip:kRightGrip;
  input303[hold]=1;Tick303();input303[receive]=1;Tick303();Check303(g_hands303.receiver==other&&g_hands303.hand==owner,"320 live XR palms arm release handover even though render contact publication was false");Check303(fc3pad::AmmoControls().leftGrip<.1f,"320 receiving squeeze is masked before revolver cylinder sees it");
  Tick303();Check303(g_hands303.hand==owner,"320 both grips held retain original owner without lateral pull");input303[hold]=0;Tick303();Check303(g_hands303.hand==other&&g_hands303.entity==100&&!g_hands303.empty,"320 holding grip release transfers same entity in either direction without movement");
  Reset320(owner);input303[hold]=1;Tick303();input303[receive]=1;Tick303();input303[receive]=0;Tick303();input303[hold]=0;Tick303();Check303(g_hands303.hand==owner,"320 receiving release cancels and later holding release cannot transfer");
  Reset320(owner);input303[hold]=1;Tick303();input303[receive]=1;Tick303();pose303[other].position.x+=.3f;Tick303();Check303(g_hands303.receiver<0&&fc3pad::AmmoControls().leftGrip<.1f,"320 separating cancels but cannot leak held squeeze into cylinder grab");input303[hold]=0;Tick303();Check303(g_hands303.hand==owner,"320 canceled handover cannot commit on holding release");input303[receive]=0;Tick303();input303[receive]=1;Tick303();Check303(fc3pad::AmmoControls().leftGrip>.9f,"320 fresh squeeze away from handover restores unchanged cylinder/reload grip");
  Reset320(owner);input303[receive]=1;Tick303();input303[receive]=0;Tick303();Check303(g_hands303.hand==owner&&g_hands303.receiver<0,"320 never transfers a sticky-held gun without holding-grip intent");
  Reset320(owner);input303[hold]=1;Tick303();input303[receive]=1;Tick303();tracked303[other]=false;Tick303();input303[hold]=0;Tick303();Check303(g_hands303.hand==owner,"320 receiving tracking loss cancels safely");
  Reset320(owner);input303[hold]=1;Tick303();input303[receive]=1;Tick303();fc3hands303::PublishTransfer307(true,true,false,g_hands303.generation,100,1,true,true);input303[hold]=0;Tick303();Check303(g_hands303.hand==owner,"320 busy/stale reload authorization cannot commit a handover");
 }
 // Pure State priority: same release at the source holster still transfers to an armed hand.
 fc3hands303::State state;fc3hands303::Input in;in.context=in.canChange=in.inventory=in.sphereTransfer307=in.releaseTransfer320=in.transferAllowed307=in.sphereContact307=true;in.player=1;in.entities[0]=in.equipped=100;in.tracked[0]=in.tracked[1]=true;state.initialized=true;state.player=1;state.entity=100;state.slot=0;state.hand=0;state.exited=true;state.last=1;in.now=2;state.ready[0]=state.ready[1]=true;in.hover[0]=0;in.grip[0]=1;state.Step(in);in.grip[1]=1;++in.now;state.Step(in);in.grip[0]=0;++in.now;const auto event=state.Step(in);Check303(event.transfer&&!event.stow&&state.hand==1&&!state.empty,"320 armed transfer takes precedence over coincident holster release");
 puts("PASS:320 .44 release handover actual XR/controller/cylinder-input arbitration both directions,sticky holding,cancellation,tracking,busy gates and holster priority; no game launched");
}
