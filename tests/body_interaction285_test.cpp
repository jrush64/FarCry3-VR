#include "../src/body_interaction285.h"
#include "../src/hand_ui_settings.h"
#include <cstdio>
#include <cstdlib>
using namespace fc3body285;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
Input At(const Settings& settings,int n,std::uint64_t now,float grip=0){Input in;in.allowed=in.tracked=true;in.hand=in.motion=settings.slots[n].position;in.now=now;in.grip=grip;return in;}
int main(){
 Settings settings;
 for(int n=0;n<5;++n){State state;auto in=At(settings,n,1000);Check(state.Step(settings,in).draw<0,"neutral cannot draw");
  in.now+=110;Check(state.Step(settings,in).hover==n,"dwell identifies requested body zone");
  in.now+=10;in.grip=1;Check(state.Step(settings,in).holding,"fresh squeeze acquires zone");
  in.now+=10;in.hand.y+=.25f;Check(state.Step(settings,in).draw<0,"head-relative motion alone never draws");
  in.now+=10;in.motion.y+=.13f;Check(state.Step(settings,in).draw==settings.slots[n].sector,"real tracked pull draws exactly chosen action");
  for(int k=0;k<120;++k){in.now+=10;Check(state.Step(settings,in).draw<0,"held squeeze cannot repeat selection");}
 }
 puts("PASS: five zones, dwell, squeeze/pull, head-only motion rejection, once per grip");
 for(int failure=0;failure<7;++failure){State state;auto in=At(settings,0,1000);state.Step(settings,in);in.now+=110;state.Step(settings,in);in.grip=1;in.now+=10;state.Step(settings,in);
  in.now+=10;switch(failure){case 0:in.tracked=false;break;case 1:in.allowed=false;break;case 2:in.conflict=true;break;case 3:in.epoch++;break;case 4:in.now+=300;break;case 5:in.motion.x=NAN;break;case 6:in.grip=NAN;break;}
  Check(state.Step(settings,in).draw<0,"loss/conflict/discontinuity never draws");
  in=At(settings,0,in.now+10,1);in.motion.y+=.3f;Check(state.Step(settings,in).draw<0,"reacquisition with held grip cannot draw");
 }
 {State state;auto in=At(settings,0,1000,1);state.Step(settings,in);in.now+=110;state.Step(settings,in);in.motion.y+=.3f;in.now+=10;Check(state.Step(settings,in).draw<0,"startup with grip held requires release");}
 {State state;auto in=At(settings,0,1000);in.hand={1,0,0};state.Step(settings,in);in.grip=1;in.now+=10;state.Step(settings,in);in.hand=settings.slots[0].position;in.now+=110;Check(!state.Step(settings,in).holding,"held grip cannot acquire by entering zone");}
 puts("PASS: tracking/gameplay/conflict/epoch/time-gap/NaN and held-on-start guards");
 float view[16]={1,0,0,-2,0,1,0,-3,0,0,1,-4,0,0,0,1};
 const auto head=HeadPosition(view);Check(Distance(head,{2,3,4})<1e-5f,"inverse view recovers translated head");
 Check(Distance(Relative({3,3,4},head,1.57079632679f,2),{0,0,.5f})<1e-5f,"body zones respect yaw, translation and world scale");
 float hand[16]={1,0,.5f,0,0,1,.3f,0,0,0,1,0,0,0,0,1};float x=0,y=0;
 Check(Point(hand,view,x,y)&&x>y&&y>0,"controller points into head-relative wheel quadrant");
 hand[10]=-1;Check(!Point(hand,view,x,y),"backward controller cannot choose opposite wheel sector");
 hand[10]=1;hand[2]=hand[6]=0;Check(!Point(hand,view,x,y),"straight-ahead deadzone prevents arbitrary sector");
 hand[2]=NAN;Check(!Point(hand,view,x,y),"invalid controller orientation cannot select");
 for(int n=0;n<8;++n){Sector(n,x,y);Check(std::fabs(x*x+y*y-1)<1e-5f,"all eight wheel directions have unit magnitude");}
 puts("PASS: head inverse, body yaw/scale, wheel geometry and eight sectors");
 {
  float head[3]={},forward[3]={0,0,1},panel[3]={0,0,.5f},normal[3]={0,0,-1};
  Check(fc3handui::Glance285(head,forward,panel,normal,false),"raised facing wrist is visible");
  normal[2]=1;Check(!fc3handui::Glance285(head,forward,panel,normal,true),"back of wrist cannot display health through the arm");normal[2]=-1;
  panel[0]=.35f;Check(!fc3handui::Glance285(head,forward,panel,normal,false)&&fc3handui::Glance285(head,forward,panel,normal,true),"glance entry/exit hysteresis prevents boundary flicker");
  panel[0]=0;panel[2]=2;Check(!fc3handui::Glance285(head,forward,panel,normal,true),"distant stale wrist cannot show HUD");
  panel[2]=NAN;Check(!fc3handui::Glance285(head,forward,panel,normal,true),"invalid wrist cannot show HUD");
  Check(!fc3handui::Settings{}.glanceOnly285,"existing wrist visibility is preserved by default");
  puts("PASS: optional wrist glance gaze/facing/distance/hysteresis and preserved defaults");
 }
}
