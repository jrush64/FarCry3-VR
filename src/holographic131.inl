// Physical reflex optics: leave each completed eye's world pixels in place.
// Only a collimated, per-eye dot is rasterized inside the native lens polygon.
bool HoloProfile131(){
 const auto& p=g_scopeEyes60[0];
 return p.spec>=0&&p.spec<int(sizeof(kScopeSpecs60)/sizeof(kScopeSpecs60[0]))&&kScopeSpecs60[p.spec].reflex&&
  p.ownerProfile==g_weaponMechanicProfile&&p.epoch==g_armPoseEpoch&&p.frame<=g_frame&&g_frame-p.frame<=2;
}
// A temporary native camera binding loss must not blank an otherwise current
// controller-owned sight. Rebuild the visual ray from the SAME fixed gun root;
// never retain an old weapon ray or relax reload/action ownership.
bool HoloRay132(V3& ray){
 if(!g_aimActive||!g_handRightValid||!MotionInteractionEnabled())return false;
 if(BarrelBody206(ray))return true; // 206: the measured barrel ray
 if(g_shotAimProfile129==g_weaponMechanicProfile&&g_reticleAimEpoch73==g_armPoseEpoch&&std::abs(g_frame-g_reticleAimFrame73)<=1&&Length(g_reticleAimBody73)>.5){ray=g_reticleAimBody73;return true;}
 Mat4 root={},grip={};if(!ControllerCurrentRoot124(root,grip))return false;ray=Unit({root.m[1],root.m[5],root.m[9]});
 if(ScopeAimValid72())ray=g_scopeAim72.filtered;
 return std::isfinite(ray.x)&&std::isfinite(ray.y)&&std::isfinite(ray.z)&&Length(ray)>.5;
}
bool HoloAllowed131(){
 return HoloProfile131()&&g_holoEnabled131&&g_enabled&&g_inGameplay&&MotionInteractionEnabled()&&g_eyeViewsSet&&g_handRightValid&&
  fc3pad::Latest().active&&!g_flatViewActive&&!g_pistolMenu.open&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked()&&
  !fc3pad::WeaponSelectionRequested()&&!g_pistolReload.pending&&!g_pistolReload.removalHeld&&!g_reload.held&&g_aimActive;
}
bool HoloPacket131(int eye,ScopeOptics60& optics){
 if(eye<0||eye>1||!HoloAllowed131())return false;
 const auto& p=g_scopeEyes60[eye];
 if(p.spec!=g_scopeEyes60[0].spec||p.ownerProfile!=g_weaponMechanicProfile||p.epoch!=g_armPoseEpoch||p.frame>g_frame||g_frame-p.frame>1)return false;
 if(ScopeBodyOptics72(p,eye,optics))return true;
 optics=ScopeOpticsForEye60(ScopeBase62(p),1,true);return optics.valid;
}
bool OpticShotDot131(int eye,ImVec2& dot){
 if(eye<0||eye>1||!g_aimActive)return false;
 // At finite range the bullet starts at the muzzle, not at the headset.
 // Project the same muzzle-origin target as the regular crosshair when it is
 // available; the collimated direction below remains the fallback.
 V3 target212={};if(ReticleMuzzleTarget212(eye,target212)){
  const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
  dot={float(projection.m[0]*target212.x/target212.z+projection.m[2]),float(projection.m[5]*target212.y/target212.z+projection.m[6])};
  return std::isfinite(dot.x)&&std::isfinite(dot.y);
 }
 V3 ray={};if(!BarrelBody206(ray)){if(g_reticleAimEpoch73!=g_armPoseEpoch||std::abs(g_frame-g_reticleAimFrame73)>1||g_shotAimProfile129!=g_weaponMechanicProfile)return false;ray=g_reticleAimBody73;}
 const auto v=ScopeBodyView72(eye);
 // w=0: collimation at infinity. Eye translation moves the glass window,
 // not the distant aiming direction. Each XR projection retains its offsets.
 const V3 p={v.m[0]*ray.x+v.m[1]*ray.y+v.m[2]*ray.z,v.m[4]*ray.x+v.m[5]*ray.y+v.m[6]*ray.z,v.m[8]*ray.x+v.m[9]*ray.y+v.m[10]*ray.z};
 if(!std::isfinite(p.z)||p.z<=.025)return false;
 const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
 dot={float(projection.m[0]*p.x/p.z+projection.m[2]),float(projection.m[5]*p.y/p.z+projection.m[6])};
 return std::isfinite(dot.x)&&std::isfinite(dot.y);
}
bool HoloDot131(int eye,ImVec2& dot){
 if(eye<0||eye>1||!HoloAllowed131())return false;
 if(OpticShotDot131(eye,dot))return true;
 V3 ray={};if(!HoloRay132(ray))return false;
 const auto v=ScopeBodyView72(eye);const V3 p={v.m[0]*ray.x+v.m[1]*ray.y+v.m[2]*ray.z,v.m[4]*ray.x+v.m[5]*ray.y+v.m[6]*ray.z,v.m[8]*ray.x+v.m[9]*ray.y+v.m[10]*ray.z};
 if(!std::isfinite(p.z)||p.z<=.025)return false;const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
 dot={float(projection.m[0]*p.x/p.z+projection.m[2]),float(projection.m[5]*p.y/p.z+projection.m[6])};return std::isfinite(dot.x)&&std::isfinite(dot.y);
}
float HoloRadius131(int eye,float width,float height){
 const auto p=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);
 const double radians=double(g_holoDotMoa131)*3.141592653589793/(180.*60.*2.);
 return float((std::max)(1.25,(std::min)(8.,.5*(std::min)(width*fabs(p.m[0]),height*fabs(p.m[5]))*tan(radians))));
}
bool HoloNativeGlass131(ID3D11DeviceContext* context,const DrawCall& call,int variant){
 if(call.kind||call.d||(variant!=1&&variant!=4)||!HoloAllowed131())return false;
 const int spec=ScopeGeometry60(NativeWeaponGeometry(context,call));
 if(spec<0||!kScopeSpecs60[spec].reflex||spec!=g_scopeEyes60[0].spec)return false;
 ScopeOptics60 p[2];ImVec2 dots[2];return HoloPacket131(0,p[0])&&HoloPacket131(1,p[1])&&HoloDot131(0,dots[0])&&HoloDot131(1,dots[1]);
}
bool HoloResources131(){
 const bool previousOwn131=g_own;g_own=true;struct RestoreOwn131{bool value;~RestoreOwn131(){g_own=value;}} restore{previousOwn131};
 if(g_scopeDevice60&&g_scopeDevice60!=g_dev){ClearScopeResources60();return false;}
 if(!g_scopeContext60){
  ImGuiContext* prior=ImGui::GetCurrentContext();g_scopeContext60=ImGui::CreateContext();ImGui::SetCurrentContext(g_scopeContext60);
  auto& io=ImGui::GetIO();io.IniFilename=io.LogFilename=nullptr;
  const bool ok=ImGui_ImplDX11_Init(g_dev,g_ctx);ImGui::SetCurrentContext(prior);
  if(!ok){ImGui::DestroyContext(g_scopeContext60);g_scopeContext60=nullptr;return false;}g_scopeDevice60=g_dev;
 }
 if(!g_holoPs131){auto* code=Native1911Compile48("cbuffer Dot:register(b0){float2 centre;float radius;float pad;};float4 main(float4 pos:SV_Position,float4 colour:COLOR0,float2 uv:TEXCOORD0):SV_Target{float a=saturate(radius+.75-length(pos.xy-centre));return float4(colour.rgb,colour.a*a);}","ps_5_0");
  const bool ok=code&&SUCCEEDED(g_dev->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&g_holoPs131));SafeRelease(code);if(!ok)return false;}
 return ScopeBuffer62(g_holoConstants131,16);
}
struct HoloDraw131{float dot[4]={};};
void HoloDotCallback131(const ImDrawList*,const ImDrawCmd* command){
 const auto* data=static_cast<const HoloDraw131*>(command->UserCallbackData);
 g_oUpdateSubresource(g_ctx,g_holoConstants131,0,nullptr,data->dot,0,0);g_ctx->PSSetConstantBuffers(0,1,&g_holoConstants131);g_ctx->PSSetShader(g_holoPs131,nullptr,0);
}
void HoloAperture131(ImDrawList* draw,const ScopeOptics60& lens,float width,float height){
 const auto pixel=[&](ImVec2 p){return ImVec2((p.x+1)*.5f*width,(1-p.y)*.5f*height);};
 const auto centre=pixel(lens.center);const ImU32 colour=ReticleColour52();draw->PushTextureID(ImGui::GetIO().Fonts->TexID);
 draw->PrimReserve(64*3,64*3);
 for(int i=0;i<64;++i){const int j=(i+1)%64;const auto base=ImDrawIdx(draw->_VtxCurrentIdx);draw->PrimWriteIdx(base);draw->PrimWriteIdx(base+1);draw->PrimWriteIdx(base+2);draw->PrimWriteVtx(centre,{},colour);draw->PrimWriteVtx(pixel(lens.edge[i]),{},colour);draw->PrimWriteVtx(pixel(lens.edge[j]),{},colour);}
 draw->PopTextureID();
}
bool HoloInside133(const ScopeOptics60& lens,ImVec2 point){bool inside=false;for(int i=0,j=63;i<64;j=i++){const auto a=lens.edge[i],b=lens.edge[j];if((a.y>point.y)!=(b.y>point.y)&&point.x<(b.x-a.x)*(point.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}return inside;}
void ScopeReticleLine131(ImDrawList*,const ScopeOptics60&,ImVec2,ImVec2,float,float,ImU32);
void HoloGuide133(ImDrawList* draw,const ScopeOptics60& lens,ImVec2 dot,float width,float height){
 if(!g_holoGuide133||HoloInside133(lens,dot))return;
 // A chevron is an acquisition cue, never a fake centred impact dot. Its
 // direction indicates the real angular aim outside the physical window.
 const double dx=(dot.x-lens.center.x)*width,dy=(dot.y-lens.center.y)*height,length=std::hypot(dx,dy);if(length<1e-5)return;
 double radius=1e9;for(const auto& p:lens.edge)radius=(std::min)(radius,std::hypot(double(p.x-lens.center.x)*width,double(p.y-lens.center.y)*height));
 const double size=(std::max)(1.,(std::min)(8.,radius*.32));const ImVec2 tip={float(lens.center.x+dx/length*size/width),float(lens.center.y+dy/length*size/height)};
 const ImVec2 a={float(tip.x-2*dx/length*size/width+dy/length*size/width),float(tip.y-2*dy/length*size/height-dx/length*size/height)};
 const ImVec2 b={float(tip.x-2*dx/length*size/width-dy/length*size/width),float(tip.y-2*dy/length*size/height+dx/length*size/height)};
 ScopeReticleLine131(draw,lens,a,tip,width,height,ReticleColour52());ScopeReticleLine131(draw,lens,b,tip,width,height,ReticleColour52());
}
float CentreDotRadius135(float height){return (std::max)(2.5f,height*.0023f)*g_holoDotMoa131/4.f;}
void AppendCentreDot135(ImDrawList* draw,const ScopeOptics60& lens,float width,float height,HoloDraw131& data){
 data={{(lens.center.x+1)*.5f*width,(1-lens.center.y)*.5f*height,CentreDotRadius135(height),0}};
 draw->AddCallback(HoloDotCallback131,&data);HoloAperture131(draw,lens,width,height);draw->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState,nullptr);
}
// 206: the collimated dot at the TRUE shot direction, clipped to the physical glass, like a real reflex sight.
void AppendTrueDot206(ImDrawList* draw,const ScopeOptics60& lens,ImVec2 dot,int eye,float width,float height,HoloDraw131& data){
 data={{(dot.x+1)*.5f*width,(1-dot.y)*.5f*height,HoloRadius131(eye,width,height),0}};
 draw->AddCallback(HoloDotCallback131,&data);HoloAperture131(draw,lens,width,height);draw->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState,nullptr);
}
void RenderHolographic131(ID3D11Texture2D* left,ID3D11Texture2D* right){
 ScopeOptics60 lens[2];ImVec2 dots[2]={};
 // 206: the dot is drawn whenever the sight is up (no LT needed) at the shot direction; the glass-centre marker is
 // only the legacy option ("Centre dot inside sight when zooming"), because it never showed where shots go.
 if(!g_redDot||!left||!right||!g_ctx||!g_dev||!HoloPacket131(0,lens[0])||!HoloPacket131(1,lens[1])||!HoloResources131())return;
 const bool centre206=g_centerDot135&&OpticZoomHeld133();bool inside206=false;
 if(!centre206)for(int eye=0;eye<2;++eye){if(!HoloDot131(eye,dots[eye]))return;if(HoloInside133(lens[eye],dots[eye]))inside206=true;}
 ImGuiContext* prior=ImGui::GetCurrentContext();const bool own=g_own;g_own=true;ImGui::SetCurrentContext(g_scopeContext60);
 ID3D11RenderTargetView* saved[8]={};ID3D11DepthStencilView* depth=nullptr;ID3D11Buffer* ps0=nullptr;g_ctx->OMGetRenderTargets(8,saved,&depth);g_ctx->PSGetConstantBuffers(0,1,&ps0);
 ID3D11Texture2D* images[2]={left,right};int rendered=0;
 for(int eye=0;eye<2;++eye){
  D3D11_TEXTURE2D_DESC td={};images[eye]->GetDesc(&td);ID3D11RenderTargetView* target=nullptr;if(FAILED(g_dev->CreateRenderTargetView(images[eye],nullptr,&target)))continue;
  auto& io=ImGui::GetIO();io.DisplaySize={float(td.Width),float(td.Height)};io.DeltaTime=1.f/60;
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();auto* draw=ImGui::GetForegroundDrawList();
  HoloDraw131 data={};if(centre206)AppendCentreDot135(draw,lens[eye],float(td.Width),float(td.Height),data);else AppendTrueDot206(draw,lens[eye],dots[eye],eye,float(td.Width),float(td.Height),data);
  ImGui::Render();g_ctx->OMSetRenderTargets(1,&target,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());SafeRelease(target);++rendered;
 }
 g_ctx->OMSetRenderTargets(8,saved,depth);g_ctx->PSSetConstantBuffers(0,1,&ps0);for(auto*& p:saved)SafeRelease(p);SafeRelease(depth);SafeRelease(ps0);
 ImGui::SetCurrentContext(prior);g_own=own;if(rendered==2){g_holoRenderedFrame131=g_frame;if(centre206||inside206)g_centerDotFrame135=g_frame;}
 static LONG logged=-1;if(rendered==2&&logged!=g_weaponMechanicProfile){logged=g_weaponMechanicProfile;Log("[HOLO131] physical1x window profile%ld frame%ld, per-eye collimated dot on the %s %.1fMOA; dots %.3f/%.3f and %.3f/%.3f, inside glass %d",g_weaponMechanicProfile,g_frame,centre206?"glass centre (legacy option)":"barrel ray",g_holoDotMoa131,dots[0].x,dots[0].y,dots[1].x,dots[1].y,inside206?1:0);}
}
