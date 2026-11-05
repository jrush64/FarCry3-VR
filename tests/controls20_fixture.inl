
void TestControls20()
{
 fc3pad::Controllers c={};c.active=true;
 fc3pad::SetPistolCalibration(false);g_menuExitWait=false;fc3pad::SetManualReload(false,false);fc3pad::SetOneShellX(false);fc3pad::EndOneShellX();
 fc3pad::SetMagazineHandling(false);fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);fc3pad::SetXOwned(false);fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);
 fc3pad::Publish(c);fc3pad::SetDedicatedInteractions(true);fc3pad::SetGameplayControls(true);fc3pad::Publish(c);
 // B must hold native interaction even with every weapon-local X owner set.
 fc3pad::SetShotgunHandling(true);fc3pad::SetManualReload(true,false);fc3pad::SetOneShellX(true);fc3pad::BeginOneShellX();
 fc3pad::SetMagazineHandling(true,true);fc3pad::SetLeverHandling(true);fc3pad::SetLeverXOwned(true);fc3pad::SetXOwned(true);fc3pad::SetMacheteContext(true);fc3pad::SetMacheteHandling(true);
 c.leftGrip=c.rightGrip=0;c.x=true;
 for(int i=0;i<180;++i){fc3pad::Publish(c);const auto p=FromControllers(g_state);Check((p.buttons&kX)&&!(p.buttons&kB)&&fc3pad::InteractionOnly(),"B interaction survives every weapon X owner without crouching");Check(!fc3pad::AmmoControls().x,"ordinary X never manufactures physical-reload X");}
 c.x=false;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"X interaction releases cleanly");c.leftGrip=1;
 // Explicit wheel wins over physical ownership and the visible machete.
 c.y=true;c.rightX=.7f;c.rightY=-.8f;fc3pad::Publish(c);auto p=FromControllers(g_state);fc3pad::ApplyLookOverride(p);
 Check((p.buttons&kLeftShoulder)&&!(p.buttons&(kY|kB))&&p.thumbRY==Axis(-.8f)&&fc3pad::WeaponSelectionRequested(),"wheel owns selection despite weapon locks, no crouch or heal");
 fc3pad::SetGameplayControls(false);Check(fc3pad::DedicatedInteractionLayout()&&(FromControllers(g_state).buttons&kLeftShoulder),"inventory frozen-world frame cannot drop held wheel ownership");
 fc3pad::RequestReload();Check(g_reloadPulseUntil==0,"weapon selection cannot inherit insertion pulse");
 c.leftGrip=0;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&(kY|kLeftShoulder|kB)),"release grip before Y cannot switch, heal or crouch");
 c.y=false;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kB),"held-down selection stick must recenter before crouch");
 c.rightY=0;fc3pad::Publish(c);c.y=true;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kY,"fresh Y can switch away from a held machete");
 c.y=false;fc3pad::Publish(c);fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);fc3pad::SetMagazineHandling(false);fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);fc3pad::SetXOwned(false);fc3pad::SetManualReload(false,false);fc3pad::SetShotgunHandling(false);fc3pad::SetOneShellX(false);fc3pad::EndOneShellX();
 c={};c.active=true;fc3pad::Publish(c);
 for(int i=0;i<100;++i){
  c.leftGrip=1;c.y=false;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kLeftShoulder),"support grip alone never opens weapon wheel");
  c.y=true;fc3pad::Publish(c);c.y=false;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kLeftShoulder,"tap Y while holding grip keeps wheel open");
  c.leftGrip=0;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kLeftShoulder),"grip release selects and closes every cycle");
 }
 ObserveNativeStance(0);c.rightX=0;c.rightY=0;fc3pad::Publish(c);c.rightY=-.85f;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kB,"stick down crouches");
 c.rightY=-.55f;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kB,"crouch threshold hysteresis avoids repeat edges");
 c.rightY=0;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kB),"crouch releases at neutral");
 c.rightX=1;c.rightY=-.8f;fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kB),"dominant horizontal turn cannot crouch");
 c={};c.active=true;fc3pad::Publish(c);fc3pad::SetGameplayControls(false);c.b=true;fc3pad::Publish(c);Check((FromControllers(g_state).buttons&kB)&&!(FromControllers(g_state).buttons&kX),"native menu B stays Back");
 fc3pad::SetGameplayControls(true);fc3pad::Publish(c);Check(!(FromControllers(g_state).buttons&kX),"held menu Back cannot leak as gameplay interaction");c.b=false;fc3pad::Publish(c);c.x=true;fc3pad::Publish(c);Check(FromControllers(g_state).buttons&kX,"fresh X after menu works");
 c.active=false;c.x=c.y=c.a=true;c.leftGrip=c.rightTrigger=1;fc3pad::Publish(c);p=FromControllers(g_state);Check(!p.buttons&&!p.rightTrigger&&!fc3pad::Latest().x&&!fc3pad::FireHeld(),"inactive controllers drop stale buttons and firing");
 fc3pad::GuardMenuExit();c={};c.active=true;c.rightStickClick=true;fc3pad::Publish(c);Check(fc3pad::GameplayInputBlocked(),"held stick click cannot escape Insert guard");c.rightStickClick=false;fc3pad::Publish(c);Check(!fc3pad::GameplayInputBlocked(),"neutral controls leave Insert guard");
 fc3pad::SetGameplayControls(false);fc3pad::SetLookOverride(false,0,0);
 puts("PASS: controls20 dedicated B interaction, raw reload isolation, weapon ownership, both wheel release orders, 100 selection cycles, crouch hysteresis, menu transitions and stale tracking cleanup");
}

