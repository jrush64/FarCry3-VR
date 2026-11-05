// Native controls/animation own locomotion and hands. Only HMD orientation is
// composed at the verified camera producer, with draw-matched compensation.
bool g_nativeActionCamera282=false;
int g_nativeActionMask282=0;
bool NativeActionContext282(bool headset,int action){
 return fc3aim::SpecificNativeAction283(action)&&headset&&g_enabled&&g_headTracking&&g_eyeViewsSet&&g_inGameplay&&g_frameCameraValid&&!g_flatViewActive&&
  !g_pistolMenu.open&&!fc3pad::GameplayInputBlocked()&&!fc3pad::NativeMenuOpen();
}
bool TickNativeAction282(bool headset){
 const int action=fc3aim::NativeActionCamera282();
 if(NativeActionContext282(headset,action)&&
    fc3aim::SetVehicleCamera279(true,g_eyeViewsBody[0].view,g_frameCamera.right,g_frameCamera.up,g_frameCamera.forward)){
  if(!g_nativeActionCamera282||g_nativeActionMask282!=action)Log("[ACTION282] native action camera requested mask=%d; composition requires native/draw match; game controls and animated hands retained",action);
  g_nativeActionMask282=action;g_nativeActionCamera282=true;g_nativeFoot281=false;g_slideCamera280=false;
  g_nativeVehicleCamera279=true;g_vehicleCamera92=false;g_vehicleCameraLostAt92=0;
  fc3aim::EndVisibility68();fc3aim::Suspend();fc3pad::SetLookOverride(false,0,0);ResetScopeStabilization72();
  g_visibilityCamera68=g_visibilityHandoff68=false;
  NativeVehicleActual279(g_frameCamera);return true;
 }
 if(g_nativeActionCamera282){
  g_nativeActionCamera282=false;g_nativeActionMask282=0;
  if(fc3aim::NativeHandoff284()){
   g_nativeFoot281=g_nativeVehicleCamera279=true;g_bodyValid=false;
   NativeVehicleActual279(g_frameCamera);QueryPerformanceCounter(&g_aimLast);
   Log("[HANDOFF284] renderer retained native camera across direct script-to-gun exit until arm release and passive aim binding");
   return false;
  }
  fc3aim::SetVehicleCamera279(false,nullptr);g_nativeVehicleCamera279=false;
  // The script may have deliberately turned Jason. Ordinary aim must acquire
  // its current native view rather than retain a pre-script weapon heading.
  g_bodyValid=g_aimActive=false;QueryPerformanceCounter(&g_aimLast);
  Log("[ACTION282] native action camera released; ordinary ownership reacquires the current view");
 }
 return false;
}
