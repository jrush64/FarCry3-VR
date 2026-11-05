// Physical telescope: native gun/lens placement, one eye, no binocular image fusion.
struct ScopeEyeState131{int selected=-1,spec=-1,setting=-1;LONG profile=-1;UINT epoch=0;}g_scopeEyeState131;
void ResetScopeEye131(){g_scopeEyeState131={};}
bool OpticContains131(const ScopeOptics60& lens,ImVec2 p){
 bool inside=false;for(int i=0,j=63;i<64;j=i++){
  const auto a=lens.edge[i],b=lens.edge[j];if((a.y>p.y)!=(b.y>p.y)&&p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x)inside=!inside;
 }return inside;
}
int SelectScopeEye131(const ScopeOptics60* lens,const ImVec2* dot){
 auto& s=g_scopeEyeState131;const int spec=g_scopeEyes60[0].spec;
 if(s.selected>=0&&s.spec==spec&&s.profile==g_weaponMechanicProfile&&s.epoch==g_armPoseEpoch&&s.setting==g_scopeEye131)return s.selected;
 int eye=g_scopeEye131==1?1:0;
 if(g_scopeEye131==0){double score[2]={};for(int e=0;e<2;++e){const double radius=std::hypot(double(lens[e].edge[0].x-lens[e].center.x),double(lens[e].edge[0].y-lens[e].center.y));score[e]=std::hypot(double(dot[e].x-lens[e].center.x),double(dot[e].y-lens[e].center.y))/(std::max)(.0001,radius);}eye=score[1]<=score[0]?1:0;}
 s={eye,spec,g_scopeEye131,g_weaponMechanicProfile,g_armPoseEpoch};return eye;
}
void ScopeReticleLine131(ImDrawList* draw,const ScopeOptics60& lens,ImVec2 a,ImVec2 b,float width,float height,ImU32 colour){
 std::vector<double> cuts={0,1};const double dx=b.x-a.x,dy=b.y-a.y;
 for(int i=0;i<64;++i){const auto c=lens.edge[i],d=lens.edge[(i+1)%64];const double ex=d.x-c.x,ey=d.y-c.y,den=dx*ey-dy*ex;
  if(fabs(den)<1e-12)continue;const double t=((c.x-a.x)*ey-(c.y-a.y)*ex)/den,u=((c.x-a.x)*dy-(c.y-a.y)*dx)/den;
  if(t>0&&t<1&&u>=0&&u<=1)cuts.push_back(t);
 }
 std::sort(cuts.begin(),cuts.end());auto pixel=[&](double t){return ImVec2(float((a.x+dx*t+1)*.5*width),float((1-a.y-dy*t)*.5*height));};
 for(size_t i=1;i<cuts.size();++i){const double mid=(cuts[i-1]+cuts[i])*.5;if(OpticContains131(lens,{float(a.x+dx*mid),float(a.y+dy*mid)}))draw->AddLine(pixel(cuts[i-1]),pixel(cuts[i]),colour,1.25f);}
}
void DrawPhysicalScope131(ImDrawList* draw,ImTextureID texture,const ScopeOptics60& lens,ImVec2 dot,float width,float height,bool fallback=false){
 auto pixel=[&](ImVec2 p){return ImVec2((p.x+1)*.5f*width,(1-p.y)*.5f*height);};
 auto uv=[&](ImVec2 p){if(fallback){const float zoom=ScopeZoom63();return ImVec2(.5f+.5f*(lens.center.x+(p.x-lens.center.x)/zoom),.5f-.5f*(lens.center.y+(p.y-lens.center.y)/zoom));}return ImVec2(.5f+.5f*(p.x-dot.x),.5f-.5f*(p.y-dot.y));};
 draw->PushTextureID(texture);draw->PrimReserve(64*3,64*3);
 for(int i=0;i<64;++i){const int j=(i+1)%64;const auto base=ImDrawIdx(draw->_VtxCurrentIdx);draw->PrimWriteIdx(base);draw->PrimWriteIdx(base+1);draw->PrimWriteIdx(base+2);draw->PrimWriteVtx(pixel(lens.center),uv(lens.center),IM_COL32_WHITE);draw->PrimWriteVtx(pixel(lens.edge[i]),uv(lens.edge[i]),IM_COL32_WHITE);draw->PrimWriteVtx(pixel(lens.edge[j]),uv(lens.edge[j]),IM_COL32_WHITE);}
 draw->PopTextureID();draw->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState,nullptr);
 if(HoloProfile131())return;
 if(g_centerDot135&&g_redDot)dot=lens.center;
 const double radius=std::hypot(double(lens.edge[0].x-lens.center.x),double(lens.edge[0].y-lens.center.y))*.8;
 ScopeReticleLine131(draw,lens,{float(dot.x-radius),dot.y},{float(dot.x+radius),dot.y},width,height,IM_COL32(0,0,0,240));
 ScopeReticleLine131(draw,lens,{dot.x,float(dot.y-radius)},{dot.x,float(dot.y+radius)},width,height,IM_COL32(0,0,0,240));
}
void RenderPhysicalScope131(ID3D11Texture2D* left,ID3D11Texture2D* right){
 ScopeOptics60 lens[2];ImVec2 dot[2];
 for(int eye=0;eye<2;++eye){if(g_scopeProjectionFrame62[eye]!=g_frame)return;
  if(HoloProfile131()){if(!HoloPacket131(eye,lens[eye]))return;}else lens[eye]=ScopeOpticsForEye60(g_scopeEyes60[eye],ScopeZoom63(),true);if(!lens[eye].valid)return;
  if(!(HoloProfile131()?HoloDot131(eye,dot[eye]):OpticShotDot131(eye,dot[eye])))dot[eye]=lens[eye].axis;
 }
 const int eye=SelectScopeEye131(lens,dot);ID3D11Texture2D* output=eye?right:left;
 auto* clean=ScopeCleanImage61(output);const bool fallback=clean==nullptr;
 // Never reuse stale/failed optical replay. Magnify the current own-eye glass
 // interior if replay failed; contraction around the convex native aperture
 // keeps source samples inside the lens instead of enlarging gun/housing.
 const bool copied=ScopeImage60(eye,clean?clean:output);SafeRelease(clean);if(!copied||!HoloResources131())return;
 if(!g_scopeOpaquePs60){auto* code=Native1911Compile48("Texture2D image:register(t0);SamplerState samp:register(s0);float4 main(float4 position:SV_Position,float4 colour:COLOR0,float2 uv:TEXCOORD0):SV_Target{return float4(image.Sample(samp,uv).rgb*colour.rgb,colour.a);}","ps_5_0");const bool ok=code&&SUCCEEDED(g_dev->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&g_scopeOpaquePs60));SafeRelease(code);if(!ok)return;}
 D3D11_TEXTURE2D_DESC td={};output->GetDesc(&td);ID3D11RenderTargetView* target=nullptr;if(FAILED(g_dev->CreateRenderTargetView(output,nullptr,&target)))return;
 const bool own=g_own;g_own=true;ImGuiContext* prior=ImGui::GetCurrentContext();ImGui::SetCurrentContext(g_scopeContext60);
 ID3D11RenderTargetView* saved[8]={};ID3D11DepthStencilView* depth=nullptr;ID3D11Buffer* ps0=nullptr;g_ctx->OMGetRenderTargets(8,saved,&depth);g_ctx->PSGetConstantBuffers(0,1,&ps0);
 auto& io=ImGui::GetIO();io.DisplaySize={float(td.Width),float(td.Height)};io.DeltaTime=1.f/60;ImGui_ImplDX11_NewFrame();ImGui::NewFrame();auto* draw=ImGui::GetForegroundDrawList();
 draw->AddCallback(ScopeOpaqueCallback60,nullptr);DrawPhysicalScope131(draw,(ImTextureID)g_scopeSrv60[eye],lens[eye],dot[eye],float(td.Width),float(td.Height),fallback);
 HoloDraw131 centreData={};const bool centre=g_redDot&&g_centerDot135;if(centre)AppendCentreDot135(draw,lens[eye],float(td.Width),float(td.Height),centreData);
 ImGui::Render();g_ctx->OMSetRenderTargets(1,&target,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
 g_ctx->OMSetRenderTargets(8,saved,depth);g_ctx->PSSetConstantBuffers(0,1,&ps0);SafeRelease(ps0);for(auto*& p:saved)SafeRelease(p);SafeRelease(depth);SafeRelease(target);ImGui::SetCurrentContext(prior);g_own=own;
 if(centre)g_centerDotFrame135=g_frame;
 g_scopeRendered60=true;if(!HoloProfile131()&&!ScopePreviewActive65()){g_scopeCarryProfile65=g_weaponMechanicProfile;g_scopeCarryEpoch65=g_armPoseEpoch;}
 static LONG failureLog=-1000;if(fallback&&g_frame-failureLog>=120){failureLog=g_frame;Log("[SCOPE135] current-eye lens magnification fallback profile%ld frame%ld zoom%.2f cleanFailed%d; no stale replay",g_weaponMechanicProfile,g_frame,ScopeZoom63(),g_scopeCleanFailed61);}
 static LONG lastProfile=-1;static int lastEye=-1;if(lastProfile!=g_weaponMechanicProfile||lastEye!=eye){lastProfile=g_weaponMechanicProfile;lastEye=eye;Log("[SCOPE131] native mesh aperture profile%ld frame%ld zoom%.2f eye%d locked until release; no gun enlargement, no synthetic eye convergence",g_weaponMechanicProfile,g_frame,ScopeZoom63(),eye);}
}
