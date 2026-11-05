// Native registration binds "activate_trail" to 112B0780. Unlike activate,
// this handler does not test +234: launch can create a trail on an unlit bottle.
// Suppress only this exact local, never-armed Molotov event, not physics/fire.
constexpr BYTE kTrailBytes176[]={0x80,0xb9,0xcc,0x00,0x00,0x00,0x00,0x74,0x06,0x51,0xe8,0xb1,0xfc,0xff,0xff,0xb0,0x01,0xc2,0x04,0x00};
void* g_trailOriginal176=nullptr;bool g_trailInstalled176=false;
unsigned g_trailBlocked176=0;
using NativeTrail176=bool(__thiscall*)(void*,UINT);
bool BlockMolotovTrail176(UINT component){
 AcquireSRWLockExclusive(&g_manualLock143);
 bool block=false;ULONGLONG local=0,id=0;BYTE active=1;
 if(InterlockedCompareExchange(&g_enabled,0,0)&&LocalIdentity143(local)&&local&&
    Word(component)==g_engine+0x1fc67c0&&ReadAt(component,0x234,&active,1)&&!active){
  const UINT ref=Word(component,4);
  if(ref&&ReadAt(Word(ref,12),8,&id,8)&&id){
   const auto& s=g_safe155;
   block=!s.armed&&s.target.archetype==0x000006f6a4abae8dULL&&s.target.owner==local&&
       s.target.projectile==id&&s.target.projectileRef==ref&&SafeIdentity155(s);
   if(!block)for(auto& item:g_looseSafe156){
    if(!item.id)continue;
    ULONGLONG current=0;if(!ReadAt(Word(item.reference,12),8,&current,8)||current!=item.id){item={};continue;}
    if(item.component==component&&item.reference==ref&&item.id==id&&item.owner==local&&item.archetype==0x000006f6a4abae8dULL){block=true;break;}
   }
  }
 }
 if(block&&g_trailBlocked176!=~0u)++g_trailBlocked176;
 ReleaseSRWLockExclusive(&g_manualLock143);return block;
}
bool __fastcall TrailGate176(void* self,void*,UINT event){
 if(BlockMolotovTrail176(reinterpret_cast<UINT>(self)))return true; // native handler's success/ret4 contract
 return reinterpret_cast<NativeTrail176>(g_trailOriginal176)(self,event);
}
