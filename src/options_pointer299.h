#pragma once
#include "native_ui_ray298.h"
namespace fc3ui {
inline float OptionsMetres299(float width,float height){
 const float scale=(std::max)(1.f,(std::min)(2.5f,height/900.f));
 return 1.05f/(std::max)(1.f,(std::min)(1020.f*scale,width-16.f));
}
inline bool OptionsHit299(RayPoint298 origin,RayPoint298 direction,float width,float height,int x,int y,int w,int h,bool captured,float& u,float& v){
 if(!std::isfinite(width)||!std::isfinite(height)||width<32||height<32||x<0||y<0||w<=0||h<=0||x>width||y>height||w>width-x||h>height-y)return false;
 float eyeU,eyeV;if(!ProjectRay298(origin,direction,1.15f,-1,1,-1,1,eyeU,eyeV))return false;
 const float metres=OptionsMetres299(width,height),px=(2*eyeU-1)*1.15f/metres+width*.5f,py=(2*eyeV-1)*1.15f/metres+height*.5f;
 if(px<0||px>=width||py<0||py>=height||(!captured&&(px<x||px>=x+w||py<y||py>=y+h)))return false;
 u=px/width;v=py/height;return true;
}
struct OptionsChordStep299 {bool toggle=false,consume=false;};
struct OptionsChord299 {
 bool menuWas=true,owned=false;
 OptionsChordStep299 Step(bool active,bool menu,float rightGrip,float leftGrip){
  if(!active){menuWas=true;return {false,owned};}
  const bool edge=menu&&!menuWas;menuWas=menu;
  if(!menu){owned=false;return {};}
  if(edge&&std::isfinite(rightGrip)&&std::isfinite(leftGrip)&&rightGrip>.75f&&leftGrip<.35f){owned=true;return {true,true};}
  return {false,owned};
 }
};
}
