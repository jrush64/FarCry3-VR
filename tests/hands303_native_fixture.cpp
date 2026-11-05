// Offline-only native snapshot seam. Production selector code is included
// unchanged; no fixture helper is linked into the candidate DLL.
#include <vector>
#include "../src/aim_probe.cpp"
void SeedInventory303(const fc3select290::Inventory& inventory,int result){
 AcquireSRWLockExclusive(&g_selectorLock290);g_selectorInventory290=inventory;
 if(result){g_selectorRequest290={};InterlockedExchange(&g_selectorResult290,result);}
 ReleaseSRWLockExclusive(&g_selectorLock290);
}
int PendingPreparation330(){AcquireSRWLockShared(&g_selectorLock290);const int slot=g_selectorRequest290.slot;ReleaseSRWLockShared(&g_selectorLock290);return slot;}
bool HasReadOnlyWatch330(){return InterlockedCompareExchange(&g_selectorWatch330,0,0)!=0;}
void SeedFoot357(){g_sample.valid=true;g_sample.vehicle=0;g_sample.slide280=0;g_sample.action128=0;g_sample.ms=GetTickCount64();}
unsigned ExerciseDualAudit362(){
 auto* memory=static_cast<BYTE*>(VirtualAlloc(nullptr,32768,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!memory)return 0;fc3select290::Inventory inv;inv.valid=true;inv.owner=9123;Equipped206 equipped;equipped.valid=true;equipped.ms=GetTickCount64();
 for(int i=0;i<2;++i){auto& w=inv.weapons[i];BYTE* block=memory+i*16384;w.wrapper=UINT(block);w.entity=600+i;w.id=900+i;strcpy_s(w.name,i?"21562d16":"08a08c59");*reinterpret_cast<UINT*>(block+4)=UINT(block+4096);*reinterpret_cast<UINT*>(block+0x48)=UINT(block+8192);*reinterpret_cast<UINT*>(block+4096+0xc)=w.entity;*reinterpret_cast<UINT*>(block+8192+0xcc)=i?7:12;*reinterpret_cast<UINT*>(block+12288+0x48)=w.wrapper;}
 std::vector<BYTE> before(memory,memory+32768);unsigned result=0;
 auto sample=[&](int slot){const auto& w=inv.weapons[slot];equipped.entity=inv.equipped=w.entity;equipped.wrapper=w.wrapper;equipped.weapon=MuzzleRead58(w.wrapper,0x48);equipped.strategy=UINT(memory+slot*16384+12288);equipped.ms=GetTickCount64();AuditDualInventory362(inv,equipped);};
 sample(0);if(g_dualAudit362.seen==1&&!g_dualAudit362.distinct)result|=1;
 sample(1);if(g_dualAudit362.seen==3&&g_dualAudit362.distinct)result|=2;
 if(g_dualAudit362.slots[0].loaded==12&&g_dualAudit362.slots[1].loaded==7)result|=4;
 if(!memcmp(memory,before.data(),before.size()))result|=8;
 *reinterpret_cast<UINT*>(memory+16384+0x48)=UINT(memory+8192);sample(1);if(!g_dualAudit362.distinct)result|=16;
 VirtualFree(memory,0,MEM_RELEASE);AuditDualInventory362(inv,equipped);if(!g_dualAudit362.seen&&!g_dualAudit362.distinct)result|=32;
 strcpy_s(inv.weapons[0].name,"unknown");strcpy_s(inv.weapons[1].name,"unknown");AuditDualInventory362(inv,equipped);if(!g_dualAudit362.seen)result|=64;
 return result;
}
