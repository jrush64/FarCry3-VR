UINT g_localCalls123=0,g_configCalls123=0,g_configPointer123=0,g_configInput123=0;
UINT* __cdecl LocalHandleFixture123(UINT* handle){++g_localCalls123;handle[0]=0x80009100;handle[1]=0x12345678;return handle;}
__declspec(naked) void ConfigFixture123(){
    __asm mov g_configInput123,eax
    __asm inc g_configCalls123
    __asm mov eax,g_configPointer123
    __asm ret
}
void TestMacheteRecovery123()
{
    const auto oldBlade=g_macheteBladeContact;const auto oldVisual=g_machete;const bool oldGameplay=g_inGameplay;const auto oldCuts=g_macheteCuts121;
    fc3pad::Controllers neutral={};neutral.active=true;fc3pad::Publish(neutral);fc3pad::SetPistolCalibration(false);fc3pad::SetMacheteContext(true);fc3pad::SetMacheteHandling(true);fc3pad::CancelMacheteSource();fc3pad::CancelMacheteAttack();
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x2830000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(image!=nullptr,"recovery123 native image fixture");
    const UINT base=reinterpret_cast<UINT>(image);
    auto jump=[&](UINT rva,void* fn){image[rva]=0xe9;const INT32 offset=INT32(reinterpret_cast<UINT>(fn)-(base+rva+5));memcpy(image+rva+1,&offset,4);};
    jump(0xc80190,reinterpret_cast<void*>(LocalHandleFixture123));jump(0x1316990,reinterpret_cast<void*>(ConfigFixture123));
    UINT strategy[20]={},wrapper[8]={},entity[8]={},receiver[32]={},manager[8]={};BYTE root[0x1000]={},config[0x200]={};
    const UINT strategyPtr=reinterpret_cast<UINT>(strategy),rootPtr=reinterpret_cast<UINT>(root);
    strategy[0]=base+0x1f99568;strategy[1]=reinterpret_cast<UINT>(wrapper);strategy[0x24/4]=rootPtr;wrapper[3]=reinterpret_cast<UINT>(entity);entity[2]=0x80009100;entity[3]=0x12345678;
    memcpy(root,&strategyPtr,4);receiver[0x48/4]=rootPtr;manager[3]=1;*reinterpret_cast<UINT*>(image+0x2829474)=reinterpret_cast<UINT>(manager);
    const float damage=55,range=2;memcpy(config+8,&damage,4);memcpy(config+0x10c,&range,4);g_configPointer123=reinterpret_cast<UINT>(config);
    g_macheteBladeContact=MacheteBladeContactState{};auto& b=g_macheteBladeContact;b.enabled=true;b.engine=base;b.strategy=0;g_machete.held=true;g_inGameplay=true;g_configCalls123=g_localCalls123=0;
    entity[3]^=1;MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(!b.strategy&&!g_configCalls123,"another actor's tick cannot acquire the player's machete damage controller");entity[3]^=1;
    MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(b.strategy==strategyPtr&&b.config==g_configPointer123&&b.recovered123==1,"a cleared damage controller recovers from the current local player's native tick without a swipe");
    Check(g_configInput123==rootPtr+0xbe4&&g_configCalls123==1,"actual x86 config adapter passes native root+BE4 in EAX exactly once");
    MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(g_configCalls123==1&&b.recovered123==1,"valid current controller does not repeatedly resolve or reacquire");
    UINT newStrategy[20]={};memcpy(newStrategy,strategy,sizeof(strategy));const UINT newPtr=reinterpret_cast<UINT>(newStrategy);memcpy(root,&newPtr,4);strategy[0]=0;
    b.havePrev=true;MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(b.strategy==newPtr&&b.recovered123==2&&!b.havePrev,"weapon switch replaces a destroyed cached strategy and resets old motion history");
    newStrategy[0x24/4]=0;MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(b.recovered123==2&&g_configCalls123==2,"reciprocal root ownership is required before a native helper call");newStrategy[0x24/4]=rootPtr;
    b.strategy=0;g_machete.held=false;MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(!b.strategy&&g_configCalls123==2,"stowed blade cannot acquire or damage");g_machete.held=true;
    fc3pad::SetPistolCalibration(true);MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(!b.strategy,"Insert pauses controller recovery");fc3pad::SetPistolCalibration(false);
    const float invalid=0;memcpy(config+8,&invalid,4);MacheteRefreshController123(reinterpret_cast<UINT>(receiver));Check(!b.strategy,"invalid current melee config fails closed");
    Check(!(fc3pad::ButtonsOutput()&0x80),"controller recovery never requests an automatic attack animation");
    fc3pad::SetMacheteHandling(false);fc3pad::SetMacheteContext(false);g_macheteBladeContact=oldBlade;g_machete=oldVisual;g_inGameplay=oldGameplay;g_macheteCuts121=oldCuts;VirtualFree(image,0,MEM_RELEASE);
    std::puts("PASS: recovery123 cleared/stale melee controller, local-owner and reciprocal-root validation, native EAX config ABI, weapon switch, pause/stow/invalid guards and no automatic swipe");
}
