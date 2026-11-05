void TestVehicleWheel187(){
 fc3aim::VehicleSample170 s={};s.identityValid=s.seatValid=true;s.vehicleId=42;s.type=0;s.actualSeat=1;
 fc3pad::Controllers c={};c.active=true;
 fc3aim::SetVehicleSampleForTest187(&s);fc3aim::SetVehicleStateForTest(1);fc3pad::SetCarWheelSteeringEnabled(true);
 fc3pad::Publish(c);fc3pad::SetCarWheelInput352(42,.5f);
 Check(fc3pad::SteeringOutput()>16000,"352 verified current-car render lease reaches steering");
 c.leftX=-.6f;fc3pad::Publish(c);Check(fc3pad::SteeringOutput()<-19000,"352 analog input cancels lease");
 c.leftX=0;fc3pad::Publish(c);Check(fc3pad::SteeringOutput()==0,"352 cancelled lease cannot revive");
 fc3pad::SetCarWheelInput352(43,.5f);Check(fc3pad::SteeringOutput()==0,"352 other vehicle identity cannot steer");
 s.actualSeat=2;fc3aim::SetVehicleSampleForTest187(&s);fc3pad::SetCarWheelInput352(42,.5f);
 Check(fc3pad::SteeringOutput()==0,"352 passenger cannot consume a wheel lease");
 fc3pad::SetCarWheelSteeringEnabled(false);Check(fc3pad::SteeringOutput()==0,"352 feature disable restores native input");
 fc3aim::SetVehicleSampleForTest187(nullptr);fc3aim::SetVehicleStateForTest(-1);fc3pad::Publish(c);
 puts("PASS:352 render-owned current-car steering lease, analog override and seat guards");
}

