// Role permission shared by sidearm contact adapters. Native inventory stays unchanged.
bool LeftSidearm315(bool presentation=false){
 const auto s=presentation?fc3hands303::ReadPresentation314():fc3hands303::Read();
 return fc3hands303::Sidearm315(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&s.enabled&&s.active&&s.engaged&&!s.empty&&s.hand==1&&s.nativeHoldingSide==0&&!MacheteHeld();
}
Mat4 SidearmMirror315(){Mat4 m=HandIdentity();m.m[0]=-1;return m;}
// Semantic contact wrist has a reflected bind axis; restore a proper rigid
// model transform after applying the native left-hand ammo attachment.
Mat4 SidearmCarried315(const Mat4& wrist,const Mat4& attachment){
 const Mat4 body=Multiply(wrist,attachment);const auto held=fc3hands303::Read();
 const bool reflected=fc3hands303::Sidearm315(g_weaponMechanicProfile)&&held.enabled&&!held.empty&&held.hand==1&&held.nativeHoldingSide==0;
 return reflected?Multiply(body,SidearmMirror315()):body;
}
