// Hidden-weapon visibility has its own camera purpose. Never emit gamepad axes.
bool g_visibilityCamera68=false,g_visibilityHandoff68=false;double g_visibilityBodyYaw68=0;
void VisibilityBasisMatrix68(double yaw,double pitch,double roll,double rightSign,double (&axes)[3][3]){
 V3 f={std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),std::sin(pitch)};
 V3 r={rightSign*std::sin(yaw),-rightSign*std::cos(yaw),0};
 V3 u={-std::sin(pitch)*std::cos(yaw),-std::sin(pitch)*std::sin(yaw),std::cos(pitch)};
 const V3 rr=Add(Scale(r,std::cos(roll)),Scale(u,std::sin(roll))),uu=Sub(Scale(u,std::cos(roll)),Scale(r,std::sin(roll)));
 const V3 rows[3]={rr,uu,f};for(int i=0;i<3;++i){axes[i][0]=rows[i].x;axes[i][1]=rows[i].y;axes[i][2]=rows[i].z;}
}
bool VisibilityHeadOffsets68(V3 head,const double (&axes)[3][3],double yaw,double pitch,double& offsetYaw,double& offsetPitch){
 if(Length(head)<1e-6)return false;head=Unit(head);V3 world={axes[0][0]*head.x+axes[1][0]*head.y+axes[2][0]*head.z,axes[0][1]*head.x+axes[1][1]*head.y+axes[2][1]*head.z,axes[0][2]*head.x+axes[1][2]*head.y+axes[2][2]*head.z};
 const double horizontal=std::hypot(world.x,world.y);const double targetYaw=horizontal<1e-8?yaw:std::atan2(world.y,world.x);
 offsetYaw=std::remainder(targetYaw-yaw,6.283185307179586);offsetPitch=std::atan2(world.z,horizontal)-pitch;return std::isfinite(offsetYaw)&&std::isfinite(offsetPitch);
}
double VisibilityCameraSign68(const float* right,const float* up,const float* forward){
 const V3 cross={double(up[1])*forward[2]-double(up[2])*forward[1],double(up[2])*forward[0]-double(up[0])*forward[2],double(up[0])*forward[1]-double(up[1])*forward[0]};
 return right[0]*cross.x+right[1]*cross.y+right[2]*cross.z<0?1:-1;
}
// Vehicle camera (candidate 102): the rendered camera ignores the native player look. Set once,
// kept until a weapon is visible or there has been no renderable camera for 15 s (run 67572: a
// frame without a camera used to clear it, so the look lane re-engaged and froze the view six
// times in one car ride). vehicle_look102.inl takes the camera over while it is set.
bool g_vehicleCamera92=false;ULONGLONG g_vehicleCameraLostAt92=0;
void ReleaseHiddenCameraBasis127(bool confirmed)
{
 if(confirmed&&!MotionInteractionEnabled()){
  g_visibilityHandoff68=false;g_bodyValid=g_aimActive=false;
 }
}
bool TickVisibilityCamera68(bool headset){
 const bool renderable=headset&&g_headTracking&&g_enabled&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive;
 const bool eligible=renderable&&!MotionInteractionEnabled()&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&!fc3pad::WeaponSelectionRequested();
 // A vehicle/cinematic can keep its own render camera still while the local
 // player's look setter accepts HMD yaw. In that case V*Q^T cancels the only
 // working head motion and the whole image appears glued to the headset. Learn
 // this strictly from a large observed turn, then retain raw HMD tracking until
 // a visible weapon regains camera ownership. No vehicle input is synthesized.
 bool& detachedCamera92=g_vehicleCamera92;static ULONGLONG responseAt92=0;
 static double responseHead92=0,responseCamera92=0;
 const ULONGLONG nowTick92=GetTickCount64();
 if(renderable)g_vehicleCameraLostAt92=0;else if(!g_vehicleCameraLostAt92)g_vehicleCameraLostAt92=nowTick92;
 if(MotionInteractionEnabled()||g_visibleWeaponDrawsLastFrame>0||(g_vehicleCameraLostAt92&&nowTick92-g_vehicleCameraLostAt92>15000)){
  if(detachedCamera92)Log("[VISIBILITY92] vehicle camera released (%s)",g_vehicleCameraLostAt92?"no camera for 15 s":"weapon visible");
  detachedCamera92=false;
 }
 // Candidate 106: the engine says whether the player sits in a vehicle. When it answers, it
 // decides, at once and both ways (no head turn, no glued view at entry); the camera tests
 // below remain only for an engine that does not answer.
 bool engineVehicle=false;int engineSeat=-1;static int lastEngineState92=-1;
 const bool engineKnown=fc3aim::VehicleState(engineVehicle,engineSeat);
 if(engineKnown){
  const int state=engineVehicle?1:0;
  if(state!=lastEngineState92){Log(engineVehicle?"[VEHICLE STATE] in a vehicle (seat %d): vehicle camera":"[VEHICLE STATE] on foot (seat %d)",engineSeat);lastEngineState92=state;}
  detachedCamera92=engineVehicle&&!MotionInteractionEnabled()&&g_visibleWeaponDrawsLastFrame==0;
 }else lastEngineState92=-1;
 if(!eligible)responseAt92=0;
 // (Candidates 104/105 tried an offset-to-native-look check; in a vehicle the game rewrites the look
 // to the vehicle direction every tick, so it could not see a vehicle. Replaced by the engine state.)
 if(eligible&&!engineKnown){
  const float* raw=g_eyeViewsBody[0].view;
  const double headYaw=std::atan2(double(raw[8]),double(raw[10]));
  const double cameraYaw=std::atan2(double(g_frameCamera.forward[1]),double(g_frameCamera.forward[0]));
  const ULONGLONG now=GetTickCount64();
  if(std::isfinite(headYaw)&&std::isfinite(cameraYaw)){
   if(!responseAt92||!g_visibilityCamera68){responseAt92=g_visibilityCamera68?now:0;responseHead92=headYaw;responseCamera92=cameraYaw;}
   else if(now-responseAt92>=300){ // candidate 105: 0.25 rad in 300 ms (on foot the camera follows within 0.03 rad)
    const double headTurn=std::fabs(std::remainder(headYaw-responseHead92,6.283185307179586));
    const double cameraTurn=std::fabs(std::remainder(cameraYaw-responseCamera92,6.283185307179586));
    if(!detachedCamera92&&headTurn>0.25&&cameraTurn<headTurn*.25){
     detachedCamera92=true;
     Log("[VISIBILITY92] rendered camera did not follow %.2f rad headset turn (camera %.2f): raw HMD tracking until weapon camera returns",headTurn,cameraTurn);
    }
    responseAt92=now;responseHead92=headYaw;responseCamera92=cameraYaw;
   }
  }
 }
 if(detachedCamera92&&eligible){
  if(g_visibilityCamera68)fc3aim::EndVisibility68();
  g_visibilityCamera68=g_visibilityHandoff68=false;g_bodyValid=g_aimActive=false;
  return false;
 }
 double yaw=0,pitch=0,roll=0;
 if(!renderable||(!eligible&&!g_visibilityCamera68)||!fc3aim::VisibilityBasis68(yaw,pitch,roll)){
  if(g_visibilityCamera68)fc3aim::EndVisibility68();g_visibilityCamera68=false;
  // Keep ordinary controller/weapon dropouts stable; release only a proved
  // native/draw camera conflict, not mere missing input or an unbound getter.
  ReleaseHiddenCameraBasis127(fc3aim::HiddenCameraBlocked127());
  return false;
 }
 // Derive handedness from the observed draw, rather than guessing world axes.
 const auto& camera=g_frameCamera;const double sign=VisibilityCameraSign68(camera.right,camera.up,camera.forward);
 V3 head={};for(const auto& eye:g_eyeViewsBody){head.x+=eye.view[8];head.y+=eye.view[9];head.z+=eye.view[10];}
 if(Length(head)<1e-6)return false;double headYaw=0,headPitch=0;
 g_visibilityBodyYaw68=yaw;g_bodyYaw=yaw;g_rightSign=sign;
 const double observedYaw=std::atan2(camera.forward[1],camera.forward[0]),observedPitch=std::asin((std::max)(-1.f,(std::min)(1.f,camera.forward[2])));
 const V3 nominalRight={sign*std::sin(observedYaw),-sign*std::cos(observedYaw),0},nominalUp={-std::sin(observedPitch)*std::cos(observedYaw),-std::sin(observedPitch)*std::sin(observedYaw),std::cos(observedPitch)};
 roll=std::atan2(camera.right[0]*nominalUp.x+camera.right[1]*nominalUp.y+camera.right[2]*nominalUp.z,camera.right[0]*nominalRight.x+camera.right[1]*nominalRight.y+camera.right[2]*nominalRight.z);
 VisibilityBasisMatrix68(yaw,pitch,roll,sign,g_bodyAxes);
 if(!VisibilityHeadOffsets68(head,g_bodyAxes,yaw,pitch,headYaw,headPitch))return false;
 if(eligible){if(!fc3aim::DriveVisibility68(headYaw,headPitch))return false;}else fc3aim::EndVisibility68(); // pause retains visual compensation only
 // V*Q^T cancels ONLY the native camera rotation, using observed draw axes.
 // V still contains the actual current HMD orientation, once, with full stereo.
 const float* observed[3]={camera.right,camera.up,camera.forward};
 for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_aimQ[i][j]=observed[i][0]*g_bodyAxes[j][0]+observed[i][1]*g_bodyAxes[j][1]+observed[i][2]*g_bodyAxes[j][2];
 if(!g_visibilityCamera68)Log("[VISIBILITY68] hidden-weapon native head camera acquired; native stick axes retained; observed draw compensation enabled");
 g_visibilityCamera68=g_visibilityHandoff68=true;g_bodyValid=false;g_aimActive=true;
 static ULONGLONG next=0;if(GetTickCount64()>=next){next=GetTickCount64()+500;Log("[VISIBILITY68] frame%ld body %.5f/%.5f headOffset %.5f/%.5f observed %.5f/%.5f epoch%u",g_frame,yaw,pitch,headYaw,headPitch,std::atan2(camera.forward[1],camera.forward[0]),std::asin((std::max)(-1.f,(std::min)(1.f,camera.forward[2]))),g_armPoseEpoch);}
 return true;
}
