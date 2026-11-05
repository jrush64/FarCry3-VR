#include "../src/native_ui_pointer.h"
#include <cstdio>
#include <cstdlib>
void Check(bool x,const char* m){if(!x){std::fprintf(stderr,"FAIL: %s\n",m);std::exit(1);}}
int main(){
 for(float sx:{.45f,.7f,1.f})for(float sy:{.35f,.65f,1.f})for(float ox:{-.06f,.07f})for(float oy:{-.08f,.03f}){
  fc3ui::PanelTransform panel={sx,sy,ox,oy};
  for(float u:{0.f,.25f,.5f,.75f,1.f})for(float v:{0.f,.25f,.5f,.75f,1.f}){
   float x=0,y=0;Check(panel.Map(u,v,x,y),"valid panel map");
   Check(std::fabs((2*x-1)-((2*u-1)*sx+ox))<1e-6f,"cursor X matches game's native flat vertex transform");
   Check(std::fabs((1-2*y)-((1-2*v)*sy+oy))<1e-6f,"cursor Y matches game's native flat vertex transform");
  }
 }
 fc3ui::PanelTransform bad;bad.scaleX=NAN;float x,y;Check(!bad.Map(.5f,.5f,x,y),"invalid panel rejected");
 fc3ui::PointerState state;float u=100,v=50;
 for(int i=0;i<120;++i){auto out=state.Step(true,false,.3f,0,1.f/120.f,u,v,400,200);u=out.x;v=out.y;}
 Check(u>115,"subpixel stick increments accumulate instead of rounding away per frame");
 auto out=state.Step(true,true,0,0,.01f,u,v,400,200);Check(out.down,"A clicks native pointer after neutral entry");
 out=state.Step(false,true,0,0,.01f,u,v,400,200);Check(out.up,"focus loss releases only owned click");
 std::puts("PASS: UI34 cursor/native panel coordinate agreement, subpixel accumulation and click lifetime");
}
