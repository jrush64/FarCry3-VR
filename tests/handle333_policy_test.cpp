#define main OriginalPolicy303
#include "weapon_hands303_test.cpp"
#undef main
State Held333(int owner){State s;s.initialized=true;s.empty=false;s.player=123;s.entity=10;s.slot=0;s.hand=owner;s.reference=s.epoch=1;s.ready[0]=s.ready[1]=true;s.down[0]=s.down[1]=false;s.last=1000;return s;}
Input Input333(){auto i=Base303();i.reference=i.epoch=1;i.now=1000;i.direct333=i.transferAllowed307=true;return i;}
int main(){
 OriginalPolicy303();
 for(int owner=0;owner<2;++owner){
  const int other=1-owner;auto s=Held333(owner);auto i=Input333();i.handle333=true;i.grip[other]=1;auto e=Step303(s,i);Check303(e.transfer&&s.hand==other&&s.entity==10&&e.drawSlot<0,"333 handle squeeze transfers immediately, no owning grip,pull or native equip");
  i.grip[owner]=1;e=Step303(s,i);Check303(!e.transfer&&s.hand==other,"333 old holding grip cannot instantly take gun back");i.grip[owner]=0;Step303(s,i);i.grip[owner]=1;e=Step303(s,i);Check303(e.transfer&&s.hand==owner,"333 releasing only the old holding grip permits a deliberate return");
  s=Held333(owner);i=Input333();i.grip[other]=1;Step303(s,i);Check303(s.hand==owner&&s.gripAction333[other]==fc3grab333::None,"333 squeeze in empty space claims no part");i.handle333=true;e=Step303(s,i);Check303(e.transfer,"333 held free grip entering handle works without a button edge");
  s=Held333(owner);i=Input333();i.support333=true;i.grip[other]=1;Step303(s,i);Check303(s.hand==owner&&s.gripAction333[other]==fc3grab333::Support,"333 foregrip grip claims support, not ownership");i.support333=false;i.handle333=true;e=Step303(s,i);Check303(!e.transfer,"333 one support squeeze cannot change action halfway through");i.grip[other]=0;Step303(s,i);i.grip[other]=1;e=Step303(s,i);Check303(e.transfer,"333 release support then grip handle transfers immediately");
  s=Held333(owner);i=Input333();i.handle333=i.reloadNear333=true;i.grip[other]=1;e=Step303(s,i);Check303(e.transfer,"333 firing handle wins an idle overlapping magazine target");
  s=Held333(owner);i=Input333();i.busy333=i.handle333=true;i.transferAllowed307=false;i.canChange=false;i.grip[other]=1;e=Step303(s,i);Check303(!e.transfer&&s.gripAction333[other]==fc3grab333::Reload,"333 an active reload owns the squeeze despite handle contact");i.busy333=false;i.transferAllowed307=true;e=Step303(s,i);Check303(!e.transfer,"333 finishing reload cannot reinterpret a still-held grip as transfer");
  s=Held333(owner);i=Input333();i.reloadNear333=true;i.grip[other]=1;Step303(s,i);i.reloadNear333=false;i.handle333=true;e=Step303(s,i);Check303(!e.transfer&&s.gripAction333[other]==fc3grab333::Reload,"333 actual mechanism grab retains its action until release");
  s=Held333(owner);i=Input333();i.ammoNear333=true;i.grip[other]=1;Step303(s,i);Check303(s.gripAction333[other]==fc3grab333::Reload,"333 pouch grip owns ammunition action");
  s=Held333(owner);i=Input333();i.nearHandle=true;i.sphereTransfer307=false;i.handle333=false;i.grip[other]=1;e=Step303(s,i);Check303(!e.transfer&&s.receiver<0,"333 unavailable native handle cannot fall back to old palm-proximity transfer");
  s=Held333(owner);i=Input333();i.handle333=true;i.grip[other]=1;i.tracked[other]=false;e=Step303(s,i);Check303(!e.transfer,"333 missing receiving tracking cannot grab");
  s=Held333(owner);i=Input333();i.handle333=true;i.grip[other]=1;i.now+=300;e=s.Step(i);Check303(!e.transfer,"333 stalled input requires neutral instead of grabbing on recovery");
 }
 puts("PASS:333 one-handle immediate transfer,held-grip entry,one-action latch,support/reload/pouch isolation,anti-bounce and tracking safety in both hands;offline only");
}
