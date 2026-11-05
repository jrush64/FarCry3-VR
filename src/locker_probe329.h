#pragma once
#include "selector290.h"
#include <cstring>
// Read-only telemetry shared by the existing native and render threads.
// No requests, inventory writes, asset scans, GPU copies or per-frame files.
namespace fc3locker329 {
struct Native {
 fc3select290::Inventory inv;unsigned weapon[4]={},properties[4]={};bool lease=false;int action=-1,seat=-1;unsigned revision=0;
};
inline bool Same(const Native& a,const Native& b){
 if(a.inv.valid!=b.inv.valid||a.inv.owner!=b.inv.owner||a.inv.equipped!=b.inv.equipped||a.lease!=b.lease||a.action!=b.action||a.seat!=b.seat)return false;
 for(int i=0;i<4;++i){const auto& x=a.inv.weapons[i];const auto& y=b.inv.weapons[i];if(x.wrapper!=y.wrapper||x.entity!=y.entity||x.id!=y.id||strcmp(x.name,y.name)||a.weapon[i]!=b.weapon[i]||a.properties[i]!=b.properties[i])return false;}return true;
}
struct Shared {SRWLOCK lock=SRWLOCK_INIT;Native value;};
inline Shared& Store(){static Shared s;return s;}
inline Native Read(){auto& s=Store();AcquireSRWLockShared(&s.lock);auto n=s.value;ReleaseSRWLockShared(&s.lock);return n;}
inline bool Publish(Native& n){auto& s=Store();AcquireSRWLockExclusive(&s.lock);const bool changed=!s.value.revision||!Same(s.value,n);n.revision=s.value.revision+(changed?1u:0u);s.value=n;ReleaseSRWLockExclusive(&s.lock);return changed;}
struct Budget {unsigned used=0;bool Take(unsigned limit){if(used>=limit)return false;++used;return true;}};
struct Sampler {ULONGLONG last=0;bool Due(ULONGLONG now){if(last&&now>=last&&now-last<100)return false;last=now;return true;}};
struct Visual {
 unsigned long long owner=0,id[4]={};unsigned equipped=0,entity[4]={},parts[4]={},rawEntity[4]={};unsigned long long rawId[4]={},modelHash[4]={};
 unsigned nativeRevision=0,ready=0,rawReady=0,refresh=0,tried=0;bool valid=false;int target=-1;
 bool operator==(const Visual& b)const{
  if(nativeRevision!=b.nativeRevision||owner!=b.owner||equipped!=b.equipped||ready!=b.ready||rawReady!=b.rawReady||refresh!=b.refresh||tried!=b.tried||valid!=b.valid||target!=b.target)return false;
  for(int i=0;i<4;++i)if(id[i]!=b.id[i]||entity[i]!=b.entity[i]||parts[i]!=b.parts[i]||rawId[i]!=b.rawId[i]||rawEntity[i]!=b.rawEntity[i]||modelHash[i]!=b.modelHash[i])return false;return true;
 }
};
}
