#pragma once
#include <cmath>
#include <cstdint>
namespace fc3pull286 {
struct Vec {double x=0,y=0,z=0;};
inline bool Finite(Vec p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline Vec Sub(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline double Dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double Length(Vec a){return std::sqrt(Dot(a,a));}
struct Key {
 std::uint64_t owner=0,weapon=0;unsigned item=0,epoch=0;int profile=-1;unsigned reference=0;
 bool operator==(const Key& other)const{return owner==other.owner&&weapon==other.weapon&&item==other.item&&epoch==other.epoch&&profile==other.profile&&reference==other.reference;}
 bool Valid()const{return owner&&weapon&&item&&profile>=0;}
};
struct Input {
 bool allowed=false,eligible=false;double grip=0,range=2.5,pull=.12;
 Vec hand,forward,target;Key key;std::uint64_t now=0;
};
struct Result {bool acquired=false,committed=false;};
struct State {
 bool armed=false,down=false,active=false;Key key;
 Vec origin,axis,lastHand;std::uint64_t started=0,lastAt=0;
 void Reset(){armed=down=active=false;started=lastAt=0;key={};}
 void ObserveGrip(double grip){if(!std::isfinite(grip)){Reset();return;}if(grip<.25){armed=true;down=false;}else if(grip>.75){down=true;armed=false;}}
 Result Step(const Input& in){
  Result result;
  const bool valid=in.allowed&&Finite(in.hand)&&Finite(in.forward)&&Finite(in.target)&&std::isfinite(in.grip)&&
   std::isfinite(in.range)&&in.range>=.5&&in.range<=3&&std::isfinite(in.pull)&&in.pull>=.08&&in.pull<=.3;
  if(!valid){Reset();return result;}
  if(in.grip<.25){armed=true;down=false;active=false;}
  const bool pressed=in.grip>.75&&!down;if(in.grip>.75)down=true;
  const Vec delta=Sub(in.target,in.hand);const double distance=Length(delta),norm=Length(in.forward);
  if(!in.eligible||!in.key.Valid()||!std::isfinite(distance)||distance<.20||distance>in.range||norm<.5||norm>1.5){active=false;if(pressed)armed=false;return result;}
  const double alignment=Dot(delta,in.forward)/(distance*norm);
  if(active){
   const bool gap=in.now<lastAt||in.now-lastAt>100;
   if(!(key==in.key)||in.grip<.5||gap||in.now<started||in.now-started>1000||Length(Sub(in.hand,lastHand))>.25||alignment<.90){Reset();return result;}
   lastAt=in.now;lastHand=in.hand;
   if(Dot(Sub(origin,in.hand),axis)>=in.pull){result.committed=true;Reset();}
   return result;
  }
  if(pressed){
   if(armed&&alignment>=.985){active=true;armed=false;origin=lastHand=in.hand;axis={in.forward.x/norm,in.forward.y/norm,in.forward.z/norm};key=in.key;started=lastAt=in.now;result.acquired=true;}
   else armed=false;
  }
  return result;
 }
};
inline bool Advance(Vec current,Vec destination,double seconds,Vec& next){
 next=current;if(!Finite(current)||!Finite(destination)||!std::isfinite(seconds)||seconds<0||seconds>.1)return false;
 const Vec delta=Sub(destination,current);const double distance=Length(delta),travel=6*seconds;
 if(distance<1e-8){next=destination;return true;}
 const double f=travel<distance?travel/distance:1;next={current.x+delta.x*f,current.y+delta.y*f,current.z+delta.z*f};return true;
}
}
