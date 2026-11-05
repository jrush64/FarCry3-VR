#pragma once
#include <openxr/openxr.h>
#include <cmath>
namespace fc3xr {
inline XrQuaternionf PoseMultiply(XrQuaternionf a,XrQuaternionf b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline XrVector3f PoseRotate(XrQuaternionf q,XrVector3f v){const XrQuaternionf p=PoseMultiply(PoseMultiply(q,{v.x,v.y,v.z,0}),{-q.x,-q.y,-q.z,q.w});return {p.x,p.y,p.z};}
inline bool BodyRotation(const float* m,XrQuaternionf& q)
{
 if(!m)return false;float c[3][3];for(int col=0;col<3;++col)for(int row=0;row<3;++row){const float v=m[row*4+col];if(!std::isfinite(v))return false;c[col][row]=v*((col==2?-1.f:1.f)*(row==2?-1.f:1.f));}
 const float n0=std::sqrt(c[0][0]*c[0][0]+c[0][1]*c[0][1]+c[0][2]*c[0][2]);if(n0<1e-6f)return false;for(int r=0;r<3;++r)c[0][r]/=n0;
 const float dot=c[0][0]*c[1][0]+c[0][1]*c[1][1]+c[0][2]*c[1][2];for(int r=0;r<3;++r)c[1][r]-=dot*c[0][r];const float n1=std::sqrt(c[1][0]*c[1][0]+c[1][1]*c[1][1]+c[1][2]*c[1][2]);if(n1<1e-6f)return false;for(int r=0;r<3;++r)c[1][r]/=n1;
 float z[3]={c[0][1]*c[1][2]-c[0][2]*c[1][1],c[0][2]*c[1][0]-c[0][0]*c[1][2],c[0][0]*c[1][1]-c[0][1]*c[1][0]};if(z[0]*c[2][0]+z[1]*c[2][1]+z[2]*c[2][2]<=1e-6f)return false;for(int r=0;r<3;++r)c[2][r]=z[r];float a[9];for(int r=0;r<3;++r)for(int col=0;col<3;++col)a[r*3+col]=c[col][r];
 const float trace=a[0]+a[4]+a[8];if(trace>0){const float s=std::sqrt(trace+1)*2;q={(a[7]-a[5])/s,(a[2]-a[6])/s,(a[3]-a[1])/s,s*.25f};}else{int i=a[4]>a[0]?1:0;if(a[8]>a[i*3+i])i=2;const int j=(i+1)%3,k=(i+2)%3;const float s=std::sqrt(1+a[i*3+i]-a[j*3+j]-a[k*3+k])*2;if(s<1e-6f)return false;float v[3]={};v[i]=s*.25f;v[j]=(a[j*3+i]+a[i*3+j])/s;v[k]=(a[k*3+i]+a[i*3+k])/s;q={v[0],v[1],v[2],(a[k*3+j]-a[j*3+k])/s};}const float n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);if(!std::isfinite(n)||n<1e-6f)return false;q={q.x/n,q.y/n,q.z/n,q.w/n};return true;
}
inline bool BodyToReferencePose(const float* body,float scale,float yaw,XrVector3f reference,XrPosef& out)
{
 if(!body||!std::isfinite(scale)||scale<=0||!std::isfinite(yaw)||!std::isfinite(reference.x)||!std::isfinite(reference.y)||!std::isfinite(reference.z))return false;if(!std::isfinite(body[3])||!std::isfinite(body[7])||!std::isfinite(body[11]))return false;XrQuaternionf rotation;if(!BodyRotation(body,rotation))return false;const XrQuaternionf inverseReference={0,std::sin(yaw*.5f),0,std::cos(yaw*.5f)};const auto p=PoseRotate(inverseReference,{body[3]/scale,body[7]/scale,-body[11]/scale});out.position={reference.x+p.x,reference.y+p.y,reference.z+p.z};out.orientation=PoseMultiply(inverseReference,rotation);return true;
}
inline bool BodyBoxToReferencePose(const float* body,float scale,float yaw,XrVector3f reference,XrPosef& out)
{
 if(!body)return false;float box[16];for(int i=0;i<16;++i)box[i]=body[i];
 const double d=double(box[0])*(double(box[5])*box[10]-double(box[6])*box[9])-double(box[1])*(double(box[4])*box[10]-double(box[6])*box[8])+double(box[2])*(double(box[4])*box[9]-double(box[5])*box[8]);
 // A centered box is unchanged by reversing a local axis. Native mirrored
 // weapon bases therefore have an exact proper-rotation representation.
 // This rule is box-only; asymmetric hand/UI transforms remain strict.
 if(d<0){box[0]*=-1;box[4]*=-1;box[8]*=-1;}
 return BodyToReferencePose(box,scale,yaw,reference,out);
}
}
