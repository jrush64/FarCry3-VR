bool LeftShotgun318(bool presentation=false){
 const auto h=presentation?fc3hands303::ReadPresentation314():fc3hands303::Read();
 return fc3hands303::Shotgun318(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&h.enabled&&h.active&&h.engaged&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0&&!MacheteHeld();
}
bool LeftShotgunOwner318(){const auto h=fc3hands303::Read();return fc3hands303::Shotgun318(g_weaponMechanicProfile)&&h.enabled&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0;}
bool ShotgunSupportAccepted342(const fc3pad::Controllers& c){
 const auto h=fc3hands303::Read();return h.enabled&&h.active&&h.engaged&&!h.empty&&g_frame-g_weaponMechanicFrame<=2&&
  (M133SupportProfile56()||SpasModel())&&fc3pad::SupportHold342(c,g_weaponMechanicProfile,h.entity,h.generation,GetTickCount64());
}
// A native animation lease is not an actual shell transaction. Once the XR
// resolver owns the M133 support squeeze, only physical reload state can take it.
bool M133ControllerSupport356(){
 return g_manualReloadEnabled&&M133SupportProfile56()&&ShotgunSupportAccepted342(fc3pad::Latest())&&
  !g_reload.held&&!g_reload.pending&&!g_reloadAlignment&&!g_manualReload.pending&&!g_manualReload.cycle;
}
bool ShotgunPhysicalWrist318(int role,Mat4& wrist){
 const int physical=LeftShotgunOwner318()?1-role:role;if(physical<0||physical>1||!g_physicalValid303[physical])return false;
 const auto& a=g_freeHandAttachment[physical];if(a.epoch!=g_armPoseEpoch||a.profile!=g_weaponMechanicProfile||g_frame-a.frame>2)return false;
 Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[physical][i];wrist=Multiply(controller,a.relative);return true;
}
V3 ShotgunMotion318(int role,const V3& fallback){if(!LeftShotgun318())return fallback;const int side=1-role;if(!g_physicalValid303[side])return fallback;return ControllerWrist(g_physicalBody303[side],side);}
// Proper virtual role wrist for the1887's temporary support-hand gun parent.
Mat4 ShotgunCanonicalWrist318(int role,const Mat4& actual){
 const double physicalPalm=role==0?-.6781:.6781;Mat4 shift=HandIdentity();shift.m[3]=2*physicalPalm;return Multiply(actual,shift);
}
bool ShotgunPalm318(int role,const float* offset,V3& palm){
 if(!LeftShotgun318())return false;Mat4 wrist={};if(!ShotgunPhysicalWrist318(role,wrist))return false;
 const double scale=(std::max)(.1f,g_worldScale);palm=Apply(wrist,{(role==0?-.6781:.6781)-offset[0]*scale,.0519+offset[1]*scale,1.0387+offset[2]*scale});return true;
}
Mat4 ShotgunHeld318(const Mat4& canonical,const Mat4& attachment,int role,bool semantic=true){
 const Mat4 old=Multiply(canonical,attachment);if(!LeftShotgunOwner318())return old;
 Mat4 actual={};if(ShotgunPhysicalWrist318(role,actual)){const auto m=SidearmMirror315();return Multiply(actual,Multiply(m,Multiply(attachment,m)));}
 return semantic?Multiply(old,SidearmMirror315()):old;
}

V3 ShotgunCameraWrist318(const V3& fallback){return LeftShotgun318()?Apply(ArmBodyToCamera(),ShotgunMotion318(1,fallback)):fallback;}
