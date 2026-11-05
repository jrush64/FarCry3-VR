#include "../src/magazine_pull286.h"
#include <cstdio>
#include <cstdlib>
using namespace fc3pull286;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
Input Ready(){Input in;in.allowed=in.eligible=true;in.hand={0,0,0};in.forward={0,0,1};in.target={0,0,1};in.key={1,2,3,4,20,5};in.now=1000;return in;}
void Acquire(State& state,Input& in){in.grip=0;state.Step(in);in.now+=10;in.grip=1;Check(state.Step(in).acquired,"fresh pointed grip acquires existing item");}
int main(){
 {State state;auto in=Ready();Acquire(state,in);in.now+=20;in.hand.z=-.06;Check(!state.Step(in).committed,"short pull cannot commit");
  in.now+=20;in.hand.z=-.13;Check(state.Step(in).committed,"backward physical pull commits once");
  for(int n=0;n<100;++n){in.now+=10;Check(!state.Step(in).committed,"held grip cannot repeat recovery");}}
 {State state;auto in=Ready();in.grip=1;Check(!state.Step(in).acquired,"startup with held grip cannot acquire");
  in.grip=0;state.Step(in);in.now+=10;in.grip=1;in.target={1,0,0};Check(!state.Step(in).acquired,"sideways target cannot acquire");
  in.target={0,0,1};in.now+=10;Check(!state.Step(in).acquired,"aiming after a held squeeze cannot acquire");}
 {State state;auto in=Ready();Acquire(state,in);in.hand.x=.15;in.now+=20;Check(!state.Step(in).committed,"sideways movement is not a backward pull");}
 for(int failure=0;failure<13;++failure){State state;auto in=Ready();Acquire(state,in);in.now+=20;
  switch(failure){case 0:in.allowed=false;break;case 1:in.eligible=false;break;case 2:in.grip=0;break;case 3:in.key.owner++;break;case 4:in.key.weapon++;break;case 5:in.key.item++;break;case 6:in.key.epoch++;break;case 7:in.key.profile++;break;case 8:in.key.reference++;break;case 9:in.now+=200;break;case 10:in.hand.z=-1;break;case 11:in.hand.x=NAN;break;case 12:in.forward.z=INFINITY;break;}
  Check(!state.Step(in).committed,"invalid ownership, tracking or motion cannot commit");in=Ready();in.now+=300;in.grip=1;in.hand.z=-.15;Check(!state.Step(in).committed,"interrupted pull cannot resume on held grip");}
 {State state;auto in=Ready();in.target={0,0,3.01};state.Step(in);in.now+=10;in.grip=1;Check(!state.Step(in).acquired,"out of range item cannot acquire");}
 Vec point={0,0,2},next;
 for(int n=0;n<30;++n){Check(Advance(point,{0,0,0},.016,next),"bounded return movement succeeds");Check(Length(next)<=Length(point)&&Length(Sub(next,point))<=.096001,"return approaches hand at bounded speed without overshoot");point=next;}
 Check(Length(point)<1e-8,"return reaches hand");Check(!Advance(point,{NAN,0,0},.016,next)&&!Advance(point,{0,0,0},.2,next),"invalid destination or frame gap cannot move object");
 puts("PASS:286 point/grip/backward pull, one event,13 cancellation cases, range and bounded return path");
}
