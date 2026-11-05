void TestNativeAction282(){
 const bool enabled=g_enabled,tracking=g_headTracking,eyes=g_eyeViewsSet,game=g_inGameplay,cam=g_frameCameraValid,flat=g_flatViewActive,menu=g_pistolMenu.open;
 const auto controls=fc3pad::Latest();fc3pad::Controllers idle={};fc3pad::Publish(idle);
 g_enabled=g_headTracking=g_eyeViewsSet=g_inGameplay=g_frameCameraValid=true;g_flatViewActive=g_pistolMenu.open=false;
 for(int action:{1,2,4,8,16,32,256,512,1024,2048,4096})Check(NativeActionContext282(true,action),"283 confirmed specific native animation can own camera without motion climbing");
 Check(!NativeActionContext282(true,0)&&!NativeActionContext282(false,1),"282 idle/no-headset cannot own native action camera");
 Check(!NativeActionContext282(true,64)&&!NativeActionContext282(true,3),"282 generic/ambiguous masks cannot select scripted camera");
 g_headTracking=false;Check(!NativeActionContext282(true,1),"282 disabled headtracking leaves script unchanged");g_headTracking=true;
 g_flatViewActive=true;Check(!NativeActionContext282(true,1),"282 flat/script screen refuses world camera writes");g_flatViewActive=false;
 g_pistolMenu.open=true;Check(!NativeActionContext282(true,1),"282 insert menu refuses script camera writes");g_pistolMenu.open=false;
 g_inGameplay=false;Check(!NativeActionContext282(true,1),"282 loading/menu scene refuses script camera writes");
 const bool oldWorld=g_worldViewActive;const LONG oldBins=g_mainBinCount;
 const LONG oldDraws=g_stats.stereoDraws,oldLast=g_lastStereoDraws;
 // Captured live ladder counts: sparse rail/sky views alternate with fuller
 // views. They must never turn a confirmed 3D world into menu mode.
 g_flatViewActive=false;g_frameCameraValid=true;
 for(int count:{93,686,88,681,104,103,642,99,641,102,663,85,648}){
  g_mainBinCount=count;g_stats.stereoDraws=oldLast+count;fc3stereo::PublishWorldPresence283();
  Check(g_inGameplay&&fc3stereo::InWorldView(),"283 recorded sparse ladder views retain gameplay/tracking regardless of scene complexity");
 }
 g_flatViewActive=true;fc3stereo::PublishWorldPresence283();Check(!g_inGameplay&&!fc3stereo::InWorldView(),"283 actual flat view still suspends world controls");
 g_flatViewActive=false;g_frameCameraValid=false;fc3stereo::PublishWorldPresence283();Check(!g_inGameplay&&!fc3stereo::InWorldView(),"283 actual absent world camera still suspends world controls");
 g_worldViewActive=oldWorld;g_mainBinCount=oldBins;g_stats.stereoDraws=oldDraws;g_lastStereoDraws=oldLast;
 g_enabled=enabled;g_headTracking=tracking;g_eyeViewsSet=eyes;g_inGameplay=game;g_frameCameraValid=cam;g_flatViewActive=flat;g_pistolMenu.open=menu;fc3pad::Publish(controls);
 puts("PASS:282 scripted renderer eligibility retains native animation controls and refuses no-headset/flat/menu/loading contexts");
}