// Vehicle head look (candidate 102): only after the vehicle camera is detected, never on foot.
void TestVehicleLook()
{
    const bool keepEnabled=g_enabled,keepTracking=g_headTracking,keepEyes=g_eyeViewsSet,keepGameplay=g_inGameplay,keepCam=g_frameCameraValid,keepFlat=g_flatViewActive;
    const CameraSummary keepCamera=g_frameCamera;const LONG keepDraws=g_visibleWeaponDrawsLastFrame;
    g_enabled=g_headTracking=g_eyeViewsSet=g_inGameplay=g_frameCameraValid=true;g_flatViewActive=false;g_visibleWeaponDrawsLastFrame=0;g_pistolMenu.open=false;
    const bool keepMotion=g_motionGun;g_motionGun=false; // no weapon: motion interaction off
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
    auto camera=[](double yaw){CameraSummary s={};s.forward[0]=float(std::cos(yaw));s.forward[1]=float(std::sin(yaw));s.right[0]=float(std::sin(yaw));s.right[1]=float(-std::cos(yaw));s.up[2]=1;s.xScale=.37f;s.yScale=.66f;s.mainView=1;return s;};
    auto head=[](double yaw){for(auto& e:g_eyeViewsBody){std::memset(e.view,0,sizeof(e.view));e.view[0]=float(std::cos(yaw));e.view[2]=float(-std::sin(yaw));e.view[5]=1;e.view[8]=float(std::sin(yaw));e.view[10]=float(std::cos(yaw));e.view[15]=1;}};
    g_frameCamera=camera(0);head(0);g_vehicleLook=VehicleLook102{};
    g_vehicleCamera92=false;
    Check(!TickVehicleLook102(true)&&!g_vehicleLook.active,"vehicle look: never runs on foot (no vehicle camera detected)");
    g_vehicleCamera92=true;
    Check(TickVehicleLook102(true)&&g_vehicleLook.active&&!g_vehicleLook.headingValid&&!g_aimActive,"vehicle look: engages on the vehicle camera, plain view until the heading is known");
    g_vehicleLook.stableSince-=500;TickVehicleLook102(true);
    Check(g_vehicleLook.headingValid&&std::fabs(g_vehicleLook.heading)<1e-6,"vehicle look: heading from the recentred, steady camera");
    TickVehicleLook102(true);Check(g_vehicleLook.stickX==0&&g_aimActive,"vehicle look: head forward, stick at rest, view anchored");
    head(.5);TickVehicleLook102(true);
    Check(g_vehicleLook.stickX>.25f,"vehicle look: head right pushes the look stick right");
    g_frameCamera=camera(-.25);TickVehicleLook102(true);
    Check(g_vehicleLook.stickX>.25f&&g_vehicleLook.stickX<1.f,"vehicle look: half way, still pushing but less");
    Check(std::fabs(g_bodyAxes[2][0]-1)<1e-6&&std::fabs(g_bodyAxes[2][1])<1e-6,"vehicle look: the body basis is the vehicle heading, not the turned camera");
    Check(std::fabs(g_aimQ[2][2]-std::cos(.25))<1e-6,"vehicle look: Q removes exactly the look-stick turn");
    g_frameCamera=camera(-.5);TickVehicleLook102(true);
    Check(g_vehicleLook.stickX==0,"vehicle look: camera on the head, stick at rest");
    // the head stays turned for seconds: the heading must NOT jump to the camera (candidate 101's spin)
    g_vehicleLook.stableSince-=5000;for(int i=0;i<3;++i)TickVehicleLook102(true);
    Check(std::fabs(g_vehicleLook.heading)<1e-6,"vehicle look: a turned head never re-takes the heading from the camera");
    // head back forward: pulled back toward the heading, then the game's recentred camera is the heading
    head(0);TickVehicleLook102(true);Check(g_vehicleLook.stickX<-.25f,"vehicle look: head forward again pulls the camera back");
    g_frameCamera=camera(-.05);TickVehicleLook102(true);Check(g_vehicleLook.stickX==0,"vehicle look: near the heading the stick rests for the game's recentring");
    TickVehicleLook102(true);
    Check(std::fabs(g_vehicleLook.heading+.05)<1e-6,"vehicle look: stick at rest near the heading: the camera is the heading at once (a turning car turns the view, no judder)");
    // The user's correction keeps ownership for the episode; no timeout can seize it back.
    c.rightX=.9f;fc3pad::Publish(c);TickVehicleLook102(true);
    Check(!g_aimActive&&!g_vehicleLook.headingValid&&g_vehicleLook.stickX==0,"vehicle look: the user's right stick suspends it and shows the plain camera");
    c.rightX=0;fc3pad::Publish(c);TickVehicleLook102(true);Check(!g_aimActive,"vehicle look: grace after the user's stick");
    g_vehicleLook.stableSince-=5000;head(.4);TickVehicleLook102(true);
    Check(!g_vehicleLook.headingValid&&!g_aimActive&&g_vehicleLook.stickX==0&&g_vehicleLookDirection.manualControl175,"175 corrected camera stays manual after old timeout, even with nonzero head yaw");
    g_vehicleLookLastTick-=2000;TickVehicleLook102(true);
    Check(g_vehicleLookDirection.manualControl175&&!g_aimActive&&g_vehicleLook.stickX==0,"175 tick gap does not re-enable rejected automatic look");
    VehicleLookStop("175 eligibility interruption fixture");TickVehicleLook102(true);
    Check(g_vehicleLookDirection.manualControl175&&!g_aimActive&&g_vehicleLook.stickX==0,"175 temporary eligibility stop preserves manual ownership");
    // Explicit vehicle exit permits a fresh episode for the remaining automatic-look tests.
    g_vehicleCamera92=false;TickVehicleLook102(true);
    Check(g_vehicleLookDirection.manualControl175,"175 camera heuristic dropout with unknown native ownership preserves manual latch");
    fc3aim::SetVehicleStateForTest(0);TickVehicleLook102(true);fc3aim::SetVehicleStateForTest(-1);
    g_vehicleCamera92=true;head(0);TickVehicleLook102(true);
    g_vehicleLook.stableSince-=500;TickVehicleLook102(true);Check(g_vehicleLook.headingValid&&!g_vehicleLookDirection.manualControl175,"175 vehicle exit re-arms a fresh automatic episode");
    // travel direction while the head is turned (stick pushed): a turning car moves the heading; reversing does not
    const double h0=g_vehicleLook.heading;head(.6);g_frameCamera=camera(h0);TickVehicleLook102(true);Check(g_vehicleLook.stickX>.25f,"vehicle look: head turned, pushing");
    g_vehicleLook.lastPosMs=GetTickCount64()-300;g_vehicleLook.lastFrameMs=GetTickCount64()-300;
    g_frameCamera.position[0]=g_vehicleLook.lastPos[0]+float(3*std::cos(h0-.3));g_frameCamera.position[1]=g_vehicleLook.lastPos[1]+float(3*std::sin(h0-.3));TickVehicleLook102(true);
    Check(g_vehicleLook.heading<h0-.05&&g_vehicleLook.heading>=h0-.3-1e-9,"vehicle look: the heading turns toward the travel direction while the head is turned");
    const double h1=g_vehicleLook.heading;
    g_vehicleLook.lastPosMs=GetTickCount64()-300;g_vehicleLook.lastFrameMs=GetTickCount64()-300;g_frameCamera.position[0]-=6;TickVehicleLook102(true);
    Check(std::fabs(g_vehicleLook.heading-h1)<1e-9,"vehicle look: reversing keeps the heading");
    head(0);
    // unresponsive camera releases the stick
    head(1.0);g_frameCamera=camera(g_vehicleLook.heading);TickVehicleLook102(true);
    Check(g_vehicleLook.stickX>.6f&&g_vehicleLook.pushSince>0,"vehicle look: pushing hard");
    // the camera turns the pushed way while the head keeps moving away: the error stays large, NOT a failure
    {const double h=g_vehicleLook.heading;g_vehicleLook.pushSince-=2000;head(1.6);g_frameCamera=camera(h-.6);TickVehicleLook102(true);
     Check(!g_vehicleLook.failed&&g_vehicleLook.stickX>.6f,"vehicle look: a camera that turns while the head keeps moving is responsive (run 08:05 false release)");
     g_vehicleLook.pushSince-=2000;TickVehicleLook102(true);}
    Check(g_vehicleLook.failed&&g_vehicleLook.stickX==0,"vehicle look: a camera that does not turn for 1.5 s releases the stick");
    g_vehicleLook.failedAt-=6000;TickVehicleLook102(true);Check(!g_vehicleLook.failed&&g_vehicleLook.stickX>.6f,"vehicle look: retried after 5 s");
    // candidate 107: a reversed look stick (run 6764's jetski suspicion) is learned from the first hard push
    {
        auto& dir=g_vehicleLookDirection;
        auto restart=[&](){g_vehicleLook=VehicleLook102{};VehicleLookForgetDirection(-1);g_frameCamera=camera(0);head(0);TickVehicleLook102(true);g_vehicleLook.stableSince-=500;TickVehicleLook102(true);};
        restart();Check(g_vehicleLook.headingValid&&std::fabs(g_vehicleLook.heading)<1e-6&&!dir.learned&&dir.sign>0,"vehicle look 107: new episode, direction not yet learned");
        head(1.0);TickVehicleLook102(true);g_vehicleLook.learnSince-=1000;
        c.rightX=.9f;fc3pad::Publish(c);g_frameCamera=camera(.4);TickVehicleLook102(true);
        Check(!dir.learned&&g_vehicleLook.learnSince==0&&g_vehicleLook.learnDirection==0,"172 manual look cancels unfinished automatic direction measurement");
        c.rightX=0;fc3pad::Publish(c);g_vehicleLook.stableSince-=500;TickVehicleLook102(true);
        TickVehicleLook102(true);
        Check(!dir.learned&&dir.sign>0&&g_vehicleLook.learnSince==0&&dir.manualControl175&&!g_aimActive,"175 manual rotation never restarts automatic direction measurement in the same episode");
        restart();
        head(1.0);TickVehicleLook102(true);Check(g_vehicleLook.stickX>.6f&&g_vehicleLook.learnSince>0,"vehicle look 107: first hard push starts the 300 ms measurement");
        g_frameCamera=camera(.3);g_vehicleLook.learnSince-=300;TickVehicleLook102(true); // pushed right, the camera turned left
        Check(dir.learned&&dir.sign<0&&dir.flips==1&&!g_vehicleLook.failed&&g_vehicleLook.stickX>.6f,"vehicle look 107: camera turned opposite to the push -> direction learned reversed, stick kept (not released)");
        g_vehicleLook.pushSince-=2000;g_frameCamera=camera(-.2);TickVehicleLook102(true);
        Check(!g_vehicleLook.failed&&dir.sign<0&&dir.flips==1,"vehicle look 107: with the reversed stick the camera follows the push, no release, no flip");
        c.rightX=.9f;fc3pad::Publish(c);TickVehicleLook102(true);c.rightX=0;fc3pad::Publish(c);
        Check(dir.learned&&dir.sign<0&&g_vehicleLook.learnSince==0&&g_vehicleLook.learnDirection==0,"172 user's stick clears measurement timers but preserves completed reversed direction");
        fc3aim::SetVehicleStateForTest(1);TickVehicleLook102(true);
        Check(!dir.learned&&dir.sign>0&&dir.seat==0,"vehicle look 107: another seat forgets the learned direction");
        fc3aim::SetVehicleStateForTest(-1);
        restart();head(1.0);TickVehicleLook102(true);
        g_frameCamera=camera(-.3);g_vehicleLook.learnSince-=300;TickVehicleLook102(true);
        Check(dir.learned&&dir.sign>0&&dir.flips==0,"vehicle look 107: camera turned the pushed way -> direction learned normal");
        g_vehicleLook.pushSince-=2000;g_frameCamera=camera(.3);TickVehicleLook102(true);
        Check(dir.sign<0&&dir.flips==1&&!g_vehicleLook.failed,"vehicle look 107: 1.5 s of turning the other way flips the direction instead of releasing");
        g_vehicleLook.pushSince-=2000;g_frameCamera=camera(.6);TickVehicleLook102(true);
        Check(dir.sign>0&&dir.flips==2&&!g_vehicleLook.failed,"vehicle look 107: flipped back once more");
        g_vehicleLook.pushSince-=2000;g_frameCamera=camera(.9);TickVehicleLook102(true);
        Check(g_vehicleLook.failed&&g_vehicleLook.stickX==0&&dir.flips==2,"vehicle look 107: after two flips a wrong-way camera releases the stick (old guard)");
        // run 6764: on foot with a weapon this tick does not run; the jetski inherited the car's heading and push
        g_vehicleLookLastTick-=2000;TickVehicleLook102(true);
        Check(g_vehicleLook.active&&!g_vehicleLook.headingValid&&!g_vehicleLook.failed&&g_vehicleLook.pushSince==0&&!dir.learned,"vehicle look 107: a tick gap starts a new vehicle episode (no stale heading, push, release or direction)");
        g_vehicleLook.stableSince-=500;TickVehicleLook102(true);
    }
    // released when the vehicle camera ends (a weapon appears); disabled by the ini
    g_vehicleCamera92=false;Check(!TickVehicleLook102(true)&&!g_vehicleLook.active&&!g_vehicleLook.failed,"vehicle look: off and re-armed when the vehicle camera ends");
    g_vehicleCamera92=true;g_vehicleLookEnabled=false;Check(!TickVehicleLook102(true),"vehicle look: disabled by [Vehicle] HeadLook=0");
    g_vehicleLookEnabled=true;
    // the engine's vehicle state decides the vehicle camera at once, both ways, with no head turn
    g_vehicleCamera92=false;head(0);fc3aim::SetVehicleStateForTest(1);TickVisibilityCamera68(true);
    Check(g_vehicleCamera92,"vehicle state: in a vehicle seat -> vehicle camera immediately, head forward");
    Check(TickVehicleLook102(true)&&g_vehicleLook.active,"vehicle state: the vehicle look takes over in the same frame");
    fc3aim::SetVehicleStateForTest(0);TickVisibilityCamera68(true);
    Check(!g_vehicleCamera92,"vehicle state: on foot -> never the vehicle camera");
    head(1.0);for(int i=0;i<3;++i)TickVisibilityCamera68(true);
    Check(!g_vehicleCamera92,"vehicle state: on foot, a head turn with a still camera cannot fake a vehicle");
    fc3aim::SetVehicleStateForTest(1);g_visibleWeaponDrawsLastFrame=3;TickVisibilityCamera68(true);
    Check(!g_vehicleCamera92,"vehicle state: a visible weapon (passenger shooting) keeps the aiming camera");
    g_visibleWeaponDrawsLastFrame=0;fc3aim::SetVehicleStateForTest(-1);head(0);
    g_vehicleLookEnabled=true;g_vehicleCamera92=false;TickVehicleLook102(true);fc3pad::SetLookOverride(false,0,0);
    g_enabled=keepEnabled;g_headTracking=keepTracking;g_eyeViewsSet=keepEyes;g_inGameplay=keepGameplay;g_frameCameraValid=keepCam;g_flatViewActive=keepFlat;g_frameCamera=keepCamera;g_visibleWeaponDrawsLastFrame=keepDraws;g_motionGun=keepMotion;
    c={};fc3pad::Publish(c);g_aimActive=false;
    std::puts("PASS: vehicle look: never on foot, plain view until a recentred heading, look stick follows the head, view anchored to the heading (Q removes only the stick turn), turned head never re-takes the heading, recentring and travel direction update it (reversing ignored), user's stick shows the plain camera, unresponsive camera releases the stick, reversed look stick learned in 300 ms and remembered per seat (wrong-way turns flip it at most twice, then release), a tick gap starts a new vehicle episode, off with the vehicle camera, ini switch");
}

