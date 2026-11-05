#pragma once
#include <cmath>
namespace fc3handui {
struct Panel { float x,y,z,pitch,yaw,roll,width; float left,top,right,bottom; };
struct Settings { bool enabled=true,health=true,ammo=true,wheel=true,glanceOnly285=false,levelText345=false; Panel panels[3]={
 {0,.07f,-.03f,-90,0,0,.22f,0,.72f,.30f,1},
 {0,.07f,.12f,-90,0,0,.20f,.70f,.72f,1,1},
 {0,.10f,0,-90,0,0,.34f,.23f,.10f,.77f,.90f}}; };
inline void ClearWrist345(Settings& settings){settings.levelText345=true;for(int i=0;i<2;++i){auto& panel=settings.panels[i];panel.x=i?.15f:-.15f;panel.y=.07f;panel.z=0;panel.width=.16f;}}
inline float& Field(Panel& p,int i){static float Panel::* const fields[]={&Panel::x,&Panel::y,&Panel::z,&Panel::pitch,&Panel::yaw,&Panel::roll,&Panel::width,&Panel::left,&Panel::top,&Panel::right,&Panel::bottom};return p.*fields[i];}
inline float Field(const Panel& p,int i){Panel copy=p;return Field(copy,i);}
inline bool Valid(const Panel& p) { for(int i=0;i<11;++i)if(!std::isfinite(Field(p,i)))return false; return p.width>=.04f&&p.width<=1&&p.left>=0&&p.top>=0&&p.right<=1&&p.bottom<=1&&p.right-p.left>=.01f&&p.bottom-p.top>=.01f; }
inline bool Glance285(const float* head,const float* forward,const float* panel,const float* normal,bool wasVisible){
 if(!head||!forward||!panel||!normal)return false;float d[3],length=0,fl=0,nl=0;
 for(int a=0;a<3;++a){if(!std::isfinite(head[a])||!std::isfinite(forward[a])||!std::isfinite(panel[a])||!std::isfinite(normal[a]))return false;
  d[a]=panel[a]-head[a];length+=d[a]*d[a];fl+=forward[a]*forward[a];nl+=normal[a]*normal[a];}
 length=std::sqrt(length);fl=std::sqrt(fl);nl=std::sqrt(nl);if(length<.12f||length>1.3f||fl<.5f||nl<.5f)return false;
 float gaze=0,facing=0;for(int a=0;a<3;++a){gaze+=d[a]*forward[a]/(length*fl);facing-=d[a]*normal[a]/(length*nl);}
 return gaze>(wasVisible?.75f:.85f)&&facing>(wasVisible?.05f:.2f);
}
inline float PlacementAxis(float x){if(!std::isfinite(x)||std::fabs(x)<=.25f)return 0;return (x>0?1.f:-1.f)*(std::fmin(1.f,std::fabs(x))-.25f)/.75f;}
inline void AdjustPanel(Panel& p,float rx,float ry,float lx,float ly,bool rotate,float seconds){if(!std::isfinite(seconds)||seconds<=0)return;seconds=std::fmin(seconds,.05f);rx=PlacementAxis(rx);ry=PlacementAxis(ry);lx=PlacementAxis(lx);ly=PlacementAxis(ly);if(rotate){p.yaw=std::fmax(-180.f,std::fmin(180.f,p.yaw+rx*45*seconds));p.pitch=std::fmax(-180.f,std::fmin(180.f,p.pitch+ry*45*seconds));p.roll=std::fmax(-180.f,std::fmin(180.f,p.roll+lx*45*seconds));}else{p.x=std::fmax(-.5f,std::fmin(.5f,p.x+rx*.15f*seconds));p.y=std::fmax(-.5f,std::fmin(.5f,p.y+ry*.15f*seconds));p.z=std::fmax(-.5f,std::fmin(.5f,p.z+ly*.15f*seconds));}}
}
