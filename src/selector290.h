#pragma once
#include <windows.h>
namespace fc3select290 {
struct Weapon {unsigned wrapper=0,entity=0;unsigned long long id=0;char name[80]={};};
struct Inventory {unsigned long long owner=0;ULONGLONG at=0;Weapon weapons[4];unsigned equipped=0;bool valid=false;};
// Native input-thread producer; render/XR only exchange immutable identities.
void KeepAlive(bool active);
void Watch330(bool active); // read-only inventory freshness, never selection permission
Inventory Read();
bool Request(const Inventory& shown,int slot);
int Result(); // 0 pending/idle, 1 exact equipped identity, -1 refused/timeout
}
