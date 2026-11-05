static UINT g_nativeFixtureRegisters[4]={};static float g_nativeFixtureSse[4]={};
static const float g_nativeFixtureSeed[4]={1.25f,-2.5f,3.75f,-4.f};
static UINT g_nativeFixtureFlags=0;
__declspec(naked) void NativeFixtureReturn()
{
    __asm {
        mov [g_nativeFixtureRegisters],eax
        mov [g_nativeFixtureRegisters+4],ecx
        mov [g_nativeFixtureRegisters+8],edx
        mov [g_nativeFixtureRegisters+12],ebx
        movups [g_nativeFixtureSse],xmm0
        pushfd
        pop eax
        mov [g_nativeFixtureFlags],eax
        ret 4
    }
}
__declspec(naked) void __cdecl CallNativeFixture(UINT strategy)
{
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push dword ptr [ebp+8]
        mov eax,11111111h
        mov ecx,22222222h
        mov edx,33333333h
        mov ebx,44444444h
        movups xmm0,[g_nativeFixtureSeed]
        stc
        call MacheteImpactEntry
        pop ebx
        pop ebp
        ret
    }
}
static void* g_nativeFixtureEntry=nullptr;
__declspec(naked) void __cdecl CallNativeHookFixture(UINT strategy)
{
    __asm {
        push ebp
        mov ebp,esp
        push dword ptr [ebp+8]
        movups xmm0,[g_nativeFixtureSeed]
        call dword ptr [g_nativeFixtureEntry]
        movups [g_nativeFixtureSse],xmm0
        pop ebp
        ret
    }
}
void TestMacheteNativeProbe()
{
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,16384,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(page!=nullptr,"native melee fixture memory");const UINT strategy=reinterpret_cast<UINT>(page);
    const UINT wrapper=strategy+1024,ref=strategy+2048,actor=strategy+3072,entity=strategy+4096;
    const UINT stack=strategy+6144,bp=strategy+8192,hit=strategy+10240,target=strategy+12288;
    *reinterpret_cast<UINT*>(strategy)=0x11fba818;*reinterpret_cast<UINT*>(strategy+0x48)=wrapper;
    *reinterpret_cast<UINT*>(wrapper+4)=ref;*reinterpret_cast<UINT*>(wrapper+0x28)=ref+128;
    *reinterpret_cast<UINT*>(ref+0xc)=entity;*reinterpret_cast<UINT*>(ref+128+0xc)=actor;
    *reinterpret_cast<float*>(entity+0x30)=.25f;*reinterpret_cast<float*>(entity+0x50)=123.f;
    *reinterpret_cast<UINT*>(stack+4)=strategy;*reinterpret_cast<UINT*>(bp+8)=strategy;
    *reinterpret_cast<UINT*>(stack+0x14)=hit;*reinterpret_cast<UINT*>(stack+0x18)=2;
    *reinterpret_cast<UINT*>(hit+0x18)=0x13579abc;
    *reinterpret_cast<float*>(stack+0xf0+0x4c)=1.75f;
    std::vector<BYTE> before(page,page+16384);
    g_macheteNative.enabled=true;g_macheteNative.engine=0x10000000;
    g_macheteNativeQueued=0;g_macheteMelee.sequence=99;
    g_macheteNativeOriginal[0]=reinterpret_cast<void*>(NativeFixtureReturn);
    volatile UINT canary=0x76543210;CallNativeFixture(strategy);
    Check(canary==0x76543210&&g_nativeFixtureRegisters[0]==0x11111111&&g_nativeFixtureRegisters[1]==0x22222222&&
          g_nativeFixtureRegisters[2]==0x33333333&&g_nativeFixtureRegisters[3]==0x44444444&&
          (g_nativeFixtureFlags&1),"native impact bridge preserves stack, integer registers and EFLAGS");
    Compare(g_nativeFixtureSeed,g_nativeFixtureSse,4,"native impact bridge restores deliberately clobbered SSE");
    UINT saved[9]={};saved[2]=bp;saved[3]=stack-4;saved[6]=target;
    ObserveMacheteNative(2,saved);ObserveMacheteNative(3,saved);ObserveMacheteNative(4,saved);
    Check(g_macheteNativeQueued==4&&g_macheteNativeQueue[0].strategy==strategy&&
          g_macheteNativeQueue[0].actor==actor&&g_macheteNativeQueue[0].nativeEntity==entity&&
          g_macheteNativeQueue[1].hitCount==2&&g_macheteNativeQueue[1].hit[6]==0x13579abc&&
          g_macheteNativeQueue[2].target==target&&g_macheteNativeQueue[2].event[19]==0x3fe00000&&
          !(g_macheteNativeQueue[3].flags&(16|32|64)),"native entry/query/dispatch/end evidence; no end-stage dangling scratch reads");
    Check(std::memcmp(before.data(),page,16384)==0,"native probe leaves all engine-style strategy/target/event/ammo bytes unchanged");
    *reinterpret_cast<UINT*>(strategy)=0;ObserveMacheteNative(2,saved);
    Check(g_macheteNativeQueued==4,"unrelated strategy ignored");*reinterpret_cast<UINT*>(strategy)=0x11fba818;
    FlushMacheteNativeProbe();CloseMacheteNativeProbe();
    HANDLE file=CreateFileW(g_macheteNative.path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(file!=INVALID_HANDLE_VALUE&&GetFileSize(file,nullptr)==4*sizeof(MacheteNativeRecord),"automatic durable native records written");
    MacheteNativeRecord disk[4];DWORD got=0;
    Check(ReadFile(file,disk,sizeof(disk),&got,nullptr)&&got==sizeof(disk)&&disk[1].phase==2&&disk[1].hitCount==2&&
          disk[2].phase==3&&disk[2].event[19]==0x3fe00000,"query and dispatched damage level survive the actual file");
    CloseHandle(file);DeleteFileW(g_macheteNative.path);
    // Real MinHook relocation of the exact native entry's aligned-stack prolog.
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(code!=nullptr,"native hook code memory");memcpy(code,kMacheteNativeSignatures[0],16);
    const BYTE tail[]={0x5b,0x8b,0xe5,0x5d,0xc2,0x04,0x00};memcpy(code+16,tail,sizeof(tail));
    FlushInstructionCache(GetCurrentProcess(),code,23);
    const auto init=MH_Initialize();Check(init==MH_OK||init==MH_ERROR_ALREADY_INITIALIZED,"native melee hook library");
    Check(MH_CreateHook(code,reinterpret_cast<void*>(MacheteImpactEntry),&g_macheteNativeOriginal[0])==MH_OK&&
          MH_EnableHook(code)==MH_OK,"relocate and enable actual native aligned-stack entry prolog");
    g_nativeFixtureEntry=code;canary=0xabcdef12;CallNativeHookFixture(strategy);
    Check(canary==0xabcdef12&&g_macheteNativeQueued==1,"real detour forwards one observation and original ret4 ABI");
    Compare(g_nativeFixtureSeed,g_nativeFixtureSse,4,"native trampoline preserves SSE");
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK,"native melee fixture hook cleanup");
    if(init==MH_OK)Check(MH_Uninitialize()==MH_OK,"native melee library cleanup");
    // Exercise every new mid-function site, including the actual end epilogue.
    Check(MH_Initialize()==MH_OK,"mid-function probe hook library");
    void* bridges[]={reinterpret_cast<void*>(MacheteImpactQuery),reinterpret_cast<void*>(MacheteImpactDispatch),reinterpret_cast<void*>(MacheteImpactEnd)};
    for(int i=1;i<4;++i)
    {
        memset(code,0xcc,4096);BYTE* site=code;
        if(i==3)
        {
            const BYTE prolog[]={0x55,0x8b,0xec,0x53,0x56,0x57};memcpy(code,prolog,6);site=code+6;
            memcpy(site,kMacheteNativeSignatures[i],16);
        }
        else
        {
            const UINT prefix=i==1?5:7;memcpy(code,kMacheteNativeSignatures[i],prefix);
            const BYTE ret[]={0xc2,0x04,0x00};memcpy(code+prefix,ret,3);
        }
        FlushInstructionCache(GetCurrentProcess(),code,32);g_macheteNativeQueued=0;
        Check(MH_CreateHook(site,bridges[i-1],&g_macheteNativeOriginal[i])==MH_OK&&MH_EnableHook(site)==MH_OK,
              "native cmp, stack-relative lea and pop/mov-ESP epilogue detours relocate");
        g_nativeFixtureEntry=code;canary=0xabcdef12;CallNativeHookFixture(strategy);
        Check(canary==0xabcdef12&&g_macheteNativeQueued==1&&g_macheteNativeQueue[0].phase==static_cast<UINT>(i+1),
              "all actual mid-function trampolines retain stack and deliver one correctly identified phase");
        Compare(g_nativeFixtureSeed,g_nativeFixtureSse,4,"all native mid-function probes preserve SSE");
        Check(MH_DisableHook(site)==MH_OK&&MH_RemoveHook(site)==MH_OK,"mid-function native hook cleanup");
        g_macheteNativeOriginal[i]=nullptr;
    }
    Check(MH_Uninitialize()==MH_OK,"mid-function hook library cleanup");
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x1250000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(image!=nullptr,"native signature image fixture");
    for(int i=0;i<4;++i)memcpy(image+kMacheteNativeSites[i],kMacheteNativeSignatures[i],16);
    Check(MacheteNativeSignaturesMatch(image),"all four exact engine signatures match");
    image[kMacheteNativeSites[2]]^=1;Check(!MacheteNativeSignaturesMatch(image),"changed dispatch instructions refuse the entire probe installation");
    g_macheteNative.enabled=false;g_macheteNative.engine=0;g_macheteNativeQueued=0;
    g_macheteNativeOriginal[0]=nullptr;g_nativeFixtureEntry=nullptr;g_macheteMelee.sequence=0;
    VirtualFree(image,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: native machete impact probe preserves stack/registers/flags/SSE; all four actual entry/cmp/stack-LEA/epilogue MinHook sites and ret4; read-only query/target/dispatched level and durable files; excludes unrelated strategies and stale end references; exact signature guard");
}
