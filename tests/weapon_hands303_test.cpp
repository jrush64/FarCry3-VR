#include "../src/weapon_hands303.h"
#include <cstdio>
#include <cstdlib>
using namespace fc3hands303;
void Check303(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
Input Base303(){Input i;i.context=i.canChange=i.inventory=true;i.player=123;i.equipped=10;for(int n=0;n<4;++n)i.entities[n]=10+n;i.tracked[0]=i.tracked[1]=true;i.now=100;return i;}
Event Step303(State& state,Input& in){in.now+=16;return state.Step(in);}
void Neutral303(State& state,Input& in){in.grip[0]=in.grip[1]=0;Step303(state,in);}
void Draw303(State& state,Input& in,int hand,int slot){Neutral303(state,in);in.hover[hand]=slot;in.grip[hand]=1;auto e=Step303(state,in);Check303(e.drawSlot==slot&&e.drawHand==hand,"fresh grip requests exact slot for that physical hand");in.equipped=in.entities[slot];Check303(state.Complete(in,true),"ownership waits for matching native equip readback");}
int main(){
 State state;auto in=Base303();state.Step(in);Check303(state.empty,"holster-first starts empty without discarding native inventory");
 Draw303(state,in,1,0);Check303(!state.empty&&state.hand==1&&state.entity==10,"left draw owns exact weapon entity");
 in.grip[1]=0;Step303(state,in);Check303(!state.empty,"releasing grip inside draw holster cannot instantly stow");
 in.hover[1]=-1;Step303(state,in);in.hover[0]=1;in.grip[0]=1;auto e=Step303(state,in);
 Check303(e.drawSlot<0&&state.entity==10,"strict mode refuses a second gun until the first is holstered");
 Neutral303(state,in);in.hover[0]=-1;in.nearHandle=false;in.grip[0]=1;e=Step303(state,in);
 Check303(!e.transfer&&state.hand==1,"a grip away from the handle cannot transfer ownership");
 Neutral303(state,in);in.grip[1]=1;Step303(state,in);in.nearHandle=true;in.grip[0]=1;e=Step303(state,in);
 Check303(!e.transfer&&state.receiver==0,"other grip at handle offers transfer while old grip still holds");
 in.grip[1]=0;e=Step303(state,in);Check303(e.transfer&&state.hand==0&&state.entity==10,"old-hand release transfers same entity without native equip request");
 in.hover[0]=-1;Step303(state,in);in.hover[0]=0;in.grip[0]=0;e=Step303(state,in);
 Check303(e.stow&&state.empty,"return to original holster and release produces empty hands");
 Draw303(state,in,0,2);in.hover[0]=-1;in.grip[0]=0;Step303(state,in);
 Check303(!state.empty&&state.entity==12,"release outside holster retains the held weapon");
 Neutral303(state,in);in.nearHandle=true;in.hover[1]=-1;in.grip[1]=1;e=Step303(state,in);
 Check303(e.transfer&&state.hand==1&&state.entity==12,"free-hand handle grip transfers when old grip was already released");
 Neutral303(state,in);in.hover[0]=0;in.nearHandle=false;in.strict=false;in.grip[0]=1;e=Step303(state,in);
 Check303(e.drawSlot==0,"optional quick mode permits replacing the held weapon");
 in.equipped=10;Check303(state.Complete(in,true)&&state.hand==0,"quick replacement also requires exact native confirmation");
 for(int failure=0;failure<5;++failure){State s;auto i=Base303();s.Step(i);i.hover[1]=1;i.grip[1]=1;Step303(s,i);i.equipped=11;
  if(failure==0)i.player++;if(failure==1)i.entities[1]++;if(failure==2)i.equipped=12;if(failure==3)i.inventory=false;
  Check303(!s.Complete(i,failure!=4)&&s.empty,"wrong owner/slot/equipped/failure cannot produce a held gun");}
 for(int failure=0;failure<6;++failure){State s;auto i=Base303();s.Step(i);i.hover[1]=0;i.grip[1]=1;
  if(failure==0)i.context=false;if(failure==1)i.canChange=false;if(failure==2)i.tracked[1]=false;if(failure==3)i.grip[1]=NAN;if(failure==4)i.now+=200;if(failure==5)i.inventory=false;
  Check303(Step303(s,i).drawSlot<0,"untracked, busy, stale or invalid context cannot draw");}
 State interrupted;auto pause=Base303();interrupted.Step(pause);Draw303(interrupted,pause,1,0);pause.hover[1]=-1;Step303(interrupted,pause);pause.context=false;pause.equipped=0;Step303(interrupted,pause);
 Check303(!interrupted.empty&&interrupted.entity==10,"scripted native holster retains holding-hand identity");pause.context=true;pause.equipped=10;pause.reference++;auto cancel=Step303(interrupted,pause);
 Check303(cancel.drawSlot<0&&interrupted.receiver<0,"recenter cancels stale gestures without dropping held identity");
 State paused;auto resume=Base303();paused.Step(resume);Draw303(paused,resume,1,0);resume.hover[1]=-1;Step303(paused,resume);resume.context=false;Step303(paused,resume);resume.hover[1]=0;resume.grip[1]=0;Step303(paused,resume);resume.context=true;Step303(paused,resume);Check303(!paused.empty,"releasing a grip while paused cannot stow a weapon on resume");
 State quickBow;auto bow=Base303();bow.strict=false;bow.nativeHoldingSide=1;quickBow.Step(bow);Check303(!quickBow.empty&&quickBow.hand==1,"quick mode preserves the bow's established native holding side");
 State swapped;auto other=Base303();swapped.Step(other);Draw303(swapped,other,1,0);other.player++;Step303(swapped,other);
 Check303(swapped.empty&&swapped.receiver<0,"new local player cannot inherit held identity");
 State stow307;auto si307=Base303();stow307.Step(si307);Draw303(stow307,si307,0,0);si307.sphereTransfer307=si307.sphereContact307=si307.transferAllowed307=true;si307.hover[0]=-1;Step303(stow307,si307);si307.hover[0]=0;si307.grip[0]=0;si307.grip[1]=1;const auto sr307=Step303(stow307,si307);Check303(sr307.stow&&!sr307.transfer&&stow307.empty,"307 existing holster return/release wins over simultaneous receiving-hand grip");
 //309: start from native support, squeeze and pull toward receiving hand.
 for(int owner=0;owner<2;++owner){State h;auto i=Base303();h.Step(i);Draw303(h,i,owner,0);i.hover[owner]=-1;Neutral303(h,i);i.sphereTransfer307=i.transferAllowed307=true;i.grip[owner]=1;Step303(h,i);i.grip[1-owner]=1;auto a=Step303(h,i);
 Check303(h.receiver<0&&!a.transfer,"309 grip without existing two-hand support cannot arm");
 i.grip[1-owner]=0;Step303(h,i);i.sphereContact307=true;i.grip[1-owner]=1;a=Step303(h,i);Check303(h.receiver==1-owner&&!a.transfer,"309 existing support plus fresh free grip arms without transfer");
 const int free=1-owner;const float outward=free==1?-1.f:1.f;
 i.receivingRelative309[free][1]=.3f;Check303(!Step303(h,i).transfer,"309 forward/back pull cannot transfer");
 i.receivingRelative309[free][0]=-.1f*outward;Check303(!Step303(h,i).transfer,"309 pulling toward wrong side cannot transfer");
 i.receivingRelative309[free][0]=.04f*outward;Check303(!Step303(h,i).transfer,"309 small lateral jitter cannot transfer");
 i.sphereContact307=false;i.receivingRelative309[free][0]=.08f*outward;Check303(!Step303(h,i).transfer,"309 original hand moving away alone cannot transfer");i.receivingPosition309[free][0]=.08f*outward;a=Step303(h,i);Check303(a.transfer&&h.hand==free&&i.grip[owner]==1&&a.drawSlot<0,"309 directed outward pull transfers even after support releases, without original release");
 i.grip[owner]=0;Step303(h,i);i.sphereContact307=true;i.grip[owner]=1;Step303(h,i);i.grip[owner]=0;Step303(h,i);Check303(h.receiver<0,"309 receiving release cancels armed transfer");
 i.grip[owner]=1;Step303(h,i);i.transferAllowed307=false;Check303(!Step303(h,i).transfer&&h.receiver<0,"309 reload preempts armed transfer");
 }
 State turned309;auto ti309=Base303();turned309.Step(ti309);Draw303(turned309,ti309,0,0);Neutral303(turned309,ti309);ti309.sphereTransfer307=ti309.sphereContact307=ti309.transferAllowed307=true;ti309.viewRight309[0]=0;ti309.viewRight309[1]=-1;ti309.grip[1]=1;Step303(turned309,ti309);ti309.receivingRelative309[1][1]=ti309.receivingPosition309[1][1]=.08f;Check303(Step303(turned309,ti309).transfer,"309 lateral pull follows yaw axis captured at grip, not world X");
 Snapshot view;view.enabled=view.active=view.engaged=true;view.empty=false;view.entity=11;view.hand=1;
 Check303(!HeldVisible(view,11,11)&&HeldVisible(view,11,10),"held slot is absent from holsters while other slots remain");
 view.empty=true;Check303(HeldVisible(view,11,11),"stowed native-equipped identity returns to holster");
 view.empty=false;Check303(PhysicalHand(view,0)==1&&PhysicalHand(view,1)==0,"left ownership maps primary/support poses without mirroring a weapon");
 view.engaged=false;Check303(!SuppressWeapon(view)&&PhysicalHand(view,0)==0,"native actions and throwables retain their original hand/render paths");view.engaged=true;view.nativeHoldingSide=1;
 Check303(PhysicalHand(view,0)==0&&PhysicalHand(view,1)==1,"left-held bow preserves its native left bow/right string roles");view.hand=0;Check303(PhysicalHand(view,1)==0,"right-held bow routes the bow body to the requested right hand");view.nativeHoldingSide=0;
 view.enabled=false;Check303(PhysicalHand(view,0)==0&&!HeldVisible(view,11,11),"legacy fallback preserves right roles and hides equipped duplicate");
 puts("PASS:303 ownership policy, exact equip, stow, strict/quick modes, hand transfers and cancellation");
}
