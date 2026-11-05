// Replay only the exact native normal arrow when the engine omits its draw.
// Matches scene render-target/depth pass; bow skin and rigid arrow use different VS.
// Captured GPU resources are private copies; no ammo/damage state is changed.
//
// PID82728 timeline (fc3_animation_82728_*.bin, kind 1): every first-person
// arrow draw (VB 19776, VS C386478F2F9495EB) bound the WEAPON camera
// (xScale 1.554 against main 0.368), and the game drew it only while its own
// reload had nocked an arrow (equip frames 2246-2302 and 2739-2855), never
// again after the first physical release. The old HullCamera test (xScale
// within 1% of main) rejected all of those draws, so the four copied passes
// were fired/stuck arrows under the WORLD camera; those world draws also
// refreshed gameFrame ("the game draws it itself"), and the replay rejected
// the bow draw's own weapon camera by the same test. Nothing was ever replayed.
void ClearBowArrowPart(HullPart& p){
 SafeRelease(p.layout);SafeRelease(p.vs);SafeRelease(p.ps);SafeRelease(p.vb);SafeRelease(p.ib);SafeRelease(p.globals);SafeRelease(p.clip);
 for(auto*& b:p.psCb)SafeRelease(b);for(auto*& t:p.textures)SafeRelease(t);for(auto*& a:p.samplers)SafeRelease(a);
 SafeRelease(p.raster);SafeRelease(p.depth);SafeRelease(p.blend);p=HullPart{};
}
struct BowArrowPass{HullPart part;UINT stride=0,offset=0;unsigned long long vs=0;LONG frame=-1;float xScale=0;};
struct BowArrowVisual{BowArrowPass pass[6];int count=0;ID3D11DeviceContext* deferred=nullptr;LONG gameFrame=-100000,worldFrame=-100000,stateFrame=-1;bool failed=false,logged=false,nativeWas=false;LONG replayed=0;UINT notes=0,skipLines=0,successLines=0;ULONGLONG skipAt[12]={},successAt=0;}g_bowArrow;
bool BowArrowDraw(const fc3weapon::Geometry& d,UINT start,UINT count){return BowArrowGeometry(d)&&start==0&&count==2352;}
// The bow is a first-person model: its own draws and its arrow's bind the
// weapon camera (main view flag, projection unlike the world camera's), the
// same rule HandleDraw uses to tell first-person pieces apart. Fired or stuck
// arrows reuse the mesh under the world camera and are not the bow's arrow.
bool BowWeaponCamera(ID3D11Buffer* b,CameraSummary& camera){UINT size=sizeof(camera);return b&&SUCCEEDED(b->GetPrivateData(kCameraSummaryGuid,&size,&camera))&&size==sizeof(camera)&&camera.mainView&&fabs(camera.xScale-g_mainXScale)>=g_mainXScale*.01f;}
// Bounded native-draw timeline: where the game put the arrow and when it
// started or stopped drawing it, against the physical state at that moment.
void BowArrowNote(const char* what,ID3D11DeviceContext* context,const CameraSummary& camera){
 auto& V=g_bowArrow;if(V.notes>=24)return;++V.notes;float rows[16]={};ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);const UINT bytes=ReadCopy(cb,rows,sizeof(rows));SafeRelease(cb);if(bytes<60)rows[12]=rows[13]=rows[14]=0;
 Log("[BOW ARROW] native arrow draw %s at frame %ld: camera xScale %.3f (main %.3f, mainView %u), native world %.2f %.2f %.2f (cb0 %u bytes), held %d nocked %d falling %d draw %.2f",what,static_cast<long>(g_frame),camera.xScale,g_mainXScale,camera.mainView,rows[3]+rows[12],rows[7]+rows[13],rows[11]+rows[14],bytes,g_bow.held,g_bow.nocked,g_bow.falling,g_bow.draw);
}
// This draw's pass key: vertex shader + render-target / depth formats.
bool BowArrowPassKey(ID3D11DeviceContext* context,unsigned long long& vs,DXGI_FORMAT (&formats)[8],DXGI_FORMAT& depthFormat,UINT* targets=nullptr)
{
 ID3D11VertexShader* shader=nullptr;context->VSGetShader(&shader,nullptr,nullptr);vs=ShaderHash(shader);SafeRelease(shader);
 ID3D11RenderTargetView* views[8]={};ID3D11DepthStencilView* depth=nullptr;context->OMGetRenderTargets(8,views,&depth);
 UINT n=0;for(UINT i=0;i<8;++i){formats[i]=HullFormat(views[i]);if(views[i])++n;SafeRelease(views[i]);}if(targets)*targets=n;
 depthFormat=HullFormat(depth);const bool ok=depth!=nullptr;SafeRelease(depth);return ok;
}
int BowArrowFindPass(unsigned long long vs,const DXGI_FORMAT (&formats)[8],DXGI_FORMAT depthFormat)
{
 auto& V=g_bowArrow;
 for(int k=0;k<V.count;++k){const auto& p=V.pass[k].part;bool same=V.pass[k].vs==vs&&p.depthFormat==depthFormat;for(UINT i=0;i<8&&same;++i)same=p.formats[i]==formats[i];if(same)return k;}
 return -1;
}
int BowArrowFindTargetPass(const DXGI_FORMAT (&formats)[8],DXGI_FORMAT depthFormat){
 for(int k=0;k<g_bowArrow.count;++k){const auto& p=g_bowArrow.pass[k].part;bool same=p.ready&&p.depthFormat==depthFormat;for(int i=0;i<8&&same;++i)same=p.formats[i]==formats[i];if(same)return k;}return -1;
}
void ReleaseBowArrowResources(){for(auto& p:g_bowArrow.pass)ClearBowArrowPart(p.part);SafeRelease(g_bowArrow.deferred);g_bowArrow=BowArrowVisual{};}
void ObserveBowArrowDraw(ID3D11DeviceContext* context,const DrawCall& call)
{
 if(call.kind!=0||call.d!=0||!BowModel())return;const auto d=NativeWeaponGeometry(context,call);if(!BowArrowDraw(d,call.b,call.a))return;
 auto& V=g_bowArrow;CameraSummary camera={};ID3D11Buffer* cameraCb=nullptr;context->VSGetConstantBuffers(1,1,&cameraCb);const bool weapon=BowWeaponCamera(cameraCb,camera);SafeRelease(cameraCb);
 if(!weapon){if(g_frame-V.worldFrame>120)BowArrowNote("seen under the WORLD camera (a fired or stuck arrow, not the bow's; ignored)",context,camera);V.worldFrame=g_frame;return;}
 if(g_frame-V.gameFrame>30)BowArrowNote("RESUMED under the weapon camera (the game is drawing the nocked arrow itself)",context,camera);
 V.gameFrame=g_frame;if(!g_dev||V.failed||V.count>=6)return;
 unsigned long long vs=0;DXGI_FORMAT formats[8]={},depthFormat=DXGI_FORMAT_UNKNOWN;UINT targets=0;if(!BowArrowPassKey(context,vs,formats,depthFormat,&targets)||BowArrowFindPass(vs,formats,depthFormat)>=0)return;
 ID3D11Buffer* cb=nullptr;context->VSGetConstantBuffers(0,1,&cb);std::vector<float> data(kSmallCbMax/4);const UINT bytes=ReadCopy(cb,data.data(),kSmallCbMax);SafeRelease(cb);
 if(bytes<80){if(V.notes<24){++V.notes;Log("[BOW ARROW] weapon-camera arrow pass not copied: cb0 copy is %u bytes (vs %016llX, %u target(s))",bytes,vs,targets);}return;}data.resize(bytes/4);
 auto& P=V.pass[V.count];P.vs=vs;P.xScale=camera.xScale;
 ID3D11Buffer* vb=nullptr;context->IAGetVertexBuffers(0,1,&vb,&P.stride,&P.offset);SafeRelease(vb);
 bool ok=SnapshotVisualPart(context,call,P.part,data);if(ok&&!V.deferred)ok=SUCCEEDED(g_dev->CreateDeferredContext(0,&V.deferred));
 if(!ok){ClearBowArrowPart(P.part);P=BowArrowPass{};Log("[BOW ARROW] a weapon-camera arrow pass could not be copied");return;}
 ++V.count;Log("[BOW ARROW] arrow pass %d copied from the WEAPON camera (vs %016llX, %u target(s), first format %d, depth format %d, xScale %.3f): available for replay; nothing drawn by the mod yet",V.count,vs,P.part.targets,static_cast<int>(formats[0]),static_cast<int>(depthFormat),camera.xScale);
}
// One line per reason every 3 s (the "game draws it itself" reason once), 60 lines in all.
void BowArrowSkip(int reason,const char* text,float xScale,UINT mainView){
 auto& V=g_bowArrow;const ULONGLONG now=GetTickCount64();if(reason<0||reason>=12||V.skipLines>=60||now<V.skipAt[reason])return;V.skipAt[reason]=reason==0?~0ull:now+3000;++V.skipLines;
 Log("[BOW ARROW] replay skipped at frame %ld: %s | copied passes %d, weapon-camera arrow age %ld frame(s), world arrow age %ld, this draw's camera xScale %.3f (main %.3f, mainView %u), held %d nocked %d falling %d draw %.2f",static_cast<long>(g_frame),text,V.count,static_cast<long>(g_frame-V.gameFrame),static_cast<long>(g_frame-V.worldFrame),xScale,g_mainXScale,mainView,g_bow.held,g_bow.nocked,g_bow.falling,g_bow.draw);
}
void ReplayBowArrow(ID3D11DeviceContext* context,const DrawCall& call)
{
 auto& V=g_bowArrow;const auto& R=g_bow;
 if(!BowModel()||!MotionInteractionEnabled())return;
 if(call.kind!=0||call.d!=0||call.b!=14982||call.a!=7140)return;
 {const auto d=NativeWeaponGeometry(context,call);if(!BowGeometry(d))return;}
 ID3D11Buffer* cameraBuffer=nullptr;context->VSGetConstantBuffers(1,1,&cameraBuffer);CameraSummary camera={};const bool weaponCamera=BowWeaponCamera(cameraBuffer,camera);
 const bool nativeNow=g_frame-V.gameFrame<=1; // the game draws it itself
 if(weaponCamera&&V.stateFrame!=g_frame){V.stateFrame=g_frame;if(V.nativeWas&&!nativeNow)BowArrowNote("ENDED (last weapon-camera arrow draw was 2+ frames ago)",context,camera);V.nativeWas=nativeNow;}
 const bool wanted=(R.held||R.nocked||R.falling)&&BowActive()&&(R.falling||BowHandsReady());
 if(!wanted){SafeRelease(cameraBuffer);return;}
 if(nativeNow){BowArrowSkip(0,"the game is drawing the arrow itself this frame (BowWorld places it)",camera.xScale,camera.mainView);SafeRelease(cameraBuffer);return;}
 if(!V.count||!V.deferred){BowArrowSkip(1,"no weapon-camera arrow pass has been copied yet (the game has not drawn its own arrow since the bow appeared)",camera.xScale,camera.mainView);SafeRelease(cameraBuffer);return;}
 if(!weaponCamera){if(camera.mainView)BowArrowSkip(4,"this bow draw is not under the weapon camera",camera.xScale,camera.mainView);SafeRelease(cameraBuffer);return;} // shadow/world passes of the bow: silent
 unsigned long long vs=0;DXGI_FORMAT formats[8]={},depthFormat=DXGI_FORMAT_UNKNOWN;UINT targets=0;if(!BowArrowPassKey(context,vs,formats,depthFormat,&targets)){BowArrowSkip(2,"this bow draw has no depth target",camera.xScale,camera.mainView);SafeRelease(cameraBuffer);return;}
 const int k=BowArrowFindTargetPass(formats,depthFormat);
 if(k<0){char text[160];snprintf(text,sizeof(text),"no copied pass matches this bow draw's targets (%u target(s), first format %d, depth format %d, vs %016llX)",targets,static_cast<int>(formats[0]),static_cast<int>(depthFormat),vs);BowArrowSkip(3,text,camera.xScale,camera.mainView);SafeRelease(cameraBuffer);return;}
 if(V.pass[k].frame==g_frame){SafeRelease(cameraBuffer);return;}
 auto& P=V.pass[k];const auto& p=P.part;
 bool ok=cameraBuffer!=nullptr;
 std::vector<float> current(p.data.size());
 {ID3D11Buffer* g=nullptr;context->VSGetConstantBuffers(0,1,&g);const UINT need=static_cast<UINT>(current.size()*4);const UINT got=ReadCopy(g,current.data(),need);SafeRelease(g);if(got!=need){char text[120];snprintf(text,sizeof(text),"bow draw cb0 copy is %u bytes, the copied arrow pass needs %u",got,need);BowArrowSkip(5,text,camera.xScale,camera.mainView);ok=false;}}
 ID3D11RenderTargetView* targetViews[8]={},*twins[8]={};ID3D11DepthStencilView* depth=nullptr,*twinDepth=nullptr;
 context->OMGetRenderTargets(8,targetViews,&depth);
 if(ok){for(UINT i=0;i<p.targets;++i)if(targetViews[i]){twins[i]=static_cast<ID3D11RenderTargetView*>(TwinView(targetViews[i],ViewKind::RenderTarget,false));ok=ok&&twins[i];}
  twinDepth=static_cast<ID3D11DepthStencilView*>(TwinView(depth,ViewKind::DepthStencil,false));ok=ok&&twinDepth&&g_eyeCb[1][0]&&g_eyeCb[1][1];if(!ok)BowArrowSkip(6,"no twin (right-eye) target/depth or eye constants for this pass",camera.xScale,camera.mainView);}
 Mat4 viewInverse={};if(ok&&!Invert(HullView(camera),viewInverse)){ok=false;BowArrowSkip(7,"bow draw camera view is not invertible",camera.xScale,camera.mainView);}
 if(ok)
 {
  auto data=current;WritePistolWorld(data.data(),Multiply(viewInverse,Multiply(ArmBodyToCamera(),BowArrowBodyNow())));
  D3D11_VIEWPORT viewport[16]={};UINT nvp=16;context->RSGetViewports(&nvp,viewport);D3D11_RECT scissor[16]={};UINT nsc=16;context->RSGetScissorRects(&nsc,scissor);
  const bool own=g_own;g_own=true;auto* dc=V.deferred;dc->ClearState();dc->RSSetViewports(nvp,viewport);dc->RSSetScissorRects(nsc,scissor);
  context->UpdateSubresource(p.globals,0,nullptr,data.data(),0,0);
  UINT stride=P.stride,offset=P.offset;dc->IASetVertexBuffers(0,1,&p.vb,&stride,&offset);
  dc->IASetIndexBuffer(p.ib,DXGI_FORMAT_R16_UINT,p.indexOffset);dc->IASetInputLayout(p.layout);dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  dc->VSSetShader(p.vs,nullptr,0);dc->PSSetShader(p.ps,nullptr,0);
  ID3D11Buffer* vertexCb[3]={p.globals,cameraBuffer,p.clip};dc->VSSetConstantBuffers(0,3,vertexCb);
  dc->PSSetConstantBuffers(0,8,const_cast<ID3D11Buffer**>(p.psCb));dc->PSSetShaderResources(0,16,const_cast<ID3D11ShaderResourceView**>(p.textures));dc->PSSetSamplers(0,16,const_cast<ID3D11SamplerState**>(p.samplers));
  dc->RSSetState(p.raster);dc->OMSetDepthStencilState(p.depth,p.stencil);dc->OMSetBlendState(p.blend,p.blendFactor,p.mask);
  for(int eye=0;eye<2;++eye)
  {
   dc->OMSetRenderTargets(p.targets,eye?twins:targetViews,eye?twinDepth:depth);
   dc->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyeCb[1][eye]);
   if(HasFlag(p.ps,kPsPatchedGuid))dc->PSSetConstantBuffers(dxbc::kEyeCbSlot,1,&g_eyePsCb[eye]);
   dc->DrawIndexed(p.draw.a,p.draw.b,p.draw.d);
  }
  ID3D11CommandList* list=nullptr;
  if(SUCCEEDED(dc->FinishCommandList(FALSE,&list))){context->ExecuteCommandList(list,TRUE);list->Release();P.frame=g_frame;++V.replayed;
   const ULONGLONG now=GetTickCount64();
   if(!V.logged){Log("[BOW ARROW] arrow DRAWN by the mod (both eyes, pass %d of %d, weapon camera xScale %.3f): the game is not drawing it",k+1,V.count,camera.xScale);V.logged=true;V.successAt=now+5000;}
   else if(V.successLines<20&&now>=V.successAt){++V.successLines;V.successAt=now+5000;Log("[BOW ARROW] arrow still drawn by the mod at frame %ld (%ld replayed draws so far; held %d nocked %d falling %d draw %.2f)",static_cast<long>(g_frame),static_cast<long>(V.replayed),g_bow.held,g_bow.nocked,g_bow.falling,g_bow.draw);}}
  else BowArrowSkip(8,"deferred command list could not be finished",camera.xScale,camera.mainView);
  g_own=own;
 }
 for(UINT i=0;i<8;++i){SafeRelease(targetViews[i]);SafeRelease(twins[i]);}
 SafeRelease(depth);SafeRelease(twinDepth);SafeRelease(cameraBuffer);
}
