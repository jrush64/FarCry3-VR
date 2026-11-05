#pragma once
#include <windows.h>
#include <cmath>
#include <cstring>
namespace fc3capture331 {
// A timer alone cannot establish that a native model/material/hand was drawn.
// Only call Frame after the existing complete-frame capture checks succeed.
struct Gate {
 int slot=-1;LONG last=-1;ULONGLONG since=0;unsigned long long signature=0;
 float fingers[25*12]={};
 void Reset(){slot=-1;last=-1;since=0;signature=0;}
 bool Frame(int target,LONG frame,ULONGLONG now,unsigned long long shape,const float* pose){
  bool same=slot==target&&frame==last+1&&signature==shape&&now>=since;
  for(int b=3;b<20;++b)for(int i=0;i<12;++i){const float a=pose[b*12+i],p=fingers[b*12+i];if(!std::isfinite(a)||!std::isfinite(p)||std::fabs(a-p)>(i%4==3?.015f:.1f))same=false;}
  if(!same)since=now;
  slot=target;last=frame;signature=shape;memcpy(fingers,pose,sizeof(fingers));
  return same&&now-since>=60;
 }
};
}
