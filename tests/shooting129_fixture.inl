// Runs after the accepted GPU grip fixture.
void TestShooting129(ID3D11Device* device,ID3D11DeviceContext* context){
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=true;g_weaponMotionGuard=false;
 g_machete.held=false;g_pistolMenu.open=false;g_reload={};g_reloadAlignment=false;g_r700={};g_lever={};
 fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 g_scopeEyes60[0]={};g_scopeEyes60[1]={};g_weaponSteady129=.65f;
 for(int profile:{12,20,8,7,41,28,14,9,31,25,21,17,35,2,36,22,23,43,34,42,16,32,18,15,3})for(bool two:{false,true}){
  g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_reload.shotgunFrame=g_frame;g_twoHand=two;g_pistolReload={};g_pistolReload.enabled=true;g_pistolReload.magazinePresent=true;
  Mat4 root={},grip={};Check(ControllerCurrentRoot124(root,grip),"129 current controller root");
  V3 raw={};Check(FirearmAxis129(raw)&&Length(Sub(raw,Unit({root.m[1],root.m[5],root.m[9]})))<1e-7,"129 shot basis equals displayed fixed barrel across25 profiles and both hand modes");
  if(profile==12){
   g_aimActive=true;for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_bodyAxes[i][j]=i==j?1.:0.;
   PublishReticleAim73(Unit(Add(raw,{.2,.1,0})));g_shotAimWorld107={0,0,-1};g_shotAimProfile129=-1;
   ReconcileGripAim193(!two);
   Check(Length(Sub(g_reticleAimBody73,raw))<1e-6&&Length(Sub(g_shotAimWorld107,raw))<1e-6&&
    g_shotAimProfile129==profile&&g_shotAimEpoch107==g_armPoseEpoch,"193 grip transition replaces pre-grip reticle and shot with final fixed barrel");
   g_aimActive=false;
  }
  if(profile==12||profile==20){
   g_aimActive=true;g_scopeEyes60[0]={};g_scopeEyes60[0].spec=5;g_scopeEyes60[0].ownerProfile=profile;g_scopeEyes60[0].epoch=g_armPoseEpoch;g_scopeEyes60[0].frame=g_frame;
   g_reticleAimFrame73=-1000;g_shotAimProfile129=-1;ResetScopeStabilization72();V3 recovered={};
   Check(HoloAllowed131()&&HoloRay132(recovered)&&Length(Sub(recovered,raw))<1e-7,"D50/signature1911 dot recovers current controller barrel across native camera dropout, in either grip");
   g_pistolReload.cycle=g_pistolReload.slideClosing=g_pistolReload.waitTriggerRelease=true;
   Check(HoloAllowed131()&&HoloRay132(recovered),"normal pistol firing cycle cannot switch physical reticle off");
   g_pistolReload.cycle=g_pistolReload.slideClosing=g_pistolReload.waitTriggerRelease=false;
   g_pistolReload.removalHeld=true;Check(!HoloAllowed131(),"manual magazine ownership still suspends physical reticle");g_pistolReload.removalHeld=false;
   g_scopeEyes60[0]={};g_aimActive=false;
  }
  ResetScopeStabilization72();ScopeAimCommand72(raw);Check(g_scopeAim72.active&&!g_scopeAim72.scoped129,"unscoped steadiness works before firing or holding LT");
  const auto rawHand=std::vector<float>(g_handBody[0],g_handBody[0]+16);const auto reload=g_pistolReload;
  g_scopeAim72.ms-=16;const V3 jitter=Unit(Add(raw,{.004,.002,.001}));V3 filtered=jitter;ScopeAimCommand72(filtered);
  Check(Length(Sub(filtered,raw))<Length(Sub(jitter,raw))*.55,"unscoped held aim damps tremor without trigger gating");
  const V3 transformed=Unit(Sub(Apply(g_scopeAim72.turn,jitter),Apply(g_scopeAim72.turn,{})));
  Check(Length(Sub(transformed,filtered))<1e-6,"same rigid correction sends displayed barrel to filtered shot ray");
  Check(memcmp(rawHand.data(),g_handBody[0],16*sizeof(float))==0&&memcmp(&reload,&g_pistolReload,sizeof(reload))==0,"smoothing does not alter physical controller/reload inputs");
  g_reload.held=true;Check(!ScopeAimValid72(),"physical reload immediately releases visual smoothing");g_reload.held=false;
  fc3aim::SetNativeActionForTest128(1);Check(!MotionInteractionEnabled()&&!ScopeAimValid72(),"native climb owns arms and disables aim smoothing");fc3aim::SetNativeActionForTest128(-1);
  g_weaponSteady129=0;filtered=jitter;ScopeAimCommand72(filtered);Check(!g_scopeAim72.active&&Length(Sub(filtered,jitter))<1e-9,"zero weapon steadiness is a complete immediate bypass");g_weaponSteady129=.65f;
 }
 g_machete.held=true;fc3pad::SetMacheteHandling(true);fc3aim::SetVehicleStateForTest(1);TickMachete(true);
 Check(!MacheteHeld()&&!MotionInteractionEnabled(),"confirmed vehicle stows machete and suspends arm lease even with visible weapon draws");
 fc3aim::SetVehicleStateForTest(-1);ResetScopeStabilization72();g_weaponSteady129=.45f;
 puts("PASS: shooting12925 profiles x one/two hands: fixed barrel ray, idle aim damping, shared visual/shot correction, zero bypass, raw reload preservation, action release and vehicle stow");
}
void TestScopeRay129(ID3D11Device* device,ID3D11DeviceContext* context){
 TestScopeProjection62(device,context);fc3pad::Controllers controls129={};controls129.active=true;controls129.leftGrip=controls129.leftTrigger=1;fc3pad::Publish(controls129);g_aimActive=true;g_frameCameraValid=true;g_frameCamera={};g_frameCamera.mainView=1;
 g_frameCamera.right[0]=g_frameCamera.up[1]=g_frameCamera.forward[2]=1;g_frameCamera.xScale=g_frameCamera.yScale=g_frameCamera.zScale=1;
 const auto identity=HandIdentity();g_shotAimWorld107=Unit({.1,.02,1});g_shotAimFrame107=g_frame;g_shotAimEpoch107=g_armPoseEpoch;
 for(int spec=0;spec<5;++spec){
  g_weaponMechanicProfile=kScopeSpecs60[spec].profile<0?12:kScopeSpecs60[spec].profile;g_shotAimProfile129=g_weaponMechanicProfile;
  for(int eye=0;eye<2;++eye){auto& p=g_scopeEyes60[eye];p.spec=spec;p.ownerProfile=g_weaponMechanicProfile;p.epoch=g_armPoseEpoch;p.frame=g_frame;p.bodyValid72=false;
   p.draw.anchor.world=identity;p.draw.anchor.world.m[3]=-kScopeSpecs60[spec].center.x;p.draw.anchor.world.m[7]=.5-kScopeSpecs60[spec].center.y;p.draw.anchor.world.m[11]=-kScopeSpecs60[spec].center.z;
  }
  g_scopeEyes60[1].draw.anchor.world.m[7]+=.05;
  float paired129[dxbc::kEyeRows*4]={};ScopePresentation62(paired129,1,0);ScopePresentation62(paired129,1,1);
  Check(fabs(g_scopePresentation62[0][0]-g_scopePresentation62[1][0])<1e-7,"asymmetric lens distances still consume one shared size history");
  for(int eye=0;eye<2;++eye){UpdateScopeProjection62(identity.m,eye);Check(g_scopeShotFrame129[eye]==g_frame,"all five magnifying scope geometries use current outgoing shot axis");
   float rows[dxbc::kEyeRows*4]={};Read(device,context,g_scopeVs62[eye],rows);Mat4 narrow={};for(int i=0;i<16;++i)narrow.m[i]=rows[i]+(i%5==0?1.:0.);
   const V3 dir=g_shotAimWorld107;double p[4]={dir.x,dir.y,dir.z,dir.z},clip[4]={};for(int r=0;r<4;++r)for(int j=0;j<4;++j)clip[r]+=narrow.m[r*4+j]*p[j];
   Check(fabs(clip[0]/clip[3])<1e-6&&fabs(clip[1]/clip[3])<1e-6,"uploaded optical raster places outgoing ray exactly under reticle");
   auto uv=ScopeSampleOffset129({.3f,-.2f},eye,4);Check(uv.x==0&&uv.y==0,"animated ocular axis cannot shift the sampled reticle target a second time");
  }
 }
 V3 axis={};g_shotAimProfile129=-1;Check(!ScopeShotAxis129(axis),"scope rejects ray from another weapon");g_shotAimProfile129=g_weaponMechanicProfile;g_shotAimEpoch107++;Check(!ScopeShotAxis129(axis),"scope rejects old pose epoch");g_shotAimEpoch107=g_armPoseEpoch;g_shotAimFrame107=g_frame-3;Check(!ScopeShotAxis129(axis),"scope rejects stale ray");
 g_aimActive=false;g_shotAimProfile129=-1;ResetScopeProjection62();context->ClearState();
 puts("PASS: optics129 all5 magnifying geometries (physical reflex now131), both-eye uploaded narrow-raster/reticle/shot agreement and stale weapon/epoch/frame rejection");
}
