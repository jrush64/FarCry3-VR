void TestPistolFrame45(ID3D11Device* d,ID3D11DeviceContext* c){
 const auto pending=g_1911FramePending,final=g_1911FrameFinal;const auto pistol=g_pistolReload;const auto settings=g_pistolGuides;const auto menu=g_pistolMenu;const auto preview=g_pistolGuidePreview;
 const LONG frame=g_frame,profile=g_weaponMechanicProfile,mechanic=g_weaponMechanicFrame;const float mainScale=g_mainXScale;const Mat4 placement=g_guideShaderPlacement[1];const LONG placementFrame=g_guideShaderPlacementFrame[1];
 g_weaponMechanicProfile=20;g_mainXScale=.368f;g_pistolMenu.open=false;g_pistolGuides.box=true;g_pistolGuides.preview=true;
 ID3D11Buffer* vb=Buffer(d,177904,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(d,32682,D3D11_BIND_INDEX_BUFFER),*world=Buffer(d,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(d,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=240;c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);ID3D11Buffer* cb[2]={world,camera};c->VSSetConstantBuffers(0,2,cb);
 // Replay the real weapon camera, not the world camera which hid Frame45's bug.
 CameraSummary native={};native.right[0]=native.up[1]=native.forward[2]=1;native.mainView=1;native.xScale=1.5544246f;native.yScale=2.790552f;
 int pairs=0;
 for(const auto& row:kSplitCapture43){
  ++g_frame;g_weaponMechanicFrame=g_frame;g_1911FramePending={};g_1911FrameFinal={};
  Mat4 desired={};memcpy(desired.m,row.desiredParent,sizeof(desired.m));float globals[88]={};memcpy(globals,row.parent,20*sizeof(float));WritePistolWorld(globals,desired);
  for(int i=0;i<3;++i)native.position[i]=float(desired.m[i*4+3]);camera->SetPrivateData(kCameraSummaryGuid,sizeof(native),&native);c->UpdateSubresource(world,0,nullptr,globals,0,0);
  g_guideShaderPlacement[1]=HandIdentity();g_guideShaderPlacementFrame[1]=g_frame;
  Stage1911Frame(c,18,276,globals,world);Check(g_1911FramePending.valid,"actual main FRAME geometry stages its completed split upload");
  Confirm1911Frame(c,8664,1338,world,1);Check(!g_1911FrameFinal.valid,"magazine draw cannot publish FRAME parent");
  Confirm1911Frame(c,18,276,camera,1);Check(!g_1911FrameFinal.valid,"wrong bound world buffer cannot publish FRAME parent");
  Confirm1911Frame(c,18,276,world,4);Check(!g_1911FrameFinal.valid,"native hand-placement variant cannot publish insertion parent");
  Confirm1911Frame(c,18,276,world,1);Check(g_1911FrameFinal.valid,"completed stereo FRAME draw promotes immutable insertion parent");
  const Mat4 expected=Multiply(g_1911FramePending.toBody,Multiply(HullView(native),HullWorld(globals)));
  for(int i=0;i<16;++i)Check(std::fabs(expected.m[i]-g_1911FrameFinal.body.m[i])<1e-10,"parent matches the uploaded FRAME and its actual draw camera placement");
  g_pistolReload.rootBody=expected;g_pistolReload.rootBody.m[3]+=.5; // predictor/root changes after the actual draw
  const auto unchanged=g_pistolGuides;const float* p=PistolGuidePoint(0);const V3 center=Apply(expected,{p[0],p[1],p[2]});
  const Mat4 box=PistolGuideBox(g_pistolReload.rootBody,0);Check(Length(Sub(center,{box.m[3],box.m[7],box.m[11]}))<1e-9,"blue insertion box stays on final FRAME despite intermediate root replacement");
  Check(PistolGuideContact(g_pistolReload.rootBody,0,center),"physical insertion accepts the same final FRAME center");
  V3 outside={};outside=Apply(box,{g_pistolGuides.size[0][0]*(std::max)(.1f,g_worldScale)*.5+g_pistolGuides.handDiameter*(std::max)(.1f,g_worldScale)*.5+.002,0,0});
  Check(!PistolGuideContact(g_pistolReload.rootBody,0,outside),"unchanged saved box and hand radii still reject outside contact");
  Check(!memcmp(&unchanged,&g_pistolGuides,sizeof(unchanged)),"resolving final FRAME never rewrites saved calibration");
  ++pairs;
 }
 g_pistolMenu.open=true;g_frame+=20;Mat4 visual={};Check(Resolve1911Insertion(g_pistolReload.rootBody,visual,true),"Insert retains the confirmed FRAME snapshot for positioning");Check(!Resolve1911Insertion(g_pistolReload.rootBody,visual,false),"Insert snapshot cannot authorize physical reload");
 g_pistolMenu.open=false;Check(!Resolve1911Insertion(g_pistolReload.rootBody,visual,false),"stale final draw cannot silently fall back to predicted root");
 g_1911FrameFinal.epoch=g_armPoseEpoch+1;Check(!Resolve1911Insertion(g_pistolReload.rootBody,visual,true),"pose lifecycle rejects cached insertion parent");
 g_weaponMechanicProfile=1;Check(Resolve1911Insertion(g_pistolReload.rootBody,visual,false),"6P9 retains its existing parent path");
 g_1911FramePending=pending;g_1911FrameFinal=final;g_pistolReload=pistol;g_pistolGuides=settings;g_pistolMenu=menu;g_pistolGuidePreview=preview;g_frame=frame;g_weaponMechanicProfile=profile;g_weaponMechanicFrame=mechanic;g_mainXScale=mainScale;g_guideShaderPlacement[1]=placement;g_guideShaderPlacementFrame[1]=placementFrame;
 c->ClearState();vb->Release();ib->Release();world->Release();camera->Release();printf("PASS: frame45 %d captured FRAME poses, same-parent contact/box, draw ownership, stale/Insert rejection, calibration preservation and other-profile isolation\n",pairs);
}
