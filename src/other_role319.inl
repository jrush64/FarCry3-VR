// Physical-role adapters only; native inventory/ammo handlers remain canonical.
bool LeftOther319(bool presentation=false){const auto h=presentation?fc3hands303::ReadPresentation314():fc3hands303::Read();return fc3hands303::Other319(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&h.enabled&&h.active&&h.engaged&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0&&!MacheteHeld();}
bool LeftOtherOwner319(){const auto h=fc3hands303::Read();return fc3hands303::Other319(g_weaponMechanicProfile)&&h.enabled&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0;}
bool OtherWrist319(int role,Mat4& wrist){const int side=LeftOtherOwner319()?1-role:role;if(side<0||side>1||!g_physicalValid303[side])return false;const auto& a=g_freeHandAttachment[side];if(a.epoch!=g_armPoseEpoch||a.profile!=g_weaponMechanicProfile||g_frame-a.frame>2)return false;Mat4 controller={};for(int i=0;i<16;++i)controller.m[i]=g_physicalBody303[side][i];wrist=Multiply(controller,a.relative);return true;}
V3 OtherMotion319(int role,const V3& old){if(!LeftOther319()||!g_physicalValid303[1-role])return old;return ControllerWrist(g_physicalBody303[1-role],1-role);}
V3 OtherCameraWrist319(const V3& old){return LeftOther319()?Apply(ArmBodyToCamera(),OtherMotion319(1,old)):old;}
Mat4 OtherCarried319(const Mat4& wrist,const Mat4& attachment){const auto body=Multiply(wrist,attachment);return LeftOtherOwner319()?Multiply(body,SidearmMirror315()):body;}
bool RightBow319(bool presentation=false){const auto h=presentation?fc3hands303::ReadPresentation314():fc3hands303::Read();return g_weaponMechanicProfile==10&&g_frame-g_weaponMechanicFrame<=2&&h.enabled&&h.active&&h.engaged&&!h.empty&&h.nativeHoldingSide==1&&h.hand==0;}
Mat4 BowCarried319(const Mat4& wrist,const Mat4& attach){const auto old=Multiply(wrist,attach);const auto h=fc3hands303::Read();return g_weaponMechanicProfile==10&&h.enabled&&!h.empty&&h.nativeHoldingSide==1&&h.hand==0?Multiply(old,SidearmMirror315()):old;}

Mat4 OtherController319(int role,const Mat4& fallback){if(!LeftOther319()||!g_physicalValid303[1-role])return fallback;Mat4 m={};for(int i=0;i<16;++i)m.m[i]=g_physicalBody303[1-role][i];return m;}

// Mirrored meshes use a reflected grasp; the unmirrored RPG keeps its native contact. Reload
// hands still fit their individual operating parts through OtherMirrorHand319.
V3 OtherSupportPoint344(V3 point){if(LeftOther319(true)&&!RpgModel())point.x=-point.x;return point;}
Mat4 OtherSupportFrame344(const Mat4& frame){if(!LeftOther319(true))return frame;const auto m=SidearmMirror315();if(RpgModel()){auto plane=m;plane.m[3]=2*Apply(frame,{-.6781,.0519,1.0387}).x;return Multiply(plane,Multiply(frame,m));}return Multiply(m,Multiply(frame,m));}

bool RightBowRoot350(Mat4&);Mat4 BowHoldingGrip350();
