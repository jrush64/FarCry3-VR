// Execute the actual displaced instructions with synthetic valid native objects.
// Compare baseline and hooked x86 state; this does not assert live damage accuracy.
static void* g_quickFixtureEntry=nullptr;
static UINT g_quickFixturePage=0;
__declspec(naked) void __cdecl CallQuickHookFixture(UINT variant)
{
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        mov eax,[g_quickFixturePage]
        lea edx,[eax+768]
        push edx
        lea edx,[eax+256]
        push edx
        push eax
        mov ecx,eax
        mov ebx,eax
        lea esi,[eax+1024]
        lea edi,[eax+768]
        lea eax,[eax+512]
        mov edx,[ebp+8]
        lea eax,[eax+edx*4]
        fninit
        fld1
        movups xmm0,[g_nativeFixtureSeed]
        movups xmm1,[g_nativeFixtureSeed]
        movups xmm2,[g_nativeFixtureSeed]
        movups xmm3,[g_nativeFixtureSeed]
        movups xmm4,[g_nativeFixtureSeed]
        movups xmm5,[g_nativeFixtureSeed]
        movups xmm6,[g_nativeFixtureSeed]
        movups xmm7,[g_nativeFixtureSeed]
        cmp dword ptr [ebp+8],0
        jne QuickFixtureSetDirection
        push 203h
        jmp QuickFixtureFlagsReady
    QuickFixtureSetDirection:
        push 603h
    QuickFixtureFlagsReady:
        popfd
        call dword ptr [g_quickFixtureEntry]
        lea esp,[esp+12]
        fxsave [g_routingFixtureFx]
        pushfd
        pushad
        mov esi,esp
        mov edi,offset g_routingFixtureRegisters
        mov ecx,9
        cld
        rep movsd
        popad
        popfd
        fstp st(0)
        pop edi
        pop esi
        pop ebx
        pop ebp
        cld
        ret
    }
}
void TestMacheteQuickProbe()
{
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&code,"quick native fixture allocations");const UINT actor=reinterpret_cast<UINT>(page);
    g_quickFixturePage=actor;
    auto put=[&](UINT offset,UINT value){memcpy(page+offset,&value,4);};
    put(0,actor+3000);put(4,actor+256);put(12,actor+320);put(0x24,actor+256);
    put(128,actor+132);put(140,0);
    put(256+12,actor+320);put(320,actor+3000);
    put(512,0x91323040);put(516,0x99e9d625);put(768,actor+1536);put(772,1);
    put(1024,0x11f8db30);for(UINT i=1;i<64;++i)put(1024+i*4,0x3f800000+i);
    for(UINT i=0;i<36;++i)put(1536+i*4,0x100000+i);
    g_macheteQuick.enabled=true;g_macheteQuick.engine=0x10000000;g_macheteQuick.window=1;
    Check(MH_Initialize()==MH_OK,"quick-contact hook library");
    void* hooks[]={reinterpret_cast<void*>(QuickRequest35),reinterpret_cast<void*>(QuickSelectedSignal),
        reinterpret_cast<void*>(QuickNativeContact),reinterpret_cast<void*>(QuickQueryPose),reinterpret_cast<void*>(QuickQueryBroadResult),
        reinterpret_cast<void*>(QuickContactEntry),reinterpret_cast<void*>(QuickQueryGate),reinterpret_cast<void*>(QuickObjectDispatch),
        reinterpret_cast<void*>(QuickActorDispatch)};
    const UINT lengths[]={6,6,8,8,7,6,6,6,5};
    for(int index=0;index<9;++index)
    {
        memset(code,0xcc,4096);UINT used=0;
        auto bytes=[&](const BYTE* p,UINT n){memcpy(code+used,p,n);used+=n;};
        auto imm=[&](UINT value){memcpy(code+used,&value,4);used+=4;};
        const bool entry=index==0||index==2||index==5;
        if(!entry)
        {
            const BYTE prefix[]={0x55,0x8b,0xec,0x81,0xec,0x00,0x02,0x00,0x00};bytes(prefix,sizeof(prefix));
            for(UINT offset=0;offset<0x180;offset+=4)
            {
                const BYTE store[]={0xc7,0x84,0x24};bytes(store,3);imm(offset);
                imm(offset==0x20?actor:offset==0x18||offset==0x24?actor+1536:offset==0x28?1:0x3f000000);
            }
        }
        BYTE* site=code+used;bytes(kMacheteQuickSignatures[index],lengths[index]);
        // Relocate this real absolute global operand to owned fixture memory.
        if(index==2){const UINT global=actor+128;memcpy(site+4,&global,4);}
        if(index==6)
        {
            const BYTE branch[]={0x74,0x06,0xff,0x05};bytes(branch,sizeof(branch));imm(reinterpret_cast<UINT>(&g_routingFixtureCalls));
        }
        const BYTE tail[]={0x8b,0xe5,0x5d,0xc3};bytes(tail,sizeof(tail));
        g_quickFixtureEntry=code;FlushInstructionCache(GetCurrentProcess(),code,used);
        for(UINT variant=0;variant<2;++variant)
        {
            g_routingFixtureCalls=0;CallQuickHookFixture(variant);
            UINT regs[9];BYTE fx[512];memcpy(regs,g_routingFixtureRegisters,sizeof(regs));memcpy(fx,g_routingFixtureFx,sizeof(fx));
            const UINT calls=g_routingFixtureCalls;
            Check(MH_CreateHook(site,hooks[index],&g_macheteQuickOriginal[index])==MH_OK&&MH_EnableHook(site)==MH_OK,
                "all nine actual displaced quick-melee sequences relocate with MinHook");
            g_routingFixtureCalls=0;g_macheteQuickQueued=0;volatile UINT canary=0x76543210;
            CallQuickHookFixture(variant);
            if(memcmp(regs,g_routingFixtureRegisters,sizeof(regs))||memcmp(fx,g_routingFixtureFx,sizeof(fx))||calls!=g_routingFixtureCalls)
            {
                std::printf("Quick state index%d variant%u dispatch %u/%u\n",index,variant,calls,g_routingFixtureCalls);
                for(int k=0;k<9;++k)if(regs[k]!=g_routingFixtureRegisters[k])std::printf(" reg%d %08x/%08x\n",k,regs[k],g_routingFixtureRegisters[k]);
                for(int k=0;k<512;++k)if(fx[k]!=g_routingFixtureFx[k])std::printf(" fx%d %02x/%02x\n",k,fx[k],g_routingFixtureFx[k]);
            }
            Check(canary==0x76543210&&memcmp(regs,g_routingFixtureRegisters,sizeof(regs))==0&&
                memcmp(fx,g_routingFixtureFx,sizeof(fx))==0&&calls==g_routingFixtureCalls,
                "quick boundaries preserve all registers, ESP, flags, x87/SSE and both original gate outcomes");
            Check(g_macheteQuickQueued==1&&g_macheteQuickQueue[0].kind==static_cast<UINT>(index+1),"quick phase recorded");
            if(index==1)Check(g_macheteQuickQueue[0].gate==(variant?0x99e9d625:0x91323040),"selected swing CRC captured before stock signal send");
            if(index==4)Check(g_macheteQuickQueue[0].event[2]==1&&g_macheteQuickQueue[0].payload[0]==0x100000,
                "broad-phase contact count and owned contact rows captured");
            if(index==6)Check(g_macheteQuickQueue[0].gate==(variant?4u:0u),"native query gate is observed and remains unmodified");
            Check(MH_DisableHook(site)==MH_OK&&MH_RemoveHook(site)==MH_OK,"quick fixture hook cleanup");
        }
    }
    Check(MH_Uninitialize()==MH_OK,"quick hook library cleanup");
    // Verify payload extraction independently from execution and report unreadable memory.
    UINT saved[9]={};const UINT stack=actor+2048,frame=actor+2304;
    saved[3]=stack-4;saved[2]=frame;saved[4]=actor;saved[7]=actor+512;
    put(2304+8,actor);put(2304+12,actor+256);put(2304+16,actor+768);
    for(UINT i=0;i<16;++i)put(2048+0x110+i*4,i==0?0x3f800000:0x3f000000);
    put(2048+0x5c,0x3f800000);put(2048+0x48,0x40000000);
    const std::vector<BYTE> unchanged(page,page+4096);
    g_macheteQuickQueued=0;ObserveMacheteQuick(4,saved);
    Check((g_macheteQuickQueue[0].flags&32)&&g_macheteQuickQueue[0].payload[0]==0x3f800000&&
        g_macheteQuickQueue[0].payload[3]==0x40000000&&g_macheteQuickQueue[0].payload[6]==0x3f800000&&
        memcmp(unchanged.data(),page,4096)==0,"native query origin, endpoint and attachment matrix copied read-only");
    saved[0]=actor+768;put(768,1);g_macheteQuickQueued=0;ObserveMacheteQuick(5,saved);
    Check(g_macheteQuickQueue[0].flags&128,"unreadable contact storage is explicit");
    put(772,0xffffffff);g_macheteQuickQueued=0;ObserveMacheteQuick(5,saved);
    Check(g_macheteQuickQueue[0].flags&128,"corrupt contact count refuses unsafe sampling");
    const LONG old=g_macheteQuick.raw[4];g_macheteQuick.window=0;g_macheteQuickQueued=0;ObserveMacheteQuick(4,saved);
    Check(g_macheteQuick.raw[4]==old+1&&g_macheteQuickQueued==0,"raw coverage outside held window remains distinct from player attribution");
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x1300000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(image!=nullptr,"quick signature image");
    for(int i=0;i<9;++i)memcpy(image+kMacheteQuickSites[i],kMacheteQuickSignatures[i],16);
    // Reproduce the actual loader's HIGHLOW fixup independently from guard code.
    const UINT relocatedGlobal=reinterpret_cast<UINT>(image)+0x2829528u;
    memcpy(image+0x12d8574,&relocatedGlobal,4);
    Check(MacheteQuickSignaturesMatch(image),"all nine exact signatures accept independently relocated image");
    image[0x12d8574]^=1;
    Check(MacheteQuickSignatureMismatch(image)==3,"wrong relocated global address is rejected precisely");
    image[0x12d8574]^=1;
    image[0x12d8573]^=1;
    Check(MacheteQuickSignatureMismatch(image)==3,"changed native global-load opcode is rejected");
    image[0x12d8573]^=1;
    const UINT preferredGlobal=0x12829528u;
    memcpy(image+0x12d8574,&preferredGlobal,4);
    Check(MacheteQuickSignatureMismatch(image)==3,"unrelocated preferred address in relocated image is rejected");
    memcpy(image+0x12d8574,&relocatedGlobal,4);
    image[kMacheteQuickSites[8]]^=1;Check(!MacheteQuickSignaturesMatch(image),"altered actor-dispatch site refuses installation");
    g_macheteQuickQueued=0;g_macheteQuick.lost=0;MacheteQuickRecord zero;zero.installed=1;
    for(int i=0;i<129;++i)QueueMacheteQuick(zero);
    Check(g_macheteQuickQueued==128&&g_macheteQuick.lost==1,"quick queue overflow is explicit");
    g_macheteQuickQueued=0;QueueMacheteQuick(zero);FlushMacheteQuickProbe();CloseMacheteQuickProbe();
    HANDLE file=CreateFileW(g_macheteQuick.path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(file!=INVALID_HANDLE_VALUE&&GetFileSize(file,nullptr)==732,"quick zero-activity status is durably saved");
    MacheteQuickRecord disk;DWORD got=0;
    Check(ReadFile(file,&disk,sizeof(disk),&got,nullptr)&&got==732&&disk.magic==0x514d5031&&disk.installed==1,"actual QMP1 file readback");
    Check(CopyFileW(g_macheteQuick.path,L"machete_quick_fixture.bin",FALSE),"retain actual QMP1 fixture for decoder");
    g_macheteQuick.file=file;QueueMacheteQuick(zero);FlushMacheteQuickProbe();
    Check(g_macheteQuick.error==ERROR_ACCESS_DENIED&&g_macheteQuick.file==INVALID_HANDLE_VALUE,"quick real I/O failure is reported");
    DeleteFileW(g_macheteQuick.path);
    g_macheteQuick.enabled=false;g_macheteQuick.window=0;g_macheteQuickQueued=0;
    VirtualFree(image,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: nine real quick-contact MinHook boundaries, both query gate outcomes, complete x86/flags/x87/SSE equivalence; read-only query/contact payloads, guards, bad pointers/counts, zero-event durable QMP1, queue overflow and I/O failure");
}
