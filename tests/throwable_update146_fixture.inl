unsigned releaseOriginalEntries146=0;
__declspec(naked) void CallUpdate146(UINT){
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        mov ebx,[g_strategy]
        mov esi,12345678h
        mov edi,87654321h
        mov ecx,ebx
        mov eax,13572468h
        mov edx,24681357h
        fninit
        fld dword ptr [kX87]
        movups xmm0,[g_seed]
        push 0
        push dword ptr [ebp+8]
        stc
        std
        call dword ptr [g_entry]
        mov [g_registers],eax
        mov [g_registers+4],ecx
        mov [g_registers+8],edx
        mov [g_registers+12],ebx
        mov [g_registers+16],esi
        mov [g_registers+20],edi
        movups [g_sse],xmm0
        fstp dword ptr [g_x87]
        pushfd
        pop eax
        mov [g_flags],eax
        cld
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
void TestUpdateBoundary146(UINT wrapper){
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(code&&MH_Initialize()==MH_OK,"native-update ABI allocation/library");
    memcpy(code,kUpdateBytes146,16);
    const BYTE tail[]={0x5f,0x5e,0x8b,0xe5,0x5d,0xc2,0x08,0x00};memcpy(code+16,tail,sizeof(tail));
    FlushInstructionCache(GetCurrentProcess(),code,32);g_entry=code;
    CallUpdate146(wrapper);UINT baseline[6];memcpy(baseline,g_registers,sizeof(baseline));const UINT flags=g_flags;
    Check(MH_CreateHook(code,reinterpret_cast<void*>(UpdateBoundary146),&g_updateOriginal146)==MH_OK&&MH_EnableHook(code)==MH_OK,"exact native update prologue relocation");
    SetIdentityReader143(ReadLocal143);PublishContext143(fixtureLocal143,3,true);
    UINT regs[9]={g_strategy,wrapper,releaseFrame146,0,g_strategy,0,g_projectile,0,0};Observe(1,regs);Prepared143 target;
    Check(CurrentPrepared143(target),"active ABI release preparation");
    Check(QueueRelease143(target,{GetTickCount64(),fixtureLocal143,3,{100.1,101.2,102.3},{-2,3,4}}),"active ABI release ticket");
    const auto calls=releaseCalls146;g_nativeRelease146=reinterpret_cast<NativeRelease146>(ReleaseFixture146);
    volatile UINT canary=0x01234567;CallUpdate146(wrapper);
    Check(releaseCalls146==calls+1&&!ManualStatus143().pending,"relocated update actually executes native release callback before original prologue");
    g_nativeRelease146=nullptr;CancelRelease143();SetIdentityReader143(nullptr);
    Check(canary==0x01234567&&!memcmp(baseline,g_registers,sizeof(baseline))&&((g_flags^flags)&0x401)==0&&
          !memcmp(g_seed,g_sse,sizeof(g_seed))&&g_x87==kX87,"native update retains stack/ret8/integer/CF/DF/SSE/x87 and executes original prologue");
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"native-update detour cleanup");
    memcpy(code,kReleaseBytes146,16);code[16]=0;code[17]=0xff;code[18]=0x05;
    const UINT marker=reinterpret_cast<UINT>(&releaseOriginalEntries146);memcpy(code+19,&marker,4);
    const BYTE releaseTail[]={0x5b,0x5d,0xc2,0x04,0x00};memcpy(code+23,releaseTail,sizeof(releaseTail));FlushInstructionCache(GetCurrentProcess(),code,32);
    Check(MH_CreateHook(code,reinterpret_cast<void*>(ReleaseGate146),&g_releaseOriginal146)==MH_OK&&MH_EnableHook(code)==MH_OK,"exact native release entry prologue relocation");
    auto nativeEntry=reinterpret_cast<NativeRelease146>(code);releaseOriginalEntries146=0;nativeEntry(reinterpret_cast<void*>(g_strategy),wrapper);
    Check(releaseOriginalEntries146==1&&canary==0x01234567,"ordinary native release retains ret4 stack and executes original method");
    g_manualLaunched146=target;nativeEntry(reinterpret_cast<void*>(g_strategy),wrapper);
    Check(releaseOriginalEntries146==1&&canary==0x01234567,"late animation release skips exact manually launched projectile with correct ret4 stack");
    g_manualLaunched146={};Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK&&MH_Uninitialize()==MH_OK,"release/update detour cleanup");g_releaseOriginal146=nullptr;
    g_updateOriginal146=nullptr;VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS: exact seventh native-update MinHook boundary, ret8 stack, integer/CF/DF/SSE/x87 preservation and original prologue.");
}
