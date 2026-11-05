#include "../src/locker_prepare330.h"
#include <cstdio>
#include <cstdlib>
using namespace fc3prepare330;
void Check(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
fc3select290::Inventory Inv(){fc3select290::Inventory i;i.valid=true;i.owner=123;for(int n=0;n<4;++n){i.weapons[n].entity=100+n;i.weapons[n].id=300+n;}return i;}
Output Start(State& s,const fc3select290::Inventory& i,unsigned ready=0){s.Step(i,ready,true,true,0,1000);return s.Step(i,ready,true,true,0,1600);}
int main(){
 for(int original=-1;original<4;++original){
  State s;auto i=Inv();if(original>=0)i.equipped=i.weapons[original].entity;auto o=Start(s,i);Check(o.started&&o.active&&o.target==0,"stable missing loadout starts automatically without grips");
  unsigned ready=0;ULONGLONG now=1600;
  for(int slot=0;slot<4;++slot){Check(o.target==slot&&(o.request==slot||i.equipped==i.weapons[slot].entity),"only the exact next owned missing slot is requested");i.equipped=i.weapons[slot].entity;now+=100;o=s.Step(i,ready,true,true,1,now);Check(o.active&&o.target==slot,"equipped readback alone does not invent a prepared model");ready|=1u<<slot;now+=700;o=s.Step(i,ready,true,true,1,now);}
  if(original>=0&&original!=3){Check(o.active&&o.request==original&&s.phase==3,"preparation restores original native weapon before releasing ownership");i.equipped=i.weapons[original].entity;o=s.Step(i,ready,true,true,1,now+100);}
  Check(o.finished&&!s.active,"complete loadout ends bounded preparation");Check(!s.Step(i,ready,true,true,1,now+1000).active,"ready loadout never cycles again");
 }
 {State s;auto i=Inv();auto o=Start(s,i,15);Check(!o.active,"all four cached models bypass preparation");}
 {State s;auto i=Inv();auto o=Start(s,i,13);Check(o.request==1,"partial cache prepares only missing slot");}
 {State s;auto i=Inv();s.Step(i,0,true,false,0,1000);Check(!s.Step(i,0,true,true,0,1600).active&&!s.Step(i,0,true,true,0,1800).active,"held input and neutral debounce delay preparation");Check(s.Step(i,0,true,true,0,2000).started,"neutral input permits preparation after debounce");}
 {State s;auto i=Inv();Start(s,i);auto o=s.Step(i,0,false,true,0,1700);Check(!o.active&&s.active,"menu or action context suspends internal selection");o=s.Step(i,0,true,true,0,10000);Check(o.active&&!o.failed&&o.target==0,"paused time cannot falsely time out native arrival");}
 {State s;auto i=Inv();Start(s,i);auto o=s.Step(i,0,true,true,-1,1700);Check(o.failed&&s.failed==1&&o.request==1,"native refusal skips failed slot instead of looping");}
 {State s;auto i=Inv();auto o=Start(s,i);ULONGLONG now=1600;for(int slot=0;slot<4;++slot){now+=2600;o=s.Step(i,0,true,true,0,now);}Check(o.finished&&s.failed==15&&!s.Step(i,0,true,true,0,now+5000).active,"unavailable native draws fail once per loadout");i.weapons[0].id++;o=s.Step(i,0,true,true,0,now+5100);Check(o.changed&&s.failed==0&&!o.active,"changed inventory clears failed attempts and debounces again");}
 {State s;auto i=Inv();i.equipped=103;Start(s,i);i.equipped=100;s.Step(i,0,true,true,1,1700);auto o=s.Step(i,1,true,false,1,1800);Check(o.request==3&&s.restore,"user input cancels further models and restores original");i.equipped=103;o=s.Step(i,1,true,false,1,1900);Check(o.finished,"cancel finishes only after restoration readback");}
 {State s;auto i=Inv();Start(s,i);i.equipped=100;s.Step(i,0,true,true,1,1700);auto o=s.Step(i,0,true,true,1,4801);Check(o.failed&&(s.failed&1)&&o.request==1,"model capture timeout does not mark an uncaptured appearance ready");}
 puts("PASS:330 automatic preparation policy: empty/nonempty ownership, four models, cache skip, debounce, context pause, refusal, timeout, cancellation and restore; offline only");
}
