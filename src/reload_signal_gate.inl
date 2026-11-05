// Equipment request4 reaches this site only AFTER the native reload predicate
// and signal selection. Request3 (start shooting) is never hooked.
struct ReloadSignalGate {
#ifdef FC3_AMMO_TEST
    bool enabled=false;
#else
    bool enabled=true;
#endif
    bool manualPistol=true,manualSkorpion=true,manualAk=true;
    UINT pistolState=0,pistolWrapper=0,pistolWeapon=0; ULONGLONG pistolPermissionUntil=0;
    bool installed=false; ULONGLONG retry=0;
    LONG blocked=0,allowed=0;
} g_reloadSignal;
void* g_reloadSignalOriginal=nullptr;
void* g_reloadSignalFallback=nullptr;
void* g_reloadRequestOriginal=nullptr;
void ObserveMagazineReloadRequest(UINT,UINT);bool RevolverManual();bool SpasManual();bool FlareActive();bool RpgActive();bool Gm94Active();bool FlamerActive();
__declspec(naked) void NativeReloadRequestBoundary()
{
    __asm {
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        push dword ptr [eax+4] // saved ESI: equipment wrapper
        push dword ptr [eax+40] // original first stack argument: native state
        call ObserveMagazineReloadRequest
        add esp,8
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        jmp dword ptr [g_reloadRequestOriginal]
    }
}
bool InstallReloadRequestAt(void* site)
{
    const BYTE signature[]={0x55,0x8b,0xec,0x8b,0x46,0x48};
    if(!site||memcmp(site,signature,sizeof(signature)))return false;
    if(MH_CreateHook(site,reinterpret_cast<void*>(NativeReloadRequestBoundary),&g_reloadRequestOriginal)!=MH_OK)return false;
    if(MH_EnableHook(site)!=MH_OK){MH_RemoveHook(site);g_reloadRequestOriginal=nullptr;return false;}
    return true;
}
void LoadReloadSignalSettings(const wchar_t* path)
{
    g_reloadSignal.enabled=GetPrivateProfileIntW(L"Reload",L"BlockAutomatic",1,path)!=0;
    g_reloadSignal.manualSkorpion=GetPrivateProfileIntW(L"SkorpionReload",L"Mode",0,path)==0;
    g_reloadSignal.manualAk=GetPrivateProfileIntW(L"AkReload",L"Mode",0,path)==0;
    g_reloadSignal.manualPistol=GetPrivateProfileIntW(L"PistolReload",L"Mode",0,path)==0;
}
bool ReloadSignalLocal(UINT state,UINT wrapper)
{
    const UINT actor=ReloadRead(state,0),weapon=ReloadRead(wrapper,0x48);
    if(!actor||!weapon||!g_singleReloadAuthority||!g_singleReloadManager||!ReloadContext(state,wrapper,weapon))return false;
    const UINT component=ReloadRead(actor,0x24);
    if(!component||state!=component+0x718)return false;
    const UINT reference=ReloadRead(actor,4),manager=ReloadRead(g_singleReloadManager,0);
    UINT id[2]={};if(!reference||!manager||!ReloadTraceRead(reference,id,sizeof(id)))return false;
    __try{return g_singleReloadAuthority(manager,id[0],id[1])!=0;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// Manual insertion will grant this exact native-owner permission. Raw X never
// creates one. It authorizes one transition, not a time window of repeated reloads.
bool GrantPistolReloadPermission(UINT state,UINT wrapper)
{
    if(!(((g_weaponMechanicProfile==20||g_weaponMechanicProfile==1||g_weaponMechanicProfile==12)&&g_reloadSignal.manualPistol)||(g_weaponMechanicProfile==39&&g_reloadSignal.manualSkorpion)||(RifleProfile(g_weaponMechanicProfile)&&g_reloadSignal.manualAk))||!g_motionGun||!MagazineProfile(g_weaponMechanicProfile)||g_frame-g_weaponMechanicFrame>2||!ReloadSignalLocal(state,wrapper))return false;
    g_reloadSignal.pistolState=state;g_reloadSignal.pistolWrapper=wrapper;
    g_reloadSignal.pistolWeapon=ReloadRead(wrapper,0x48);
    g_reloadSignal.pistolPermissionUntil=GetTickCount64()+3000;
    return true;
}
void ObservePistolEmpty(UINT,UINT);
bool PistolContactReady(UINT,UINT);
void PistolContactAccepted(UINT,UINT);
bool Lever1887Active();bool BowModel();
bool __cdecl RejectEmptyReloadSignal(UINT state,UINT wrapper,UINT signalPointer)
{
    const UINT signal=ReloadRead(signalPointer,0);
    // CRC32 values independently matched against extracted native XML signals.
    if(signal!=0xaf41179b&&signal!=0x3f456fdf)return false;
    const auto c=fc3pad::Latest();
    // Plain normalized X is interaction-only, including Simple mode and partially loaded weapons.
    // A real insertion pulse still reaches the existing per-weapon permission gate.
    if(fc3pad::InteractionOnly()&&ReloadSignalLocal(state,wrapper)){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[CONTROLS20] dedicated interaction: reload signal suppressed");return true;}
    // A physical M133/Bull shell is already at its port. Use the existing
    // game-thread request seam to finish one native transaction immediately.
    // The optional empty-reload toggle cannot grant a second animation here.
    if(g_manualReloadEnabled&&M133SupportProfile56()&&g_manualReload.pending&&
       g_motionGun&&g_frame-g_weaponMechanicFrame<=2&&ReloadSignalLocal(state,wrapper)){
        InstantM133Insert251(state,wrapper);return true;
    }
    if(!g_reloadSignal.enabled||!g_motionGun||g_weaponMechanicProfile<0||g_frame-g_weaponMechanicFrame>2||!ReloadSignalLocal(state,wrapper))return false;
    if(BowModel()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] bow animation/automatic refill refused; physical nock owns native refill");return true;}
    if(RevolverManual()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] revolver Manual: X/automatic reload refused (speedloader by hand)");return true;}
    if(FlamerActive()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] flamethrower: X/automatic reload refused (lever + canister by hand)");return true;}
    if(Gm94Active()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] GM-94: X/automatic reload refused (shells by hand)");return true;}
    if(RpgActive()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] RPG-7: X/automatic reload refused (rocket by hand)");return true;}
    if(FlareActive()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] flare gun: X/automatic reload refused (shell by hand)");return true;}
    if(SpasManual()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] SPAS Manual: X/automatic reload refused (shells by hand)");return true;}
    if(Lever1887Active()){if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] 1887 X/automatic reload refused: shells load by hand");return true;}
    const UINT weapon=ReloadRead(wrapper,0x48);
    if(((g_weaponMechanicProfile==20||g_weaponMechanicProfile==1||g_weaponMechanicProfile==12)&&g_reloadSignal.manualPistol)||(g_weaponMechanicProfile==39&&g_reloadSignal.manualSkorpion)||(RifleProfile(g_weaponMechanicProfile)&&g_reloadSignal.manualAk))
    {
        ObservePistolEmpty(state,wrapper);
        if(PistolContactReady(state,wrapper)&&GrantPistolReloadPermission(state,wrapper))PistolContactAccepted(state,wrapper);
        const bool permission=g_reloadSignal.pistolPermissionUntil&&GetTickCount64()<=g_reloadSignal.pistolPermissionUntil&&
            g_reloadSignal.pistolState==state&&g_reloadSignal.pistolWrapper==wrapper&&g_reloadSignal.pistolWeapon==weapon;
        if(permission){g_reloadSignal.pistolPermissionUntil=0;return false;}
        if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] 1911 native reload denied: no manual insertion permission; loaded %u; X interaction preserved",ReloadRead(weapon,0xcc));
        return true;
    }
    // INTERACT18: physical pump shotguns must forward ordinary X to doors and
    // mission prompts. Deny only reload signals, including partial-tube reload,
    // unless a real shell insertion has issued its bounded pending request.
    if(g_manualReloadEnabled&&fc3pad::ManualShotgunInputActive())
    {
        const bool insertion=g_manualReload.pending&&GetTickCount64()<=g_manualReload.deadline;
        if(!insertion&&InterlockedIncrement(&g_reloadSignal.blocked)<=16)
            Log("[INTERACT18] manual shotgun reload denied; ordinary X interaction preserved");
        return !insertion;
    }
    if(ReloadRead(weapon,0xcc)!=0)return false;
    // Preserve explicit native X until magazine ejection/insertion/slide exists.
    // Accepted shotgun insertion generates X and keeps its pending lease.
    if(fc3pad::ReloadOutput()||(g_manualReloadEnabled&&g_manualReload.pending&&GetTickCount64()<=g_manualReload.deadline))
    {
        if(InterlockedIncrement(&g_reloadSignal.allowed)<=16)Log("[RELOAD SIGNAL] empty reload authorized by explicit input/insertion, signal %08X",signal);
        return false;
    }
    if(InterlockedIncrement(&g_reloadSignal.blocked)<=16)Log("[RELOAD SIGNAL] empty automatic reload signal suppressed; native readiness fallback preserved, model %s",fc3weapon::kProfileNames[g_weaponMechanicProfile]);
    return true;
}
__declspec(naked) void NativeReloadSignalBoundary()
{
    __asm {
        pushfd
        pushad
        mov eax,esp
        sub esp,544
        and esp,0fffffff0h
        mov [esp+512],eax
        fxsave [esp]
        push dword ptr [eax+28]
        push dword ptr [eax+4]
        push dword ptr [ebp+8]
        call RejectEmptyReloadSignal
        add esp,12
        test al,al
        jz passthrough
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        jmp dword ptr [g_reloadSignalFallback]
    passthrough:
        fxrstor [esp]
        mov esp,[esp+512]
        popad
        popfd
        jmp dword ptr [g_reloadSignalOriginal]
    }
}
bool InstallReloadSignalAt(void* site,void* fallback)
{
    const BYTE signature[]={0x8b,0x00,0x51,0x8b,0xcc};
    if(!site||!fallback||memcmp(site,signature,sizeof(signature)))return false;
    g_reloadSignalFallback=fallback;
    if(MH_CreateHook(site,reinterpret_cast<void*>(NativeReloadSignalBoundary),&g_reloadSignalOriginal)!=MH_OK)return false;
    if(MH_EnableHook(site)!=MH_OK){MH_RemoveHook(site);g_reloadSignalOriginal=nullptr;return false;}
    return true;
}
bool InstallReloadSignalGate()
{
    if((!g_reloadSignal.enabled&&!fc3pad::DedicatedInteractionsEnabled())||g_reloadSignal.installed)return g_reloadSignal.installed;
    if(!g_reload.installed||GetTickCount64()<g_reloadSignal.retry)return false;
    g_reloadSignal.retry=GetTickCount64()+3000;
    auto* base=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));if(!base)return false;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp!=1664789792||nt->OptionalHeader.SizeOfImage!=48332800)return false;
    const BYTE predicate[]={0x55,0x8b,0xec,0x8b,0x46,0x48,0x57,0x6a,0x01};
    const BYTE branch[]={0x84,0xc0,0x74,0x28,0x8b,0x7e,0x48,0xe8};
    const BYTE fallback[]={0x8b,0x4e,0x48,0x8b,0x89,0xe8,0,0,0,0x8b,0x11};
    if(memcmp(base+0x12e1bb0,predicate,sizeof(predicate))||memcmp(base+0x12e1bbe,branch,sizeof(branch))||memcmp(base+0x12e1bea,fallback,sizeof(fallback)))return false;
    if(!InstallReloadSignalAt(base+0x12e1bca,base+0x12e1bea))return false;
    if(!InstallReloadRequestAt(base+0x12e1bb0)){
        MH_DisableHook(base+0x12e1bca);MH_RemoveHook(base+0x12e1bca);g_reloadSignalOriginal=nullptr;return false;
    }
    g_reloadSignal.installed=true;
    Log("[RELOAD SIGNAL] empty reload signal boundary installed; firing and native readiness paths unchanged; 1911 X reload requires manual insertion permission; other magazine X remains native fallback");
    return true;
}

