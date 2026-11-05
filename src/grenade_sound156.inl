// The installed grfra prime MAB has one CSoundEvent: PlaySound F6E9,
// sound type8. Defer that exact local-player animation event, not all audio.
constexpr BYTE kPinSoundBytes156[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x0c,0x53,0x8b,0xd9,0x8a,0x43,0x37,0x56};
using NativeSound156=void(__thiscall*)(void*,UINT,UINT,UINT);
void* g_soundOriginal156=nullptr;
bool g_soundInstalled156=false;
struct PinSound156 {
    ULONGLONG owner=0,projectile=0,ms=0;
    UINT self=0,reference=0,index=0,firstPerson=0;
    BYTE event[52]={};bool requested=false,played=false;
}g_pinSound156;
unsigned g_soundBlocked156=0,g_soundPlayed156=0;
unsigned g_soundSamples156=0;
bool SoundActor156(UINT self,ULONGLONG& owner,UINT& reference){
    reference=self>=8?Word(self-8):0;
    return ReadAt(Word(reference,12),8,&owner,8)&&owner&&owner!=~0ULL;
}
#include "d50_audio170.inl"
#include "d50_sound225.inl"
void BindPinSound156(const Prepared143& t){
    if(g_pinSound156.owner==t.owner&&!g_pinSound156.played&&GetTickCount64()-g_pinSound156.ms<1500)
        g_pinSound156.projectile=t.projectile;
}
void __fastcall PinSoundGate156(void* self,void*,UINT index,UINT firstPerson,UINT event){
    ObserveD50Audio170(reinterpret_cast<UINT>(self),index,firstPerson,event);
    bool suppress=DeferD50Sound225(reinterpret_cast<UINT>(self),firstPerson,event),record=false;LaunchRecord sample;const UINT receiver=reinterpret_cast<UINT>(self);
    // The real Molotov prime clip plays this lighting sequence9ms after draw.
    // Manual lighter ownership replaces that sequence; drawing an unlit bottle
    // must not play it. Do not mute cook/throw sounds or NPCs by family/shader.
    if(Word(event)==g_engine+0x1f4d618&&Word(event,0x18)==0x18975a&&Word(event,0x1c)==15){
        AcquireSRWLockShared(&g_manualLock143);ULONGLONG actor=0,local=0;UINT reference=0;
        suppress=g_molotovIntent158&&InterlockedCompareExchange(&g_enabled,0,0)&&LocalIdentity143(local)&&SoundActor156(receiver,actor,reference)&&
            (actor==local||(g_safe155.target.archetype==0x000006f6a4abae8dULL&&SafeIdentity155(g_safe155)&&actor==g_safe155.target.nativeEntityId149));
        ReleaseSRWLockShared(&g_manualLock143);
    }
    if(Word(event)==g_engine+0x1f4d618&&Word(event,0x18)==0xf6e9&&Word(event,0x1c)==8){
        AcquireSRWLockExclusive(&g_manualLock143);
        ULONGLONG actor=0,local=0;UINT reference=0;
        const bool identity=LocalIdentity143(local)&&SoundActor156(receiver,actor,reference);
        if(g_soundSamples156++<8){record=true;sample.phase=8;sample.wallMs=GetTickCount64();sample.thread=GetCurrentThreadId();sample.ownerId=actor;sample.projectileId=g_safe155.target.projectile;
            sample.eventWords[0]=0xf6e9;sample.eventWords[1]=8;sample.eventWords[2]=reference;sample.eventWords[3]=index;sample.eventWords[4]=firstPerson;sample.eventWords[5]=g_pinIntent155;sample.eventWords[6]=UINT(local);sample.eventWords[7]=UINT(local>>32);}
        if(g_pinIntent155&&InterlockedCompareExchange(&g_enabled,0,0)&&identity&&
           (actor==local||actor==g_safe155.target.nativeEntityId149)){
            PinSound156 next;next.owner=local;next.ms=GetTickCount64();next.self=receiver;next.reference=reference;next.index=index;next.firstPerson=firstPerson;
            if(Read(event,next.event,sizeof(next.event))){
                if(SafeIdentity155(g_safe155))next.projectile=g_safe155.target.projectile;
                g_pinSound156=next;suppress=true;++g_soundBlocked156;
            }
        }
        ReleaseSRWLockExclusive(&g_manualLock143);
    }
    if(record&&InterlockedCompareExchange(&g_capture143,0,0)){AcquireSRWLockExclusive(&g_queueLock);if(g_queued<64)g_queue[g_queued++]=sample;ReleaseSRWLockExclusive(&g_queueLock);}
    if(!suppress&&g_soundOriginal156)reinterpret_cast<NativeSound156>(g_soundOriginal156)(self,index,firstPerson,event);
}
void PumpPinSound156(UINT strategy,UINT wrapper){
    PinSound156 sound;bool play=false;
    AcquireSRWLockExclusive(&g_manualLock143);
    auto& s=g_pinSound156;ULONGLONG actor=0;UINT reference=0;const auto now=GetTickCount64();
    if(s.requested&&!s.played&&s.projectile==g_safe155.target.projectile&&SafeIdentity155(g_safe155)&&
       strategy==g_safe155.target.strategy&&wrapper==g_safe155.target.nativeWrapper149&&
       g_context143.active&&g_context143.owner==s.owner&&g_context143.epoch==g_safe155.target.epoch&&now>=g_context143.ms&&now-g_context143.ms<250&&
       SoundActor156(s.self,actor,reference)&&reference==s.reference&&
       (actor==s.owner||actor==g_safe155.target.nativeEntityId149)&&g_soundOriginal156){
        s.played=true;sound=s;play=true;++g_soundPlayed156;
    }
    ReleaseSRWLockExclusive(&g_manualLock143);
    if(play)reinterpret_cast<NativeSound156>(g_soundOriginal156)(reinterpret_cast<void*>(sound.self),sound.index,sound.firstPerson,reinterpret_cast<UINT>(sound.event));
}
