// Draw into the completed eye images BEFORE Frame() submits them, using the
// aim projection saved with their scene constants, not next-frame tracking.
ImVec2 ReticleRadii(const fc3stereo::EyeView& view,float width,float height,float radius)
{
 const float sx=view.tanRight-view.tanLeft,sy=view.tanUp-view.tanDown;
 if(!(width>0&&height>0&&sx>0&&sy>0)||!std::isfinite(sx)||!std::isfinite(sy))return ImVec2(radius,radius);
 const float x=radius*(width/height)*(sy/sx);
 if(!std::isfinite(x)||x<=0)return ImVec2(radius,radius);
 return ImVec2(x,radius);
}
void ReticleEllipse(ImDrawList* draw,ImVec2 point,ImVec2 radius,ImU32 colour)
{
 for(int i=0;i<32;++i){const float a=i*(6.28318530718f/32);draw->PathLineTo(ImVec2(point.x+std::cos(a)*radius.x,point.y+std::sin(a)*radius.y));}
 draw->PathFillConvex(colour);
}
ImGuiContext* g_reticleContext=nullptr;ID3D11Device* g_reticleDevice=nullptr;
#ifdef FC3_AMMO_TEST
const fc3aim::LaserSample194* g_laserFixture194=nullptr;
#endif
bool LaserProjection194(int eye,const Mat4& gameView,const float* world,ImVec2& ndc)
{
 if(eye<0||eye>1||!world)return false;
 const V3 gameEye=Apply(gameView,{world[0],world[1],world[2]});Mat4 view={};
 for(int i=0;i<16;++i)view.m[i]=g_eyeViews[eye].view[i];
 const V3 p=Apply(view,gameEye);if(!std::isfinite(p.z)||p.z<=.025)return false;
 const auto& e=g_eyeViews[eye];const double sx=e.tanRight-e.tanLeft,sy=e.tanUp-e.tanDown;
 if(!(sx>0&&sy>0))return false;
 ndc={float((2*p.x/p.z-e.tanRight-e.tanLeft)/sx),float((2*p.y/p.z-e.tanUp-e.tanDown)/sy)};
 return std::isfinite(ndc.x)&&std::isfinite(ndc.y);
}
void RenderRedDot(ID3D11Texture2D* left,ID3D11Texture2D* right)
{
 // Laser geometry is rendered separately against the captured weapon depth.
 if(g_laserMode194)return;
 static ULONGLONG nextLaserLog203=0;static unsigned laserLogs203=0;
 if(ThrowableOwns144()||(g_centerDotFrame135==g_frame&&g_laserMode194==0)||(!g_redDot&&g_laserMode194==0)||!g_enabled||!g_inGameplay||g_flatViewActive||g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::WheelOwnsGrip()||
    !g_eyeViewsSet||g_visibleWeaponDrawsLastFrame<=0||!left||!right||!g_dev||!g_ctx){
  if(g_laserMode194&&g_inGameplay&&g_eyeViewsSet&&laserLogs203<16&&GetTickCount64()>=nextLaserLog203){nextLaserLog203=GetTickCount64()+500;++laserLogs203;
   Log("[LASER203] overlay gate closed: gameplay%d flat%d menu%d eyes%d weaponDraws%ld targets%d/%d",g_inGameplay?1:0,g_flatViewActive?1:0,(g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::WheelOwnsGrip())?1:0,g_eyeViewsSet?1:0,g_visibleWeaponDrawsLastFrame,left?1:0,right?1:0);}
  return;
 }
 fc3aim::LaserSample194 laser194={};
 bool haveLaser194=fc3aim::ReadLaser194(laser194);
#ifdef FC3_AMMO_TEST
 if(g_laserFixture194){laser194=*g_laserFixture194;haveLaser194=true;}
#endif
 const bool laserReady194=g_laserMode194!=0&&g_aimActive&&haveLaser194&&
  g_muzzleLaserFrame203.valid&&g_muzzleLaserFrame203.frame==g_frame&&g_muzzleLaserFrame203.epoch==g_armPoseEpoch&&
  laser194.profile==g_muzzleLaserFrame203.profile&&laser194.epoch==g_armPoseEpoch;
 float currentMuzzle194[3]={};
 if(laserReady194){const V3 current=Apply(g_muzzleLaserFrame203.move,{laser194.nativeMuzzle[0],laser194.nativeMuzzle[1],laser194.nativeMuzzle[2]});
  currentMuzzle194[0]=float(current.x);currentMuzzle194[1]=float(current.y);currentMuzzle194[2]=float(current.z);}
 const bool own=g_own;g_own=true;ImGuiContext* previous=ImGui::GetCurrentContext();
 if(g_reticleContext&&g_reticleDevice!=g_dev){ImGui::SetCurrentContext(g_reticleContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(g_reticleContext);g_reticleContext=nullptr;}
 if(!g_reticleContext){
  g_reticleContext=ImGui::CreateContext();ImGui::SetCurrentContext(g_reticleContext);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
  if(!ImGui_ImplDX11_Init(g_dev,g_ctx)){ImGui::DestroyContext(g_reticleContext);g_reticleContext=nullptr;ImGui::SetCurrentContext(previous);g_own=own;return;}
  g_reticleDevice=g_dev;
 }
 ImGui::SetCurrentContext(g_reticleContext);
 ID3D11RenderTargetView* saved[8]={};ID3D11DepthStencilView* depth=nullptr;
 g_ctx->OMGetRenderTargets(8,saved,&depth);
 ID3D11Texture2D* eyes[2]={left,right};
 for(int eye=0;eye<2;++eye){
  const float x=g_redDotNdc[eye][0],y=g_redDotNdc[eye][1];
  const bool hud194=g_laserMode194==0&&g_redDot&&g_redDotValid[eye]&&std::isfinite(x)&&std::isfinite(y)&&fabs(x)<=1&&fabs(y)<=1;
  ImVec2 laserStart194={},laserEnd194={};const bool beam194=laserReady194&&LaserProjection194(eye,g_muzzleLaserFrame203.view,currentMuzzle194,laserStart194)&&LaserProjection194(eye,g_muzzleLaserFrame203.view,laser194.point,laserEnd194);
  if(eye==0&&g_laserMode194&&laserLogs203<16&&GetTickCount64()>=nextLaserLog203){nextLaserLog203=GetTickCount64()+500;++laserLogs203;
   Log("[LASER203] mode%d sample%d ready%d beam%d frame%ld confirmed%ld epoch%u/%u profile%u/%u start %.2f/%.2f end %.2f/%.2f",g_laserMode194,haveLaser194?1:0,laserReady194?1:0,beam194?1:0,g_frame,g_muzzleLaserFrame203.frame,g_armPoseEpoch,g_muzzleLaserFrame203.epoch,laser194.profile,g_muzzleLaserFrame203.profile,laserStart194.x,laserStart194.y,laserEnd194.x,laserEnd194.y);}
  if(!hud194&&!beam194)continue;
  D3D11_TEXTURE2D_DESC td={};eyes[eye]->GetDesc(&td);ID3D11RenderTargetView* rtv=nullptr;
  if(FAILED(g_dev->CreateRenderTargetView(eyes[eye],nullptr,&rtv)))continue;
  auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(static_cast<float>(td.Width),static_cast<float>(td.Height));io.DeltaTime=1.f/60;
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
  const ImVec2 point((x+1)*.5f*td.Width,(1-y)*.5f*td.Height);
  const float radius=(std::max)(2.5f,td.Height*.0023f);
  auto* draw=ImGui::GetForegroundDrawList();
  if(beam194){
   const auto px194=[&](ImVec2 p){return ImVec2((p.x+1)*.5f*td.Width,(1-p.y)*.5f*td.Height);};
   const ImVec2 start=px194(laserStart194),end=px194(laserEnd194);
   if(g_laserMode194==2||g_laserMode194==3){draw->AddLine(start,end,IM_COL32(0,0,0,200),3.5f);draw->AddLine(start,end,ReticleColour52(),1.7f);}
   if(laser194.hit&&(g_laserMode194==1||g_laserMode194==3)){
    ReticleEllipse(draw,end,ReticleRadii(g_eyeViews[eye],float(td.Width),float(td.Height),radius+2),IM_COL32(0,0,0,240));
    ReticleEllipse(draw,end,ReticleRadii(g_eyeViews[eye],float(td.Width),float(td.Height),radius),ReticleColour52());
   }
  }
  if(hud194){ReticleEllipse(draw,point,ReticleRadii(g_eyeViews[eye],float(td.Width),float(td.Height),radius+2),IM_COL32(0,0,0,240));
   ReticleEllipse(draw,point,ReticleRadii(g_eyeViews[eye],float(td.Width),float(td.Height),radius),ReticleColour52());}
  ImGui::Render();g_ctx->OMSetRenderTargets(1,&rtv,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());rtv->Release();
 }
 g_ctx->OMSetRenderTargets(8,saved,depth);for(auto* v:saved)if(v)v->Release();if(depth)depth->Release();
 ImGui::SetCurrentContext(previous);g_own=own;
}
