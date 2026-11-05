// Keep the rendered head frustum inside the native scene camera when possible.
// This changes camera direction only; physical controller/contact poses stay raw.
double CameraAimCone67(){
 double cone=35.0*3.14159265358979/180;
 if(!g_eyeViewsSet||g_worldProjection.xs<=0||g_worldProjection.ys<=0)return cone;
 double needX=0,needY=0;for(const auto& e:g_eyeViews){needX=(std::max)(needX,(std::max)(fabs(std::atan(double(e.tanLeft))),fabs(std::atan(double(e.tanRight)))));needY=(std::max)(needY,(std::max)(fabs(std::atan(double(e.tanDown))),fabs(std::atan(double(e.tanUp)))));}
 const double margin=3.0*3.14159265358979/180;
 return (std::max)(0.0,(std::min)(cone,(std::min)(std::atan(1.0/g_worldProjection.xs)-needX-margin,std::atan(1.0/g_worldProjection.ys)-needY-margin)));
}
V3 CameraConeDirection67(V3 head,V3 gun,double cone){
 if(Length(head)<1e-8||Length(gun)<1e-8)return gun;head=Unit(head);gun=Unit(gun);cone=(std::max)(0.0,(std::min)(cone,3.14159265358979));
 const double dot=(std::max)(-1.0,(std::min)(1.0,head.x*gun.x+head.y*gun.y+head.z*gun.z));
 if(dot>=std::cos(cone))return gun;
 V3 tangent=Sub(gun,Scale(head,dot));
 // Antipodal slerp divides by sin(pi). Choose a finite orthogonal direction.
 if(Length(tangent)<1e-8){const V3 axis=fabs(head.x)<.8?V3{1,0,0}:V3{0,1,0};tangent=Sub(axis,Scale(head,axis.x*head.x+axis.y*head.y+axis.z*head.z));}
 return Unit(Add(Scale(head,std::cos(cone)),Scale(Unit(tangent),std::sin(cone))));
}