void TestVehicleEpisode170()
{
 const auto prior=g_vehicleLookDirection;VehicleLookForgetDirection(0);
 fc3aim::VehicleSample170 sample;sample.identityValid=sample.seatValid=true;sample.pawnId=1;sample.vehicleId=10;sample.type=0;sample.actualSeat=0;
 Check(VehicleEpisode170(&sample,0,false,true),"170 first verified vehicle starts fresh episode");
 g_vehicleLookDirection.learned=true;g_vehicleLookDirection.sign=-1;g_vehicleLookDirection.manualControl175=true;
 Check(!VehicleEpisode170(&sample,0,false,true)&&g_vehicleLookDirection.learned,"170 same owner/vehicle/seat keeps learned direction");
 Check(VehicleEpisode170(&sample,0,false,false)&&g_vehicleLookDirection.manualControl175&&g_vehicleLookDirection.identity170,"175 camera heuristic false with same verified vehicle preserves correction");
 Check(VehicleEpisode170(nullptr,-1,true,false)&&g_vehicleLookDirection.manualControl175&&g_vehicleLookDirection.identity170,"175 unknown ownership and camera gap cannot seize look back");
 Check(!VehicleEpisode170(&sample,0,false,true)&&g_vehicleLookDirection.manualControl175,"175 recovered same native episode remains manual");
 Check(!VehicleEpisode170(nullptr,0,false,true)&&g_vehicleLookDirection.learned&&g_vehicleLookDirection.manualControl175,"170 transient sample gap does not invent new episode");
 Check(VehicleEpisode170(nullptr,1,false,true)&&!g_vehicleLookDirection.identity170,"170 confirmed new legacy type resets cache even after native identity was learned");
 VehicleEpisode170(&sample,0,false,true);sample.vehicleId=11;Check(VehicleEpisode170(&sample,0,false,true)&&!g_vehicleLookDirection.learned&&g_vehicleLookDirection.sign==1&&!g_vehicleLookDirection.manualControl175,"170 same-type different vehicle resets old direction");
 sample.actualSeat=1;Check(VehicleEpisode170(&sample,0,false,true),"170 actual seat change resets direction even with unchanged type");
 sample.seatValid=false;Check(!VehicleEpisode170(&sample,0,false,true),"170 unknown seat does not manufacture seat change");
 sample.seatValid=true;sample.pawnId=2;Check(VehicleEpisode170(&sample,0,false,true),"170 respawn/new pawn resets vehicle episode");
 Check(VehicleEpisode170(&sample,0,true,true),"170 tick gap still resets");
 Check(VehicleEpisode170(nullptr,-1,false,false)&&!g_vehicleLookDirection.identity170,"170 exit discards native identity");
 Check(VehicleEpisode170(nullptr,1,false,true)&&!g_vehicleLookDirection.identity170,"170 legacy type fallback remains available without native sample");
 g_vehicleLookDirection.manualControl175=true;
 Check(VehicleEpisode170(nullptr,-1,false,false,true)&&!g_vehicleLookDirection.manualControl175,"175 confirmed native on-foot state clears manual latch");
 g_vehicleLookDirection=prior;puts("PASS:170 vehicle episode uses exact pawn/vehicle/seat, same-type change, respawn, transient sample loss and legacy fallback guards.");
}
