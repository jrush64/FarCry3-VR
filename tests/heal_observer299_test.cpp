#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
UINT g_engine=0;int logs299=0,counterCalls299=0,eventCalls299=0;UINT lastPayload299=0,lastContext299=0,lastExtra299=0;void* lastThis299=nullptr;
namespace fc3 {void Log(const char*,...){++logs299;}}
UINT MuzzleRead58(UINT a,UINT off=0){UINT value=0;if(!a||off>UINT(-1)-a)return 0;__try {memcpy(&value,(void*)(a+off),4);}__except(EXCEPTION_EXECUTE_HANDLER){}return value;}
bool MuzzleCopy58(UINT a,void* out,UINT bytes){if(!a||bytes>UINT(-1)-a)return false;__try{memcpy(out,(void*)a,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
extern "C" MH_STATUS WINAPI MH_CreateHook(LPVOID,LPVOID,LPVOID*){return MH_ERROR_UNSUPPORTED_FUNCTION;}
extern "C" MH_STATUS WINAPI MH_EnableHook(LPVOID){return MH_ERROR_NOT_CREATED;}
extern "C" MH_STATUS WINAPI MH_DisableHook(LPVOID){return MH_ERROR_NOT_CREATED;}
extern "C" MH_STATUS WINAPI MH_RemoveHook(LPVOID){return MH_ERROR_NOT_CREATED;}
#include "../src/heal_observer299.inl"
void Check(bool ok,const char* why){if(!ok){fprintf(stderr,"FAIL: %s\n",why);exit(1);}printf("PASS: %s\n",why);}
UINT __fastcall Counter299(void* self,void*,UINT argument){++counterCalls299;lastThis299=self;lastPayload299=argument;float* values=(float*)MuzzleRead58((UINT)self,0x2c);if(values)values[4]+=1;return 0x778899aa;}
UINT __fastcall Event299(void* self,void*,UINT context,UINT extra){++eventCalls299;lastThis299=self;lastContext299=context;lastExtra299=extra;return 0x12345678;}
int main(){
 UINT owner[4]={101,202,0,123};BYTE component[0x50]={},event[0x20]={};float health[10]={};health[4]=2;health[5]=6;((UINT*)health)[2]=77;*(UINT*)(component+4)=(UINT)owner;*(UINT*)(component+0x2c)=(UINT)health;UINT context[2]={0,(UINT)component};
 InterlockedExchange64(&g_healLocal299,(LONGLONG(202)<<32)|101);g_healCounter299=reinterpret_cast<HealCounter299>(Counter299);g_healEvent299=reinterpret_cast<HealEvent299>(Event299);BeginHeal299();logs299=0;UINT counterPayload[7]={};counterPayload[6]=77;
 UINT result=ObserveHealCounter299(component,nullptr,(UINT)counterPayload);Check(result==0x778899aa&&counterCalls299==1&&lastThis299==component&&lastPayload299==(UINT)counterPayload,"native counter thiscall original receives exact self/payload once and preserves return register");Check(health[4]==3&&health[5]==6&&logs299==1,"only original modifies counter; passive observer logs the resulting local delta");
 const int before=logs299;owner[0]=999;ObserveHealCounter299(component,nullptr,9);Check(counterCalls299==2&&logs299==before&&health[4]==4,"nonlocal actor still receives original call without local evidence");owner[0]=101;
 InterlockedExchange(&g_healUntil299,0);ObserveHealCounter299(component,nullptr,10);Check(counterCalls299==3&&logs299==before,"outside capture window original runs without logging or interception");
 *(UINT*)(event+0x10)=0;result=ObserveHealEvent299(event,nullptr,(UINT)context,0x77665544);Check(result==0x12345678&&eventCalls299==1&&lastThis299==event&&lastContext299==(UINT)context&&lastExtra299==0x77665544&&HealWindow299(),"native CGO heal event forwards both original stack arguments/result and arms bounded observation");
 *(UINT*)(event+0x10)=1;result=ObserveHealEvent299(event,nullptr,(UINT)context,0xfedcba98);Check(result==0x12345678&&eventCalls299==2&&lastExtra299==0xfedcba98,"finish event remains game-owned and forwarded exactly once");
 auto reading=HealRead299((UINT)component);Check(reading.valid&&reading.current==5&&reading.maximum==6&&reading.id==77,"candidate native health counter read is bounded and retains its counter ID");health[4]=NAN;Check(!HealRead299((UINT)component).valid,"nonfinite counter cannot become healing evidence");health[4]=5;
 Check(!HealOwner299(0)&&!HealRead299(0).valid,"null/unreadable native owner and counter are clean no-ops");g_healInside299=true;const int oldLogs=logs299;ObserveHealCounter299(component,nullptr,11);g_healInside299=false;Check(counterCalls299==4&&logs299==oldLogs,"reentrant callback always forwards original without recursive observation");
 BYTE fixture[8]={1,2,3,4,5,6,7,8};g_engine=(UINT)fixture;unsigned skip[1]={2};unsigned hash=2166136261u;for(int i=0;i<8;++i)hash=(hash^(i>=2&&i<6?0:fixture[i]))*16777619u;fc3heal299::Contract contract={0,8,hash,skip,1};Check(HealContract299(contract),"contract fingerprints include every non-relocated native byte");fixture[3]=100;Check(HealContract299(contract),"ASLR relocation bytes do not invalidate exact native contract");fixture[0]^=1;Check(!HealContract299(contract),"changed native instructions refuse observer installation");
 puts("PASS:299 passive healing observer original-call ABI, local ownership, bounds, no counter/stock writes and relocation-aware contracts; no engine calls/game launched");
}
