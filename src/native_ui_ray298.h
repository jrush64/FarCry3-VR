#pragma once
#include "native_ui_pointer.h"
namespace fc3ui {
struct RayPoint298 {float x,y,z;};
// Eye-local OpenXR coordinates. Intersect the same depth plane as native UI,
// then project with the FOV actually submitted, including asymmetric frusta.
inline bool ProjectRay298(RayPoint298 p,RayPoint298 d,float depth,float left,float right,float down,float up,float& u,float& v){
 if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(d.x)||!std::isfinite(d.y)||!std::isfinite(d.z)||!std::isfinite(depth)||depth<.01f||!std::isfinite(left)||!std::isfinite(right)||!std::isfinite(down)||!std::isfinite(up)||right<=left||up<=down||d.z>=-.0001f)return false;
 const float t=(-depth-p.z)/d.z;if(!std::isfinite(t)||t<=0)return false;
 const float x=p.x+t*d.x,y=p.y+t*d.y;
 u=(1+(2*x/depth-right-left)/(right-left))*.5f;
 v=(1-(2*y/depth-up-down)/(up-down))*.5f;
 return std::isfinite(u)&&std::isfinite(v);
}
inline bool NativeRayHit298(const PanelTransform* panels,const float* screen,float& u,float& v){
 float x[2],y[2];if(!panels||!screen)return false;
 for(int i=0;i<2;++i){const auto& p=panels[i];if(!std::isfinite(p.scaleX)||!std::isfinite(p.scaleY)||p.scaleX<=0||p.scaleY<=0||!std::isfinite(p.offsetX)||!std::isfinite(p.offsetY)||!std::isfinite(screen[i*2])||!std::isfinite(screen[i*2+1])||screen[i*2]<0||screen[i*2]>1||screen[i*2+1]<0||screen[i*2+1]>1)return false;
  x[i]=((2*screen[i*2]-1-p.offsetX)/p.scaleX+1)*.5f;y[i]=(1-(1-2*screen[i*2+1]-p.offsetY)/p.scaleY)*.5f;
 }
 // A stale/mismatched panel cannot silently click an unrelated native position.
 if(std::fabs(x[0]-x[1])>.05f||std::fabs(y[0]-y[1])>.05f)return false;
 u=(x[0]+x[1])*.5f;v=(y[0]+y[1])*.5f;
 return std::isfinite(u)&&std::isfinite(v)&&u>=0&&u<=1&&v>=0&&v<=1;
}
struct RayInput298 {bool point=false,click=false,reserveTrigger=false;float u=0,v=0;};
struct RayState298 {
 bool armed=false,pressed=false,reserved=false,stickOverride=false,filtered=false;
 float referenceU=0,referenceV=0,u=0,v=0;
 RayInput298 Step(bool ready,bool hit,float targetU,float targetV,float trigger,float grip,float rx,float ry,float dt){
  RayInput298 out;const bool valid=std::isfinite(trigger)&&std::isfinite(grip)&&std::isfinite(rx)&&std::isfinite(ry)&&std::isfinite(dt)&&dt>=0&&dt<=.12f;
  if(!ready){armed=pressed=stickOverride=filtered=false;if(std::isfinite(trigger)&&trigger<.25f)reserved=false;out.reserveTrigger=reserved;return out;}
  const bool stick=std::fabs(rx)>.2f||std::fabs(ry)>.2f;
  if(stick){stickOverride=true;referenceU=targetU;referenceV=targetV;}
  else if(stickOverride&&hit&&std::isfinite(targetU)&&std::isfinite(targetV)&&((targetU-referenceU)*(targetU-referenceU)+(targetV-referenceV)*(targetV-referenceV)>.0025f))stickOverride=false;
  const bool pointing=valid&&hit&&std::isfinite(targetU)&&std::isfinite(targetV)&&!stickOverride&&grip<.5f;
  if(!pointing){armed=pressed=filtered=false;}else{
   if(trigger<.25f)armed=true;
   pressed=armed&&(pressed?trigger>=.25f:trigger>.75f);
   reserved=true;
   if(!filtered){u=targetU;v=targetV;filtered=true;}else{const float blend=1-std::exp(-35*dt);u+=(targetU-u)*blend;v+=(targetV-v)*blend;}
   out.point=true;out.click=pressed;out.u=u;out.v=v;
  }
  // Leaving the panel while squeezing releases the click, but cannot convert
  // that same squeeze into native map zoom. A neutral trigger ends the lease.
  if(!pointing&&std::isfinite(trigger)&&trigger<.25f)reserved=false;
  out.reserveTrigger=reserved;return out;
 }
};
}
