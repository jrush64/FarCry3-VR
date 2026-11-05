struct AmmoOwnerFixture { HANDLE begin=nullptr,changed=nullptr,finish=nullptr; volatile UINT* words=nullptr; };
DWORD WINAPI AmmoOwnerFixtureWriter(LPVOID opaque)
{
    auto* fixture=static_cast<AmmoOwnerFixture*>(opaque);WaitForSingleObject(fixture->begin,5000);
    fixture->words[0]=4;fixture->words[3]=996;fixture->words[0]=5;fixture->words[3]=995;fixture->words[0]=4;
    SetEvent(fixture->changed);WaitForSingleObject(fixture->finish,5000);return 0;
}
void TestAmmoOwner()
{
    g_ammoOwner=AmmoOwnerState{};
    auto* page=static_cast<UINT*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(page!=nullptr,"owner fixture allocation");
    const UINT shape[12]={3,7,0x3e99999a,997,1000,0,0,2,1,5,0x40000000,0x40000000};memcpy(page,shape,sizeof(shape));
    Check(AmmoOwnerMatch(page),"captured staged loaded/reserve signature matches");page[4]=999;Check(!AmmoOwnerMatch(page),"nonconserved or transient layout rejected");page[4]=1000;
    // Discovery uses actual process memory and excludes its own module and stack.
    AmmoOwnerDiscover();Check(g_ammoOwner.count==1&&g_ammoOwner.targets[0].address==reinterpret_cast<UINT>(page),"focused scanner relocates unique heap pair");
    g_ammoOwner=AmmoOwnerState{};g_ammoOwner.targets[0]={reinterpret_cast<UINT>(page),3,997};g_ammoOwner.loaded=3;g_ammoOwner.reserve=997;g_ammoOwner.begun=GetTickCount64();
    g_ammoOwner.handler=AddVectoredExceptionHandler(1,AmmoOwnerException);Check(g_ammoOwner.handler!=nullptr,"real vectored handler registered");
    AmmoOwnerFixture fixture={CreateEventW(nullptr,TRUE,FALSE,nullptr),CreateEventW(nullptr,TRUE,FALSE,nullptr),CreateEventW(nullptr,TRUE,FALSE,nullptr),page};
    auto& watched=g_ammoOwner.threads[0];watched.handle=CreateThread(nullptr,0,AmmoOwnerFixtureWriter,&fixture,0,&watched.id);Check(watched.handle!=nullptr,"real writer thread");
    InterlockedExchange(&g_ammoOwner.threadCount,1);
    // Preserve an existing breakpoint in slot2; the test's pair must use free slots.
    Check(SuspendThread(watched.handle)!=static_cast<DWORD>(-1),"suspend fixture");CONTEXT original={};original.ContextFlags=CONTEXT_DEBUG_REGISTERS;Check(GetThreadContext(watched.handle,&original)!=FALSE,"read debug registers");
    original.Dr2=reinterpret_cast<UINT>(page+64);original.Dr7|=1u<<4;Check(SetThreadContext(watched.handle,&original)!=FALSE,"fixture existing slot2");ResumeThread(watched.handle);
    Check(AmmoOwnerArm(watched,reinterpret_cast<UINT>(page)),"install real loaded/reserve write watchpoints");Check(watched.slots[0]!=2&&watched.slots[1]!=2,"existing debug slot preserved");
    SetEvent(fixture.begin);Check(WaitForSingleObject(fixture.changed,5000)==WAIT_OBJECT_0,"writes complete without blocking/crashing");
    if(g_ammoOwner.used!=5)printf("362 observer diagnostics: actual write receipts=%ld expected=5\n",g_ammoOwner.used);
    Check(g_ammoOwner.used==5,"five actual ammo writes captured exactly once");
    const UINT loaded[5]={4,4,5,5,4},reserve[5]={997,996,996,995,995};
    for(UINT i=0;i<5;++i)
    {const auto& e=g_ammoOwner.events[i];Check(e.ready&&e.thread==watched.id&&e.loaded==loaded[i]&&e.reserve==reserve[i]&&e.eip&&e.codeBase==e.eip-32,"writer registers/code and individual loaded/reserve changes captured");}
    Check(page[0]==4&&page[3]==995&&page[1]==7,"probe never alters writer's result or capacity");
    Check(AmmoOwnerDisarm(watched,reinterpret_cast<UINT>(page)),"real watchpoint cleanup");
    Check(SuspendThread(watched.handle)!=static_cast<DWORD>(-1),"verify cleanup suspension");CONTEXT restored={};restored.ContextFlags=CONTEXT_DEBUG_REGISTERS;Check(GetThreadContext(watched.handle,&restored)!=FALSE,"verify restored debug context");
    Check(restored.Dr0==original.Dr0&&restored.Dr1==original.Dr1&&restored.Dr2==original.Dr2&&restored.Dr3==original.Dr3&&restored.Dr7==original.Dr7,"original four debug addresses/control restored exactly");
    original.Dr7&=~(3u<<4);SetThreadContext(watched.handle,&original);ResumeThread(watched.handle);
    SetEvent(fixture.finish);WaitForSingleObject(watched.handle,5000);CloseHandle(watched.handle);CloseHandle(fixture.begin);CloseHandle(fixture.changed);CloseHandle(fixture.finish);
    RemoveVectoredExceptionHandler(g_ammoOwner.handler);VirtualFree(page,0,MEM_RELEASE);g_ammoOwner=AmmoOwnerState{};
    g_ammoOwner.count=0;InterlockedExchange(&g_ammoOwner.worker,2);TickAmmoOwnerProbe(false);
    Check(!g_ammoOwner.done&&!g_ammoOwner.worker&&!g_ammoOwner.quiet,"missing startup object retries discovery after quiet interval");
    g_ammoOwner.count=5;InterlockedExchange(&g_ammoOwner.worker,2);TickAmmoOwnerProbe(false);
    Check(!g_ammoOwner.done&&!g_ammoOwner.worker&&!g_ammoOwner.count,"ambiguous object never selects an arbitrary counter");
    g_ammoOwner.count=1;InterlockedExchange(&g_ammoOwner.worker,2);TickAmmoOwnerProbe(false);
    Check(g_ammoOwner.done,"completed writer capture is retained");g_ammoOwner=AmmoOwnerState{};
    AmmoOwnerObserved(0x2000,4,996,2);Check(g_ammoOwner.observedCommits==0&&!g_ammoOwner.worker,"ambiguous transfer does not arm writer trace");
    AmmoOwnerObserved(0x2000,4,996,1);Check(g_ammoOwner.observedCommits==1&&!g_ammoOwner.worker,"one conserved insertion insufficient to arm");
    AmmoOwnerObserved(0x3000,5,995,1);Check(g_ammoOwner.observedCommits==1&&!g_ammoOwner.worker,"different counter cannot inherit prior confirmation");
    AmmoOwnerObserved(0x3000,6,994,1);Check(g_ammoOwner.count==1&&g_ammoOwner.targets[0].address==0x3000&&g_ammoOwner.worker==2,"two unique live transfers select relocated target");g_ammoOwner=AmmoOwnerState{};
    std::puts("PASS: actual x86 writer instruction capture; existing debug slots and cleanup; live two-insertion selection and ambiguity refusal; no source mutation");
}
