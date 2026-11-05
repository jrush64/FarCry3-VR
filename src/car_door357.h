#pragma once
#include <cmath>
#include <cstdint>
namespace fc3door357 {
struct Point {double x=0,y=0,z=0;};
inline bool Finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline double Distance(Point a,Point b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
struct Input {bool allowed=false,tracked=false;float grip=0;Point hand,handle,anchor;unsigned epoch=0;std::uint64_t now=0;};
struct Result {bool held=false,enter=false,grabbed=false;};
struct State {
 bool armed=false,held=false,spent=false;unsigned epoch=0;std::uint64_t at=0,began=0;Point anchor,start;
 void Cancel(){held=false;armed=false;spent=false;}
 Result Step(const Input& in){
  Result out;const bool gap=at&&(in.now<at||in.now-at>120);at=in.now;
  if(!in.allowed||!in.tracked||!Finite(in.hand)||!Finite(in.handle)||!Finite(in.anchor)||!std::isfinite(in.grip)||gap||epoch!=in.epoch){epoch=in.epoch;Cancel();return out;}
  if(in.grip<.25f){Cancel();armed=true;return out;}
  if(held){
   if(Distance(anchor,in.anchor)>.04||in.grip<.35f||in.now<began||in.now-began>1800||Distance(in.hand,in.handle)>.35){Cancel();return out;}
   out.held=true;
   // Driver's exterior door pulls toward negative asset X. Tangential hand
   // motion, head turning and walking with a held controller cannot count.
   if(!spent&&start.x-in.hand.x>=.075&&std::fabs(start.y-in.hand.y)<.15&&std::fabs(start.z-in.hand.z)<.15){spent=true;out.enter=true;}
   return out;
  }
  if(armed&&in.grip>.7f&&Distance(in.hand,in.handle)<=.115){held=true;armed=false;spent=false;start=in.hand;anchor=in.anchor;began=in.now;out.held=out.grabbed=true;}
  else if(in.grip>.7f)armed=false;
  return out;
 }
};
}
