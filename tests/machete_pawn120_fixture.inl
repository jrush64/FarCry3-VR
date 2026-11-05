UINT g_pawnEntity120=0,g_pawnDescriptor120[8]={},g_pawnHelperCalls120=0;bool g_pawnExists120=true,g_pawnBlocked120=false;
V3 g_pawnWrittenPoint120={};float g_pawnWrittenDamage120=0;
UINT __cdecl PawnDescriptorFixture120(){return reinterpret_cast<UINT>(g_pawnDescriptor120);}
UINT __cdecl PawnLookupCheck120(UINT entity,UINT descriptor)
{
    Check(entity==g_pawnEntity120&&descriptor==reinterpret_cast<UINT>(g_pawnDescriptor120)+0x14,"CPawn lookup uses native EAX entity and descriptor+14 stack argument");
    return g_pawnExists120?0x1234:0;
}
__declspec(naked) UINT PawnLookupFixture120()
{
    __asm push dword ptr [esp+4]
    __asm push eax
    __asm call PawnLookupCheck120
    __asm add esp,8
    __asm ret 4
}
bool __cdecl PawnClassifierFixture120(UINT wrapper)
{
    InterlockedDecrement(reinterpret_cast<volatile LONG*>(wrapper+8));return false;
}
void __cdecl PawnHelperCheck120(UINT strategy,UINT wrapper,UINT config,UINT contact)
{
    ++g_pawnHelperCalls120;Check(strategy==g_macheteBladeContact.strategy&&config==g_macheteBladeContact.callConfig118,"pawn damage helper receives the latched controller and private config");
    float xyz[3];Check(ReloadTraceRead(contact+0x14,xyz,12)&&fabs(xyz[0]-4.25f)<1e-6,"pawn damage helper receives real physical contact via EAX");
    if(!g_pawnBlocked120){
        BYTE event[0xc0]={};const UINT vtable=g_macheteBladeContact.engine+0x1f55bcc;memcpy(event,&vtable,4);
        memcpy(event+0x48,reinterpret_cast<void*>(config+8),4);memcpy(event+0x4c,reinterpret_cast<void*>(config+8),4);
        UINT frame[8]={},saved[8]={};frame[3]=wrapper;saved[2]=reinterpret_cast<UINT>(frame);saved[1]=reinterpret_cast<UINT>(event);
        Check(MacheteBladeObserveQuick(8,saved,0),"existing native event hook intercepts physical pawn event");
        float hit[3];memcpy(hit,event+0x18,12);g_pawnWrittenPoint120={hit[0],hit[1],hit[2]};memcpy(&g_pawnWrittenDamage120,event+0x48,4);
        Check(!memcmp(event+0x18,event+0x5c,12)&&event[0x6a]==1,"both native pawn event hit fields receive blade point and valid flag");
    }
    // Original 12D5C50 consumes its target reference on all paths.
    InterlockedDecrement(reinterpret_cast<volatile LONG*>(wrapper+8));
}
__declspec(naked) void PawnHelperFixture120()
{
    __asm push ebp
    __asm mov ebp,esp
    __asm push eax
    __asm push dword ptr [ebp+16]
    __asm push dword ptr [ebp+12]
    __asm push dword ptr [ebp+8]
    __asm call PawnHelperCheck120
    __asm add esp,16
    __asm pop ebp
    __asm ret
}
void TestMachetePawn120()
{
    auto oldCuts=g_macheteCuts121;auto oldBlade=g_macheteBladeContact;auto oldActor=g_macheteActor119;auto oldPawn=g_machetePawn120;
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x1740000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(image!=nullptr,"pawn native ABI fixture image");
    auto jump=[&](UINT rva,void* target){image[rva]=0xe9;const UINT relative=reinterpret_cast<UINT>(target)-reinterpret_cast<UINT>(image+rva)-5;memcpy(image+rva+1,&relative,4);};
    jump(0x2535c0,reinterpret_cast<void*>(PawnDescriptorFixture120));jump(0x1706640,reinterpret_cast<void*>(PawnLookupFixture120));jump(0x12d2250,reinterpret_cast<void*>(PawnClassifierFixture120));jump(0x12d5c50,reinterpret_cast<void*>(PawnHelperFixture120));
    UINT wrapper[8]={},entity[32]={},sentinel[8]={},self[8]={},selfEntity[32]={},results[1]={};BYTE stack[1024]={},frame[64]={},config[0x200]={};
    wrapper[2]=10;wrapper[3]=reinterpret_cast<UINT>(entity);sentinel[2]=10;self[3]=reinterpret_cast<UINT>(selfEntity);results[0]=reinterpret_cast<UINT>(wrapper);g_pawnEntity120=wrapper[3];
    const UINT vector=reinterpret_cast<UINT>(results),one=1,previous=reinterpret_cast<UINT>(sentinel);memcpy(stack+0x240,&vector,4);memcpy(stack+0x244,&one,4);memcpy(stack+0x14,&previous,4);
    auto& b=g_macheteBladeContact;b=MacheteBladeContactState{};b.enabled=true;b.inCall=1;b.callThread=GetCurrentThreadId();b.engine=reinterpret_cast<UINT>(image);b.strategy=0x2345;b.callConfig118=reinterpret_cast<UINT>(config);float damage=240;memcpy(config+8,&damage,4);
    auto& a=g_macheteActor119;a=MacheteActorState119{};a.pass=true;a.query=true;a.self=reinterpret_cast<UINT>(self);a.count=1;a.hits[0]={results[0],{4.25,5.5,6.75}};
    g_machetePawn120=MachetePawnState120{};g_pawnHelperCalls120=0;g_pawnBlocked120=false;g_pawnExists120=true;
    UINT saved[8]={};saved[3]=reinterpret_cast<UINT>(stack)-4;saved[2]=reinterpret_cast<UINT>(frame);
    Check(MacheteActorSelect119(saved)==1&&g_pawnHelperCalls120==1&&g_machetePawn120.dispatches==1,"a false rigid classifier still dispatches shape-validated CPawn damage through native helper");
    Check(frame[0x14]==1&&MacheteNativeRead(reinterpret_cast<UINT>(stack),0x14)==previous,"pawn event does not enter the separate rigid/static melee event block");
    Check(wrapper[2]==10&&sentinel[2]==10&&results[0]==reinterpret_cast<UINT>(wrapper),"classifier, consumed helper reference and native vector ownership remain balanced");
    Check(Length(Sub(g_pawnWrittenPoint120,{4.25,5.5,6.75}))<1e-6&&g_pawnWrittenDamage120==240&&b.events==1&&b.objectHits==0,"pawn event preserves native damage amount and blade impact, counted independently from props");
    a.query=true;MacheteActorSelect119(saved);Check(g_pawnHelperCalls120==1,"a dispatched pawn hit enforces the shared per-entity cooldown");
    b.cooldownCount=0;g_pawnBlocked120=true;a.query=true;MacheteActorSelect119(saved);
    Check(g_machetePawn120.blocked==1&&g_machetePawn120.dispatches==1&&b.cooldownCount==0&&wrapper[2]==10,"native builder rejection causes no false damage flash, cooldown or reference leak");
    g_pawnExists120=false;a.query=true;MacheteActorSelect119(saved);Check(g_machetePawn120.nonPawns==1&&g_pawnHelperCalls120==2,"non-pawn scenery cannot enter pawn damage route");
    // Real generic-contact hook: pawn events must share actor-search contact
    // latching and right-hand feedback, with no second native dispatch.
    g_macheteCuts121=MacheteCuts121{};g_pawnBlocked120=false;g_pawnExists120=true;b.cooldownCount=0;b.queryApplied118=true;
    b.start={4.25,5.5,6.75};b.end={4.3,5.5,6.75};b.half=.1f;b.radius=.025f;a.pass=false;
    BYTE contact[0x30]={};const float hit[3]={4.25f,5.5f,6.75f};memcpy(contact+0x14,hit,12);
    auto* site=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(site!=nullptr,"generic pawn hook fixture");
    UINT global=0;const UINT address=reinterpret_cast<UINT>(&global);site[0]=0x8b;site[1]=0x0d;memcpy(site+2,&address,4);site[6]=0xeb;site[7]=24;const BYTE finish[]={0xb8,3,0,0,0,0xc3};memcpy(site+32,finish,6);
    const auto initialized=MH_Initialize();Check(initialized==MH_OK||initialized==MH_ERROR_ALREADY_INITIALIZED,"generic pawn hook initialize");
    const auto oldCleanup=g_pawnRouteCleanup121;g_pawnRouteCleanup121=reinterpret_cast<UINT>(MacheteActorSkipFixture118);
    b.inCall=0;UINT expected[8]={},actual[8]={};CallMacheteActorFixture118(expected,site,reinterpret_cast<UINT>(frame),results[0],reinterpret_cast<UINT>(contact));
    Check(MH_CreateHook(site,reinterpret_cast<void*>(MachetePawnRouteBoundary121),&g_pawnRouteOriginal121)==MH_OK&&MH_EnableHook(site)==MH_OK,"generic pawn route production MinHook installs");
    CallMacheteActorFixture118(actual,site,reinterpret_cast<UINT>(frame),results[0],reinterpret_cast<UINT>(contact));Check(!memcmp(actual,expected,sizeof(actual)),"ordinary generic pawn path preserves displaced instruction, flags, registers and SSE");
    b.inCall=1;MacheteCutBegin121(1000,true);g_macheteCuts121.queries=4;const UINT dispatched=g_machetePawn120.dispatches;
    CallMacheteActorFixture118(actual,site,reinterpret_cast<UINT>(frame),results[0],reinterpret_cast<UINT>(contact));
    Check(actual[0]==2&&actual[1]==results[0]&&actual[2]==0x13579bdf&&(actual[3]&1)&&g_machetePawn120.dispatches==dispatched+1&&g_macheteCuts121.hits==1&&wrapper[2]==10,"generic pawn shape hit uses guarded native event exactly once, retains ABI and balances references");
    const UINT feedback=g_macheteCuts121.feedback;CallMacheteActorFixture118(actual,site,reinterpret_cast<UINT>(frame),results[0],reinterpret_cast<UINT>(contact));
    Check(g_machetePawn120.dispatches==dispatched+1&&g_macheteCuts121.feedback==feedback,"duplicate generic pawn overlap cannot dispatch or vibrate again");
    MacheteCutEnd121(true);MacheteCutBegin121(1016,false);g_macheteCuts121.queries=4;g_macheteCuts121.targets[0].latched=false;b.cooldownCount=0;a.query=true;
    a.pass=true;Check(MacheteActorSelect119(saved)==2&&saved[1]==previous,"idle observation enters native reference cleanup with owned selection in ESI and bypasses miss feedback");a.pass=false;CallMacheteActorFixture118(actual,site,reinterpret_cast<UINT>(frame),results[0],reinterpret_cast<UINT>(contact));
    Check(g_machetePawn120.dispatches==dispatched+1&&g_macheteCuts121.feedback==feedback,"stationary observation suppresses both actor-search and generic pawn damage even when armed");
    MacheteCutEnd121(true);MH_DisableHook(site);MH_RemoveHook(site);VirtualFree(site,0,MEM_RELEASE);g_pawnRouteCleanup121=oldCleanup;if(initialized==MH_OK)MH_Uninitialize();
    g_macheteCuts121=oldCuts;g_macheteBladeContact=oldBlade;g_macheteActor119=oldActor;g_machetePawn120=oldPawn;VirtualFree(image,0,MEM_RELEASE);
    std::puts("PASS: pawn120 false rigid classification routes actual blade contact to native CPawn damage; native descriptor/EAX/stack ABI, damage and hit fields, balanced consumed references, separate dispatch/cooldown, rejected builder and non-pawn isolation");
}

