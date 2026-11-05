// Vehicle head look (candidate 102, 2026-10-15). Runs ONLY while g_vehicleCamera92 is set: the
// on-foot head camera (visibility lane) was driving the native look and the rendered camera did not
// follow (on foot it follows within 0.03 rad, runs 14616/67572/2632, so this never runs on foot).
//
// Facts it rests on (runs 14616 and 2632): the vehicle camera turns with the pad's right stick
// (the game's vehicle free look, rate control, about 2.2 rad/s at full deflection) and, with the
// stick at rest, recentres on the vehicle's forward direction (camera yaw = travel heading within
// 0.03 rad over 30 samples). Candidate 101 got two things wrong, both fixed here: it engaged on foot
// in settlements (it keyed on the FOV override, which is also on there), and it re-took the heading
// from the camera after one second of a resting stick even while the head was turned, so the view
// spun and snapped back. Here the heading comes only from a recentred camera (the mod's stick idle, head
// forward, camera steady) and from the travel direction while driving; the user's own right stick
// releases automatic look for that vehicle episode and shows the plain camera.
//
// Ini [Vehicle]: HeadLook (default 1), LookGainPercent (default 200: full stick at 0.5 rad error).
//
// Candidate 107 (jetski, run 6764): two of the three jetski "camera turned only -x rad" releases
// were measured against push state left over from the previous vehicle. This tick does not run on
// foot while a weapon is out, so the state was never reset between the car and the jetski (heading
// 0.03 from the car applied to a jetski facing -1.37, a 1.4 rad view jump at entry). Now a tick gap
// or a seat change starts a new vehicle episode. The look stick direction is also learned per
// episode (kept while the engine reports the same seat): the first hard push is measured over
// 300 ms and a camera that turns the opposite way flips the stick instead of releasing it.
bool g_vehicleLookEnabled=true;float g_vehicleLookGain=2.0f;
struct VehicleLook102
{
    bool active=false,headingValid=false,failed=false;
    double heading=0,lastCamYaw=0,pushCamYaw=0,travelYaw=0,learnCamYaw=0;
    float lastPos[3]={},stickX=0;
    ULONGLONG lastPosMs=0,lastFrameMs=0,stableSince=0,pushSince=0,lastStatus=0,failedAt=0,learnSince=0;
    int pushDirection=0,learnDirection=0;bool travelValid=false,holding=false;
    int frames=0;
}g_vehicleLook;
// The look stick direction for the current vehicle: sign multiplies the stick sent to the game.
struct VehicleLookDirection107{int seat=-1;float sign=1;bool learned=false;int flips=0;unsigned long long pawn170=0,vehicle170=0;int actualSeat170=-1;bool identity170=false,seatValid170=false,manualControl175=false;}g_vehicleLookDirection;
ULONGLONG g_vehicleLookLastTick=0;
double Wrap102(double a){return std::remainder(a,6.283185307179586);}
void VehicleLookStop(const char* why)
{
    if(g_vehicleLook.active){Log("[VEHICLE LOOK] off (%s) after %d frames",why,g_vehicleLook.frames);fc3pad::SetLookOverride(false,0,0);}
    const bool failed=g_vehicleLook.failed;const ULONGLONG failedAt=g_vehicleLook.failedAt;g_vehicleLook=VehicleLook102{};
    g_vehicleLook.failed=failed&&g_vehicleCamera92;g_vehicleLook.failedAt=failedAt; // a new vehicle gets a new chance
}
void VehicleLookForgetDirection(int seat){g_vehicleLookDirection=VehicleLookDirection107{};g_vehicleLookDirection.seat=seat;}
bool VehicleEpisode170(const fc3aim::VehicleSample170* sample,int legacyType,bool gap,bool camera,bool confirmedOnFoot175=false)
{
    auto& d=g_vehicleLookDirection;
    const bool valid=sample&&sample->identityValid;
    const bool different=valid&&(!d.identity170||d.pawn170!=sample->pawnId||d.vehicle170!=sample->vehicleId||
        (d.seatValid170&&sample->seatValid&&d.actualSeat170!=sample->actualSeat));
    const bool changed=confirmedOnFoot175||gap||!camera||different||(!valid&&legacyType>=0&&legacyType!=d.seat);
    // A pause/tick gap must not revoke the user's manual correction in the same vehicle.
    // Only exit or a confirmed owner/vehicle/seat/type change re-arms automatic look.
    // Camera eligibility is heuristic; missing ownership evidence must preserve manual
    // control. A fresh native on-foot sample or confirmed identity change can reset it.
    const bool keepManual175=d.manualControl175&&!confirmedOnFoot175&&!(different&&d.identity170)&&
        !(!valid&&legacyType>=0&&legacyType!=d.seat);
    if(changed&&!keepManual175)VehicleLookForgetDirection(legacyType);
    if(valid&&!confirmedOnFoot175){d.identity170=true;d.pawn170=sample->pawnId;d.vehicle170=sample->vehicleId;
        if(sample->seatValid){d.seatValid170=true;d.actualSeat170=sample->actualSeat;}d.seat=legacyType;}
    return changed;
}
void VehicleLookLearned(double turned,const char* how)
{
    Log("[VEHICLE LOOK] look stick direction learned: %s (seat %d, camera turned %.2f rad the pushed way %s)",
        g_vehicleLookDirection.sign<0?"reversed":"normal",g_vehicleLookDirection.seat,turned,how);
}
// The plain camera (no anchoring): what candidate 98 showed in a vehicle.
void VehicleLookRawView(){g_aimActive=false;g_bodyValid=false;}
#include "glider_look257.inl"
bool NativeVehicleActual279(const CameraSummary& camera)
{
    float body[9];UINT serial=0;
    if(!g_nativeVehicleCamera279||!fc3aim::VehicleCameraBasis279(camera.right,camera.up,camera.forward,body,serial)){
        g_aimActive=g_bodyValid=false;return false;
    }
    const float* observed[3]={camera.right,camera.up,camera.forward};
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_bodyAxes[i][j]=body[i*3+j];
    for(int i=0;i<3;++i)for(int j=0;j<3;++j){g_aimQ[i][j]=0;for(int k=0;k<3;++k)g_aimQ[i][j]+=observed[i][k]*g_bodyAxes[j][k];}
    g_bodyYaw=std::atan2(g_bodyAxes[2][1],g_bodyAxes[2][0]);g_rightSign=VisibilityCameraSign68(camera.right,camera.up,camera.forward);g_bodyValid=false;g_aimActive=true;
    static ULONGLONG next=0;const auto now=GetTickCount64();if(now>=next){next=now+1000;Log("[VEHICLE279] draw camera matched native serial=%u forward=%.6f/%.6f/%.6f",serial,camera.forward[0],camera.forward[1],camera.forward[2]);}
    return true;
}
bool TickVehicleLook102(bool headset)
{
    const bool renderable=headset&&g_headTracking&&g_enabled&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive;
    const bool eligible=renderable&&!MotionInteractionEnabled()&&!g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&
        !fc3pad::NativeMenuOpen()&&!fc3pad::WeaponSelectionRequested();
    const ULONGLONG now=GetTickCount64();
    fc3aim::VehicleSample170 native170;const bool hasNative170=fc3aim::GetVehicleSample170(native170);
    // A new vehicle episode: no tick for a second (on foot with a weapon this tick never runs) or
    // another seat. Run 6764 entered the jetski with the car's heading and push still active.
    {
        bool inVehicle=false;int seat=-1;
        const bool stateKnown175=fc3aim::VehicleState(inVehicle,seat);
        const int seatKey=stateKnown175&&inVehicle?seat:-1;
        const bool onFoot175=stateKnown175&&!inVehicle&&!hasNative170;
        const bool gap=g_vehicleLookLastTick&&now-g_vehicleLookLastTick>1000;
        const bool learned170=g_vehicleLookDirection.learned;
        if(VehicleEpisode170(hasNative170?&native170:nullptr,seatKey,gap,g_vehicleCamera92,onFoot175))
        {
            if(g_vehicleLook.active&&g_vehicleCamera92)VehicleLookStop(gap?"not ticked for a second: new vehicle episode":"vehicle seat changed");
            g_vehicleLook.failed=false; // a new vehicle gets a new chance
            if(learned170)Log("[VEHICLE LOOK] direction forgotten (native vehicle/seat episode or legacy type %d)",seatKey);
        }
        g_vehicleLookLastTick=now;
    }
    if(!g_vehicleLookEnabled||!eligible||!g_vehicleCamera92||!fc3pad::Latest().active)
    {
        fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;
        if(!g_vehicleCamera92)g_gliderLook257.active=false;
        if(g_vehicleLook.active)VehicleLookStop(!g_vehicleCamera92?"vehicle camera released":"not eligible");
        else if(!g_vehicleCamera92)g_vehicleLook.failed=false;
        return false;
    }
    if(fc3aim::SetVehicleCamera279(true,g_eyeViewsBody[0].view,g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward)){
        g_nativeVehicleCamera279=true;g_vehicleLook.active=false;g_gliderLook257.active=false;
        fc3pad::SetLookOverride(false,0,0);NativeVehicleActual279(g_frameCamera);return true;
    }
    g_nativeVehicleCamera279=false;
    auto& v=g_vehicleLook;const auto& camera=g_frameCamera;auto& dir=g_vehicleLookDirection;
    const double camYaw=std::atan2(double(camera.forward[1]),double(camera.forward[0]));
    if(!std::isfinite(camYaw))return false;
    if(hasNative170&&TickGliderLook257(native170,camera,now))return true;
    g_gliderLook257.active=false;
    if(!v.active)
    {
        v.active=true;v.headingValid=false;v.frames=0;v.lastCamYaw=camYaw;v.stableSince=now;
        std::memcpy(v.lastPos,camera.position,sizeof(v.lastPos));v.lastPosMs=now;
        Log("[VEHICLE LOOK] on: the rendered camera ignores the native look (vehicle); waiting for the recentred camera%s",v.failed?" (stick released earlier in this vehicle)":"");
    }
    ++v.frames;
    if(v.failed&&now-v.failedAt>5000){v.failed=false;Log("[VEHICLE LOOK] retrying the look stick");}
    if(std::fabs(Wrap102(camYaw-v.lastCamYaw))>.01)v.stableSince=now;
    v.lastCamYaw=camYaw;
    float px=0,py=0;fc3pad::LookStick(px,py);
    const float* raw=g_eyeViewsBody[0].view;
    const double headYaw=std::atan2(double(raw[8]),double(raw[10])); // positive = right
    static ULONGLONG sampleAt170=0;static UINT samples170=0;
    if(hasNative170&&now-sampleAt170>=500&&samples170<256){sampleAt170=now;++samples170;
        Log("[VEHICLE170] frame=%ld now=%llu sample=%llu pawn=%016llX vehicle=%016llX type=%d seat=%d seatValid=%d basisValid=%d basis=%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f position=%.5f/%.5f/%.5f camera=%.6f/%.6f/%.6f headYaw=%.6f heading=%.6f headingValid=%d stick=%.4f/%.4f",
        g_frame,now,native170.ms,native170.pawnId,native170.vehicleId,native170.type,native170.actualSeat,native170.seatValid,native170.basisValid,
        native170.basis[0][0],native170.basis[0][1],native170.basis[0][2],native170.basis[1][0],native170.basis[1][1],native170.basis[1][2],native170.basis[2][0],native170.basis[2][1],native170.basis[2][2],native170.position[0],native170.position[1],native170.position[2],camera.forward[0],camera.forward[1],camera.forward[2],headYaw,v.heading,v.headingValid,px,py);
        Log("[VEHICLE173] frame=%ld sample=%llu pawn=%016llX vehicle=%016llX pawnBasisValid=%d lookValid=%d pawnBasis=%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f/%.6f localLook=%.6f/%.6f/%.6f worldLook=%.6f/%.6f/%.6f",
            g_frame,native170.ms,native170.pawnId,native170.vehicleId,native170.pawnBasisValid173,native170.lookValid173,
            native170.pawnBasis173[0][0],native170.pawnBasis173[0][1],native170.pawnBasis173[0][2],native170.pawnBasis173[1][0],native170.pawnBasis173[1][1],native170.pawnBasis173[1][2],native170.pawnBasis173[2][0],native170.pawnBasis173[2][1],native170.pawnBasis173[2][2],
            native170.localLook173[0],native170.localLook173[1],native170.localLook173[2],native170.worldLook173[0],native170.worldLook173[1],native170.worldLook173[2]);
    }
    // 175: PID72032 repeatedly corrected the view, then the 800 ms timeout re-enabled
    // a synthetic stick (0.69..1.00) using the same non-neutral tracking yaw. Do not
    // seize camera ownership again after the user has corrected it this episode.
    if(std::fabs(px)>.2f){
        if(!dir.manualControl175)Log("[VEHICLE LOOK] manual correction: automatic look released for this vehicle episode");
        dir.manualControl175=true;v.headingValid=false;
    }
    if(dir.manualControl175){
        fc3pad::SetLookOverride(false,0,0);v.stickX=0;v.pushSince=0;v.pushDirection=0;v.holding=false;v.headingValid=false;
        // Manual look is not a response to the automatic stick. Discard the interrupted
        // 300 ms measurement so the user's turn cannot teach a false/reversed direction.
        // The completed per-episode direction in dir remains intact.
        v.learnSince=0;v.learnDirection=0;v.learnCamYaw=camYaw;
        VehicleLookRawView();return true;
    }
    // 2. No heading yet: leave the stick idle; the game recentres the camera on the vehicle.
    if(!v.headingValid)
    {
        fc3pad::SetLookOverride(true,0,py);v.stickX=0;VehicleLookRawView();
        if(now-v.stableSince>=400)
        {
            v.heading=camYaw;v.headingValid=true;v.travelValid=false;v.lastFrameMs=now;
            std::memcpy(v.lastPos,camera.position,sizeof(v.lastPos));v.lastPosMs=now;
            Log("[VEHICLE LOOK] heading %.2f from the recentred camera; the look stick now follows the head",camYaw);
        }
        return true;
    }
    // 3. The heading follows the vehicle, every frame. With the stick at rest and the camera near
    //    the heading the game keeps the camera on the vehicle, so the heading IS the camera: a
    //    turning car turns the view at once (candidate 103 moved it in 100 ms steps of 30 %, the
    //    judder seen while steering). While the stick is pushed, the heading follows the
    //    travel direction (measured over 250 ms, reversing or sliding more than 0.5 rad off ignored)
    //    with a 0.3 s time constant.
    const double frameDt=(std::min)(.1,double(now-v.lastFrameMs)/1000.0);v.lastFrameMs=now;
    {
        const double dx=double(camera.position[0])-v.lastPos[0],dy=double(camera.position[1])-v.lastPos[1];
        const double dt=double(now-v.lastPosMs)/1000.0;
        if(dt>=.25)
        {
            const double speed=std::hypot(dx,dy)/dt;
            v.travelValid=speed>2.0&&dt<1.0;if(v.travelValid)v.travelYaw=std::atan2(dy,dx);
            std::memcpy(v.lastPos,camera.position,sizeof(v.lastPos));v.lastPosMs=now;
        }
    }
    if(std::fabs(v.stickX)<.01f&&std::fabs(Wrap102(camYaw-v.heading))<.2)v.heading=camYaw;
    else if(v.travelValid){const double d=Wrap102(v.travelYaw-v.heading);if(std::fabs(d)<.5)v.heading=Wrap102(v.heading+d*(std::min)(1.0,frameDt/.3));}
    const bool headForward=std::fabs(headYaw)<.12;
    // 4. Drive the look stick toward the head.
    const double sign=VisibilityCameraSign68(camera.right,camera.up,camera.forward); // +1: a right turn lowers the world yaw
    const double desired=Wrap102(v.heading-sign*headYaw);
    const double error=sign*Wrap102(camYaw-desired); // positive: the camera must turn right
    // Hysteresis (candidate 105): the smallest stick the game accepts (0.25) turns the camera
    // about 0.5 rad/s, far too strong for a 0.03 rad error, and with the stick at rest the game
    // pulls the camera back to centre, so run 08:29 shook: the stick flipped sign every status
    // line. Now the stick engages above 0.06 rad of error and lets go below 0.02, so the camera
    // settles within a degree of the head and stays there.
    float x=0;
    const double absError=std::isfinite(error)?std::fabs(error):0;
    if(v.failed||!std::isfinite(error)||(headForward&&absError<.1))v.holding=false;
    else if(v.holding?absError<.02:absError<.06)v.holding=false;
    else v.holding=true;
    if(v.holding)
    {
        const double c=g_vehicleLookGain*error;
        x=float((c<0?-1:1)*(.25+.75*(std::min)(1.0,std::fabs(c)))); // past the game's stick dead zone
    }
    v.stickX=x; // the intended direction; the game gets it times the learned sign
    fc3pad::SetLookOverride(true,dir.sign*x,py); // pitch stays the user's
    const int direction=std::fabs(x)>.6f?(x>0?1:-1):0;
    // 5a. Learn the stick direction from the first hard push (candidate 107): over 300 ms a camera
    //     that turned more than 0.1 rad the pushed way is normal, more than 0.1 rad the other way
    //     reversed (the sign flips at once); less than that is measured again.
    if(!dir.learned)
    {
        if(direction!=0&&direction==v.learnDirection&&v.learnSince)
        {
            if(now-v.learnSince>=300)
            {
                const double turned=-sign*direction*Wrap102(camYaw-v.learnCamYaw);
                if(std::fabs(turned)>.1)
                {
                    if(turned<0){dir.sign=-dir.sign;++dir.flips;v.pushSince=0;v.pushDirection=0;}
                    dir.learned=true;VehicleLookLearned(turned,"over 300 ms");
                }
                v.learnSince=now;v.learnCamYaw=camYaw;
            }
        }
        else{v.learnDirection=direction;v.learnSince=direction?now:0;v.learnCamYaw=camYaw;}
    }
    // 5b. A camera that does not TURN under 1.5 s of hard pushing in one direction is not this
    //    vehicle's free look: release the stick for 5 s, then retry. Judged by how far the camera
    //    turned in the pushed direction, never by the remaining error: a head that keeps moving
    //    keeps the error large while the camera follows (run 08:05: a false release here ended
    //    the head look for the rest of the drive). A camera that clearly turned the OTHER way
    //    flips the learned direction instead (at most twice per seat, then the release applies).
    if(direction!=0&&direction==v.pushDirection&&v.pushSince)
    {
        if(now-v.pushSince>1500)
        {
            const double turned=-sign*direction*Wrap102(camYaw-v.pushCamYaw); // positive: turned the pushed way
            if(turned<-.15&&dir.flips<2){dir.sign=-dir.sign;++dir.flips;dir.learned=true;VehicleLookLearned(turned,"over 1.5 s");}
            else if(turned<.15){v.failed=true;v.failedAt=now;v.stickX=0;fc3pad::SetLookOverride(true,0,py);Log("[VEHICLE LOOK] camera turned only %.2f rad under 1.5 s of hard pushing: stick released, retry in 5 s",turned);}
            v.pushSince=now;v.pushCamYaw=camYaw;
        }
    }
    else{v.pushDirection=direction;v.pushSince=direction?now:0;v.pushCamYaw=camYaw;}
    // 6. The eye views anchored to the vehicle heading (yaw only): the body basis is the camera's
    //    own axes turned about world up by (heading - camYaw), so Q removes only the look-stick turn.
    const double d=Wrap102(v.heading-camYaw),cd=std::cos(d),sd=std::sin(d);
    const float* observed[3]={camera.right,camera.up,camera.forward};
    for(int i=0;i<3;++i){const double x0=observed[i][0],y0=observed[i][1];g_bodyAxes[i][0]=x0*cd-y0*sd;g_bodyAxes[i][1]=x0*sd+y0*cd;g_bodyAxes[i][2]=observed[i][2];}
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_aimQ[i][j]=observed[i][0]*g_bodyAxes[j][0]+observed[i][1]*g_bodyAxes[j][1]+observed[i][2]*g_bodyAxes[j][2];
    g_bodyYaw=v.heading;g_rightSign=sign;g_bodyValid=false;g_aimActive=true;
    if(now-v.lastStatus>=2000){v.lastStatus=now;Log("[VEHICLE LOOK] frame%ld heading %.2f camera %.2f head %.2f stick %.2f%s error %.2f",g_frame,v.heading,camYaw,headYaw,x,dir.sign<0?" (reversed)":"",error);}
    return true;
}
