unsigned releaseCalls146=0;UINT releaseFrame146=0,nativeWrapperFixture149=0;
void __fastcall ReleaseFixture146(void* self,void*,UINT wrapper){
    const UINT strategy=reinterpret_cast<UINT>(self);++releaseCalls146;
    Check(wrapper==nativeWrapperFixture149&&wrapper!=Word(strategy,0x38),"native execution wrapper is distinct from inventory wrapper");
    UINT regs[9]={strategy,0,releaseFrame146,0,strategy,0,Word(Word(strategy,0xf4),12),0,0};
    Observe(2,regs);Observe(4,regs);Observe(5,regs);
}
ULONGLONG fixtureLocal143=0;
bool ReadLocal143(ULONGLONG& id){id=fixtureLocal143;return id!=0;}
void TestManualLaunch143(UINT a,UINT strategy,UINT wrapper,UINT owner,UINT projectileRef,UINT projectile,UINT inventory,UINT event,UINT execution){
    using namespace fc3manual;
    const ULONGLONG local=0x1111222233334444ULL;
    fixtureLocal143=local;SetIdentityReader143(ReadLocal143);g_queued=0;
    const UINT frame=a+9216,config=a+10240;nativeWrapperFixture149=execution;
    *reinterpret_cast<UINT*>(frame+12)=execution;*reinterpret_cast<UINT*>(frame+16)=projectileRef;*reinterpret_cast<UINT*>(execution+0x24)=config;
    *reinterpret_cast<BYTE*>(config+0x789)=0;
    UINT regs[9]={strategy,execution,frame,0,strategy,0,projectile,0,0};
    const auto now=GetTickCount64();
    const float original[3]={100,101,102},speed[3]={5,6,7};
    auto resetVectors=[&](){memcpy(reinterpret_cast<void*>(strategy+0x88),original,12);memcpy(reinterpret_cast<void*>(strategy+0xc8),speed,12);};
    unsigned generation=0;
    auto prepare=[&](bool fresh=true){if(fresh)*reinterpret_cast<ULONGLONG*>(projectile+8)=0x5555666677778888ULL+(++generation);regs[0]=strategy;regs[1]=execution;Observe(1,regs);Prepared143 result;Check(CurrentPrepared143(result),"current prepared projectile belongs to proven native local actor");return result;};
    auto release=[&](){return fc3manual::Release{GetTickCount64(),local,3,{100.1,101.2,102.3},{-2,3,4}};};
    PublishContext143(local,3,true);
    const UINT ownerReference=Word(wrapper,0x28);
    // A valid-looking weapon entity must never substitute for a missing pawn.
    *reinterpret_cast<UINT*>(wrapper+0x28)=0;Observe(1,regs);
    Prepared143 missing;Check(!CurrentPrepared143(missing),"weapon entity is not a fallback for absent owning pawn");
    *reinterpret_cast<UINT*>(wrapper+0x28)=ownerReference;
    auto target=prepare();const unsigned serial=target.serial;
    Check(target.nativeWrapper149==execution&&target.nativeWrapper149!=wrapper,"captured native preparation binds distinct execution and inventory objects");
    Prepared143 wrongBinding;Check(!BindExecution149(strategy,wrapper,wrongBinding),"inventory wrapper is not substituted for native execution object");
    Check(prepare(false).serial==serial,"repeated native ready sample is same preparation, not another inventory consume");
    // A held item may remain in hand for seconds. Its ID is revalidated at the
    // native release boundary; it must not expire just because prep was earlier.
    g_prepared143.ms=now-10000;
    Prepared143 held;Check(CurrentPrepared143(held),"fresh interaction retains long-held prepared projectile identity");
    auto command=release();command.epoch=4;
    Check(!QueueRelease143(target,command),"wrong tracking epoch cannot queue release");
    command=release();command.owner=local+1;Check(!QueueRelease143(target,command),"wrong local actor cannot queue release");
    command=release();command.velocity.x=NAN;Check(!QueueRelease143(target,command),"nonfinite launch cannot queue");
    command=release();command.ms=now-500;Check(!QueueRelease143(target,command),"stale release timestamp rejected");
    command=release();Check(QueueRelease143(target,command)&&!QueueRelease143(target,command),"one ticket bound to exact prepared projectile");
    resetVectors();const UINT beforeInventory=*reinterpret_cast<UINT*>(inventory+0x60);
    const float angular=*reinterpret_cast<float*>(strategy+0xd4);
    *reinterpret_cast<ULONGLONG*>(Word(wrapper,0x28))=local+7;Observe(2,regs);
    Check(ManualStatus143().pending&&!memcmp(reinterpret_cast<void*>(strategy+0x88),original,12),"NPC launch does not consume or apply Jason's queued throw");
    *reinterpret_cast<ULONGLONG*>(Word(wrapper,0x28))=local;
    Check(target.projectileRef==projectileRef,"ticket retains exact native projectile reference");
    regs[0]=strategy;Observe(2,regs);
    Check(std::fabs(reinterpret_cast<float*>(strategy+0x88)[0]-100.1f)<1e-5f&&reinterpret_cast<float*>(strategy+0xc8)[0]==-2,
        "actual production release boundary rewrites origin before native SetEntityPosition and physics velocity");
    Check(*reinterpret_cast<UINT*>(inventory+0x60)==beforeInventory&&*reinterpret_cast<float*>(strategy+0xd4)==angular,
        "physical release preserves native inventory and angular impulse");
    Observe(4,regs);regs[0]=event;Observe(5,regs);
    Check(!ManualStatus143().pending&&ManualStatus143().completed==1&&!QueueRelease143(target,release()),"native launch completion retires ticket; old projectile cannot be reused");
    regs[0]=strategy;Observe(1,regs);Prepared143 repeated;Check(!CurrentPrepared143(repeated),"duplicate ready event cannot rearm an already launched projectile");
    regs[0]=strategy;resetVectors();target=prepare();PublishContext143(local,3,true);
    Check(QueueRelease143(target,release()),"second independent preparation can queue");
    *reinterpret_cast<BYTE*>(config+0x789)=1;
    Observe(2,regs);Check(!ManualStatus143().pending&&!memcmp(reinterpret_cast<void*>(strategy+0x88),original,12)&&!memcmp(reinterpret_cast<void*>(strategy+0xc8),speed,12),"unknown event-only branch stays native without guessing payload semantics");
    *reinterpret_cast<BYTE*>(config+0x789)=0;
    target=prepare();PublishContext143(local,3,true);Check(QueueRelease143(target,release()),"fresh release after rejected branch");
    fixtureLocal143=local+1;Observe(2,regs);
    Check(!ManualStatus143().pending&&!memcmp(reinterpret_cast<void*>(strategy+0x88),original,12),"local actor lifecycle change cancels native write");
    fixtureLocal143=local;target=prepare();PublishContext143(local,3,true);Check(QueueRelease143(target,release()),"tracking cancellation setup");
    PublishContext143(local,3,false);Observe(2,regs);Check(!ManualStatus143().pending&&!memcmp(reinterpret_cast<void*>(strategy+0x88),original,12),"lost tracking/context cannot apply pending throw");
    PublishContext143(local,3,true);target=prepare();command=release();command.origin.x=150;Check(QueueRelease143(target,command),"nearby-origin validation occurs against actual native release");
    Observe(2,regs);Check(!ManualStatus143().pending&&!memcmp(reinterpret_cast<void*>(strategy+0x88),original,12),"far native/hand origin disagreement refuses teleport");
    const auto archetype=*reinterpret_cast<ULONGLONG*>(strategy+0x40);
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=0x123456789ULL;target=prepare();Check(!QueueRelease143(target,release()),"unverified C4/mine/other archetype cannot use shared throw override");
    *reinterpret_cast<ULONGLONG*>(strategy+0x40)=archetype;
    target=prepare();PublishContext143(local,3,true);Check(QueueRelease143(target,release()),"capture cap independence setup");
    StopCapture143();Observe(2,regs);Check(ManualStatus143().pending&&reinterpret_cast<float*>(strategy+0xc8)[0]==-2,"disk recording stop retains native release bridge");
    CancelRelease143();PublishContext143(local,3,true);target=prepare();resetVectors();
    Check(QueueRelease143(target,release()),"native-update immediate release ticket setup");
    g_nativeRelease146=reinterpret_cast<NativeRelease146>(ReleaseFixture146);releaseFrame146=frame;releaseCalls146=0;
    const UINT stack=a+15360;*reinterpret_cast<UINT*>(stack+8)=execution;UINT entry[9]={};entry[6]=strategy;entry[3]=stack;
    // Real native test is bit0x200 + bit8; the old fixture repeated a
    // mistaken0x2000 bit and could never expose the bad readiness mask.
    *reinterpret_cast<UINT*>(projectile+0x60)=0x2008;
    PumpRelease146(entry);Check(releaseCalls146==0,"unrelated0x2000 bit cannot authorize native launch");
    *reinterpret_cast<UINT*>(projectile+0x60)=0x208;
    // Recorded slow frames exceed60ms. Explicit focused context stays valid
    // across90ms, while a paused/cancelled context must still reject.
    g_context143.ms=GetTickCount64()-90;const auto completed=ManualStatus143().completed;
    entry[6]=strategy+4;PumpRelease146(entry);Check(releaseCalls146==0,"unrelated native gadget update cannot dispatch ticket");entry[6]=strategy;
    const UINT boundEntity=Word(Word(execution,4),12);const auto nativeId=*reinterpret_cast<ULONGLONG*>(boundEntity+8);
    *reinterpret_cast<ULONGLONG*>(boundEntity+8)=nativeId+1;PumpRelease146(entry);Check(releaseCalls146==0,"reused native execution entity is rejected before release");*reinterpret_cast<ULONGLONG*>(boundEntity+8)=nativeId;
    *reinterpret_cast<UINT*>(execution+0x24)=config+16;PumpRelease146(entry);Check(releaseCalls146==0,"changed execution configuration is rejected before release");*reinterpret_cast<UINT*>(execution+0x24)=config;
    PumpRelease146(entry);Check(releaseCalls146==1&&ManualStatus143().completed==completed+1&&reinterpret_cast<float*>(strategy+0xc8)[0]==-2,"next native update dispatches exact projectile with physical velocity, without waiting for animation");
    PumpRelease146(entry);Check(releaseCalls146==1,"duplicate native update cannot launch twice");
    Check(AlreadyPhysical146(strategy,execution),"native animation release is recognized as the same already-launched projectile");
    g_releaseOriginal146=reinterpret_cast<void*>(ReleaseFixture146);ReleaseGate146(reinterpret_cast<void*>(strategy),nullptr,execution);
    Check(releaseCalls146==1,"late native animation event cannot overwrite trajectory or launch same projectile again");
    *reinterpret_cast<ULONGLONG*>(Word(wrapper,0x28))=local+7;ReleaseGate146(reinterpret_cast<void*>(strategy),nullptr,execution);
    Check(releaseCalls146==2,"different owner retains ordinary native release path");*reinterpret_cast<ULONGLONG*>(Word(wrapper,0x28))=local;releaseCalls146=1;g_releaseOriginal146=nullptr;

    target=prepare();Check(QueueRelease143(target,release()),"native update paused-context setup");PublishContext143(local,3,false);PumpRelease146(entry);Check(releaseCalls146==1,"lost tracking/context forbids immediate native dispatch");
    g_nativeRelease146=nullptr;
    CancelRelease143();SetIdentityReader143(nullptr);g_capture143=1;g_queued=0;resetVectors();
    Prepared143 absent;Check(!CurrentPrepared143(absent),"no proven identity reader means no native projectile ownership");
    std::puts("PASS: production launch bridge exact projectile/owner/epoch tickets, real native origin/velocity writes, one-shot completion, native inventory preserved, event/NPC/lifecycle/tracking/stale/teleport rejection and capture-cap independence.");
}
