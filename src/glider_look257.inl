// Confirmed native vehicle type2 in the recorded glider run. Keep the older
// vehicle policy for other types until their camera controls are verified.
struct GliderAxis257 {
 float command=0,sign=1;bool holding=false,blocked=false,learned=false;
 ULONGLONG since=0;double start=0;
};
struct GliderLook257 {
 bool active=false,manual=false,vehicleBasis=false;
 unsigned long long pawn=0,vehicle=0;int seat=-1;
 ULONGLONG last=0,manualUntil=0,status=0;
 double body[3][3]={},previous[3][3]={},camera[3][3]={};
 GliderAxis257 yaw,pitch;
}g_gliderLook257;
void OrthonormalGlider257(double (&axes)[3][3]){
 V3 f=Unit({axes[2][0],axes[2][1],axes[2][2]}),r={axes[0][0],axes[0][1],axes[0][2]},u={axes[1][0],axes[1][1],axes[1][2]};
 auto dot=[](V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
 r=Unit(Sub(r,Scale(f,dot(r,f))));u=Unit(Sub(Sub(u,Scale(f,dot(u,f))),Scale(r,dot(u,r))));
 const V3 rows[3]={r,u,f};for(int i=0;i<3;++i){axes[i][0]=rows[i].x;axes[i][1]=rows[i].y;axes[i][2]=rows[i].z;}
}
float GliderAxisDrive257(GliderAxis257& a,double error,double observed,ULONGLONG now){
 if(!std::isfinite(error)||!std::isfinite(observed))return 0;
 if(a.since&&now>=a.since+300){
  const double moved=std::remainder(observed-a.start,6.283185307179586);
  // Both axes use positive command -> positive observed coordinate here.
  if(std::fabs(moved)>.015&&std::fabs(a.command)>.3f){
   // One verified direction per acquisition. Re-learning against each last
   // frame's command caused repeated flips while the vehicle/head was turning.
   if(!a.learned&&moved*a.command<0)a.sign=-a.sign;
   a.learned=true;
   a.since=0;
  }else if(now>=a.since+1500){a.blocked=true;a.since=0;}
 }
 if(a.blocked||std::fabs(error)<(a.holding?.02:.06)){a.holding=false;a.command=0;a.since=0;return 0;}
 a.holding=true;
 const float wanted=float((error<0?-1:1)*(.25+.75*(std::min)(1.,g_vehicleLookGain*std::fabs(error))));
 if(!a.since||wanted*a.command<=0){a.since=now;a.start=observed;}
 a.command=wanted;
 return a.sign*wanted;
}
bool TickGliderLook257(const fc3aim::VehicleSample170& sample,const CameraSummary& camera,ULONGLONG now){
 auto& s=g_gliderLook257;
 if(!sample.identityValid||!sample.seatValid||sample.type!=2||sample.actualSeat!=1)return false;
 const float* raw=g_eyeViewsBody[0].view;
 if(!std::isfinite(raw[8])||!std::isfinite(raw[9])||!std::isfinite(raw[10])||std::hypot(std::hypot(double(raw[8]),double(raw[9])),double(raw[10]))<.5)return false;
 const bool owner=s.active&&s.pawn==sample.pawnId&&s.vehicle==sample.vehicleId&&s.seat==sample.actualSeat;
 if(!owner){s=GliderLook257{};s.active=true;s.pawn=sample.pawnId;s.vehicle=sample.vehicleId;s.seat=sample.actualSeat;}
 const float* observed[3]={camera.right,camera.up,camera.forward};
 const auto controls=fc3pad::Latest();float px=0,ignored=0;fc3pad::LookStick(px,ignored);
 const float py=controls.rightY; // native vehicle look, not the on-foot crouch mapping
 const bool manual=std::fabs(px)>.2f||std::fabs(py)>.2f;
 s.last=now;
 const bool wasManual=s.manual;
 if(manual){s.manual=true;s.manualUntil=now+150;}
 const bool seed=!owner;
 if(seed){
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)s.body[i][j]=observed[i][j];
  s.yaw=GliderAxis257{};s.pitch=GliderAxis257{};
  s.yaw.learned=true; // recorded native type2 mapping: right stick lowers yaw
 }else if(wasManual){
  // Native manual-look deltas turn the existing reference. Keep compensation
  // active while the stick is held, so entering manual look never adds HMD twice.
  double next[3][3]={};
  for(int i=0;i<3;++i)for(int k=0;k<3;++k){double local=0;for(int j=0;j<3;++j)local+=s.camera[k][j]*s.body[i][j];for(int j=0;j<3;++j)next[i][j]+=local*observed[k][j];}
  memcpy(s.body,next,sizeof(next));
 }else if(s.vehicleBasis&&sample.basisValid){
  // Apply only actual vehicle rotation to the saved view reference. No axis
  // needs to be guessed as "forward"; all three copied basis rows participate.
  double next[3][3]={};
  for(int i=0;i<3;++i)for(int k=0;k<3;++k){double local=0;for(int j=0;j<3;++j)local+=s.previous[k][j]*s.body[i][j];for(int j=0;j<3;++j)next[i][j]+=local*sample.basis[k][j];}
  memcpy(s.body,next,sizeof(next));
 }
 // Captured axes are floats. Repeated native basis deltas must not accumulate
 // scale/shear in the visual reference during a long flight.
 OrthonormalGlider257(s.body);
 s.vehicleBasis=sample.basisValid;
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)s.camera[i][j]=observed[i][j];
 if(sample.basisValid)for(int i=0;i<3;++i)for(int j=0;j<3;++j)s.previous[i][j]=sample.basis[i][j];
 const bool manualView=manual||(s.manual&&now<s.manualUntil);
 if(s.manual&&!manualView){s.manual=false;s.yaw=GliderAxis257{};s.pitch=GliderAxis257{};s.yaw.learned=true;Log("[GLIDER257] manual look released; head-follow resumes from the existing view reference");}
 V3 h=Unit({raw[8],raw[9],raw[10]});
 V3 target={};for(int j=0;j<3;++j){const double v=s.body[0][j]*h.x+s.body[1][j]*h.y+s.body[2][j]*h.z;if(j==0)target.x=v;else if(j==1)target.y=v;else target.z=v;}
 const double yaw=std::atan2(camera.forward[1],camera.forward[0]);
 const double pitch=std::atan2(camera.forward[2],std::hypot(camera.forward[0],camera.forward[1]));
 const double desiredYaw=std::atan2(target.y,target.x),desiredPitch=(std::max)(-1.48,(std::min)(1.48,std::atan2(target.z,std::hypot(target.x,target.y))));
 // Existing recorded glider mapping: positive X turns toward decreasing yaw.
 // Measure camera response relative to the vehicle-attached reference, so
 // flight turns/bank do not masquerade as reversed look controls.
 double local[3]={};for(int i=0;i<3;++i)for(int j=0;j<3;++j)local[i]+=observed[2][j]*s.body[i][j];
 const double localYaw=std::atan2(local[0],local[2]),localPitch=std::atan2(local[1],std::hypot(local[0],local[2]));
 const float x=manualView?(manual?px:0):GliderAxisDrive257(s.yaw,Wrap102(yaw-desiredYaw),localYaw,now);
 const float y=manualView?(manual?py:0):GliderAxisDrive257(s.pitch,desiredPitch-pitch,localPitch,now);
 fc3pad::SetLookOverride(true,x,y);
 for(int i=0;i<3;++i)for(int j=0;j<3;++j){g_bodyAxes[i][j]=s.body[i][j];g_aimQ[i][j]=0;for(int k=0;k<3;++k)g_aimQ[i][j]+=observed[i][k]*s.body[j][k];}
 g_bodyYaw=std::atan2(s.body[2][1],s.body[2][0]);g_bodyValid=false;g_aimActive=true;
 if(now-s.status>=1000){s.status=now;Log("[GLIDER257] frame%ld target %.4f/%.4f observed %.4f/%.4f stick %.3f/%.3f blocked %d/%d",g_frame,desiredYaw,desiredPitch,yaw,pitch,x,y,s.yaw.blocked?1:0,s.pitch.blocked?1:0);}
 return true;
}
