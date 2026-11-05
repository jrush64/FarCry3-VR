void TestBarsMath355(){
 using namespace fc3bars355;Input i;i.id=1;i.model=0;i.allowed=true;i.now=100;i.end[0]={.4,-.1,.2};i.end[1]={-.4,-.1,.2};
 i.hand[0]={i.end[0],1,true};i.hand[1]={i.end[1],0,true};Bars b;
 Check(b.Update(i)&&b.held[0]&&b.angle==0,"355 right handle grabs without recentering");
 i.now+=10;i.hand[0].p=Turn(i.end[0],.2);Check(b.Update(i)&&std::fabs(b.angle-.2)<1e-9&&b.Demand(i.lock)<0,"355 handlebars turn around stem localZ and steer left");
 i.hand[1].p=Turn(i.end[1],.2);i.hand[1].grip=1;i.now+=10;
 Check(b.Update(i)&&b.held[0]&&b.held[1]&&std::fabs(b.angle-.2)<1e-9,"355 second handle joins without a steering jump");
 for(int h=0;h<2;++h)i.hand[h].p=Turn(i.end[h],.35);i.now+=10;Check(b.Update(i)&&std::fabs(b.angle-.35)<1e-9,"355 both hands share one stem angle");
 i.hand[0].grip=0;i.now+=10;Check(b.Update(i)&&!b.held[0]&&b.held[1],"355 one release leaves the other handle steering");
 i.hand[1].tracked=false;i.now+=10;Check(!b.Update(i),"355 tracking loss drops the last handle");
 i.hand[1].tracked=true;i.now+=10;Check(!b.Update(i),"355 tracking recovery needs release and regrip");
 i.hand[1].grip=0;i.now+=10;b.Update(i);i.hand[1].grip=1;i.hand[1].p=i.end[1];i.nativeAngle=0;i.now+=10;Check(b.Update(i),"355 tracking recovery rearms after release");
 i.allowed=false;i.now+=10;Check(!b.Update(i),"355 menu/analog ownership drops handles");
 i.allowed=true;i.now+=10;Check(!b.Update(i),"355 held grip cannot silently resume after analog override");
 Bars center;i.hand[0]={{0,0,.2},1,true};i.hand[1].grip=0;i.now+=10;Check(!center.Update(i),"355 steering stem center is not a handle grip");
 i.hand[0].p=i.end[1];i.now+=10;Check(!center.Update(i),"355 right hand cannot acquire the opposite side accidentally");
 i.hand[0].p=i.end[0];i.now+=10;Check(center.Update(i),"355 identified matching handle is reachable");
 i.now+=151;Check(!center.Update(i),"355 stale tracking/draw publication releases steering");
 puts("PASS:355 handle-only contact, stem steering, two-hand join/release, grip recovery, analog/menu/stale guards");
}
void TestBarPose355(){
 const Mat4 identity=HandIdentity();g_barModel355=0;g_barOwner355=123;g_barBodyAt355=g_barSeen355=GetTickCount64();g_bodyToBar355=identity;
 g_bars355={};g_bars355.held[0]=g_bars355.held[1]=true;g_bars355.angle=.3;g_barNativeAngle355=.2;
 for(auto& e:g_eyeViewsBody){memset(e.view,0,sizeof(e.view));e.view[0]=e.view[5]=e.view[10]=e.view[15]=1;}
 for(int side=0;side<2;++side){g_barHand355[side]={};const V3 grip=kBarAssets355[0].grip[side];Mat4 rest=identity;
  rest.m[3]=grip.x-(side?-.6781:.6781);rest.m[7]=grip.y-.0519;rest.m[11]=grip.z-1.0387;
  const Mat4 native=Multiply(BarTurn355(.2),rest);float bones[576]={};for(int b=0;b<48;++b)StoreBone(bones,b,identity);StoreBone(bones,2,native);
  for(int b=3;b<=19;++b){Mat4 finger=identity;finger.m[3]=b*.001;StoreBone(bones,b,Multiply(native,finger));}
  for(int sample=0;sample<6;++sample){++g_frame;CaptureBarHand355(123,side,identity,bones);}
  Check(g_barHand355[side].valid,"355 actual native handle grip captures independently for each anatomical hand");
  Mat4 move={};Check(BarGripMove355(123,side,identity,move),"355 native wrist follows controlled handlebar");
  const Mat4 expected=Multiply(BarTurn355(.3),rest);for(int i=0;i<12;++i)Check(std::fabs(move.m[i]-expected.m[i])<2e-6,"355 wrist retains native handle offset while turning");
  StoreBone(bones,2,move);Check(BarGripFingers355(123,side,bones),"355 native finger articulation follows its own wrist");
 }
 g_bars355.Drop();float unused[576]={};Check(!BarGripFingers355(123,0,unused),"355 release restores free fingers");
 g_barOwner355=0;g_barModel355=-1;
 puts("PASS:355 native left/right handle poses, canonical stem rotation and free-hand release");
}
void TestBarGpu355(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 fc3pad::SetVehicleSmoothing363(true,0);
 TestBarsMath355();TestBarPose355();g_oCreateBuffer=MirrorCreate337;
 g_enabled=g_headTracking=g_inGameplay=g_eyeViewsSet=true;g_flatViewActive=false;g_pistolMenu.open=false;
 fc3pad::SetHandlebarEnabled355(true);fc3pad::SetHandlebarLock355(35);fc3aim::SetVehicleStateForTest(1);
 for(int model=0;model<2;++model){
  const auto& asset=kBarAssets355[model];const auto d=*std::find_if(std::begin(kBarDraws355),std::end(kBarDraws355),[&](const BarDraw355& x){return x.model==unsigned(model);});
  auto* vb=Buffer(device,d.vb,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,d.ib,D3D11_BIND_INDEX_BUFFER);UINT stride=32,offset=0;
  context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);context->VSSetShader(nullptr,nullptr,0);context->VSSetConstantBuffers(0,4,cb);
  fc3aim::VehicleSample170 v={};v.vehicleId=123;v.pawnId=12;v.type=asset.type;v.actualSeat=1;v.identityValid=v.seatValid=v.basisValid=true;
  v.basis[0][0]=v.basis[1][1]=v.basis[2][2]=1;fc3aim::SetVehicleSampleForTest187(&v);
  fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);g_barOwner355=0;g_barModel355=-1;g_barTick355=-100;g_bars355={};
  CameraSummary camera={};camera.mainView=1;camera.xScale=camera.yScale=1;camera.right[0]=camera.up[1]=camera.forward[2]=1;
  auto seed=[&](double angle){const Mat4 native=Multiply(asset.rest,BarTurn355(angle));float rows[88]={};for(int i=0;i<12;++i)rows[i]=float(native.m[i]);rows[40]=91.25f;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(camera),&camera);};
  auto hands=[&](double angle){for(int h=0;h<2;++h){const V3 p=Apply(asset.rest,Apply(BarTurn355(angle),asset.grip[h]));Mat4 pose=HandIdentity();pose.m[3]=p.x;pose.m[7]=p.y+.015;pose.m[11]=p.z-.015;
   for(int i=0;i<16;++i)g_physicalBody303[h][i]=float(pose.m[i]);g_physicalValid303[h]=true;}g_physicalAt352=GetTickCount64();};
  for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
  const DrawCall draw={0,d.count,d.start,0,0,0};seed(0);hands(0);++g_frame;
  Check(!BuildHandlebarWorld355(context,draw,0)&&g_barModel355==model,"355 exact native model matches before a grip; unheld mesh stays native");
  fc3pad::Controllers queried={};fc3aim::VehicleSample170 native={};Check(CarHandsAllowed353(native,queried),"355 identified jetski/ATV driver has free tracked hands without holding handles");
  controls.leftGrip=controls.rightGrip=1;fc3pad::Publish(controls);++g_frame;hands(0);
  ID3D11Buffer* result=BuildHandlebarWorld355(context,draw,0);Check(result&&g_bars355.held[0]&&g_bars355.held[1],"355 both actual handles acquire on their tilted native steering assembly");
  hands(.25);seed(-.2);++g_frame;result=BuildHandlebarWorld355(context,draw,0);Check(result&&std::fabs(g_bars355.angle-.25)<1e-5,"355 controller path replaces animated native bar rotation");
  Check(fc3pad::SteeringOutput()<-12000,"355 actual pad encoder receives handlebar steering for native type0 or1");
  float rows[88]={};Read(device,context,result,rows);const Mat4 expected=Multiply(asset.rest,BarTurn355(.25));
  for(int i=0;i<12;++i)Check(std::fabs(rows[i]-expected.m[i])<2e-5,"355 actual GPU bar transform rotates around tilted localZ stem");
  Check(rows[40]==91.25f,"355 unrelated world constants remain intact");
  float otherRows[88]={};for(int i=0;i<12;++i)otherRows[i]=float(asset.rest.m[i]);otherRows[3]+=4;
  cb[0]->SetPrivateData(kCbCopyGuid,sizeof(otherRows),otherRows);
  Check(!BuildHandlebarWorld355(context,draw,0)&&g_barOwner355==123,"355 same-model parked vehicle cannot receive controlled bar transform or steal driver identity");
  otherRows[0]=std::nanf("");cb[0]->SetPrivateData(kCbCopyGuid,sizeof(otherRows),otherRows);
  Check(!BuildHandlebarWorld355(context,draw,0),"355 nonfinite native world matrices cannot bind or move handlebars");
  seed(-.2);Check(!BuildHandlebarWorld355(context,draw,1),"355 bars cannot acquire first-person weapon camera");
  fc3pad::SetHandlebarInput355(999,asset.type,.5f);Check(fc3pad::SteeringOutput()==0,"355 wrong full vehicle identity cannot steer");
  fc3pad::SetHandlebarInput355(123,1-asset.type,.5f);Check(fc3pad::SteeringOutput()==0,"355 wrong native vehicle type cannot consume bar lease");
  fc3pad::SetHandlebarInput355(123,asset.type,.5f);Check(fc3pad::SteeringOutput()>16000,"355 matching native type consumes its separate bar lease");
  // No handlebar draw: fresh main-world camera alone must maintain steering.
  const double look=.6;camera.right[0]=camera.forward[2]=float(std::cos(look));camera.right[2]=float(-std::sin(look));camera.forward[0]=float(std::sin(look));
  for(auto& eye:g_eyeViewsBody){eye.view[0]=camera.right[0];eye.view[2]=camera.right[2];eye.view[8]=camera.forward[0];eye.view[10]=camera.forward[2];}
  ++g_frame;hands(.25);TickHandlebars355(camera);
  Check(g_bars355.Active()&&std::fabs(g_bars355.angle-.25)<1e-5&&fc3pad::SteeringOutput()<-12000,"355 looking behind/out of bar visibility keeps the grip and does not steer from head motion");
  Sleep(125);Check(fc3pad::SteeringOutput()==0,"355 render stall expires steering lease without another publication");
  controls.rightTrigger=.8f;controls.leftTrigger=.4f;fc3pad::Publish(controls);
  Check(fc3pad::FireOutput()>200&&fc3pad::AimOutput()>100,"355 native accelerator/brake triggers remain available while gripping");
  controls.leftX=.7f;fc3pad::Publish(controls);++g_frame;TickHandlebars355(camera);
  Check(!g_bars355.Active()&&fc3pad::SteeringOutput()>22000,"355 native analog steering takes priority immediately");
  controls.leftX=0;fc3pad::Publish(controls);++g_frame;TickHandlebars355(camera);Check(!g_bars355.Active(),"355 stick release cannot reacquire a still-held grip");
  controls.leftGrip=controls.rightGrip=0;fc3pad::Publish(controls);++g_frame;TickHandlebars355(camera);
  v.actualSeat=2;fc3aim::SetVehicleSampleForTest187(&v);++g_frame;TickHandlebars355(camera);Check(g_barModel355<0&&fc3pad::SteeringOutput()==0,"355 passenger transition removes bar identity and steering");
  context->IASetIndexBuffer(nullptr,DXGI_FORMAT_UNKNOWN,0);vb->Release();ib->Release();
 }
 // Existing actual world-arm dispatch test, now in a confirmed type1 jetski.
 g_barOwner355=123;g_barModel355=1;g_barTick355=-100;g_bars355={};TestCarHandsDraw353(device,context,cb,1);
 g_barModel355=-1;g_barOwner355=0;fc3pad::SetHandlebarInput355(0,-1,0);
 for(auto*& b:g_barCb355)SafeRelease(b);memset(g_barCbSize355,0,sizeof(g_barCbSize355));
 puts("PASS:355 both real bar model GPU transforms/input, jetski world-camera arm dispatch, no-grip free hands, native-animation isolation, head-turn/visibility independence and analog/passenger guards");
}
