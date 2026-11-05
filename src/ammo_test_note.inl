void UpdateAmmoTestNote()
{
    if(BowNote()||RepairToolNote())return;
    if(Weapons8CaptureNote())return;
    if(Lever1887Note())return;
    if(FlareNote())return;
    if(RpgNote())return;
    if(Gm94Note())return;
    if(FlamerNote())return;
    if(RevolverNote())return;
    if(SpasNote())return;
    if(PistolReloadNote())return;
    if(AnimationProbeNote())return; // This identification run uses the four-weapon instructions.
    if(ManualReloadNote())return;
    if(g_manualSettings.mode==2&&!g_manualReloadEnabled){fc3xr::SetTestNote(ReloadPoseActive()?L"ORDINARY RELOAD ACTIVE\nThe game loads the remaining shells normally.":L"ORDINARY X RELOAD\nBulk reload selected in Reload Mode=2.");return;}
    if(g_singleShellEnabled&&!g_physicalReloadEnabled)
    {
        if(!g_reload.installed){fc3xr::SetTestNote(L"ONE-SHELL RELOAD HOOK NOT READY\nKeep the shotgun equipped.\nDo not test until this message changes.");return;}
        if(g_reloadCountTrace.cycle){fc3xr::SetTestNote(L"LOADING ONE SHELL\nLet this reload cycle finish.\nExtra X presses during it are ignored.");return;}
        fc3xr::SetTestNote(L"X = ONE SHELL\nTap and release X; let it finish.\nPress X again for the next shell. Grip is optional.");return;
    }
    if(g_nativeReloadCountStore&&!g_physicalReloadEnabled)
    {
        if(ReloadPoseActive()){fc3xr::SetTestNote(L"REGULAR RELOAD - RECORDING\nLet X reload finish normally.\nThe probe does not change shell count or ammo.");return;}
        if(g_reloadCountTrace.ends&&g_reloadCountTrace.inserts){fc3xr::SetTestNote(L"NATIVE RELOAD TRACE CAPTURED\nRegular reload and its actual count were recorded.\nNo physical-shell sequence needed.");return;}
        fc3xr::SetTestNote(L"REGULAR X ONLY - PROBE READY\nReload normally from a partly loaded shotgun.\nNo grip+X or shell insertion test in this build.");return;
    }
    if(!g_physicalReloadEnabled)
    {fc3xr::SetTestNote(ReloadPoseActive()?L"NATIVE RELOAD ACTIVE\nGrip changes do not end this reload pose.\nMove the left hand; it should keep tracking.":L"RELOAD POSE TEST\nUse X normally; squeeze/release left grip during reload.\nWaist reload remains paused.");return;}
    // Probes keep recording; actionable reload notes take priority during this candidate.
    if(!g_reload.installed&&AnimationProbeNote())return;
    if(g_reload.installed)
    {
        if(g_reload.failed){fc3xr::SetTestNote(L"INSERT DID NOT COMMIT\nOrdinary reload restored. Exit and say done.");return;}
        if(g_reload.held){double distance=0;fc3xr::SetTestNote(ReloadContactDistance(distance)&&distance<.15?L"SHELL AT RECEIVER\nRelease left grip to insert.":L"SHELL HELD\nMove the visible shell under receiver.");return;}
        if(g_reload.credit){fc3xr::SetTestNote(L"INSERTION QUEUED\nWaiting for the native shell event.");return;}
        if(g_reload.ready&&!ReloadShellReady()){fc3xr::SetTestNote(L"WAITING FOR SHELL VISUAL\nShell mesh must be captured before pickup.");return;}
        if(g_reload.ready&&ReloadRead(g_reload.weapon,0xcc)>=7)
        {fc3xr::SetTestNote(L"SHOTGUN FULL\nFire and pump, then grip at left waist for a shell.");return;}
        if(g_reload.ready)
        {wchar_t note[256];_snwprintf_s(note,256,_TRUNCATE,L"PHYSICAL RELOAD READY - %ld INSERTED\nGrip at left waist to take another shell.",g_reload.commits);fc3xr::SetTestNote(note);return;}
        fc3xr::SetTestNote(L"LEARNING NATIVE RELOAD\nShoot, pump, then X reload once normally.");return;
    }
    const LONG error=InterlockedCompareExchange(&g_autoAmmo.error,0,0);
    if(error)
    {
        wchar_t note[256];_snwprintf_s(note,256,_TRUNCATE,L"FILE ERROR %ld - RETRYING\nKeep this run open; no setup sequence needed.",error);fc3xr::SetTestNote(note);return;
    }
    const LONG writes=InterlockedCompareExchange(&g_ammoOwner.saved,0,0);
    if(writes>=3)
    {
        wchar_t note[256];_snwprintf_s(note,256,_TRUNCATE,L"CAPTURED: %ld writer changes\nFinish reloading, then exit and say done.",writes);fc3xr::SetTestNote(note);return;
    }
    if(InterlockedCompareExchange(&g_ammoOwner.worker,0,0)==1)
    {fc3xr::SetTestNote(L"RECORDING AMMO WRITERS\nShoot, pump, then X reload normally.");return;}
    const LONG changes=InterlockedCompareExchange(&g_autoAmmo.changes,0,0);
    if(InterlockedCompareExchange(&g_autoAmmo.candidates,0,0)>0)
    {
        wchar_t note[256];_snwprintf_s(note,256,_TRUNCATE,L"RECORDING: %ld ammo-layout changes\nShoot, pump, then X reload normally.",changes);fc3xr::SetTestNote(note);return;
    }
    if(InterlockedCompareExchange(&g_autoAmmo.packets,0,0)>0)
    {fc3xr::SetTestNote(L"RECORDING TEST INPUTS\nFinding live ammo counters automatically.");return;}
    fc3xr::SetTestNote(L"OPENING CAPTURE FILE\nRecording starts automatically.");
}
