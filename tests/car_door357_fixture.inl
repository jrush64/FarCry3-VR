extern void SeedFoot357();
void TestDoor357(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 using namespace fc3door357;
 Input i;i.allowed=i.tracked=true;i.epoch=1;i.now=100;i.handle={0,0,0};i.hand={0,0,0};i.anchor={10,0,0};State s;
 s.Step(i);i.now+=10;s.Step(i);i.grip=1;i.now+=10;Check(s.Step(i).grabbed,"357 fresh squeeze acquires exterior handle");
 i.hand.y=.1;i.now+=10;Check(!s.Step(i).enter,"357 sideways movement cannot open door");i.hand.y=0;i.hand.x=-.08;i.now+=10;Check(s.Step(i).enter,"357 outward pull requests one native entry");i.now+=10;Check(!s.Step(i).enter,"357 held handle cannot repeat entry");
 i.now+=10;i.tracked=false;Check(!s.Step(i).held,"357 tracking loss releases door");i.now+=10;i.tracked=true;Check(!s.Step(i).held,"357 held grip cannot reacquire after tracking loss");
 i.grip=0;i.hand={};i.now+=10;s.Step(i);i.grip=1;i.now+=10;s.Step(i);i.anchor.x+=1;i.now+=10;Check(!s.Step(i).held,"357 cannot transfer a held handle to another car instance");
 s={};i={};i.allowed=i.tracked=true;i.now=100;i.epoch=1;s.Step(i);i.now+=10;s.Step(i);i.grip=1;i.now+=10;s.Step(i);i.now+=121;i.hand.x=-.08;Check(!s.Step(i).enter,"357 stale draw cannot open car");
 g_enabled=g_headTracking=g_inGameplay=g_eyeViewsSet=g_doors357Enabled=true;g_flatViewActive=false;g_pistolMenu.open=false;g_reload={};g_manualReload={};g_reloadAlignment=false;
 fc3pad::SetGameplayControls(true);fc3pad::SetSelector290(false);fc3aim::SetVehicleStateForTest(0);SetRenderHands303(0,true,g_armPoseEpoch);
 auto* vb=Buffer(device,716640,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,126360,D3D11_BIND_INDEX_BUFFER);UINT stride=32,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);context->VSSetConstantBuffers(0,4,cb);context->VSSetShader(nullptr,nullptr,0);
 float globals[88]={};globals[0]=globals[5]=globals[10]=1;globals[3]=-.9157632f;globals[7]=.7792645f;globals[11]=.8648809f;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);
 CameraSummary cam={};cam.mainView=true;cam.right[0]=cam.up[1]=cam.forward[2]=1;cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
 const DrawCall draw={0,651,33714,0,0,0};g_door357[0]=g_door357[1]={};g_doorFrame357[0]=g_doorFrame357[1]=-100;g_physicalValid303[0]=true;g_physicalValid303[1]=false;
 fc3pad::Controllers c={};c.active=true;
 auto poll=[&](float grip,double pull,int variant=0){SeedFoot357();c.rightGrip=grip;fc3pad::Publish(c);Mat4 hand=HandIdentity();hand.m[3]=globals[3]+kDoorHandle357.x-pull;hand.m[7]=globals[7]+kDoorHandle357.y+.015;hand.m[11]=globals[11]+kDoorHandle357.z-.015;for(int n=0;n<16;++n)g_physicalBody303[0][n]=float(hand.m[n]);g_physicalAt352=GetTickCount64();++g_frame;ObserveCarDoor357(context,draw,variant);};
 poll(0,0);poll(0,0);poll(1,0,1);Check(!g_door357[0].held,"357 non-main draw cannot grab door");poll(1,0);Check(g_door357[0].held,"357 actual sedan draw, rigid-node transform and tracked palm acquire handle");poll(1,.085);Check(g_door357[0].spent,"357 actual scene contact/pull reaches entry intent");
 Check((fc3pad::ButtonsOutput()&0x4000)!=0,"357 physical door pull emits native X interaction");Check(fc3pad::InteractionOnly(),"357 door-generated X is interaction-only and cannot authorize native reload");Check(fc3pad::FireOutput()==0,"357 door entry suppresses accidental firing");
 fc3aim::SetVehicleStateForTest(1);Check((fc3pad::ButtonsOutput()&0x4000)==0,"357 exterior request cannot become an in-vehicle exit");fc3aim::SetVehicleStateForTest(0);fc3pad::SetDoorInput357(false,false);
 vb->Release();ib->Release();puts("PASS:357 sedan native rigid-node contact, free-hand grip/outward pull, one-shot native X entry, current-instance/tracking/stale/context isolation; native entry outcome pending headset");
}
