Mat4 HandIdentity();
// Coordinates are body metres; profile values are independent of world scale.
CalibrationProfile136& CalibrationCurrent136(){return g_calibPreview136?g_calibDraft136:g_calibProfiles136[g_calibSlot136];}
bool CalibrationActive136(){return g_calibPreview136||(g_calibEnabled136&&g_calibProfiles136[g_calibSlot136].valid);}
float g_calibRaw136[2][16]={};bool g_calibRawValid136[2]={};
// Body calibration observes raw tracking; it never rotates/translates controller or weapon poses.
void CalibrationTrack138(const float* right,const float* left){
 const float* raw[2]={right,left};for(int side=0;side<2;++side){g_calibRawValid136[side]=raw[side]!=nullptr;if(!raw[side])continue;for(int i=0;i<16;++i)if(!std::isfinite(raw[side][i]))g_calibRawValid136[side]=false;memcpy(g_calibRaw136[side],raw[side],64);}
}
bool CalibrationHead136(V3& position,V3* forward=nullptr){if(!g_eyeViewsSet)return false;position={};V3 facing={};for(int eye=0;eye<2;++eye){Mat4 v={},inverse={};const auto& e=g_aimActive?g_eyeViewsBody[eye]:g_eyeViews[eye];for(int i=0;i<16;++i)v.m[i]=e.view[i];if(!Invert(v,inverse))return false;position=Add(position,Scale(Apply(inverse,{}),.5));facing=Add(facing,{inverse.m[2],inverse.m[6],inverse.m[10]});}if(forward)*forward=Unit(facing);return true;}
bool CalibrationShoulder136(int side,V3& out){if(!CalibrationActive136())return false;V3 head={};if(!CalibrationHead136(head))return false;const auto& p=CalibrationCurrent136();const double scale=(std::max)(.1f,g_worldScale);const double x=(side?-1:1)*p.shoulder[0]*scale,z=p.shoulder[2]*scale;out=Add(head,{cos(p.yaw)*x+sin(p.yaw)*z,p.shoulder[1]*scale,-sin(p.yaw)*x+cos(p.yaw)*z});return true;}
double CalibrationUpperScale136(double upper,double forearm,int side){const auto& p=CalibrationCurrent136();const double wanted=(p.reach+(p.forwardFit?p.reachDelta[side]:0))*(std::max)(.1f,g_worldScale);const double minimum=p.forwardFit?.2:1.;return (std::max)(minimum,(std::min)(double(p.upperLimit),(wanted-forearm)/(std::max)(.001,upper)));}
void PauseCalibration136();
// Equal shoulder-to-wrist radii in two orthogonal fully extended poses.
// Coordinates are relative to the headset in the captured forward-facing body frame.
bool CalibrationFit139(const V3* sideways,const V3* front,CalibrationProfile136& out){
 for(int side=0;side<2;++side)for(const V3 v:{sideways[side],front[side]})if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))return false;
 const double half=(front[0].x-front[1].x)*.5;
 const double sy=(sideways[0].y+sideways[1].y)*.5;
 if(half<.12||half>.34||fabs((front[0].x+front[1].x)*.5)>.12)return false;
 double depth=0;
 for(int side=0;side<2;++side){
  const V3 t=sideways[side],f=front[side];const double sx=side?-half:half,dz=f.z-t.z;
  if(dz<.20||fabs(f.y-sy)>.15||fabs(t.y-sy)>.10)return false;
  depth+=((f.x-sx)*(f.x-sx)-(t.x-sx)*(t.x-sx)+(f.y-sy)*(f.y-sy)-(t.y-sy)*(t.y-sy)+f.z*f.z-t.z*t.z)/(4*dz);
 }
 if(depth<-.45||depth>.20||sy<-.55||sy>-.06)return false;
 double reach[2]={};
 for(int side=0;side<2;++side){const V3 shoulder={side?-half:half,sy,depth};const double a=Length(Sub(sideways[side],shoulder)),b=Length(Sub(front[side],shoulder));if(fabs(a-b)>.06)return false;reach[side]=(a+b)*.5;}
 if(fabs(reach[0]-reach[1])>.08)return false;
 const double length=(reach[0]+reach[1])*.5;if(length<.32||length>.85)return false;
 out.shoulder[0]=float(half);out.shoulder[1]=float(sy);out.shoulder[2]=float(depth);out.reach=float(length);for(int side=0;side<2;++side)out.reachDelta[side]=float(reach[side]-length);out.forwardFit=true;out.valid=true;return true;
}
