void TestMacheteMelee(ID3D11Device* device)
{
    MacheteStroke s;ULONGLONG now=10000;V3 pos={0,0,0};
    auto sample=[&](double x,double y,double z,UINT dt=16){now+=dt;pos={x,y,z};return StepMacheteStroke(s,pos,now);};
    for(int i=0;i<30;++i)Check(!sample(0,0,0),"settling cannot attack");
    Check(s.armed,"a settled hand arms the physical stroke");
    for(int i=0;i<100;++i)Check(!sample(.004*(i%2),0,0),"hand tremor cannot form a meaningful stroke");
    int attacks=0;for(int i=1;i<=25;++i)attacks+=sample(i*.03,0,0);
    Check(attacks==1,"one continuous fast cut requests exactly one attack");
    for(int i=0;i<50;++i)Check(!sample(.75+(i%2)*.03,0,0),"continuing fast motion cannot rearm");
    for(int i=0;i<15;++i)sample(.75,0,0);
    attacks=0;for(int i=1;i<=5;++i)attacks+=sample(.75,i*.03,0);
    Check(attacks==1,"a settled hand permits a second stroke in another direction");
    for(UINT interval:{11u,33u,50u})
    {
        s=MacheteStroke{};for(int i=0;i<30;++i)sample(0,0,0,interval);
        attacks=0;for(int i=1;i<=15;++i)attacks+=sample(0,0,i*interval*.0015,interval);
        Check(attacks==1,"physical stroke works at 90, 30 and 20 Hz without repeated attacks");
    }
    s=MacheteStroke{};for(int i=0;i<30;++i)sample(0,0,0);
    Check(!sample(1,0,0)&&!s.armed,"tracking teleport resets the detector without attacking");
    Check(!sample(1.02,0,0,200)&&!s.armed,"long frame or tracking gap cannot become an attack");
    Check(!sample(NAN,0,0)&&!s.seeded,"invalid pose resets the detector");
    s=MacheteStroke{};for(int i=0;i<30;++i)sample(0,0,0);
    Check(!StepMacheteStroke(s,{.1,0,0},now)&&!s.armed,"duplicate time does not infer an infinite velocity");
    s=MacheteStroke{};for(int i=0;i<30;++i)sample(0,0,0);
    for(int i=1;i<200;++i)Check(!sample(i*.005,0,0),"slow repositioning never attacks even over a long distance");
    s=MacheteStroke{};for(int i=0;i<3;++i)sample(0,0,0);
    attacks=0;for(int i=1;i<=4;++i)attacks+=sample(i*.04,0,0);
    sample(.16,0,0); // a single slow sample between cuts
    for(int i=1;i<=4;++i)attacks+=sample(.16,i*.04,0);
    Check(attacks==2,"two physical cuts re-arm on slowdown without requiring a 150 ms rest");

    ClearMacheteVisual();g_dev=device;g_macheteMelee=MacheteMeleeState{};g_macheteMelee.enabled=true;
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
    fc3pad::SetMacheteContext(false);Check(!fc3pad::RequestMacheteAttack(),"attack cannot escape gameplay/ownership scope");
    fc3pad::SetMacheteContext(true);Check(!fc3pad::RequestMacheteAttack(),"a firearm cannot issue the owned machete pulse");
    fc3pad::SetMacheteHandling(true);c.leftStickClick=c.rightStickClick=true;fc3pad::Publish(c);
    Check(!fc3pad::RequestMacheteAttack(),"calibration cannot request a melee attack");
    c={};c.active=true;c.rightTrigger=1;c.x=c.y=true;c.leftGrip=c.rightGrip=1;fc3pad::Publish(c);
    Check(fc3pad::RequestMacheteAttack(),"owned attack uses the established native R3 path");
    fc3pad::CancelMacheteSource();
    Check((fc3pad::ButtonsOutput()&0x0080)&&!fc3pad::FireOutput()&&!fc3pad::AimOutput()&&!fc3pad::ReloadOutput(),"canceling cached source cannot cancel the attack or leak firearm input");
    Check(fc3pad::Latest().rightTrigger==1&&fc3pad::Latest().x,"melee conversion retains raw controls");
    c.leftStickClick=true;fc3pad::Publish(c);Check(!(fc3pad::ButtonsOutput()&0x0080),"calibration/sprint click cannot interrupt a pulse into a new R3 edge");
    fc3pad::SetMacheteHandling(false);c.leftStickClick=false;fc3pad::Publish(c);
    Check(!(fc3pad::ButtonsOutput()&0x0080)&&!fc3pad::FireOutput(),"stow cancels melee and retains trigger-release firearm protection");
    c.rightTrigger=0;fc3pad::Publish(c);

    g_enabled=g_motionGun=g_eyeViewsSet=g_handRightValid=true;g_frame=52000;
    for(auto& p:g_machete.parts)p.ready=true; // tick-only fixture: no fake COM pointer
    // Readiness additionally needs a real deferred context, supplied by the WARP device.
    Check(SUCCEEDED(g_dev->CreateDeferredContext(0,&g_machete.deferred)),"melee fixture deferred context");
    g_machete.held=true;g_machete.handFrame=g_frame;
    now=GetTickCount64();g_machete.started=now-2000;
    fc3pad::SetMacheteContext(true);fc3pad::SetMacheteHandling(true);
    const LONG shot=g_lastShotFrame;const int pump=g_pumpStage;const bool chamber=g_chambered;
    // Reproduce the live failure: the engine draws the held blade every frame.
    // That evidence must not be treated as an attack cooldown.
    auto tick=[&](){g_macheteMelee.lastNative=now;fc3pad::Publish(c);TickMacheteMelee(c,now);};
    g_worldScale=2;for(int i=0;i<16;++i)g_handBody[0][i]=i%5==0?1.f:0.f;
    c={};c.active=true;for(int i=0;i<15;++i){now+=16;tick();}
    Check(g_macheteMelee.stroke.armed,"production motion samples settle and arm");
    const auto previousContact116=g_macheteBladeContact;g_macheteBladeContact.enabled=true;g_macheteBladeContact.installed=true;g_macheteBladeContact.strategy=1;
    Check(MacheteBladeContactActive(),"fixture reproduces direct-contact ownership that previously suppressed native attacks");
    for(int i=1;i<=5;++i){now+=16;g_handBody[0][3]=i*.06f;tick();}
    Check(g_macheteMelee.sequence==1&&g_macheteMelee.pending&&(fc3pad::ButtonsOutput()&0x0080),"physical stroke requests native attack even with a blade draw every frame");
    g_macheteBladeContact=previousContact116;
    ObserveNativeMachete(now+16);Check(!g_macheteMelee.pending&&g_macheteMelee.observed==1,"native blade callback marks swipe evidence separately from request");
    const auto observed=g_macheteMelee.observed;ObserveNativeMachete(now+20);
    Check(g_macheteMelee.observed==observed,"multiple submesh frames cannot count as repeated attacks");
    now+=100;c.rightTrigger=1;tick();Check(g_macheteMelee.sequence==1,"trigger in cooldown is consumed without queuing another attack");
    now+=1000;tick();Check(g_macheteMelee.sequence==1,"holding the trigger after cooldown cannot attack automatically");
    c.rightTrigger=0;now+=16;tick();c.rightTrigger=1;now+=16;tick();
    Check(g_macheteMelee.sequence==2&&g_macheteMelee.pending,"fresh right trigger provides an independent native attack comparison");
    now+=1900;tick();Check(!g_macheteMelee.pending&&g_macheteMelee.observed==1&&g_macheteMelee.sequence==2,"unobserved swipe expires without auto retry or hit claim");
    c.rightTrigger=0;now+=16;tick();g_machete.handFrame=g_frame-2;c.rightTrigger=1;now+=16;tick();
    Check(g_macheteMelee.sequence==2,"stale or unavailable rendered hand refuses an invisible attack");
    g_machete.handFrame=g_frame;now+=1000;tick();Check(g_macheteMelee.sequence==2,"held trigger blocked on stale hand cannot attack on recovery");
    c.rightTrigger=0;now+=16;tick();
    for(int i=0;i<15;++i){now+=16;tick();}
    g_worldScale=1;now+=16;g_handBody[0][3]=.15f;tick();
    Check(!g_macheteMelee.stroke.armed&&g_macheteMelee.sequence==2,"scale changes reset velocity rather than synthesize a swing");
    ++g_armPoseEpoch;now+=16;tick();Check(!g_macheteMelee.stroke.armed,"calibration resets the stroke sample");
    Check(g_lastShotFrame==shot&&g_pumpStage==pump&&g_chambered==chamber,"native melee requests never change shotgun shot/chamber/pump state");

    wchar_t trace[MAX_PATH]={};wcscpy_s(trace,g_macheteMelee.path);StopMacheteMelee();
    HANDLE file=CreateFileW(trace,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(file!=INVALID_HANDLE_VALUE,"automatic melee probe is durable after stow");
    MacheteMeleeRecord r;DWORD bytes=0;UINT requests=0,responses=0,timeouts=0,stows=0;
    while(ReadFile(file,&r,sizeof(r),&bytes,nullptr)&&bytes)
    {
        Check(bytes==sizeof(r)&&r.magic==0x4D4D5031&&r.version==1&&r.bytes==sizeof(r),"saved melee trace records parse exactly");
        if(r.event==1){++requests;Check((r.flags&16)!=0,"requested event records actual converted native R3 output");}
        if(r.event==2){++responses;Check(r.sequence==1,"native response correlates to the correct swing");}
        if(r.event==3)++timeouts;if(r.event==4)++stows;
    }
    CloseHandle(file);Check(requests==2&&responses==1&&timeouts==1&&stows==1,"automatic probe preserves requests, observed swipe, ignored request and stow distinctly");
    DeleteFileW(trace);
    g_macheteMelee.enabled=false;g_macheteMelee.pending=true;g_macheteMelee.cooldown=now+220;
    Check(RequestMacheteMelee("physical swing",now+300),"missing native draw acknowledgement never blocks the next physical cut");
    Check(RequestMacheteMelee("physical swing",now+600),"consecutive cuts have no 900 ms cooldown or 1800 ms pending lock");
    g_macheteMelee=MacheteMeleeState{};ClearMacheteVisual();g_worldScale=1;g_dev=nullptr;
    c={};c.active=true;fc3pad::Publish(c);
    std::puts("PASS: physical machete stroke detection at 20/30/90 Hz; tremor, slow move, teleport, stale hand, calibration/scale and continuous-swing rejection; scoped one native R3 pulse, fresh trigger comparison, cooldown/no held-input retry, correlated durable native-swipe probe; firearm pump/chamber unchanged; actual damage requires headset target test");
}
