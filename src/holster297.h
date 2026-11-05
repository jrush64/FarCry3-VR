#pragma once
#include <cmath>
namespace fc3holster297 {
enum Mode { Four=0,Single=1,Front=2 };
struct Point {float x,y,z;};
struct Settings {int mode=Four;float height=0,radius=.12f;bool guides=false,stowOutline=true;bool custom[4]={};Point positions[4]={};};
inline bool ValidPoint(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::fabs(p.x)<=1&&p.y>=-1.4f&&p.y<=.15f&&std::fabs(p.z)<=1;}
inline bool Valid(const Settings& s){if(!(s.mode>=Four&&s.mode<=Front&&std::isfinite(s.height)&&s.height>=-.25f&&s.height<=.25f&&std::isfinite(s.radius)&&s.radius>=.08f&&s.radius<=.20f))return false;for(int i=0;i<4;++i)if(s.custom[i]&&!ValidPoint(s.positions[i]))return false;return true;}
inline Point Centre(const Settings& s,int slot){
 slot=slot>=0&&slot<4?slot:0;const int placed=s.mode==Single?1:slot;
 if(s.custom[placed]){auto p=s.positions[placed];p.y+=s.height;return p;}
 if(s.mode==Single)return {.28f,-.65f+s.height,.02f};
 const Point points[4]={{-.28f,-.65f,.02f},{.28f,-.65f,.02f},{-.19f,-.60f,.22f},{.19f,-.60f,.22f}};
 auto p=points[slot>=0&&slot<4?slot:0];p.y+=s.height;return p;
}
inline void Seated(Settings& s){const Point points[4]={{-.24f,-.38f,-.30f},{.24f,-.38f,-.30f},{-.24f,-.64f,-.38f},{.24f,-.64f,-.38f}};s.height=0;for(int i=0;i<4;++i){s.custom[i]=true;s.positions[i]=points[i];}}
inline int Next(int slot,int direction,const bool* occupied){if(!occupied||slot<0||slot>=4||!direction)return slot;for(int n=1;n<=4;++n){const int i=(slot+(direction>0?n:-n)+4)%4;if(occupied[i])return i;}return slot;}
struct Flick {
 bool armed=false;
 void Begin(float x){armed=std::isfinite(x)&&std::fabs(x)<.25f;}
 int Step(float x,float y){if(!std::isfinite(x)||!std::isfinite(y)){armed=false;return 0;}if(std::fabs(x)<.25f){armed=true;return 0;}if(armed&&std::fabs(x)>.65f&&std::fabs(x)>std::fabs(y)){armed=false;return x>0?1:-1;}return 0;}
};
}
