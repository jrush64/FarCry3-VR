#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace fc3contact292 {
struct P {float x=0,y=0,z=0;};
inline P Add(P a,P b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline P Sub(P a,P b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline P Mul(P a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float Dot(P a,P b){return a.x*b.x+a.y*b.y+a.z*b.z;}
struct Triangle {P a,b,c;};
inline P Closest(P p,const Triangle& t){
 const P ab=Sub(t.b,t.a),ac=Sub(t.c,t.a),ap=Sub(p,t.a);const float d1=Dot(ab,ap),d2=Dot(ac,ap);if(d1<=0&&d2<=0)return t.a;
 const P bp=Sub(p,t.b);const float d3=Dot(ab,bp),d4=Dot(ac,bp);if(d3>=0&&d4<=d3)return t.b;
 const float v=d1*d4-d3*d2;if(v<=0&&d1>=0&&d3<=0)return Add(t.a,Mul(ab,d1/(d1-d3)));
 const P cp=Sub(p,t.c);const float d5=Dot(ab,cp),d6=Dot(ac,cp);if(d6>=0&&d5<=d6)return t.c;
 const float w=d5*d2-d1*d6;if(w<=0&&d2>=0&&d6<=0)return Add(t.a,Mul(ac,d2/(d2-d6)));
 const float u=d3*d6-d5*d4;if(u<=0&&d4-d3>=0&&d5-d6>=0)return Add(t.b,Mul(Sub(t.c,t.b),(d4-d3)/(d4-d3+d5-d6)));
 const float sum=u+v+w;if(std::fabs(sum)<1e-12f)return t.a;return Add(t.a,Add(Mul(ab,w/sum),Mul(ac,v/sum)));
}
struct Node {float low[3],high[3];unsigned start=0,count=0,left=0,right=0;};
struct Collider {
 std::vector<Triangle> triangles;std::vector<unsigned> order;std::vector<Node> nodes;
 unsigned BuildNode(unsigned start,unsigned count){
  const unsigned id=unsigned(nodes.size());nodes.push_back({});Node node={};node.start=start;node.count=count;
  for(int k=0;k<3;++k){node.low[k]=INFINITY;node.high[k]=-INFINITY;}
  for(unsigned i=start;i<start+count;++i){const auto& t=triangles[order[i]];const P vertices[]={t.a,t.b,t.c};for(const auto& p:vertices){const float a[]={p.x,p.y,p.z};for(int k=0;k<3;++k){node.low[k]=(std::min)(node.low[k],a[k]);node.high[k]=(std::max)(node.high[k],a[k]);}}}
  if(count>8){int axis=0;for(int i=1;i<3;++i)if(node.high[i]-node.low[i]>node.high[axis]-node.low[axis])axis=i;
   const auto centre=[&](unsigned i){const auto& t=triangles[i];const P p=Add(Add(t.a,t.b),t.c);return axis==0?p.x:axis==1?p.y:p.z;};const unsigned middle=start+count/2;
   std::nth_element(order.begin()+start,order.begin()+middle,order.begin()+start+count,[&](unsigned a,unsigned b){return centre(a)<centre(b);});node.count=0;node.left=BuildNode(start,count/2);node.right=BuildNode(middle,count-count/2);
  }nodes[id]=node;return id;
 }
 void Build(){order.resize(triangles.size());for(unsigned i=0;i<order.size();++i)order[i]=i;nodes.clear();if(!order.empty())BuildNode(0,unsigned(order.size()));}
 void Search(unsigned id,P p,float& distance,P& nearest)const{
  const auto& node=nodes[id];const float a[]={p.x,p.y,p.z};float box=0;for(int k=0;k<3;++k){const float d=a[k]<node.low[k]?node.low[k]-a[k]:a[k]>node.high[k]?a[k]-node.high[k]:0;box+=d*d;}if(box>distance)return;
  if(node.count){for(unsigned i=node.start;i<node.start+node.count;++i){const auto q=Closest(p,triangles[order[i]]),d=Sub(p,q);const float n=Dot(d,d);if(n<=distance){distance=n;nearest=q;}}}else{Search(node.left,p,distance,nearest);Search(node.right,p,distance,nearest);}
 }
 bool Nearest(P p,float radius,P& nearest,float& squared)const{
  if(nodes.empty()||!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return false;
  squared=radius*radius;nearest={INFINITY,0,0};Search(0,p,squared,nearest);return std::isfinite(nearest.x);
 }
 // Sweep from the last displayed palm. Retaining the resolved palm prevents
 // the virtual hand jumping through a thin surface when the real hand crosses.
 bool Sweep(P previous,P wanted,float radius,P& resolved)const{
  const auto delta=Sub(wanted,previous);const float length=std::sqrt(Dot(delta,delta));const int steps=(std::max)(1,(std::min)(128,int(std::ceil(length/(radius*.5f)))));
  P prior=previous;
  for(int i=1;i<=steps;++i){const P point=Add(previous,Mul(delta,float(i)/steps));P surface;float squared=0;
   if(Nearest(point,radius,surface,squared)){
    auto normal=Sub(prior,surface);float n=std::sqrt(Dot(normal,normal));if(n<1e-6f){normal=Sub(point,surface);n=std::sqrt(Dot(normal,normal));}if(n<1e-6f){resolved=prior;return true;}
    resolved=Add(surface,Mul(normal,(radius+.001f)/n));return true;
   }prior=point;
  }resolved=wanted;return false;
 }
};
}
