#pragma once
#include <cmath>
#include <cstdint>
namespace fc3pickup292 {
struct Pickup {
 int slot=-1;float origin[3]={};std::uint64_t at=0;bool requested=false;
 void Cancel(){*this=Pickup{};}
 void Begin(int s,float x,float y,float z,std::uint64_t now){Cancel();slot=s;origin[0]=x;origin[1]=y;origin[2]=z;at=now;}
 bool Pulled(float x,float y,float z,std::uint64_t now)const{
  const float dx=x-origin[0],dy=y-origin[1],dz=z-origin[2];
  return slot>=0&&!requested&&now>=at+150&&std::isfinite(dx)&&std::isfinite(dy)&&std::isfinite(dz)&&dx*dx+dy*dy+dz*dz>=.08f*.08f;
 }
};
inline int Handle(const float* positions,const float* palm,const bool* occupied,int previous=-1){
 float distance=.055f*.055f;int result=-1;
 for(int i=0;i<4;++i)if(occupied[i]){float sum=0;for(int j=0;j<3;++j){const float d=palm[j]-positions[i*3+j];sum+=d*d;}const float limit=i==previous?.07f*.07f:.055f*.055f;if(std::isfinite(sum)&&sum<=limit){if(result<0||sum<distance){distance=sum;result=i;}}}
 return result;
}
}
