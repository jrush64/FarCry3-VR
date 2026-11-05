
static int g_windowFixtureCalls=0;
static LRESULT CALLBACK WindowFixture(HWND,UINT,WPARAM,LPARAM){++g_windowFixtureCalls;return 0x29;}

void TestGameplayAssist(ID3D11Device* device,ID3D11DeviceContext* context)
{
 const bool oldGodMode123=g_godMode123;g_godMode123=false;
 const auto previousWindowProc=g_menuWndProc;g_menuWndProc=WindowFixture;
 fc3pad::Controllers neutral={};fc3pad::Publish(neutral);fc3pad::GuardMenuExit();
 Check(fc3pad::NativeInputBlocked(),"neutral exit keeps keyboard/mouse blocked");
 g_windowFixtureCalls=0;Check(InsertWndProc(nullptr,WM_LBUTTONDOWN,0,0)==0&&!g_windowFixtureCalls,"held exit click cannot leak into gameplay");
 fc3pad::SetPistolCalibration(true);g_windowFixtureCalls=0;Check(InsertWndProc(nullptr,WM_LBUTTONDOWN,0,0)==0&&!g_windowFixtureCalls,"Insert consumes native mouse click");
 fc3pad::SetPistolCalibration(false);fc3pad::Publish(neutral);
 fc3pad::Controllers menu={};menu.active=true;menu.menu=true;fc3pad::Publish(menu);
 Check(fc3pad::NativeMenuOpen()&&fc3pad::GameplayInputBlocked()&&!fc3pad::MenuOpen()&&!fc3pad::NativeInputBlocked(),"native Start opening blocks weapons but permits keyboard/mouse UI");
 for(UINT message:{UINT(WM_MOUSEMOVE),UINT(WM_LBUTTONDOWN),UINT(WM_LBUTTONUP),UINT(WM_MOUSEWHEEL),UINT(WM_KEYDOWN),UINT(WM_CHAR)}){
  g_windowFixtureCalls=0;Check(InsertWndProc(nullptr,message,0,0)==0x29&&g_windowFixtureCalls==1,"native menu receives original mouse/keyboard window message exactly once");
 }
 g_menuWndProc=previousWindowProc;
 std::puts("PASS: offline29 actual window hook forwards native menu mouse/keyboard, preserves result, keeps Insert exclusive and retains neutral exit protection");
 Sleep(760);fc3pad::Publish(neutral); // expire this fixture's Start-opening guard before gameplay pixel checks

 for(float width:{1024.f,2048.f})for(float height:{768.f,1024.f})for(float sx:{1.5f,2.4f})for(float sy:{1.2f,2.1f}){
  fc3stereo::EyeView eye={};eye.tanLeft=-sx*.4f;eye.tanRight=sx*.6f;eye.tanDown=-sy*.55f;eye.tanUp=sy*.45f;
  const auto r=ReticleRadii(eye,width,height,4.f);
  Check(std::fabs(r.x*sx/width-r.y*sy/height)<1e-6f,"reticle tangent radius equal on non-square asymmetric eye frusta");
 }
 fc3stereo::EyeView invalid={};Check(ReticleRadii(invalid,1024,768,4).x==4,"unknown projection keeps finite pixel fallback");
 std::puts("PASS: offline28 reticle uses per-eye tangent span and texture dimensions; asymmetric frusta and finite fallback");
 auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x2830000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 Check(image!=nullptr,"fake engine image");UINT base=reinterpret_cast<UINT>(image);
 image[0x132c342]=0x6a;image[0x132c343]=0x48;image[0x132c344]=0x68;
 *reinterpret_cast<UINT*>(image+0x132c345)=base+0x1fb8418;memcpy(image+0x1fb8418,"GodMode",8);
 image[0x1329f6a]=0xc7;image[0x1329f6b]=0;
 *reinterpret_cast<UINT*>(image+0x1329f6c)=base+0x1fb8b1c;
 image[0x26577a]=0xa3;*reinterpret_cast<UINT*>(image+0x26577b)=base+0x282a648;
 UINT object[36]={};object[0]=base+0x1fb8b1c;object[0x48/4]=2;object[0x44/4]=0x12345678;object[0x4c/4]=0x87654321;
 *reinterpret_cast<UINT*>(image+0x282a648)=reinterpret_cast<UINT>(object);
 g_assistTestBase=base;g_infiniteHealth=true;TickGameplayAssist();
 Check(object[18]==1&&g_healthStatus==1,"native god mode enabled");
 Check(object[17]==0x12345678&&object[19]==0x87654321,"only god-mode field changes");
 g_godMode123=true;TickGameplayAssist();Check(object[18]==1&&g_healthStatus==1,"210 God mode requests invulnerable native mode1 (mode2 lets hits land)");
 g_infiniteHealth=false;TickGameplayAssist();Check(object[18]==1,"210 God mode alone keeps native mode1 with infinite health disabled");
 g_godMode123=false;TickGameplayAssist();Check(object[18]==2,"disable restores original native mode");
 object[18]=0;g_godMode123=true;TickGameplayAssist();object[18]=2;g_godMode123=false;TickGameplayAssist();Check(object[18]==2,"turning protection off preserves another owner's intervening setting");
 object[18]=2;
 image[0x132c343]=0x4c;g_infiniteHealth=true;TickGameplayAssist();Check(object[18]==2&&g_healthStatus!=1,"reject mismatched field registration");image[0x132c343]=0x48;
 object[0]=0;TickGameplayAssist();Check(object[18]==2,"reject wrong settings object");object[0]=base+0x1fb8b1c;
 TickGameplayAssist();Check(object[18]==1,"enable after settings become available");
 *reinterpret_cast<UINT*>(image+0x282a648)=0;TickGameplayAssist();Check(g_healthOwned==nullptr,"release ownership on object loss");
 g_infiniteHealth=false;TickGameplayAssist();Check(object[18]==1,"no stale-pointer restore after object loss");
 g_godMode123=true;TickGameplayAssist();Check(object[18]==1,"lost settings object is never restored through a stale pointer");g_godMode123=false;
 wchar_t godFolder123[MAX_PATH],godIni123[MAX_PATH];GetTempPathW(MAX_PATH,godFolder123);swprintf_s(godIni123,L"%sfc3_god123_%lu.ini",godFolder123,GetCurrentProcessId());DeleteFileW(godIni123);
 LoadGodMode123(godIni123);Check(g_godMode123,"test god mode defaults on when absent");g_godMode123=false;Check(SaveGodMode123(godIni123),"God mode off saves");g_godMode123=true;LoadGodMode123(godIni123);Check(!g_godMode123,"saved God mode off survives restart");DeleteFileW(godIni123);
 g_godMode123=oldGodMode123;g_assistTestBase=0;VirtualFree(image,0,MEM_RELEASE);
 std::puts("PASS: native health setting guards, exact field, reversible toggle, settings lifecycle");
 g_dev=device;g_ctx=context;g_enabled=true;g_inGameplay=true;g_flatViewActive=false;g_pistolMenu.open=false;g_eyeViewsSet=true;g_visibleWeaponDrawsLastFrame=10;g_redDot=true;
 D3D11_TEXTURE2D_DESC td={};td.Width=400;td.Height=200;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;
 ID3D11Texture2D* eyes[2]={};ID3D11RenderTargetView* rt[2]={};
 const float black[4]={0,0,0,1};
 for(int i=0;i<2;++i){Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&eyes[i])),"eye texture");Check(SUCCEEDED(device->CreateRenderTargetView(eyes[i],nullptr,&rt[i])),"eye RTV");context->ClearRenderTargetView(rt[i],black);g_redDotValid[i]=true;g_redDotNdc[i][0]=i==0?.2f:-.2f;g_redDotNdc[i][1]=.3f;}
 context->OMSetRenderTargets(1,&rt[0],nullptr);D3D11_VIEWPORT vp={3,4,100,80,0,1};context->RSSetViewports(1,&vp);
 ImGuiContext* oldContext=ImGui::CreateContext();g_own=false;
 for(int i=0;i<2;++i){g_eyeViews[i].tanLeft=-1;g_eyeViews[i].tanRight=1;g_eyeViews[i].tanDown=-1;g_eyeViews[i].tanUp=1;}
 RenderRedDot(eyes[0],eyes[1]);
 Check(ImGui::GetCurrentContext()==oldContext&&!g_own,"reticle restores ImGui and hook ownership");
 ID3D11RenderTargetView* restored=nullptr;context->OMGetRenderTargets(1,&restored,nullptr);Check(restored==rt[0],"reticle restores render target");restored->Release();
 D3D11_VIEWPORT actual={};UINT count=1;context->RSGetViewports(&count,&actual);Check(!memcmp(&vp,&actual,sizeof(vp)),"reticle restores viewport");
 auto redPixels=[&](int eye,bool checkPosition){
  D3D11_TEXTURE2D_DESC sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;
  Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"readback texture");context->CopyResource(stage,eyes[eye]);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&m)),"reticle readback");
  int n=0,minX=100000,minY=100000,maxX=-1,maxY=-1;double sx=0,sy=0;for(UINT y=0;y<td.Height;++y)for(UINT x=0;x<td.Width;++x){const BYTE* p=static_cast<const BYTE*>(m.pData)+y*m.RowPitch+x*4;if(p[0]>180&&p[1]<70&&p[2]<70){++n;sx+=x+.5;sy+=y+.5;minX=(std::min)(minX,int(x));maxX=(std::max)(maxX,int(x));minY=(std::min)(minY,int(y));maxY=(std::max)(maxY,int(y));}}
  context->Unmap(stage,0);stage->Release();
  if(checkPosition)Check(maxX-minX>1.4f*(maxY-minY),"actual GPU reticle corrects non-square eye image instead of a pixel circle");
  if(checkPosition)Check(n>8&&fabs(sx/n-(eye==0?240:160))<1&&fabs(sy/n-70)<1,"actual red pixels use per-eye aim position");return n;
 };
 redPixels(0,true);redPixels(1,true);
 const int colour52=g_redDotColour52;
 for(int choice=0;choice<8;++choice){g_redDotColour52=choice;for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderRedDot(eyes[0],eyes[1]);for(int side=0;side<2;++side){D3D11_TEXTURE2D_DESC read=td;read.BindFlags=0;read.Usage=D3D11_USAGE_STAGING;read.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;device->CreateTexture2D(&read,nullptr,&staging);context->CopyResource(staging,eyes[side]);D3D11_MAPPED_SUBRESOURCE mapped={};context->Map(staging,0,D3D11_MAP_READ,0,&mapped);const BYTE* pixel=(BYTE*)mapped.pData+70*mapped.RowPitch+(side?160:240)*4;const ImU32 wanted=kReticleColours52[choice].colour;for(int channel=0;channel<4;++channel)Check(abs(int(pixel[channel])-int((wanted>>(8*channel))&255))<3,"all8 reticle colours reach correct stereo GPU aim pixel");context->Unmap(staging,0);staging->Release();}}
 wchar_t colourPath52[MAX_PATH];GetTempPathW(MAX_PATH,colourPath52);wcscat_s(colourPath52,L"fc3-reticle52-fixture.ini");DeleteFileW(colourPath52);WritePrivateProfileStringW(L"Pistol",L"PortX",L"0.123456",colourPath52);g_redDotColour52=6;Check(SaveReticleColour52(colourPath52),"save chosen crosshair colour");g_redDotColour52=0;LoadReticleColour52(colourPath52);Check(g_redDotColour52==6,"crosshair colour persists across load");wchar_t saved52[32];GetPrivateProfileStringW(L"Pistol",L"PortX",L"",saved52,32,colourPath52);Check(!wcscmp(saved52,L"0.123456"),"colour save preserves calibrated weapon keys");WritePrivateProfileStringW(L"Reticle",L"Colour",L"999",colourPath52);LoadReticleColour52(colourPath52);Check(g_redDotColour52==0,"invalid crosshair setting falls back to red");DeleteFileW(colourPath52);g_redDotColour52=colour52;
 puts("PASS: colour52 all8 stereo GPU reticle colours, persisted selection, invalid-value fallback and calibration isolation");
 { // 209: the barrel stays rigid with the drawn gun while the raw aim ray lags a turning gun by one frame
  BarrelLocal206 b={};const double step=2*3.14159265358979/180;V3 prevY={0,1,0};double worst=0;
  for(int f=0;f<90;++f){const double a=f*step;const V3 x={std::cos(a),-std::sin(a),0},y={std::sin(a),std::cos(a),0},z={0,0,1};
   const V3 lagged=prevY; /* aim from the previous frame */ const V3 w=BarrelUpdate206(b,lagged,x,y,z,7,false,1000+f,1);worst=(std::max)(worst,AngleDeg206(w,y));prevY=y;}
  Check(worst<.01,"209 turning gun: laser/shot barrel stays on the drawn barrel, never on the one-frame-late aim");
  const double off=.5*3.14159265358979/180;const V3 x={1,0,0},y={0,1,0},z={0,0,1},aim={std::sin(off),std::cos(off),0};V3 w={};
  for(int f=0;f<200;++f)w=BarrelUpdate206(b,aim,x,y,z,7,false,2000+f,1);
  Check(AngleDeg206(w,aim)<.05,"209 still gun: small rest offset between aim and drawn barrel is learned");
  const V3 farAim={std::sin(.2),std::cos(.2),0};BarrelLocal206 c={};for(int f=0;f<50;++f)w=BarrelUpdate206(c,farAim,x,y,z,9,false,3000+f,1);
  Check(AngleDeg206(w,y)<.01,"209 an aim more than 3 deg off the drawn barrel is never learned as the barrel");
  puts("PASS: 209 rigid drawn-barrel ray under stick turns, rest-only offset learning, large-offset refusal");
 }
 {
  wchar_t laserIni[MAX_PATH];GetTempPathW(MAX_PATH,laserIni);wcscat_s(laserIni,L"fc3-laser194-fixture.ini");DeleteFileW(laserIni);
  const int oldMode=g_laserMode194;g_laserMode194=3;Check(SaveLaserMode194(laserIni),"194 laser line+dot mode saves");
  g_laserMode194=0;LoadReticleColour52(laserIni);Check(g_laserMode194==3,"194 shared-colour laser mode reloads");
  const auto identity=HandIdentity();for(int eye=0;eye<2;++eye)for(int i=0;i<16;++i)g_eyeViews[eye].view[i]=float(identity.m[i]);
  const float worldPoint[3]={1,0,5};ImVec2 projected={};
  Check(LaserProjection194(0,identity,worldPoint,projected)&&fabs(projected.x-.2f)<1e-5&&fabs(projected.y)<1e-5,
   "194 surface point projects to its actual stereo angular position");
  for(int i=0;i<16;++i)g_eyeViewsBody[0].view[i]=float(identity.m[i]);
  g_eyeViewsBody[0].view[3]=3.f;g_eyeViewsBody[0].view[7]=-2.f;
  Check(LaserProjection194(0,identity,worldPoint,projected)&&fabs(projected.x-.2f)<1e-5&&fabs(projected.y)<1e-5,
   "204 laser stays on the weapon eye when the independent arm/body eye moves");
  Mat4 gameView=identity;gameView.m[3]=1;g_eyeViews[0].view[3]=-.5f;
  Check(LaserProjection194(0,gameView,worldPoint,projected)&&fabs(projected.x-.3f)<1e-5,
   "204 laser projection uses the weapon eye and native game view under nonidentity transforms");
  g_eyeViews[0].view[3]=0;
  DeleteFileW(laserIni);g_laserMode194=oldMode;fc3aim::SetLaserEnabled194(oldMode!=0);
 }
 {
  const int oldMode=g_laserMode194,oldColour=g_redDotColour52;const bool oldDot=g_redDot,oldAim=g_aimActive;
  const auto oldStage=g_muzzleRender58;const auto oldLaserFrame=g_muzzleLaserFrame203;double oldQ[3][3];memcpy(oldQ,g_aimQ,sizeof(oldQ));for(int i=0;i<3;++i)for(int j=0;j<3;++j)g_aimQ[i][j]=i==j?1.:0.;const auto identity=HandIdentity();
  g_laserMode194=3;g_redDotColour52=1;g_redDot=false;g_aimActive=true;
  g_muzzleLaserFrame203={identity,identity,g_frame,g_armPoseEpoch,12,true};g_muzzleLaserFrame203.native=identity;g_muzzleLaserFrame203.barrelWorld={1,0,0};g_muzzleLaserFrame203.barrelValid=true; // 206: barrel +X in the drawn frame
  g_muzzleRender58={};MuzzleDiscardStage58(); // a later attachment draw clears the transient stage before Present
  fc3aim::LaserSample194 laser={};laser.nativeMuzzle[0]=-.3f;laser.nativeMuzzle[2]=.5f;laser.point[0]=.3f;laser.point[2]=.5f;laser.profile=12;laser.epoch=g_armPoseEpoch;laser.ms=GetTickCount64();laser.hit=true;laser.distance=.6f;laser.localValid=true;laser.muzzleLocal[0]=-.3f;laser.muzzleLocal[2]=.5f;g_laserFixture194=&laser;
  D3D11_TEXTURE2D_DESC depthDesc={};depthDesc.Width=td.Width;depthDesc.Height=td.Height;depthDesc.MipLevels=depthDesc.ArraySize=1;depthDesc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;depthDesc.SampleDesc.Count=1;depthDesc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
  ID3D11Texture2D* depthTex=nullptr;ID3D11DepthStencilView* depthView=nullptr;ID3D11DepthStencilState* depthState=nullptr;
  Check(SUCCEEDED(device->CreateTexture2D(&depthDesc,nullptr,&depthTex))&&SUCCEEDED(device->CreateDepthStencilView(depthTex,nullptr,&depthView)),"205 depth fixture texture and view");
  D3D11_DEPTH_STENCIL_DESC depthStateDesc={};depthStateDesc.DepthEnable=true;depthStateDesc.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depthStateDesc.DepthFunc=D3D11_COMPARISON_LESS_EQUAL; // 206: a weapon pass that writes depth
  Check(SUCCEEDED(device->CreateDepthStencilState(&depthStateDesc,&depthState)),"205 depth fixture state");
  const auto oldWidth=g_bbW,oldHeight=g_bbH;const auto oldPlacement=g_guideShaderPlacement[1];const auto oldPlacementFrame=g_guideShaderPlacementFrame[1];g_bbW=td.Width;g_bbH=td.Height;g_guideShaderPlacement[1]=identity;g_guideShaderPlacementFrame[1]=g_frame;
  ID3D11RenderTargetView* oldTarget=nullptr;ID3D11DepthStencilView* oldDepth=nullptr;context->OMGetRenderTargets(1,&oldTarget,&oldDepth);ID3D11DepthStencilState* oldDepthState=nullptr;UINT oldStencilRef=0;context->OMGetDepthStencilState(&oldDepthState,&oldStencilRef);
  ID3D11Buffer* oldCameraCb=nullptr,*oldEyeCb=nullptr;context->VSGetConstantBuffers(1,1,&oldCameraCb);context->VSGetConstantBuffers(dxbc::kEyeCbSlot,1,&oldEyeCb);
  ID3D11Buffer* cameraCb=Buffer(device,sizeof(float)*224,D3D11_BIND_CONSTANT_BUFFER),*eyeCb=Buffer(device,sizeof(float)*dxbc::kEyeRows*4,D3D11_BIND_CONSTANT_BUFFER),*worldCb=Buffer(device,64,D3D11_BIND_CONSTANT_BUFFER);
  float cameraRows[224]={},eyeRows[dxbc::kEyeRows*4]={};for(int i:{0,5,10,15})cameraRows[i]=1;cameraCb->SetPrivateData(kCbCopyGuid,sizeof(cameraRows),cameraRows);eyeCb->SetPrivateData(kCbCopyGuid,sizeof(eyeRows),eyeRows);
  context->VSSetConstantBuffers(1,1,&cameraCb);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeCb);context->OMSetRenderTargets(1,&rt[0],depthView);context->OMSetDepthStencilState(depthState,0);context->ClearDepthStencilView(depthView,D3D11_CLEAR_DEPTH,1,0);
  g_muzzleRender58.native=identity;g_muzzleRender58.finalWorld=identity;g_muzzleRender58.view=identity;g_muzzleRender58.context=context;g_muzzleRender58.buffer=worldCb;g_muzzleRender58.frame=g_frame;g_muzzleRender58.epoch=g_armPoseEpoch;g_muzzleRender58.start=0;g_muzzleRender58.count=6;g_muzzleRender58.profile=12;g_muzzleRender58.valid=true;
  DrawCall laserDraw={};laserDraw.a=6;for(int eye=0;eye<2;++eye)CaptureLaserEye205(context,laserDraw,worldCb,1,eye);
  Check(g_laserPending206[0].armed&&g_laserPending206[1].armed&&!g_laserEye205[0].valid,"207 core draw only arms the depth capture (attachments draw after it)");
  { // 207: a draw on the same depth target keeps it armed; a draw with another depth target flushes it
   LaserDrawObserve206(context);Check(g_laserPending206[0].armed&&!g_laserEye205[0].valid,"207 same depth target keeps the capture pending");
   ID3D11DepthStencilView* keep=nullptr;ID3D11RenderTargetView* keepRt=nullptr;context->OMGetRenderTargets(1,&keepRt,&keep);context->OMSetRenderTargets(1,&keepRt,nullptr);
   LaserDrawObserve206(context);Check(!g_laserPending206[0].armed&&!g_laserPending206[1].armed&&g_laserEye205[0].valid&&g_laserEye205[1].valid,"207 switching away from the weapon depth target flushes both eyes");
   context->OMSetRenderTargets(1,&keepRt,keep);SafeRelease(keep);SafeRelease(keepRt);
  }
  Check(g_laserEye205[0].valid&&g_laserEye205[1].valid&&g_laserEye205[0].depthView!=depthView,"205 confirmed core draw captures independent eye depth snapshots");
  context->VSSetConstantBuffers(1,1,&oldCameraCb);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&oldEyeCb);context->OMSetRenderTargets(1,&oldTarget,oldDepth);context->OMSetDepthStencilState(oldDepthState,oldStencilRef);
  context->ClearDepthStencilView(depthView,D3D11_CLEAR_DEPTH,1,0);
  for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
  D3D11_TEXTURE2D_DESC read=td;read.BindFlags=0;read.Usage=D3D11_USAGE_STAGING;read.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* stage=nullptr;device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);D3D11_MAPPED_SUBRESOURCE pixels={};context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
  const auto greenAt=[&](int x,int y){const BYTE* p=static_cast<const BYTE*>(pixels.pData)+y*pixels.RowPitch+x*4;return p[1]>180&&p[0]<100;};
  Check(greenAt(200,100),"205 laser draws through the weapon draw's eye and visible depth");
  context->Unmap(stage,0);stage->Release();
  for(auto& e:g_laserEye205)context->ClearDepthStencilView(e.depthView,D3D11_CLEAR_DEPTH,.25f,0);
  for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
  device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
  Check(!greenAt(200,100),"205 nearer weapon depth hides the laser beam");
  context->Unmap(stage,0);stage->Release();
  for(auto& e:g_laserEye205)context->ClearDepthStencilView(e.depthView,D3D11_CLEAR_DEPTH,1,0);g_muzzleLaserFrame203.move.m[3]=.5;
  for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
  device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
  Check(greenAt(250,100)&&!greenAt(200,100),"205 beam begins at the latest confirmed gun pose");
  Check(greenAt(340,100),"206 beam follows the current drawn barrel ray for the sampled distance, not the stale endpoint");
  context->Unmap(stage,0);stage->Release();
  g_laserEye205[0].valid=false;for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
  device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
  Check(!greenAt(250,100),"205 missing weapon depth cannot fall back to the floating overlay");
  context->Unmap(stage,0);stage->Release();
  { // 206: 4x MSAA weapon depth is resolved for the test; an ALWAYS pass can never be the occluder
   UINT levels=0;const bool msaa=SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R24G8_TYPELESS,4,&levels))&&levels>0;
   if(!msaa)puts("SKIP: 206 this adapter has no 4x MSAA for R24G8 depth");
   else{
    D3D11_TEXTURE2D_DESC msDesc=depthDesc;msDesc.Format=DXGI_FORMAT_R24G8_TYPELESS;msDesc.SampleDesc.Count=4;msDesc.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* msTex=nullptr;ID3D11DepthStencilView* msView=nullptr;D3D11_DEPTH_STENCIL_VIEW_DESC msViewDesc={};msViewDesc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;msViewDesc.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DMS;
    Check(SUCCEEDED(device->CreateTexture2D(&msDesc,nullptr,&msTex))&&SUCCEEDED(device->CreateDepthStencilView(msTex,&msViewDesc,&msView)),"206 MSAA depth fixture");
    D3D11_DEPTH_STENCIL_DESC alwaysDesc=depthStateDesc;alwaysDesc.DepthFunc=D3D11_COMPARISON_ALWAYS;ID3D11DepthStencilState* alwaysState=nullptr;Check(SUCCEEDED(device->CreateDepthStencilState(&alwaysDesc,&alwaysState)),"206 ALWAYS state");
    auto capture=[&](ID3D11DepthStencilState* state){context->VSSetConstantBuffers(1,1,&cameraCb);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&eyeCb);context->OMSetRenderTargets(0,nullptr,msView);/* a 4x depth view cannot share the OM with the 1x colour target */context->OMSetDepthStencilState(state,0);for(auto& e:g_laserEye205)e.valid=false;for(int eye=0;eye<2;++eye)CaptureLaserEye205(context,laserDraw,worldCb,1,eye);LaserClearObserve206(context,msView);/* 207: a clear of the armed target flushes before the content goes */context->VSSetConstantBuffers(1,1,&oldCameraCb);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&oldEyeCb);context->OMSetRenderTargets(1,&oldTarget,oldDepth);context->OMSetDepthStencilState(oldDepthState,oldStencilRef);};
    context->ClearDepthStencilView(msView,D3D11_CLEAR_DEPTH,1,0);capture(alwaysState);
    Check(!g_laserEye205[0].valid&&!g_laserEye205[1].valid,"206 an ALWAYS depth pass is refused as the laser's occluder");
    capture(depthState);
    Check(g_laserEye205[0].valid&&g_laserEye205[1].valid&&g_laserEye205[0].textureDesc.SampleDesc.Count==4&&g_laserEye205[0].depthTexture!=msTex,"206 4x MSAA weapon depth is accepted and resolved");
    for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
    device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
    Check(greenAt(300,100),"206 laser draws against the resolved far MSAA depth");
    context->Unmap(stage,0);stage->Release();
    context->ClearDepthStencilView(msView,D3D11_CLEAR_DEPTH,.25f,0);capture(depthState);
    for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderLaserDepth205(eyes[0],eyes[1]);
    device->CreateTexture2D(&read,nullptr,&stage);context->CopyResource(stage,eyes[0]);context->Map(stage,0,D3D11_MAP_READ,0,&pixels);
    Check(!greenAt(300,100),"206 nearer resolved MSAA depth hides the laser");
    context->Unmap(stage,0);stage->Release();
    SafeRelease(alwaysState);SafeRelease(msView);SafeRelease(msTex);
   }
  }
  SafeRelease(oldCameraCb);SafeRelease(oldEyeCb);SafeRelease(oldTarget);SafeRelease(oldDepth);SafeRelease(oldDepthState);SafeRelease(cameraCb);SafeRelease(eyeCb);SafeRelease(worldCb);
  for(auto& e:g_laserEye205)ClearLaserEye205(e);for(auto& p:g_laserPending206)ClearLaserPending206(p);ClearLaserResolve206();SafeRelease(depthState);SafeRelease(depthView);SafeRelease(depthTex);g_bbW=oldWidth;g_bbH=oldHeight;g_guideShaderPlacement[1]=oldPlacement;g_guideShaderPlacementFrame[1]=oldPlacementFrame;
  g_laserFixture194=nullptr;g_muzzleRender58=oldStage;g_muzzleLaserFrame203=oldLaserFrame;memcpy(g_aimQ,oldQ,sizeof(oldQ));g_laserMode194=oldMode;g_redDotColour52=oldColour;g_redDot=oldDot;g_aimActive=oldAim;
 }
 for(auto* v:rt)context->ClearRenderTargetView(v,black);
 g_pistolMenu.open=true;RenderRedDot(eyes[0],eyes[1]);Check(redPixels(0,false)==0&&redPixels(1,false)==0,"no dot in INSERT menu");g_pistolMenu.open=false;
 g_redDotValid[0]=false;g_redDotNdc[1][0]=2;RenderRedDot(eyes[0],eyes[1]);Check(redPixels(0,false)==0&&redPixels(1,false)==0,"no invalid or offscreen dot");
 {
  g_nativeCursorVisible=true;g_nativeCursorX=g_nativeCursorY=.25f;
  const auto oldPanels0=g_nativeUiPanel[0],oldPanels1=g_nativeUiPanel[1];
  ObserveNativeUiPanel(0,.6f,.5f,.1f,-.1f);ObserveNativeUiPanel(1,.7f,.6f,-.1f,.04f);
  for(auto* v:rt)context->ClearRenderTargetView(v,black);
  RenderNativeUiCursor(eyes[0],eyes[1],true);
  Check(ImGui::GetCurrentContext()==oldContext&&!g_own,"native cursor restores ImGui context and hook ownership");
  ID3D11RenderTargetView* current=nullptr;context->OMGetRenderTargets(1,&current,nullptr);Check(current==rt[0],"native cursor restores render target");current->Release();
  D3D11_VIEWPORT after={};UINT nvp=1;context->RSGetViewports(&nvp,&after);Check(!memcmp(&vp,&after,sizeof(vp)),"native cursor restores viewport");
  auto countCursor=[&](int eye){
   D3D11_TEXTURE2D_DESC sd=td;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
   ID3D11Texture2D* stage=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&sd,nullptr,&stage)),"cursor readback texture");context->CopyResource(stage,eyes[eye]);
   D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(stage,0,D3D11_MAP_READ,0,&mapped)),"cursor GPU readback");int n=0;
   const UINT wantedX=eye==0?160:110,wantedY=eye==0?85:66;
   for(UINT y=0;y<td.Height;++y)for(UINT x=0;x<td.Width;++x){const BYTE* px=static_cast<const BYTE*>(mapped.pData)+y*mapped.RowPitch+x*4;if(px[0]>180&&px[1]>180&&px[2]>180){++n;Check(x>=wantedX-2&&x<=wantedX+15&&y>=wantedY-2&&y<=wantedY+19,"native cursor pixels follow each eye's map panel, not full-eye client coordinates");}}
   context->Unmap(stage,0);stage->Release();return n;
  };
  Check(countCursor(0)>=2&&countCursor(1)>=2,"visible native cursor is copied into BOTH completed eye images");
  for(auto* v:rt)context->ClearRenderTargetView(v,black);RenderNativeUiCursor(eyes[0],eyes[1],false);
  Check(!countCursor(0)&&!countCursor(1),"cursor cannot draw in gameplay");
  g_pistolMenu.open=true;RenderNativeUiCursor(eyes[0],eyes[1],true);Check(!countCursor(0),"Insert retains its own cursor");g_pistolMenu.open=false;
  g_nativeUiPanel[0]=oldPanels0;g_nativeUiPanel[1]=oldPanels1;
  g_nativeCursorVisible=false;ImGui::SetCurrentContext(g_nativeCursorContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_nativeCursorContext);g_nativeCursorContext=nullptr;g_nativeCursorDevice=nullptr;ImGui::SetCurrentContext(oldContext);
  puts("PASS: UI33 native menu cursor actual stereo GPU pixels, coordinate agreement, gameplay/Insert gates and state restoration");
 }
 ImGui::SetCurrentContext(g_reticleContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_reticleContext);g_reticleContext=nullptr;g_reticleDevice=nullptr;ImGui::SetCurrentContext(oldContext);ImGui::DestroyContext(oldContext);
 context->ClearState();for(auto* v:rt)v->Release();for(auto* t:eyes)t->Release();g_dev=nullptr;g_ctx=nullptr;
 std::puts("PASS: red-dot GPU pixels in both eyes, distinct stereo aim positions, menu/offscreen gates and renderer state restoration");
}
