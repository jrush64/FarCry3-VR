// The native camera is a visibility camera, not the physical weapon ray.
V3 g_reticleAimBody73={};
LONG g_reticleAimFrame73=-1000;
UINT g_reticleAimEpoch73=0;
void PublishReticleAim73(V3 direction){
 if(!std::isfinite(direction.x)||!std::isfinite(direction.y)||!std::isfinite(direction.z)||Length(direction)<1e-6){g_reticleAimFrame73=-1000;return;}
 g_reticleAimBody73=Unit(direction);g_reticleAimFrame73=g_frame+1;g_reticleAimEpoch73=g_armPoseEpoch;
}
bool BarrelBody206(V3&);
#ifdef FC3_AMMO_TEST
const fc3aim::LaserSample194* g_reticleFixture212=nullptr;
#endif
// A muzzle-origin shot and a wrist-origin crosshair are parallel rays, so the
// crosshair sits below the impact when the muzzle is above the wrist. Project
// the current drawn muzzle and barrel ray into the eye used for the crosshair.
bool ReticleMuzzleTarget212(int eye,V3& out){
 if(eye<0||eye>1)return false;
 const auto& f=g_muzzleLaserFrame203;
 if(!f.valid||f.frame>g_frame||g_frame-f.frame>1||f.epoch!=g_armPoseEpoch||!f.barrelValid||
    BarrelFamily206(int(f.profile))!=BarrelFamily206(int(g_weaponMechanicProfile)))return false;
 fc3aim::LaserSample194 sample={};bool have=fc3aim::ReadLaser194(sample);
#ifdef FC3_AMMO_TEST
 if(g_reticleFixture212){sample=*g_reticleFixture212;have=true;}
#endif
 if(!have||!sample.localValid||sample.epoch!=f.epoch||
    BarrelFamily206(int(sample.profile))!=BarrelFamily206(int(f.profile)))return false;
 const V3 muzzle=Apply(Multiply(f.move,f.native),{sample.muzzleLocal[0],sample.muzzleLocal[1],sample.muzzleLocal[2]});
 const V3 ray=Unit(f.barrelWorld);
 if(!std::isfinite(muzzle.x)||!std::isfinite(muzzle.y)||!std::isfinite(muzzle.z)||
    !std::isfinite(ray.x)||!std::isfinite(ray.y)||!std::isfinite(ray.z)||Length(ray)<.5)return false;
 const double distance=sample.hit&&std::isfinite(sample.distance)&&sample.distance>1?sample.distance:15.;
 Mat4 xrView={};for(int i=0;i<16;++i)xrView.m[i]=g_eyeViews[eye].view[i];
 // The confirmed draw supplies the game WORLD-to-camera view. The caller's
 // ReticleTarget73 view is already the XR eye view and must not be used here.
 out=Apply(xrView,Apply(f.view,Add(muzzle,Scale(ray,distance))));
 return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z)&&out.z>.1;
}
bool ReticleRoot345(Mat4& root,Mat4& grip){return fc3hands303::Shotgun318(g_weaponMechanicProfile)?ShotgunRoot318(root):ControllerCurrentRoot124(root,grip);}
bool ReticleTarget73(int eye,const Mat4& nativeView,V3& out){
 Mat4 view=nativeView;V3 point={0,0,15};
 if(g_aimActive&&MotionInteractionEnabled()){
  if(!g_handRightValid)return false;
  V3 direction=g_reticleAimBody73;V3 barrel206={};const bool haveBarrel206=BarrelBody206(barrel206);
  if(haveBarrel206)direction=barrel206; // 206: the same ray the shot and laser use
  else if(g_reticleAimEpoch73!=g_armPoseEpoch||std::abs(g_frame-g_reticleAimFrame73)>1){
   // A native-camera aim binding gap must not blank a standard firearm dot.
   // Use only the freshly observed current weapon's controller-owned root.
   Mat4 root={},grip={};if((!fc3hands303::MagazineSidearm315(g_weaponMechanicProfile)&&g_weaponMechanicProfile!=42&&!fc3hands303::MagazineRifle317(g_weaponMechanicProfile)&&!fc3hands303::OtherMagazine319(g_weaponMechanicProfile)&&!fc3hands303::Shotgun318(g_weaponMechanicProfile))||g_weaponMechanicFrame>g_frame||g_frame-g_weaponMechanicFrame>2||g_reload.held||g_reload.pending||g_pistolReload.pending||g_pistolReload.removalHeld||!fc3pad::Latest().active||!ReticleRoot345(root,grip))return false;
   direction=ScopeAimValid72()?g_scopeAim72.filtered:Unit({root.m[1],root.m[5],root.m[9]});if(Length(direction)<.5)return false;
  }
  if(haveBarrel206&&ReticleMuzzleTarget212(eye,out))return true;
  // Use the uncompensated body eye view exactly once. Head rotation changes
  // the projection of this ray, but never changes the ray itself.
  for(int i=0;i<16;++i)view.m[i]=g_eyeViewsBody[eye].view[i];
  const V3 origin129=ScopeAimValid72()?g_scopeAim72.pivot:ControllerWrist(g_handBody[0],0);
  // 206: the crosshair sits at the real impact distance (first scene hit along the barrel), so near targets
  // no longer show the parallax of a fixed 15 m point. Clear ray or no sample: 15 m as before.
  double range206=15;fc3aim::LaserSample194 sample206={};
  if(haveBarrel206&&fc3aim::ReadLaser194(sample206)&&sample206.hit&&sample206.epoch==g_armPoseEpoch&&std::isfinite(sample206.distance)&&sample206.distance>1)range206=sample206.distance;
  point=Add(origin129,Scale(direction,range206));
 }
 out=Apply(view,point);
 return std::isfinite(out.x)&&std::isfinite(out.y)&&std::isfinite(out.z)&&out.z>.1;
}
