// Read the same beautifier component as script GetBeautifier (676340).
// 194282d reads component+34, then vtable+4 RTTI and compares type+14.
// All calls are read-only, on the existing native input thread. Never retain a pointer.
void* g_beautifierOf128=nullptr;
int g_actionTest128=-1;
volatile LONG g_nativeActionOwned128=0;
bool g_slideBound280=false;
bool g_extraActionBound283=false;
bool ExtraActionSignatures283(const BYTE* base){
 const UINT sites[]={0x3b6661,0x254bf1,0x3b6321,0x3b6361,0x3b68e1};
 const UINT names[]={0x1fe2c28,0x1f89aa8,0x1fe2acc,0x1fe2ae8,0x1fe2d24};
 const UINT types[]={0x2884ce8,0x287dfa0,0x2884e3c,0x2884e58,0x2884c08};
 const char* text[]={"CPawnBeautifierPickupPlayer","CPawnBeautifierCinematicFirst","CPawnBeautifierCarryPickup","CPawnBeautifierCarryDrop","CPawnBeautifierTrainDoor"};
 for(int i=0;i<5;++i)if(base[sites[i]-8]!=0x68||base[sites[i]-1]!=0xbf||
  *reinterpret_cast<const UINT*>(base+sites[i]-7)!=reinterpret_cast<UINT>(base)+names[i]||
  *reinterpret_cast<const UINT*>(base+sites[i])!=reinterpret_cast<UINT>(base)+types[i]||strcmp(reinterpret_cast<const char*>(base+names[i]),text[i]))return false;
 return true;
}
bool SlideSignatures280(const BYTE* base){
 const UINT image=reinterpret_cast<UINT>(base);
 return base[0x254eb9]==0x68&&*reinterpret_cast<const UINT*>(base+0x254eba)==image+0x1f89be0&&
  base[0x254ec0]==0xbf&&*reinterpret_cast<const UINT*>(base+0x254ec1)==image+0x287de84&&
  base[0x254ecb]==0xb8&&*reinterpret_cast<const UINT*>(base+0x254ecc)==image+0x287de84&&
  !strcmp(reinterpret_cast<const char*>(base+0x1f89be0),"CPawnBeautifierSlide");
}
int NativeAction128(const UINT* handle,int* slide280=nullptr,UINT* type283=nullptr)
{
 if(type283)*type283=0;
 if(slide280)*slide280=-1;
 if(g_actionTest128>=0){if(slide280)*slide280=0;return g_actionTest128&0x1f3f;}
 if(!g_beautifierOf128)return -1;
 UINT component=0,h0=handle[0],h1=handle[1];void* query=g_beautifierOf128;
 __try {
  __asm {
   push h1
   push h0
   call query
   add esp,8
   mov component,eax
  }
  if(!component)return -1;
  const UINT current=*reinterpret_cast<UINT*>(component+0x34);
  if(!current)return -1;
  const UINT table=*reinterpret_cast<UINT*>(current),getType=*reinterpret_cast<UINT*>(table+4);
  if(getType<g_engine||getType-g_engine>=g_engineBytes)return -1;
  UINT descriptorType=0;
  __asm {
   mov ecx,current
   call getType
   mov descriptorType,eax
  }
  if(!descriptorType||*reinterpret_cast<UINT*>(descriptorType+4)<4)return -1;
  if(type283&&descriptorType>=g_engine&&descriptorType-g_engine<g_engineBytes)*type283=descriptorType-g_engine;
  const UINT id=*reinterpret_cast<UINT*>(descriptorType+0x14);
  if(g_extraActionBound283){
   const UINT types[]={0x2884ce8,0x287dfa0,0x2884e3c,0x2884e58,0x2884c08};
   for(int i=0;i<5;++i)if(descriptorType==g_engine+types[i])return 256<<i;
  }
  // Slide is a separate camera owner, not a broad "no control" arm action.
  if(slide280&&g_slideBound280)*slide280=descriptorType==g_engine+0x287de84?1:0;
  // Only specific actions: climbing, zipline, takedown, MP takedown, ladder, vault.
  // LIVE FAILED129: generic FirstNoControl (old bit64) is also active during ordinary play.
  const UINT descriptors[]={0x2884e20,0x28855c4,0x2885758,0x2884c24,0x287df64,0x287de68};
  for(int i=0;i<6;++i){const auto* descriptor=reinterpret_cast<const UINT*>(g_engine+descriptors[i]);
   if(descriptor[1]>=4&&descriptor[5]&&descriptor[5]==id)return 1<<i;}
  return 0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
bool ActionSignatures128(const BYTE* base)
{
 auto target=[&](UINT site)->UINT{if(base[site]!=0xe8)return 0;int rel=0;memcpy(&rel,base+site+1,4);return site+5+rel;};
 const BYTE prologue[]={0x55,0x8b,0xec,0x83,0xec,0x08,0x8b,0x45,0x0c,0x8b,0x4d,0x08};
 const BYTE active[]={0x8b,0x70,0x34,0x85,0xf6,0x74,0x24};
 if(target(0x6763c9)!=0x657cc0||memcmp(base+0x657cc0,prologue,sizeof(prologue))||
    memcmp(base+0x194282d,active,sizeof(active)))return false;
 const UINT registrations[]={0x3b63a1,0x3b69e1,0x3b6861,0x3b68a1,0x254cf1,0x254f01};
 const UINT descriptors[]={0x2884e20,0x28855c4,0x2885758,0x2884c24,0x287df64,0x287de68};
 for(int i=0;i<6;++i){UINT address=0;memcpy(&address,base+registrations[i],4);if(address!=reinterpret_cast<UINT>(base)+descriptors[i])return false;}
 return true;
}

