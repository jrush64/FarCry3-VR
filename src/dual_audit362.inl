// Read-only prerequisites for true dual wielding. No equip/fire/reload calls,
// ammo writes, second cosmetic gun or input remapping occur in this observer.
#include "selector_expected292.h"
struct DualSlot362 {unsigned entity=0,wrapper=0,weapon=0,strategy=0,loaded=0;unsigned long long id=0;bool valid=false,observed=false;};
struct DualAudit362 {SRWLOCK lock=SRWLOCK_INIT;unsigned long long owner=0;DualSlot362 slots[4];unsigned seen=0;bool distinct=false;unsigned reports=0,shots=0;}g_dualAudit362;
bool DualPistol362(const fc3select290::Weapon& weapon){
 const char* body=SelectorExpectedBody292(weapon.name);return body&&(!strcmp(body,"kimber.xbg")||!strcmp(body,"6p9.xbg")||!strcmp(body,"desert_eagle.xbg")||!strcmp(body,"stealthhunter.xbg"));
}
void AuditDualInventory362(const fc3select290::Inventory& inventory,const Equipped206& equipped){
 unsigned pistols=0;for(const auto& w:inventory.weapons)if(w.entity&&DualPistol362(w))++pistols;
 if(!inventory.valid||!inventory.owner||pistols<2){AcquireSRWLockExclusive(&g_dualAudit362.lock);for(auto& s:g_dualAudit362.slots)s={};g_dualAudit362.seen=0;g_dualAudit362.distinct=false;ReleaseSRWLockExclusive(&g_dualAudit362.lock);return;}
 DualSlot362 sampled[4];const auto now=GetTickCount64();
 for(int i=0;i<4;++i){const auto& w=inventory.weapons[i];auto& out=sampled[i];if(!w.entity||!w.wrapper||!DualPistol362(w))continue;
  out.entity=w.entity;out.wrapper=w.wrapper;out.id=w.id;const UINT reference=MuzzleRead58(w.wrapper,4);out.weapon=MuzzleRead58(w.wrapper,0x48);
  out.valid=reference&&MuzzleRead58(reference,0xc)==w.entity&&out.weapon&&out.weapon<=0xffffff2fu&&MuzzleCopy58(out.weapon+0xcc,&out.loaded,4)&&out.loaded<=1000&&MuzzleRead58(w.wrapper,0x48)==out.weapon;
  out.observed=out.valid&&equipped.valid&&equipped.entity==w.entity&&equipped.wrapper==w.wrapper&&equipped.weapon==out.weapon&&inventory.equipped==w.entity&&now>=equipped.ms&&now-equipped.ms<=100&&MuzzleRead58(equipped.strategy,0x48)==w.wrapper;
  if(out.observed)out.strategy=equipped.strategy;
 }
 AcquireSRWLockExclusive(&g_dualAudit362.lock);auto& state=g_dualAudit362;
 if(state.owner!=inventory.owner){state.owner=inventory.owner;for(auto& s:state.slots)s={};}
 state.seen=0;state.distinct=false;
 for(int i=0;i<4;++i){const auto previous=state.slots[i];auto& next=sampled[i];
  if(next.valid&&previous.valid&&next.id==previous.id&&next.entity==previous.entity&&next.wrapper==previous.wrapper&&next.weapon==previous.weapon&&previous.observed){next.observed=true;if(!next.strategy)next.strategy=previous.strategy;}
  const bool changed=next.observed&&(!previous.observed||next.id!=previous.id||next.weapon!=previous.weapon||next.loaded!=previous.loaded);
  state.slots[i]=next;if(next.observed)state.seen|=1u<<i;
  if(changed&&state.reports++<64)fc3::Log("[DUAL362] read-only slot=%d id=%016llX entity=%08X wrapper=%08X nativeWeapon=%08X observedStrategy=%08X loaded=%u nativeEquipped=%08X; independent inactive fire NOT verified",i+1,next.id,next.entity,next.wrapper,next.weapon,next.strategy,next.loaded,inventory.equipped);
 }
 for(int a=0;a<4;++a)for(int b=a+1;b<4;++b)if(state.slots[a].observed&&state.slots[b].observed&&state.slots[a].weapon!=state.slots[b].weapon&&state.slots[a].entity!=state.slots[b].entity)state.distinct=true;
 ReleaseSRWLockExclusive(&g_dualAudit362.lock);
}
void AuditDualShot362(UINT wrapper,UINT entity){
 AcquireSRWLockExclusive(&g_dualAudit362.lock);auto& state=g_dualAudit362;
 for(int i=0;i<4;++i)if(state.slots[i].observed&&state.slots[i].entity==entity&&state.slots[i].wrapper==wrapper&&state.shots++<24){fc3::Log("[DUAL362 SHOT] accepted native bullet entity=%08X wrapper=%08X slot=%d strategy=%08X distinctInstancesObserved=%d; single native equipped fire unchanged",entity,wrapper,i+1,state.slots[i].strategy,state.distinct);break;}
 ReleaseSRWLockExclusive(&g_dualAudit362.lock);
}
