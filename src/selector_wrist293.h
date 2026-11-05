#pragma once
#include <cmath>
namespace fc3wrist293 {
// Native bind-local wrist coordinates are right handed, while the arm body's
// right/up/forward coordinates are left handed. A pose quaternion cannot carry
// that reflection. Change the LOCAL frame explicitly; preserve the wrist point.
// Applying this twice also converts contact targets back to native skin space.
inline bool ChangeLocalFrame(const float* input,float* output){
 if(!input||!output)return false;
 for(int i=0;i<16;++i){if(!std::isfinite(input[i]))return false;}
 for(int i=0;i<16;++i)output[i]=input[i]*((i==2||i==6||i==10)?-1.f:1.f);
 return true;
}
}
