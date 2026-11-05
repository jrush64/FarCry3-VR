// Scene-only aiming assistance: one crop per actual eye, around its own ray.
// This never touches XR poses, native camera/ADS, gun/arm rows or gameplay aim.
struct OpticSceneState132{double scale=1;ULONGLONG tick=0;LONG profile=-1;UINT epoch=0;}g_opticScene132;
bool OpticAligned132(){
 if(!g_enabled||!g_inGameplay||g_flatViewActive||g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::GameplayInputBlocked()||fc3pad::WeaponSelectionRequested()||!MotionInteractionEnabled()||!g_eyeViewsSet||!g_handRightValid||!fc3pad::Latest().active||PistolBusy()||g_reload.held||g_reload.pending||ReloadPoseActive())return false;
 const bool holo=HoloProfile131();if(!holo&&(!g_scopeEnabled60||!ScopeHoldAllowed65()||ScopeTrigger65()<=.35f))return false;
 for(int eye=0;eye<2;++eye){const auto& packet=g_scopeEyes60[eye];if(packet.spec<0||packet.spec>=int(sizeof(kScopeSpecs60)/sizeof(kScopeSpecs60[0]))||(kScopeSpecs60[packet.spec].profile>=0&&kScopeSpecs60[packet.spec].profile!=g_weaponMechanicProfile)||packet.ownerProfile!=g_weaponMechanicProfile||packet.epoch!=g_armPoseEpoch||packet.frame>g_frame||g_frame-packet.frame>2)continue;
  ScopeOptics60 lens={};if(!ScopeBodyOptics72(packet,eye,lens))lens=ScopeOpticsForEye60(ScopeBase62(packet),1,true);
  ImVec2 dot={};if(!(holo?HoloDot131(eye,dot):OpticShotDot131(eye,dot))||!lens.valid)continue;
  if(fabs(lens.center.x)<.45&&fabs(lens.center.y)<.45&&OpticContains131(lens,dot))return true;
 }return false;
}
// Zoom is an input action, independent of eye-to-glass placement. Physical
// aperture clipping still belongs to the lens renderer, not activation.
bool OpticZoomHeld133(){
 if(!g_enabled||!g_inGameplay||g_flatViewActive||g_pistolMenu.open||fc3pad::NativeMenuOpen()||fc3pad::GameplayInputBlocked()||fc3pad::WeaponSelectionRequested()||!MotionInteractionEnabled()||!g_eyeViewsSet||!g_handRightValid||!fc3pad::Latest().active||g_reload.held||g_reload.pending||g_pistolReload.pending||g_pistolReload.removalHeld)return false;
 const auto& p=g_scopeEyes60[0];if(p.spec<0||p.spec>=int(sizeof(kScopeSpecs60)/sizeof(kScopeSpecs60[0]))||p.ownerProfile!=g_weaponMechanicProfile||p.epoch!=g_armPoseEpoch||p.frame>g_frame||g_frame-p.frame>2)return false;
 if(kScopeSpecs60[p.spec].profile>=0&&kScopeSpecs60[p.spec].profile!=g_weaponMechanicProfile)return false;
 return HoloProfile131()?g_holoEnabled131&&fc3pad::Latest().leftTrigger>.35f:g_scopeEnabled60&&ScopeHoldAllowed65()&&ScopeTrigger65()>.35f;
}
bool HoloLensActive132(){return g_holoEnabled131&&g_holoLensEnabled132&&g_holoLensZoom132>1.001f&&HoloProfile131()&&OpticZoomHeld133();}
void UpdateOpticScene132(){
 auto& s=g_opticScene132;const auto now=GetTickCount64();
 const bool holo=HoloProfile131(),enabled=holo?g_holoSceneEnabled132:g_scopeSceneEnabled132;
 const bool active=enabled&&OpticZoomHeld133();const double wanted=active?(holo?g_holoSceneZoom132:g_scopeSceneZoom132):1;
 if(!active||s.profile!=g_weaponMechanicProfile||s.epoch!=g_armPoseEpoch||!s.tick||now-s.tick>250)s.scale=1;
 if(active){const double dt=(std::min)(.05,double(now-s.tick)*.001);s.scale+=(wanted-s.scale)*(1-std::exp(-dt/.12));}
 s.tick=now;s.profile=g_weaponMechanicProfile;s.epoch=g_armPoseEpoch;
}
Mat4 OpticSceneMatrix132(const Mat4& matrix,int variant,int eye){
 if((variant!=0&&variant!=2)||g_opticScene132.scale<=1.00001||g_opticScene132.profile!=g_weaponMechanicProfile||g_opticScene132.epoch!=g_armPoseEpoch)return matrix;
 ImVec2 dot={};if(!(HoloProfile131()?HoloDot131(eye,dot):OpticShotDot131(eye,dot))){
  // Brief loss of native aim publication must not disable a held zoom action.
  ScopeOptics60 lens={};const auto& p=g_scopeEyes60[eye];if(p.ownerProfile==g_weaponMechanicProfile&&p.epoch==g_armPoseEpoch&&p.frame<=g_frame&&g_frame-p.frame<=2&&ScopeBodyOptics72(p,eye,lens))dot=lens.axis;
  else {const auto projection=EyeProjectionMatrix(g_eyeViews[eye],g_worldProjection);dot={float(projection.m[2]),float(projection.m[6])};}
 }
 return Multiply(ScopeCrop62(g_opticScene132.scale,dot.x,dot.y),matrix);
}
