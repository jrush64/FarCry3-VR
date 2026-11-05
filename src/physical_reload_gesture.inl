// Waist pickup uses the controller palm; insertion uses the actual rendered shell and receiver.
V3 ReloadPalm(const float* pose)
{
    V3 p=ControllerWrist(pose,1);
    return {p.x+pose[2]*.055,p.y+pose[6]*.055,p.z+pose[10]*.055};
}
void TickPhysicalReload(bool gameplay)
{
    static ULONGLONG attempted=0;const auto now=GetTickCount64();
    if(!g_reload.installed&&now-attempted>=3000){attempted=now;ReloadInstall();}
    InstallReloadSignalGate();
    const auto c=fc3pad::AmmoControls();const bool grip=c.leftGrip>.7f;
    if(PistolHandling())
    {
        // A shotgun's inactive cleanup must not cancel the pistol's generated X.
        if(g_manualReload.pending){InterlockedExchange(&g_manualReload.pending,0);fc3pad::CancelReloadRequest();}
        if(g_reload.held)ManualDrop();
        fc3pad::SetShotgunHandling(false);fc3pad::SetOneShellX(false);fc3pad::SetManualReload(false,false);
        return;
    }
    const bool shotgunHandling=!KnownNonPumpModel()&&MotionInteractionEnabled()&&gameplay&&c.active&&g_frame-g_reload.shotgunFrame<=2;
    fc3pad::SetShotgunHandling(shotgunHandling);
    fc3pad::SetOneShellX(g_singleShellEnabled&&g_reload.installed&&shotgunHandling);
    if(g_manualReloadEnabled){TickManualReload(shotgunHandling,c);return;}
    fc3pad::SetManualReload(false,false);
    fc3xr::SetReloadMarkers(nullptr,nullptr,0,0);
    if(!g_physicalReloadEnabled)
    {
        g_reload.held=false;g_reload.grip=grip;
        InterlockedExchange(&g_reload.mode,0);InterlockedExchange(&g_reload.credit,0);InterlockedExchange(&g_reload.pending,0);
        fc3pad::CancelReloadRequest();
        return;
    }
    if(!gameplay||!c.active||!g_handLeftValid||!g_handRightValid||g_frame-g_reload.shotgunFrame>90||
       (g_reload.ready&&!ReloadContext(g_reload.state,g_reload.wrapper,g_reload.weapon)))
    {
        g_reload.held=false;g_reload.grip=grip;InterlockedExchange(&g_reload.mode,0);InterlockedExchange(&g_reload.credit,0);InterlockedExchange(&g_reload.pending,0);
        return;
    }
    // Interaction/magazine release cannot cancel a physical shell lease.
    const V3 left=ReloadPalm(g_handBody[1]);
    if(g_reload.ready)fc3pad::SuppressLeftGrip(g_twoHand||g_reload.held||(left.y<-.40&&left.x<.15));
    if(g_reload.ready&&ReloadShellReady()&&!g_reload.failed&&!g_reload.held&&!g_reload.credit&&((grip&&!g_reload.grip&&!g_twoHand&&left.y<-.40&&left.x<.15)||
       (c.x&&fc3pad::AmmoPickupOverride(c)))&&ReloadRead(g_reload.weapon,0xcc)<7)
    {
        g_reload.held=true;g_reload.deadline=now+120000;InterlockedExchange(&g_reload.mode,1);
        fc3xr::Buzz(1);Log("[PHYSICAL RELOAD] shell picked up at belt; no reload input sent");
    }
    if(g_reload.held&&!grip&&c.leftGrip<.35f)
    {
        g_reload.held=false;
        double distance=0;
        if(ReloadContactDistance(distance)&&distance<.15)
        {g_reload.deadline=now+120000;InterlockedExchange(&g_reload.credit,1);fc3pad::RequestReload();InterlockedIncrement(&g_reload.requests);Log("[PHYSICAL RELOAD] one insertion queued, port distance %.3f m",distance);}
        else Log("[PHYSICAL RELOAD] shell released outside receiver; no ammo credit");
    }
    g_reload.grip=grip;
}
void MoveReloadShell(float* bones,const Mat4& fromView)
{
    Mat4 desired={};
    if(!CapturedShellTarget(desired))
    {for(int b=0;b<48;++b)bones[b*12+7]=-100;return;}
    const Mat4 target=Multiply(fromView,desired);Mat4 inverse={};
    if(!Invert(BoneMatrix(bones,0),inverse))return;
    MoveBones(bones,0,47,Multiply(target,inverse));
}

