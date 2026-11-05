void TestCarMath352(){
 using namespace fc3wheel352;
 Input in;in.vehicle=42;in.model=0;in.allowed=true;in.now=1000;in.radius=.2;in.fullLock=Pi/2;
 in.hand[0]={.2,0,0,1,true};Wheel w;
 Check(w.Update(in)&&w.owner==0&&w.angle==0,"352 rim grip acquires one physical hand without steering jump");
 in.now+=10;in.hand[0].x=.2*std::cos(.4);in.hand[0].z=.2*std::sin(.4);
 Check(w.Update(in)&&std::fabs(w.angle-.4)<1e-10&&w.Demand(in.fullLock)<-.2,"352 counterclockwise rim turn steers left");
 in.hand[1]={-.2,0,0,1,true};in.now+=10;
 Check(w.Update(in)&&w.owner==0,"352 second hand cannot steal the acquired wheel");
 in.hand[0].grip=0;in.now+=10;Check(w.Update(in)&&w.owner==1,"353 releasing the first hand retains the other attached hand");
 in.hand[1].grip=0;in.now+=10;w.Update(in);in.hand[1].grip=1;in.nativeAngle=.4;in.now+=10;
 Check(w.Update(in)&&w.owner==1&&std::fabs(w.angle-.4)<1e-10,"352 left hand regrips current wheel without resetting");
 in.hand[1].x=-.2*std::cos(.3);in.hand[1].z=-.2*std::sin(.3);in.now+=10;
 Check(w.Update(in)&&std::fabs(w.angle-.7)<1e-10,"352 polar unwrap crosses the minus pi boundary");
 // 364: sticky grip. Tracking loss, depth, radius, stalls and glitches pause the hand; only the grip button or the context releases it.
 in.hand[1].tracked=false;in.now+=10;Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-.7)<1e-10&&!w.released364[1],"364 tracking loss holds the grip and the wheel angle");
 in.hand[1].tracked=true;in.hand[1].x=-.2*std::cos(.5);in.hand[1].z=-.2*std::sin(.5);in.now+=10;
 Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-.7)<1e-10,"364 tracking recovery re-anchors without snapping the wheel");
 in.hand[1].x=-.2*std::cos(.6);in.hand[1].z=-.2*std::sin(.6);in.now+=10;Check(w.Update(in)&&std::fabs(w.angle-.8)<1e-10,"364 steering resumes from the re-anchored contact");
 in.hand[1].y=-.4;in.now+=10;Check(w.Update(in)&&w.gripping[1]&&!w.released364[1],"364 a hand 40 cm behind the rim plane stays attached");
 in.hand[1].x=-.5*std::cos(.7);in.hand[1].z=-.5*std::sin(.7);in.now+=10;Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-.9)<1e-10,"364 a hand far outside the rim stays attached and still steers by its polar angle");
 in.hand[1].x=-.03;in.hand[1].z=0;in.now+=10;Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-.9)<1e-10,"364 a hand over the hub holds the angle instead of releasing");
 in.hand[1].x=-.2*std::cos(.7);in.hand[1].z=-.2*std::sin(.7);in.hand[1].y=0;in.now+=10;Check(w.Update(in)&&std::fabs(w.angle-.9)<1e-10,"364 leaving the hub re-anchors without a delta");
 in.hand[1].x=.2*std::cos(.7);in.hand[1].z=.2*std::sin(.7);in.now+=10;Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-.9)<1e-10,"364 a one-sample half-turn glitch is ignored, grip kept");
 in.hand[1].x=.2*std::cos(.8);in.hand[1].z=.2*std::sin(.8);in.now+=10;Check(w.Update(in)&&std::fabs(w.angle-1.0)<1e-10,"364 steering continues after the glitch");
 in.now+=900;in.hand[1].x=.2*std::cos(.9);in.hand[1].z=.2*std::sin(.9);Check(w.Update(in)&&w.gripping[1]&&std::fabs(w.angle-1.1)<1e-10,"364 a 900 ms frame stall keeps the grip and applies the real movement");
 in.hand[1].grip=.2f;in.now+=10;Check(!w.Update(in)&&w.released364[1]==fc3wheel352::kGrip&&w.release363[1]==fc3wheel352::kGrip,"364 letting the grip button go is the release, reported that update");
 in.hand[1].grip=1;in.now+=10;Check(w.Update(in)&&w.gripping[1]&&!w.released364[1],"364 squeezing again regrips at the rim");
 in.allowed=false;in.now+=10;Check(!w.Update(in)&&w.released364[1]==fc3wheel352::kContext,"352 menus and seat exits revoke ownership, reported that update");
 in.allowed=true;in.now+=10;Check(!w.Update(in)&&!w.released364[1],"352 returning from menu does not reacquire held grip");
 in.hand[1].grip=0;in.now+=10;w.Update(in);in.hand[1].grip=1;in.hand[1].y=.3;in.now+=10;
 Check(!w.Update(in),"364 grip 30 cm behind the wheel plane cannot acquire");
 in.hand[1].y=-.15;in.now+=10;Check(w.Update(in),"364 a grip within 20 cm of the rim plane acquires");
 in.hand[1].grip=0;in.now+=10;w.Update(in);in.hand[1].grip=1;in.hand[1].y=0;in.hand[1].x=.05;in.hand[1].z=0;in.now+=10;
 Check(!w.Update(in),"352 wheel center is not a rim grab");
 in.hand[1].x=.3;in.now+=10;Check(w.Update(in),"364 a grip 10 cm outside the rim acquires");
 in.hand[1].grip=0;in.now+=10;w.Update(in);in.hand[1].grip=1;in.hand[1].x=.2;in.hand[1].y=std::nan("");in.now+=10;Check(!w.Update(in),"352 nonfinite controller point is rejected");
 // 365: drawn-wrist offset from the rim centreline, wheel space: pull = -Y (toward the driver), out = along the rim radius.
 g_handRimPullCm366[1]=4;g_handRimOutCm366[1]=1;g_handRimPullCm366[0]=2;g_handRimOutCm366[0]=6;const V3 o0=HandRimOffset365(0,1),o90=HandRimOffset365(Pi/2,1),r0=HandRimOffset365(0,0);
 Check(std::fabs(o0.x-.01)<1e-9&&std::fabs(o0.y+.04)<1e-9&&std::fabs(o0.z)<1e-9&&std::fabs(o90.x)<1e-9&&std::fabs(o90.z-.01)<1e-9&&std::fabs(r0.x-.06)<1e-9&&std::fabs(r0.y+.02)<1e-9,"365 hand-on-rim offset follows the rim angle and pulls toward the driver, per hand");
 g_handRimPullCm366[0]=g_handRimPullCm366[1]=g_handRimOutCm366[0]=g_handRimOutCm366[1]=0;Check(Length(HandRimOffset365(1.2,1))==0&&Length(HandRimOffset365(1.2,0))==0,"365 zero offset reproduces the 363 centreline contact");g_handRimPullCm366[0]=g_handRimPullCm366[1]=4;g_handRimOutCm366[0]=6;g_handRimOutCm366[1]=1;
 puts("PASS:353 real-rim math, cooperative hand ownership, no snap, angular wrap, 364 sticky grip: grip-button/context release only, 365 rim offset");
}
void TestCarGpu352(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 fc3pad::SetVehicleSmoothing363(false,0);TestCarMath352();
 g_enabled=g_headTracking=g_inGameplay=g_eyeViewsSet=true;g_flatViewActive=false;g_pistolMenu.open=false;
 fc3aim::SetVehicleStateForTest(1);fc3aim::VehicleSample170 sample={};sample.identityValid=sample.seatValid=sample.basisValid=true;
 sample.vehicleId=42;sample.pawnId=24;sample.type=0;sample.actualSeat=1;sample.basis[0][0]=sample.basis[1][1]=sample.basis[2][2]=1;
 fc3aim::SetVehicleSampleForTest187(&sample);fc3pad::SetCarWheelSteeringEnabled(true);fc3pad::SetCarWheelFullLockDegrees(90);
 fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
 g_oCreateBuffer=MirrorCreate337;g_car352={};g_carFrame352=-100;
 auto* wheelVb=Buffer(device,716640,D3D11_BIND_VERTEX_BUFFER);auto* wheelIb=Buffer(device,126360,D3D11_BIND_INDEX_BUFFER);
 UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&wheelVb,&stride,&offset);context->IASetIndexBuffer(wheelIb,DXGI_FORMAT_R16_UINT,0);
 context->VSSetConstantBuffers(0,4,cb);context->VSSetShader(nullptr,nullptr,0);
 const auto& asset=kCarAssets352[0];const DrawCall draw={0,684,36612,0,0,0};
 auto seed=[&](double nativeAngle,double headAngle=0.){
  Mat4 turn=HandIdentity();turn.m[0]=turn.m[10]=std::cos(nativeAngle);turn.m[2]=-std::sin(nativeAngle);turn.m[8]=std::sin(nativeAngle);
  const Mat4 native=Multiply(asset.rest,turn);float rows[88]={};for(int i=0;i<12;++i)rows[i]=float(native.m[i]);rows[32]=123.25f;
  cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
  CameraSummary cam={};cam.mainView=1;cam.right[0]=float(std::cos(headAngle));cam.right[2]=float(-std::sin(headAngle));cam.up[1]=1;cam.forward[0]=float(std::sin(headAngle));cam.forward[2]=float(std::cos(headAngle));
  cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);
  for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=cam.right[0];eye.view[2]=cam.right[2];eye.view[5]=1;eye.view[8]=cam.forward[0];eye.view[10]=cam.forward[2];eye.view[15]=1;}
 };
 auto hand=[&](int side,double a){Mat4 pose=HandIdentity();const V3 p=Apply(asset.rest,Add(asset.hub,{asset.radius*std::cos(a),0,asset.radius*std::sin(a)}));
  pose.m[3]=p.x;pose.m[7]=p.y+.015;pose.m[11]=p.z-.015;
  for(int i=0;i<16;++i)g_physicalBody303[side][i]=float(pose.m[i]);g_physicalValid303[side]=true;g_physicalAt352=GetTickCount64();};
 seed(0);hand(0,0);g_physicalValid303[1]=false;++g_frame;
 Check(!BuildCarWheelWorld352(context,draw,0),"352 ungripped wheel keeps its native draw");
 controls.rightGrip=1;fc3pad::Publish(controls);++g_frame;
 ID3D11Buffer* result=BuildCarWheelWorld352(context,draw,0);Check(result&&g_car352.owner==0,"352 actual car draw acquires right rim grip");
 Check(fc3pad::SteeringOutput()==0,"352 input receives a neutral current-angle grab");
 auto TriggerByte365=[](float v){return static_cast<unsigned char>((std::max)(0.f,(std::min)(1.f,v))*255.f+.5f);};
 // 365: left stick Y becomes accelerator/brake in the driver seat; the game never sees the raw axis.
 fc3pad::SetStickThrottle365(true);controls.leftY=.56f;fc3pad::Publish(controls);
 Check(fc3pad::RightTriggerOutput365()==TriggerByte365((.56f-.12f)/.88f)&&fc3pad::LeftTriggerOutput365()==0&&fc3pad::ThumbLYOutput365()==0,"365 stick up = proportional accelerator, stick Y hidden from the game");
 controls.leftY=-1.f;fc3pad::Publish(controls);
 Check(fc3pad::LeftTriggerOutput365()==255&&fc3pad::RightTriggerOutput365()==0,"365 stick down = full brake/reverse");
 controls.leftY=.05f;controls.rightTrigger=.4f;fc3pad::Publish(controls);
 Check(fc3pad::RightTriggerOutput365()==TriggerByte365(.4f)&&fc3pad::ThumbLYOutput365()==0,"365 inside the dead zone the real trigger still drives");
 fc3pad::SetStickThrottle365(false);controls.leftY=.8f;controls.rightTrigger=0;fc3pad::Publish(controls);
 Check(fc3pad::RightTriggerOutput365()==0&&fc3pad::ThumbLYOutput365()!=0,"365 setting off restores the native stick axis");
 fc3pad::SetStickThrottle365(true);controls.leftY=0;fc3pad::Publish(controls);++g_frame;BuildCarWheelWorld352(context,draw,0);
 seed(-.6,.6);hand(0,.3);++g_frame;result=BuildCarWheelWorld352(context,draw,0);
 Check(result&&std::fabs(g_car352.angle-.3)<1e-5&&fc3pad::SteeringOutput()<-5000,"352 real camera turn cancels out, animated native wheel cannot steer controller");
 float rows[88]={};Read(device,context,result,rows);Mat4 expected=HandIdentity();expected.m[0]=expected.m[10]=std::cos(.3);expected.m[2]=-std::sin(.3);expected.m[8]=std::sin(.3);
 const V3 moved=Apply(expected,asset.hub);expected.m[3]=asset.hub.x-moved.x;expected.m[7]=asset.hub.y-moved.y;expected.m[11]=asset.hub.z-moved.z;expected=Multiply(asset.rest,expected);
 for(int i=0;i<12;++i)Check(std::fabs(rows[i]-expected.m[i])<2e-5,"352 GPU wheel transform follows controlled axle, not native animation");
 Check(rows[32]==123.25f,"352 unrelated world constants preserved");
 // Driving arms come from fixed neutral bones and physical controls, not
 // Jason's native wheel animation or whichever weapon hand was dominant.
 hand(1,2.8);float armA[576]={},armB[576]={};
 const bool oldAim=g_aimActive;g_aimActive=true;
 for(int side=0;side<2;++side){
  Check(BuildCarArm352(context,side)!=nullptr,"352 car arms have their own tracked path");
  Read(device,context,g_armPaletteCb[side],armA);
  float animated[576]={};for(int b=0;b<48;++b){Mat4 m=HandIdentity();m.m[3]=b*.13;StoreBone(animated,b,m);}
  cb[3]->SetPrivateData(kCbCopyGuid,sizeof(animated),animated);
  Check(BuildCarArm352(context,side)!=nullptr,"352 arm path accepts changing native driving animation");
  Read(device,context,g_armPaletteCb[side],armB);Compare(armA,armB,25*12,"352 both physical arms ignore native driving animation");
 }
 g_aimActive=oldAim;
 const DrawCall secondPart={0,1926,37296,0,0,0};Check(BuildCarWheelWorld352(context,secondPart,0)!=nullptr&&std::fabs(g_car352.angle-.3)<1e-5,"352 material parts share one update per frame");
 const DrawCall wrongPart={0,792,0,0,0,0};Check(!BuildCarWheelWorld352(context,wrongPart,0),"352 chassis draw cannot be moved or acquired");
 Check(!BuildCarWheelWorld352(context,draw,1),"352 first-person camera variant cannot be treated as world wheel");
 float other[88]={};for(int i=0;i<12;++i)other[i]=float(asset.rest.m[i]);other[3]+=5;
 cb[0]->SetPrivateData(kCbCopyGuid,sizeof(other),other);Check(!BuildCarWheelWorld352(context,draw,0),"352 same mesh on another car remains untouched");
 seed(-.6,.6);controls.leftX=.6f;fc3pad::Publish(controls);++g_frame;
 Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&g_car352.owner==0&&fc3pad::SteeringOutput()>19000,"364 explicit stick motion steers while held but can no longer throw the hand off the rim");
 controls.leftX=0;fc3pad::Publish(controls);++g_frame;
 Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&g_car352.owner==0&&fc3pad::SteeringOutput()<-5000,"364 stick release hands steering straight back to the still-held wheel");
 g_physicalAt352=GetTickCount64()-300;++g_frame;
 Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&g_car352.owner==0&&g_car352.paused364[0],"364 a 300 ms old physical pose pauses the hand instead of dropping it");
 hand(0,.3);++g_frame;Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&!g_car352.paused364[0]&&std::fabs(g_car352.angle-.3)<1e-5,"364 fresh pose resumes without a snap");
 // 366: a stale vehicle sample pauses the wheel instead of dropping the hands.
 fc3aim::SetVehicleSampleForTest187(nullptr);++g_frame;
 Check(!BuildCarWheelWorld352(context,draw,0)&&g_car352.owner==0&&g_car352.gripping[0]&&g_carDeniedSince366!=0,"366 stale vehicle sample holds the grip (pause, not release)");
 fc3aim::SetVehicleSampleForTest187(&sample);hand(0,.3);++g_frame;
 Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&g_car352.owner==0&&g_carDeniedSince366==0&&std::fabs(g_car352.angle-.3)<1e-5,"366 sample back: same grip, same angle");
 fc3aim::SetVehicleSampleForTest187(nullptr);++g_frame;BuildCarWheelWorld352(context,draw,0);g_carDeniedSince366=GetTickCount64()-1600;++g_frame;
 Check(!BuildCarWheelWorld352(context,draw,0)&&g_car352.owner<0&&g_car352.release363[0]==fc3wheel352::kContext,"366 a sample stale for 1.5 s finally releases with reason 6");
 fc3aim::SetVehicleSampleForTest187(&sample);controls.rightGrip=0;fc3pad::Publish(controls);++g_frame;BuildCarWheelWorld352(context,draw,0);controls.rightGrip=1;fc3pad::Publish(controls);++g_frame;
 Check(BuildCarWheelWorld352(context,draw,0)!=nullptr&&g_car352.owner==0,"366 regrip after the release");
 controls.leftX=0;controls.rightGrip=0;fc3pad::Publish(controls);++g_frame;BuildCarWheelWorld352(context,draw,0);
 Check(g_car352.owner<0&&g_car352.release363[0]==fc3wheel352::kGrip,"364 grip button release is the release");
 sample.actualSeat=2;fc3aim::SetVehicleSampleForTest187(&sample);controls.rightGrip=1;fc3pad::Publish(controls);++g_frame;
 Check(!BuildCarWheelWorld352(context,draw,0)&&fc3pad::SteeringOutput()==0,"352 passenger cannot drive");
 sample.actualSeat=1;sample.type=1;fc3aim::SetVehicleSampleForTest187(&sample);++g_frame;
 Check(!BuildCarWheelWorld352(context,draw,0),"352 water vehicles cannot activate car wheel");
 fc3aim::SetVehicleSampleForTest187(nullptr);fc3aim::SetVehicleStateForTest(0);fc3pad::SetCarWheelSteeringEnabled(false);
 context->IASetIndexBuffer(nullptr,DXGI_FORMAT_UNKNOWN,0);wheelVb->Release();wheelIb->Release();
 for(auto*& b:g_carCb352)SafeRelease(b);memset(g_carCbBytes352,0,sizeof(g_carCbBytes352));
 puts("PASS:352 actual WARP world upload, head-turn cancellation, fixed axle, native animation isolation, current-car identity and pad steering");
}
