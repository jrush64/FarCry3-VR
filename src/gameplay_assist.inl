// Native cheat_GodMode metadata registers an int at cheat-settings +0x48.
// Registration RVA 132c342, constructor 1329f60, singleton store 26577a.
// No damage-function patch or trainer hook. Resolve and validate afresh each tick.
bool g_infiniteHealth=true, g_redDot=true;
// 210: native mode1 is the invulnerable mode. IsInvulnerable 10CE420 tests ==1
// (10CE4D7) and Jason's ApplyDamage 10D0F40 exits on it; the engine forces 1
// itself (1329990, 1112C72). Mode2 (the demo command 1FDBD8C) lets hits land
// and refills later, so both options now request 1; God mode adds GOD210.
bool g_godMode123=true,g_godModeSaveFailed123=false;
void LoadGodMode123(const wchar_t* path){g_godMode123=GetPrivateProfileIntW(L"Gameplay",L"GodMode",1,path)!=0;}
bool SaveGodMode123(const wchar_t* path){return WritePrivateProfileStringW(L"Gameplay",L"GodMode",g_godMode123?L"1":L"0",path)!=0;}
#include "reticle_colour52.inl"
float g_redDotNdc[2][2]={};bool g_redDotValid[2]={};
UINT g_assistTestBase=0;
volatile LONG* GodModeSetting(UINT base)
{
 if(!base)return nullptr;
 __try {
  const BYTE* reg=reinterpret_cast<const BYTE*>(base+0x132c342);
  if(reg[0]!=0x6a||reg[1]!=0x48||reg[2]!=0x68||
     *reinterpret_cast<const UINT*>(reg+3)!=base+0x1fb8418||
     memcmp(reinterpret_cast<const void*>(base+0x1fb8418),"GodMode",8))return nullptr;
  const BYTE* ctor=reinterpret_cast<const BYTE*>(base+0x1329f6a);
  if(ctor[0]!=0xc7||ctor[1]!=0x00||*reinterpret_cast<const UINT*>(ctor+2)!=base+0x1fb8b1c)return nullptr;
  const BYTE* store=reinterpret_cast<const BYTE*>(base+0x26577a);
  if(store[0]!=0xa3||*reinterpret_cast<const UINT*>(store+1)!=base+0x282a648)return nullptr;
  const UINT object=*reinterpret_cast<const UINT*>(base+0x282a648);
  if(!object||(object&3)||*reinterpret_cast<const UINT*>(object)!=base+0x1fb8b1c)return nullptr;
  auto* setting=reinterpret_cast<volatile LONG*>(object+0x48);
  if(*setting<0||*setting>2)return nullptr;
  return setting;
 } __except(EXCEPTION_EXECUTE_HANDLER) {return nullptr;}
}
volatile LONG* g_healthOwned=nullptr;LONG g_healthOriginal=0;
LONG g_healthWritten123=1;
int g_healthStatus=0; // 0 waiting, 1 enabled, 2 disabled, 3 unavailable
void TickGameplayAssist()
{
 const UINT base=g_assistTestBase?g_assistTestBase:reinterpret_cast<UINT>(GetModuleHandleW(L"FC3_d3d11.dll"));
 auto* setting=GodModeSetting(base);
 // Never dereference a cached pointer across a level/settings-object lifetime.
 if(setting!=g_healthOwned){g_healthOwned=nullptr;g_healthOriginal=0;}
 const LONG requested=(g_godMode123||g_infiniteHealth)?1:0;
 fc3aim::SetGodMode210(g_godMode123);
 int status=requested?0:2;
 if(setting) {
  __try {
   if(requested) {
    if(!g_healthOwned){g_healthOriginal=*setting;g_healthOwned=setting;}
    InterlockedExchange(setting,requested);g_healthWritten123=requested;status=1;
   } else if(g_healthOwned) {
    InterlockedCompareExchange(setting,g_healthOriginal,g_healthWritten123);g_healthOwned=nullptr;
   }
  } __except(EXCEPTION_EXECUTE_HANDLER) {g_healthOwned=nullptr;status=3;}
 }
 static LONG lastRequested=-1;
 if(status!=g_healthStatus||requested!=lastRequested){g_healthStatus=status;lastRequested=requested;Log("[ASSIST123] Jason protection %s; native cheat_GodMode %ld",status==1?"applied":status==2?"off":status==3?"unavailable":"waiting for settings",requested);}
}
