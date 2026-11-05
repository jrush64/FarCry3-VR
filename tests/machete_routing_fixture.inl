static void* g_routingFixtureEntry=nullptr;
static UINT g_routingFixtureRegisters[9]={},g_routingFixtureCalls=0,g_routingFixtureEvent=0;
__declspec(align(16)) static BYTE g_routingFixtureFx[512]={};
__declspec(naked) void RoutingFixtureDispatch()
{
    __asm {
        inc [g_routingFixtureCalls]
        mov eax,[esp+4]
        mov [g_routingFixtureEvent],eax
        ret 4
    }
}
__declspec(naked) void __cdecl CallRoutingHookFixture(UINT strategy,UINT gate)
{
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        push edi
        push dword ptr [ebp+8]
        mov eax,[ebp+12]
        mov ecx,[ebp+8]
        mov edx,33333333h
        mov ebx,44444444h
        mov esi,66666666h
        mov edi,77777777h
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
        stc
        call dword ptr [g_routingFixtureEntry]
        fxsave [g_routingFixtureFx]
        pushfd
        pushad
        mov esi,esp
        mov edi,offset g_routingFixtureRegisters
        mov ecx,9
        rep movsd
        popad
        popfd
        fstp st(0)
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
void TestMacheteRoutingProbe()
{
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&code,"routing fixture data and code");const UINT strategy=reinterpret_cast<UINT>(page);
    *reinterpret_cast<UINT*>(page)=0x11fba818;
    g_macheteRouting.enabled=true;g_macheteRouting.engine=0x10000000;
    g_macheteRoutingQueued=0;g_macheteRouting.window=1;
    g_macheteNative.enabled=true;g_macheteNative.engine=0x10000000;g_macheteNativeQueued=0;
    const UINT stack=strategy+512;*reinterpret_cast<UINT*>(stack+4)=strategy;
    UINT saved[9]={};saved[3]=stack-4;
    *reinterpret_cast<UINT*>(page)=0x11fba8b8;ObserveMacheteNative(1,saved);
    Check(g_macheteRouting.raw[1]==1&&g_macheteNativeQueued==0&&g_macheteRoutingQueued==1&&
        g_macheteRoutingQueue[0].strategyVtable==0x11fba8b8&&!(g_macheteRoutingQueue[0].flags&4),
        "raw helper coverage distinguishes an invoked but filtered class from zero calls");
    *reinterpret_cast<UINT*>(page)=0x11fba818;ObserveMacheteNative(1,saved);
    Check(g_macheteRouting.raw[1]==2&&g_macheteNativeQueued==1&&(g_macheteRoutingQueue[1].flags&4),
        "MNP1 class filtering and records remain unchanged");
    g_macheteNativeQueued=0;g_macheteRoutingQueued=0;
    g_macheteRouting.window=0;ObserveMacheteRouting(5,saved);
    Check(g_macheteRouting.raw[5]==1&&g_macheteRoutingQueued==0,"outside held window counts without detailed AI traffic");
    ObserveMacheteRouting(10,saved);ObserveMacheteRouting(5,nullptr);
    Check(g_macheteRouting.raw[5]==1,"invalid boundaries and null state rejected");
    g_machete.held=true;g_macheteMelee.sequence=321;g_macheteMelee.pending=true;
    g_frame=1234;g_armPoseEpoch=7;g_handBody[0][3]=.125f;g_handRight[3]=.25f;
    g_machete.handFrame=g_frame;g_machete.hand.m[3]=.375f;g_frameCameraValid=true;
    SnapshotMacheteRouting();MacheteRoutingRecord snapshot;MacheteRoutingEvidence(snapshot);
    Check(snapshot.frame==1234&&snapshot.sequence==321&&snapshot.pose.epoch==7&&
        snapshot.pose.controllerBody[3]==.125f&&snapshot.pose.controllerCamera[3]==.25f&&
        snapshot.pose.hand[3]==.375f&&(snapshot.poseFlags&(1|2|8|16))==(1|2|8|16),
        "render frame, epoch, held/request state and three distinct transforms are copied together");
    *reinterpret_cast<UINT*>(page+0x48)=strategy+1024;
    *reinterpret_cast<UINT*>(page+1024+0x28)=strategy+1280;
    *reinterpret_cast<UINT*>(page+1024+4)=strategy+1536;
    *reinterpret_cast<UINT*>(page+1280+0xc)=strategy+1792;
    *reinterpret_cast<UINT*>(page+1536+0xc)=strategy+2048;
    const std::vector<BYTE> unchanged(page,page+4096);saved[0]=strategy;saved[7]=1;
    g_macheteRoutingQueued=0;ObserveMacheteRouting(6,saved);
    Check(g_macheteRoutingQueue[0].event[0]==strategy+1024&&g_macheteRoutingQueue[0].event[1]==strategy+1792&&
        g_macheteRoutingQueue[0].event[2]==strategy+2048&&memcmp(unchanged.data(),page,4096)==0,
        "native gate's opaque actor/entity references are recorded without any source mutation");
    AcquireSRWLockExclusive(&g_macheteRoutingPoseLock);MacheteRoutingRecord busy;MacheteRoutingEvidence(busy);
    ReleaseSRWLockExclusive(&g_macheteRoutingPoseLock);
    Check(!(busy.flags&16),"native probe marks unavailable snapshot instead of blocking render or inventing a pose");
    Check(MH_Initialize()==MH_OK,"routing hook library");
    void* hooks[]={reinterpret_cast<void*>(MacheteCallbackEntry),reinterpret_cast<void*>(MacheteAuthorityReturn),
        reinterpret_cast<void*>(MacheteMeleeEvent),reinterpret_cast<void*>(MacheteBeginEvent),reinterpret_cast<void*>(MacheteEndEvent)};
    for(int index=0;index<5;++index)
    {
        memset(code,0xcc,4096);UINT used=0;
        auto bytes=[&](const BYTE* p,UINT n){memcpy(code+used,p,n);used+=n;};
        auto imm=[&](UINT value){memcpy(code+used,&value,4);used+=4;};
        BYTE* site=nullptr;
        if(index==0)
        {
            site=code;bytes(kMacheteRoutingSignatures[0],8);
            const BYTE tail[]={0x5f,0x5e,0x8b,0xe5,0x5d,0xc2,0x04,0x00};bytes(tail,sizeof(tail));
        }
        else if(index==1)
        {
            const BYTE prefix[]={0x55,0x8b,0xec,0x56,0x57,0x8b,0xf9};bytes(prefix,sizeof(prefix));site=code+used;
            bytes(kMacheteRoutingSignatures[1],16);
            const BYTE tail[]={0x5d,0xc2,0x04,0x00};bytes(tail,sizeof(tail));
            const UINT callee=reinterpret_cast<UINT>(RoutingFixtureDispatch);
            const UINT relative=callee-reinterpret_cast<UINT>(site+10);memcpy(site+6,&relative,4);
        }
        else
        {
            const BYTE prefix[]={0x55,0x8b,0xec,0x83,0xec,0x28};bytes(prefix,sizeof(prefix));
            if(index>2){const BYTE regs[]={0x56,0x57};bytes(regs,2);}
            for(UINT word=0;word<(index==2?7u:8u);++word)
            {
                const UINT value=word==0?(index==2?0x11fb141c:0x12015214):word==6&&index>2?(index==3?1u:0u):0x55550000+word;
                if(index==2){const BYTE store[]={0xc7,0x45,static_cast<BYTE>(0xe4+word*4)};bytes(store,3);}
                else{const BYTE store[]={0xc7,0x44,0x24,static_cast<BYTE>(8+word*4)};bytes(store,4);}
                imm(value);
            }
            const BYTE dispatcher[]={0xba};bytes(dispatcher,1);imm(reinterpret_cast<UINT>(RoutingFixtureDispatch));
            site=code+used;bytes(kMacheteRoutingSignatures[index],index==2?6:7);
            if(index>2){const BYTE regs[]={0x5f,0x5e};bytes(regs,2);}
            const BYTE tail[]={0x8b,0xe5,0x5d,0xc2,0x04,0x00};bytes(tail,sizeof(tail));
        }
        FlushInstructionCache(GetCurrentProcess(),code,used);g_routingFixtureEntry=code;
        for(UINT gate=0;gate<2;++gate)
        {
            g_routingFixtureCalls=0;CallRoutingHookFixture(strategy,gate);
            UINT expectedRegs[9];BYTE expectedFx[512];memcpy(expectedRegs,g_routingFixtureRegisters,sizeof(expectedRegs));
            memcpy(expectedFx,g_routingFixtureFx,sizeof(expectedFx));const UINT expectedCalls=g_routingFixtureCalls;
            Check(MH_CreateHook(site,hooks[index],&g_macheteRoutingOriginal[index])==MH_OK&&MH_EnableHook(site)==MH_OK,
                "relocate and enable actual callback, conditional gate and stack-relative event dispatch bytes");
            g_routingFixtureCalls=0;g_macheteRoutingQueued=0;
            volatile UINT canary=0x12345678;CallRoutingHookFixture(strategy,gate);
            Check(canary==0x12345678&&memcmp(expectedRegs,g_routingFixtureRegisters,sizeof(expectedRegs))==0&&
                memcmp(expectedFx,g_routingFixtureFx,sizeof(expectedFx))==0&&g_routingFixtureCalls==expectedCalls,
                "hooked and unhooked native paths agree for every integer/ESP/EFLAGS register, x87/SSE, both branch outcomes and dispatch count");
            Check(g_macheteRoutingQueued==1&&g_macheteRoutingQueue[0].kind==static_cast<UINT>(index+5),"all five native bridges deliver the correct phase");
            const auto& r=g_macheteRoutingQueue[0];
            if(index==1)Check(r.gate==gate&&r.strategy==strategy,"local-authority result recorded without changing the original conditional branch");
            if(index>1)Check((r.flags&(8|32))==(8|32)&&r.receiver==strategy&&r.event[6]==(index==3?1u:index==4?0u:0x55550006),
                "native receiver, full event bytes and machete begin/end payload survive stack reads");
            Check(MH_DisableHook(site)==MH_OK&&MH_RemoveHook(site)==MH_OK,"routing fixture hook cleanup");
        }
    }
    Check(MH_Uninitialize()==MH_OK,"routing library cleanup");
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x1250000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(image!=nullptr,"routing signature image");
    for(int i=0;i<5;++i)memcpy(image+kMacheteRoutingSites[i],kMacheteRoutingSignatures[i],16);
    Check(MacheteRoutingSignaturesMatch(image),"all five exact engine site guards match");
    image[kMacheteRoutingSites[4]]^=1;Check(!MacheteRoutingSignaturesMatch(image),"altered end site refuses installation");
    g_macheteRoutingQueued=0;g_macheteRouting.lost=0;
    for(int i=0;i<129;++i)QueueMacheteRouting(snapshot);
    Check(g_macheteRoutingQueued==128&&g_macheteRouting.lost==1,"bounded native queue reports loss");
    g_macheteRoutingQueued=0;
    for(int i=0;i<10;++i)g_macheteRouting.raw[i]=0;
    for(int i=0;i<5;++i)g_macheteNative.phases[i]=0;
    MacheteRoutingRecord zero;MacheteRoutingEvidence(zero);zero.installed=3;QueueMacheteRouting(zero);
    FlushMacheteRoutingProbe();CloseMacheteRoutingProbe();
    HANDLE file=CreateFileW(g_macheteRouting.path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(file!=INVALID_HANDLE_VALUE&&GetFileSize(file,nullptr)==sizeof(zero),"automatic zero-activity coverage file exists and is durable");
    MacheteRoutingRecord disk;DWORD got=0;
    Check(ReadFile(file,&disk,sizeof(disk),&got,nullptr)&&got==sizeof(disk)&&disk.magic==0x4d525431&&disk.kind==0&&
        disk.installed==3&&disk.raw[1]==0&&disk.raw[5]==0&&disk.pose.epoch==7,"zero calls, installation and coherent pose survive real file readback");
    Check(CopyFileW(g_macheteRouting.path,L"machete_routing_fixture.bin",FALSE),"retain actual zero-call capture for the Python decoder");
    // A handle that cannot be written must produce an explicit error and loss.
    g_macheteRouting.file=file;g_macheteRouting.full=false;QueueMacheteRouting(zero);FlushMacheteRoutingProbe();
    Check(g_macheteRouting.error==ERROR_ACCESS_DENIED&&g_macheteRouting.file==INVALID_HANDLE_VALUE&&g_macheteRouting.lost==2,
        "actual file write failure records error and loss instead of reporting successful capture");
    DeleteFileW(g_macheteRouting.path);
    g_macheteRouting.enabled=false;g_macheteRouting.window=0;g_macheteRoutingQueued=0;
    g_macheteNative.enabled=false;g_macheteNative.engine=0;g_macheteNativeQueued=0;
    g_machete.held=false;g_macheteMelee.pending=false;g_macheteMelee.sequence=0;g_routingFixtureEntry=nullptr;
    VirtualFree(image,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);VirtualFree(page,0,MEM_RELEASE);
    std::puts("PASS: routing coverage before class filter, coherent nonblocking pose snapshot, all five real MinHook boundaries and both authority branches; exact register/ESP/flags/x87/SSE/dispatch equivalence; zero-event durable file, queue loss and actual I/O failure");
}
