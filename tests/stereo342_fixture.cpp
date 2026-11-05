// Real shipping mechanics, offline-only setup. Not linked into the DLL.
#include "../src/stereo.cpp"
unsigned ExerciseShotgunSupport342(int profile,bool pump){
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=++g_frame;g_motionGun=true;g_weaponMotionGuard=false;
 g_handRightValid=g_handLeftValid=g_eyeViewsSet=true;g_inGameplay=g_enabled=true;g_pistolMenu.open=false;
 fc3aim::SetNativeActionForTest128(0);fc3aim::SetVehicleStateForTest(0);
 fc3pad::SetGameplayControls(true);fc3pad::SetGripContext(true,true);fc3pad::SetMagazineHandling(false,false);fc3pad::SetLeverHandling(false);fc3pad::SetLeverXOwned(false);
 g_reload={};g_manualReload={};g_reloadAlignment=false;g_pistolReload={};g_twoHand=false;
 g_armJoints[0].valid=g_armJoints[1].valid=true;g_chambered=true;g_pumpStage=0;
 for(int h=0;h<2;++h){for(int k=0;k<16;++k)g_handBody[h][k]=k%5==0?1.f:0.f;}
 const auto c=fc3pad::Latest();unsigned result=ShotgunSupportAccepted342(c)?1:0;
 if(profile==34){
  g_spas={};g_spas.simple=false;g_spas.clip=-1;g_spas.needPump=pump;g_spas.frameBody=HandIdentity();g_spas.frameFrame=g_frame;g_spas.leftFrame=-100000;
  TickSpas(true);if(g_spas.grab)result|=2;if(!g_spas.carrying&&!g_spas.pending)result|=4;
  UpdateTwoHand();if(g_twoHand)result|=8;
 }else{
  // Deliberately unreachable legacy box: an already accepted XR support
  // squeeze must not need a second narrow hit to enter the two-hand hold.
  g_m133SupportLoaded56=true;g_m133Support56={};g_m133Support56.positioned=true;g_m133Support56.position[0]=50;
  g_reloadContact.port=true;g_reloadContact.portFrame=g_frame;g_reloadContact.portParent=HandIdentity();
  UpdateTwoHand();if(g_twoHand)result|=2|8;if(!g_reload.held&&!g_reload.pending)result|=4;
 }
 return result;
}

unsigned BusyPolicy345(int profile,bool support,bool pumping,bool pending){
 g_weaponMechanicProfile=profile;g_weaponMechanicFrame=g_frame;g_twoHand=support;g_reload={};g_manualReload={};g_reloadAlignment=false;g_pistolReload={};g_pistolReload.enabled=true;g_spas={};g_spas.clip=-1;g_spas.simple=false;g_spas.grab=support;g_spas.needPump=pumping;g_spas.pending=pending;g_spas.pump=pumping?.3:0;g_pumpStage=0;g_lever={};g_lever.clip=-1;g_r700={};g_r700.clip=-1;g_flamer={};g_flamerClip={};g_gm94={};g_gm94Clip={};g_rpg={};g_revolver={};g_revolver.clip=-1;g_flare={};g_bow={};
 return (BodyBusy285()?1u:0u)|(BodyBusy285(false)?2u:0u)|(ShotgunBusy318()?4u:0u)|(GrabBusy333()?8u:0u);
}
