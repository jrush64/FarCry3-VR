#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace fc3bars355 {
constexpr double Pi=3.14159265358979323846;
struct Point{double x=0,y=0,z=0;};
inline Point Turn(Point p,double a){return {p.x*std::cos(a)-p.y*std::sin(a),p.x*std::sin(a)+p.y*std::cos(a),p.z};}
struct Hand{Point p;float grip=0;bool tracked=false;};
struct Input{std::uint64_t id=0,now=0;int model=-1;bool allowed=false;double nativeAngle=0,lock=35*Pi/180,smoothing363=0;Point end[2];Hand hand[2];};
struct Bars{
 std::uint64_t id=0,at=0;int model=-1;double angle=0,target363=0; double polar[2]={};bool held[2]={},wait[2]={};
 unsigned release363[2]={};
 void Drop(unsigned reason=6){for(int h=0;h<2;++h)if(held[h])release363[h]=reason;held[0]=held[1]=false;}
 bool Active()const{return held[0]||held[1];}
 bool Update(const Input& i){
  const auto previous363=at;
  const bool stale=at&&(i.now<at||i.now-at>150);
  if(id!=i.id||model!=i.model){Drop();id=i.id;model=i.model;angle=target363=i.nativeAngle;}
  if(!i.allowed||!i.id||i.model<0||stale||!std::isfinite(i.lock)||i.lock<.1||i.lock>1.3||!std::isfinite(i.nativeAngle)){
   Drop(stale?7:6);wait[0]=wait[1]=true;at=i.now;return false;
  }
  at=i.now;double sum=0;int count=0;
  auto valid=[](const Hand& h){return h.tracked&&std::isfinite(h.p.x)&&std::isfinite(h.p.y)&&std::isfinite(h.p.z)&&std::isfinite(h.grip);};
  for(int h=0;h<2;++h){if(i.hand[h].grip<.3f)wait[h]=false;if(!held[h])continue;
   const auto& hand=i.hand[h];const double a=std::atan2(hand.p.y,hand.p.x),turn=std::remainder(a-polar[h],2*Pi);
   if(!valid(hand)||hand.grip<.35f||std::fabs(hand.p.z-i.end[h].z)>.25||
      std::fabs(std::hypot(hand.p.x,hand.p.y)-std::hypot(i.end[h].x,i.end[h].y))>.25||std::fabs(turn)>.85){release363[h]=!hand.tracked?1:hand.grip<.35f?2:std::fabs(hand.p.z-i.end[h].z)>.25?3:std::fabs(std::hypot(hand.p.x,hand.p.y)-std::hypot(i.end[h].x,i.end[h].y))>.25?4:5;held[h]=false;wait[h]=hand.grip>=.3f;continue;}
   polar[h]=a;sum+=turn;++count;
  }
  if(count){target363=(std::max)(-i.lock,(std::min)(i.lock,target363+sum/count));const double dt=previous363&&i.now>=previous363?(i.now-previous363)*.001:0;const double alpha=i.smoothing363>0&&std::isfinite(i.smoothing363)?-std::expm1(-dt/i.smoothing363):1;angle+=alpha*(target363-angle);}else angle=target363=i.nativeAngle;
  for(int h=0;h<2;++h)if(!held[h]&&!wait[h]&&valid(i.hand[h])&&i.hand[h].grip>.7f){
   const Point target=Turn(i.end[h],angle),p=i.hand[h].p;
   if(std::sqrt((p.x-target.x)*(p.x-target.x)+(p.y-target.y)*(p.y-target.y)+(p.z-target.z)*(p.z-target.z))<.12){held[h]=true;polar[h]=std::atan2(p.y,p.x);}
  }
  return Active();
 }
 double Demand(double lock)const{return Active()?(std::max)(-1.,(std::min)(1.,-angle/lock)):0;}
};
}
