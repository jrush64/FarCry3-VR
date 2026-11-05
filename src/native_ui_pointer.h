#pragma once
#include <cmath>
#include <algorithm>
namespace fc3ui {
// Use the SAME clip-space affine transform as the game's flat UI vertices.
// The input is a real native client coordinate, never an independent VR cursor.
struct PanelTransform {
 float scaleX=1,scaleY=1,offsetX=0,offsetY=0;
 bool Map(float u,float v,float& x,float& y)const {
  if(!std::isfinite(u)||!std::isfinite(v)||!std::isfinite(scaleX)||!std::isfinite(scaleY)||
     !std::isfinite(offsetX)||!std::isfinite(offsetY)||scaleX<=0||scaleY<=0)return false;
  x=((2*u-1)*scaleX+offsetX+1)*.5f;
  y=(1-((1-2*v)*scaleY+offsetY))*.5f;
  return std::isfinite(x)&&std::isfinite(y);
 }
};
struct PointerStep {float x=0,y=0;bool move=false,down=false,up=false;};
struct PointerState {
 bool active=false,button=false,waitA=false;
 PointerStep Step(bool ready,bool a,float rx,float ry,float dt,float x,float y,float width,float height){
  PointerStep out;out.x=x;out.y=y;
  if(!ready||!(width>1&&height>1)||!std::isfinite(width)||!std::isfinite(height)||!std::isfinite(x)||!std::isfinite(y)){
   out.up=button;active=button=waitA=false;return out;
  }
  if(!active){active=true;waitA=a;}
  if(!a)waitA=false;
  const bool press=a&&!waitA;out.down=press&&!button;out.up=!press&&button;button=press;
  auto axis=[](float v){if(!std::isfinite(v)||std::fabs(v)<=.2f)return 0.f;v=(std::max)(-1.f,(std::min)(1.f,v));return (v>0?1.f:-1.f)*(std::fabs(v)-.2f)/.8f;};
  dt=std::isfinite(dt)?(std::max)(0.f,(std::min)(.05f,dt)):0;
  const float dx=axis(rx)*dt*height*.85f,dy=-axis(ry)*dt*height*.85f;
  out.x=(std::max)(0.f,(std::min)(width-1,x+dx));out.y=(std::max)(0.f,(std::min)(height-1,y+dy));
  out.move=out.x!=x||out.y!=y;return out;
 }
};
}
