
void TestPistolPreview(ID3D11Device* device,ID3D11DeviceContext* context)
{
 auto oldSettings=g_pistolGuides;auto oldMenu=g_pistolMenu;auto oldPistol=g_pistolReload;auto oldPreview=g_pistolGuidePreview;
 const bool oldAim=g_aimActive,oldLeft=g_handLeftValid,oldRight=g_handRightValid;g_aimActive=false;
 g_pistolGuides=PistolGuideSettings{};g_pistolGuides.preview=true;g_pistolReload=PistolReloadState{};g_pistolReload.enabled=true;
 g_pistolMenu=PistolGuideMenu{};SetInsertOpen(true);g_handRightValid=g_handLeftValid=false;
 fc3pad::Controllers c={};fc3pad::Publish(c);g_weaponMechanicProfile=20;g_weaponMechanicFrame=g_frame;
 for(int i=0;i<kEyeVariants;++i){g_guideShaderPlacement[i]=HandIdentity();g_guideShaderPlacementFrame[i]=g_frame;}
 ID3D11Buffer* vb=Buffer(device,177904,D3D11_BIND_VERTEX_BUFFER),*ib=Buffer(device,32682,D3D11_BIND_INDEX_BUFFER),*world=Buffer(device,352,D3D11_BIND_CONSTANT_BUFFER),*camera=Buffer(device,896,D3D11_BIND_CONSTANT_BUFFER);
 UINT stride=32,offset=240;context->IASetVertexBuffers(0,1,&vb,&stride,&offset);context->IASetIndexBuffer(ib,DXGI_FORMAT_R16_UINT,0);
 CameraSummary native={};native.right[0]=native.up[1]=native.forward[2]=1;native.mainView=1;camera->SetPrivateData(kCameraSummaryGuid,sizeof(native),&native);
 float globals[88]={};globals[0]=globals[5]=globals[10]=1;globals[3]=.2f;globals[7]=-.3f;globals[11]=.8f;world->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);ID3D11Buffer* cb[2]={world,camera};context->VSSetConstantBuffers(0,2,cb);
 DrawCall draw={};draw.a=276;draw.b=18;
 Check(!PistolHandling(),"controllers asleep do not authorize physical pistol handling");
 ObservePistolGuideDraw(context,draw,4);ObservePistolGuidePalm(HandIdentity());V3 port={},hand={};float radius=0;bool palm=false;
 Check(PistolMenuGuidePoints(port,hand,radius,palm)&&palm,"native displayed pistol and both markers remain available with no tracked controllers");
 Check(std::fabs(port.x-(.2+g_pistolGuides.port[0]))<1e-6,"marker follows actual displayed native FRAME");
 const auto before=g_pistolReload;
 for(int frame=0;frame<90;++frame)
 {
  ++g_frame;g_weaponMechanicFrame=g_frame;for(int k=0;k<kEyeVariants;++k)g_guideShaderPlacementFrame[k]=g_frame;
  globals[3]+=.001f;world->SetPrivateData(kCbCopyGuid,sizeof(globals),globals);g_pistolGuides.port[0]+=.001f;
  ObservePistolGuideDraw(context,draw,4);TickPistolGuideMenu(c,false);
  Check(PistolMenuGuidePoints(port,hand,radius,palm)&&palm&&std::fabs(port.x-(globals[3]+g_pistolGuides.port[0]))<1e-6,"mouse calibration remains visible and live through 90 controller-inactive frames");
 }
 Check(!memcmp(&before,&g_pistolReload,sizeof(before)),"visual anchors never refresh physical contact/rack state or authorize ammo");
 g_pistolMenu.target=1;Check(PistolMenuGuidePoints(port,hand,radius,palm),"slide guide visible without live right controller");const auto slide=port;
 g_pistolMenu.target=2;g_pistolGuides.hand[0]+=.04f;Check(PistolMenuGuidePoints(port,hand,radius,palm)&&Length(Sub(port,slide))<1e-6,"editing hand marker retains slide/weapon marker alongside it");
 // Already moved solid world is recorded directly with variant1; no double hand transform.
 globals[3]=.6f;ObservePistolGuideRoot(context,18,276,globals,1);g_pistolMenu.target=0;Check(PistolMenuGuidePoints(port,hand,radius,palm)&&std::fabs(port.x-(.6+g_pistolGuides.port[0]))<1e-6,"completed motion world marker agrees with displayed FRAME");
 float source[88]={};source[0]=source[5]=source[10]=1;source[3]=-.8f;world->SetPrivateData(kCbCopyGuid,sizeof(source),source);ObservePistolGuideDraw(context,draw,1);
 Check(PistolMenuGuidePoints(port,hand,radius,palm)&&std::fabs(port.x-(.6+g_pistolGuides.port[0]))<1e-6,"still-bound native source cannot replace completed weapon guide parent");
 g_pistolMenu.open=false;++g_frame;g_weaponMechanicFrame=g_frame;globals[3]=.72f;ObservePistolGuideRoot(context,18,276,globals,1);Mat4 movedPalm=HandIdentity();movedPalm.m[3]=.23;ObservePistolGuidePalm(movedPalm);
 Check(g_pistolGuidePreview.rootFrame==g_frame&&g_pistolGuidePreview.palmFrame==g_frame&&std::fabs(g_pistolGuidePreview.root.m[3]-.72)<1e-6&&std::fabs(g_pistolGuidePreview.palm.m[3]-.23)<1e-6,"gameplay observes current displayed weapon and palm without calibration menu");g_pistolMenu.open=true;
 g_weaponMechanicProfile=0;Check(!PistolMenuGuidePoints(port,hand,radius,palm),"switching away hides pistol-only preview");g_weaponMechanicProfile=20;
 ++g_armPoseEpoch;Check(!PistolMenuGuidePoints(port,hand,radius,palm),"pose epoch change rejects old calibration anchors");--g_armPoseEpoch;
 g_weaponGuideShown[20]=false;Check(!PistolMenuGuidePoints(port,hand,radius,palm),"per-weapon boolean hides preview explicitly");g_weaponGuideShown[20]=true;
 g_frame+=3;g_weaponMechanicFrame=g_frame;Check(PistolMenuGuidePoints(port,hand,radius,palm),"Insert preserves cached visual guides when game draws pause");g_pistolMenu.open=false;Check(!PistolMenuGuidePoints(port,hand,radius,palm),"gameplay still rejects stale visual guides");g_pistolMenu.open=true;
 SetInsertOpen(false);Check(g_pistolGuidePreview.rootFrame<0,"closing menu clears calibration-only anchors");fc3pad::Publish(c);
 ID3D11Buffer* none[2]={};context->VSSetConstantBuffers(0,2,none);vb->Release();ib->Release();world->Release();camera->Release();
 g_pistolGuides=oldSettings;g_pistolMenu=oldMenu;g_pistolReload=oldPistol;g_pistolGuidePreview=oldPreview;g_aimActive=oldAim;g_handLeftValid=oldLeft;g_handRightValid=oldRight;
 std::puts("PASS: calibration markers use actual native/completed weapon draw anchors while controllers are inactive; live edits, hand/weapon coexistence, switch/epoch/hidden cleanup and no physical reload authorization.");
}
