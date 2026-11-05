#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "selector_policy290.h"
namespace fc3select291 {
using P=fc3select290::Point;
struct Vertex {float p[3],n[3];};
inline P Sub(P a,P b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline P Add(P a,P b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline P Mul(P a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(P a,P b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline P Cross(P a,P b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float Segment(P p,P a,P b){const auto d=Sub(b,a);const float l=Dot(d,d);const float t=l>1e-12f?(std::max)(0.f,(std::min)(1.f,Dot(Sub(p,a),d)/l)):0;const auto v=Sub(p,Add(a,Mul(d,t)));return Dot(v,v);}
inline float Triangle(P p,P a,P b,P c){const auto ab=Sub(b,a),ac=Sub(c,a),n=Cross(ab,ac),ap=Sub(p,a);const float nn=Dot(n,n);
 if(nn>1e-15f){const float d=Dot(ap,n);const auto q=Sub(ap,Mul(n,d/nn));const float aa=Dot(ab,ab),bb=Dot(ac,ac),cc=Dot(ab,ac),qa=Dot(q,ab),qb=Dot(q,ac),den=aa*bb-cc*cc;
  if(den>1e-15f){const float s=(qa*bb-qb*cc)/den,t=(qb*aa-qa*cc)/den;if(s>=0&&t>=0&&s+t<=1)return d*d/nn;}}
 return (std::min)(Segment(p,a,b),(std::min)(Segment(p,b,c),Segment(p,c,a)));}
inline float Distance(const std::vector<Vertex>& vertices,const float bounds[6],P p,float radius=.06f){
 const float v[3]={p.x,p.y,p.z};for(int i=0;i<3;++i)if(!std::isfinite(v[i])||v[i]<bounds[i]-radius||v[i]>bounds[i+3]+radius)return INFINITY;
 float nearest=INFINITY;
 for(size_t i=0;i+2<vertices.size();i+=3){const auto* a=vertices[i].p;const auto* b=vertices[i+1].p;const auto* c=vertices[i+2].p;
  nearest=(std::min)(nearest,Triangle(p,{a[0],a[1],a[2]},{b[0],b[1],b[2]},{c[0],c[1],c[2]}));if(nearest<1e-10f)break;}
 return nearest;
}
inline bool Hit(const std::vector<Vertex>& vertices,const float bounds[6],P p,float radius=.06f){return Distance(vertices,bounds,p,radius)<=radius*radius;}
}
