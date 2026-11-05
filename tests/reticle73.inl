void TestReticle73(){
 g_aimActive=g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_weaponMotionGuard=false;g_flatViewActive=false;
 const auto identity=HandIdentity();for(int i=0;i<16;++i){g_handBody[0][i]=float(identity.m[i]);g_eyeViewsBody[0].view[i]=g_eyeViewsBody[1].view[i]=float(identity.m[i]);}
 g_eyeViewsBody[0].view[3]=.032f;g_eyeViewsBody[1].view[3]=-.032f;
 ++g_frame;PublishReticleAim73({0,0,1});V3 straight={},turned={},headMoved={},otherEye={};
 Check(ReticleTarget73(0,identity,straight),"regular weapon reticle has a current physical ray");
 PublishReticleAim73({.3,0,1});Check(ReticleTarget73(0,identity,turned)&&turned.x/turned.z-straight.x/straight.z>.29,"right-hand aim moves regular reticle with fixed native camera and fixed head");
 const auto ray=g_reticleAimBody73;const double a=.2;g_eyeViewsBody[0].view[0]=g_eyeViewsBody[0].view[10]=float(cos(a));g_eyeViewsBody[0].view[2]=float(-sin(a));g_eyeViewsBody[0].view[8]=float(sin(a));
 Check(ReticleTarget73(0,identity,headMoved)&&fabs(headMoved.x/headMoved.z-turned.x/turned.z)>.1&&Length(Sub(ray,g_reticleAimBody73))<1e-12,"head turn reprojects fixed weapon ray instead of dragging it with head tracking");
 for(int i=0;i<16;++i)g_eyeViewsBody[0].view[i]=float(identity.m[i]);g_eyeViewsBody[0].view[3]=.032f;
 Check(ReticleTarget73(0,identity,turned)&&ReticleTarget73(1,identity,otherEye)&&fabs(turned.x-otherEye.x-.064)<1e-5,"both eyes project the same weapon target with stereo disparity");
 Mat4 camera=identity;camera.m[0]=camera.m[10]=cos(.8);camera.m[2]=-sin(.8);camera.m[8]=sin(.8);V3 changedCamera={};Check(ReticleTarget73(0,camera,changedCamera)&&Length(Sub(changedCamera,turned))<1e-10,"visibility camera rotation cannot move the motion reticle");
 g_frame+=3;Check(!ReticleTarget73(0,identity,turned),"stale physical aim fails closed rather than using head-forward camera");++g_frame;PublishReticleAim73({0,0,-1});Check(!ReticleTarget73(0,identity,turned),"weapon aimed behind eye does not draw head-forward dot");
 // A muzzle 25 cm above the wrist must put the close-range crosshair at the
 // muzzle-origin hit, in both eyes. This caught shots landing above the HUD crosshair.
 const auto oldFrame=g_muzzleLaserFrame203;const auto oldBarrel=g_barrelBody206;const auto oldBarrelFrame=g_barrelFrame206;
 const auto oldBarrelEpoch=g_barrelEpoch206;const auto oldBarrelFamily=g_barrelFamily206;const auto oldProfile=g_weaponMechanicProfile;
 g_weaponMechanicProfile=20;g_barrelBody206={0,0,1};g_barrelFrame206=g_frame;g_barrelEpoch206=g_armPoseEpoch;g_barrelFamily206=20;
 g_muzzleLaserFrame203={};g_muzzleLaserFrame203.valid=true;g_muzzleLaserFrame203.frame=g_frame;g_muzzleLaserFrame203.epoch=g_armPoseEpoch;
 g_muzzleLaserFrame203.profile=20;g_muzzleLaserFrame203.native=identity;g_muzzleLaserFrame203.move=identity;
 // The game weapon lives far from world zero. Its native camera returns it
 // to camera space; ReticleTarget73 receives an XR eye view, not that camera.
 g_muzzleLaserFrame203.move.m[3]=1000;g_muzzleLaserFrame203.view=identity;g_muzzleLaserFrame203.view.m[3]=-1000;
 g_muzzleLaserFrame203.barrelWorld={0,0,1};g_muzzleLaserFrame203.barrelValid=true;
 for(int eye=0;eye<2;++eye)for(int i=0;i<16;++i)g_eyeViews[eye].view[i]=float(identity.m[i]);
 g_eyeViews[0].view[3]=.032f;g_eyeViews[1].view[3]=-.032f;
 fc3aim::LaserSample194 sample={};sample.ms=GetTickCount64();sample.epoch=g_armPoseEpoch;sample.profile=20;
 sample.localValid=sample.hit=true;sample.distance=2;sample.muzzleLocal[1]=.25f;
 g_reticleFixture212=&sample;
 Mat4 leftEye=identity,rightEye=identity;leftEye.m[3]=.032;rightEye.m[3]=-.032;
 V3 left={},right={};Check(ReticleTarget73(0,leftEye,left)&&ReticleTarget73(1,rightEye,right),"muzzle-origin crosshair is available in both eyes, away from world origin");
 Check(fabs(left.y-.25)<1e-5&&fabs(right.y-.25)<1e-5&&fabs(left.z-2)<1e-5&&fabs(right.z-2)<1e-5&&fabs(left.x-right.x-.064)<1e-5,"close-range crosshair projects the shot from the muzzle, not the lower wrist");
 Check(fabs(left.x/left.z)<1&&fabs(left.y/left.z)<1&&fabs(right.x/right.z)<1&&fabs(right.y/right.z)<1,"muzzle-origin HUD marker remains on-screen in both eyes with a nonzero native world position");
 for(int eye=0;eye<2;++eye){ImVec2 dot={};Check(OpticShotDot131(eye,dot),"holographic aiming dot uses the current muzzle-origin target");
  const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
  Check(fabs(dot.y-(projection.m[5]*.125+projection.m[6]))<1e-5&&fabs(dot.x-(projection.m[0]*(eye?-.016:.016)+projection.m[2]))<1e-5,"holographic dot aligns with the finite-range muzzle hit in both eyes");}
 sample.localValid=false;Check(ReticleTarget73(0,identity,left)&&fabs(left.y-.25)>.1,"invalid muzzle falls back to the existing physical reticle");g_reticleFixture212=nullptr;
 g_muzzleLaserFrame203=oldFrame;g_barrelBody206=oldBarrel;g_barrelFrame206=oldBarrelFrame;
 g_barrelEpoch206=oldBarrelEpoch;g_barrelFamily206=oldBarrelFamily;g_weaponMechanicProfile=oldProfile;
 g_aimActive=false;Check(ReticleTarget73(0,identity,turned)&&fabs(turned.x)<1e-10&&turned.z==15,"non-motion camera reticle path retained");
 puts("PASS: reticle73 physical right-hand aim, head-independent ray, visibility-camera isolation, stereo projection and stale/behind-eye rejection");
}
