// Passive native healing evidence. No synthetic inputs, counter writes, stock
// changes, native requests or animation interception. Original calls always run.
#include "heal_contract299.h"
namespace {
__declspec(align(8)) volatile LONGLONG g_healLocal299=0;
volatile LONG g_healUntil299=0,g_healSequence299=0,g_healRecords299=0;
using HealCounter299=UINT(__thiscall*)(void*,UINT);
using HealEvent299=UINT(__thiscall*)(void*,UINT,UINT);
HealCounter299 g_healCounter299=nullptr;HealEvent299 g_healEvent299=nullptr;
bool g_healInstalled299=false,g_healAttempted299=false;
SRWLOCK g_healReadLock299=SRWLOCK_INIT;LONG g_healLastSequence299=-1;UINT g_healLastComponent299=0;float g_healLastCurrent299=0,g_healLastMaximum299=0;
__declspec(thread) bool g_healInside299=false;
struct HealReading299 {float current=0,maximum=0;UINT counter=0,id=0;bool valid=false;};
bool HealOwner299(UINT component){
 const UINT reference=MuzzleRead58(component,4);unsigned long long identity=0;
 return reference&&MuzzleCopy58(reference,&identity,8)&&identity&&identity==static_cast<unsigned long long>(InterlockedCompareExchange64(&g_healLocal299,0,0));
}
HealReading299 HealRead299(UINT component){
 HealReading299 out;out.counter=MuzzleRead58(component,0x2c);
 float values[2];if(!out.counter||!MuzzleCopy58(out.counter+0x10,values,sizeof(values)))return out;
 out.current=values[0];out.maximum=values[1];out.id=MuzzleRead58(out.counter,8);
 out.valid=std::isfinite(out.current)&&std::isfinite(out.maximum)&&out.maximum>0&&out.maximum<=1000000&&out.current>=0&&out.current<=out.maximum+.01f;return out;
}
bool HealWindow299(){const LONG until=InterlockedCompareExchange(&g_healUntil299,0,0);return until&&static_cast<LONG>(until-GetTickCount())>0;}
void BeginHeal299(){
 if(!HealWindow299()){const LONG sequence=InterlockedIncrement(&g_healSequence299);InterlockedExchange(&g_healRecords299,0);fc3::Log("[HEAL299] seq=%ld bounded native observation armed; input/event request is not heal completion",sequence);}
 InterlockedExchange(&g_healUntil299,static_cast<LONG>(GetTickCount()+12000));
}
UINT __fastcall ObserveHealCounter299(void* component,void*,UINT counterEvent){
 if(!g_healCounter299)return 0;
 if(g_healInside299||!HealWindow299()||!HealOwner299(reinterpret_cast<UINT>(component)))return g_healCounter299(component,counterEvent);
 g_healInside299=true;const auto before=HealRead299(reinterpret_cast<UINT>(component));
 const UINT result=g_healCounter299(component,counterEvent);const auto after=HealRead299(reinterpret_cast<UINT>(component));
 if(before.valid&&after.valid&&before.counter==after.counter&&before.id==after.id&&MuzzleRead58(counterEvent,0x18)==after.id&&TryAcquireSRWLockExclusive(&g_healReadLock299)){
  const LONG sequence=InterlockedCompareExchange(&g_healSequence299,0,0);const UINT self=reinterpret_cast<UINT>(component);
  if(sequence!=g_healLastSequence299||self!=g_healLastComponent299||std::fabs(after.current-g_healLastCurrent299)>.0001f||std::fabs(after.maximum-g_healLastMaximum299)>.0001f){
   if(InterlockedIncrement(&g_healRecords299)<=64)fc3::Log("[HEAL299] seq=%ld local matching counter event id=%08X value=%.6f/%.6f callbackBefore=%.6f/%.6f component=%08X; native event ID matched, stock consumption unverified",sequence,after.id,after.current,after.maximum,before.current,before.maximum,self);
   g_healLastSequence299=sequence;g_healLastComponent299=self;g_healLastCurrent299=after.current;g_healLastMaximum299=after.maximum;
  }ReleaseSRWLockExclusive(&g_healReadLock299);
 }
 g_healInside299=false;return result;
}
UINT __fastcall ObserveHealEvent299(void* event,void*,UINT context,UINT extra){
 if(!g_healEvent299)return 0;
 const UINT actor=MuzzleRead58(context,4),type=MuzzleRead58(reinterpret_cast<UINT>(event),0x10);const bool local=HealOwner299(actor);
 if(local&&type==0)BeginHeal299();
 if(local&&HealWindow299()&&InterlockedIncrement(&g_healRecords299)<=64)
  fc3::Log("[HEAL299] seq=%ld native CGOStateEventHeal request=%u actor=%08X context=%08X; original forwarded unchanged, request is not heal completion",InterlockedCompareExchange(&g_healSequence299,0,0),type,actor,context);
 return g_healEvent299(event,context,extra);
}
bool HealContract299(const fc3heal299::Contract& contract){
 UINT hash=2166136261u;
 for(UINT i=0;i<contract.bytes;++i){BYTE value=0;bool skip=false;for(UINT j=0;j<contract.count;++j)if(i>=contract.skip[j]&&i-contract.skip[j]<4)skip=true;
  if(!skip&&!MuzzleCopy58(g_engine+contract.rva+i,&value,1))return false;hash=(hash^value)*16777619u;}
 return hash==contract.hash;
}
void InstallHealObserver299(){
 if(!g_engine||g_healAttempted299)return;g_healAttempted299=true;
 char name[20]={};if(!MuzzleCopy58(g_engine+0x1fe98d8,name,19)||strcmp(name,"CGOStateEventHeal")||
    !HealContract299(fc3heal299::CounterContract)||!HealContract299(fc3heal299::EventContract)){
  fc3::Log("[HEAL299] native observer contract refused; normal healing unchanged");return;}
 auto* counter=reinterpret_cast<void*>(g_engine+fc3heal299::CounterContract.rva);auto* event=reinterpret_cast<void*>(g_engine+fc3heal299::EventContract.rva);
 if(MH_CreateHook(counter,reinterpret_cast<void*>(ObserveHealCounter299),reinterpret_cast<void**>(&g_healCounter299))!=MH_OK)return;
 if(MH_CreateHook(event,reinterpret_cast<void*>(ObserveHealEvent299),reinterpret_cast<void**>(&g_healEvent299))!=MH_OK){MH_RemoveHook(counter);g_healCounter299=nullptr;return;}
 const bool a=MH_EnableHook(counter)==MH_OK,b=MH_EnableHook(event)==MH_OK;
 if(!a||!b){MH_DisableHook(counter);MH_DisableHook(event);fc3::Log("[HEAL299] observer enable failed; disabled with trampolines retained for in-flight calls");return;}
 g_healInstalled299=true;fc3::Log("[HEAL299] passive local native counter/event observer ready; normal hold-Y, animations, stock and weapons unchanged");
}
}
namespace fc3aim {void BeginHealObservation299(){BeginHeal299();}}
