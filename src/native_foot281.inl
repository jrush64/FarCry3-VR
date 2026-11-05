// Common on-foot native view: swimming, dismounts and other weapon-free views.
// Selection is camera ownership, not a growing list of animation class names.
bool g_nativeFoot281=false;
bool NativeFootContext281(bool headset){
 return headset&&g_enabled&&g_headTracking&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&
  !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen()&&!fc3pad::WeaponSelectionRequested()&&
  (!fc3aim::NativeActionOwnsArms128()||fc3aim::NativeHandoff284())&&
  (g_visibleWeaponDrawsLastFrame==0||(g_nativeFoot281&&(!fc3aim::Ready()||fc3aim::NativeActionOwnsArms128())))&&!ThrowableOwns144()&&!ThrowableArmLease150()&&!MacheteMotionLease();
}
bool TickNativeFoot281(bool headset){
 if(NativeFootContext281(headset)&&fc3aim::NativeFootCamera281()){
  float body[9];const float* observed[3]={g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward};
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)body[i*3+j]=g_aimActive?float(g_bodyAxes[i][j]):observed[i][j];
  if(fc3aim::SetVehicleCamera279(true,g_eyeViewsBody[0].view,g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward,body,true)){
   if(!g_nativeFoot281)Log("[NATIVE281] weapon-free on-foot view acquired; native camera owns head tracking across dismount/swimming");
   g_nativeFoot281=g_nativeVehicleCamera279=true;g_vehicleCamera92=false;g_vehicleCameraLostAt92=0;
   fc3aim::EndVisibility68();fc3aim::Suspend();fc3pad::SetLookOverride(false,0,0);
   g_visibilityCamera68=g_visibilityHandoff68=false;NativeVehicleActual279(g_frameCamera);return true;
  }
 }
 if(g_nativeFoot281){
  g_nativeFoot281=false;fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;
  // Returning to an armed view keeps the final native reference; it must not
  // seed body yaw from a render camera that already contains the HMD turn.
  g_bodyValid=g_aimActive&&g_motionGun&&g_aimEnabled&&g_visibleWeaponDrawsLastFrame>0;
  QueryPerformanceCounter(&g_aimLast);Log("[NATIVE281] native view released; old command/receipts cancelled before camera handoff");
 }
 return false;
}
