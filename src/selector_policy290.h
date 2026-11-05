#pragma once
#include <cmath>
#include <cstdint>
namespace fc3select290 {
struct Point {float x=0,y=0,z=0;};
inline Point Centre(int i){return {i%2?.23f:-.23f,i<2?-.10f:-.39f,-.62f};}
inline int Touch(Point hand){
 if(!std::isfinite(hand.x)||!std::isfinite(hand.y)||!std::isfinite(hand.z))return -1;
 for(int i=0;i<4;++i){const auto p=Centre(i);
  if(std::fabs(hand.x-p.x)<=.145f&&std::fabs(hand.y-p.y)<=.085f&&std::fabs(hand.z-p.z)<=.10f)return i;}
 return -1;
}
struct Gesture {
 bool open=false,pending=false,ready=false,aWas=true,gripWas=true;
 std::uint64_t last=0;int hover=-1;
 void Close(){open=pending=ready=false;hover=-1;}
 int Step(bool allowed,bool tracked,bool a,float grip,int touched,std::uint64_t now){
  const bool down=std::isfinite(grip)&&grip>(gripWas?.35f:.75f);
  const bool gap=last&&(now<last||now-last>120);last=now;
  const bool edge=a&&!aWas;aWas=a;
  const bool grab=down&&!gripWas&&!gap;gripWas=down;
  if(!allowed||!tracked){Close();ready=false;return -1;}
  if(gap)ready=false; // Keep the visible rack through a hitch; require observed neutral before grabbing.
  if(!open){if(edge&&down){open=true;ready=false;}return -1;}
  if(pending)return -1;
  if(edge){Close();return -1;} // A is released after opening; a new chord cancels.
  if(grip<.25f)ready=true;
  hover=touched;
  if(ready&&grab&&touched>=0&&touched<4){pending=true;return touched;}
  return -1;
 }
};
}
