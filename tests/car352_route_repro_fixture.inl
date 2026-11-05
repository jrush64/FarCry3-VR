int g_carDraws353=0,g_carExpectedSide353=0;
ID3D11Buffer* g_carNativeBones353=nullptr;
ID3D11Buffer* g_carEye353[2]={};
void STDMETHODCALLTYPE CarIssue353(ID3D11DeviceContext* c,UINT,UINT,INT){
 ID3D11Buffer* bones=nullptr,*eye=nullptr;c->VSGetConstantBuffers(3,1,&bones);c->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&eye);
 Check(bones==g_armPaletteCb[g_carExpectedSide353]&&bones!=g_carNativeBones353,"353 actual HandleDraw installs tracked anatomical car arm palette");
 Check(eye==g_carEye353[g_carDraws353%2],"353 car arms retain world-eye camera, not weapon camera");
 ++g_carDraws353;SafeRelease(bones);SafeRelease(eye);
}
void TestCarHandsDraw353(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Buffer** cb){
 g_enabled=g_headTracking=g_inGameplay=g_eyeViewsSet=true;g_flatViewActive=false;g_pistolMenu.open=false;
 g_oCreateBuffer=MirrorCreate337;g_car352={};g_carSeen352=0;
 fc3aim::SetVehicleStateForTest(1);fc3aim::VehicleSample170 vehicle={};vehicle.vehicleId=123;vehicle.pawnId=12;
 vehicle.identityValid=vehicle.seatValid=vehicle.basisValid=true;vehicle.type=0;vehicle.actualSeat=1;
 vehicle.basis[0][0]=vehicle.basis[1][1]=vehicle.basis[2][2]=1;fc3aim::SetVehicleSampleForTest187(&vehicle);
 fc3pad::SetCarWheelSteeringEnabled(false);fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
 float physical[2][16]={};for(int side=0;side<2;++side){physical[side][0]=physical[side][5]=physical[side][10]=physical[side][15]=1;
  physical[side][3]=side?-.3f:.3f;physical[side][7]=-.2f;physical[side][11]=.4f;}
 fc3stereo::SetHandPoses(physical[0],physical[1]);
 for(auto& eye:g_eyeViewsBody){memset(eye.view,0,sizeof(eye.view));eye.view[0]=eye.view[5]=eye.view[10]=eye.view[15]=1;}
 g_armJoints[0].valid=g_armJoints[1].valid=false; // seated animation must not be measured
 auto* vb=Buffer(device,316240,D3D11_BIND_VERTEX_BUFFER);auto* ib=Buffer(device,78330,D3D11_BIND_INDEX_BUFFER);
 UINT stride=40,offset=0;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 float rows[88]={};rows[0]=rows[5]=rows[10]=1;cb[0]->SetPrivateData(kCbCopyGuid,sizeof(rows),rows);
 float bones[576]={};for(int b=0;b<48;++b)StoreBone(bones,b,HandIdentity());cb[3]->SetPrivateData(kCbCopyGuid,sizeof(bones),bones);
 CameraSummary cam={};cam.mainView=1;cam.xScale=cam.yScale=1;cam.right[0]=cam.up[1]=cam.forward[2]=1;
 cb[1]->SetPrivateData(kCameraSummaryGuid,sizeof(cam),&cam);context->VSSetConstantBuffers(0,4,cb);
 g_frameCamera=cam;g_frameCameraValid=true;g_mainXScale=1;
 const char* source="float4 main(uint id:SV_VertexID):SV_Position{return float4(0,0,0,1);}";
 ID3DBlob* bytecode=nullptr;Check(SUCCEEDED(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&bytecode,nullptr)),"353 draw-route shader");
 ID3D11VertexShader* shader=nullptr;Check(SUCCEEDED(device->CreateVertexShader(bytecode->GetBufferPointer(),bytecode->GetBufferSize(),nullptr,&shader)),"353 vertex shader");bytecode->Release();SetFlag(shader,kVsPatchedGuid,1);context->VSSetShader(shader,nullptr,0);
 ID3D11Texture2D* color[2]={};ID3D11RenderTargetView* target[2]={};D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=16;d.MipLevels=d.ArraySize=1;d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET;
 for(int eye=0;eye<2;++eye){Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&color[eye])),"353 render texture");Check(SUCCEEDED(device->CreateRenderTargetView(color[eye],nullptr,&target[eye])),"353 render view");
  g_carEye353[eye]=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);g_eyeCb[0][eye]=g_carEye353[eye];}
 target[0]->SetPrivateDataInterface(kTwinGuid,target[1]);context->OMSetRenderTargets(1,&target[0],nullptr);
 const auto oldDraw=g_oDrawIndexed;g_oDrawIndexed=CarIssue353;g_carNativeBones353=cb[3];g_carDraws353=0;
 float first[2][576]={},second[2][576]={};
 for(int side=0;side<2;++side){g_carExpectedSide353=side;const DrawCall draw={0,10443,side?6861u:17304u,0,0,0};
  HookDrawIndexed(context,draw.a,draw.b,0);Read(device,context,g_armPaletteCb[side],first[side]);}
 Check(g_carDraws353==4,"353 entry produces two tracked hands in both eyes without wheel registration or grip");
 ID3D11Buffer* restored=nullptr;context->VSGetConstantBuffers(3,1,&restored);Check(restored==cb[3],"353 native arm binding restored after car draws");SafeRelease(restored);
 physical[0][3]+=.12f;physical[1][7]+=.15f;fc3stereo::SetHandPoses(physical[0],physical[1]);
 for(int side=0;side<2;++side){g_carExpectedSide353=side;HookDrawIndexed(context,10443,side?6861:17304,0);Read(device,context,g_armPaletteCb[side],second[side]);
  double delta=0;for(int i=0;i<25*12;++i)delta+=std::fabs(first[side][i]-second[side][i]);Check(delta>.1,"353 each real arm draw follows its controller before a wheel grip");}
 // Losing one physical controller hides that arm instead of flashing the
 // native steering/thigh animation. The other arm continues to be controlled.
 fc3stereo::SetHandPoses(nullptr,physical[1]);const int before=g_carDraws353;HookDrawIndexed(context,10443,17304,0);
 Check(g_carDraws353==before,"353 lost physical hand cannot fall back to native seated animation");
 // The same geometry on foot returns to the original weapon/reload route.
 vehicle.actualSeat=2;fc3aim::SetVehicleSampleForTest187(&vehicle);bool handled=false;
 fc3aim::SetVehicleStateForTest(0);fc3aim::SetVehicleSampleForTest187(nullptr);
 g_oDrawIndexed=oldDraw;context->OMSetRenderTargets(0,nullptr,nullptr);context->VSSetShader(nullptr,nullptr,0);context->IASetIndexBuffer(nullptr,DXGI_FORMAT_UNKNOWN,0);
 for(int eye=0;eye<2;++eye){g_eyeCb[0][eye]=nullptr;SafeRelease(g_carEye353[eye]);target[eye]->Release();color[eye]->Release();}shader->Release();vb->Release();ib->Release();
 puts("PASS:353 actual HookDrawIndexed/HandleDraw world-camera car arms, both eyes, free motion on entry, no wheel dependency, no seated-animation fallback, binding restoration and foot/passenger isolation");
}
void TestDualWheel353(){}
