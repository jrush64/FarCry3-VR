#include "../src/xr_body_pose.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace fc3xr;
void Check(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
float Gap(XrVector3f a,XrVector3f b){return std::fabs(a.x-b.x)+std::fabs(a.y-b.y)+std::fabs(a.z-b.z);}
int main(){unsigned count=0,oldFailed=0;const XrVector3f reference={1.3f,-.7f,4.2f},offset={.08f,.05f,-.04f};
 for(float yaw:{0.f,.8f,-1.2f,1.57079632679f})for(float scale:{.4f,1.f,2.f})for(int tick=0;tick<60;++tick){const float t=tick*.07f;const XrQuaternionf parent=PoseMultiply({0,std::sin(t*.5f),0,std::cos(t*.5f)},{std::sin(t*.17f),0,0,std::cos(t*.17f)});const XrVector3f at={.3f+std::sin(t)*.2f,1.1f+std::cos(t)*.1f,-.5f+t*.04f};const XrQuaternionf inverse={0,std::sin(-yaw*.5f),0,std::cos(-yaw*.5f)},relative=PoseMultiply(inverse,parent);float body[16]={};const float dimensions[3]={.5f,2.f,1.3f};
  for(int col=0;col<3;++col){XrVector3f axis={};if(col==0)axis.x=1;else if(col==1)axis.y=1;else axis.z=1;auto rotated=PoseRotate(relative,axis);const float v[3]={rotated.x,rotated.y,rotated.z};for(int row=0;row<3;++row)body[row*4+col]=v[row]*dimensions[col]*(row==2?-1.f:1.f)*(col==2?-1.f:1.f);}
  const auto position=PoseRotate(inverse,{at.x-reference.x,at.y-reference.y,at.z-reference.z});body[3]=position.x*scale;body[7]=position.y*scale;body[11]=-position.z*scale;body[15]=1;XrPosef recovered={};Check(BodyToReferencePose(body,scale,yaw,reference,recovered),"scaled drawn body pose is valid");Check(Gap(recovered.position,at)<1e-5f,"translated/recentered parent origin round trips");const auto intendedOffset=PoseRotate(parent,offset),actualOffset=PoseRotate(recovered.orientation,offset);Check(Gap(actualOffset,intendedOffset)<1e-5f,"local hand UI attachment remains invariant through dynamic parent rotation and recenter yaw");
  const auto local=PoseRotate({-recovered.orientation.x,-recovered.orientation.y,-recovered.orientation.z,recovered.orientation.w},actualOffset);Check(Gap(local,offset)<1e-5f,"moving UI stays attached in parent-local coordinates");
  for(int corner=0;corner<8;++corner){XrVector3f point={(corner&1)?.12f:-.12f,(corner&2)?.03f:-.03f,(corner&4)?.06f:-.06f};Check(Gap(PoseRotate(parent,point),PoseRotate(recovered.orientation,point))<1e-5f,"box corners share the corrected parent orientation despite nonuniform basis scale");}
  XrQuaternionf bodyQ={};Check(BodyRotation(body,bodyQ),"scale-free body quaternion");const auto old=PoseMultiply(inverse,bodyQ);if(Gap(PoseRotate(old,offset),intendedOffset)>.01f)++oldFailed;++count;
 }
 float invalid[16]={};XrPosef p={};Check(!BodyToReferencePose(invalid,1,0,reference,p),"degenerate native basis fails closed");Check(oldFailed>300,"previous negative reference-yaw prefix fails the same nonzero-recenter trajectory");std::printf("PASS: %u dynamic body/reference round trips and 5760 box corners; old negative-yaw formula fails %u attachment samples\n",count,oldFailed);return 0;}
