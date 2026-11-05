#include "selector290.h"
namespace {
#include "dual_audit362.inl"
SRWLOCK g_selectorLock290=SRWLOCK_INIT;
fc3select290::Inventory g_selectorInventory290;
struct SelectionRequest290 {fc3select290::Inventory shown;int slot=-1;ULONGLONG at=0;bool sent=false;} g_selectorRequest290;
volatile LONG g_selectorLease290=0,g_selectorResult290=0;
volatile LONG g_selectorWatch330=0;
bool SelectorCall291(UINT site,UINT target){BYTE opcode=0;int relative=0;return MuzzleCopy58(g_engine+site,&opcode,1)&&opcode==0xe8&&MuzzleCopy58(g_engine+site+1,&relative,4)&&site+5+relative==target;}
bool SelectorSignatures290(){
 if(!g_engine)return false;
 __try {
  const auto* b=reinterpret_cast<const BYTE*>(g_engine);
  UINT hash=2166136261u;for(UINT i=0;i<0x14b;++i)hash=(hash^b[0x131c020+i])*16777619u;
  return hash==0x93ccf80du&&SelectorCall291(0x6707e3,0x66e540)&&SelectorCall291(0x66e57b,0x198ad60)&&SelectorCall291(0x198ae2a,0x131c020)&&
   !memcmp(b+0x13169e0,"\x55\x8b\xec\x83\xe4\xf8\x83\xec\x34",9)&&
   !memcmp(b+0x131c020,"\x55\x8b\xec\x8b\x45\x08\x83\xec\x10",9)&&
   b[0x1b26a49]==0x68&&MuzzleRead58(g_engine+0x1b26a4a)==g_engine+0x6706d0&&
   !memcmp(b+0x208cfd4,"ForceDrawWeapon",16)&&
   !memcmp(b+0x131c0fc,"\x89\x8f\xb8\x00\x00\x00",6)&&
   !memcmp(b+0x1989837,"\x8b\x40\x24\x52\x05\xe4\x0b\x00\x00",9)&&
   b[0x1989840]==0x68&&MuzzleRead58(g_engine+0x1989841)==g_engine+0x219d1b0&&
   !memcmp(b+0x2049b2c,"select_weapon_slot0",19)&&
   !memcmp(b+0x198ae26,"\x6a\x00\x50\x53\xe8",5);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
UINT SelectorSlot290(UINT inventory,int slot){
 UINT result=0;void* fn=reinterpret_cast<void*>(g_engine+0x13169e0);
 const UINT category=g_engine+0x219d1b0;
 __try {__asm {push slot
  push category
  mov eax,inventory
  call fn
  mov result,eax}}
 __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
 return result;
}
bool SelectorString290(UINT string,char (&out)[80]){
 if(!string)return false;
 for(unsigned i=0;i<79;++i){unsigned char c=0;if(!MuzzleCopy58(string+0xc+i,&c,1)||c>126||(!c&&i==0)||(c&&c<32)){out[0]=0;return false;}
  out[i]=char(c);if(!c)return true;}
 out[0]=0;return false;
}
void SelectorName290(UINT wrapper,char (&out)[80]){
 // CWeapon -> template -> properties, registered unique weapon name at +4C.
 const UINT properties=MuzzleRead58(MuzzleRead58(wrapper,0x48),0x64);
 const UINT string=MuzzleRead58(properties,0x4c);
 if(SelectorString290(string,out))return;
 // 131CC00..131CC16: native weapon entity's own name, same string layout.
 const UINT entity=MuzzleRead58(MuzzleRead58(wrapper,4),0xc);
 if(SelectorString290(MuzzleRead58(entity,0x10),out)){
  const char* suffix=out;for(const char* c=out;*c;++c)if(*c=='/'||*c==':'||*c=='\\')suffix=c+1;
  if(suffix!=out)memmove(out,suffix,strlen(suffix)+1);return;
 }
 out[0]=0;
}
void LockerNative329(const UINT* handle,UINT pawn,ULONGLONG now){
 static fc3locker329::Sampler sampler;static fc3locker329::Budget budget;if(!sampler.Due(now)||!handle||!pawn||!SelectorSignatures290())return;
 fc3locker329::Native n;n.inv.at=now;n.inv.owner=static_cast<unsigned long long>(handle[0])|(static_cast<unsigned long long>(handle[1])<<32);
 const LONG lease=InterlockedCompareExchange(&g_selectorLease290,0,0);n.lease=lease&&static_cast<LONG>(GetTickCount()-lease)<=150;
 const UINT owner=MuzzleRead58(pawn,0x24),inventory=owner?owner+0xbe4:0;if(!inventory)return;
 n.action=NativeAction128(handle);NativeVehicle(handle,n.seat);
 n.inv.valid=true;n.inv.equipped=MuzzleRead58(MuzzleRead58(inventory,0xb4),0xc);
 for(int i=0;i<4;++i){auto& w=n.inv.weapons[i];w.wrapper=SelectorSlot290(inventory,i);if(!w.wrapper)continue;const UINT ref=MuzzleRead58(w.wrapper,4);w.entity=MuzzleRead58(ref,0xc);
  if(!w.entity||!MuzzleCopy58(ref,&w.id,8)||!w.id||w.id==~0ULL)n.inv.valid=false;SelectorName290(w.wrapper,w.name);n.weapon[i]=MuzzleRead58(w.wrapper,0x48);n.properties[i]=MuzzleRead58(n.weapon[i],0x64);
 }
 if(!fc3locker329::Publish(n)||!budget.Take(256))return;
 fc3::Log("[LOCKER329 NATIVE] ms=%llu revision=%u owner=%016llX valid=%d equipped=%08X interactionLease=%d action=%d seat=%d; observation only",now,n.revision,n.inv.owner,n.inv.valid,n.inv.equipped,n.lease,n.action,n.seat);
 for(int i=0;i<4;++i){const auto& w=n.inv.weapons[i];fc3::Log("[LOCKER329 SLOT] revision=%u slot=%d name=%s entity=%08X id=%016llX wrapper=%08X weapon=%08X properties=%08X nativeObjectPresent=%d",n.revision,i+1,w.name,w.entity,w.id,w.wrapper,n.weapon[i],n.properties[i],n.weapon[i]&&n.properties[i]);}
}
void SelectorNative290(const UINT* handle,UINT pawn){
 const ULONGLONG now=GetTickCount64();
 LockerNative329(handle,pawn,now); // observes locker transactions even without interaction permission
 const LONG lease=InterlockedCompareExchange(&g_selectorLease290,0,0);
 const LONG watch=InterlockedCompareExchange(&g_selectorWatch330,0,0);
 const bool interaction=lease&&static_cast<LONG>(GetTickCount()-lease)<=150;
 if(!interaction&&(!watch||static_cast<LONG>(GetTickCount()-watch)>150))return;
 static ULONGLONG last=0;if(now-last<25)return;last=now;
 fc3select290::Inventory fresh;fresh.at=now;
 fresh.owner=static_cast<unsigned long long>(handle[0])|(static_cast<unsigned long long>(handle[1])<<32);
 const UINT owner=MuzzleRead58(pawn,0x24),inventory=owner?owner+0xbe4:0;
 int seat=-1;
 const bool allowed=SelectorSignatures290()&&inventory&&NativeVehicle(handle,seat)==0;
 // Watching inventory grants no native draw permission. Script/action ownership
 // and the separate interaction lease are still required for every request.
 const bool selectable=allowed&&interaction&&NativeAction128(handle)==0;
 if(allowed){
  fresh.valid=true;fresh.equipped=MuzzleRead58(MuzzleRead58(inventory,0xb4),0xc);
  for(int i=0;i<4;++i){auto& w=fresh.weapons[i];w.wrapper=SelectorSlot290(inventory,i);
   if(!w.wrapper)continue;const UINT ref=MuzzleRead58(w.wrapper,4);w.entity=MuzzleRead58(ref,0xc);
   if(!w.entity||!MuzzleCopy58(ref,&w.id,8)||w.id==~0ULL||!w.id){fresh.valid=false;break;}
   SelectorName290(w.wrapper,w.name);
  }
  if(fresh.valid){Equipped206 observed362;AcquireSRWLockShared(&g_muzzleLock58);observed362=g_equipped206;ReleaseSRWLockShared(&g_muzzleLock58);AuditDualInventory362(fresh,observed362);}
  static unsigned lastEntities[4]={};static unsigned long long lastOwner=0;
  bool changed=lastOwner!=fresh.owner;for(int i=0;i<4;++i)changed=changed||lastEntities[i]!=fresh.weapons[i].entity;
  if(changed){lastOwner=fresh.owner;for(int i=0;i<4;++i){lastEntities[i]=fresh.weapons[i].entity;fc3::Log("[SELECT291] native slot=%d name=%s entity=%08X id=%016llX wrapper=%08X",i+1,fresh.weapons[i].name,fresh.weapons[i].entity,fresh.weapons[i].id,fresh.weapons[i].wrapper);}}
 }
 SelectionRequest290 request;
 AcquireSRWLockExclusive(&g_selectorLock290);g_selectorInventory290=fresh;request=g_selectorRequest290;
 if(request.slot>=0&&!request.sent)g_selectorRequest290.sent=true;
 ReleaseSRWLockExclusive(&g_selectorLock290);
 if(request.slot<0)return;
 if(!selectable){InterlockedExchange(&g_selectorResult290,-1);AcquireSRWLockExclusive(&g_selectorLock290);g_selectorRequest290={};ReleaseSRWLockExclusive(&g_selectorLock290);return;}
 const auto& expected=request.shown.weapons[request.slot];
 bool matches=fresh.valid&&fresh.owner==request.shown.owner;
 for(int i=0;i<4&&matches;++i)matches=fresh.weapons[i].id==request.shown.weapons[i].id&&fresh.weapons[i].entity==request.shown.weapons[i].entity;
 if(!matches||now-request.at>2000){InterlockedExchange(&g_selectorResult290,-1);}
 else if(fresh.equipped==expected.entity){InterlockedExchange(&g_selectorResult290,1);}
 else if(!request.sent){
  // Named ForceDrawWeapon: 6706D0 -> 66E540 -> 198AD60 -> 131C020.
  // It queues an OWNED CWeapon for draw. 290's131CB50 was REMOVE and is forbidden.
  using Draw=void(__stdcall*)(UINT,UINT,UINT);
  __try {reinterpret_cast<Draw>(g_engine+0x131c020)(inventory,fresh.weapons[request.slot].wrapper,0);
   fc3::Log("[SELECT291] native ForceDrawWeapon slot=%d entity=%08X id=%016llX owner=%016llX",request.slot+1,expected.entity,expected.id,fresh.owner);
  }__except(EXCEPTION_EXECUTE_HANDLER){InterlockedExchange(&g_selectorResult290,-1);}
 }
 if(InterlockedCompareExchange(&g_selectorResult290,0,0)){
  fc3::Log("[SELECT291] equipped readback slot=%d expected=%08X actual=%08X result=%ld",request.slot+1,expected.entity,fresh.equipped,g_selectorResult290);
  AcquireSRWLockExclusive(&g_selectorLock290);g_selectorRequest290={};ReleaseSRWLockExclusive(&g_selectorLock290);
 }
}
}
namespace fc3select290 {
void Watch330(bool active){InterlockedExchange(&g_selectorWatch330,active?GetTickCount():0);}
void KeepAlive(bool active){InterlockedExchange(&g_selectorLease290,active?GetTickCount():0);
 if(!active){AcquireSRWLockExclusive(&g_selectorLock290);g_selectorRequest290={};ReleaseSRWLockExclusive(&g_selectorLock290);}}
Inventory Read(){AcquireSRWLockShared(&g_selectorLock290);auto i=g_selectorInventory290;ReleaseSRWLockShared(&g_selectorLock290);
 if(GetTickCount64()-i.at>150)i.valid=false;return i;}
bool Request(const Inventory& shown,int slot){
 if(!shown.valid||slot<0||slot>3||!shown.weapons[slot].entity||GetTickCount64()-shown.at>150)return false;
 AcquireSRWLockExclusive(&g_selectorLock290);const bool idle=g_selectorRequest290.slot<0;
 if(idle){g_selectorRequest290.shown=shown;g_selectorRequest290.slot=slot;g_selectorRequest290.at=GetTickCount64();g_selectorRequest290.sent=false;InterlockedExchange(&g_selectorResult290,0);}
 ReleaseSRWLockExclusive(&g_selectorLock290);return idle;
}
int Result(){return int(InterlockedCompareExchange(&g_selectorResult290,0,0));}
}
