void TestSceneRaster132(ID3D11Device* device,ID3D11DeviceContext* context,int eye,ImVec2 dot){
 const auto identity=HandIdentity();const auto scene=OpticSceneMatrix132(identity,0,eye),gun=OpticSceneMatrix132(identity,4,eye);
 auto* blob=Native1911Compile48("cbuffer E:register(b13){float4 r[17];};float4 main(uint id:SV_VertexID):SV_Position{float2 a[3]={float2(-.025,-.3),float2(.025,-.3),float2(0,.3)};float4 p=float4(a[id]+r[16].xy,.5,1);return p+float4(dot(r[0],p),dot(r[1],p),dot(r[2],p),dot(r[3],p));}","vs_5_0");ID3D11VertexShader* vs=nullptr;Check(blob&&SUCCEEDED(device->CreateVertexShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&vs)),"scene assist raster VS");SafeRelease(blob);
 blob=Native1911Compile48("float4 main():SV_Target{return float4(1,0,0,1);}","ps_5_0");ID3D11PixelShader* ps=nullptr;Check(blob&&SUCCEEDED(device->CreatePixelShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&ps)),"scene assist raster PS");SafeRelease(blob);
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=512;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;ID3D11Texture2D *target=nullptr,*staging=nullptr;ID3D11RenderTargetView* rtv=nullptr;
 Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&target))&&SUCCEEDED(device->CreateRenderTargetView(target,nullptr,&rtv)),"scene assist raster output");td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(SUCCEEDED(device->CreateTexture2D(&td,nullptr,&staging)),"scene assist raster readback");
 auto* cb=Buffer(device,17*16,D3D11_BIND_CONSTANT_BUFFER);context->ClearState();context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetConstantBuffers(dxbc::kEyeCbSlot,1,&cb);context->OMSetRenderTargets(1,&rtv,nullptr);D3D11_VIEWPORT vp={0,0,512,512,0,1};context->RSSetViewports(1,&vp);D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;ID3D11RasterizerState* rs=nullptr;Check(SUCCEEDED(device->CreateRasterizerState(&rd,&rs)),"scene raster state");context->RSSetState(rs);
 auto pixels=[&](const Mat4& matrix){float rows[68]={};for(int i=0;i<16;++i)rows[i]=float(matrix.m[i]-(i%5==0));rows[64]=dot.x;rows[65]=dot.y;context->UpdateSubresource(cb,0,nullptr,rows,0,0);const float clear[4]={};context->ClearRenderTargetView(rtv,clear);context->Draw(3,0);context->CopyResource(staging,target);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&m)),"actual scene-assist GPU pixels");UINT count=0;for(UINT y=0;y<512;++y)for(UINT x=0;x<512;++x)if(static_cast<BYTE*>(m.pData)[y*m.RowPitch+x*4]>200)++count;context->Unmap(staging,0);return count;};
 const UINT normal=pixels(identity),zoomed=pixels(scene),weapon=pixels(gun);Check(normal>20&&zoomed>normal*1.1&&weapon==normal,"actual GPU scene detail grows while weapon raster remains pixel-identical in size");
 context->ClearState();SafeRelease(rs);SafeRelease(cb);SafeRelease(vs);SafeRelease(ps);SafeRelease(rtv);SafeRelease(staging);SafeRelease(target);
}
void TestHolographic131(ID3D11Device* device,ID3D11DeviceContext* context){
 context->ClearState();ClearScopeResources60();g_dev=device;g_ctx=context;g_oCopyResource=ProbeCopy;g_oUpdateSubresource=Update;
 g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=g_aimActive=true;g_weaponMotionGuard=g_flatViewActive=false;
 g_pistolMenu.open=false;g_pistolReload={};g_pistolReload.enabled=g_pistolReload.magazinePresent=true;g_reload={};g_r700={};g_machete.held=false;g_twoHand=false;g_holoEnabled131=true;g_holoGuide133=false;g_redDotColour52=0;
 fc3pad::SetPistolCalibration(false);fc3pad::SetGameplayControls(true);fc3pad::Controllers controls={};controls.active=true;fc3pad::Publish(controls);
 ++g_armPoseEpoch;++g_frame;g_weaponMechanicProfile=31;g_weaponMechanicFrame=g_frame;g_shotAimProfile129=31;PublishReticleAim73({0,0,1});ResetScopeStabilization72();
 for(int r=0;r<3;++r)for(int c=0;c<3;++c)g_aimQ[r][c]=r==c?1.:0.;
 const auto identity=HandIdentity();for(int side=0;side<2;++side)for(int i=0;i<16;++i)g_handBody[side][i]=float(identity.m[i]);
 for(int eye=0;eye<2;++eye){auto& e=g_eyeViews[eye];for(int i=0;i<16;++i)e.view[i]=float(identity.m[i]);e.view[3]=eye?-.032f:.032f;e.tanLeft=eye?-.9f:-1.1f;e.tanRight=eye?1.1f:.9f;e.tanUp=1;e.tanDown=-1;g_eyeViewsBody[eye]=e;}
 constexpr UINT width=512,height=512;D3D11_TEXTURE2D_DESC td={};td.Width=width;td.Height=height;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 ID3D11Texture2D* images[2]={};std::vector<BYTE> original[2];for(int eye=0;eye<2;++eye){original[eye].resize(width*height*4);for(UINT i=0;i<width*height;++i){original[eye][i*4]=eye?100:40;original[eye][i*4+1]=eye?130:70;original[eye][i*4+2]=eye?160:90;original[eye][i*4+3]=255;}D3D11_SUBRESOURCE_DATA data={original[eye].data(),width*4,0};Check(SUCCEEDED(device->CreateTexture2D(&td,&data,&images[eye])),"holo131 separate per-eye world textures");}
 auto read=[&](int eye){auto d=td;d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&staging)),"holo staging");context->CopyResource(staging,images[eye]);D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped)),"holo actual pixels");std::vector<BYTE> pixels(width*height*4);for(UINT y=0;y<height;++y)memcpy(pixels.data()+y*width*4,static_cast<BYTE*>(mapped.pData)+y*mapped.RowPitch,width*4);context->Unmap(staging,0);SafeRelease(staging);return pixels;};
 auto packet=[&](int spec,double x){
  Mat4 inverseRig={};Check(Invert(ScopeRigBody72(0),inverseRig),"physical holo rig inverse");const auto& s=kScopeSpecs60[spec];
  for(int eye=0;eye<2;++eye){auto& p=g_scopeEyes60[eye];p={};p.spec=spec;p.ownerProfile=31;p.epoch=g_armPoseEpoch;p.frame=g_frame;p.bodyValid72=true;p.bodySide72=0;
   const V3 centre={x,0,.5};p.localCenter72=Apply(inverseRig,centre);p.localAxis72=Apply(inverseRig,Add(centre,{0,0,100}));
   for(int i=0;i<64;++i){const auto v=s.outline[i];p.localEdge72[i]=Apply(inverseRig,Add(centre,{v.x-s.center.x,v.z-s.center.z,v.y-s.center.y}));}
  }
 };
 auto* oldPsBuffer=Buffer(device,16,D3D11_BIND_CONSTANT_BUFFER);context->PSSetConstantBuffers(0,1,&oldPsBuffer);
 for(int spec:{5,6})for(int alignedEye:{0,1}){
  packet(spec,alignedEye?.032:-.032);for(int eye=0;eye<2;++eye)context->UpdateSubresource(images[eye],0,nullptr,original[eye].data(),width*4,0);
  Check(HoloAllowed131()&&!ScopeReady60()&&!ScopeCleanAllowed61(),"physical hologram works one-handed with no LT and never starts telescope rendering");
  float rows[dxbc::kEyeRows*4]={};ScopePresentation62(rows,1,0);for(float value:rows)Check(value==0,"holo cannot enlarge or move weapon shader rows");
  ImVec2 dot[2];ScopeOptics60 lens[2];for(int eye=0;eye<2;++eye){Check(HoloDot131(eye,dot[eye])&&HoloPacket131(eye,lens[eye]),"both-eye native window and collimated ray available");const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);Check(fabs(dot[eye].x-projection.m[2])<1e-7&&fabs(dot[eye].y-projection.m[6])<1e-7,"collimated shot ray retains each eye asymmetric projection");}
  // Exercise the production scene-only matrix with both real XR projections.
  Check(OpticAligned132(),"scene assist only enters through actual aligned glass");
  controls.leftTrigger=1;fc3pad::Publish(controls);g_holoSceneEnabled132=true;g_holoSceneZoom132=2;g_opticScene132={};UpdateOpticScene132();g_opticScene132.tick-=40;UpdateOpticScene132();
  Check(g_opticScene132.scale>1&&g_opticScene132.scale<2,"scene zoom eases in instead of snapping");
  for(int eye=0;eye<2;++eye){const auto base=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);const auto scene=OpticSceneMatrix132(base,0,eye);const auto weapon=OpticSceneMatrix132(base,4,eye);
   Check(memcmp(base.m,weapon.m,sizeof(base.m))==0,"scene assistance never changes weapon projection");
   const V3 ray={0,0,1};const auto aimed=Apply(scene,ray);Check(fabs(aimed.x-dot[eye].x)<1e-6&&fabs(aimed.y-dot[eye].y)<1e-6,"scene zoom retains each eye asymmetric aiming ray with no artificial convergence");
   Check(fabs(scene.m[0]-base.m[0]*g_opticScene132.scale)<1e-6,"scene details magnify while sight remains native size");TestSceneRaster132(device,context,eye,dot[eye]);context->PSSetConstantBuffers(0,1,&oldPsBuffer);
   Mat4 inverse={};Check(Invert(scene,inverse),"zoomed scene has inverse for depth reconstruction");const auto round=Multiply(inverse,scene);for(int i=0;i<16;++i)Check(fabs(round.m[i]-(i%5==0))<1e-6,"zoom projection and pixel reconstruction agree");
  }
  g_pistolMenu.open=true;UpdateOpticScene132();Check(g_opticScene132.scale==1,"opening menu drops scene assist immediately");g_pistolMenu.open=false;g_holoSceneEnabled132=false;UpdateOpticScene132();
  g_holoLensEnabled132=true;g_holoLensZoom132=1.75f;Check(HoloLensActive132()&&ScopeCleanAllowed61()&&fabs(ScopeZoom63()-1.75f)<1e-6,"independent reflex lens toggle starts bounded clean optics only when aligned");
  for(int eye=0;eye<2;++eye){auto* clean=TwinResource(images[eye],true,kScopeClean61);Check(clean!=nullptr,"reflex isolated clean eye source");auto source=original[eye];for(size_t i=0;i<source.size();i+=4){source[i]=220;source[i+1]=30;source[i+2]=40;}context->UpdateSubresource(clean,0,nullptr,source.data(),width*4,0);SafeRelease(clean);const ScopeCleanStamp61 stamp={g_frame,g_armPoseEpoch};images[eye]->SetPrivateData(kScopeCleanStamp61,sizeof(stamp),&stamp);}
  g_scopeCleanFrame61=g_frame;g_scopeCleanEpoch61=g_armPoseEpoch;g_scopeCleanDraws61=1;g_scopeCleanFailed61=false;UpdateScopeProjection62(identity.m,0);UpdateScopeProjection62(identity.m,1);RenderScopes60(images[0],images[1]);Check(g_scopeRendered60&&g_holoRenderedFrame131==g_frame,"reflex lens magnifier renders through clean source and retains collimated dot");
  for(int eye=0;eye<2;++eye){const auto pixels=read(eye);UINT changed=0;for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){const UINT at=(y*width+x)*4;if(memcmp(pixels.data()+at,original[eye].data()+at,4)){++changed;Check(OpticContains131(lens[eye],{2.f*(x+.5f)/width-1,1-2.f*(y+.5f)/height}),"reflex magnification and dot cannot escape native glass");}}Check(eye==alignedEye?changed>100:changed==0,"reflex magnifier uses one aligned eye, other eye remains original");context->UpdateSubresource(images[eye],0,nullptr,original[eye].data(),width*4,0);}
  g_holoLensEnabled132=false;ResetScopeEye131();controls.leftTrigger=0;fc3pad::Publish(controls);
  RenderScopes60(images[0],images[1]);Check(g_holoRenderedFrame131==g_frame&&!g_scopeRendered60,"real hologram dot renderer runs without a magnified scene copy");
  for(int eye=0;eye<2;++eye){auto pixels=read(eye);UINT changed=0;for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){const UINT at=(y*width+x)*4;if(memcmp(pixels.data()+at,original[eye].data()+at,4)){++changed;const ImVec2 point={2.f*(x+.5f)/width-1,1-2.f*(y+.5f)/height};Check(OpticContains131(lens[eye],point),"actual dot pixels are clipped to native glass polygon");const double dx=x+.5-(dot[eye].x+1)*.5*width,dy=y+.5-(1-dot[eye].y)*.5*height;Check(std::hypot(dx,dy)<HoloRadius131(eye,width,height)+1,"world pixels change only inside the small aiming dot");}}
   Check(eye==alignedEye?(changed>0&&changed<100):(changed==0),"only eye physically aligned through the glass sees the dot; other world image untouched");
  }
  ID3D11Buffer* restored=nullptr;context->PSGetConstantBuffers(0,1,&restored);Check(restored==oldPsBuffer,"holo restores native pixel constant binding");SafeRelease(restored);
  // Lateral head motion shifts the glass projection, not the infinite ray.
  const ImVec2 before=dot[0];const auto old=g_eyeViewsBody[0];g_eyeViewsBody[0].view[3]+=.01f;ImVec2 after={};ScopeOptics60 moved={};Check(HoloDot131(0,after)&&HoloPacket131(0,moved)&&fabs(after.x-before.x)<1e-7&&fabs(moved.center.x-lens[0].center.x)>.005,"head translation moves window around dot instead of pinning it to lens centre");g_eyeViewsBody[0]=old;
 }
 // A physically scoped source cannot be inherited from a previous weapon/epoch.
 g_scopeEyes60[1].epoch++;ScopeOptics60 rejected={};Check(!HoloPacket131(1,rejected),"mismatched opposite-eye lens rejected");g_scopeEyes60[1].epoch--;
 g_reticleAimFrame73=g_frame-3;ImVec2 rejectedDot={};Check(!OpticShotDot131(0,rejectedDot),"old shot ray cannot create a frozen hologram");PublishReticleAim73({0,0,1});
 controls.leftTrigger=1;controls.leftGrip=1;fc3pad::Publish(controls);g_twoHand=true;Check(!ScopeStabilizationActive72(),"holographic LT does not switch to telescope aim filter or enlargement");
 packet(3,.032);Check(OpticAligned132(),"magnifying scope scene assist enters with two-handed LT and actual lens alignment");
 // Regression: a scope away from the eye must activate with normal held LT.
 packet(3,.5);Check(!OpticAligned132()&&OpticZoomHeld133(),"held normal zoom no longer requires face/lens alignment");
 g_scopeSceneEnabled132=true;g_holoSceneEnabled132=true;g_scopeSceneZoom132=2;g_holoSceneZoom132=3;g_opticScene132={};UpdateOpticScene132();g_opticScene132.tick-=40;UpdateOpticScene132();Check(g_opticScene132.scale>1&&g_opticScene132.scale<2,"scope scene strength is independent of reflex strength");
 g_scopeSceneEnabled132=false;UpdateOpticScene132();Check(g_opticScene132.scale==1,"scope scene switch fully bypasses despite enabled reflex assist");g_holoSceneEnabled132=false;
 g_scopeLensEnabled132=false;Check(ScopeZoom63()==1,"scope lens toggle gives 1x without removing physical glass");g_scopeLensEnabled132=true;packet(6,.032);
 controls.leftTrigger=controls.leftGrip=0;fc3pad::Publish(controls);g_twoHand=false;g_weaponSteady129=0;g_holoSteady131=.75f;V3 direction={0,0,1};ResetScopeStabilization72();ScopeAimCommand72(direction);g_scopeAim72.ms-=16;direction=Unit({.008,0,1});const double jitter=direction.x;ScopeAimCommand72(direction);Check(g_scopeAim72.active&&direction.x<jitter*.4,"dedicated sight steadiness damps idle aim even with generic weapon smoothing at zero");g_weaponSteady129=.45f;
 g_aimActive=false;ResetScopeStabilization72();ClearScopeResources60();context->ClearState();SafeRelease(oldPsBuffer);for(auto*& image:images)SafeRelease(image);
 puts("PASS: holo131 two native lens shapes, asymmetric per-eye projection, actual GPU dot-only changes, exact glass clipping, left/right alignment, head parallax, one-handed no-LT, zero gun warp, state restoration and separate sight steadiness");
}

