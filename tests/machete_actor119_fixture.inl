UINT g_rayBody119=0,g_rayCalls119=0,g_damageableCalls119=0;bool g_rayHit119=true;
bool __fastcall ActorRayFixture119(void* body,void*,const float* origin,const float* delta,void* output,int flags)
{
    Check(reinterpret_cast<UINT>(body)==g_rayBody119&&flags==1,"native body ray receives this pointer, four stack arguments and native flags");++g_rayCalls119;
    if(!g_rayHit119)return false;
    const float point[3]={origin[0]+delta[0]*.5f,origin[1]+delta[1]*.5f,origin[2]+delta[2]*.5f};memcpy(output,point,12);return true;
}
bool __cdecl ActorDamageableFixture119(UINT wrapper)
{
    ++g_damageableCalls119;InterlockedDecrement(reinterpret_cast<volatile LONG*>(wrapper+8));return true;
}
void TestMacheteActors119()
{
    const auto init=MH_Initialize();Check(init==MH_OK||init==MH_ERROR_ALREADY_INITIALIZED,"actor119 fixture initializes hooks");
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x1740000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(image!=nullptr,"actor hook image fixture");
    for(const auto& hook:g_actorHooks119){BYTE signature[8];MacheteActorSignature121(hook,image,signature);memcpy(image+hook.rva,signature,8);}
    Check(InstallMacheteActors119(image),"all four production character hooks install against verified native instruction prefixes");
    RemoveMacheteActors119(image);image[g_actorHooks119[0].rva]^=1;
    Check(!InstallMacheteActors119(image),"engine signature mismatch refuses character hooks");VirtualFree(image,0,MEM_RELEASE);
    const auto oldBlade=g_macheteBladeContact;const auto oldActor=g_macheteActor119;
    auto& b=g_macheteBladeContact;b=MacheteBladeContactState{};b.inCall=1;b.callThread=GetCurrentThreadId();b.queryApplied118=true;b.axis={0,0,1};b.start={1,2,3};b.end={1.1,2,3};b.radius=.025f;b.half=.05f;
    auto& a=g_macheteActor119;a=MacheteActorState119{};a.pass=true;
    // Execute each production hook's native pass-through, including original
    // instruction relocation and complete register/flags/SSE preservation.
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(page!=nullptr,"actor boundary ABI page");
    UINT abiWrapper[8]={},abiTable[32]={},abiContact[8]={},abiFrame[8]={};abiWrapper[0]=reinterpret_cast<UINT>(abiTable);abiWrapper[3]=reinterpret_cast<UINT>(abiContact);
    const int prefixes[]={8,5,7,8};b.inCall=0;
    for(int i=0;i<4;++i){
        const auto& hook=g_actorHooks119[i];memset(page,0,128);memcpy(page,hook.signature,prefixes[i]);
        page[prefixes[i]]=0xeb;page[prefixes[i]+1]=BYTE(32-prefixes[i]-2);
        const UINT wrapperAddress=reinterpret_cast<UINT>(abiWrapper);memcpy(page+12,&wrapperAddress,4);
        const BYTE finish[]={0xb8,3,0,0,0,0xc3};memcpy(page+32,finish,sizeof(finish));
        UINT expected[8]={},actual[8]={};CallMacheteActorFixture118(expected,page,reinterpret_cast<UINT>(abiFrame),wrapperAddress,reinterpret_cast<UINT>(abiContact));
        Check(MH_CreateHook(page,hook.boundary,hook.original)==MH_OK&&MH_EnableHook(page)==MH_OK,"production actor boundary fixture hooks");
        CallMacheteActorFixture118(actual,page,reinterpret_cast<UINT>(abiFrame),wrapperAddress,reinterpret_cast<UINT>(abiContact));
        Check(memcmp(expected,actual,sizeof(actual))==0,"all character hooks preserve ordinary native registers, flags, SSE and displaced instructions");
        MH_DisableHook(page);MH_RemoveHook(page);
    }
    VirtualFree(page,0,MEM_RELEASE);b.inCall=1;
    UINT table[64]={},body[8]={},wrapper[8]={},entity[32]={},self[8]={},selfEntity[32]={},sentinel[8]={};
    table[0xa8/4]=reinterpret_cast<UINT>(ActorRayFixture119);body[0]=reinterpret_cast<UINT>(table);g_rayBody119=reinterpret_cast<UINT>(body);
    wrapper[2]=10;wrapper[3]=reinterpret_cast<UINT>(entity);self[3]=reinterpret_cast<UINT>(selfEntity);sentinel[2]=10;
    BYTE stack[1024]={},frame[64]={};UINT saved[8]={};saved[3]=reinterpret_cast<UINT>(stack)-4;saved[2]=reinterpret_cast<UINT>(frame);
    const UINT selfPointer=reinterpret_cast<UINT>(self);memcpy(stack+0x18,&selfPointer,4);
    MacheteActorSearch119(saved);float broadOrigin[3];memcpy(broadOrigin,stack+0x5c,12);
    Check(a.query&&a.self==selfPointer&&fabs(broadOrigin[0]-1.05)<1e-6,"actor broad phase is centered on swept blade rather than camera/player feet");
    UINT bodies[]={reinterpret_cast<UINT>(body)};const UINT data=reinterpret_cast<UINT>(bodies),one=1;memcpy(stack+0x48,&data,4);memcpy(stack+0x4c,&one,4);saved[0]=reinterpret_cast<UINT>(wrapper);saved[4]=0;
    g_rayHit119=true;g_rayCalls119=0;
    Check(MacheteActorBody119(saved)==1&&a.count==1&&g_rayCalls119==1,"a real native body-shape hit enters the native retained-wrapper result list");
    a.count=0;g_rayHit119=false;
    Check(MacheteActorBody119(saved)==2&&a.count==0&&g_rayCalls119==21,"nearby actors whose physics shape misses every blade ray are rejected");
    g_rayHit119=true;saved[0]=selfPointer;Check(MacheteActorBody119(saved)==2,"physical machete cannot damage its player body");saved[0]=reinterpret_cast<UINT>(wrapper);
    Check(MacheteActorBody119(saved)==1,"guard body can be tested again after a miss");
    UINT results[]={reinterpret_cast<UINT>(wrapper)};const UINT resultData=reinterpret_cast<UINT>(results),prior=reinterpret_cast<UINT>(sentinel);
    memcpy(stack+0x240,&resultData,4);memcpy(stack+0x244,&one,4);memcpy(stack+0x14,&prior,4);frame[0x14]=1;
    b.engine=reinterpret_cast<UINT>(ActorDamageableFixture119)-0x12d2250;g_damageableCalls119=0;
    Check(MacheteActorSelect119(saved)==1&&MacheteNativeRead(reinterpret_cast<UINT>(stack),0x14)==reinterpret_cast<UINT>(wrapper)&&frame[0x14]==0,"only shape-validated damageable actor enters native event path");
    Check(wrapper[2]==11&&sentinel[2]==9&&g_damageableCalls119==1,"selection owns exactly one reference, releases sentinel and balances consumed damageability reference");
    Check(MacheteNativeRead(reinterpret_cast<UINT>(stack),0x240)==resultData&&MacheteNativeRead(reinterpret_cast<UINT>(stack),0x244)==1,"native result vector remains intact for its normal reference cleanup");
    MacheteActorDispatch119(saved);Check(a.dispatches==1&&b.events==1&&a.dispatchMs!=0,"character dispatch is recorded separately from scenery collision");
    a.query=true;const UINT before=wrapper[2];Check(MacheteActorSelect119(saved)==1&&frame[0x14]==1&&wrapper[2]==before,"per-target cooldown blocks repeated character damage without changing vector ownership");
    b.inCall=0;Check(MacheteActorBody119(saved)==0&&MacheteActorSelect119(saved)==0,"ordinary native attacks retain original character filtering");
    g_macheteBladeContact=oldBlade;g_macheteActor119=oldActor;
    if(init==MH_OK)MH_Uninitialize();
    std::puts("PASS: actor119 native body-ray ABI, blade-centered character search, actual shape hit and miss, self rejection, guarded damage selection, balanced native wrapper references, intact cleanup vector, cooldown and separate character dispatch evidence");
}
