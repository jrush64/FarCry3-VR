//317 extends the AK adapter only to the explicitly supported magazine families.
bool LeftAk316(bool presentation=false){
 const auto h=presentation?fc3hands303::ReadPresentation314():fc3hands303::Read();
 return fc3hands303::MagazineRifle317(g_weaponMechanicProfile)&&g_frame-g_weaponMechanicFrame<=2&&h.enabled&&h.active&&h.engaged&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0&&!MacheteHeld();
}
Mat4 AkCarried316(const Mat4& wrist,const Mat4& attachment){
 const auto h=fc3hands303::Read();const Mat4 body=Multiply(wrist,attachment);
 return (fc3hands303::MagazineRifle317(g_weaponMechanicProfile)||fc3hands303::OtherMagazine319(g_weaponMechanicProfile))&&h.enabled&&!h.empty&&h.hand==1&&h.nativeHoldingSide==0?Multiply(body,SidearmMirror315()):body;
}
