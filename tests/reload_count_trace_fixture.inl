static void* g_traceFixtureStore=nullptr;
static UINT g_traceFixtureProperty=0,g_traceFixtureEax=0,g_traceFixtureEcx=0,g_traceFixtureFlags=0;
static const float g_traceFixtureSeed[4]={1.25f,-2.5f,3.75f,-4.f};
static float g_traceFixtureSse[4]={};
static UINT g_traceFixtureStackCount=0;
__declspec(naked) void __cdecl FixtureTraceNativeStore(UINT state,UINT wrapper,UINT property,UINT count)
{
    __asm
    {
        push ebp
        mov ebp,esp
        push ebx
        sub esp,64
        mov ebx,[ebp+8]
        mov eax,[ebp+16]
        mov ecx,[ebp+20]
        mov [esp+16],ecx // callee native ESP+14h includes the pushed return address
        movups xmm0,[g_traceFixtureSeed]
        stc
        call dword ptr [g_traceFixtureStore]
        mov [g_traceFixtureEax],eax
        mov [g_traceFixtureEcx],ecx
        pushfd
        pop edx
        mov [g_traceFixtureFlags],edx
        movups [g_traceFixtureSse],xmm0
        mov edx,[esp+16]
        mov [g_traceFixtureStackCount],edx
        add esp,64
        pop ebx
        pop ebp
        ret
    }
}
void __stdcall FixtureTraceNativeBegin(UINT state,UINT wrapper)
{
    FixtureReloadBegin(state,wrapper);
    FixtureTraceNativeStore(state,wrapper,g_traceFixtureProperty,7-ReloadRead(g_reloadFixtureWeapon,0xcc));
}
void TestReadOnlyReloadCountTrace()
{
    const LONG savedFrame=g_frame;const bool savedMotion=g_motionGun;
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* tablePage=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&tablePage&&code,"read-only count trace allocation");
    const UINT actor=reinterpret_cast<UINT>(page),component=actor+4096,state=component+0x718,wrapper=actor+8192;
    const UINT weapon=actor+12288,hud=actor+16384,property=actor+20480,table=reinterpret_cast<UINT>(tablePage);
    *reinterpret_cast<UINT*>(actor)=table;*reinterpret_cast<UINT*>(actor+0x24)=component;*reinterpret_cast<UINT*>(state)=actor;
    *reinterpret_cast<UINT*>(wrapper+0x48)=weapon;*reinterpret_cast<UINT*>(weapon+0xcc)=5;
    *reinterpret_cast<UINT*>(hud+0x134)=5;*reinterpret_cast<UINT*>(hud+0x138)=7;
    *reinterpret_cast<UINT*>(hud+0x140)=997;*reinterpret_cast<UINT*>(hud+0x144)=1002;
    *reinterpret_cast<UINT*>(table+0x7c)=0x12345678;DWORD protection=0;
    Check(VirtualProtect(tablePage,4096,PAGE_READONLY,&protection)!=0,"native-style read-only function table");
    UINT getter=0;
    Check(!AmmoRead(table+0x7c,&getter,4)&&ReloadRead(table,0x7c)==0,"former writable-only reader reproduces missing live getter");
    Check(ReloadTraceRead(table+0x7c,&getter,4)&&getter==0x12345678,"passive trace can read read-only function table");
    std::memcpy(code,kNativeReloadCountStore,sizeof(kNativeReloadCountStore));code[sizeof(kNativeReloadCountStore)]=0xc3;
    Check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection)!=0,"real count store instructions in executable read-only memory");
    FlushInstructionCache(GetCurrentProcess(),code,8);
    const auto init=MH_Initialize();Check(init==MH_OK||init==MH_ERROR_ALREADY_INITIALIZED,"native count hook library");
    Check(MH_CreateHook(code,reinterpret_cast<void*>(ReloadCountStoreBoundary),&g_nativeReloadCountStore)==MH_OK,"hook actual x86 count/value-dirty store bytes");
    Check(MH_EnableHook(code)==MH_OK,"activate count observation hook");g_traceFixtureStore=code;g_traceFixtureProperty=property;
    g_frame=60000;g_motionGun=true;g_reload=PhysicalReloadState{};g_reloadPose=ReloadPoseCycle{};
    g_reloadCountTrace=ReloadCountTrace{};g_reload.installed=true;g_reload.shotgunFrame=g_frame;g_physicalReloadEnabled=false;
    g_reloadFixtureWeapon=weapon;g_reloadFixtureHud=hud;
    g_nativeReloadBegin=FixtureTraceNativeBegin;g_nativeReloadEnd=FixtureReloadEnd;g_nativeReload=FixtureNativeReload;
    fc3pad::Controllers input={};input.active=true;input.leftGrip=1;fc3pad::Publish(input);ObserveAmmoWidget(wrapper,hud);
    volatile UINT stackCanary=0x13579abc;
    ReloadBeginBoundary(state,wrapper);
    Check(stackCanary==0x13579abc&&g_reloadCountTrace.writes==1&&g_reloadCountTrace.property==property,"native register/stack context captures actual property pointer once");
    Check(ReloadRead(property,8)==2&&(ReloadRead(property,4)&255)==1,"original game store still writes computed two-shell count and dirty byte");
    Check(g_traceFixtureEax==property&&g_traceFixtureEcx==2&&(g_traceFixtureFlags&1),"observer preserves native EAX/ECX and flags for original store");
    Compare(g_traceFixtureSeed,g_traceFixtureSse,4,"observer preserves SSE across deliberate probe clobber");
    Check(ReloadRead(weapon,0xcc)==5&&ReloadRead(hud,0x140)==997&&!g_reload.mode&&!g_reload.credit,"observation does not limit count, add ammo or arm physical gate");
    ReloadBoundary(state,wrapper,0,-1);FixtureTraceNativeStore(state,wrapper,property,1);
    // Native Advance clears consumed and re-arms pending while count is positive.
    *reinterpret_cast<WORD*>(state+8)=1;
    Check(ReloadRead(weapon,0xcc)==6&&ReloadRead(hud,0x140)==996&&g_reloadCountTrace.inserts==1,"first original native transfer observed unchanged");
    input.leftGrip=0;fc3pad::Publish(input);ReloadBoundary(state,wrapper,0,-1);FixtureTraceNativeStore(state,wrapper,property,0);
    *reinterpret_cast<WORD*>(state+8)=0;
    ReloadEndBoundary(state,wrapper,1);
    Check(ReloadRead(weapon,0xcc)==7&&ReloadRead(hud,0x140)==995&&g_reloadCountTrace.inserts==2&&g_reloadCountTrace.ends==1,"full ordinary reload survives grip changes and finishes without one-shell cap");
    Check(g_reloadCountTrace.writes==1&&ReloadRead(property,8)==0&&!ReloadPoseActive(),"scope only observes start store; later stores execute unchanged and native end restores posing");
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"native count hook cleanup");
    if(init==MH_OK)Check(MH_Uninitialize()==MH_OK,"count hook library cleanup");
    Check(VirtualProtect(tablePage,4096,PAGE_NOACCESS,&protection)!=0&&!ReloadTraceRead(table+0x7c,&getter,4),"passive read refuses inaccessible memory");
    g_nativeReloadCountStore=nullptr;g_traceFixtureStore=nullptr;g_reloadCountTrace=ReloadCountTrace{};
    g_reloadPose=ReloadPoseCycle{};g_reload=PhysicalReloadState{};g_nativeReload=nullptr;g_nativeReloadBegin=nullptr;g_nativeReloadEnd=nullptr;
    fc3pad::SetShotgunHandling(false);fc3pad::Publish(fc3pad::Controllers{});g_frame=savedFrame;g_motionGun=savedMotion;
    VirtualFree(page,0,MEM_RELEASE);VirtualFree(tablePage,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS: former reader fails on read-only vtable; actual engine count-store bytes hooked via MinHook; register/stack/flags/SSE preserved; original count/dirty writes unchanged; normal 5/997->7/995 reload, grip changes and native end intact; no physical input or ammo override");
}
