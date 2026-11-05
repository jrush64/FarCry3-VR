#pragma once
#include <cmath>
namespace fc3grab295 {
constexpr float Radius=.10f,ExitRadius=.12f,HalfLength=.04f;
// Two endpoints per weapon, in the same reference space as the displayed palm.
inline int Handle(const float* endpoints,const float* palm,const bool* occupied,int previous=-1,float enterRadius=Radius,float leaveRadius=ExitRadius){
 if(!endpoints||!palm||!occupied)return -1;
 for(int k=0;k<3;++k)if(!std::isfinite(palm[k]))return -1;
 int best=-1;float nearest=INFINITY;
 for(int i=0;i<4;++i)if(occupied[i]){const float* a=endpoints+i*6;const float* b=a+3;float length=0,dot=0;
  for(int k=0;k<3;++k){const float d=b[k]-a[k];length+=d*d;dot+=(palm[k]-a[k])*d;}
  float t=length>1e-10f?dot/length:0;t=t<0?0:t>1?1:t;float squared=0;
  for(int k=0;k<3;++k){const float d=palm[k]-(a[k]+t*(b[k]-a[k]));squared+=d*d;}
  const float radius=i==previous?leaveRadius:enterRadius;
  if(std::isfinite(squared)&&squared<=radius*radius&&(squared<nearest||(squared==nearest&&i==previous))){nearest=squared;best=i;}
 }return best;
}
}
