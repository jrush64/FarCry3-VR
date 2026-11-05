void TestPouch361(){
 fc3pouch361::State state;
 Check(!state.Step(true,true,0,0,1,100),"361 first reference cannot acquire an item");state.Step(true,true,0,0,1,110);
 Check(state.Step(true,true,1,.1f,1,120),"361 fresh empty-hand pouch squeeze draws");
 Check(!state.Step(true,true,1,.1f,1,130),"361 held squeeze cannot draw another item");
 state.Step(false,true,0,0,1,140);Check(!state.Step(true,true,1,.1f,1,150),"361 holding a gun/context denial requires release before pouch entry");
 state.Step(true,true,0,0,1,160);Check(!state.Step(true,true,1,.3f,1,170),"361 squeezing away from pouch cannot draw");Check(!state.Step(true,true,1,.1f,1,180),"361 moving an already held grip into pouch cannot draw");
 state.Step(true,true,0,0,1,190);Check(!state.Step(true,true,1,.1f,1,350),"361 stale tracking cannot produce a draw");state.Step(true,true,0,0,1,360);Check(!state.Step(true,false,1,.1f,1,370),"361 tracking loss cannot produce a draw");

 g_pouchEnabled361=true;g_throw144.enabled=true;g_throw144.owned=false;g_pouchState361={};g_enabled=g_motionGun=g_inGameplay=g_eyeViewsSet=true;g_worldScale=1;g_weaponMechanicProfile=-1;g_weaponMechanicFrame=g_frame;g_reload={};g_manualReload={};g_reloadPose={};g_reloadAlignment=false;g_pistolMenu.open=false;
 for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
 auto hand361=HandIdentity();hand361.m[3]=g_pouchPosition361[0];hand361.m[7]=g_pouchPosition361[1]+.015;hand361.m[11]=g_pouchPosition361[2]-.015;for(int n=0;n<16;++n)g_physicalBody303[0][n]=float(hand361.m[n]);g_physicalValid303[0]=true;g_physicalAt352=GetTickCount64();SetRenderHands303(0,true,g_armPoseEpoch);SeedFoot357();
 fc3pad::Controllers actual361={};actual361.active=true;PouchDraw361(true,actual361);PouchDraw361(true,actual361);actual361.rightGrip=1;
 fc3pad::SetDoorInput357(true,false);actual361.rightGrip=0;PouchDraw361(true,actual361);actual361.rightGrip=1;Check(!PouchDraw361(true,actual361),"363 accepted car-door grip reserves this squeeze from the throwable pouch");fc3pad::SetDoorInput357(false,false);actual361.rightGrip=0;PouchDraw361(true,actual361);PouchDraw361(true,actual361);actual361.rightGrip=1;
 Check(PouchDraw361(true,actual361),"361 actual fresh native-foot context and physical controller at body-relative pouch produce a draw intent");

 extern void PublishRawPouch361(float);
 g_throw144.kind=3;g_throw144.seen=-1000;g_throw144.owned=false;g_pouchState361={};
 PublishRawPouch361(0);TickThrowable144(true);PublishRawPouch361(0);TickThrowable144(true);PublishRawPouch361(1);TickThrowable144(true);
 Check(g_throw144.owned&&g_throw144.drawing&&g_pouchSession361&&g_throw144.kind==0,"362 stale previous C4 visual must not block a fresh native throwable pouch draw");
 ResetThrowable144("offline pouch fixture complete");
 SetRenderHands303(0,false,g_armPoseEpoch);actual361.rightGrip=0;PouchDraw361(true,actual361);actual361.rightGrip=1;Check(!PouchDraw361(true,actual361),"361 held firearm denies actual pouch draw before any reload or native input");g_pouchEnabled361=false;
 fc3pad::Controllers c={};c.active=true;c.rightGrip=1;c.b=true;fc3pad::SetGameplayControls(true);fc3aim::SetVehicleStateForTest(0);fc3pad::SetPhysicalThrowable361(false);fc3pad::SetThrowableInput144(false,false);fc3pad::SetHandsStowed303(false);fc3pad::SetSelector290(false);fc3pad::Publish(c);
 Check((fc3pad::ButtonsOutput()&0x200)==0,"361 old grip+B still cannot draw grenades");
 c.b=false;fc3pad::SetPhysicalThrowable361(true);fc3pad::SetThrowableInput144(true,true);fc3pad::SetHandsStowed303(true);fc3pad::Publish(c);
 Check((fc3pad::ButtonsOutput()&0x200)!=0&&fc3pad::FireOutput()==0,"361 explicit physical owner alone emits native grenade hold while firearm remains stowed");
 c.active=false;fc3pad::Publish(c);Check((fc3pad::ButtonsOutput()&0x200)!=0,"361 controller loss cannot invent a grenade release");
 c.active=true;fc3pad::Publish(c);fc3pad::SetThrowableInput144(true,false);Check((fc3pad::ButtonsOutput()&0x200)==0,"361 release ticket ends native shoulder hold");fc3pad::SetPhysicalThrowable361(false);fc3pad::SetThrowableInput144(false,false);fc3pad::SetHandsStowed303(false);
 puts("PASS:361 fresh physical pouch intent, firearm/context/tracking/range/stale isolation, disabled old chord, actual pad/native hold and tracking-loss preservation; projectile/pin/lighter outcomes pending headset");
}
