#pragma once
#include "selector290.h"
#include <cstring>
namespace fc3prepare330 {
inline bool Same(const fc3select290::Inventory& a,const fc3select290::Inventory& b){
 if(a.owner!=b.owner)return false;for(int i=0;i<4;++i)if(a.weapons[i].entity!=b.weapons[i].entity||a.weapons[i].id!=b.weapons[i].id||strcmp(a.weapons[i].name,b.weapons[i].name))return false;return true;
}
struct Output {int request=-1,target=-1;bool active=false,started=false,finished=false,changed=false,failed=false,retry331=false,rearmed331=false;};
struct State {
 fc3select290::Inventory identity;bool known=false,active=false,restore=false;int original=-1,target=-1,phase=0;unsigned failed=0;ULONGLONG changedAt=0,idleAt=0,at=0,began=0,pause=0;
 unsigned attempts331[4]={},settledEntity331=0;ULONGLONG settledAt331=0;bool rearmed331=false;
 Output Step(const fc3select290::Inventory& inv,unsigned ready,bool eligible,bool idle,int result,ULONGLONG now){
  Output out;if(!inv.valid||!eligible){settledEntity331=0;settledAt331=0;if(active&&!pause)pause=now;if(!idle)idleAt=0;return out;}
  if(pause){const auto delay=now>=pause?now-pause:0;at+=delay;began+=delay;pause=0;}
  if(!known||!Same(identity,inv)){out.changed=known;identity=inv;known=true;active=restore=false;original=target=-1;phase=0;failed=0;changedAt=now;idleAt=0;memset(attempts331,0,sizeof(attempts331));rearmed331=false;settledEntity331=0;settledAt331=0;}
  if(inv.equipped!=settledEntity331){settledEntity331=inv.equipped;settledAt331=now;}
  unsigned present=0;for(int i=0;i<4;++i)if(inv.weapons[i].entity)present|=1u<<i;
  if(!active){
   // One fresh bounded recovery pass after native weapon drawing really
   // returns. A transient interrupted preparation is not permanent cache state.
   if(failed&&!rearmed331&&settledEntity331&&now-settledAt331>=750){failed=0;memset(attempts331,0,sizeof(attempts331));rearmed331=out.rearmed331=true;idleAt=0;}
   if(!idle){idleAt=0;return out;}if(!idleAt)idleAt=now;
   if(!(present&~ready&~failed)||now-changedAt<500||now-idleAt<300)return out;
   original=-1;for(int i=0;i<4;++i)if(inv.weapons[i].entity&&inv.equipped==inv.weapons[i].entity)original=i;
   active=true;restore=false;began=now;phase=0;out.started=true;
  }
  out.active=true;
  if(!idle)restore=true;
  if(now-began>20000){restore=true;failed|=present&~ready;}
  const auto retry=[&](){if(restore||attempts331[target]>=2)return false;++attempts331[target];phase=1;at=now;out.request=out.target=target;out.retry331=true;return true;};
  if(phase==1){
   if(inv.equipped==inv.weapons[target].entity){phase=2;at=now;}
   else if(result<0||now-at>2500){if(retry())return out;failed|=1u<<target;target=-1;phase=0;out.failed=true;}
   else{out.target=target;return out;}
  }
  if(phase==2){
   if(ready&(1u<<target)){target=-1;phase=0;}
   else if(inv.equipped!=inv.weapons[target].entity||now-at>3000){if(retry())return out;if(!restore){failed|=1u<<target;out.failed=true;}target=-1;phase=0;}
   else{out.target=target;return out;}
  }
  if(phase==3){
   if(inv.equipped==inv.weapons[original].entity||result<0||now-at>2500){out.failed=inv.equipped!=inv.weapons[original].entity;active=false;out.active=false;out.finished=true;target=-1;return out;}
   return out;
  }
  if(!restore){for(int i=0;i<4;++i)if((present&~ready&~failed)&(1u<<i)){target=i;break;}}
  if(restore||target<0){
   if(original>=0&&inv.equipped!=inv.weapons[original].entity){phase=3;at=now;out.request=original;return out;}
   active=false;out.active=false;out.finished=true;return out;
  }
  at=now;out.target=target;
  if(inv.equipped==inv.weapons[target].entity)phase=2;
  else{phase=1;out.request=target;}
  return out;
 }
};
}