void TestPhysicalScope131(ID3D11Device* device,ID3D11DeviceContext* context){
 g_redDot=false;
 TestScopeProjection62(device,context);ClearScopeResources60();g_dev=device;g_ctx=context;g_oCopyResource=ProbeCopy;g_oUpdateSubresource=Update;g_aimActive=false;g_frameCameraValid=false;
 g_scopeEnabled60=g_enabled=g_inGameplay=g_motionGun=g_eyeViewsSet=g_handRightValid=g_handLeftValid=g_twoHand=true;g_weaponMotionGuard=g_flatViewActive=false;g_pistolMenu.open=false;g_pistolReload={};g_pistolReload.magazinePresent=true;g_r700={};
 fc3pad::SetGameplayControls(true);fc3pad::Controllers input={};input.active=true;input.leftGrip=input.leftTrigger=1;fc3pad::Publish(input);++g_frame;
 constexpr UINT size=256;D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=size;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 ID3D11Texture2D* images[2]={};std::vector<BYTE> original[2],source[2];for(int eye=0;eye<2;++eye){original[eye].resize(size*size*4);source[eye].resize(size*size*4);for(UINT i=0;i<size*size;++i){original[eye][i*4]=eye?30:10;original[eye][i*4+1]=20;original[eye][i*4+2]=eye?10:30;original[eye][i*4+3]=255;source[eye][i*4]=eye?20:200;source[eye][i*4+1]=eye?200:80;source[eye][i*4+2]=eye?80:20;source[eye][i*4+3]=255;}D3D11_SUBRESOURCE_DATA data={original[eye].data(),size*4,0};Check(SUCCEEDED(device->CreateTexture2D(&td,&data,&images[eye])),"physical scope eye image");}
 auto pixels=[&](int eye){auto desc=td;desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ID3D11Texture2D* staging=nullptr;Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)),"physical scope staging");context->CopyResource(staging,images[eye]);D3D11_MAPPED_SUBRESOURCE m={};Check(SUCCEEDED(context->Map(staging,0,D3D11_MAP_READ,0,&m)),"physical scope pixels");std::vector<BYTE> out(size*size*4);for(UINT y=0;y<size;++y)memcpy(out.data()+y*size*4,static_cast<BYTE*>(m.pData)+y*m.RowPitch,size*4);context->Unmap(staging,0);SafeRelease(staging);return out;};
 for(int spec=0;spec<5;++spec)for(int selected:{0,1}){
  g_scopeEye131=selected?1:2;ResetScopeEye131();g_scopeZoom60=spec+1;g_weaponMechanicProfile=kScopeSpecs60[spec].profile<0?31:kScopeSpecs60[spec].profile;g_weaponMechanicFrame=g_frame;
  const auto& descriptor=kScopeSpecs60[spec];Check(descriptor.outline!=nullptr,"every recognized magnifier has an extracted mesh rim, not a guessed circle");
  Mat4 translated=HandIdentity();translated.m[3]=-descriptor.center.x;translated.m[7]=.18-descriptor.center.y;translated.m[11]=-descriptor.center.z;
  const auto rotation=TurnAndMove({0,1,0},Unit({.07,1,.04}),{},{});const auto world=Multiply(rotation,translated);
  for(int eye=0;eye<2;++eye){auto& p=g_scopeEyes60[eye];p={};p.spec=spec;p.ownerProfile=g_weaponMechanicProfile;p.epoch=g_armPoseEpoch;p.frame=g_frame;p.draw.valid=true;p.draw.frame=g_frame;p.draw.anchor.world=world;p.draw.camera[0]=p.draw.camera[6]=p.draw.camera[9]=p.draw.camera[13]=1;for(int i=0;i<16;++i)p.baseEye[i]=p.draw.eye[i]=float(i%5==0);
   context->UpdateSubresource(images[eye],0,nullptr,original[eye].data(),size*4,0);auto* clean=TwinResource(images[eye],true,kScopeClean61);Check(clean!=nullptr,"physical telescope isolated per-eye optical source");context->UpdateSubresource(clean,0,nullptr,source[eye].data(),size*4,0);SafeRelease(clean);const ScopeCleanStamp61 stamp={g_frame,g_armPoseEpoch};images[eye]->SetPrivateData(kScopeCleanStamp61,sizeof(stamp),&stamp);
   float rows[dxbc::kEyeRows*4]={};for(int variant:{1,3,4}){ScopePresentation62(rows,variant,eye);for(float value:rows)Check(value==0,"magnifying scope cannot scale or translate gun/arms in either eye");}
  }
  g_scopeCleanFrame61=g_frame;g_scopeCleanEpoch61=g_armPoseEpoch;g_scopeCleanDraws61=1;g_scopeCleanFailed61=false;const auto identity=HandIdentity();UpdateScopeProjection62(identity.m,0);UpdateScopeProjection62(identity.m,1);
  RenderScopes60(images[0],images[1]);Check(g_scopeRendered60&&g_scopeEyeState131.selected==selected,"only selected scope eye receives magnification");
  const auto lens=ScopeOpticsForEye60(g_scopeEyes60[selected],ScopeZoom63(),true);UINT changed=0,coloured=0;
  for(int eye=0;eye<2;++eye){const auto result=pixels(eye);if(eye!=selected){Check(result==original[eye],"non-scope eye preserves every original scene pixel, avoiding shared zoom/convergence");continue;}
   for(UINT y=0;y<size;++y)for(UINT x=0;x<size;++x){const UINT at=(y*size+x)*4;if(!memcmp(result.data()+at,original[eye].data()+at,4))continue;++changed;
    const ImVec2 point={2.f*(x+.5f)/size-1,1-2.f*(y+.5f)/size};if(!OpticContains131(lens,point)){
     double closest=1e9;for(int i=0;i<64;++i){const auto a=lens.edge[i],b=lens.edge[(i+1)%64];const double dx=b.x-a.x,dy=b.y-a.y,t=(std::max)(0.,(std::min)(1.,((point.x-a.x)*dx+(point.y-a.y)*dy)/(dx*dx+dy*dy+1e-20)));closest=(std::min)(closest,std::hypot(point.x-a.x-t*dx,point.y-a.y-t*dy)*size*.5);}
     Check(closest<1.5,"zoom and reticle never escape the actual mesh aperture beyond antialias fringe");
    }
    if(eye?result[at+1]>150:result[at]>150)++coloured;
   }
  }
  Check(changed>20&&coloured>10,"native-size lens contains its own eye optical pixels, not the opposite-eye image");
 }
 g_scopeEye131=0;ResetScopeEye131();ScopeOptics60 pair[2]={};ImVec2 dots[2]={};pair[0].center={0,0};pair[1].center={.3f,0};pair[0].edge[0]={.1f,0};pair[1].edge[0]={.4f,0};Check(SelectScopeEye131(pair,dots)==0,"automatic eye chooses alignment");pair[0].center={.3f,0};pair[1].center={0,0};pair[0].edge[0]={.4f,0};pair[1].edge[0]={.1f,0};Check(SelectScopeEye131(pair,dots)==0,"head sway cannot swap scope eyes mid-aim");input.leftTrigger=0;fc3pad::Publish(input);RenderScopes60(images[0],images[1]);Check(g_scopeEyeState131.selected==-1,"release unlocks scope eye");Check(SelectScopeEye131(pair,dots)==1,"next aim can select better aligned eye");
 ClearScopeResources60();context->ClearState();for(auto*& image:images)SafeRelease(image);g_scopeEye131=0;
 puts("PASS: physical scopes131 all5 extracted rim geometries x left/right eye, rotated native-size windows, every other-eye pixel unchanged, no gun/arm scale, own-eye optical sources, clipped reticle and eye lock/release");
}
