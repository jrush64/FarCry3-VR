#include "legacy_car366_arm.inl"
#include "legacy_vehicle362_grip.inl"
extern void DoorFixtureReference363();extern unsigned DoorFixtureStatus363();extern bool DoorFixtureExpired363();
void TestSteeringMath363(){
 using namespace fc3wheel352;
 auto filtered=[](int step){Wheel w;Input i;i.allowed=true;i.vehicle=12;i.model=0;i.now=100;i.radius=.2;i.smoothing363=.07;i.hand[0]={.2,0,0,1,true};w.Update(i);for(int elapsed=step;elapsed<=70;elapsed+=step){i.now=100+elapsed;i.hand[0].x=.2*cos(.2);i.hand[0].z=.2*sin(.2);w.Update(i);}return w;};
 const auto fine=filtered(5),coarse=filtered(10);Check(std::fabs(fine.angle-coarse.angle)<1e-6&&std::fabs(fine.angle-.2*(1-std::exp(-1.)))<1e-6,"363 time-based wheel smoothing is independent of sample rate and retains all accumulated physical turn");
 Wheel w=fine;Input i;i.allowed=true;i.vehicle=12;i.model=0;i.now=180;i.hand[0]={.2*cos(.2),0,.2*sin(.2),0,true};i.smoothing363=.07;Check(!w.Update(i)&&w.Demand(i.fullLock)==0,"363 wheel release immediately cancels demand without a smoothing tail");
 fc3bars355::Bars bars;fc3bars355::Input b;b.allowed=true;b.id=24;b.model=1;b.now=100;b.smoothing363=.07;b.end[0]={.35,0,.2};b.end[1]={-.35,0,.2};b.hand[0]={b.end[0],1,true};bars.Update(b);b.now=110;b.hand[0].p=fc3bars355::Turn(b.end[0],.2);bars.Update(b);Check(bars.angle>0&&bars.angle<.2&&std::fabs(bars.target363-.2)<1e-6,"363 bars smooth the common visible angle without losing raw turn");b.now+=10;b.hand[0].grip=0;Check(!bars.Update(b)&&bars.Demand(b.lock)==0,"363 handlebar release is immediate");
 fc3pad::SetVehicleSmoothing363(false,45);fc3pad::SetVehicleSmoothing363(true,70);Check(fc3pad::VehicleSmoothing363(false)==45&&fc3pad::VehicleSmoothing363(true)==70,"363 separate car/bar smoothing controls");fc3pad::SetVehicleSmoothing363(true,NAN);Check(fc3pad::VehicleSmoothing363(true)==70,"363 invalid smoothing cannot poison steering");fc3pad::SetCarWheelFullLockDegrees(360);Check(fc3pad::CarWheelFullLockDegrees()==360,"363 gentler car sensitivity permits360-degree full lock");fc3pad::SetCarWheelFullLockDegrees(90);
 puts("PASS:363 steering sensitivity bounds, frame-rate independent smoothing, accumulated raw input and immediate grip release");
}
void TestVehicleContact363(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 for(int side=0;side<2;++side){
  ArmJoints j={};j.valid=true;j.w[2]=1;Mat4 hand=HandIdentity(),twists[2]={};hand.m[0]=hand.m[5]=0;hand.m[1]=-1;hand.m[4]=1;
  g_armForearmRigid77[side]=HandIdentity();CarForearmTwist367(j,hand,HandIdentity(),side,twists);
  const double share[2]={.7,.35};for(int k=0;k<2;++k){
   const double a=fc3wheel352::Pi*.5*share[k];
   Check(std::fabs(twists[k].m[0]-cos(a))<1e-6&&std::fabs(twists[k].m[4]-sin(a))<1e-6,"367 hand roll distributes70/35 percent to both anatomical forearm skin bones");
   Check(Length(Sub(Apply(twists[k],{0,0,1}),V3{0,0,1}))<1e-6,"367 twist keeps wrist pivot fixed");
  }
 }
 // 365: the rim-centreline contact is measured with the hand offset at zero; the offset itself is tested in TestHandRimOffset365.
 g_handRimPullCm366[0]=15;g_handRimPullCm366[1]=-5;g_handRimOutCm366[0]=12;g_handRimOutCm366[1]=-8; //367 adversarial legacy sliders must not move the final fist off the wheel
 g_enabled=g_headTracking=g_inGameplay=g_eyeViewsSet=true;g_flatViewActive=false;g_pistolMenu.open=false;g_nativeVehicleCamera279=false;g_oCreateBuffer=MirrorCreate337;g_oUpdateSubresource=Update;
 for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
 const char* hlsl="row_major float3x4 _WorldMatrix;float3 _WorldPositionFractions;float4 main(float3 p:POSITION):SV_POSITION{return float4(mul(_WorldMatrix,float4(p,1))+_WorldPositionFractions,1);}";
 ID3DBlob *code=nullptr,*errors=nullptr;Check(SUCCEEDED(D3DCompile(hlsl,strlen(hlsl),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&code,&errors)),"363 real world/fractions shader compiles");SafeRelease(errors);ID3D11VertexShader* shader=nullptr;Check(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader)),"363 world shader");shader->SetPrivateData(kEjectionBytecodeGuid,UINT(code->GetBufferSize()),code->GetBufferPointer());SafeRelease(code);context->VSSetShader(shader,nullptr,0);context->VSSetConstantBuffers(0,4,cb);bool fractions=false;Check(WeaponWorldOffset(context,&fractions)==0&&fractions,"363 actual shader reflection declares world fractions");
 double worstOld=0,worstNew=0;
 for(int bars=0;bars<2;++bars)for(int side=0;side<2;++side)for(int step=0;step<4;++step){
  fc3aim::VehicleSample170 v={};v.vehicleId=42;v.pawnId=12;v.identityValid=v.seatValid=v.basisValid=true;v.type=bars?1:0;v.actualSeat=1;v.basis[0][0]=v.basis[1][1]=v.basis[2][2]=1;v.position[0]=32+step*.8f;v.position[1]=64+step*.4f;v.position[2]=12;
  fc3aim::SetVehicleStateForTest(1);fc3aim::SetVehicleSampleForTest187(&v);fc3pad::SetCarWheelSteeringEnabled(true);fc3pad::Controllers c={};c.active=true;if(side)c.leftGrip=1;else c.rightGrip=1;fc3pad::Publish(c);
  const double turn=.35*step;float globals[88]={};globals[0]=globals[5]=globals[10]=1;globals[3]=v.position[0];globals[7]=v.position[1];globals[11]=v.position[2];globals[12]=.23f;globals[13]=-.17f;globals[14]=.31f;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);
  CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;for(int k=0;k<3;++k)camera.position[k]=globals[k*4+3]+globals[12+k];cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);
  Mat4 cameraWorld=HandIdentity();for(int k=0;k<3;++k)cameraWorld.m[k*4+3]=camera.position[k];const Mat4 base=Multiply(CarPose352(v),bars?kBarAssets355[1].rest:kCarAssets352[1].rest);Mat4 inverse={};Check(Invert(base,inverse),"363 vehicle root inverse");
  g_car352={};g_barModel355=-1;g_barOwner355=0;g_carHand354={};g_barHand355[side]={};++g_frame;g_physicalAt352=GetTickCount64();g_physicalValid303[side]=true;for(int n=0;n<16;++n)g_physicalBody303[side][n]=float(HandIdentity().m[n]);
  const auto bind=V3{side?-.6781:.6781,.0519,1.0387};Mat4 nativeHand=HandIdentity();V3 targetLocal={};
  if(!bars){const auto& a=kCarAssets352[1];g_car352.vehicle=42;g_car352.model=1;g_car352.owner=side;g_car352.gripping[side]=true;g_car352.angle=g_car352.target363=turn;g_car352.gripOffset[side]=side?fc3wheel352::Pi:0;g_carFrame352=g_frame;g_carSeen352=GetTickCount64();g_carBodyToWheel353=HandIdentity();
   auto& p=g_carHand354;p.vehicle=42;p.model=1;p.epoch=g_armPoseEpoch;p.hand=step<3;p.canonicalLeft=HandIdentity();p.canonicalLeft.m[3]=a.radius+.05+.6781;p.canonicalLeft.m[7]=.0316-.0519;p.canonicalLeft.m[11]=-1.0387;
   const double polar=turn+g_car352.gripOffset[side];targetLocal=Add(a.hub,{a.radius*cos(polar),0,a.radius*sin(polar)});
  }else{const auto& a=kBarAssets355[1];g_barOwner355=42;g_barModel355=1;g_barTick355=g_frame;g_barBodyAt355=GetTickCount64();g_bars355={};g_bars355.held[side]=true;g_bars355.angle=g_bars355.target363=turn;g_bodyToBar355=HandIdentity();auto& p=g_barHand355[side];p.id=42;p.valid=step<3;for(auto& f:p.finger)f=HandIdentity();p.hand=HandIdentity();const auto displaced=Add(a.grip[side],{.08,.05,0});p.hand.m[3]=displaced.x-bind.x;p.hand.m[7]=displaced.y-bind.y;p.hand.m[11]=displaced.z-bind.z;targetLocal=Apply(BarTurn355(turn),a.grip[side]);}
  float raw[576]={};for(int b=0;b<48;++b)StoreBone(raw,b,HandIdentity());memcpy(raw,kNeutralArmView[side],sizeof(kNeutralArmView[side]));cb[3]->SetPrivateData(kCbCopyGuid,sizeof(raw),raw);
  Check(BuildCarArm352(context,side)!=nullptr,"363 actual seated-arm palette builds with moving vehicle/fractional world");float skin[576]={};Read(device,context,g_armPaletteCb[side],skin);
  Mat4 actualWorld=HandIdentity();for(int r=0;r<3;++r)for(int k=0;k<4;++k)actualWorld.m[r*4+k]=globals[r*4+k]+(k==3?globals[12+r]:0);
  V3 skinnedGrip367=Apply(BoneMatrix(skin,2),bind);
  if(!bars){ // Independent four-joint skin witness from uploaded GPU palette.
   const double mirror=side?-1.:1.;skinnedGrip367={};
   const int bone367[4]={7,9,10,12};const V3 point367[4]={{.658*mirror,.076,1.079},{.687*mirror,.085,1.018},{.671*mirror,.050,1.080},{.699*mirror,.057,1.007}};
   for(int j=0;j<4;++j)skinnedGrip367=Add(skinnedGrip367,Scale(Apply(BoneMatrix(skin,bone367[j]),point367[j]),.25));
   // Wrist seam and hand-roll correction must coexist with the contact constraint.
   const V3 wrist367={.591*mirror,.011,1.149};
   const double seam367=Length(Sub(Apply(BoneMatrix(skin,20),wrist367),Apply(BoneMatrix(skin,2),wrist367)));
   if(seam367>.012)printf("367 seam side%d step%d gap%.6f\n",side,step,seam367);
   Check(seam367<.012,"367 wrist seam stays within the native7.85mm gap and existing1.35 stretch allowance");
  }
  const V3 actual=Apply(actualWorld,skinnedGrip367),expected=Apply(base,targetLocal);const double error=Length(Sub(actual,expected));worstNew=(std::max)(worstNew,error);if(error>.003)printf("363 contact bars%d side%d step%d error%.6f\n",bars,side,step,error);Check(error<.003,"363 actual uploaded hand stays on visible current wheel/bar through rotation and vehicle movement");
  if(!bars){
   Check(LegacyBuildCarArm366(context,side)!=nullptr,"367 unmodified366 car-arm witness builds");
   float oldSkin367[576]={};Read(device,context,g_armPaletteCb[side],oldSkin367);
   V3 oldGrip367={};const double mirror=side?-1.:1.;const int bi[4]={7,9,10,12};const V3 bp[4]={{.658*mirror,.076,1.079},{.687*mirror,.085,1.018},{.671*mirror,.050,1.080},{.699*mirror,.057,1.007}};
   for(int j=0;j<4;++j)oldGrip367=Add(oldGrip367,Scale(Apply(BoneMatrix(oldSkin367,bi[j]),bp[j]),.25));
   const double oldError367=Length(Sub(Apply(actualWorld,oldGrip367),expected));
   Check(oldError367>.02,"367 old366 fixed-point/slider placement fails the actual curled-finger contact check");
   printf("PASS:367 car side%d step%d old366 posed-grip error %.5fm new %.6fm\n",side,step,oldError367,error);
  }
  Mat4 legacy={};const bool ready=bars?LegacyBarGripMove363(42,side,HandIdentity(),legacy):LegacyCarGripMove363(42,side,HandIdentity(),legacy);Check(ready==bool(step<3),"363 native-template availability is independent of fallback contact locking");const auto oldWorld=Apply(cameraWorld,Apply(legacy,bind));if(ready)worstOld=(std::max)(worstOld,Length(Sub(oldWorld,expected)));
 }
 Check(worstOld>.04&&worstNew<.003,"363 old captured-pose offset reproduces air steering; corrected contact remains within3mm");printf("PASS:363 old native-pose contact error %.5fm -> corrected GPU/world contact %.6fm; both hands, sedan/jetski, moving body and world fractions\n",worstOld,worstNew);
 g_car352={};g_carHand354={};g_barModel355=-1;g_barOwner355=0;g_bars355={};context->VSSetShader(nullptr,nullptr,0);shader->Release();fc3aim::SetVehicleStateForTest(0);
}
void TestDoor363(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 TestDoor357(device,context,cb);DoorFixtureReference363();SetRenderHands303(0,true,g_armPoseEpoch);g_door357[0]=g_door357[1]={};g_doorFrame357[0]=g_doorFrame357[1]=-100;g_doorModel363=-1;
 const auto& asset=kDoorAssets363[1];const auto it=std::find_if(std::begin(kDoorDraws363),std::end(kDoorDraws363),[](const DoorDraw363& d){return d.model==1&&d.start==27870;});Check(it!=std::end(kDoorDraws363),"363 tested sedan02 exterior handle draw exists");
 auto* vb=Buffer(device,it->vb,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,it->ib,D3D11_BIND_INDEX_BUFFER);UINT stride=32,zero=0;context->IASetVertexBuffers(0,1,&vb,&stride,&zero);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);context->VSSetConstantBuffers(0,4,cb);context->VSSetShader(nullptr,nullptr,0);float rows[88]={};rows[0]=rows[5]=rows[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);CameraSummary camera={};camera.mainView=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);for(auto& e:g_eyeViewsBody){memset(e.view,0,sizeof(e.view));e.view[0]=e.view[5]=e.view[10]=e.view[15]=1;}
 fc3pad::Controllers c={};c.active=true;DrawCall draw={0,it->count,it->start,0,0,0};
 auto poll=[&](float grip,double pull){SeedFoot357();c.rightGrip=grip;fc3pad::Publish(c);auto hand=HandIdentity();hand.m[3]=asset.handle.x-pull;hand.m[7]=asset.handle.y+.015;hand.m[11]=asset.handle.z-.015;for(int n=0;n<16;++n)g_physicalBody303[0][n]=float(hand.m[n]);g_physicalValid303[0]=true;g_physicalValid303[1]=false;g_physicalAt352=GetTickCount64();++g_frame;ObserveCarDoor357(context,draw,0);};
 fc3pad::SetThrowableInput144(true,true);poll(0,0);poll(0,0);poll(1,0);Check(!g_door357[0].held&&DoorFixtureStatus363()==0,"363 an existing throwable owner cannot share the door grip or highlight");fc3pad::SetThrowableInput144(false,false);
 poll(0,0);poll(0,0);Check(DoorFixtureStatus363()==1,"363 door02 approach publishes cyan door/handle outline even with weapon guides disabled");poll(1,0);Check(DoorFixtureStatus363()==3&&g_door357[0].held,"363 accepted door02 grip publishes green highlight");poll(1,.085);Check(g_door357[0].spent&&(fc3pad::ButtonsOutput()&0x4000),"363 actual tested sedan02 grip/pull reaches native entry input");Check(DoorFixtureExpired363(),"363 missing current door draws expire highlight");fc3pad::SetDoorInput357(false,false);vb->Release();ib->Release();puts("PASS:363 both sedan identities, actual door02 contact/grip/entry input, distinct hover/held outline state and stale visibility expiry");
}
