// Empty D50 CSoundEvent 173068 occurs at native slide return (2.0667 s).
// Store scalar identity/routing only; never retain the listener, event or slot.
using SoundExecute225=void(__thiscall*)(void*,UINT*,UINT,UINT);
void* g_soundExecute225=nullptr;
struct D50Sound225 {
 UINT state=0,wrapper=0,weapon=0,epoch=0,reference=0,thread=0;ULONGLONG owner=0,referenceId=0,begin=0;
 bool active=false,deferred=false,released=false,played=false;BYTE route=0;
} g_d50Sound225;
bool SoundExecuteReady225(){
 if(g_soundExecute225)return true;
 const BYTE code[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x0c,0x80,0x79,0x28,0,0x53,0x8b,0x59,0x1c};
 BYTE got[sizeof(code)]={};
 if(!g_engine||!g_soundInstalled156||!Read(g_engine+0x16d4100,got,sizeof(got))||memcmp(got,code,sizeof(code)))return false;
 g_soundExecute225=reinterpret_cast<void*>(g_engine+0x16d4100);return true;
}
void UpdateD50Sound225(UINT stage,bool valid,UINT state,UINT wrapper,UINT weapon,UINT epoch,UINT flags,ULONGLONG owner,UINT reference,ULONGLONG referenceId,ULONGLONG now){
 auto& s=g_d50Sound225; // caller owns g_d50AudioLock170
 if(!valid||!stage){s={};return;}
 if(stage==1){
  s={};if(!(flags&1)||!SoundExecuteReady225())return;
  s.active=true;s.state=state;s.wrapper=wrapper;s.weapon=weapon;s.epoch=epoch;s.owner=owner;s.reference=reference;s.referenceId=referenceId;s.begin=now;s.thread=GetCurrentThreadId();
 }else if(s.active&&(s.state!=state||s.wrapper!=wrapper||s.weapon!=weapon||s.epoch!=epoch||s.owner!=owner||s.reference!=reference||s.referenceId!=referenceId||now<s.begin||now-s.begin>60000))s={};
}
bool D50SoundIdentity225(const D50Sound225& s,ULONGLONG local){
 UINT actor=0,reference=0;ULONGLONG id=0,entityId=0;const auto now=GetTickCount64();
 return s.active&&s.owner==local&&now>=s.begin&&now-s.begin<=60000&&
  D50AudioStructure170(s.state,s.wrapper,s.weapon,actor,reference,id)&&reference==s.reference&&id==s.referenceId&&ReadAt(Word(reference,12),8,&entityId,8)&&entityId==local;
}
bool DeferD50Sound225(UINT receiver,UINT firstPerson,UINT event){
 if(Word(event)!=g_engine+0x1f4d618||Word(event,0x18)!=0x173068||Word(event,0x1c)!=15||!firstPerson)return false;
 BYTE route=0,alternate=0;if(!Read(event+0x20,&route,1)||!Read(event+0x28,&alternate,1)||alternate||route>1)return false;
 ULONGLONG local=0,actor=0;UINT reference=0;
 AcquireSRWLockShared(&g_manualLock143);const bool localValid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 if(!localValid||!SoundActor156(receiver,actor,reference)||actor!=local)return false;
 AcquireSRWLockExclusive(&g_d50AudioLock170);auto& s=g_d50Sound225;
 const bool valid=D50SoundIdentity225(s,local)&&reference==s.reference&&s.thread==GetCurrentThreadId();
 bool block=false;
 if(valid){
  if(!s.released){s.route=route;s.deferred=true;block=true;++g_d50AudioStats170.deferred225;}
  else if(s.played)block=true;
  else {s.played=true;} // rack already released: original event is no longer premature
 }
 ReleaseSRWLockExclusive(&g_d50AudioLock170);return block;
}
bool RequestD50SoundImpl225(UINT state,UINT wrapper,UINT weapon,UINT epoch){
 ULONGLONG local=0;AcquireSRWLockShared(&g_manualLock143);const bool valid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 AcquireSRWLockExclusive(&g_d50AudioLock170);auto& s=g_d50Sound225;
 const bool same=valid&&D50SoundIdentity225(s,local)&&s.state==state&&s.wrapper==wrapper&&s.weapon==weapon&&s.epoch==epoch;
 if(same)s.released=true;
 const bool request=same&&s.deferred&&!s.played;
 ReleaseSRWLockExclusive(&g_d50AudioLock170);return request;
}
bool DispatchD50Scalar226(const D50Sound225& sample,UINT sound){
 // Exact scalar CSoundEvent ABI; no animation event/listener is retained.
 UINT token[2]={~0u,~0u};BYTE event[52]={};
 *reinterpret_cast<UINT*>(event)=g_engine+0x1f4d618;
 *reinterpret_cast<UINT*>(event+0x18)=sound;*reinterpret_cast<UINT*>(event+0x1c)=15;event[0x20]=sample.route;
 bool called=false;
 __try{
  if(g_soundExecute225&&sample.thread==GetCurrentThreadId()&&Word(sample.reference,8)>0&&Word(sample.reference,8)<0x100000&&Writable143(sample.reference+8,4)){
   InterlockedIncrement(reinterpret_cast<volatile LONG*>(sample.reference+8));
   reinterpret_cast<SoundExecute225>(g_soundExecute225)(event,token,sample.reference,0);called=true;
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){} // native completion is uncertain: never retry
 return called;
}
bool BindD50ManualSoundImpl226(UINT state,UINT wrapper,UINT weapon,UINT epoch,bool needsRack){
 ULONGLONG local=0,id=0,entityId=0;UINT actor=0,reference=0;
 AcquireSRWLockShared(&g_manualLock143);const bool valid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 if(!valid||!SoundExecuteReady225()||!D50AudioStructure170(state,wrapper,weapon,actor,reference,id)||
    !ReadAt(Word(reference,12),8,&entityId,8)||entityId!=local)return false;
 D50Sound225 sample;sample.active=true;sample.state=state;sample.wrapper=wrapper;sample.weapon=weapon;sample.epoch=epoch;
 sample.owner=local;sample.reference=reference;sample.referenceId=id;sample.thread=GetCurrentThreadId();sample.begin=GetTickCount64();
 sample.route=1;sample.deferred=needsRack; // native MAB route byte is 01; physical release owns the slide event
 AcquireSRWLockExclusive(&g_d50AudioLock170);g_d50Sound225=sample;ReleaseSRWLockExclusive(&g_d50AudioLock170);
 return DispatchD50Scalar226(sample,0x173067); // common partial/empty magazine seating event; no reload timeline
}
bool PumpD50SoundImpl225(UINT state,UINT wrapper,UINT weapon,UINT epoch){
 ULONGLONG local=0;AcquireSRWLockShared(&g_manualLock143);const bool valid=LocalIdentity143(local);ReleaseSRWLockShared(&g_manualLock143);
 D50Sound225 sample;
 AcquireSRWLockExclusive(&g_d50AudioLock170);auto& s=g_d50Sound225;
 const bool same=valid&&D50SoundIdentity225(s,local)&&s.state==state&&s.wrapper==wrapper&&s.weapon==weapon&&s.epoch==epoch;
 const bool play=same&&s.released&&s.deferred&&!s.played&&s.thread==GetCurrentThreadId()&&g_soundExecute225;
 if(play){s.played=true;sample=s;} // consume before native call, never replay twice
 else if(!same&&s.active){s={};++g_d50AudioStats170.cancelled225;}
 ReleaseSRWLockExclusive(&g_d50AudioLock170);
 if(!play)return false;
 const bool called=DispatchD50Scalar226(sample,0x173068);
 AcquireSRWLockExclusive(&g_d50AudioLock170);
 if(called)++g_d50AudioStats170.played225;else ++g_d50AudioStats170.cancelled225;
 ReleaseSRWLockExclusive(&g_d50AudioLock170);return called;
}
