// Candidate 107 fixtures.
// Sprint latch, PID63244 (M133): sprint 7748-8374 at 6.0-7.8 m/s, a shot at frame 8373 fired
// mid-sprint, "one-hand shot rotation compensated 44.6 degrees" once the player stopped. The
// captured firing peak pose stands in for the steady sprint carry pose (both depart > 8 degrees
// from the captured standing grip).
void TestShotHandSprint107()
{
    fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
    const CameraSummary savedCamera=g_frameCamera;const bool savedCameraValid=g_frameCameraValid;
    g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_twoHand=false;g_reload=PhysicalReloadState{};
    const auto& stand=kShotFixture[0][0];const auto& carry=kShotFixture[0][1];
    float raw[576]={};ArmJoints joints={};double seconds=100,x=0;
    // Production order (111): the frame starts with no world camera (reset at Present), the
    // first-person draws observe the hand, the world camera arrives, Present samples it.
    auto step=[&](const CapturedHandFixture& f,double metresPerSecond){
        seconds+=1/60.;x+=metresPerSecond/60.;++g_frame;g_reload.shotgunFrame=g_frame;g_shotHandClockForTest107=seconds;
        g_frameCameraValid=false;memcpy(raw,f.bones,sizeof(f.bones));FixtureBodyTurn(f);ObserveShotHand(raw,f.globals,FixtureHandCamera(f),joints);
        g_frameCameraValid=true;g_frameCamera.position[0]=static_cast<float>(x);g_frameCamera.position[1]=1;g_frameCamera.position[2]=2;TrackShotHandMotion111();};
    // 1. Sprinting never relearns the idle grip; the shot after the sprint releases on the stand pose.
    g_shotHand=ShotHandControl{};g_shotHandMotion107=ShotHandMotion107{};g_chambered=true;g_lastShotFrame=-100000;
    for(int i=0;i<10;++i)step(stand,0);
    Check(g_shotHand.valid&&!g_shotHand.active,"standing still learns the idle grip");
    const Mat4 standIdle=g_shotHand.idle;
    Check(HandAngle(standIdle,g_shotHand.current)<1e-6,"idle is the standing hold");
    for(int i=0;i<40;++i)step(stand,4.0);
    Check(!ShotHandSprinting107(),"running at 4.0 m/s (PID63244 run 3.4-4.3) is not a sprint");
    // Run 45704 run-up: the carry pose settles at 4.5 m/s, below the threshold, and is learned...
    for(int i=0;i<12;++i)step(carry,4.5);
    Check(HandAngle(g_shotHand.idle,standIdle)>8,"run-up control: a steady carry pose below 5 m/s is learned first");
    for(int i=0;i<60;++i)step(carry,7.2);
    Check(ShotHandSprinting107()&&HandAngle(g_shotHand.idle,standIdle)<1e-6,"camera sampled at Present: sprint detected, the run-up carry pose is discarded and the standing grip restored; never relearned at 7.2 m/s");
    g_lastShotFrame=g_frame+1;g_chambered=false;step(carry,7.2);
    Check(g_shotHand.active&&g_shotHand.departed,"a shot fired mid-sprint arms the correction against the standing grip");
    for(int i=0;i<30;++i)step(stand,0);
    Check(g_shotHand.active,"before the pump the correction keeps owning the wrist");
    g_chambered=true;step(stand,0);
    Check(!g_shotHand.active,"after the sprint stops and the pump chambers, the standing hold releases the correction: no latch");
    for(int i=0;i<10;++i)step(stand,0);
    Check(g_shotHand.valid&&HandAngle(g_shotHand.idle,standIdle)<1e-6,"idle relearns the standing hold");
    // 2. Backstop: a stale idle (any stance learned while still) cannot pin the gun forever.
    g_shotHand=ShotHandControl{};g_shotHandMotion107=ShotHandMotion107{};g_chambered=true;g_lastShotFrame=-100000;
    for(int i=0;i<10;++i)step(carry,0);
    Check(g_shotHand.valid&&HandAngle(g_shotHand.idle,standIdle)>8,"control: a still, different stance is learned as idle");
    g_lastShotFrame=g_frame+1;g_chambered=false;step(stand,0);step(stand,0);
    Check(g_shotHand.active&&g_shotHand.departed,"control: the shot arms the correction against that stance");
    for(int i=0;i<80;++i)step(stand,0);
    Check(g_shotHand.active,"not chambered: the correction is kept (pump pending)");
    g_chambered=true;for(int i=0;i<kShotHandSettleFrames107-1;++i)step(stand,0);
    Check(g_shotHand.active,"settled for less than 0.75 s with the chamber ready: still corrected");
    step(stand,0);step(stand,0);
    Check(!g_shotHand.active,"settled 0.75 s at a different stance with the chamber ready: released");
    for(int i=0;i<10;++i)step(stand,0);
    Check(g_shotHand.valid&&HandAngle(g_shotHand.idle,standIdle)<1e-6,"idle relearns from the settled stance");
    // A long steady stance (> 0.75 s before the sprint) is a real idle and is not discarded.
    g_shotHand=ShotHandControl{};g_shotHandMotion107=ShotHandMotion107{};g_chambered=true;g_lastShotFrame=-100000;
    for(int i=0;i<90;++i)step(stand,0);for(int i=0;i<10;++i)step(stand,7.2);
    Check(g_shotHand.valid&&HandAngle(g_shotHand.idle,standIdle)<1e-6,"a standing grip held 1.5 s before sprinting survives the sprint onset");
    // Quiet counts frames: six observations inside two frames do not learn an idle.
    g_shotHand=ShotHandControl{};g_shotHandMotion107=ShotHandMotion107{};
    ++g_frame;g_reload.shotgunFrame=g_frame;g_frameCameraValid=false;for(int i=0;i<4;++i){memcpy(raw,stand.bones,sizeof(stand.bones));FixtureBodyTurn(stand);ObserveShotHand(raw,stand.globals,FixtureHandCamera(stand),joints);}
    ++g_frame;g_reload.shotgunFrame=g_frame;for(int i=0;i<4;++i){memcpy(raw,stand.bones,sizeof(stand.bones));FixtureBodyTurn(stand);ObserveShotHand(raw,stand.globals,FixtureHandCamera(stand),joints);}
    Check(!g_shotHand.valid,"eight observations in two frames are not six quiet frames");
    g_shotHandClockForTest107=-1;g_frameCamera=savedCamera;g_frameCameraValid=savedCameraValid;
    g_shotHand=ShotHandControl{};g_shotHandMotion107=ShotHandMotion107{};g_reload=PhysicalReloadState{};g_aimActive=false;g_motionGun=false;g_chambered=true;
    std::puts("PASS: sprint detector samples the world camera at Present (first-person draws precede it); a run-up carry pose learned < 0.75 s before the sprint is discarded; quiet counts frames; mid-sprint shot releases after the pump; a settled stale stance releases once chambered");
}
// Muzzle flash / tracer particles (111): world-space vertices near the native muzzle follow the gun.
void TestMuzzleFlash111()
{
    Mat4 native=HandIdentity();native.m[0]=native.m[5]=std::cos(.4);native.m[1]=-std::sin(.4);native.m[4]=std::sin(.4);native.m[3]=-600;native.m[7]=2200;native.m[11]=130;
    const V3 muzzle=Apply(native,V3{.05,.68,.06});V3 local={};
    Check(MuzzleFlashLocal111(native,muzzle,local)&&Length(Sub(local,V3{.05,.68,.06}))<1e-6,"native muzzle expressed in the native FRAME axes");
    Mat4 bad=native;bad.m[0]=3;Check(!MuzzleFlashLocal111(bad,muzzle,local),"non-rigid FRAME refused");
    Check(!MuzzleFlashLocal111(native,Apply(native,V3{0,3,0}),local),"a point 3 m from the FRAME is not its muzzle");
    // PID63244 D50: drawn tip 0.24 m below the native muzzle; move = translation + small turn.
    Mat4 move=TurnAndMove({0,1,0},{.05,1,0},Apply(native,V3{0,0,0}),Add(Apply(native,V3{0,0,0}),V3{0,0,-.24}));
    struct Vertex{float p[3];float rest[6];};std::vector<Vertex> vb(5);
    const V3 at[5]={muzzle,Add(muzzle,V3{.2,0,0}),Add(muzzle,V3{0,.39,0}),Add(muzzle,V3{0,0,1.5}),Add(muzzle,V3{-.1,.1,.05})};
    for(int i=0;i<5;++i){vb[i].p[0]=float(at[i].x);vb[i].p[1]=float(at[i].y);vb[i].p[2]=float(at[i].z);for(float& r:vb[i].rest)r=7.f;}
    const std::vector<UINT> refs={0,1,2,3};double nearest=-1;
    const UINT moved=MuzzleFlashMoveVertices111(reinterpret_cast<unsigned char*>(vb.data()),UINT(vb.size()*sizeof(Vertex)),sizeof(Vertex),0,muzzle,.4,move,refs,&nearest);
    Check(moved==3&&nearest<1e-3,"three referenced vertices within 0.4 m moved; the 1.5 m one (other effect) kept");
    const V3 drawnTip=Apply(move,muzzle);
    Check(std::fabs(vb[0].p[0]-drawnTip.x)<1e-3&&std::fabs(vb[0].p[1]-drawnTip.y)<1e-3&&std::fabs(vb[0].p[2]-drawnTip.z)<1e-3,"the flash centre lands on the drawn muzzle (same transport as the bullet)");
    Check(std::fabs(vb[3].p[2]-float(at[3].z))<1e-6&&std::fabs(vb[4].p[0]-float(at[4].x))<1e-6,"far and unreferenced vertices untouched");
    bool restKept=true;for(const auto& v:vb)for(float r:v.rest)restKept=restKept&&r==7.f;Check(restKept,"colour/uv/size attributes after the position untouched");
    Check(MuzzleFlashMoveVertices111(nullptr,0,36,0,muzzle,.4,move,refs,&nearest)==0&&MuzzleFlashMoveVertices111(reinterpret_cast<unsigned char*>(vb.data()),UINT(vb.size()*sizeof(Vertex)),8,0,muzzle,.4,move,refs,&nearest)==0,"bad buffer or stride refused");
    // Window: a fresh core-draw pose and a shot within 1 s, same epoch.
    g_muzzleFlash111=MuzzleFlash111{};g_muzzleFlashPose111=MuzzleFlashPose111{};g_muzzleFlashClockForTest111=50000;
    g_muzzleFlash111.local=local;g_muzzleFlash111.localValid=true;g_muzzleFlash111.seen=true;g_muzzleFlash111.shotMs=50000;g_muzzleFlash111.epoch=g_armPoseEpoch;
    g_muzzleFlashPose111.valid=true;g_muzzleFlashPose111.frame=g_frame;g_muzzleFlashPose111.epoch=g_armPoseEpoch;g_muzzleFlashPose111.native=native;g_muzzleFlashPose111.move=move;
    D3D11_BUFFER_DESC d={};d.Usage=D3D11_USAGE_DYNAMIC;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;d.ByteWidth=147456;
    Check(MuzzleFlashKeepVertexShadow111(d),"pooled 147456-byte particle VB shadowed right after a shot");
    d.ByteWidth=300000;Check(!MuzzleFlashKeepVertexShadow111(d),"large VBs never shadowed");d.ByteWidth=147456;
    g_muzzleFlashClockForTest111=51001;Check(!MuzzleFlashKeepVertexShadow111(d),"no shadowing 1 s after the shot");
    g_muzzleFlashClockForTest111=50500;g_muzzleFlashPose111.frame=g_frame-3;Check(!MuzzleFlashKeepVertexShadow111(d),"stale core-draw pose stops the transport");
    g_muzzleFlashClockForTest111=0;g_muzzleFlash111=MuzzleFlash111{};g_muzzleFlashPose111=MuzzleFlashPose111{};
    std::puts("PASS: muzzle flash/tracer particles: native muzzle anchored in the native FRAME; world vertices within 0.4 m moved to the drawn muzzle by the bullet's transport, others and attributes untouched; 1 s window, fresh pose, VB size bound");
}
// Shot direction: the native shot (camera forward plus spread) turned onto the aim/reticle ray.
void TestShotAim107()
{
    const float camera[3]={1,0,0};const double turn=.3;const float aim[3]={float(std::cos(turn)),float(std::sin(turn)),0};
    float out[3]={};
    Check(fc3aim::AimShotDirection107(camera,aim,camera,out)&&std::fabs(out[0]-aim[0])<1e-6&&std::fabs(out[1]-aim[1])<1e-6&&std::fabs(out[2])<1e-6,"unspread native shot lands exactly on the aim ray");
    const float spread[3]={2*float(std::cos(.03)),0,2*float(std::sin(.03))};
    Check(fc3aim::AimShotDirection107(camera,aim,spread,out),"spread shot accepted");
    const double len=std::sqrt(double(out[0])*out[0]+double(out[1])*out[1]+double(out[2])*out[2]);
    const double off=std::acos((std::max)(-1.,(std::min)(1.,(out[0]*aim[0]+out[1]*aim[1]+out[2]*aim[2])/len)));
    Check(std::fabs(len-2)<1e-5&&std::fabs(off-.03)<1e-5&&std::fabs(std::asin(out[2]/len)-.03)<1e-5,"native spread angle and vector length kept around the aim ray");
    const float same[3]={0,0,1};Check(fc3aim::AimShotDirection107(same,same,same,out)&&std::fabs(out[2]-1)<1e-6,"camera already on the aim: shot unchanged");
    const float sideways[3]={0,1,0};Check(!fc3aim::AimShotDirection107(camera,aim,sideways,out),"shot far from the camera forward is not re-aimed (fallback)");
    const float behind[3]={-1,0,0};Check(!fc3aim::AimShotDirection107(camera,behind,camera,out),"aim more than 75 degrees from the camera is not trusted");
    const float bad[3]={NAN,0,0};Check(!fc3aim::AimShotDirection107(bad,aim,camera,out)&&!fc3aim::AimShotDirection107(camera,aim,bad,out),"non-finite input rejected");
    // PID63244 D50 geometry: camera forward on the native hip pose, aim along the drawn barrel.
    // The old transport R_move*s re-applies the hip angle; the aim ray does not.
    const double hip=.06;const float hipCamera[3]={float(std::cos(hip)),0,float(std::sin(hip))},barrel[3]={1,0,0};
    Check(fc3aim::AimShotDirection107(hipCamera,barrel,hipCamera,out)&&std::fabs(out[0]-1)<1e-6&&std::fabs(out[2])<1e-6,"hip-angle camera shot lands on the drawn barrel ray");
    std::puts("PASS: shot direction turned from the native camera forward onto the aim/reticle ray with spread and length kept; far shots and untrusted aims fall back");
}
