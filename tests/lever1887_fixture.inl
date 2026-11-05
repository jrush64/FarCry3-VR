#include "lever1887_live_data.inl"
// 1887 hand switch + lever through the real BuildWeaponWorld GPU path with PID56072 data.
void TestLever1887(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const auto oldLever=g_lever;const auto oldFree=g_freeHandAttachment[0],oldFreeLeft=g_freeHandAttachment[1];const LONG oldProfile=g_weaponMechanicProfile;const double oldScale=g_worldScale;g_worldScale=1;
 g_weaponMechanicProfile=0;g_weaponMechanicFrame=++g_frame;g_motionGun=g_handLeftValid=g_handRightValid=g_eyeViewsSet=true;g_weaponMotionGuard=false;g_pistolMenu.open=false;g_twoHand=false;
 g_lever=Lever1887{};auto& L=g_lever;
 fc3pad::Controllers c={};c.active=true;fc3pad::Publish(c);
 Check(Lever1887Model()&&Lever1887Active()&&!L.hold&&L.chambered,"1887 recognized, in the right hand, chambered");
 context->ClearState();g_dev=device;g_oCreateBuffer=PistolFixtureCreate;
 ID3D11Buffer* vb=Buffer(device,217840,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,47196,D3D11_BIND_INDEX_BUFFER);
 ID3D11Buffer* globals=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=400;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary cam={};memcpy(cam.right,kL1887Camera,12);memcpy(cam.up,kL1887Camera+3,12);memcpy(cam.forward,kL1887Camera+6,12);memcpy(cam.position,kL1887Camera+9,12);cam.mainView=1;
 auto setCamera=[&](const CameraSummary& s){camera->SetPrivateData(kCameraSummaryGuid,sizeof(s),&s);};
 setCamera(cam);ID3D11Buffer* cb[2]={globals,camera};context->VSSetConstantBuffers(0,2,cb);
 g_frameGunValid=true;g_frameGunMove=HandIdentity();
 for(int s=0;s<2;++s){const Mat4 m=HandIdentity();for(int i=0;i<16;++i)g_handBody[s][i]=static_cast<float>(m.m[i]);}
 g_handBody[0][3]=.3f;g_handBody[0][7]=-.2f;g_handBody[0][11]=.4f;g_handBody[1][3]=-.1f;g_handBody[1][7]=-.2f;g_handBody[1][11]=.6f;
 auto draw=[&](const float* g,UINT s,UINT n,float* out){globals->SetPrivateData(kCbCopyGuid,88*4,g);auto* o=BuildWeaponWorld(context,s,n);Check(o!=nullptr,"1887 GPU draw");Read(device,context,o,out);};
 auto same=[&](const Mat4& a,const Mat4& b){double e=0;for(int i=0;i<12;++i)e=(std::max)(e,std::fabs(a.m[i]-b.m[i]));return e<.001;};
 auto inv=[](const Mat4& m){Mat4 r={};Invert(m,r);return r;};
 Mat4 bi=inv(ArmBodyToCamera());
 auto toBody=[&](const CameraSummary& s,const float* g){return Multiply(bi,Multiply(HullView(s),HullWorld(g)));};
 float frame[88]={},part[88]={};
 auto fire=[&](float t){c.rightTrigger=t;fc3pad::Publish(c);UpdateLever1887Fire(c);return fc3pad::FireOutput();};
 // Right hand: ordinary path; lever drawn shut, hammer cocked (no automatic native cycle).
 draw(kL1887FrameGlobals,42,10899,frame);const Mat4 frameBody=toBody(cam,frame);
 Check(L.parentValid&&Lever1887RightStable(),"231 immutable grip anchors 1887 on first draw without learning equip animation");
 for(int n=0;n<16;++n){++g_frame;g_weaponMechanicFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);}
 Check(L.parentValid&&same(toBody(cam,frame),frameBody)&&Lever1887RightStable(),"right-hand lock learned after the gun sits still; no visible change");
 {Mat4 move={};float bones[576];memcpy(bones,kL1887RightBones,sizeof(bones));Mat4 right={};for(int i=0;i<16;++i)right.m[i]=g_handBody[0][i];
  Check(Lever1887RightHoldingMove(HandIdentity(),bones,move),"stable right arm is placed on the gun");
  const Mat4 wrist=Multiply(move,BoneMatrix(bones,2)),expected=Multiply(ArmBodyToCamera(),Multiply(Multiply(right,L.parentRel),kL1887RightHandFromFrame));
  Check(same(wrist,expected),"right wrist sits on the 1887 in the native quiet hand pose");}
 draw(kL1887LeverGlobals,13626,7170,part);Check(same(Multiply(inv(frameBody),toBody(cam,part)),kL1887LeverRest),"right hand: lever stays shut");
 {V3 aim={};Check(Lever1887AimBody(aim)&&Length(Sub(aim,Unit(V3{frameBody.m[1],frameBody.m[5],frameBody.m[9]})))<1e-6,"one-handed 1887 aims along the displayed barrel, not the controller");}
 
 {float fingers[576];memcpy(fingers,kL1887RightBones,sizeof(fingers));const Mat4 wrist=BoneMatrix(fingers,2);Check(Lever1887RightGrip(fingers),"right hand keeps the quiet holding grip");
  for(int b=3;b<=19;++b){const Mat4 e=Multiply(wrist,BoneMatrix(kL1887RightQuietGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-e.m[i])<.001,"quiet right fingers keep the wrist");}}
 Check(fire(0)==0&&fire(1)==255&&!L.chambered,"first pull fires; the chamber is now empty");
 for(int i=0;i<8;++i){++g_frame;g_weaponMechanicFrame=g_frame;fire(1);}Check(fire(1)==0,"held trigger cannot fire again");fire(0);Check(fire(1)==0,"a new pull cannot fire before the lever");fire(0);
 // The game's post-shot lever cycle swings its wrist against the gun: the gun must not follow it.
 {Mat4 swing=TurnAndMove({0,1,0},{0,.8,.6},{0,0,0},{0,0,0});swing.m[7]+=.05;g_frameGunMove=swing;
  ++g_frame;g_weaponMechanicFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);
  Check(same(toBody(cam,frame),frameBody),"post-shot: gun stays locked to the right controller, the native cycle cannot move it");
  draw(kL1887LeverGlobals,13626,7170,part);Check(same(Multiply(inv(toBody(cam,frame)),toBody(cam,part)),kL1887LeverRest),"post-shot: lever stays shut");
  g_frameGunMove=HandIdentity();}
 ++g_frame;g_weaponMechanicFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);draw(kL1887HammerGlobals,21264,1956,part);
 Check(same(Multiply(inv(toBody(cam,frame)),toBody(cam,part)),kL1887HammerFired),"after the shot the hammer is down, not auto-cocked");
 // Reported: wood/parts animating by themselves. Every part has a FIXED rest on the gun body, whatever the game does.
 {float wild[88];memcpy(wild,kL1887ShellGlobals,sizeof(wild));Mat4 w=HullWorld(kL1887ShellGlobals);w.m[3]+=.3;w.m[11]-=.2;WritePistolWorld(wild,Multiply(TurnAndMove({0,1,0},{.6,.8,0},{0,0,0},{0,0,0}),w));
  ++g_frame;g_weaponMechanicFrame=g_frame;draw(wild,23220,378,part);draw(kL1887FrameGlobals,42,10899,frame); // cradle drawn BEFORE the body
  Check(same(toBody(cam,part),Multiply(toBody(cam,frame),kL1887CradleRest)),"cradle stays at its fixed rest even when the game moves it");
  draw(wild,20904,360,part);Check(same(toBody(cam,part),Multiply(toBody(cam,frame),kL1887ShellBRest)),"shell piece stays at its fixed rest");
  stride=20;offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);draw(wild,0,42,part);Check(same(toBody(cam,part),toBody(cam,frame)),"small frame piece rigid on the body");
  stride=32;offset=400;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);}
 const Mat4 beforeGrab=toBody(cam,frame);
 // Left hand: the SAME pose every time (normal native forend hold), plus the INSERT adjustment.
 Mat4 left=HandIdentity();left.m[3]=-.25;left.m[7]=-.1;left.m[11]=.45;L.leftHandBody=left;L.leftHandFrame=g_frame;
 // Two hands like normal: LEFT grip on the forend does NOT take the gun; the locked gun turns toward the real left wrist.
 auto placeRealLeft=[&](const V3& body){const Mat4 m=HandIdentity();for(int i=0;i<16;++i)g_handBody[1][i]=g_handLeft[i]=static_cast<float>(m.m[i]);
  g_handBody[1][3]=static_cast<float>(body.x-g_wristOffset[1][0]);g_handBody[1][7]=static_cast<float>(body.y-g_wristOffset[1][1]);g_handBody[1][11]=static_cast<float>(body.z-g_wristOffset[1][2]);
  const V3 cam3=Apply(ArmBodyToCamera(),body);g_handLeft[3]=static_cast<float>(cam3.x-g_wristOffset[1][0]);g_handLeft[7]=static_cast<float>(cam3.y-g_wristOffset[1][1]);g_handLeft[11]=static_cast<float>(cam3.z-g_wristOffset[1][2]);};
 L.chambered=true; // ordinary two-handed support remains ordinary when no cycle is due
 {const Mat4 gun=Lever1887RightBody();const V3 support=Lever1887SupportPoint(gun),pivot=Apply(Multiply(gun,kL1887RightHandFromFrame),{.591,.011,1.149});
  const V3 dir=Unit(Sub(support,pivot));placeRealLeft(Add(support,Scale(dir,.04)));c.leftGrip=1;fc3pad::Publish(c);g_twoHand=true;TickLever1887(true);
  Check(!L.hold&&g_twoHand,"LEFT grip on the forend gives the normal two-handed hold, not a hand switch");
  const Mat4 turned=Lever1887GunBody();const V3 s2=Lever1887SupportPoint(turned),real=Lever1887RealLeftWrist();
  Check(Length(Cross(Unit(Sub(s2,pivot)),Unit(Sub(real,pivot))))<1e-4,"two-handed: the gun aims from the right hand toward the left hand");
  ++g_frame;g_weaponMechanicFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);Check(same(toBody(cam,frame),turned),"two-handed gun drawn at the turned lock");
  Mat4 move={};float bones[576];memcpy(bones,kL1887HoldLeftBones,sizeof(bones));Check(Lever1887LeftSupportMove(HandIdentity(),bones,move),"left hand placed on the forend while two-handed");
  Check(same(Multiply(move,BoneMatrix(bones,2)),Multiply(ArmBodyToCamera(),Multiply(turned,kL1887LeftSupportFromFrame))),"left wrist sits on the forend in the native support pose");
  float fingers[576];memcpy(fingers,kL1887HoldLeftBones,sizeof(fingers));Check(Lever1887LeftGrip(fingers),"two-handed left fingers use the forend grip");
  placeRealLeft(Add(support,Scale(dir,.25)));TickLever1887(true);Check(!L.hold&&g_twoHand,"pulling the hands apart does not switch");
  fc3pad::CancelReloadRequest();c.x=true;fc3pad::Publish(c);Check(!fc3pad::ReloadOutput(),"X with the left hand on the 1887 never reaches the game");
  c.x=false;fc3pad::Publish(c);TickLever1887(true);
  {const bool saved=L.chambered;L.chambered=true;c.rightGrip=1;c.rightTrigger=1;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold,"chambered gun: RIGHT grip + trigger does not switch");
   c.rightGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickLever1887(true);L.chambered=saved;}
  {Mat4 r={};for(int i=0;i<16;++i)r.m[i]=g_handBody[0][i];V3 aim={};Check(Lever1887AimBody(aim),"two-handed 1887 aims along its displayed barrel");const Mat4 g=Lever1887GunBody();
   Check(Length(Sub(aim,Unit(V3{g.m[1],g.m[5],g.m[9]})))<1e-9,"two-handed aim = the turned barrel");
   // Reported regression: crosshair thrown off-screen two-handed. The aim rotates the camera; the two-handed aim must not
   // change when the camera rotation (and the camera-space copy of the hands) changes.
   const bool oldActive=g_aimActive;double oldQ[3][3];memcpy(oldQ,g_aimQ,sizeof(oldQ));float oldLeft[16];memcpy(oldLeft,g_handLeft,sizeof(oldLeft));
   const Mat4 spin=TurnAndMove({1,0,0},{.6,.8,0},{0,0,0},{0,0,0});g_aimActive=true;for(int r=0;r<3;++r)for(int k=0;k<3;++k)g_aimQ[r][k]=spin.m[r*4+k];
   for(int i=0;i<16;++i)g_handLeft[i]=0;g_handLeft[0]=g_handLeft[5]=g_handLeft[10]=g_handLeft[15]=1;g_handLeft[3]=9;g_handLeft[7]=-9;
   V3 aim2={};Check(Lever1887AimBody(aim2)&&Length(Sub(aim2,aim))<1e-9,"two-handed aim is independent of the aim-driven camera (no feedback loop)");
   g_aimActive=oldActive;memcpy(g_aimQ,oldQ,sizeof(oldQ));memcpy(g_handLeft,oldLeft,sizeof(oldLeft));}
  {// Reported: the right hand vanished when two-handed. Drive the real right-arm builder (actual GPU upload).
   const auto gpuJoints=g_armJoints[0];g_armJoints[0]={{.2,-.15,-.2},{.35,-.3,.05},{.35,-.2,.4},true};
   ID3D11Buffer* armVb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER),*armRaw=Buffer(device,2304,D3D11_BIND_CONSTANT_BUFFER);
   UINT armStride=40,armOffset=0;context->IASetVertexBuffers(0,1,&armVb,&armStride,&armOffset);
   armRaw->SetPrivateData(kCbCopyGuid,sizeof(kL1887RightBones),kL1887RightBones);context->VSSetConstantBuffers(3,1,&armRaw);
   globals->SetPrivateData(kCbCopyGuid,sizeof(kL1887RightArmGlobals),kL1887RightArmGlobals);
   CameraSummary armCam={};memcpy(armCam.right,kL1887RightArmCamera,12);memcpy(armCam.up,kL1887RightArmCamera+3,12);memcpy(armCam.forward,kL1887RightArmCamera+6,12);memcpy(armCam.position,kL1887RightArmCamera+9,12);armCam.mainView=1;setCamera(armCam);
   Mat4 toV={},fromV={};float nativeArm[576];Check(DrawSpace(context,nativeArm,toV,fromV),"1887 right arm draw context");
   auto* pal=BuildArmPalette(context,0);Check(pal!=nullptr,"two-handed 1887: the right arm is built and drawn (no vanishing)");
   float armOut[576];Read(device,context,pal,armOut);const Mat4 wrist=Multiply(toV,BoneMatrix(armOut,2));
   const Mat4 expected=Multiply(ArmBodyToCamera(),Multiply(Lever1887GunBody(),kL1887RightHandFromFrame));
   double err=0;for(int i=0;i<12;++i)err=(std::max)(err,std::fabs(wrist.m[i]-expected.m[i]));
   Check(err<.0001,"two-handed: the right wrist sits on the turned 1887 stock, not on the forend");
   ID3D11Buffer* none=nullptr;context->VSSetConstantBuffers(3,1,&none);armRaw->Release();armVb->Release();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);setCamera(cam);g_armJoints[0]=gpuJoints;}
  c.rightGrip=1;c.rightTrigger=1;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold,"default: RIGHT grip + trigger does not switch");c.rightGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickLever1887(true);
  c.leftStickClick=true;c.rightStickClick=true;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold,"both stick clicks (arm calibration) never switch");c.leftStickClick=c.rightStickClick=false;fc3pad::Publish(c);TickLever1887(true);
  L.chambered=false;c.leftStickClick=false;c.x=false;fc3pad::Publish(c);TickLever1887(true);Check(L.hold&&!g_twoHand,"cycle due: LEFT support grip alone transfers gun without X or stick click");
  c.leftGrip=0;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold,"release LEFT grip gives it back");c.leftGrip=1;fc3pad::Publish(c);g_twoHand=false;L.leftHandFrame=g_frame;placeRealLeft(Lever1887SupportPoint(L.displayBody));{const Mat4 box=Lever1887SupportBox56();L.leftHandBody=HandIdentity();L.leftHandBody.m[3]=box.m[3]+.6781;L.leftHandBody.m[7]=box.m[7]-.0519;L.leftHandBody.m[11]=box.m[11]-1.0387;g_freeHandAttachment[1].epoch=g_armPoseEpoch+1;}TickLever1887(true);
  Check(L.hold&&!g_twoHand,"empty chamber fresh nearby LEFT grip transfers without prior two-handed state");L.leftHandBody=left;c.x=false;fc3pad::Publish(c);TickLever1887(true);
  c.leftGrip=0;fc3pad::Publish(c);TickLever1887(true);c.leftGrip=1;fc3pad::Publish(c);g_twoHand=true;TickLever1887(true);
  L.switchRightTrigger=true;
  c.rightGrip=1;fc3pad::Publish(c);TickLever1887(true);Check(L.hold,"left grip still owns gun independent of right grip");
  c.rightTrigger=1;fc3pad::Publish(c);TickLever1887(true);Check(L.hold&&!g_twoHand,"empty chamber + two hands: RIGHT grip + trigger moves the gun into the left hand");
  c.rightGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickLever1887(true);Check(L.hold,"letting go of the right controller keeps the gun in the left hand");}
 L.holdAt=GetTickCount64()-1000;++g_frame;g_weaponMechanicFrame=g_frame;L.leftHandFrame=g_frame;
 const Mat4 supportInv=inv(kL1887LeftSupportFromFrame);
 draw(kL1887FrameGlobals,42,10899,frame);Check(same(toBody(cam,frame),Multiply(left,supportInv)),"gun sits in the left hand in the normal forend hold");
 Mat4 turned=Multiply(TurnAndMove({1,0,0},{.8,0,.6},{0,0,0},{0,0,0}),left);turned.m[3]+=.15;L.leftHandBody=turned;++g_frame;g_weaponMechanicFrame=g_frame;L.leftHandFrame=g_frame;
 draw(kL1887FrameGlobals,42,10899,frame);Check(same(toBody(cam,frame),Multiply(turned,supportInv)),"same pose after moving and turning the hand (never floats)");
 L.left[0]=.02f;L.left[2]=.01f;L.left[5]=20;draw(kL1887FrameGlobals,42,10899,frame);
 const Mat4 held=Multiply(Multiply(turned,supportInv),Lever1887Adjust(L.left));Check(same(toBody(cam,frame),held),"INSERT left-hand adjustment moves/rotates the gun in the hand");
 g_frameGunMove=Multiply(TurnAndMove({0,1,0},{1,0,0},{0,0,0},{0,0,0}),HandIdentity());draw(kL1887FrameGlobals,42,10899,part);Check(same(toBody(cam,part),held),"the right hand cannot move the held gun");g_frameGunMove=HandIdentity();
 {CameraSummary moved=cam;moved.position[0]+=.6f;moved.position[2]+=.2f;const Mat4 turn=TurnAndMove({1,0,0},{.9,.43589,0},{0,0,0},{0,0,0});
  for(float* axis:{moved.right,moved.up,moved.forward}){const V3 a=Apply(turn,V3{axis[0],axis[1],axis[2]});axis[0]=float(a.x);axis[1]=float(a.y);axis[2]=float(a.z);}
  ++g_frame;g_weaponMechanicFrame=g_frame;L.leftHandFrame=g_frame;setCamera(moved);
  draw(kL1887ShellGlobals,20904,360,part);Check(same(toBody(moved,part),Multiply(held,kL1887ShellBRest)),"part drawn before the gun body stays on the held gun while the head moves");
  draw(kL1887FrameGlobals,42,10899,frame);Check(same(toBody(moved,frame),held),"gun body unchanged by head motion");setCamera(cam);}
 // Lever: pull down opens and it stays open; push back closes and chambers.
 ++g_frame;g_weaponMechanicFrame=g_frame;L.leftHandFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);
 float rightBeforeLever[16];memcpy(rightBeforeLever,g_handBody[0],sizeof(rightBeforeLever));
 auto palmAt=[&](double dx,double dy,double dz){Mat4 t=HandIdentity();t.m[3]=L.lever[0]+dx-.6781;t.m[7]=L.lever[1]+dy-.0519;t.m[11]=L.lever[2]+dz-1.0387;L.rightHandBody=Multiply(L.displayBody,t);L.rightHandFrame=g_frame;L.displayFrame=g_frame;for(int i=0;i<16;++i)g_handBody[0][i]=float(L.rightHandBody.m[i]);auto& free=g_freeHandAttachment[0];free.relative=HandIdentity();free.epoch=g_armPoseEpoch;free.frame=g_frame;free.profile=0;};
 const V3 path=Unit(Sub(V3{kL1887LeverOpenPoint[0],kL1887LeverOpenPoint[1],kL1887LeverOpenPoint[2]},V3{kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]}));
 const double openReach=Length(Sub(V3{kL1887LeverOpenPoint[0],kL1887LeverOpenPoint[1],kL1887LeverOpenPoint[2]},V3{kL1887LeverPoint[0],kL1887LeverPoint[1],kL1887LeverPoint[2]}));
 auto along=[&](double s){palmAt(path.x*s,path.y*s,path.z*s);};
 auto grip=[&](float g){c.rightGrip=g;fc3pad::Publish(c);TickLever1887(true);};
 palmAt(.3,0,0);grip(0);grip(1);Check(!L.grabbing,"right grip away from the lever does not grab it");
 grip(0);along(0);grip(1);Check(L.grabbing&&!L.leverOpen,"right grip at the lever loop grabs the closed lever");
 along(.015);TickLever1887(true);grip(0);Check(!L.leverOpen&&!L.chambered,"a short pull springs back shut, chamber unchanged");
 along(0);grip(1);along(.05);TickLever1887(true);Check(Lever1887Fraction()==1,"pulling down along the lever path opens it fully");
 draw(kL1887LeverGlobals,13626,7170,part);Check(same(Multiply(inv(toBody(cam,frame)),toBody(cam,part)),kL1887LeverOpen),"lever drawn fully open (62 deg) on the held gun");
 draw(kL1887HammerGlobals,21264,1956,part);Check(same(Multiply(inv(toBody(cam,frame)),toBody(cam,part)),kL1887HammerCocked),"hammer cocked by the lever");
 {float fingers[576];memcpy(fingers,kL1887RightBones,sizeof(fingers));const Mat4 wrist=BoneMatrix(fingers,2);Check(Lever1887RightGrip(fingers),"right hand takes the native lever grip");
  for(int b=3;b<=19;++b){const Mat4 e=Multiply(wrist,BoneMatrix(kL1887RightLeverGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-e.m[i])<.001,"native lever fingers keep the wrist");}}
 grip(0);Check(L.leverOpen&&!L.chambered&&Lever1887Fraction()==1,"released: the lever STAYS open (gate open for shells)");
 draw(kL1887LeverGlobals,13626,7170,part);Check(same(Multiply(inv(toBody(cam,frame)),toBody(cam,part)),kL1887LeverOpen),"open lever stays drawn open");
 c.leftGrip=0;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold&&L.leverOpen,"back in the right hand the lever is still open");
 UpdateLever1887Fire(c);c.rightTrigger=1;fc3pad::Publish(c);UpdateLever1887Fire(c);Check(fc3pad::FireOutput()==0,"open lever: the trigger is locked");c.rightTrigger=0;fc3pad::Publish(c);UpdateLever1887Fire(c);
 c.leftGrip=1;fc3pad::Publish(c);g_twoHand=true;c.rightGrip=1;c.rightTrigger=1;fc3pad::Publish(c);TickLever1887(true);Check(L.hold,"open lever (empty chamber): RIGHT grip + trigger takes it back into the left hand");
 c.rightGrip=0;c.rightTrigger=0;fc3pad::Publish(c);TickLever1887(true);L.leftHandFrame=g_frame;draw(kL1887FrameGlobals,42,10899,frame);
 along(openReach);grip(1);Check(L.grabbing&&L.grabFromOpen,"right grip at the open lever loop grabs it");
 along(openReach-.05);TickLever1887(true);Check(Lever1887Fraction()==0,"pushing back closes the lever");
 grip(0);Check(!L.leverOpen&&L.chambered,"closed: a round is chambered");
 {palmAt(.05,0,0);grip(0);grip(1);Check(L.grabbing,"right side, hand 5 cm out from the gun: grabs");grip(0);}
 L.chambered=false;along(0);grip(1);along(.05);TickLever1887(true);along(0);TickLever1887(true);grip(0);Check(!L.leverOpen&&L.chambered,"one pull-and-push motion also cycles the lever");
 {// Left side of the gun: the mirrored loop grabs and pulls the same way.
  const double m=-2*L.lever[0]-.05;/* hand 5 cm out from the gun on its LEFT (mirror of a right-side hand 5 cm out) */auto leftAlong=[&](double s){palmAt(m+path.x*s,path.y*s,path.z*s);};
  palmAt(-.4,0,0);grip(0);grip(1);Check(!L.grabbing,"right grip beyond mirrored loop plus visible hand sphere radius does not grab");grip(0);
  L.chambered=false;leftAlong(0);grip(1);Check(L.grabbing&&!L.leverOpen,"right grip at the LEFT side of the loop grabs the lever");
  leftAlong(.05);TickLever1887(true);Check(Lever1887Fraction()==1,"pulled from the left side: lever opens fully");grip(0);Check(L.leverOpen&&!L.chambered,"left side: lever stays open");
  leftAlong(openReach);grip(1);Check(L.grabbing&&L.grabFromOpen,"left side: grabs the open lever");leftAlong(openReach-.05);TickLever1887(true);grip(0);Check(!L.leverOpen&&L.chambered,"left side: pushing back closes and chambers");}
 Check(fire(1)==0,"still no firing while the left hand holds the gun");fire(0);
 {float fingers[576];memcpy(fingers,kL1887HoldLeftBones,sizeof(fingers));const Mat4 wrist=BoneMatrix(fingers,2);Check(Lever1887LeftGrip(fingers),"left hand takes the forend grip");
  for(int b=3;b<=19;++b){const Mat4 e=Multiply(wrist,BoneMatrix(kL1887LeftQuietGripRelative,b-3));for(int i=0;i<12;++i)Check(std::fabs(fingers[b*12+i]-e.m[i])<.001,"forend fingers keep the wrist");}}
 {// Held shell is drawn in the right hand at the native carry pose.
  L.shell=true;L.rightHandBody=HandIdentity();L.rightHandBody.m[3]=.25;L.rightHandFrame=g_frame;
  draw(kL1887ShellGlobals,20904,360,part);Check(same(toBody(cam,part),Multiply(L.rightHandBody,kL1887ShellFromHand)),"held shell drawn in the right hand at the native carry pose");
  draw(kL1887ShellGlobals,20796,108,part);Check(same(toBody(cam,part),Multiply(L.rightHandBody,kL1887ShellFromHand)),"both shell pieces travel together");L.shell=false;}
 memcpy(g_handBody[0],rightBeforeLever,sizeof(rightBeforeLever));g_freeHandAttachment[0]=oldFree;
 c.leftGrip=0;fc3pad::Publish(c);TickLever1887(true);Check(!L.hold,"release LEFT grip: gun back in the right hand");
 L.returnAt=0;L.shotFrame=-100000;g_twoHand=false;++g_frame;g_weaponMechanicFrame=g_frame;
 draw(kL1887FrameGlobals,42,10899,frame);Check(same(toBody(cam,frame),frameBody),"release snaps straight back to the exact right-hand lock (no transition)");
 L.right[1]=.01f;draw(kL1887FrameGlobals,42,10899,frame);{Mat4 r={};for(int i=0;i<16;++i)r.m[i]=g_handBody[0][i];Check(same(toBody(cam,frame),Multiply(Multiply(r,L.parentRel),Lever1887Adjust(L.right))),"INSERT right-hand adjustment moves the gun");
  Mat4 move={};float bones[576];memcpy(bones,kL1887RightBones,sizeof(bones));Check(Lever1887RightHoldingMove(HandIdentity(),bones,move)&&same(Multiply(move,BoneMatrix(bones,2)),Multiply(ArmBodyToCamera(),Multiply(toBody(cam,frame),kL1887RightHandFromFrame))),"right hand follows the adjusted gun");}
 {wchar_t dir[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"l87",0,path);Check(SaveLever1887Settings(path),"Save All - 1887");
  for(int n=0;n<17;++n)*Lever1887Value(n)=0;LoadLever1887Settings(path);
  Check(std::fabs(L.left[0]-.02f)<1e-5&&std::fabs(L.left[5]-20)<1e-4&&std::fabs(L.right[1]-.01f)<1e-5&&std::fabs(L.lever[0]-kL1887LeverPoint[0])<1e-5,"1887 hand settings survive save and reload");DeleteFileW(path);}
 L.right[1]=0;
 Check(fire(0)==0&&fire(1)==255&&!L.chambered,"fires after the lever, and the next shot needs the lever again");fire(0);
 std::puts("PASS: 1887 lever: every shot locks the trigger; LEFT grip holds the gun in the normal forend pose (+ INSERT adjustment); two hands + LEFT stick click or X moves it to the left hand (INSERT option adds RIGHT grip + trigger, empty chamber only); two-handed aim free of camera feedback; aim follows the displayed barrel; two-handed right arm drawn on the stock (actual GPU arm upload); lever pulled down (from the right OR left side) STAYS open (trigger locked, gate open), pushed back closes and chambers, one motion also cycles; short pull and grab away rejected; lever/hammer drawn from the hand, not the automatic animation; native lever fingers.");
 std::puts("PASS: 1887 held gun: every part at a fixed rest on the body (no self-animating pieces, any draw order, any head motion); left hand always the normal forend hold + INSERT adjustment (never floats); right hand always locked with the wrist on the gun; hand-offs snap with no transition; two-handed hold restored, LEFT grip + X takes the gun (X kept from the game); held shell drawn in the right hand; INSERT 1887 settings save and reload.");
 context->ClearState();vb->Release();ib->Release();globals->Release();camera->Release();
 g_freeHandAttachment[0]=oldFree;g_freeHandAttachment[1]=oldFreeLeft;g_lever=oldLever;g_weaponMechanicProfile=oldProfile;g_weaponMechanicFrame=g_frame;g_twoHand=false;g_worldScale=oldScale;fc3pad::BlockFire(false);fc3pad::RequireFullFirePull(false);fc3pad::SetLeverHandling(false);
}
