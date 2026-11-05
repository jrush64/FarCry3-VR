volatile unsigned soundCalls156=0;
volatile UINT playedSound156=0;
void __fastcall CleanupLooseFixture156(void* self,void*,UINT){
    Check(*reinterpret_cast<BYTE*>(reinterpret_cast<UINT>(self)+0x6b)==0,"released unarmed grenade never takes the native delete/stow branch");
}
void TestRelease156(UINT strategy,UINT execution,UINT projectile,UINT inventory,UINT event){
    auto* page=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto* code=static_cast<BYTE*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    Check(page&&code&&MH_Initialize()==MH_OK,"sound156 fixture allocation");
    const UINT receiver=reinterpret_cast<UINT>(page)+64,component=reinterpret_cast<UINT>(page)+512,ref=Word(strategy,0xf4);
    *reinterpret_cast<UINT*>(receiver-8)=Word(Word(strategy,0x38),0x28);
    *reinterpret_cast<UINT*>(component)=g_engine+0x1fc67c0;*reinterpret_cast<UINT*>(component+4)=ref;
    memcpy(code,kPinSoundBytes156,sizeof(kPinSoundBytes156));UINT at=sizeof(kPinSoundBytes156);
    code[at++]=0xff;code[at++]=0x05;const UINT counter=reinterpret_cast<UINT>(&soundCalls156);memcpy(code+at,&counter,4);at+=4;
    const BYTE load[]={0x8b,0x45,0x10,0x8b,0x40,0x18,0xa3};memcpy(code+at,load,sizeof(load));at+=sizeof(load);
    const UINT observed=reinterpret_cast<UINT>(&playedSound156);memcpy(code+at,&observed,4);at+=4;
    const BYTE tail[]={0x5e,0x5b,0x8b,0xe5,0x5d,0xc2,0x0c,0};memcpy(code+at,tail,sizeof(tail));
    FlushInstructionCache(GetCurrentProcess(),code,128);
    Check(MH_CreateHook(code,reinterpret_cast<void*>(PinSoundGate156),&g_soundOriginal156)==MH_OK&&MH_EnableHook(code)==MH_OK,"exact native animation sound handler ret12 MinHook relocation");
    auto play=reinterpret_cast<NativeSound156>(code);
    fixtureLocal143=0x1111222233334444ULL;SetIdentityReader143(ReadLocal143);SetPinIntent155(true);g_safe155={};g_pinSound156={};
    *reinterpret_cast<UINT*>(event)=g_engine+0x1f4d618;*reinterpret_cast<UINT*>(event+0x18)=0xf6e9;*reinterpret_cast<UINT*>(event+0x1c)=8;
    volatile UINT canary=0x10203040;play(reinterpret_cast<void*>(receiver),2,1,event);
    Check(!soundCalls156&&g_pinSound156.owner==fixtureLocal143&&canary==0x10203040,"draw pin sound suppressed even before native projectile preparation");
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x03c006e5ac354e19ULL;*reinterpret_cast<ULONGLONG*>(projectile+8)+=100;
    PublishContext143(fixtureLocal143,3,true);UINT regs[9]={strategy,execution,releaseFrame146,0,strategy,0,projectile,0,0};Observe(1,regs);
    Prepared143 target;Check(CurrentPrepared143(target)&&g_pinSound156.projectile==target.projectile,"preparation binds earlier animation sound to exact held grenade");
    PumpPinSound156(strategy,execution);Check(!soundCalls156,"held pin-in grenade remains silent");
    Check(RequestPinOut155(target),"physical pin completion requests deferred sound");PublishContext143(0,0,false);PumpPinSound156(strategy,execution);Check(!soundCalls156,"no deferred audio replay while tracking/context is paused");
    PublishContext143(fixtureLocal143,3,true);PumpPinSound156(strategy,execution);PumpPinSound156(strategy,execution);
    Check(soundCalls156==1&&playedSound156==0xf6e9&&g_pinSound156.played&&canary==0x10203040,"physical pin plays original native sound exactly once with retained ret12 stack");
    *reinterpret_cast<UINT*>(event+0x18)=0x1234;play(reinterpret_cast<void*>(receiver),2,1,event);
    Check(soundCalls156==2&&playedSound156==0x1234,"other weapon animation audio remains native");
    *reinterpret_cast<UINT*>(event+0x18)=0xf6e9;const UINT ownerRef=Word(receiver-8);
    const auto ownerId=*reinterpret_cast<ULONGLONG*>(Word(ownerRef,12)+8);*reinterpret_cast<ULONGLONG*>(Word(ownerRef,12)+8)=ownerId+1;
    play(reinterpret_cast<void*>(receiver),2,1,event);Check(soundCalls156==3,"NPC pin sounds remain native");*reinterpret_cast<ULONGLONG*>(Word(ownerRef,12)+8)=ownerId;

    SetMolotovIntent158(true);*reinterpret_cast<UINT*>(event+0x18)=0x18975a;*reinterpret_cast<UINT*>(event+0x1c)=15;
    unsigned before177=soundCalls156;play(reinterpret_cast<void*>(receiver),2,1,event);
    Check(soundCalls156==before177&&canary==0x10203040,"177 exact manual Molotov prime lighting audio suppressed with native ret12 ABI");
    *reinterpret_cast<UINT*>(event+0x18)=0x189759;play(reinterpret_cast<void*>(receiver),2,1,event);
    Check(soundCalls156==before177+1,"177 Molotov throw audio remains native");
    *reinterpret_cast<UINT*>(event+0x18)=0x18975a;*reinterpret_cast<ULONGLONG*>(Word(ownerRef,12)+8)=ownerId+1;play(reinterpret_cast<void*>(receiver),2,1,event);
    Check(soundCalls156==before177+2,"177 NPC Molotov lighting sound remains native");*reinterpret_cast<ULONGLONG*>(Word(ownerRef,12)+8)=ownerId;
    SetMolotovIntent158(false);play(reinterpret_cast<void*>(receiver),2,1,event);Check(soundCalls156==before177+3,"177 stock Molotov lighting audio remains when manual policy is off");
    *reinterpret_cast<UINT*>(event+0x18)=0xf6e9;*reinterpret_cast<UINT*>(event+0x1c)=8;

    // A new pin-IN grenade follows the same physical update/vector/dispatch
    // path, without requesting arming or replaying its pin sound.
    *reinterpret_cast<ULONGLONG*>(projectile+8)+=1;Observe(1,regs);Check(CurrentPrepared143(target),"fresh unarmed drop target");g_safe155.component=component;
    Check(LooseRelease156(target)&&g_safe155.loose&&!g_safe155.request,"right release selects physical loose grenade without arming");
    SetUnlimited147(false);*reinterpret_cast<UINT*>(inventory+0x60)=2;PreserveSafeStock155(strategy,inventory);g_stockSetter155=reinterpret_cast<UINT>(DebitFixture155);
    Check(QueueRelease143(target,{GetTickCount64(),fixtureLocal143,3,{100.1,101.2,102.3},{0,0,0}}),"pin-in zero-velocity release queues ordinary native projectile");
    *reinterpret_cast<UINT*>(projectile+0x60)=0x208;g_nativeRelease146=reinterpret_cast<NativeRelease146>(ReleaseFixture146);
    const UINT stack=reinterpret_cast<UINT>(page)+1536;*reinterpret_cast<UINT*>(stack+8)=execution;UINT entry[9]={};entry[6]=strategy;entry[3]=stack;
    const auto before=ManualStatus143().completed;PumpRelease146(entry);
    Check(ManualStatus143().completed==before+1&&g_safe155.launched&&!g_safe155.armed&&Word(inventory,0x60)==1,"pin-in drop launches and loses one finite item without native arming");
    g_cleanupOriginal155=reinterpret_cast<void*>(CleanupLooseFixture156);*reinterpret_cast<BYTE*>(strategy+0x6b)=0;CleanupGate155(reinterpret_cast<void*>(strategy),nullptr,execution);
    g_safe155={};Check(BlockLooseSafe156(component),"unarmed airborne grenade remains protected after hand ownership ends");
    *reinterpret_cast<ULONGLONG*>(projectile+8)+=1;Check(!BlockLooseSafe156(component),"retired/reused projectile reference cannot protect a different entity");
    Check(MH_DisableHook(code)==MH_OK&&MH_RemoveHook(code)==MH_OK&&MH_Uninitialize()==MH_OK,"sound156 hook cleanup");
    g_soundOriginal156=g_cleanupOriginal155=nullptr;g_nativeRelease146=nullptr;g_stockSetter155=0;g_pinSound156={};g_safe155={};SetIdentityReader143(nullptr);SetPinIntent155(false);SetUnlimited147(true);CancelRelease143();
    VirtualFree(page,0,MEM_RELEASE);VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS:156 native sound ret12 ABI, draw suppression, exact owner/item pin replay once, other audio/NPC passthrough; unarmed physical drop and finite loss; post-release safety and entity reuse.");
}
