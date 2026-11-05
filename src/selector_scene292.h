#pragma once
#include "selector290.h"
#include "locker_probe329.h"
#include "selector_native_capture292.h"
namespace fc3visual292 {
struct WeaponVisual {
 unsigned long long owner=0,id=0;unsigned entity=0;LONG frame=-1;
 std::vector<std::shared_ptr<Part>> parts;
 fc3contact292::Collider collider;
 float fingers[25*12]={};bool ready=false;
};
// Only the render thread reads or mutates visual resources. Native inventory
// requests continue to use291's independently locked, verified identity bridge.
void Target(const fc3select290::Inventory&,int slot);
void Passive295(const fc3select290::Inventory&);
void Preload296(const fc3select290::Inventory&);
const WeaponVisual* Read(const fc3select290::Inventory&,int slot);
void Contact(const float* bodyWrist,int slot,float grip);
void ClearContact();
bool Hidden();
bool Preparing330();
void SetPreparing330(bool active);
void Release();
}
#include "selector_cache296.h"
