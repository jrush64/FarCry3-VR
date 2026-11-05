#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cmath>
#include "throwable_launch_probe.h"
#include "native_launch_sites.h"

namespace fc3throw {
namespace {
UINT g_engine=0;
bool g_installed=false;
bool g_attempted=false;
volatile LONG g_enabled=0;
volatile LONG g_capture143=1;
void* g_original[6]={};
void* g_updateOriginal146=nullptr;
void* g_releaseOriginal146=nullptr;
void* g_stockOriginal147=nullptr;
bool g_stockInstalled147=false;
volatile LONG g_unlimited147=1,g_preserved147=0;
constexpr BYTE kStockBytes147[7]={0x2b,0xc2,0xe8,0x32,0x9f,0x07,0xff};
bool g_updateInstalled146=false;
using NativeRelease146=void(__thiscall*)(void*,UINT);
NativeRelease146 g_nativeRelease146=nullptr;
constexpr BYTE kUpdateBytes146[16]={0x55,0x8b,0xec,0x56,0x8b,0xf1,0x57,0x8b,0xbe,0xf4,0,0,0,0x8b,0x4f,0x0c};
constexpr BYTE kReleaseBytes146[16]={0x55,0x8b,0xec,0x53,0x8b,0x5d,0x08,0x8b,0x43,0x24,0x80,0xb8,0x89,0x07,0,0};
SRWLOCK g_queueLock=SRWLOCK_INIT, g_poseLock=SRWLOCK_INIT, g_fileLock=SRWLOCK_INIT;
LaunchRecord g_queue[64]; UINT g_queued=0, g_written=0;
volatile LONG g_lost=0, g_phases[6]={};
DWORD g_writeError=0; bool g_full=false;
PoseSample g_pose;
SRWLOCK g_manualLock143=SRWLOCK_INIT;
IdentityReader143 g_identity143=nullptr;
Prepared143 g_prepared143;
Prepared143 g_candidate144;
Prepared143 g_manualLaunched146;
ULONGLONG g_launchMs157=0;
struct Pending143 {Prepared143 target;fc3manual::Release release;DWORD thread=0;bool active=false,applied=false,scheduled=false;} g_pending143;
ManualStats143 g_manualStats143;
unsigned g_serial143=0;
ULONGLONG g_retiredOwner143=0,g_retiredProjectile143=0;
struct Context143 {ULONGLONG owner=0,ms=0;unsigned epoch=0;bool active=false;}g_context143;

bool Read(UINT address,void* output,UINT bytes) {
    if(!address||!bytes||bytes>4096||address>0xffffffffu-bytes)return false;
    __try { std::memcpy(output,reinterpret_cast<void*>(address),bytes);return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
UINT Word(UINT address,UINT offset=0) {
    UINT value=0;
    if(address&&offset<=0xffffffffu-address)Read(address+offset,&value,4);
    return value;
}
bool ReadAt(UINT address,UINT offset,void* output,UINT bytes) {
    return address&&offset<=0xffffffffu-address&&Read(address+offset,output,bytes);
}
bool Writable143(UINT address,UINT bytes){
    if(!address||!bytes||address>0xffffffffu-bytes)return false;
    for(UINT p=address;p<address+bytes;){MEMORY_BASIC_INFORMATION m={};
        if(!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
        const DWORD access=m.Protect&255;
        if(access!=PAGE_READWRITE&&access!=PAGE_WRITECOPY&&access!=PAGE_EXECUTE_READWRITE&&access!=PAGE_EXECUTE_WRITECOPY)return false;
        const UINT end=reinterpret_cast<UINT>(m.BaseAddress)+static_cast<UINT>(m.RegionSize);
        if(end<=p)return false;p=end;
    }return true;
}
bool SignaturesMatch(const BYTE* image,UINT size) {
    if(size!=kImageSize)return false;
    BYTE bytes[16];
    for(UINT i=0;i<6;++i) {
        if(kSites[i]>size-sizeof(bytes)||!Read(reinterpret_cast<UINT>(image)+kSites[i],bytes,sizeof(bytes))||
           std::memcmp(bytes,kSignatures[i],sizeof(bytes)))return false;
    }
    return true;
}
// wrapper+4 names the equipped gadget entity, NOT Jason. This distinction is
// proven by PID37148: grenade and Molotov had different IDs there, while
// native local-player identity stayed constant. The established firearm path
// obtains its owning pawn through wrapper+0x28 and compares the reference ID.
bool PawnIdentity145(UINT wrapper,ULONGLONG& id,UINT& entity){
    id=0;entity=0;const UINT reference=Word(wrapper,0x28);
    entity=Word(reference,0xc);
    return entity&&ReadAt(reference,0,&id,8)&&id&&id!=~0ULL;
}
// Native prepare ESI is its execution argument; strategy+38 is a distinct
// inventory/ownership object. PID27340 proves they differ on every request.
// The native prefix copies execution ref entity GUID into strategy+8/+C.
bool BindExecution149(UINT strategy,UINT wrapper,Prepared143& binding){
    ULONGLONG strategyId=0,entityId=0;const UINT reference=Word(wrapper,4),entity=Word(reference,12),config=Word(wrapper,0x24);BYTE branch=0;
    if(!wrapper||!reference||!entity||!config||!ReadAt(strategy,8,&strategyId,8)||
       !ReadAt(entity,8,&entityId,8)||!entityId||entityId==~0ULL||strategyId!=entityId||!ReadAt(config,0x789,&branch,1))return false;
    binding.nativeWrapper149=wrapper;binding.nativeRef149=reference;binding.nativeConfig149=config;binding.nativeEntityId149=entityId;return true;
}
bool SameExecution149(const Prepared143& target){
    Prepared143 current;return BindExecution149(target.strategy,target.nativeWrapper149,current)&&
        current.nativeRef149==target.nativeRef149&&current.nativeConfig149==target.nativeConfig149&&current.nativeEntityId149==target.nativeEntityId149;
}
bool AlreadyPhysical146(UINT strategy,UINT wrapper){
    Prepared143 done;AcquireSRWLockShared(&g_manualLock143);done=g_manualLaunched146;ReleaseSRWLockShared(&g_manualLock143);
    ULONGLONG owner=0,projectile=0;UINT entity=0;
    return InterlockedCompareExchange(&g_enabled,0,0)&&done.serial&&strategy==done.strategy&&wrapper==done.nativeWrapper149&&SameExecution149(done)&&
        Word(strategy)==g_engine+kStrategyVtable&&Word(strategy,0xf4)==done.projectileRef&&
        PawnIdentity145(Word(strategy,0x38),owner,entity)&&owner==done.owner&&
        ReadAt(Word(done.projectileRef,12),8,&projectile,8)&&projectile==done.projectile;
}
// A later native animation event can still request release after the mod's early
// dispatch. Suppress only that exact already-manually-launched projectile;
// all other native items and NPC calls execute the unmodified release entry.
bool ManualReleaseBlocked155(UINT strategy,UINT wrapper);
bool PreserveSafeStock155(UINT strategy,UINT data);
void __fastcall ReleaseGate146(void* self,void*,UINT wrapper){
    if(ManualReleaseBlocked155(reinterpret_cast<UINT>(self),wrapper))return;
    if(AlreadyPhysical146(reinterpret_cast<UINT>(self),wrapper))return;
    const auto original=reinterpret_cast<NativeRelease146>(g_releaseOriginal146);
    if(original)original(self,wrapper);
}
bool LocalIdentity143(ULONGLONG& owner){return g_identity143&&g_identity143(owner)&&owner&&owner!=~0ULL;}
// At this exact native site EAX=stock, EDX=one item to deduct, ECX=inventory
// data, EDI=throw strategy. Keep native setter/events; change only the debit.
void __cdecl PreserveStock147(UINT* saved){
    if(!InterlockedCompareExchange(&g_enabled,0,0))return;
    const UINT strategy=saved[0],wrapper=Word(strategy,0x38),data=Word(wrapper,0x3c);
    ULONGLONG archetype=0,owner=0,local=0;UINT entity=0;
    if(Word(strategy)!=g_engine+kStrategyVtable||!ReadAt(strategy,0x40,&archetype,8)||
       (archetype!=0x03c006e5ac354e19ULL&&archetype!=0x000006f6a4abae8dULL)||
       !data||saved[6]!=data||!saved[7]||saved[7]!=Word(data,0x60)||saved[5]!=1)return;
    AcquireSRWLockShared(&g_manualLock143);
    const bool localThrow=PawnIdentity145(wrapper,owner,entity)&&LocalIdentity143(local)&&owner==local;
    ReleaseSRWLockShared(&g_manualLock143);
    if(localThrow&&(PreserveSafeStock155(strategy,data)||InterlockedCompareExchange(&g_unlimited147,0,0))){saved[5]=0;InterlockedIncrement(&g_preserved147);}
#ifdef FC3_THROW_PROBE_TEST
    __asm { pxor xmm0,xmm0 }
    __asm { fninit }
#endif
}
__declspec(naked) void StockBoundary147(){
    __asm pushfd
    __asm pushad
    __asm cld
    __asm mov eax,esp
    __asm sub esp,544
    __asm and esp,0fffffff0h
    __asm mov [esp+512],eax
    __asm fxsave [esp]
    __asm push eax
    __asm call PreserveStock147
    __asm add esp,4
    __asm fxrstor [esp]
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_stockOriginal147]
}
bool SamePrepared143(const Prepared143& a,const Prepared143& b){
    return a.owner==b.owner&&a.projectile==b.projectile&&a.archetype==b.archetype&&
        a.strategy==b.strategy&&a.projectileRef==b.projectileRef&&a.serial==b.serial&&a.epoch==b.epoch&&
        a.nativeWrapper149==b.nativeWrapper149&&a.nativeRef149==b.nativeRef149&&a.nativeConfig149==b.nativeConfig149&&a.nativeEntityId149==b.nativeEntityId149;
}
#include "grenade_safety155.inl"
#include "grenade_sound156.inl"
#include "molotov_flame160.inl"
#include "molotov_trail176.inl"
// Called before the original release-branch instructions, after all strategy
// vectors have been computed but BEFORE SetEntityPosition and physics velocity.
// The alternative event payload's semantics are not proven: never write it.
void ManualObserve143(LaunchRecord& r,const UINT* saved){
    UINT debit156=0;ULONGLONG local=0;
    AcquireSRWLockExclusive(&g_manualLock143);
    const bool owned=LocalIdentity143(local)&&(r.flags&0x2d)==0x2d&&r.ownerId==local&&r.projectileId&&r.projectileId!=~0ULL;
    if(owned)r.flags|=2048;
    if(r.phase==1&&owned&&!(r.ownerId==g_retiredOwner143&&r.projectileId==g_retiredProjectile143)){
        Prepared143 next;next.owner=local;next.projectile=r.projectileId;next.archetype=r.archetype;
        next.ms=r.wallMs;next.strategy=r.strategy;next.projectileRef=r.projectileRef;
        if(!BindExecution149(r.strategy,saved[1],next)){g_pending143={};g_prepared143={};g_candidate144={};++g_manualStats143.rejected;ReleaseSRWLockExclusive(&g_manualLock143);return;}
        r.eventWords[0]=next.nativeWrapper149;r.eventWords[1]=next.nativeRef149;r.eventWords[2]=next.nativeConfig149;
        r.eventWords[3]=UINT(next.nativeEntityId149);r.eventWords[4]=UINT(next.nativeEntityId149>>32);
        next.epoch=g_context143.active&&g_context143.owner==local?g_context143.epoch:0;
        // Repeated ready observations are the same preparation, not another
        // inventory transaction. Keep an existing release bound to its serial.
        next.serial=g_prepared143.serial;
        if(!SamePrepared143(next,g_prepared143)){next.serial=++g_serial143;g_pending143={};++g_manualStats143.prepared;}
        g_prepared143=next;
        g_candidate144=next;
        CaptureSafe155(next);
    }
    auto& p=g_pending143;
    if(p.active){
        const bool expired=r.wallMs<p.release.ms||r.wallMs-p.release.ms>500;
        const bool target=r.strategy==p.target.strategy&&r.projectileRef==p.target.projectileRef&&
            r.projectileId==p.target.projectile&&r.archetype==p.target.archetype&&r.ownerId==p.target.owner;
        const bool context=g_context143.active&&g_context143.owner==p.release.owner&&g_context143.epoch==p.release.epoch&&
            r.wallMs>=g_context143.ms&&r.wallMs-g_context143.ms<=250;
        if(expired||!context||!LocalIdentity143(local)||local!=p.target.owner){
            if(expired)++g_manualStats143.expired148;else ++g_manualStats143.contextRejects148;
            p={};++g_manualStats143.rejected;}
        else if(target&&(r.phase==2||r.phase==4||r.phase==5)){
            if(!owned){p={};++g_manualStats143.rejected;}
            else if(r.phase==2){
                UINT wrapper=0,config=0,projectileArg=0;BYTE eventBranch=0;
                const bool direct=ReadAt(saved[2],12,&wrapper,4)&&wrapper&&
                    wrapper==p.target.nativeWrapper149&&SameExecution149(p.target)&&ReadAt(saved[2],16,&projectileArg,4)&&projectileArg==r.projectileRef&&
                    ReadAt(wrapper,0x24,&config,4)&&config==p.target.nativeConfig149&&ReadAt(config,0x789,&eventBranch,1)&&
                    (!eventBranch||r.forcePhysics!=0);
                r.eventWords[0]=wrapper;r.eventWords[1]=projectileArg;r.eventWords[2]=config;r.eventWords[3]=direct;r.eventWords[5]=p.target.nativeWrapper149;
                if(!direct||(r.flags&512)==0||p.applied||!Writable143(r.strategy+0x88,12)||!Writable143(r.strategy+0xc8,12)||
                    fc3manual::Length(p.release.origin-fc3manual::Vec{r.origin[0],r.origin[1],r.origin[2]})>2.0){++g_manualStats143.vectorRejects148;p={};++g_manualStats143.rejected;}
                else {
                    float origin[3]={float(p.release.origin.x),float(p.release.origin.y),float(p.release.origin.z)};
                    float velocity[3]={float(p.release.velocity.x),float(p.release.velocity.y),float(p.release.velocity.z)};
                    // Both destinations were read successfully into this record.
                    // Preserve native inventory, fuse, attribution and all other
                    // strategy/event fields, including angular impulse.
                    float oldOrigin[3],oldVelocity[3];memcpy(oldOrigin,r.origin,12);memcpy(oldVelocity,r.velocity,12);
                    __try {
                        memcpy(reinterpret_cast<void*>(r.strategy+0x88),origin,12);
                        memcpy(reinterpret_cast<void*>(r.strategy+0xc8),velocity,12);
                        memcpy(r.origin,origin,12);memcpy(r.velocity,velocity,12);
                        r.flags|=1024;p.applied=true;p.thread=r.thread;++g_manualStats143.applied;
                    }__except(EXCEPTION_EXECUTE_HANDLER){
                        __try {memcpy(reinterpret_cast<void*>(r.strategy+0x88),oldOrigin,12);memcpy(reinterpret_cast<void*>(r.strategy+0xc8),oldVelocity,12);}
                        __except(EXCEPTION_EXECUTE_HANDLER){}
                        p={};++g_manualStats143.rejected;
                    }
                }
            }else if(p.applied&&p.thread==r.thread){
                r.flags|=1024;
                if(r.phase==5){
                    if(SameItem155(p.target,g_safe155.target)){auto& safe=g_safe155;safe.launched=true;RememberLoose156();if(safe.deferredStock){if(!InterlockedCompareExchange(&g_unlimited147,0,0))debit156=safe.stockData;safe.deferredStock=false;}}
                    g_manualLaunched146=p.target;g_launchMs157=r.wallMs;g_retiredOwner143=r.ownerId;g_retiredProjectile143=r.projectileId;p={};g_prepared143={};++g_manualStats143.completed;}
            }
        }
    }
    if(r.phase==5&&owned){
        g_retiredOwner143=r.ownerId;g_retiredProjectile143=r.projectileId;
        if(g_candidate144.projectile==r.projectileId)g_candidate144={};
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
    if(debit156)DebitSafeStock155(debit156);
}
// Runs ONLY at the native gadget's own per-frame update boundary. Dispatch the
// existing virtual release entry once for the exact ready local projectile,
// instead of waiting for the RB animation event. Never call from Present/XInput.
void __cdecl PumpRelease146(const UINT* saved){
    const UINT strategy=saved[6]; // ECX at entry (this)
    UINT wrapper=0;ReadAt(saved[3],8,&wrapper,4); // original [ESP+4], after PUSHFD
    PumpArm155(strategy,wrapper);PumpPinSound156(strategy,wrapper);
    const auto now=GetTickCount64();bool dispatch=false;
    AcquireSRWLockExclusive(&g_manualLock143);
    auto& p=g_pending143;ULONGLONG local=0,owner=0,projectile=0;UINT entity=0;
    const UINT ref=Word(strategy,0xf4),projectileEntity=Word(ref,12),flags=Word(projectileEntity,0x60);
    WORD ready=0;ReadAt(projectileEntity,0x9e,&ready,2);
    const bool nativeReady=projectileEntity&&!(flags&0x4010)&&(flags&0x208)==0x208&&(!(flags&4)||ready);
    if(p.active&&strategy==p.target.strategy){
        ++g_manualStats143.updates148;g_manualStats143.lastFlags148=flags;
        if(!nativeReady)++g_manualStats143.readyBlocks148;
        g_manualStats143.lastGate148=p.scheduled?1:!nativeReady?2:
            strategy!=p.target.strategy||wrapper!=p.target.nativeWrapper149?3:
            !g_context143.active||now<g_context143.ms||now-g_context143.ms>250?4:
            now<p.release.ms||now-p.release.ms>200?5:6;
    }
    if(p.active&&!p.scheduled&&g_nativeRelease146&&InterlockedCompareExchange(&g_enabled,0,0)&&
       strategy==p.target.strategy&&wrapper==p.target.nativeWrapper149&&SameExecution149(p.target)&&Word(strategy)==g_engine+kStrategyVtable&&
       ref==p.target.projectileRef&&ReadAt(projectileEntity,8,&projectile,8)&&projectile==p.target.projectile&&
       PawnIdentity145(Word(strategy,0x38),owner,entity)&&owner==p.target.owner&&LocalIdentity143(local)&&local==owner&&
       g_context143.active&&g_context143.owner==owner&&g_context143.epoch==p.release.epoch&&
       now>=g_context143.ms&&now-g_context143.ms<=250&&now>=p.release.ms&&now-p.release.ms<=200&&nativeReady&&
       (!SameItem155(p.target,g_safe155.target)||g_safe155.armed||g_safe155.loose)){
        p.scheduled=true;dispatch=true;++g_manualStats143.earlyCalls148;g_manualStats143.lastGate148=0;
    }
    LaunchRecord attempt;const bool record=p.active&&strategy==p.target.strategy&&g_manualStats143.updates148<=64;
    if(record){attempt.phase=7;attempt.wallMs=now;attempt.thread=GetCurrentThreadId();attempt.strategy=strategy;attempt.wrapper=wrapper;attempt.projectileRef=ref;attempt.projectileEntity=projectileEntity;
        attempt.archetype=p.target.archetype;attempt.ownerId=p.target.owner;attempt.projectileId=p.target.projectile;attempt.flags=4096;
        attempt.eventWords[0]=flags;attempt.eventWords[1]=nativeReady;attempt.eventWords[2]=g_manualStats143.lastGate148;attempt.eventWords[3]=unsigned(now-g_context143.ms);attempt.eventWords[4]=unsigned(now-p.release.ms);attempt.eventWords[5]=dispatch;attempt.eventWords[6]=p.target.nativeWrapper149;attempt.eventWords[7]=Word(strategy,0x38);
        attempt.origin[0]=float(p.release.origin.x);attempt.origin[1]=float(p.release.origin.y);attempt.origin[2]=float(p.release.origin.z);attempt.velocity[0]=float(p.release.velocity.x);attempt.velocity[1]=float(p.release.velocity.y);attempt.velocity[2]=float(p.release.velocity.z);
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
    if(record&&InterlockedCompareExchange(&g_capture143,0,0)){
        AcquireSRWLockExclusive(&g_queueLock);if(g_queued<64)g_queue[g_queued++]=attempt;else InterlockedIncrement(&g_lost);ReleaseSRWLockExclusive(&g_queueLock);
    }
    // The stock call re-enters the mod's release/vector/dispatch observers. The
    // manual lock must be free; native code retains reference/lifetime ownership.
    if(dispatch){
        __try{g_nativeRelease146(reinterpret_cast<void*>(strategy),wrapper);}
        __except(EXCEPTION_EXECUTE_HANDLER){CancelRelease143();}
    }
}
__declspec(naked) void UpdateBoundary146(){
    __asm pushfd
    __asm pushad
    __asm cld
    __asm mov eax,esp
    __asm sub esp,544
    __asm and esp,0fffffff0h
    __asm mov [esp+512],eax
    __asm fxsave [esp]
    __asm push eax
    __asm call PumpRelease146
    __asm add esp,4
    __asm fxrstor [esp]
    __asm mov esp,[esp+512]
    __asm popad
    __asm popfd
    __asm jmp dword ptr [g_updateOriginal146]
}
bool ReleaseSignatures146(const BYTE* image){
    BYTE update[16],release[16];const UINT base=reinterpret_cast<UINT>(image);
    return Read(base+0x128dae0,update,16)&&Read(base+0x128da40,release,16)&&
        !memcmp(update,kUpdateBytes146,16)&&!memcmp(release,kReleaseBytes146,16)&&
        Word(base+kStrategyVtable,0x4c)==base+0x128dae0&&Word(base+kStrategyVtable,0x2c)==base+0x128da40;
}
void __cdecl Observe(UINT phase,const UINT* saved) {
    if(!InterlockedCompareExchange(&g_enabled,0,0)||phase<1||phase>6)return;
    // pushad layout: EDI, ESI, EBP, savedESP, EBX, EDX, ECX, EAX; EFLAGS.
    const UINT strategy=(phase==1||phase==6)?saved[0]:saved[4];
    if(!g_engine||Word(strategy)!=g_engine+kStrategyVtable)return;
    LaunchRecord r;r.phase=phase;r.wallMs=GetTickCount64();r.thread=GetCurrentThreadId();
    r.strategy=strategy;r.wrapper=Word(strategy,0x38);r.projectileRef=Word(strategy,0xf4);
    const bool ownerValid=PawnIdentity145(r.wrapper,r.ownerId,r.ownerEntity);
    r.projectileEntity=Word(r.projectileRef,0xc);
    if(ReadAt(strategy,0x40,&r.archetype,8))r.flags|=1;
    if(r.wrapper)r.flags|=2;
    if(ownerValid)r.flags|=4;
    if(ReadAt(r.projectileEntity,8,&r.projectileId,8)&&
       ReadAt(r.projectileEntity,0x50,r.projectilePosition,12))r.flags|=8;
    const UINT data=Word(r.wrapper,0x3c);
    BYTE infinite=0,forced=0;
    if(ReadAt(data,0x60,&r.counter,4)&&ReadAt(data,0x64,&infinite,1))r.flags|=16;
    r.infiniteFlag=infinite;
    if(ReadAt(strategy,0x88,r.origin,12)&&ReadAt(strategy,0xc8,r.velocity,12)&&
       ReadAt(strategy,0xd4,r.localAngularImpulse,12))r.flags|=32;
    if(ReadAt(strategy,0x6a,&forced,1))r.flags|=512;
    r.forcePhysics=forced;
    std::memcpy(r.registers,saved,sizeof(r.registers));
    if(phase==3||phase==5) {
        const UINT event=phase==3?saved[1]:saved[0];
        if(Read(event,r.eventWords,sizeof(r.eventWords)))r.flags|=64;
        r.dispatchTarget=saved[6];
    } else if(phase==4)r.dispatchTarget=saved[0];
    AcquireSRWLockShared(&g_poseLock);r.pose=g_pose;ReleaseSRWLockShared(&g_poseLock);
    if(r.pose.wallMs)r.flags|=128;
    // ID filter is usable only when the integration publishes the proven local
    // actor ID. Until then retain candidates, without claiming local ownership.
    if(r.pose.localOwnerId) {
        if(!(r.flags&4)||r.ownerId!=r.pose.localOwnerId)return;
        r.flags|=256;
    }
    ManualObserve143(r,saved);
    if(!InterlockedCompareExchange(&g_capture143,0,0))return;
    InterlockedIncrement(&g_phases[phase-1]);
    AcquireSRWLockExclusive(&g_queueLock);
    if(g_queued<64)g_queue[g_queued++]=r;else InterlockedIncrement(&g_lost);
    ReleaseSRWLockExclusive(&g_queueLock);
#ifdef FC3_THROW_PROBE_TEST
    // ABI fixtures require the bridge to restore deliberately clobbered state.
    __asm { pxor xmm0,xmm0 }
    __asm { fninit }
#endif
}
// These boundaries observe only. Every displaced original instruction executes
// through MinHook after full register/EFLAGS/x87/SSE restoration.
#define THROW_BOUNDARY(name,phase,slot) \
__declspec(naked) void name() { \
    __asm pushfd \
    __asm pushad \
    __asm cld \
    __asm mov eax,esp \
    __asm sub esp,544 \
    __asm and esp,0fffffff0h \
    __asm mov [esp+512],eax \
    __asm fxsave [esp] \
    __asm push eax \
    __asm push phase \
    __asm call Observe \
    __asm add esp,8 \
    __asm fxrstor [esp] \
    __asm mov esp,[esp+512] \
    __asm popad \
    __asm popfd \
    __asm jmp dword ptr [g_original+slot] \
}
THROW_BOUNDARY(Ready,1,0)
THROW_BOUNDARY(Release,2,4)
THROW_BOUNDARY(VelocityEvent,3,8)
THROW_BOUNDARY(PhysicsVelocity,4,12)
THROW_BOUNDARY(LaunchEvent,5,16)
THROW_BOUNDARY(InventoryCommit,6,20)
#undef THROW_BOUNDARY
void* g_bridges[6]={reinterpret_cast<void*>(Ready),reinterpret_cast<void*>(Release),
    reinterpret_cast<void*>(VelocityEvent),reinterpret_cast<void*>(PhysicsVelocity),
    reinterpret_cast<void*>(LaunchEvent),reinterpret_cast<void*>(InventoryCommit)};
}
#include "placeables177_native.inl"
bool Install() {
    if(g_installed)return true;
    if(g_attempted)return false;
    auto* image=reinterpret_cast<BYTE*>(GetModuleHandleW(L"FC3_d3d11.dll"));
    if(!image)return false;
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS32 nt;
    if(!Read(reinterpret_cast<UINT>(image),&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>4096||
       !Read(reinterpret_cast<UINT>(image)+dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||
       nt.FileHeader.TimeDateStamp!=kTimestamp||!SignaturesMatch(image,nt.OptionalHeader.SizeOfImage)||!ReleaseSignatures146(image)||
       memcmp(image+0x128d157,kStockBytes147,sizeof(kStockBytes147))||
       memcmp(image+0x12b1530,kActivateBytes155,sizeof(kActivateBytes155))||
       memcmp(image+0x128db70,kCleanupBytes155,sizeof(kCleanupBytes155))||
       memcmp(image+0x16aab70,kPinSoundBytes156,sizeof(kPinSoundBytes156))||
       memcmp(image+0x12b0780,kTrailBytes176,sizeof(kTrailBytes176))||
       memcmp(image+0x12b1229,kFlameUpdate160,sizeof(kFlameUpdate160))||memcmp(image+0x12b17d2,kFlameSpawn160,sizeof(kFlameSpawn160)))return false;
    g_attempted=true;
    int created=0,enabled=0;
    for(;created<6;++created)if(MH_CreateHook(image+kSites[created],g_bridges[created],&g_original[created])!=MH_OK)break;
    g_engine=reinterpret_cast<UINT>(image);
    InterlockedExchange(&g_enabled,1);
    if(created==6)for(;enabled<6;++enabled)if(MH_EnableHook(image+kSites[enabled])!=MH_OK)break;
    if(created!=6||enabled!=6) {
        InterlockedExchange(&g_enabled,0);
        for(int i=0;i<enabled;++i)MH_DisableHook(image+kSites[i]);
        // Preserve created trampolines during failed installation: a concurrently
        // entered bridge might still need one. Host teardown requires quiescence.
        return false;
    }
    if(MH_CreateHook(image+0x128da40,reinterpret_cast<void*>(ReleaseGate146),&g_releaseOriginal146)!=MH_OK||
       MH_CreateHook(image+0x128dae0,reinterpret_cast<void*>(UpdateBoundary146),&g_updateOriginal146)!=MH_OK||
       MH_EnableHook(image+0x128da40)!=MH_OK||MH_EnableHook(image+0x128dae0)!=MH_OK){
        MH_DisableHook(image+0x128da40);MH_DisableHook(image+0x128dae0);
        InterlockedExchange(&g_enabled,0);for(UINT i=0;i<6;++i)MH_DisableHook(image+kSites[i]);return false;
    }
    g_updateInstalled146=true;g_nativeRelease146=reinterpret_cast<NativeRelease146>(g_releaseOriginal146);
    if(MH_CreateHook(image+0x128d157,reinterpret_cast<void*>(StockBoundary147),&g_stockOriginal147)==MH_OK&&
       MH_EnableHook(image+0x128d157)==MH_OK)g_stockInstalled147=true;
    if(!g_stockInstalled147){g_installed=true;Disable();return false;}
    if(MH_CreateHook(image+0x12b1530,reinterpret_cast<void*>(ActivationBoundary155),&g_activateOriginal155)!=MH_OK||
       MH_CreateHook(image+0x128db70,reinterpret_cast<void*>(CleanupGate155),&g_cleanupOriginal155)!=MH_OK||
       MH_EnableHook(image+0x12b1530)!=MH_OK||MH_EnableHook(image+0x128db70)!=MH_OK){
        MH_DisableHook(image+0x12b1530);MH_DisableHook(image+0x128db70);
        g_installed=true;Disable();return false;
    }
    if(MH_CreateHook(image+0x16aab70,reinterpret_cast<void*>(PinSoundGate156),&g_soundOriginal156)!=MH_OK||MH_EnableHook(image+0x16aab70)!=MH_OK){
        MH_DisableHook(image+0x16aab70);g_safetyInstalled155=true;g_installed=true;Disable();return false;
    }
    g_soundInstalled156=true;
    if(MH_CreateHook(image+0x12b1229,reinterpret_cast<void*>(FlameUpdateBoundary160),&g_flameUpdateOriginal160)!=MH_OK||
       MH_CreateHook(image+0x12b17d2,reinterpret_cast<void*>(FlameSpawnBoundary160),&g_flameSpawnOriginal160)!=MH_OK||
       MH_EnableHook(image+0x12b1229)!=MH_OK||MH_EnableHook(image+0x12b17d2)!=MH_OK){
        MH_DisableHook(image+0x12b1229);MH_DisableHook(image+0x12b17d2);g_safetyInstalled155=true;g_installed=true;Disable();return false;}
    g_flameInstalled160=true;
    if(MH_CreateHook(image+0x12b0780,reinterpret_cast<void*>(TrailGate176),&g_trailOriginal176)!=MH_OK||MH_EnableHook(image+0x12b0780)!=MH_OK){
        MH_DisableHook(image+0x12b0780);g_safetyInstalled155=true;g_installed=true;Disable();return false;}
    g_trailInstalled176=true;g_stockSetter155=g_engine+0x307090;g_safetyInstalled155=true;g_installed=true;
    // Optional distinct placement hooks must never disable accepted throwing.
    InstallPlaceables177(image);return true;
}
void Disable() {
    DisablePlaceables177();
    PublishD50Audio170(0,0,0,0,0,0);
    CancelRelease143();
    InterlockedExchange(&g_enabled,0);
    if(g_safetyInstalled155){MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x12b1530);MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x128db70);}
    if(g_flameInstalled160){MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x12b1229);MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x12b17d2);}g_flameInstalled160=false;
    if(g_trailInstalled176)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x12b0780);g_trailInstalled176=false;
    g_safetyInstalled155=false;
    if(g_soundInstalled156)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x16aab70);g_soundInstalled156=false;
    if(g_stockInstalled147)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x128d157);
    g_stockInstalled147=false;
    if(g_updateInstalled146)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x128dae0);
    if(g_updateInstalled146)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+0x128da40);
    g_updateInstalled146=false;
    if(g_installed)for(UINT i=0;i<6;++i)MH_DisableHook(reinterpret_cast<BYTE*>(g_engine)+kSites[i]);
    // Deliberately retain trampolines; host must quiesce before removing/unloading.
    g_installed=false;
}
void PublishPose(const PoseSample& pose) {
    AcquireSRWLockExclusive(&g_poseLock);g_pose=pose;ReleaseSRWLockExclusive(&g_poseLock);
}
bool FlushTo(HANDLE file) {
    // Serialize flush callers, preserving queue FIFO and a single output writer.
    AcquireSRWLockExclusive(&g_fileLock);
    LaunchRecord batch[64];UINT count;
    AcquireSRWLockExclusive(&g_queueLock);
    count=g_queued;std::memcpy(batch,g_queue,count*sizeof(LaunchRecord));g_queued=0;
    ReleaseSRWLockExclusive(&g_queueLock);
    bool ok=true;
    for(UINT i=0;i<count;++i) {
        if(g_full||g_written>8*1024*1024-sizeof(LaunchRecord)) {
            g_full=true;InterlockedIncrement(&g_lost);continue;
        }
        DWORD written=0;
        if(!WriteFile(file,batch+i,sizeof(LaunchRecord),&written,nullptr)||written!=sizeof(LaunchRecord)) {
            g_writeError=GetLastError();if(!g_writeError)g_writeError=ERROR_WRITE_FAULT;
            InterlockedExchangeAdd(&g_lost,count-i);ok=false;break;
        }
        g_written+=written;
    }
    if(ok&&count&&!FlushFileBuffers(file)) { g_writeError=GetLastError();ok=false; }
    ReleaseSRWLockExclusive(&g_fileLock);return ok;
}
Stats Status() {
    Stats s;AcquireSRWLockShared(&g_queueLock);s.queued=g_queued;ReleaseSRWLockShared(&g_queueLock);
    s.lost=InterlockedCompareExchange(&g_lost,0,0);
    for(UINT i=0;i<6;++i)s.phaseCounts[i]=InterlockedCompareExchange(&g_phases[i],0,0);
    AcquireSRWLockShared(&g_fileLock);s.writeError=g_writeError;s.full=g_full;ReleaseSRWLockShared(&g_fileLock);
    return s;
}
void SetPinIntent155(bool enabled){AcquireSRWLockExclusive(&g_manualLock143);g_pinIntent155=enabled;ReleaseSRWLockExclusive(&g_manualLock143);}
bool PublishMolotovFlame160(const Prepared143& target,const float* matrix){
    bool finite=matrix!=nullptr;if(matrix)for(int i=0;i<16;++i)finite=finite&&std::isfinite(matrix[i]);
    AcquireSRWLockExclusive(&g_manualLock143);const auto now=GetTickCount64();
    const bool ok=finite&&target.archetype==0x000006f6a4abae8dULL&&target.serial&&SamePrepared143(target,g_prepared143)&&g_context143.active&&g_context143.owner==target.owner&&g_context143.epoch==target.epoch;
    if(ok){g_flame160.target=target;g_flame160.ms=now;memcpy(g_flame160.matrix,matrix,64);g_flame160.active=true;}else g_flame160.active=false;
    ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
MolotovChildSnapshot178 InspectMolotovChildren178(const Prepared143& target){
 MolotovChildSnapshot178 out;AcquireSRWLockShared(&g_manualLock143);
 if(SameItem155(target,g_flameReceipt169.target)&&target.epoch==g_flameReceipt169.target.epoch&&target.serial==g_flameReceipt169.target.serial)out=g_flameChildren178;
 ReleaseSRWLockShared(&g_manualLock143);return out;
}
void MolotovFlameStatus160(unsigned& updates,unsigned& spawns){AcquireSRWLockShared(&g_manualLock143);updates=g_flameUpdates160;spawns=g_flameSpawns160;ReleaseSRWLockShared(&g_manualLock143);}
MolotovSnapshot166 InspectMolotov166(const Prepared143& target,UINT knownComponent){
 MolotovSnapshot166 out;if(target.archetype!=0x000006f6a4abae8dULL||!target.projectile||!target.projectileRef)return out;
 AcquireSRWLockShared(&g_manualLock143);ULONGLONG id=0;const UINT entity=Word(target.projectileRef,12);out.entity=ReadAt(entity,8,&id,8)&&id==target.projectile;
 if(out.entity){out.positionValid=ReadAt(entity,0x50,out.position,12);out.heldIdentity=SameItem155(target,g_safe155.target);const UINT component=out.heldIdentity&&g_safe155.component?g_safe155.component:knownComponent;out.component=component&&Word(component)==g_engine+0x1fc67c0&&Word(component,4)==target.projectileRef;
 if(out.component){out.componentAddress=component;BYTE active=0;ReadAt(component,0x234,&active,1);out.lit=active!=0;}}
 const auto& receipt=g_flameReceipt169;
 if(out.entity&&SameItem155(target,receipt.target)&&target.epoch==receipt.target.epoch&&target.serial==receipt.target.serial){memcpy(out.nativeFlame,receipt.source+12,12);memcpy(out.ragTarget,receipt.applied+12,12);out.effectMs=receipt.ms;out.effectPublishedMs169=receipt.publishedMs;out.effectSpawn169=receipt.spawn;memcpy(out.effectSource169,receipt.source,64);memcpy(out.effectApplied169,receipt.applied,64);}
 out.updates=g_flameUpdates160;out.spawns=g_flameSpawns160;out.blocked=g_blockedActivation155;out.armed=g_armed155;ReleaseSRWLockShared(&g_manualLock143);return out;
}
void SetMolotovIntent158(bool enabled){AcquireSRWLockExclusive(&g_manualLock143);g_molotovIntent158=enabled;ReleaseSRWLockExclusive(&g_manualLock143);}
bool IgniteMolotov158(const Prepared143& target){
    AcquireSRWLockExclusive(&g_manualLock143);auto& s=g_safe155;
    const bool ok=target.archetype==0x000006f6a4abae8dULL&&SameItem155(target,s.target)&&SafeIdentity155(s)&&!s.cancelled&&!s.launched&&s.component&&s.eventValid;
    if(ok){s.target.epoch=target.epoch;s.request=true;}
    ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
bool MolotovLit158(const Prepared143& target){return target.archetype==0x000006f6a4abae8dULL&&PinArmed155(target);}
bool RequestPinOut155(const Prepared143& target){
    AcquireSRWLockExclusive(&g_manualLock143);const bool ok=SameItem155(target,g_safe155.target)&&SafeIdentity155(g_safe155)&&!g_safe155.cancelled;
    if(ok){g_safe155.target.epoch=target.epoch;g_safe155.request=true;if(g_pinSound156.owner==target.owner&&g_pinSound156.projectile==target.projectile)g_pinSound156.requested=true;}ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
bool LooseRelease156(const Prepared143& target){
    AcquireSRWLockExclusive(&g_manualLock143);const bool ok=SameItem155(target,g_safe155.target)&&SafeIdentity155(g_safe155)&&!g_safe155.cancelled&&!g_safe155.armed&&!g_safe155.request;
    if(ok)g_safe155.loose=true;
    ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
void PublishD50Audio170(UINT stage,UINT state,UINT wrapper,UINT weapon,UINT epoch,UINT flags){
 ULONGLONG local=0,referenceId=0;UINT actor=0,reference=0;
 AcquireSRWLockShared(&g_manualLock143);const bool localValid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 const bool valid=stage>=1&&stage<=4&&localValid&&D50AudioStructure170(state,wrapper,weapon,actor,reference,referenceId);
 const auto now=GetTickCount64();
 AcquireSRWLockExclusive(&g_d50AudioLock170);auto& c=g_d50AudioContext170;
 UpdateD50Sound225(stage,valid,state,wrapper,weapon,epoch,flags,local,reference,referenceId,now);
 if(!valid)c={};
 else if(stage==1){
  c={};c.active=true;c.updated=now;c.actor=actor;c.actorReference=reference;c.actorReferenceId=referenceId;
  c.value.beginMs=now;c.value.owner=local;c.value.generation=++g_d50AudioGeneration170;
  c.value.epoch=epoch;c.value.stage=1;c.value.flags=flags;c.value.state=state;c.value.wrapper=wrapper;c.value.weapon=weapon;
 }else if(c.active&&c.value.owner==local&&c.value.state==state&&c.value.wrapper==wrapper&&c.value.weapon==weapon&&
          c.value.epoch==epoch&&c.actor==actor&&c.actorReference==reference&&c.actorReferenceId==referenceId&&
          now>=c.value.beginMs&&now-c.value.beginMs<=8000){
  c.updated=now;c.value.flags=flags;if(stage!=4)c.value.stage=stage;
 }else c={};
 ReleaseSRWLockExclusive(&g_d50AudioLock170);
}
bool RequestD50Sound225(UINT state,UINT wrapper,UINT weapon,UINT epoch){return RequestD50SoundImpl225(state,wrapper,weapon,epoch);}
bool PumpD50Sound225(UINT state,UINT wrapper,UINT weapon,UINT epoch){return PumpD50SoundImpl225(state,wrapper,weapon,epoch);}
bool BindD50ManualSound226(UINT state,UINT wrapper,UINT weapon,UINT epoch,bool needsRack){return BindD50ManualSoundImpl226(state,wrapper,weapon,epoch,needsRack);}
UINT DrainD50Audio170(D50AudioRecord170* records,UINT capacity){
 if(!records||!capacity)return 0;
 AcquireSRWLockExclusive(&g_d50AudioLock170);
 const UINT count=capacity<g_d50AudioQueued170?capacity:g_d50AudioQueued170;
 memcpy(records,g_d50AudioQueue170,count*sizeof(*records));g_d50AudioQueued170-=count;
 if(g_d50AudioQueued170)memmove(g_d50AudioQueue170,g_d50AudioQueue170+count,g_d50AudioQueued170*sizeof(*records));
 ReleaseSRWLockExclusive(&g_d50AudioLock170);return count;
}
D50AudioStats170 D50AudioStatus170(){AcquireSRWLockShared(&g_d50AudioLock170);const auto stats=g_d50AudioStats170;ReleaseSRWLockShared(&g_d50AudioLock170);return stats;}
void PinSoundStatus156(unsigned& blocked,unsigned& played){AcquireSRWLockShared(&g_manualLock143);blocked=g_soundBlocked156;played=g_soundPlayed156;ReleaseSRWLockShared(&g_manualLock143);}
bool PinArmed155(const Prepared143& target){AcquireSRWLockShared(&g_manualLock143);const bool armed=SameItem155(target,g_safe155.target)&&g_safe155.armed;ReleaseSRWLockShared(&g_manualLock143);return armed;}
void SafetyStatus155(unsigned& blocked,unsigned& armed,unsigned& stowed){AcquireSRWLockShared(&g_manualLock143);blocked=g_blockedActivation155;armed=g_armed155;stowed=g_safeStows155;ReleaseSRWLockShared(&g_manualLock143);}
void SetIdentityReader143(IdentityReader143 reader){
    AcquireSRWLockExclusive(&g_manualLock143);g_identity143=reader;g_pending143={};g_prepared143={};g_candidate144={};g_manualLaunched146={};g_context143={};ReleaseSRWLockExclusive(&g_manualLock143);
    AcquireSRWLockExclusive(&g_d50AudioLock170);g_d50AudioContext170={};ReleaseSRWLockExclusive(&g_d50AudioLock170);
}
void PublishContext143(ULONGLONG owner,unsigned epoch,bool active){
    AcquireSRWLockExclusive(&g_manualLock143);
    if(!active||!owner||!epoch||!g_context143.active||owner!=g_context143.owner||epoch!=g_context143.epoch){g_pending143={};g_prepared143={};}
    g_context143={owner,GetTickCount64(),epoch,active&&owner!=0&&epoch!=0};ReleaseSRWLockExclusive(&g_manualLock143);
}
bool CurrentPrepared143(Prepared143& out){
    AcquireSRWLockShared(&g_manualLock143);ULONGLONG owner=0;const auto now=GetTickCount64();
    const bool ok=LocalIdentity143(owner)&&owner==g_prepared143.owner&&g_prepared143.serial&&
        g_context143.active&&g_context143.owner==owner&&g_context143.epoch==g_prepared143.epoch&&
        now>=g_context143.ms&&now-g_context143.ms<=250;
    if(ok)out=g_prepared143;ReleaseSRWLockShared(&g_manualLock143);return ok;
}
bool AdoptPrepared144(ULONGLONG archetype,Prepared143& out,ULONGLONG previouslyHeldProjectile){
    AcquireSRWLockExclusive(&g_manualLock143);ULONGLONG owner=0,actualOwner=0,actualProjectile=0,actualArchetype=0;const auto now=GetTickCount64();
    UINT actualEntity=0;auto c=g_candidate144;
    const bool ok=LocalIdentity143(owner)&&g_context143.active&&g_context143.owner==owner&&g_context143.epoch&&
        now>=g_context143.ms&&now-g_context143.ms<=250&&c.owner==owner&&c.archetype==archetype&&
        c.projectile&&c.projectile!=g_retiredProjectile143&&now>=c.ms&&(now-c.ms<=2500||c.projectile==previouslyHeldProjectile)&&
        Word(c.strategy)==g_engine+kStrategyVtable&&Word(c.strategy,0xf4)==c.projectileRef&&SameExecution149(c)&&
        PawnIdentity145(Word(c.strategy,0x38),actualOwner,actualEntity)&&actualOwner==owner&&
        ReadAt(Word(c.projectileRef,12),8,&actualProjectile,8)&&actualProjectile==c.projectile&&
        ReadAt(c.strategy,0x40,&actualArchetype,8)&&actualArchetype==archetype;
    if(ok){c.epoch=g_context143.epoch;c.serial=++g_serial143;g_prepared143=c;out=c;}
    ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
bool CurrentLocalOwner144(ULONGLONG& owner){AcquireSRWLockShared(&g_manualLock143);const bool ok=LocalIdentity143(owner);ReleaseSRWLockShared(&g_manualLock143);return ok;}
bool QueueRelease143(const Prepared143& prepared,const fc3manual::Release& release){
    AcquireSRWLockExclusive(&g_manualLock143);ULONGLONG owner=0;const auto now=GetTickCount64();
    const bool supported=prepared.archetype==0x000006e79d52dd0cULL||prepared.archetype==0x03c006e5ac354e19ULL||prepared.archetype==0x000006f6a4abae8dULL;
    const bool ok=supported&&InterlockedCompareExchange(&g_enabled,0,0)&&!g_pending143.active&&prepared.serial&&
        SamePrepared143(prepared,g_prepared143)&&LocalIdentity143(owner)&&owner==prepared.owner&&release.owner==owner&&
        g_context143.active&&g_context143.owner==owner&&g_context143.epoch==release.epoch&&
        now>=g_context143.ms&&now-g_context143.ms<=250&&
        release.epoch&&release.epoch==prepared.epoch&&now>=release.ms&&now-release.ms<=40&&
        fc3manual::Finite(release.origin)&&fc3manual::Finite(release.velocity)&&fc3manual::Length(release.velocity)<=35;
    if(ok){g_pending143.target=prepared;g_pending143.release=release;g_pending143.active=true;++g_manualStats143.queued;}
    else ++g_manualStats143.rejected;
    ReleaseSRWLockExclusive(&g_manualLock143);return ok;
}
void CancelRelease143(){AcquireSRWLockExclusive(&g_manualLock143);g_pending143={};g_prepared143={};g_context143={};ReleaseSRWLockExclusive(&g_manualLock143);}
bool ReleaseCompleted157(const Prepared143& target,ULONGLONG& launchMs){
    AcquireSRWLockShared(&g_manualLock143);
    const bool done=target.serial&&SamePrepared143(target,g_manualLaunched146);
    if(done)launchMs=g_launchMs157;
    ReleaseSRWLockShared(&g_manualLock143);return done;
}
ManualStats143 ManualStatus143(){AcquireSRWLockShared(&g_manualLock143);auto s=g_manualStats143;s.pending=g_pending143.active;ReleaseSRWLockShared(&g_manualLock143);return s;}
void StopCapture143(){InterlockedExchange(&g_capture143,0);AcquireSRWLockExclusive(&g_queueLock);g_queued=0;ReleaseSRWLockExclusive(&g_queueLock);}
void SetUnlimited147(bool enabled){InterlockedExchange(&g_unlimited147,enabled?1:0);}
bool Unlimited147(){return InterlockedCompareExchange(&g_unlimited147,0,0)!=0;}
unsigned PreservedStock147(){return static_cast<unsigned>(InterlockedCompareExchange(&g_preserved147,0,0));}
}
