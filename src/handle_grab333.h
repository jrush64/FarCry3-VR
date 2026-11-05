#pragma once
#include <windows.h>
namespace fc3grab333 {
enum Intent { None=0, Support=1, Reload=2, Holding=3, WaitRelease=4 };
struct Binding {
 bool mode=false,enabled=false,busy=false,support=false,supportActive=false,reloadNear=false,physicalReload356=false,supportCarry356=false;
 unsigned entity=0,generation=0,epoch=0,reference=0;int profile=-1;
 ULONGLONG at=0;float handle[3]={},foregrip[3]={}; // XR local to owning controller
 struct Action {float point[3]={};float radius=.10f;} actions[6];unsigned actionCount=0;
};
struct Mailbox {SRWLOCK lock=SRWLOCK_INIT;Binding value;};
inline Mailbox& Box(){static Mailbox box;return box;}
inline void Publish(const Binding& b){auto& box=Box();AcquireSRWLockExclusive(&box.lock);box.value=b;ReleaseSRWLockExclusive(&box.lock);}
inline Binding Read(){auto& box=Box();AcquireSRWLockShared(&box.lock);auto b=box.value;ReleaseSRWLockShared(&box.lock);return b;}
}
