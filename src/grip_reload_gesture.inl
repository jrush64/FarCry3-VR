void TickGripReload(bool handling,const fc3pad::Controllers& c)
{
    fc3pad::SetShellPickupBinding(handling);
    const bool pickup=fc3pad::TakeShellPickup();
    if(!handling||!g_handLeftValid||!g_handRightValid||
       (g_reload.ready&&!ReloadContext(g_reload.state,g_reload.wrapper,g_reload.weapon)))
    {
        g_reload.held=false;g_reload.mode=0;g_gripReload.pending=0;g_gripReload.contactFrames=0;
        fc3pad::CancelReloadRequest();return; // an already started native animation owns its finish
    }
    if(g_gripReload.pending&&GetTickCount64()>g_gripReload.deadline)
    {
        g_gripReload.pending=0;g_reload.held=false;g_reload.mode=0;g_gripReload.failed=1;
        fc3pad::CancelReloadRequest();Log("[GRIP RELOAD] native animation did not start; no manual ammo call or retry");
    }
    if(g_gripReload.finishing&&GetTickCount64()>g_gripReload.deadline&&!g_gripReload.failed)
    {g_gripReload.failed=1;Log("[GRIP RELOAD] native animation finish delayed; single-shell ownership retained");}
    if(g_gripReload.pending||g_gripReload.finishing)return;
    if(pickup)
    {
        if(GripReloadContext()&&ReloadShellReady()&&!g_reload.held&&!ReloadPoseActive()&&!g_gripReload.failed&&
           (!g_reload.ready||(ReloadRead(g_reload.weapon,0xcc)<7&&GripReloadReserve()>0)))
        {
            g_reload.held=true;g_reload.mode=1;g_twoHand=false;
            g_gripReload.contactFrames=0;g_gripReload.contactFrame=-1;
            fc3xr::Buzz(1);Log("[GRIP RELOAD] grip+X picked up shell; no native playback or ammo yet");
        }
        else Log("[GRIP RELOAD] pickup unavailable ready=%ld visual=%d native=%d",g_reload.ready,ReloadShellReady(),ReloadPoseActive());
    }
    if(!g_reload.held)return;
    if(c.leftGrip<.35f||ReloadPoseActive())
    {g_reload.held=false;g_reload.mode=0;g_gripReload.contactFrames=0;Log("[GRIP RELOAD] shell dropped before handoff; no ammo");return;}
    double distance=0;
    const bool fresh=ReloadContactDistance(distance)&&g_reloadContact.handFrame==g_frame&&g_reloadContact.portFrame==g_frame;
    if(!fresh||distance>.18)
    {g_gripReload.contactFrames=0;g_gripReload.contactFrame=-1;return;}
    if(g_gripReload.contactFrame==g_frame)return;
    g_gripReload.contactFrame=g_frame;
    if(++g_gripReload.contactFrames>=3)
    {
        g_gripReload.pending=1;g_gripReload.deadline=GetTickCount64()+3000;InterlockedIncrement(&g_reload.requests);
        fc3pad::RequestReload();fc3xr::Buzz(1);
        Log("[GRIP RELOAD] receiver approach %.3f m requests one native shell animation; awaiting game start",distance);
    }
}
