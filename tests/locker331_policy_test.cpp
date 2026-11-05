#include "../src/locker_prepare330.h"
#include "../src/selector_capture331.h"
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
 {State s;auto i=Inv();Start(s,i);auto o=s.Step(i,0,true,true,-1,1700);Check(o.retry331&&!o.failed&&s.failed==0&&o.request==0,"331 native refusal retries the exact same owned slot within budget");}
 {State s;auto i=Inv();auto o=Start(s,i);ULONGLONG now=1600;for(int slot=0;slot<4;++slot)for(int attempt=0;attempt<3;++attempt){now+=2600;o=s.Step(i,0,true,true,-1,now);}Check(!s.active&&s.failed==15&&!s.Step(i,0,true,true,0,now+5000).active,"unavailable native draws exhaust a bounded budget rather than looping");i.weapons[0].id++;o=s.Step(i,0,true,true,0,now+5100);Check(o.changed&&s.failed==0&&!o.active,"changed inventory clears failed attempts and debounces again");}
 {State s;auto i=Inv();i.equipped=103;Start(s,i);i.equipped=100;s.Step(i,0,true,true,1,1700);auto o=s.Step(i,1,true,false,1,1800);Check(o.request==3&&s.restore,"user input cancels further models and restores original");i.equipped=103;o=s.Step(i,1,true,false,1,1900);Check(o.finished,"cancel finishes only after restoration readback");}
 {State s;auto i=Inv();Start(s,i);i.equipped=100;s.Step(i,0,true,true,1,1700);auto o=s.Step(i,0,true,true,1,4801);Check(o.retry331&&!(s.failed&1)&&o.request==0,"331 capture timeout retries without inventing a prepared appearance");}
 {
  State s;auto i=Inv();auto o=Start(s,i,13);i.equipped=101;s.Step(i,13,true,true,1,1700);i.equipped=0;o=s.Step(i,13,true,true,1,2075);
  Check(o.retry331&&o.request==1&&!o.failed&&s.phase==1,"331 observed1887 equip-to-zero375ms later requeues owned slot immediately");
  i.equipped=101;s.Step(i,13,true,true,1,2110);o=s.Step(i,15,true,true,1,2240);Check(o.finished&&!s.failed,"331 recovered capture completes without changing cached other slots");
 }
 {State s;auto i=Inv();Start(s,i,13);ULONGLONG t=1600;Output o;for(int n=0;n<3;++n){i.equipped=101;s.Step(i,13,true,true,1,t+=50);i.equipped=0;o=s.Step(i,13,true,true,1,t+=375);}Check(o.finished&&s.failed==2,"331 repeated transient loss is bounded at three attempts");i.equipped=100;s.Step(i,13,true,true,1,t+=100);o=s.Step(i,13,true,true,1,t+=800);Check(o.rearmed331&&!s.failed,"331 stable native armed context permits one fresh bounded recovery round");}
 {State s;auto i=Inv();i.equipped=103;Start(s,i,13);i.equipped=101;s.Step(i,13,true,true,1,1700);i.equipped=0;const auto o=s.Step(i,13,true,false,1,1800);Check(!o.retry331&&o.request==3&&s.restore,"331 user cancellation during equip loss restores original instead of redrawing target");}
 {fc3capture331::Gate g;float fingers[300]={};Check(!g.Frame(1,10,1000,42,fingers),"331 one complete native frame cannot register a weapon");Check(!g.Frame(1,11,1030,42,fingers),"331 candidate needs a consistency window");Check(g.Frame(1,12,1065,42,fingers),"331 complete consistent native frames qualify before1887 withdrawal");Check(!g.Frame(1,13,1100,43,fingers),"331 late attachment/material change resets consistency window");fingers[36]=.5f;Check(!g.Frame(1,14,1200,43,fingers),"331 moving native fingers cannot seed an unsettled grip");Check(!g.Frame(1,16,1300,43,fingers),"331 missing source frame resets consistency window");g.Reset();Check(!g.Frame(1,17,1400,43,fingers),"331 incomplete capture reset cannot accept old candidate");fingers[36]=NAN;Check(!g.Frame(1,18,1500,43,fingers),"331 nonfinite captured pose fails closed");}
 puts("PASS:331 bounded equip-loss/refusal/capture recovery, stable-context rearm, cancellation, complete material-set and native-grip consistency; offline only");
}
