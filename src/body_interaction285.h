#pragma once
#include <cmath>
#include <cstdint>

// Pure interaction policy. Positions are metres in a level head-relative frame;
// no engine calls, inventory writes, camera writes or fabricated hand poses.
namespace fc3body285 {
struct Vec { float x=0,y=0,z=0; };
inline bool Finite(Vec p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline float Distance(Vec a,Vec b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
struct Slot { Vec position; float radius=.16f; int sector=0; };
struct Settings {
 bool enabled=true,pointWheel=true;
 Slot slots[5]={{{.25f,-.64f,.02f},.16f,0},{{-.25f,-.64f,.02f},.16f,1},
                {{.23f,-.17f,-.12f},.16f,2},{{-.23f,-.17f,-.12f},.16f,3},
                {{0,-.62f,.10f},.12f,8}};
 float pull=.12f;
};
struct Input {
 bool allowed=false,tracked=false,conflict=false; float grip=0;
 Vec hand,motion; std::uint64_t now=0; unsigned epoch=0;
};
struct Result { int hover=-1,draw=-1; bool pulse=false,holding=false; };
struct State {
 bool armed=false,down=false; int hover=-1,grab=-1;
 std::uint64_t hoverAt=0,grabAt=0,lastAt=0; unsigned epoch=0;
 Vec origin;
 void Reset(){armed=false;down=false;hover=grab=-1;hoverAt=grabAt=0;}
 Result Step(const Settings& settings,const Input& in){
  Result out;
  const bool gap=lastAt&&(in.now<lastAt||in.now-lastAt>150);
  const bool changed=epoch!=in.epoch;epoch=in.epoch;lastAt=in.now;
  if(!settings.enabled||!std::isfinite(settings.pull)||settings.pull<.08f||settings.pull>.3f||!in.allowed||!in.tracked||in.conflict||!Finite(in.hand)||!Finite(in.motion)||!std::isfinite(in.grip)||gap||changed){Reset();return out;}
  if(in.grip<.25f){armed=true;down=false;grab=-1;}
  const bool pressed=in.grip>.75f&&!down;
  if(in.grip>.75f)down=true;
  if(grab>=0){
   if(in.now-grabAt>2000){Reset();return out;}
   if(in.grip>.5f&&Distance(in.motion,origin)>=settings.pull){out.draw=settings.slots[grab].sector;Reset();return out;}
   out.holding=true;out.hover=grab;return out;
  }
  int nearest=-1;float best=1e9f;
  for(int n=0;n<5;++n){const auto& slot=settings.slots[n];const float d=Distance(in.hand,slot.position);
   if(slot.sector<0||slot.sector>8||!Finite(slot.position)||!std::isfinite(slot.radius)||slot.radius<.06f||slot.radius>.3f)continue;
   const float margin=n==hover?.025f:0;
   if(d<=slot.radius+margin&&d<best){best=d;nearest=n;}
  }
  if(nearest!=hover){hover=nearest;hoverAt=in.now;}
  if(armed&&hover>=0&&in.now-hoverAt>=100){
   out.hover=hover;
   if(pressed){grab=hover;origin=in.motion;grabAt=in.now;armed=false;out.holding=true;out.pulse=true;}
  }
  // Squeezing elsewhere cannot acquire a holster by moving a held grip into it.
  if(pressed&&!out.holding)armed=false;
  return out;
 }
};
inline Vec HeadPosition(const float* view){
 if(!view)return {NAN,NAN,NAN};
 return {-(view[0]*view[3]+view[4]*view[7]+view[8]*view[11]),
         -(view[1]*view[3]+view[5]*view[7]+view[9]*view[11]),
         -(view[2]*view[3]+view[6]*view[7]+view[10]*view[11])};
}
inline Vec Relative(Vec hand,Vec head,float yaw,float scale){
 if(!Finite(hand)||!Finite(head)||!std::isfinite(yaw)||!std::isfinite(scale)||scale<.1f)return {NAN,NAN,NAN};
 const float x=(hand.x-head.x)/scale,z=(hand.z-head.z)/scale;
 return {std::cos(yaw)*x-std::sin(yaw)*z,(hand.y-head.y)/scale,std::sin(yaw)*x+std::cos(yaw)*z};
}
inline bool Point(const float* hand,const float* view,float& x,float& y){
 x=y=0;if(!hand||!view)return false;
 const Vec d={hand[2],hand[6],hand[10]};if(!Finite(d))return false;
 const float right=view[0]*d.x+view[1]*d.y+view[2]*d.z;
 const float up=view[4]*d.x+view[5]*d.y+view[6]*d.z;
 const float forward=view[8]*d.x+view[9]*d.y+view[10]*d.z;
 if(!std::isfinite(right)||!std::isfinite(up)||!std::isfinite(forward)||forward<.2f)return false;
 const float length=std::sqrt(right*right+up*up);if(length<.17f)return false;
 x=right/length;y=up/length;return true;
}
inline void Sector(int sector,float& x,float& y){
 x=y=0;switch(sector){case 0:x=1;break;case 1:y=1;break;case 2:x=-1;break;case 3:y=-1;break;
 case 4:x=y=.70710678f;break;case 5:x=-.70710678f;y=.70710678f;break;
 case 6:x=y=-.70710678f;break;case 7:x=.70710678f;y=-.70710678f;break;}
}
}
