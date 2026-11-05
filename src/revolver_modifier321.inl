bool RevolverReloadLease321(){
 const auto& r=g_revolver;return RevolverModel()&&(r.open||r.angle>.5||r.grab||r.carrying||r.pending||RevolverClipPlaying()||RevolverNativeLeft());
}
void PublishRevolverModifier321(){
 const auto h=fc3hands303::Read();const bool enabled=g_weaponMechanicProfile==42&&h.enabled&&h.engaged&&!h.empty&&h.nativeHoldingSide==0;
 const bool allowed=enabled&&h.active&&h.hand>=0&&h.hand<2&&g_frame-g_weaponMechanicFrame<=2&&g_physicalValid303[0]&&g_physicalValid303[1]&&!RevolverReloadLease321()&&!g_reload.held&&!g_reload.pending&&!g_reloadAlignment&&!ReloadPoseActive()&&!g_pistolMenu.open&&!fc3pad::MenuOpen()&&!fc3pad::NativeMenuOpen()&&!fc3pad::GameplayInputBlocked();
 fc3hands303::PublishTransfer307(enabled,allowed,allowed,h.generation,h.entity,g_armPoseEpoch,true,true);
}
void RevolverModifierNote321(){
 if(g_weaponMechanicProfile!=42||g_pistolMenu.open)return;const auto h=fc3hands303::Read();const auto t=fc3hands303::TransferContact307();if(!t.modifier321||!t.enabled||!t.allowed||t.entity!=h.entity||t.generation!=h.generation||h.hand<0||h.hand>1)return;
 float rg=0,lg=0,rt=0,lt=0;fc3hands303::RawControls(rg,lg,rt,lt);if((h.hand?lg:rg)<.7f)return;
 fc3xr::SetTestNote(h.hand==0?L".44 PASS MODE\nHold both grips, pull LEFT hand left to take.\nRelease RIGHT grip to reload. Release both between passes.":L".44 PASS MODE\nHold both grips, pull RIGHT hand right to take.\nRelease LEFT grip to reload. Release both between passes.");
}
