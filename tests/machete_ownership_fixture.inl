void TestMacheteOwnership(ID3D11Device* device)
{
    ClearMacheteVisual();g_dev=device;g_macheteMelee=MacheteMeleeState{};g_macheteMelee.enabled=true;
    g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};g_reloadAlignment=false;
    g_enabled=g_motionGun=g_eyeViewsSet=g_handRightValid=g_macheteEnabled=true;
    g_macheteHoldToDraw=false;fc3pad::SetMacheteHoldMode(false);g_weaponMotionGuard=true;g_visibleWeaponDrawsLastFrame=8;
    for(auto& p:g_machete.parts)p.ready=true;
    Check(SUCCEEDED(device->CreateDeferredContext(0,&g_machete.deferred)),"ownership deferred cache");
    fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);TickMachete(true);
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);
    c.rightGrip=0;c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);
    Check(MacheteHeld(),"explicit toggle acquires cached tool");
    Check(RequestMacheteMelee("physical swing",GetTickCount64()),"native swing requested once");
    const UINT seq=g_macheteMelee.sequence;g_visibleWeaponDrawsLastFrame=0;
    Check(MotionInteractionEnabled(),"native firearm disappearance cannot revoke held machete during swipe");
    TickMachete(MotionInteractionEnabled());Check(MacheteHeld()&&g_macheteMelee.pending&&!fc3pad::FireOutput(),"production gate/tick retains machete and attack while gun is hidden");
    ObserveNativeMachete(GetTickCount64());TickMachete(MotionInteractionEnabled());
    Check(MacheteHeld()&&!g_macheteMelee.pending&&g_macheteMelee.sequence==seq,"observed stock swipe cannot stow tool or issue another attack");
    g_visibleWeaponDrawsLastFrame=8;TickMachete(true);Check(MacheteHeld(),"native shotgun return stays hidden behind persistent machete");
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(!MacheteHeld()&&!MacheteMotionLease(),"explicit put-away immediately releases lease");
    c.rightGrip=0;c.rightStickClick=false;fc3pad::Publish(c);TickMachete(true);
    g_visibleWeaponDrawsLastFrame=0;c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(MotionInteractionEnabled());
    Check(!MacheteHeld()&&!MotionInteractionEnabled(),"safe area without native weapon cannot acquire a lease");
    c.rightGrip=0;c.rightStickClick=false;g_visibleWeaponDrawsLastFrame=8;fc3pad::Publish(c);TickMachete(true);
    c.rightGrip=1;c.rightStickClick=true;fc3pad::Publish(c);TickMachete(true);Check(MacheteHeld(),"explicit redraw with visible gun");
    c.rightGrip=0;c.rightStickClick=false;fc3pad::Publish(c);g_visibleWeaponDrawsLastFrame=0;g_machete.motionUntil=GetTickCount64()-1;
    // Candidate 108: the game draws no idle machete, so a held blade whose own replay is ready keeps motion
    // control for as long as it is held (reported: motion worked only for two seconds after each RT swipe).
    Check(MacheteReady()&&MacheteMotionLease()&&MotionInteractionEnabled(),"held blade with its replay ready keeps motion control without a native weapon draw");
    TickMachete(MotionInteractionEnabled());Check(MacheteHeld(),"held blade is not stowed while its own replay is visible");
    ClearMacheteVisual();
    TickMachete(MotionInteractionEnabled());Check(!MacheteHeld()&&!MotionInteractionEnabled(),"without a ready blade replay the bounded transition permission still expires");
    ClearMacheteVisual();g_dev=nullptr;g_weaponMotionGuard=false;c={};c.active=true;fc3pad::Publish(c);
    puts("PASS: native swing/firearm disappearance/observed swipe/shotgun return retains held machete; held blade with a ready replay keeps motion control; explicit stow, safe-area acquisition and expired transition guard without a replay");
}

