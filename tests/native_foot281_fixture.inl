void TestNativeFoot281(){
 const bool enabled=g_enabled,tracking=g_headTracking,eyes=g_eyeViewsSet,game=g_inGameplay,cam=g_frameCameraValid,flat=g_flatViewActive,foot=g_nativeFoot281,native=g_nativeVehicleCamera279,aim=g_aimActive,body=g_bodyValid,motion=g_motionGun,aimOn=g_aimEnabled,menu=g_pistolMenu.open;
 const double yaw=g_bodyYaw;const LONG draws=g_visibleWeaponDrawsLastFrame;const auto controls=fc3pad::Latest();
 fc3pad::Controllers idle={};fc3pad::Publish(idle);fc3aim::SetNativeActionForTest128(0);
 g_enabled=g_headTracking=g_eyeViewsSet=g_inGameplay=g_frameCameraValid=true;g_flatViewActive=g_pistolMenu.open=false;g_visibleWeaponDrawsLastFrame=0;
 Check(NativeFootContext281(true),"281 weapon-free renderer selects common native view");
 Check(!NativeFootContext281(false),"281 no headset cannot acquire native view");
 g_headTracking=false;Check(!NativeFootContext281(true),"281 head tracking toggle refuses native view");g_headTracking=true;
 g_flatViewActive=true;Check(!NativeFootContext281(true),"281 flat/menu view refuses native view");g_flatViewActive=false;
 g_pistolMenu.open=true;Check(!NativeFootContext281(true),"281 insert menu refuses native view");g_pistolMenu.open=false;
 fc3aim::SetNativeActionForTest128(4);Check(!NativeFootContext281(true),"281 protected action keeps renderer ownership");fc3aim::SetNativeActionForTest128(0);
 g_visibleWeaponDrawsLastFrame=3;Check(!NativeFootContext281(true),"281 armed view cannot be claimed by no-weapon path");
 g_nativeFoot281=true;Check(NativeFootContext281(true),"283 visible gun cannot release an existing native view before passive aim binding");g_nativeFoot281=false;
 g_nativeFoot281=g_nativeVehicleCamera279=g_aimActive=g_motionGun=g_aimEnabled=true;g_bodyValid=false;g_bodyYaw=.37;
 Check(!TickNativeFoot281(true)&&!g_nativeFoot281&&!g_nativeVehicleCamera279&&g_bodyValid&&fabs(g_bodyYaw-.37)<1e-8,"281 armed handoff cancels native state while retaining visual heading");
 g_enabled=enabled;g_headTracking=tracking;g_eyeViewsSet=eyes;g_inGameplay=game;g_frameCameraValid=cam;g_flatViewActive=flat;g_nativeFoot281=foot;g_nativeVehicleCamera279=native;g_aimActive=aim;g_bodyValid=body;g_motionGun=motion;g_aimEnabled=aimOn;g_pistolMenu.open=menu;g_bodyYaw=yaw;g_visibleWeaponDrawsLastFrame=draws;
 fc3aim::SetNativeActionForTest128(-1);fc3pad::Publish(controls);
 puts("PASS:281 renderer native-view eligibility and armed handoff retain heading, reject headset/flat/menu/action/weapon conflicts");
}
