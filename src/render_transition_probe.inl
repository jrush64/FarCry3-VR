
// Observational only. CPU submission provenance is not proof of final pixels.
HANDLE g_renderProbe=INVALID_HANDLE_VALUE;DWORD g_renderProbeBytes=0;UINT g_renderProbeSegment=0;
bool g_vehicleProbeMarker90=false; // Numpad 1 marks a headset capture in the same passive trace.
UINT g_cameraCaptureSlot127=0;
int g_vehicleAutoCaptureReason91=0; // 1 rear, 2 draw collapse, 3 rapid stick/native-view mismatch, 4 vertical look
bool FastTurnCapture126(float stickX,float stickY,const fc3stereo::EyeView& eye,float nativeX,float nativeY)
{
 if(!std::isfinite(nativeX)||!std::isfinite(nativeY)||(std::max)(fabs(stickX),fabs(stickY))<.4||nativeX<=0||nativeY<=0)return false;
 const double yaw=fabs(atan2(double(eye.view[8]),double(eye.view[10])));
 const double pitch=fabs(atan2(double(eye.view[9]),hypot(double(eye.view[8]),double(eye.view[10]))));
 const double eyeX=(std::max)(fabs(atan(double(eye.tanLeft))),fabs(atan(double(eye.tanRight))));
 const double eyeY=(std::max)(fabs(atan(double(eye.tanUp))),fabs(atan(double(eye.tanDown))));
 const double budgetX=(std::max)(.09,atan(1.0/nativeX)-eyeX),budgetY=(std::max)(.09,atan(1.0/nativeY)-eyeY);
 return std::isfinite(yaw)&&std::isfinite(pitch)&&(yaw>budgetX+.09||pitch>budgetY+.09);
}
void TickVehicleAutoCapture91(bool headset)
{
 // A native vehicle cannot be identified yet. Capture the symptom automatically
 // in hidden-weapon gameplay and armed fast turns; no input/camera changes.
 static ULONGLONG forwardSince=0,lastCapture=0,peakAt=0;
 static LONG forwardWorldPeak=0;
 static bool turnArmed=true;
 static int captures=0;
 g_vehicleAutoCaptureReason91=0;
 if(!headset||!g_enabled||!g_headTracking||!g_eyeViewsSet||!g_inGameplay||
    !g_frameCameraValid||g_pistolMenu.open||fc3pad::GameplayInputBlocked())return;
 const ULONGLONG now=GetTickCount64();
 const float* head=g_eyeViewsBody[0].view;
 const double yaw=std::fabs(std::atan2(double(head[8]),double(head[10])));
 const double pitch257=std::fabs(std::atan2(double(head[9]),std::hypot(double(head[8]),double(head[10]))));
 if(!std::isfinite(yaw))return;
 if(yaw<0.50){
  if(!forwardSince)forwardSince=now;
  if(now-forwardSince>=800)turnArmed=true;
  if(g_mainBinCount>forwardWorldPeak||now-peakAt>10000){forwardWorldPeak=g_mainBinCount;peakAt=now;}
 }else forwardSince=0;
 const auto controls126=fc3pad::Latest();
 const bool rapid126=FastTurnCapture126(controls126.rightX,controls126.rightY,g_eyeViews[0],g_worldProjection.xs,g_worldProjection.ys);
 if(g_visibleWeaponDrawsThisFrame>0&&!rapid126)return;
 const bool collapse=forwardWorldPeak>=300&&g_mainBinCount<forwardWorldPeak/2&&yaw>0.8;
 // When the vehicle camera has a different owner, 70 degrees is already
 // outside the native vertical/horizontal cull cone; do not wait for 100.
 const bool behind=turnArmed&&yaw>(g_visibilityCamera68?1.75:1.10);
 const bool vertical257=g_vehicleCamera92&&std::isfinite(pitch257)&&pitch257>.65;
 if((rapid126||collapse||behind||vertical257)&&(!lastCapture||now-lastCapture>=10000)){
  g_vehicleAutoCaptureReason91=rapid126?3:collapse?2:behind?1:4;
  g_vehicleProbeMarker90=true;lastCapture=now;turnArmed=false;++captures;g_cameraCaptureSlot127=UINT(captures-1)%12;
  Log("[VEHICLE91] automatic eye capture %d (rolling12 pairs) reason %s frame %ld head yaw %.2f world draws %ld forward peak %ld; vehicle identity unknown",
      captures,rapid126?"rapid stick/native-view mismatch":collapse?"world-draw drop":behind?"head turned behind":"vertical vehicle look",g_frame,yaw,g_mainBinCount,forwardWorldPeak);
 }
}
//174: four retained ownership states, at most16 binding queries per sampled frame.
LONG g_renderOwnershipFrame174=-1;
UINT g_renderOwnershipQueries174=0,g_renderOwnershipOverflow174=0,g_renderOwnershipUnexamined174=0;
bool BeginRenderOwnership174(LONG frame)
{
 if(g_renderOwnershipFrame174!=frame){g_renderOwnershipFrame174=frame;g_renderOwnershipQueries174=g_renderOwnershipOverflow174=g_renderOwnershipUnexamined174=0;}
 if(g_renderOwnershipQueries174>=16){if(g_renderOwnershipUnexamined174!=~0u)++g_renderOwnershipUnexamined174;return false;}
 ++g_renderOwnershipQueries174;return true;
}
bool SameRenderOwnership174(const RenderDrawSample& a,const RenderDrawSample& b)
{
 return a.frame==b.frame&&a.context==b.context&&a.buffer==b.buffer&&a.target==b.target&&a.depth==b.depth&&
  a.width==b.width&&a.height==b.height&&a.format==b.format&&a.depthWidth==b.depthWidth&&a.depthHeight==b.depthHeight&&
  a.depthSamples==b.depthSamples&&a.depthFormat==b.depthFormat&&a.depthViewFormat==b.depthViewFormat&&
  !memcmp(&a.viewport,&b.viewport,sizeof(a.viewport))&&!memcmp(&a.camera,&b.camera,sizeof(a.camera));
}
void RetainRenderOwnership174(const RenderDrawSample& sample)
{
 RenderDrawSample* free=nullptr;
 for(auto& stored:g_renderDrawSamples){if(SameRenderOwnership174(stored,sample))return;if(stored.frame!=sample.frame&&!free)free=&stored;}
 if(free)*free=sample;else if(g_renderOwnershipOverflow174!=~0u)++g_renderOwnershipOverflow174;
}
void ObserveRenderDraw(ID3D11DeviceContext* ctx,ID3D11Buffer* buffer,const CameraSummary& camera)
{
 const auto controls126=fc3pad::Latest();
 if(g_frame%15!=0&&fabs(controls126.rightX)<.4&&fabs(controls126.rightY)<.4)return;
 if(!BeginRenderOwnership174(g_frame))return;
 RenderDrawSample s={};s.frame=g_frame;s.camera=camera;s.context=reinterpret_cast<uintptr_t>(ctx);s.buffer=reinterpret_cast<uintptr_t>(buffer);
 UINT n=1;ctx->RSGetViewports(&n,&s.viewport);
 ID3D11RenderTargetView* rtv=nullptr;ID3D11DepthStencilView* dsv=nullptr;ctx->OMGetRenderTargets(1,&rtv,&dsv);
 if(rtv){ID3D11Resource* resource=nullptr;rtv->GetResource(&resource);s.target=reinterpret_cast<uintptr_t>(resource);
  ID3D11Texture2D* texture=nullptr;if(resource&&SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture)))){D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);s.width=td.Width;s.height=td.Height;s.format=td.Format;texture->Release();}if(resource)resource->Release();rtv->Release();}
 if(dsv){D3D11_DEPTH_STENCIL_VIEW_DESC view={};dsv->GetDesc(&view);s.depthViewFormat=view.Format;
  ID3D11Resource* resource=nullptr;dsv->GetResource(&resource);s.depth=reinterpret_cast<uintptr_t>(resource);
  ID3D11Texture2D* texture=nullptr;if(resource&&SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture)))){D3D11_TEXTURE2D_DESC td={};texture->GetDesc(&td);s.depthWidth=td.Width;s.depthHeight=td.Height;s.depthSamples=td.SampleDesc.Count;s.depthFormat=td.Format;texture->Release();}if(resource)resource->Release();dsv->Release();}
 RetainRenderOwnership174(s);
}
void RenderProbeWrite(const char* text)
{
 if(g_renderProbe==INVALID_HANDLE_VALUE||g_renderProbeBytes>8*1024*1024)
 {
  if(g_renderProbe!=INVALID_HANDLE_VALUE)CloseHandle(g_renderProbe);
  wchar_t path[MAX_PATH];_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%sfc3_render_transition_%lu_%u.csv",fc3::DataFolder(),GetCurrentProcessId(),g_renderProbeSegment++%4);
  g_renderProbe=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);g_renderProbeBytes=0;
  if(g_renderProbe==INVALID_HANDLE_VALUE)return;
  const char* header="type,tick_ms,frame,data (frame: gameplay heuristic/menu/input/aim/cameras/draw counters; draw: context/CB/RT/viewport/native camera)\r\n";DWORD written=0;WriteFile(g_renderProbe,header,static_cast<DWORD>(strlen(header)),&written,nullptr);g_renderProbeBytes+=written;
  Log("[RENDER PROBE] read-only gameplay/menu trace: %S; four rotating 8 MiB segments",path);
 }
 DWORD written=0;WriteFile(g_renderProbe,text,static_cast<DWORD>(strlen(text)),&written,nullptr);g_renderProbeBytes+=written;
}
void TickRenderTransitionProbe(HWND window)
{
 static ULONGLONG last=0,flush=0;static bool oldGame=false,oldMenu=false,oldWeapon=false;static float oldScale=0;static LONG mono=0,stereo=0,camera=0,failed=0;
 const auto now=GetTickCount64();const auto c=fc3pad::Latest();const bool weapon=g_visibleWeaponDrawsThisFrame>0;
 const bool transition=oldGame!=g_inGameplay||oldMenu!=g_pistolMenu.open||oldWeapon!=weapon||std::fabs(oldScale-g_mainXScale)>.1f;
 const bool marker=g_vehicleProbeMarker90;g_vehicleProbeMarker90=false;
 if(now-last<250&&!transition&&!marker)return;
 last=now;oldGame=g_inGameplay;oldMenu=g_pistolMenu.open;oldWeapon=weapon;oldScale=g_mainXScale;
 char text[7000]={};int used=_snprintf_s(text,sizeof(text),_TRUNCATE,
 "frame,%llu,%ld,transition=%d,gameplay=%d,insert=%d,focus=%d,weapon=%ld/%ld,profile=%d,epoch=%u,BB=%ux%u,head=%d,hands=%d/%d,aimReady=%d,aimActive=%d,bodyValid=%d,bodyYaw=%.6f,rightSign=%.1f,mainScale=%.6f,world=%.6f/%.6f,camValid=%d,forward=%.6f/%.6f/%.6f,right=%.6f/%.6f/%.6f,pos=%.6f/%.6f/%.6f,sticks=%.3f/%.3f/%.3f/%.3f,triggers=%.3f/%.3f,menuButton=%d,drawMono=%ld,drawStereo=%ld,failed=%ld,uploads=%ld\r\n",
 now,g_frame,transition,g_inGameplay,g_pistolMenu.open,GetForegroundWindow()==window,g_visibleWeaponDrawsThisFrame,g_visibleWeaponDrawsLastFrame,g_weaponMechanicProfile,g_armPoseEpoch,g_bbW,g_bbH,g_eyeViewsSet,g_handRightValid,g_handLeftValid,fc3aim::Ready(),g_aimActive,g_bodyValid,g_bodyYaw,g_rightSign,g_mainXScale,g_worldProjection.xs,g_worldProjection.ys,g_frameCameraValid,g_frameCamera.forward[0],g_frameCamera.forward[1],g_frameCamera.forward[2],g_frameCamera.right[0],g_frameCamera.right[1],g_frameCamera.right[2],g_frameCamera.position[0],g_frameCamera.position[1],g_frameCamera.position[2],c.leftX,c.leftY,c.rightX,c.rightY,c.leftTrigger,c.rightTrigger,c.menu,g_stats.monoDraws-mono,g_stats.stereoDraws-stereo,g_stats.failedDraws-failed,g_stats.cameraWrites-camera);
 // Vehicle diagnosis is observation only. The player-look getter is not proof of
 // vehicle or CPU cull-camera ownership. Compare these draw-time camera values
 // with the raw and compensated eye bases, world draw count, and capture marker.
 if(used>0&&used<5600){
  const float* raw=g_eyeViewsBody[0].view;const float* eye=g_eyeViews[0].view;
  int fovState=0,fovCandidates=0,fovAttempts=0;bool fovBound=false;
  fc3fov::ProbeState(fovState,fovCandidates,fovAttempts,fovBound);
  const double camYaw=std::atan2(double(g_frameCamera.forward[1]),double(g_frameCamera.forward[0]));
  const double rawYaw=std::atan2(double(raw[8]),double(raw[10]));
  const double eyeYaw=std::atan2(double(eye[8]),double(eye[10]));
  // Recover both tracked eye origins from the raw view matrices, then average
  // away IPD. This is diagnostic only: it never changes headset placement.
  double headCenter[3]={};
  if(g_eyeViewsSet)for(int side=0;side<2;++side){
   const float* m=g_eyeViewsBody[side].view;
   for(int axis=0;axis<3;++axis)
    headCenter[axis]-=.5*(double(m[axis])*m[3]+double(m[4+axis])*m[7]+double(m[8+axis])*m[11]);
  }
  used+=_snprintf_s(text+used,sizeof(text)-used,_TRUNCATE,
   "vehicle90,%llu,%ld,marker=%d,autoReason=%d,worldDraws=%ld,weaponDraws=%ld,gameplay=%d,menu=%d,camValid=%d,camPos=%.5f/%.5f/%.5f,camForward=%.6f/%.6f/%.6f,camYaw=%.5f,rawHead=%.6f/%.6f/%.6f,rawYaw=%.5f,rawHeadCenter=%.5f/%.5f/%.5f,vrScale=%.3f,eyeHead=%.6f/%.6f/%.6f,eyeYaw=%.5f,aimActive=%d,bodyValid=%d,visibility=%d,handoff=%d,bodyYaw=%.5f,worldScale=%.5f/%.5f,fovCovered=%d,fovState=%d,fovCandidates=%d,fovAttempts=%d,fovBound=%d,grips=%.3f/%.3f,sticks=%.3f/%.3f/%.3f/%.3f,triggers=%.3f/%.3f\r\n",
   now,g_frame,marker?1:0,g_vehicleAutoCaptureReason91,g_mainBinCount,g_visibleWeaponDrawsThisFrame,g_inGameplay?1:0,fc3pad::GameplayInputBlocked()?1:0,g_frameCameraValid?1:0,
   g_frameCamera.position[0],g_frameCamera.position[1],g_frameCamera.position[2],g_frameCamera.forward[0],g_frameCamera.forward[1],g_frameCamera.forward[2],camYaw,
   raw[8],raw[9],raw[10],rawYaw,headCenter[0],headCenter[1],headCenter[2],g_worldScale,eye[8],eye[9],eye[10],eyeYaw,g_aimActive?1:0,g_bodyValid?1:0,g_visibilityCamera68?1:0,g_visibilityHandoff68?1:0,g_bodyYaw,
   g_worldProjection.xs,g_worldProjection.ys,fc3fov::Covered()?1:0,fovState,fovCandidates,fovAttempts,fovBound?1:0,c.leftGrip,c.rightGrip,c.leftX,c.leftY,c.rightX,c.rightY,c.leftTrigger,c.rightTrigger);
  // Body-frame hand poses allow replaying two-grip wheel kinematics offline.
  // Controller positions alone never feed a steering axis in this candidate.
  if(g_handRightValid&&g_handLeftValid&&used>0&&used<6200){
   const float* rightHand=g_handBody[0];const float* leftHand=g_handBody[1];
   used+=_snprintf_s(text+used,sizeof(text)-used,_TRUNCATE,
    "vehicleHands94,%llu,%ld,right=%.5f/%.5f/%.5f,left=%.5f/%.5f/%.5f,rightForward=%.5f/%.5f/%.5f,leftForward=%.5f/%.5f/%.5f,grips=%.3f/%.3f,triggers=%.3f/%.3f\r\n",
    now,g_frame,rightHand[3],rightHand[7],rightHand[11],leftHand[3],leftHand[7],leftHand[11],
    rightHand[2],rightHand[6],rightHand[10],leftHand[2],leftHand[6],leftHand[10],
    c.rightGrip,c.leftGrip,c.rightTrigger,c.leftTrigger);
  }
 }
 mono=g_stats.monoDraws;stereo=g_stats.stereoDraws;failed=g_stats.failedDraws;camera=g_stats.cameraWrites;
 for(const auto& bin:g_cameraBins)if(bin.count&&used>0&&used<6000)used+=_snprintf_s(text+used,sizeof(text)-used,_TRUNCATE,"bin,%llu,%ld,scale=%.6f,count=%ld,projection=%.6f/%.6f/%.6f/%.6f,right=%.6f/%.6f/%.6f\r\n",now,g_frame,bin.xScale,bin.count,bin.projection.xs,bin.projection.ys,bin.projection.a,bin.projection.b,bin.right[0],bin.right[1],bin.right[2]);
 for(const auto& s:g_renderDrawSamples)if(s.frame>=0&&g_frame-s.frame<=15&&used>0&&used<6000)used+=_snprintf_s(text+used,sizeof(text)-used,_TRUNCATE,"draw,%llu,%ld,context=%p,CB=%p,RT=%p,RTsize=%ux%u,format=%u,DS=%p,DSsize=%ux%u,DSformat=%u,DSviewFormat=%u,DSsamples=%u,viewport=%.1f/%.1f/%.1f/%.1f,projection=%.6f/%.6f/%.6f/%.6f,forward=%.6f/%.6f/%.6f,uploadRva=%08X,uploadMethod=%u\r\n",now,s.frame,reinterpret_cast<void*>(s.context),reinterpret_cast<void*>(s.buffer),reinterpret_cast<void*>(s.target),s.width,s.height,s.format,reinterpret_cast<void*>(s.depth),s.depthWidth,s.depthHeight,s.depthFormat,s.depthViewFormat,s.depthSamples,s.viewport.TopLeftX,s.viewport.TopLeftY,s.viewport.Width,s.viewport.Height,s.camera.xScale,s.camera.yScale,s.camera.zScale,s.camera.zOffset,s.camera.forward[0],s.camera.forward[1],s.camera.forward[2],s.camera.uploadCallerRva,s.camera.uploadMethod);
 if(g_renderOwnershipFrame174>=0&&g_frame-g_renderOwnershipFrame174<=15&&used>0&&used<6500)
  used+=_snprintf_s(text+used,sizeof(text)-used,_TRUNCATE,"draw_budget174,%llu,%ld,queries=%u,overflow=%u,unexamined=%u,capacity=4,queryCap=16,bindings=original_left\r\n",now,g_renderOwnershipFrame174,g_renderOwnershipQueries174,g_renderOwnershipOverflow174,g_renderOwnershipUnexamined174);
 RenderProbeWrite(text);if(g_renderProbe!=INVALID_HANDLE_VALUE&&now-flush>=2000){FlushFileBuffers(g_renderProbe);flush=now;}
}
