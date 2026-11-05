void TestCameraOwnership127()
{
 const bool motion=g_motionGun,enabled=g_enabled,game=g_inGameplay,eyes=g_eyeViewsSet,valid=g_frameCameraValid,head=g_headTracking,flat=g_flatViewActive,body=g_bodyValid,aim=g_aimActive,visible=g_visibilityCamera68,handoff=g_visibilityHandoff68,vehicle=g_vehicleCamera92;const auto weaponDraws=g_visibleWeaponDrawsLastFrame;BYTE p[sizeof(g_pistolReload)],machete[sizeof(g_machete)];memcpy(p,&g_pistolReload,sizeof(p));memcpy(machete,&g_machete,sizeof(machete));const auto two=g_twoHand;
 g_motionGun=false;g_enabled=g_inGameplay=g_eyeViewsSet=g_frameCameraValid=g_headTracking=true;g_flatViewActive=false;g_vehicleCamera92=false;g_visibleWeaponDrawsLastFrame=0;g_bodyValid=g_aimActive=g_visibilityCamera68=g_visibilityHandoff68=true;
 ReleaseHiddenCameraBasis127(false);Check(g_bodyValid&&g_aimActive&&g_visibilityHandoff68,"ordinary missing input must retain accepted body reference");
 ReleaseHiddenCameraBasis127(true);Check(!g_bodyValid&&!g_aimActive&&!g_visibilityHandoff68,"127 proved hidden action-camera conflict drops stale body compensation and handoff");
 Check(memcmp(&p,&g_pistolReload,sizeof(p))==0&&memcmp(&machete,&g_machete,sizeof(machete))==0&&two==g_twoHand,"camera release never changes firearm/machete state or hand ownership");
 g_motionGun=motion;g_enabled=enabled;g_inGameplay=game;g_eyeViewsSet=eyes;g_frameCameraValid=valid;g_headTracking=head;g_flatViewActive=flat;g_bodyValid=body;g_aimActive=aim;g_visibilityCamera68=visible;g_visibilityHandoff68=handoff;g_vehicleCamera92=vehicle;g_visibleWeaponDrawsLastFrame=weaponDraws;
 puts("PASS: camera127 hidden-owner loss clears stale reprojection/handoff without changing accepted weapon or machete state");
}
